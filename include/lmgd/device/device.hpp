#pragma once

#include <lmgd/device/channel.hpp>
#include <lmgd/device/track.hpp>
#include <lmgd/network/connection.hpp>

#include <nlohmann/json.hpp>

#include <asio/io_service.hpp>

#include <memory>
#include <vector>

namespace lmgd::device
{
class Device
{
public:
    Device(asio::io_service& io_service, const nlohmann::json& config);

    ~Device();

public:
    void start_recording(lmgd::network::Connection::Mode mode);
    void stop_recording();
    void fetch_binary_data(network::BinaryCallback);
    void fetch_data(network::Callback);

    const std::vector<Track>& get_tracks() const;

    MeasurementMode measurement_mode() const
    {
        return mode_;
    }

    double sampling_rate() const
    {
        return sampling_rate_;
    }

    int64_t gap_length() const
    {
        return gap_length_;
    }

private:
    void add_track(const Channel& channel, MetricType type, MetricBandwidth bandwidth);
    void check_serial_number(const nlohmann::json& config);

    friend class Channel;

private:
    asio::io_service& io_service_;
    std::string name_;
    std::unique_ptr<lmgd::network::Connection> connection_;
    std::vector<Channel> channels_;
    std::vector<Track> tracks_;
    MeasurementMode mode_;

    bool recording_;

    int64_t gap_length_;
    double sampling_rate_;
};
} // namespace lmgd::device
