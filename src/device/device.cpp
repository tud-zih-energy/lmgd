#include <lmgd/device/device.hpp>

#include <lmgd/network/connection.hpp>

#include <lmgd/except.hpp>
#include <lmgd/log.hpp>

#include <nitro/lang/enumerate.hpp>

#include <dataheap2/source_metric.hpp>

#include <cassert>
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

    for (auto channel : nitro::lang::enumerate(config["channels"]))
    {
        channels_.emplace_back(*this, channel.index() + 1, channel.value());
    }

    // set grouping -> all channels in one group
    // TODO this explodes if not all channels are used :(
    assert(channels_.size() == 6);
    connection_->check_command("GROUP " + std::to_string(channels_.size()));

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

void Device::add_track(const Channel& channel, Channel::MetricType type)
{

    auto glctrac = [this](int track_id, int phase, char type) {
        this->connection_->check_command("GLCTRAC " + std::to_string(track_id) + ", \"" + type +
                                         "1" + std::to_string(phase) + "21\"");
    };

    switch (type)
    {
    case Channel::MetricType::voltage:
        glctrac(tracks_.size(), channel.id(), 'U');
        tracks_.push_back(channel.name() + ".voltage");

        break;
    case Channel::MetricType::current:
        glctrac(tracks_.size(), channel.id(), 'I');
        tracks_.push_back(channel.name() + ".current");
        break;
    case Channel::MetricType::power:
        glctrac(tracks_.size(), channel.id(), 'P');
        tracks_.push_back(channel.name() + ".power");
        break;
    default:
        raise("WHUUUUUT. HALP. I'M TRAPPED HERE.");
    }
}

const std::vector<std::string>& Device::get_tracks() const
{
    return tracks_;
}

void Device::fetch_data(std::vector<std::reference_wrapper<dataheap2::SourceMetric>>& metrics)
{
    auto data = connection_->read_binary();

    if (data.size() == 1)
    {
        // XXX buffer with just '1' at the end... hmmm
        raise("Aliens approaching earth. I don't know what this means :(");
    }

    auto cycle_start = data.read_date();
    auto cycle_duration = data.read_time();

    for (auto& metric : metrics)
    {

        auto list = data.read_float_list();

        for (auto entry : nitro::lang::enumerate(list))
        {
            auto time_ns = cycle_start + entry.index() * cycle_duration / list.size();
            metric.get().send({ dataheap2::TimePoint(time_ns.time_since_epoch()), entry.value() });
        }
    }
}

} // namespace lmgd::device
