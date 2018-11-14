#pragma once

#include <lmgd/device/track.hpp>
#include <lmgd/device/types.hpp>

#include <metricq/metric.hpp>
#include <metricq/source.hpp>

namespace lmgd::source
{
class Metric
{
public:
    Metric(const device::Track& track, metricq::Metric<metricq::Source>& metric,
           int max_repeats = 8)
    : metric_(metric), bandwidth_(track.bandwidth()), max_repeats_(max_repeats)
    {
    }

public:
    void send(metricq::TimePoint tp, float value)
    {
        if (device::MetricBandwidth::wide == bandwidth_ ||
            device::MetricBandwidth::cycle == bandwidth_)
        {
            metric_.send({ tp, value });
            return;
        }

        if (value == last_value_ && repeat_ < max_repeats_)
        {
            ++repeat_;
        }
        else
        {
            repeat_ = 0;
            last_value_ = value;
            metric_.send({ tp, value });
        }
    }

    void flush()
    {
        metric_.flush();
    }

private:
    metricq::Metric<metricq::Source>& metric_;

    device::MetricBandwidth bandwidth_;

    int max_repeats_;
    float last_value_;
    int repeat_ = 0;
};
} // namespace lmgd::source
