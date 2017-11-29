#include <scorep/plugin/plugin.hpp>

#include <atomic>
#include <thread>

using namespace scorep::plugin::policy;

class lmg_plugin : public scorep::plugin::base<lmg_plugin, policy::async, policy::once,
                                               policy::per_host, policy::post_mortem>
{
public:
    std::vector<scorep::plugin::metric_property>
    get_metric_properties(const std::string& metric_parse)
    {
        return scorep::plugin::metric_property("lmg metric foo", "denk dir selber was aus")
            .absolute_point()
            .value_double()
            .decimal()
            .value_exponent(0);
    }

    int32_t add_metric(const std::string& metric_name)
    {
    }

    void start();
    void stop();

    template <class Cursor>
    void get_all_values(std::int32_t id, Cursor& c);

private:
    std::atomic<bool> keep_running_ = true;
    std::thread thread_;
};

SCOREP_METRIC_PLUGIN_CLASS(lmg_plugin, "lmg")
