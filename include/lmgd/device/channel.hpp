#pragma once

#include <lmgd/network/connection.hpp>

#include <lmgd/except.hpp>
#include <lmgd/log.hpp>

#include <nlohmann/json.hpp>

#include <set>
#include <string>
#include <vector>

#include <cassert>

namespace lmgd::device
{
class Device;

class Channel
{
public:
    enum class SignalCoupling : int
    {
        acdc = 0,
        ac = 1,
        gnd = 9
    };

    enum class MetricType
    {
        voltage,
        current,
        power
    };

    enum class MetricBandwidth : int
    {
        narrow = 1,
        wide = 2
    };

    using MetricSetType = std::set<std::pair<Channel::MetricType, Channel::MetricBandwidth>>;

private:
    SignalCoupling parse_coupling(const nlohmann::json& json_config);

    MetricSetType parse_metrics(const nlohmann::json& config);

    Channel(Device& device, int id, const std::string& name, MetricSetType metrics,
            Channel::SignalCoupling coupling, float current_range, float voltage_range);

public:
    Channel(device::Device& device, int id, const nlohmann::json& config);

public:
    const std::string& name() const;

    SignalCoupling coupling() const;

    float current_range() const;

    float voltage_range() const;

    const MetricSetType& metrics() const;

    int id() const;

private:
    network::Connection& connection_;
    int id_;
    std::string name_;
    MetricSetType metrics_;
    SignalCoupling coupling_;
    float current_range_;
    float voltage_range_;
};
} // namespace lmgd::device
