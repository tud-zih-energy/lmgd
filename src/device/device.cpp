#include <lmgd/device/device.hpp>

#include <lmgd/network/async_binary_line_reader.hpp>
#include <lmgd/network/connection.hpp>

#include <lmgd/except.hpp>
#include <lmgd/log.hpp>

#include <dataheap2/source_metric.hpp>

#include <nitro/jiffy/jiffy.hpp>
#include <nitro/lang/enumerate.hpp>

#include <cassert>
#include <sstream>
#include <string>

namespace lmgd::device
{

Device::Device(asio::io_service& io_service, const nlohmann::json& config) : io_service_(io_service)
{
    if (config["measurement"]["device"]["connection"].get<std::string>() != "socket")
    {
        raise("Sorry, I can only connect over sockets to my LMG device :(");
    }

    connection_ = std::make_unique<network::Connection>(
        io_service_, config["measurement"]["device"]["address"].get<std::string>());

    connection_->send_command("ERRALL?");
    Log::debug() << "Error log before:" << connection_->read_ascii();

    // just in case...
    connection_->send_command("CONT OFF");

    // check timestamps
    {
        connection_->send_command("SYSDATE?");
        auto old_device_time = connection_->read_ascii();

        auto now = nitro::jiffy::Jiffy();
        connection_->check_command("SYSDATE " + now.format("%Y:%m:%dD%H:%M:%S.%f"));

        connection_->send_command("SYSDATE?");
        auto new_device_time = connection_->read_ascii();

        Log::info() << "Adjusting device clock: (local) " << now << ", (old) " << old_device_time
                    << ", (new) " << new_device_time;
    }

    // read number of available channels on device
    std::size_t num_channels = 0;
    {
        connection_->send_command("GROUP?");
        auto grouping = connection_->read_ascii();

        std::stringstream str;
        str << grouping;

        std::string group;

        while (std::getline(str, group, ','))
        {
            num_channels += std::stoll(group);
        }
    }
    Log::info() << "Number of available device channels: " << num_channels;
    // we need this, so the vector won't reallocate, which would invalidate the references in Track
    // instances.
    channels_.reserve(std::max(config["channels"].size(), num_channels));

    for (auto channel : nitro::lang::enumerate(config["channels"]))
    {
        channels_.emplace_back(*this, channel.index() + 1, channel.value());
    }

    // check if number of channels match with what the device says
    if (channels_.size() != num_channels)
    {
        // Thomas, DO NOT comment that out, because it will mess up grouping. Fix the fucking
        // config! lmgd::device::Track also assumes exactly one group. Keep it alone.
        raise("This device has exactly ", num_channels, " channels, but ", channels_.size(),
              " are configured.");
    }

    // set grouping -> all channels in one group
    // this is technically equivalent to "GROUP {num_channels}"
    connection_->check_command("GROUP " + std::to_string(channels_.size()));

    // enable DualPath mode, so we can record metrics narrow and wide at the same time
    connection_->check_command("PROC 1");

    // set sampling rate
    connection_->check_command("GLCSR " +
                               std::to_string(config["measurement"]["sampling_rate"].get<int>()));

    // activate scope mode
    connection_->check_command("CYCLMOD SCOPE");

    // disable zero supression
    connection_->check_command("ZSUP 0");

    // no one knows, what this is for, but it is part of the example ¯\_(ツ)_/¯
    connection_->check_command("INIM");

    connection_->send_command("GLPTLEN?");
    gap_length_ = std::stoi(connection_->read_ascii());

    connection_->send_command("GLPSR?");
    sampling_rate_ = std::stof(connection_->read_ascii());

    Log::debug() << "gap_length: " << gap_length_ << ", sampling_rate: " << sampling_rate_;

    // build action command, which gets execute during the following CONT ON
    // Immortal quote: "Die Dokumentation wird noch angepasst"
    // TSCYCL is the timestamp of the cycle
    // DURCYCL I have no fucking idea. Good luck with that.
    std::string action = "ACTN; TSCYCL?; DURCYCL?";

    // each channel only has the one track -> power
    for (auto i = 0u; i < tracks_.size(); i++)
    {
        action +=
            "; GLPVAL? " + std::to_string(i) + ", (0:" + std::to_string(gap_length_ - 1) + ")";
    }

    connection_->send_command(action);

    // switch to binary mode
    connection_->mode(lmgd::network::Connection::Mode::binary);
}

Device::~Device()
{
    if (recording_)
    {
        stop_recording();
    }
}

void Device::start_recording()
{
    Log::info() << "Start recording";
    connection_->send_command("CONT ON");
    recording_ = true;
}

void Device::stop_recording()
{
    Log::info() << "Stop recording";
    connection_->send_command("CONT OFF");
    recording_ = false;
}

void Device::add_track(const Channel& channel, MetricType type, MetricBandwidth bandwidth)
{
    if (tracks_.size() >= 16)
    {
        raise("Trying to add more than 16 tracks, which is not supported.");
    }

    tracks_.emplace_back(channel, tracks_.size(), type, bandwidth);
    connection_->check_command(tracks_.back().get_glctrac_command());
}

const std::vector<Track>& Device::get_tracks() const
{
    return tracks_;
}

void Device::fetch_data(network::Connection::Callback cb)
{
    Log::debug() << "Device::fetch_data";

    connection_->read_binary_async(cb);
}

} // namespace lmgd::device
