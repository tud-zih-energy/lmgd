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

struct OffsetMetrics
{
    OffsetMetrics(metricq::Source& source, const std::string& base_metric)
    : local_offset(source[base_metric + ".local_offset"]),
      chunk_offset(source[base_metric + ".chunk_offset"])
    {
    }

    metricq::Metric<metricq::Source>& local_offset;
    metricq::Metric<metricq::Source>& chunk_offset;
};

class Source : public metricq::Source
{
public:
    Source(const std::string& server, const std::string& token, bool drop_data);
    ~Source();

    void on_source_config(const nlohmann::json& config) override;
    void on_source_ready() override;

protected:
    void on_error(const std::string& message) override;
    void on_closed() override;

private:
    void setup_device();

private:
    std::mutex config_mutex_;
    asio::signal_set signals_;
    metricq::Timer timer_;
    std::unique_ptr<lmgd::device::Device> device_;
    std::vector<lmgd::source::Metric> lmg_metrics_;
    std::vector<OffsetMetrics> offset_metrics_;
    nlohmann::json config_;
    std::atomic<bool> stop_requested_ = false;
    bool drop_data_;
    int chunk_size_;
};

} // namespace lmgd::source
