#pragma once

#include <lmgd/source/metric.hpp>

#include <dataheap2/source.hpp>
#include <dataheap2/timer.hpp>

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

class Source : public dataheap2::Source
{
public:
    Source(const std::string& server, const std::string& token);
    ~Source();

    void source_config_callback(const nlohmann::json& config) override;
    void ready_callback() override;

private:
    void setup_device();

private:
    std::mutex config_mutex_;
    asio::signal_set signals_;
    dataheap2::Timer timer_;
    std::unique_ptr<lmgd::device::Device> device_;
    std::vector<Metric> metrics_;
    nlohmann::json config_;
    std::atomic<bool> stop_requested_ = false;
};

} // namespace lmgd::source
