#define ENV_CONFIG_PREFIX "SCOREP_METRIC_LMG_PLUGIN_"

#include <lmgd/network/connection.hpp>
#include <lmgd/data.hpp>

#include <scorep/plugin/plugin.hpp>

#include <nitro/lang/enumerate.hpp>

#include <atomic>
#include <thread>

using namespace scorep::plugin::policy;

using scorep::plugin::logging;

class lmg_plugin : public scorep::plugin::base<lmg_plugin, async, once, post_mortem, scorep_clock>
{
private:
    std::vector<std::string> tracks;

public:
    std::vector<scorep::plugin::metric_property> get_metric_properties(const std::string& name)
    {
        return { scorep::plugin::metric_property(name).absolute_point().value_double() };
    }

    int32_t add_metric(const std::string& metric_name)
    {
        tracks.push_back(metric_name);
        return tracks.size() - 1;
    }

    void start()
    {
        connection_ =
            std::make_unique<lmgd::network::Connection>(scorep::environment_variable::get("HOST"));
        connection_->send_command("GROUP 7");
        for (const auto& track : nitro::lang::enumerate(tracks))
        {
            connection_->send_command("GLCTRAC " + std::to_string(track.index()) + ", \"" +
                                      track.value() + "\"");
        }
        connection_->send_command("GLCSR " + scorep::environment_variable::get("RATE", "10000"));
        connection_->send_command("CYCLMOD SCOPE");
        connection_->send_command("INIM");
        connection_->send_command("GLPTLEN?");
        this->gap_length = std::stoi(connection_->read_ascii());

        std::string action = "ACTN; TSSP?";
        for (const auto& track : nitro::lang::enumerate(tracks))
        {
            action += "; GLPVAL? " + std::to_string(track.index()) +
                      ", (0:" + std::to_string(this->gap_length - 1) + ")";
        }

        logging::debug() << "Using action: " << action;
        connection_->send_command(action);
        connection_->send_command("CONT ON");

        thread_ = std::thread([this]() { this->run_thread(); });
    }

    void stop()
    {
        // The thread doesn't write to the socket, so this is fine... right?
        connection_->send_command("CONT OFF");
        keep_running_ = false;
    }

    template <class Cursor>
    void get_all_values(std::int32_t id, Cursor& c) {
        // ...
    }

private:
    void run_thread()
    {
        while (keep_running_) {
            data_.push_back(connection_->read_binary());
        }
    }

    int64_t gap_length;
    std::unique_ptr<lmgd::network::Connection> connection_;
    std::atomic<bool> keep_running_ = true;
    std::thread thread_;
    std::vector<lmgd::BinaryData> data_;
};

SCOREP_METRIC_PLUGIN_CLASS(lmg_plugin, "lmg")
