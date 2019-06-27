#include <lmgd/device/channel.hpp>

#include <lmgd/device/device.hpp>

#include <sstream>

namespace lmgd::device
{
ChannelSignalCoupling Channel::parse_coupling(const nlohmann::json& json_config)
{
    auto config = json_config["coupling"].get<std::string>();

    if (config == "ACDC" || config == "acdc")
    {
        return ChannelSignalCoupling::acdc;
    }
    else if (config == "AC" || config == "ac")
    {
        return ChannelSignalCoupling::ac;
    }
    else if (config == "GND" || config == "gnd")
    {
        raise("GND coupling not implemented yet.");
    }

    raise("Unknown coupling requested: ", config,
          " for channel: ", json_config["name"].get<std::string>());
}

Channel::MetricSetType Channel::parse_metrics(const nlohmann::json& config, MeasurementMode mode)
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

        if (metric == "voltage")
        {
            metric_type = MetricType::voltage;
        }
        else if (metric == "voltage_min")
        {
            metric_type = MetricType::voltage_min;
        }
        else if (metric == "voltage_max")
        {
            metric_type = MetricType::voltage_max;
        }
        else if (metric == "voltage_crest")
        {
            metric_type = MetricType::voltage_crest;
        }
        else if (metric == "current")
        {
            metric_type = MetricType::current;
        }
        else if (metric == "current_min")
        {
            metric_type = MetricType::current_min;
        }
        else if (metric == "current_max")
        {
            metric_type = MetricType::current_max;
        }
        else if (metric == "current_crest")
        {
            metric_type = MetricType::current_crest;
        }
        else if (metric == "power" || metric == "active_power")
        {
            metric_type = MetricType::power;
        }
        else if (metric == "apparent" || metric == "apparent_power")
        {
            metric_type = MetricType::apparent_power;
        }
        else if (metric == "reactive" || metric == "reactive_power")
        {
            metric_type = MetricType::reactive_power;
        }
        else if (metric == "phase" || metric == "phi")
        {
            metric_type = MetricType::phi;
        }
        else
        {
            raise("Got unknown metric type '", metric_string, "' for channel ",
                  config["name"].get<std::string>());
        }

        // Gapless mode can only record power, current, and voltage
        if (mode == MeasurementMode::gapless)
        {
            if (metric_type != MetricType::voltage && metric_type != MetricType::current &&
                metric_type != MetricType::power)
            {
                raise("The metric '", metric_string,
                      "' cannot be recorded in gapless mode for channel ",
                      config["name"].get<std::string>());
            }
        }

        std::string bandwidth;
        std::getline(str, bandwidth);

        MetricBandwidth metric_bandwidth;

        if (mode == MeasurementMode::cycle)
        {
            metric_bandwidth = MetricBandwidth::cycle;
        }
        else if (bandwidth == "")
        {
            metric_bandwidth = MetricBandwidth::wide;
        }
        else if (bandwidth == "narrow")
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
            Log::warn() << "Metric " << metric_string << " has already been added to the channel '"
                        << config["name"].get<std::string>() << "'. Ignoring.";
        }
    }

    return metrics;
}

Channel::Channel(Device& device, int id, const std::string& name, MetricSetType metrics,
                 ChannelSignalCoupling coupling, float current_range, float voltage_range)
: connection_(*(device.connection_)), id_(id), name_(name), metrics_(metrics), coupling_(coupling),
  current_range_(current_range), voltage_range_(voltage_range)
{
    // setup channel on lmg device

    // we have to trust config to provide valid channels
    // first, check if the channel exists
    // connection_.send_command("CTYP" + std::to_string(id_) + "?");
    // assert(0 < id_ && id_ < 8);
    // TODO this can hang, if the suffix id_ is invalid.
    // (in case of LMG670, valid ids are: 0 < id_ < 8)
    // auto channel_type = connection_.read_ascii();

    // if (channel_type == "\"\"")
    // {
    //     raise("Can't configure invalid LMG device channel ", id_);
    // }
    // Log::info() << "Device channel " << id_ << " is a " << channel_type;

    connection_.check_command();

    connection_.check_command(":SENS:CURR:RANG:AUTO" + std::to_string(id_) + " 0");
    connection_.check_command(":SENS:CURR:RANG" + std::to_string(id_) + " " +
                              std::to_string(current_range_));

    connection_.check_command(":SENS:VOLT:RANG:AUTO" + std::to_string(id_) + " 0");
    connection_.check_command(":SENS:VOLT:RANG" + std::to_string(id_) + " " +
                              std::to_string(voltage_range_));

    for (auto metric : metrics_)
    {
        device.add_track(*this, metric.first, metric.second);
    }
}

Channel::Channel(device::Device& device, int id, const nlohmann::json& config)
: Channel(device, id, config["name"].get<std::string>(),
          parse_metrics(config, device.measurement_mode()), parse_coupling(config),
          config["current_range"].get<float>(), config["voltage_range"].get<double>())
{
}

const std::string& Channel::name() const
{
    return name_;
}

ChannelSignalCoupling Channel::coupling() const
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
