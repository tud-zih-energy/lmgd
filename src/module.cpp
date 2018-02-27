#define ENV_CONFIG_PREFIX "SCOREP_METRIC_LMG_PLUGIN_"

#include <lmgd/data.hpp>
#include <lmgd/network/connection.hpp>

#include <scorep/plugin/plugin.hpp>

#include <nitro/lang/enumerate.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>
#include <thread>
#include <utility>

using namespace scorep::plugin::policy;

using scorep::plugin::logging;

// Must be system clock for real epoch!
using local_clock = std::chrono::system_clock;

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
        convert_.synchronize_point();

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
        // std::string action = "ACTN; TSEN?; SPTPOS?";
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
        convert_.synchronize_point();

        // The thread doesn't write to the socket, so this is fine... right?
        connection_->send_command("CONT OFF");
        keep_running_ = false;
        thread_.join();

        if (data_.size() == 0)
        {
            logging::error() << "No lmg readings available.";
            return;
        }
        logging::debug() << "converting total of " << data_.size() << " lines";
        track_data_.resize(tracks_.size());
        for (auto& data_line : data_)
        {
            if (data_line.size() == 1)
            {
                break; // XXX buffer with just '1' at the end... hmmm
            }
            auto start_time_ns = data_line.read_time();
            auto num_samples = data_line.read_int();
            logging::debug() << "start time ns " << start_time_ns;
            logging::debug() << "num_samples " << num_samples;
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
        for (auto& time_list : track_data_[id])
        {
            auto start_time_ns = time_list.first;
            for (auto entry : nitro::lang::enumerate(time_list.second))
            {
                // TODO Rounding?!
                auto time_ns = std::chrono::nanoseconds(
                    start_time_ns + int64_t(entry.index() * 1000000000l / sampling_rate_));
                c.write(convert_.to_ticks(time_ns), entry.value());
            }
        }
    }

private:
    void run_thread()
    {
        while (keep_running_)
        {
            logging::debug() << "reading binary line";
            data_.push_back(connection_->read_binary());
        }
    }

    std::vector<std::string> tracks_;
    int64_t gap_length_;
    double sampling_rate_;
    std::unique_ptr<lmgd::network::Connection> connection_;
    std::atomic<bool> keep_running_ = true;
    std::thread thread_;
    std::vector<lmgd::BinaryData> data_;
    std::vector<std::vector<std::pair<int64_t, lmgd::BinaryList<float>>>> track_data_;
    scorep::chrono::time_convert<local_clock> convert_;
};

SCOREP_METRIC_PLUGIN_CLASS(lmg_plugin, "lmg")
