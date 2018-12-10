#pragma once

namespace lmgd::device
{
enum class ChannelSignalCoupling : int
{
    acdc = 0,
    ac = 1,
    // GND is only supported on LMG670
    gnd = 9
};

enum class MetricType : char
{
    phi,
    current = 'I',
    current_min,
    current_max,
    current_crest,
    power = 'P', // active power
    apparent_power,
    reactive_power,
    voltage = 'U',
    voltage_min,
    voltage_max,
    voltage_crest

};

enum class MetricBandwidth : int
{
    cycle = 0,
    narrow = 1,
    wide = 2
};

enum class MeasurementMode
{
    cycle,
    gapless
};
} // namespace lmgd::device
