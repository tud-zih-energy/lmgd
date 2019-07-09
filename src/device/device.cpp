#include <lmgd/device/device.hpp>

#include <lmgd/network/connection.hpp>

#include <lmgd/except.hpp>
#include <lmgd/log.hpp>

#include <nitro/lang/enumerate.hpp>

#include <date/date.h>
#include <date/tz.h>

#include <regex>
#include <sstream>
#include <string>

#include <cassert>
#include <cmath>

namespace lmgd::device
{

Device::Device(asio::io_service& io_service, const nlohmann::json& config) : io_service_(io_service)
{
    if (config.at("measurement").at("device").at("connection").get<std::string>() == "serial")
    {
        connection_ = std::make_unique<network::Connection>(
            io_service_,
            network::Connection::Type::serial,
            config.at("measurement").at("device").at("port").get<std::string>());
    }
    else if (config.at("measurement").at("device").at("connection").get<std::string>() == "socket")
    {
        connection_ = std::make_unique<network::Connection>(
            io_service_,
            network::Connection::Type::socket,
            config.at("measurement").at("device").at("address").get<std::string>());
    }
    else
    {
        raise("Sorry, I can only connect over network or serial to my LMG device :(");
    }

    if (config.at("measurement").at("mode").get<std::string>() == "cycle")
    {
        mode_ = MeasurementMode::cycle;
    }
    else if (config.at("measurement").at("mode").get<std::string>() == "gapless")
    {
        mode_ = MeasurementMode::gapless;
    }
    else
    {
        raise(
            "Requested unknown measurement mode: " +
            config.at("measurement").at("mode").get<std::string>());
    }

    connection_->send_command(":SYST:ERR:ALL?");
    Log::debug() << "Error log before:" << connection_->read_ascii();

    // just in case...
    connection_->send_command(":INIT:CONT OFF");

    check_serial_number(config);

    // check timestamps
    if (mode_ == MeasurementMode::gapless)
    {
        connection_->send_command(":SYST:DATE?");
        auto old_device_time = connection_->read_ascii();

        auto now = date::make_zoned(
            date::current_zone(),
            date::floor<std::chrono::nanoseconds>(std::chrono::system_clock::now()));
        connection_->check_command(":SYST:DATE " + date::format("%Y:%m:%dD%H:%M:%S", now));

        connection_->send_command("SYST:DATE?");
        auto new_device_time = connection_->read_ascii();

        Log::info() << "Adjusting device clock: (local) " << now << ", (old) " << old_device_time
                    << ", (new) " << new_device_time;
    }

    // read number of available channels on device from config
    std::size_t num_channels = config.at("measurement").at("device").at("num_channels");
    Log::info() << "Number of available device channels: " << num_channels;

    // we need this, so the vector won't reallocate, which would invalidate the references in Track
    // instances.
    channels_.reserve(std::max(config.at("channels").size(), num_channels));

    for (auto channel : nitro::lang::enumerate(config.at("channels")))
    {
        channels_.emplace_back(*this, channel.index() + 1, channel.value());
    }

    // check if number of channels match with what the device says
    if (channels_.size() != num_channels)
    {
        // Thomas, DO NOT comment that out, because it will mess up grouping. Fix the fucking
        // config! lmgd::device::Track also assumes exactly one group. Keep it alone.
        raise(
            "This device has exactly ",
            num_channels,
            " channels, but ",
            channels_.size(),
            " are configured.");
    }

    // make sure we have at least one channel
    if (channels_.empty())
    {
        raise("There are no channels in the config. Check your setup!");
    }

    // TODO while each channel can have a different coupling in the config, coupling is defined per
    // group. Therefore, only the newer LMG670 can support mixed coupling setups. However, this is
    // not supported right now. We only set the coupling here once to whatever the first channel has
    // as coupling according to the config
    connection_->check_command(
        ":INP:COUP " + std::to_string(static_cast<int>(channels_.front().coupling())));

    {
        // These commands are only available on the newer 670 device, we send them to the older
        // devices to, but we will ignore any errors.

        // First, check for any errors, so they don't get lost
        connection_->check_command();

        // set grouping to only one group comprising all channels
        connection_->send_command(":SENS:GRO:LIST " + std::to_string(num_channels));

        // enable DualPath mode, so we can record metrics narrow and wide at the same time
        connection_->send_command(":SENS:FILT:PROC 1");

        // ignore any errors
        connection_->send_command(":SYST:ERR:ALL?");
        (void)connection_->read_ascii();
    }

    // disable zero supression
    connection_->check_command(":SENS:ZPR 0");

    if (mode_ == MeasurementMode::gapless)
    {
        // set sampling rate
        connection_->check_command(
            ":SENS:GAPL:SRAT " +
            std::to_string(config.at("measurement").at("sampling_rate").get<int>()));

        // activate scope mode, in scope mode we can read the gapless values
        connection_->check_command(":SENS:SWE:MOD SCOPE");

        // Technically, this triggers the next measurement cycle.
        // But no one knows, why and if we need it, but it is part of the example ¯\_(ツ)_/¯
        connection_->check_command(":INIT:IMM");

        // read lenght of one gapless data block
        connection_->send_command(":FETC:SCOP:GAPL:TLEN?");
        gap_length_ = std::stoi(connection_->read_ascii());

        // read actual sampling rate, might differ from the requested one in the config
        // In contrast to the documentation, using the shorter but equal command
        // ":FETC:SCOP:GAPL:SRAT?" yields a totally different result. LUL WUT ¯\_(ツ)_/¯
        connection_->send_command(":FETC:SCOP:GAPL:SRATE?");
        sampling_rate_ = std::stof(connection_->read_ascii());

        Log::debug() << "gap_length: " << gap_length_ << ", sampling_rate: " << sampling_rate_;

        // build action command, which gets execute during the following CONT ON
        // Immortal quote: "Die Dokumentation wird noch angepasst"
        // TSCYCL is the timestamp of the cycle
        // DURCYCL I have no fucking idea. Good luck with that.
        std::string action = "ACTN;TSCYCL?;DURCYCL?";

        // each channel only has the one track -> power
        for (auto i = 0u; i < tracks_.size(); i++)
        {
            action +=
                "; GLPVAL? " + std::to_string(i) + ", (0:" + std::to_string(gap_length_ - 1) + ")";
        }

        // TODO fix this SHORT syntax command
        // We need SHORT syntax for this command, because TSCYCL and DURCYCL aren't documented and
        // I simply don't know the equivalent SCPI command. FeelsBadMan
        connection_->send_command("*zlang short");
        connection_->send_command(action);
        connection_->send_command("*zlang scpi");

        // Finally, let's check the error log if anything went wrong
        connection_->check_command();
    }
    else
    {
        auto sampling_interval = 1. / config.at("measurement").at("sampling_rate").get<int>();

        // We MUST use stringstream here, as std::to_string writes the interval in a different
        // format, which is not accepted by the device :(
        // stringstream: 0.05
        // to_string   : 0.05000
        std::stringstream str;
        str << sampling_interval;

        connection_->check_command(":SENS:SWE:TIME " + str.str());
        connection_->send_command(":SENS:SWE:TIME?");
        sampling_interval = std::stof(connection_->read_ascii());
        sampling_rate_ = std::round(1. / sampling_interval);
        Log::debug() << "Sampling rate: " << sampling_rate_;
    }
} // namespace lmgd::device

Device::~Device()
{
    if (recording_)
    {
        stop_recording();
    }
}

void Device::start_recording(lmgd::network::Connection::Mode mode)
{
    if (mode_ == MeasurementMode::cycle)
    {
        // build action command, which gets execute during the following CONT ON
        // Note: There is no timestamp available on the device, so we have to take them ourselves :(
        std::string action = ":TRIG:ACT;";
        for (const auto& track : tracks_)
        {
            action += track.get_action_command(mode_) + ";";
        }

        connection_->send_command(action);
        connection_->check_command();
    }

    if (mode == lmgd::network::Connection::Mode::binary)
    {
        // switch to binary mode
        connection_->mode(lmgd::network::Connection::Mode::binary);
    }

    Log::info() << "Start recording";
    connection_->send_command(":INIT:CONT ON");
    recording_ = true;
}

void Device::stop_recording()
{
    Log::info() << "Stop recording";
    // *OPC? is added for the MeasurementMode::cycle - Otherwise there wouldn't be a final marker
    // to detect the end of the datastream from the lmg device.
    connection_->send_command(":INIT:CONT OFF;*opc?");
    recording_ = false;
}

void Device::add_track(const Channel& channel, MetricType type, MetricBandwidth bandwidth)
{
    if (tracks_.size() >= 16)
    {
        raise("Trying to add more than 16 tracks, which is not supported.");
    }

    tracks_.emplace_back(channel, tracks_.size(), type, bandwidth);
    if (mode_ == MeasurementMode::gapless)
    {
        connection_->check_command(tracks_.back().get_action_command(mode_));
    }
}

const std::vector<Track>& Device::get_tracks() const
{
    return tracks_;
}

void Device::fetch_binary_data(network::BinaryCallback cb)
{
    Log::debug() << "Device::fetch_binary_data";

    connection_->read_binary_async(cb);
}

void Device::fetch_data(network::Callback cb)
{
    Log::debug() << "Device::fetch_data";

    connection_->read_async(cb);
}

void Device::check_serial_number(const nlohmann::json& config)
{
    connection_->send_command("*idn?");
    auto idn = connection_->read_ascii();

    std::regex reg("^([^,]+),([^,]+),([^,]+),([^,]+)$");
    std::smatch match;

    if (!std::regex_match(idn, match, reg))
    {
        raise("Cannot parse the identification received from device: ", idn);
    }

    Log::debug() << "Vendor: " << match[1];
    Log::debug() << "Device: " << match[2];
    Log::debug() << "Serial: " << match[3];
    Log::debug() << "Firmware: " << match[4];

    if (config.at("measurement").at("device").at("serial").get<std::string>() != match[3])
    {
        raise(
            "I'm connected to a different device, check your damn config. (device: ",
            match[3],
            "; config: ",
            config.at("measurement").at("device").at("serial").get<std::string>(),
            ")");
    }
}

} // namespace lmgd::device
