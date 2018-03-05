#pragma once

#include <lmgd/device/channel.hpp>

#include <nlohmann/json.hpp>

#include <asio/io_service.hpp>

#include <memory>
#include <vector>

namespace dataheap2
{
class SourceMetric;
}

namespace lmgd::network
{
class Connection;
}

namespace lmgd::device
{
class Device
{
public:
    Device(asio::io_service& io_service, const nlohmann::json& config);

    ~Device();

public:
    void start_recording();
    void fetch_data(std::vector<std::reference_wrapper<dataheap2::SourceMetric>>&);

    const std::vector<std::string>& get_tracks() const;

private:
    void add_track(const Channel& channel, Channel::MetricType type);

    friend class Channel;

private:
    asio::io_service& io_service_;
    std::string name_;
    std::unique_ptr<lmgd::network::Connection> connection_;
    std::vector<Channel> channels_;
    std::vector<std::string> tracks_;

    int64_t gap_length_;
    double sampling_rate_;
};
} // namespace lmgd::device
