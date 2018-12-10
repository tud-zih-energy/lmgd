#include <lmgd/device/track.hpp>

#include <lmgd/device/channel.hpp>

namespace lmgd::device
{
// This assumes, that we have exactly one group!
Track::Track(const Channel& channel, int id, MetricType type, MetricBandwidth bandwidth)
: channel_(channel), id_(id), type_(type), group_(1), phase_(channel.id()), bandwidth_(bandwidth)
{
}

std::string Track::get_action_command(MeasurementMode mode) const
{
    if (mode == MeasurementMode::gapless)
    {
        return ":SENS:GAPL:TRAC " + std::to_string(id_) + ", \"" + char(type_) +
               std::to_string(group_) + std::to_string(phase_) + std::to_string(int(bandwidth_)) +
               "1\"";
    }
    else
    {
        std::string command = ":FETC";

        switch (type_)
        {
        case MetricType::power:
            command += ":POW:ACT";
            break;
        case MetricType::apparent_power:
            command += ":POW:APP";
            break;
        case MetricType::reactive_power:
            command += ":POW:REAC";
            break;
        case MetricType::phi:
            command += ":POW:PHAS";
            break;

        case MetricType::current:
            command += ":CURR:TRMS";
            break;
        case MetricType::current_min:
            command += ":CURR:MINP";
            break;
        case MetricType::current_max:
            command += ":CURR:MAXP";
            break;
        case MetricType::current_crest:
            command += ":CURR:CFAC";
            break;

        case MetricType::voltage:
            command += ":VOLT:TRMS";
            break;
        case MetricType::voltage_min:
            command += ":VOLT:MINP";
            break;
        case MetricType::voltage_max:
            command += ":VOLT:MAXP";
            break;
        case MetricType::voltage_crest:
            command += ":VOLT:CFAC";
            break;
        }

        command += std::to_string(channel_.id()) + "?";

        return command;
    }
}

const Channel& Track::channel() const
{
    return channel_;
}

int Track::id() const
{
    return id_;
}

MetricBandwidth Track::bandwidth() const
{
    return bandwidth_;
}

MetricType Track::type() const
{
    return type_;
}

std::string Track::name() const
{
    std::string result = channel_.name();

    switch (type_)
    {
    case MetricType::power:
        result += ".power";
        break;
    case MetricType::apparent_power:
        result += ".power.apparent";
        break;
    case MetricType::reactive_power:
        result += ".power.reactive";
        break;

    case MetricType::phi:
        result += ".phi";
        break;

    case MetricType::current:
        result += ".current";
        break;
    case MetricType::current_min:
        result += ".current.min";
        break;
    case MetricType::current_max:
        result += ".current.max";
        break;
    case MetricType::current_crest:
        result += ".current.crest";
        break;

    case MetricType::voltage:
        result += ".voltage";
        break;
    case MetricType::voltage_min:
        result += ".voltage.min";
        break;
    case MetricType::voltage_max:
        result += ".voltage.max";
        break;
    case MetricType::voltage_crest:
        result += ".voltage.crest";
        break;
    }

    return result + (bandwidth_ == MetricBandwidth::wide ? ".wide" : "");
}

} // namespace lmgd::device
