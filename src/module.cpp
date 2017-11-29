#define ENV_CONFIG_PREFIX "SCOREP_METRIC_LMG_PLUGIN_"

#include <lmgd/data.hpp>
#include <lmgd/network/connection.hpp>

#include <scorep/plugin/plugin.hpp>

#include <nitro/lang/enumerate.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <thread>
#include <utility>

using namespace scorep::plugin::policy;

using scorep::plugin::logging;

// Must be system clock for real epoch!

// XXX move to CXX wrapper, seems to be useful
using local_clock = std::chrono::system_clock;
template <typename TP>
class scorep_time_converter
{
    using local_duration_t = typename TP::duration;

public:
    scorep_time_converter(TP local_start, TP local_stop, scorep::chrono::ticks scorep_start,
                          scorep::chrono::ticks scorep_stop)
    : local_start_(local_start), scorep_start_(scorep_start)
    {
        const auto local_duration = local_stop - local_start;
        const auto scorep_duration = scorep_stop - scorep_start;
        tick_rate_ = static_cast<double>(scorep_duration.count()) / local_duration.count();
    }

    template <typename T>
    scorep::chrono::ticks to_ticks(const T duration) const
    {
        return to_ticks(std::chrono::duration_cast<local_duration_t>(duration));
    }

    scorep::chrono::ticks to_ticks(const local_duration_t duration) const
    {
        return scorep::chrono::ticks(duration.count() * tick_rate_);
    }

    scorep::chrono::ticks to_ticks(const TP tp) const
    {
        const auto tp_offset = tp - local_start_;
        return scorep_start_ + to_ticks(tp_offset);
    }

private:
    TP local_start_;
    scorep::chrono::ticks scorep_start_;
    double tick_rate_;
};

class lmg_plugin : public scorep::plugin::base<lmg_plugin, async, once, post_mortem, scorep_clock>
{
public:
    std::vector<scorep::plugin::metric_property> get_metric_properties(const std::string& name)
    {
        return { scorep::plugin::metric_property(name).absolute_point().value_double() };
    }

    int32_t add_metric(const std::string& metric_name)
    {
        tracks_.push_back(metric_name);
        return tracks_.size() - 1;
    }

    void start()
    {
        local_start_ = local_clock::now();
        scorep_start_ = scorep::chrono::measurement_clock::now();

        connection_ =
            std::make_unique<lmgd::network::Connection>(scorep::environment_variable::get("HOST"));

        connection_->send_command("ERRALL?");
        logging::info() << "errors before: " << connection_->read_ascii();

        connection_->send_command("GROUP 6");
        for (const auto& track : nitro::lang::enumerate(tracks_))
        {
            connection_->send_command("GLCTRAC " + std::to_string(track.index()) + ", \"" +
                                      track.value() + "\"");
        }
        connection_->send_command("GLCSR " + scorep::environment_variable::get("RATE", "10000"));
        connection_->send_command("CYCLMOD SCOPE");
        connection_->send_command("INIM");
        connection_->send_command("GLPTLEN?");
        gap_length_ = std::stoi(connection_->read_ascii());

        connection_->send_command("GLPSR?");
        sampling_rate_ = std::stof(connection_->read_ascii());

        logging::debug() << "gap_length: " << gap_length_ << ", sampling_rate: " << sampling_rate_;

        connection_->send_command("ERRALL?");
        logging::info() << "errors: " << connection_->read_ascii();

        std::string action = "ACTN; TSSP?";
        for (const auto& track : nitro::lang::enumerate(tracks_))
        {
            action += "; GLPVAL? " + std::to_string(track.index()) +
                      ", (0:" + std::to_string(this->gap_length_ - 1) + ")";
        }

        logging::debug() << "Using action: " << action;
        connection_->send_command(action);

        connection_->mode(lmgd::network::Connection::Mode::binary);
        connection_->send_command("CONT ON");

        thread_ = std::thread([this]() { this->run_thread(); });
    }

    void stop()
    {
        local_stop_ = local_clock::now();
        scorep_stop_ = scorep::chrono::measurement_clock::now();

        // The thread doesn't write to the socket, so this is fine... right?
        connection_->send_command("CONT OFF");
        keep_running_ = false;
        thread_.join();

        if (data_.size() == 0)
        {
            logging::error() << "No lmg readings available.";
            return;
        }
        track_data_.resize(tracks_.size());
        for (auto& data_line : data_)
        {
            if (data_line.size() == 1) break; // XXX buffer with just '1' at the end... hmmm
            auto start_time_ns = data_line.read_time();
            for (const auto& track : nitro::lang::enumerate(tracks_))
            {
                auto list = data_line.read_float_list();
                track_data_[track.index()].push_back({ start_time_ns, list });
            }
        }
    }

    template <class Cursor>
    void get_all_values(std::int32_t id, Cursor& c)
    {
        auto converter = scorep_time_converter<local_clock::time_point>(
            local_start_, local_stop_, scorep_start_, scorep_stop_);

        for (auto& time_list : track_data_[id])
        {
            auto start_time_ns = time_list.first;
            for (auto entry : nitro::lang::enumerate(time_list.second))
            {
                // TODO Rounding?!
                int64_t time_ns = start_time_ns + entry.index() * 1000000l / sampling_rate_;
                local_clock::time_point tp{std::chrono::nanoseconds(time_ns)};
                c.write(converter.to_ticks(tp), entry.value());
            }
        }
    }

private:
    void run_thread()
    {
        while (keep_running_)
        {
            data_.push_back(connection_->read_binary());
        }
    }

    std::vector<std::string> tracks_;
    int64_t gap_length_;
    float sampling_rate_;
    std::unique_ptr<lmgd::network::Connection> connection_;
    std::atomic<bool> keep_running_ = true;
    std::thread thread_;
    std::vector<lmgd::BinaryData> data_;
    std::vector<std::vector<std::pair<int64_t, lmgd::BinaryList<float>>>> track_data_;

    scorep::chrono::ticks scorep_start_, scorep_stop_;
    std::chrono::time_point<local_clock> local_start_, local_stop_;
};

SCOREP_METRIC_PLUGIN_CLASS(lmg_plugin, "lmg")
