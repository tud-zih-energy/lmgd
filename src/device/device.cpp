#include <lmgd/device/device.hpp>

#include <lmgd/network/connection.hpp>

#include <lmgd/except.hpp>
#include <lmgd/log.hpp>

#include <nitro/lang/enumerate.hpp>

#include <dataheap2/source_metric.hpp>

#include <cassert>
#include <string>

namespace std
{
template <typename T>
inline std::ostream& operator<<(std::ostream& s, const std::vector<T>& v)
{
    for (auto t : v)
    {
        s << t << ",";
    }

    return s;
}
} // namespace std

namespace lmgd::device
{

Device::Device(const nlohmann::json& config)
{
    if (config["measurement"]["device"]["connection"].get<std::string>() != "socket")
    {
        raise("Sorry, I can only connect over sockets to my LMG device :(");
    }

    connection_ = std::make_unique<network::Connection>(
        config["measurement"]["device"]["address"].get<std::string>());

    for (auto channel : nitro::lang::enumerate(config["channels"]))
    {
        channels_.emplace_back(*connection_, channel.index() + 1, channel.value());
    }

    // just in case...
    connection_->send_command("CONT OFF");

    // set grouping -> all channels in one group
    // TODO this explodes if not all channels are used :(
    assert(channels_.size() == 6);
    connection_->check_command("GROUP " + std::to_string(channels_.size()));

    // set sampling rate
    connection_->check_command("GLCSR " +
                               std::to_string(config["measurement"]["samping_rate"].get<int>()));

    // activate scope mode
    connection_->check_command("CYCLMOD SCOPE");

    // no one knows, why this is for, but it is part of the example ¯\_(ツ)_/¯
    connection_->check_command("INIM");

    connection_->send_command("GLPTLEN?");
    gap_length_ = std::stoi(connection_->read_ascii());

    connection_->send_command("GLPSR?");
    sampling_rate_ = std::stof(connection_->read_ascii());

    Log::debug() << "gap_length: " << gap_length_ << ", sampling_rate: " << sampling_rate_;

    // build action command, which gets execute during the following CONT ON
    // TSSP - timestamp
    std::string action = "ACTN; TSSP?";
    // each channel only has the one track -> power
    for (auto i = 0u; i < channels_.size(); i++)
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
    connection_->send_command("CONT OFF");

    // god knows why
    connection_->mode(lmgd::network::Connection::Mode::ascii);
}

void Device::start_recording()
{
    Log::info() << "Starting recording";
    connection_->send_command("CONT ON");
}

std::vector<std::string> Device::get_metrics() const
{
    std::vector<std::string> res;

    for (auto& channel : channels_)
    {
        res.push_back(channel.name() + ".power");
    }

    return res;
}

void Device::fetch_data(std::vector<std::reference_wrapper<dataheap2::SourceMetric>>& metrics)
{
    auto data = connection_->read_binary();

    Log::info() << data.size();

    if (data.size() == 1)
    {
        Log::warn() << "Aliens approaching earth.";
        return; // XXX buffer with just '1' at the end... hmmm
    }

    auto start_time_ns = data.read_date();

    for (auto i = 0u; i < channels_.size(); i++)
    {
        auto& metric = metrics[i];

        auto list = data.read_float_list();

        for (auto entry : nitro::lang::enumerate(list))
        {
            auto time_ns =
                start_time_ns +
                std::chrono::nanoseconds(int64_t(entry.index() * 1000000000l / sampling_rate_));
            metric.get().send({ dataheap2::TimePoint(time_ns.time_since_epoch()), entry.value() });
            if (entry.index() == 0)
            {
                Log::info() << time_ns.time_since_epoch().count() << " : " << entry.value() << " W";
            }
        }
    }
}

} // namespace lmgd::device
