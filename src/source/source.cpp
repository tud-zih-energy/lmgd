#include <lmgd/source/source.hpp>

#include <lmgd/device/device.hpp>

#include <lmgd/log.hpp>

#include <memory>

namespace lmgd::source
{

Source::Source(const std::string& server, const std::string& token) : dataheap2::Source(token)
{
    Log::info() << "Called lmgd::Source::Source()";

    connect(server);
}

void Source::source_config_callback(const nlohmann::json& config)
{
    Log::info() << "Called source_config_callback()";

    device_ = std::make_unique<lmgd::device::Device>(config);
}

Source::~Source()
{
}

void Source::ready_callback()
{
    Log::info() << "Called ready_callback()";

    for (auto& metric_name : device_->get_metrics())
    {
        auto& metric = (*this)[metric_name];

        // TODO which size of chunking?
        metric.enable_chunking(10000);

        metrics_.push_back(std::ref(metric));
    }

    device_->start_recording();

    // TODO is this enough? or do we need more wakeups?
    register_timer([this]() { this->device_->fetch_data(this->metrics_); },
                   std::chrono::milliseconds(100));
}

} // namespace lmgd::source
