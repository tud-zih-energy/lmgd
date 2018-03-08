#pragma once

#include <lmgd/device/track.hpp>
#include <lmgd/device/types.hpp>

#include <dataheap2/source_metric.hpp>

namespace lmgd::source
{
class Metric
{
public:
    Metric(const device::Track& track, dataheap2::SourceMetric& metric, int max_repeats = 8)
    : metric_(metric), bandwidth_(track.bandwidth())
    {
    }

public:
    void send(dataheap2::TimePoint tp, float value)
    {
        if (device::MetricBandwidth::wide == bandwidth_)
        {
            metric_.send({ tp, value });
            return;
        }

        if (repeat_++ == 0)
        {
            last_value_ = value;
            metric_.send({ tp, value });
        }
        else
        {
            if (value != last_value_ || repeat_ == max_repeats_)
            {
                repeat_ = 0;
            }
        }
    }

private:
    dataheap2::SourceMetric& metric_;

    device::MetricBandwidth bandwidth_;

    int max_repeats_;
    float last_value_;
    int repeat_ = 0;
};
} // namespace lmgd::source
