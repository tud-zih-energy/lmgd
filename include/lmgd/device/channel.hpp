#pragma once

#include <lmgd/network/connection.hpp>

#include <lmgd/except.hpp>
#include <lmgd/log.hpp>

#include <nlohmann/json.hpp>

#include <string>

#include <cassert>

namespace lmgd::device
{
class Channel
{
public:
    enum class SignalCoupling : int
    {
        acdc = 0,
        ac = 1,
        gnd = 9
    };

private:
    SignalCoupling parse_coupling(const std::string& config)
    {
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

        raise("Unknown coupling requested: ", config);
    }

public:
    Channel(network::Connection& connection, int id, const std::string& name,
            SignalCoupling coupling, float current_range, float voltage_range)
    : connection_(connection), id_(id), name_(name), coupling_(coupling),
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
        connection_.check_command("IRNG" + std::to_string(id_) + " " +
                                  std::to_string(current_range_));

        connection_.check_command("UAUTO" + std::to_string(id_) + " 0");
        connection_.check_command("URNG" + std::to_string(id_) + " " +
                                  std::to_string(voltage_range_));

        connection_.check_command("GLCTRAC " + std::to_string(id_ - 1) + ", \"P1" +
                                  std::to_string(id_) + "21\"");
    }

    Channel(network::Connection& connection, int id, const nlohmann::json& config)
    : Channel(connection, id, config["name"].get<std::string>(),
              parse_coupling(config["coupling"].get<std::string>()),
              config["current_range"].get<float>(), config["voltage_range"].get<double>())
    {
        if (config["metrics"].size() == 0)
        {
            raise("No metrics given for channel: ", id_);
        }

        for (auto& metric : config["metrics"])
        {
            if (metric.get<std::string>() != "power")
            {
                raise("Only power metrics are supported.");
            }
        }
    }

public:
    const std::string& name() const
    {
        return name_;
    }

    SignalCoupling coupling() const
    {
        return coupling_;
    }

    float current_range() const
    {
        return current_range_;
    }

    float voltage_range() const
    {
        return voltage_range_;
    }

private:
    network::Connection& connection_;
    int id_;
    std::string name_;
    SignalCoupling coupling_;
    float current_range_;
    float voltage_range_;
};
} // namespace lmgd::device
