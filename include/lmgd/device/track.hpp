#pragma once

#include <lmgd/device/types.hpp>

#include <string>

namespace lmgd::device
{
class Channel;

class Track
{
public:
    // This assumes, that we have exactly one group!
    Track(const Channel& channel, int id, MetricType type, MetricBandwidth bandwidth);

public:
    std::string get_glctrac_command() const;

public:
    const Channel& channel() const;
    int id() const;
    MetricBandwidth bandwidth() const;
    MetricType type() const;
    std::string name() const;

private:
    const Channel& channel_;

    int id_;

    MetricType type_;
    // the group on the device
    int group_;
    // the channel inside of the group
    int phase_;
    MetricBandwidth bandwidth_;
};
} // namespace lmgd::device
