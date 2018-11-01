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
    current = 'I',
    power = 'P',
    voltage = 'U'
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
