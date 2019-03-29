#pragma once

#include <metricq/logger/nitro.hpp>

namespace lmgd
{

namespace detail
{

    using record = nitro::log::record<nitro::log::tag_attribute, nitro::log::message_attribute,
                                      nitro::log::severity_attribute, nitro::log::jiffy_attribute>;

    template <typename Record>
    class log_formater
    {
    public:
        std::string format(Record& r)
        {
            std::stringstream s;

            s << "[" << r.jiffy() << "][";

            if (!r.tag().empty())
            {
                s << r.tag() << "][";
            }

            s << r.severity() << "]: " << r.message() << '\n';

            return s.str();
        }
    };

    template <typename Record>
    using log_filter = nitro::log::filter::severity_filter<Record>;
} // namespace detail

using Log = metricq::logger::nitro::Log;

} // namespace lmgd
