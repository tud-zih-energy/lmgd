#pragma once

#include <lmgd/device/channel.hpp>
#include <lmgd/network/connection.hpp>

#include <nlohmann/json.hpp>

#include <asio/io_service.hpp>

#include <memory>
#include <vector>

namespace dataheap2
{
class SourceMetric;
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
    void stop_recording();
    void fetch_data(network::Connection::Callback);

    const std::vector<std::string>& get_tracks() const;

private:
    void add_track(const Channel& channel, Channel::MetricType type,
                   Channel::MetricBandwidth bandwidth);

    friend class Channel;

private:
    asio::io_service& io_service_;
    std::string name_;
    std::unique_ptr<lmgd::network::Connection> connection_;
    std::vector<Channel> channels_;
    std::vector<std::string> tracks_;

    bool recording_;

    int64_t gap_length_;
    double sampling_rate_;
};
} // namespace lmgd::device
