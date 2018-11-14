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
            command += ":POW";
            break;
        case MetricType::current:
            command += ":CURR";
            break;
        case MetricType::voltage:
            command += ":VOLT";
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
    case MetricType::voltage:
        result += ".voltage";
        break;
    case MetricType::current:
        result += ".current";
        break;
    case MetricType::power:
        result += ".power";
        break;
    }

    return result + (bandwidth_ == MetricBandwidth::wide ? ".wide" : "");
}

} // namespace lmgd::device
