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
        metric.metadata(metricq::Metadata::Scope::last);

        switch (bandwidth_)
        {
        case device::MetricBandwidth::cycle:
            metric.metadata["bandwidth"] = "cycle";
            break;
        case device::MetricBandwidth::narrow:
            metric.metadata["bandwidth"] = "narrow";
            break;
        case device::MetricBandwidth::wide:
            metric.metadata["bandwidth"] = "wide";
            break;
        }

        switch (track.type())
        {
        case device::MetricType::phi:
            metric.metadata.unit("°");
            break;
        case device::MetricType::current:
            metric.metadata.unit("A");
            break;
        case device::MetricType::current_min:
            metric.metadata.unit("A");
            break;
        case device::MetricType::current_max:
            metric.metadata.unit("A");
            break;
        case device::MetricType::current_crest:
            metric.metadata.unit("1");
            break;
        case device::MetricType::power:
            metric.metadata.unit("W");
            break;
        case device::MetricType::apparent_power:
            metric.metadata.unit("W");
            break;
        case device::MetricType::reactive_power:
            metric.metadata.unit("W");
            break;
        case device::MetricType::voltage:
            metric.metadata.unit("V");
            break;
        case device::MetricType::voltage_min:
            metric.metadata.unit("V");
            break;
        case device::MetricType::voltage_max:
            metric.metadata.unit("V");
            break;
        case device::MetricType::voltage_crest:
            metric.metadata.unit("1");
            break;
        }
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
