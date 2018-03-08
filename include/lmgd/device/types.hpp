#pragma once

namespace lmgd::device
{
enum class ChannelSignalCoupling : int
{
    acdc = 0,
    ac = 1,
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
    narrow = 1,
    wide = 2
};
} // namespace lmgd::device
