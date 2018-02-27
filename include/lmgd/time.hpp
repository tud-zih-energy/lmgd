#pragma once

#include <chrono>
#include <ratio>
#include <stdexcept>

namespace lmgd
{
namespace time
{
    using Duration = std::chrono::duration<std::int64_t, std::nano>;

    class LmgClock
    {
    public:
        using duration = Duration;
        using rep = Duration::rep;
        using period = Duration::rep;
        using time_point = std::chrono::time_point<LmgClock>;
        static constexpr bool is_steady = true;

        time_point now()
        {
            throw std::logic_error("Timestamps can only be taken on the lmg device.");
        }
    };

    using TimePoint = LmgClock::time_point;
} // namespace time
} // namespace lmgd
