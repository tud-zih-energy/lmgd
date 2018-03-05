#include <lmgd/source/source.hpp>

#include <lmgd/device/device.hpp>

#include <lmgd/log.hpp>

#include <memory>

namespace lmgd::source
{

Source::Source(const std::string& server, const std::string& token)
: dataheap2::Source(token), timer_(io_service)
{
    Log::info() << "Called lmgd::Source::Source()";

    connect(server);
}

void Source::source_config_callback(const nlohmann::json& config)
{
    Log::info() << "Called source_config_callback()";

    config_ = config;

    device_ = std::make_unique<lmgd::device::Device>(io_service, config);
}

Source::~Source()
{
}

void Source::ready_callback()
{
    Log::info() << "Called ready_callback()";

    for (auto& track : device_->get_tracks())
    {
        auto& source_metric = (*this)[track];
        source_metric.enable_chunking(config_["measurement"]["chunking"].get<int>());
        metrics_.push_back(std::ref(source_metric));
    }

    device_->start_recording();

    // TODO is this enough? or do we need more wakeups?
    timer_.start(
        [this](auto error_code) {
            // what could possibly go wrong?
            (void)error_code;
            this->device_->fetch_data(this->metrics_);
            return dataheap2::Timer::TimerResult::repeat;
        },
        std::chrono::milliseconds(100));
}

} // namespace lmgd::source
