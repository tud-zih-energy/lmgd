#pragma once

#include <lmgd/source/metric.hpp>

#include <metricq/source.hpp>
#include <metricq/timer.hpp>

#include <asio/basic_waitable_timer.hpp>
#include <asio/signal_set.hpp>

#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

namespace lmgd::device
{
class Device;
}

namespace lmgd::source
{

class Source : public metricq::Source
{
public:
    Source(const std::string& server, const std::string& token, bool drop_data);
    ~Source();

    void source_config_callback(const nlohmann::json& config) override;
    void ready_callback() override;

private:
    void setup_device();

private:
    std::mutex config_mutex_;
    asio::signal_set signals_;
    metricq::Timer timer_;
    std::unique_ptr<lmgd::device::Device> device_;
    std::vector<Metric> metrics_;
    nlohmann::json config_;
    std::atomic<bool> stop_requested_ = false;
    bool drop_data_;
};

} // namespace lmgd::source
