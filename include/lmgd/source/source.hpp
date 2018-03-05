#pragma once

#include <dataheap2/source.hpp>
#include <dataheap2/timer.hpp>

#include <asio/basic_waitable_timer.hpp>

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
    dataheap2::Timer timer_;
    std::unique_ptr<lmgd::device::Device> device_;
    std::vector<std::reference_wrapper<dataheap2::SourceMetric>> metrics_;
    nlohmann::json config_;
};

} // namespace lmgd::source
