#include <lmgd/device/channel.hpp>

#include <lmgd/device/device.hpp>

#include <sstream>

namespace lmgd::device
{
Channel::SignalCoupling Channel::parse_coupling(const nlohmann::json& json_config)
{
    auto config = json_config["coupling"].get<std::string>();

    if (config == "ACDC" || config == "acdc")
    {
        return SignalCoupling::acdc;
    }
    else if (config == "AC" || config == "ac")
    {
        raise("AC coupling not implemented yet.");
    }
    else if (config == "GND" || config == "gnd")
    {
        raise("GND coupling not implemented yet.");
    }

    raise("Unknown coupling requested: ", config,
          " for channel: ", json_config["name"].get<std::string>());
}

Channel::MetricSetType Channel::parse_metrics(const nlohmann::json& config)
{
    if (config["metrics"].size() == 0)
    {
        Log::info() << "No metrics given for channel: " << config["name"].get<std::string>();
    }

    Channel::MetricSetType metrics;

    for (auto& metric_config : config["metrics"])
    {
        auto metric_string = metric_config.get<std::string>();

        std::stringstream str;
        str << metric_string;

        std::string metric;
        std::getline(str, metric, '@');

        MetricType metric_type;

        if (metric == "power")
        {
            metric_type = MetricType::power;
        }
        else if (metric == "voltage")
        {
            metric_type = MetricType::voltage;
        }
        else if (metric == "current")
        {
            metric_type = MetricType::current;
        }
        else
        {
            raise("Got unknown metric type '", metric_string, "' for channel ",
                  config["name"].get<std::string>());
        }

        std::string bandwidth;
        std::getline(str, bandwidth);

        MetricBandwidth metric_bandwidth;

        if (bandwidth == "narrow" || bandwidth == "")
        {
            metric_bandwidth = MetricBandwidth::narrow;
        }
        else if (bandwidth == "wide")
        {
            metric_bandwidth = MetricBandwidth::wide;
        }
        else
        {
            raise("Got unknown bandwidth '", metric_string, "' for channel ",
                  config["name"].get<std::string>());
        }

        auto res = metrics.emplace(metric_type, metric_bandwidth);
        if (!res.second)
        {
            Log::warn() << "Metric " << metric_string << " has already been added to the channel'"
                        << config["name"].get<std::string>() << "'. Ignoring.";
        }
    }

    return metrics;
}

Channel::Channel(Device& device, int id, const std::string& name, MetricSetType metrics,
                 Channel::SignalCoupling coupling, float current_range, float voltage_range)
: connection_(*(device.connection_)), id_(id), name_(name), metrics_(metrics), coupling_(coupling),
  current_range_(current_range), voltage_range_(voltage_range)
{
    // setup channel on lmg device

    // first, check if the channel exists
    connection_.send_command("CTYP" + std::to_string(id_) + "?");

    assert(0 < id_ && id_ < 8);
    // TODO this can hang, if the suffix id_ is invalid.
    // (in case of LMG670, valid ids are: 0 < id_ < 8)
    auto channel_type = connection_.read_ascii();

    if (channel_type == "\"\"")
    {
        raise("Can't configure invalid LMG device channel ", id_);
    }
    Log::info() << "Device channel " << id_ << " is a " << channel_type;

    connection_.check_command();

    connection_.check_command("SCPL" + std::to_string(id_) + " " +
                              std::to_string(static_cast<int>(coupling_)));

    connection_.check_command("IAUTO" + std::to_string(id_) + " 0");
    connection_.check_command("IRNG" + std::to_string(id_) + " " + std::to_string(current_range_));

    connection_.check_command("UAUTO" + std::to_string(id_) + " 0");
    connection_.check_command("URNG" + std::to_string(id_) + " " + std::to_string(voltage_range_));

    for (auto metric : metrics_)
    {
        device.add_track(*this, metric.first, metric.second);
    }
}

Channel::Channel(device::Device& device, int id, const nlohmann::json& config)
: Channel(device, id, config["name"].get<std::string>(), parse_metrics(config),
          parse_coupling(config), config["current_range"].get<float>(),
          config["voltage_range"].get<double>())
{
}

const std::string& Channel::name() const
{
    return name_;
}

Channel::SignalCoupling Channel::coupling() const
{
    return coupling_;
}

float Channel::current_range() const
{
    return current_range_;
}

float Channel::voltage_range() const
{
    return voltage_range_;
}

const Channel::MetricSetType& Channel::metrics() const
{
    return metrics_;
}

int Channel::id() const
{
    return id_;
}

} // namespace lmgd::device
