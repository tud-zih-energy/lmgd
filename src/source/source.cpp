#include <lmgd/source/source.hpp>

#include <lmgd/device/device.hpp>

#include <lmgd/log.hpp>

#include <memory>

namespace lmgd::source
{

Source::Source(const std::string& server, const std::string& token)
: dataheap2::Source(token), signals_(io_service, SIGINT, SIGTERM), timer_(io_service)
{
    Log::debug() << "Called lmgd::Source::Source()";

    // Register signal handlers so that the daemon may be shut down.
    signals_.async_wait([this](auto, auto signal) {
        if (!signal)
        {
            return;
        }
        Log::info() << "Catched signal " << signal << ". Shutdown.";
        if (device_)
        {
            device_->stop_recording();
        }
        // Maybe this isn't a good idea... who knows ¯\_(ツ)_/¯
        io_service.stop();
    });

    connect(server);
}

void Source::source_config_callback(const nlohmann::json& config)
{
    Log::debug() << "Called source_config_callback()";

    config_ = config;

    device_ = std::make_unique<lmgd::device::Device>(io_service, config);
}

Source::~Source()
{
}

void Source::ready_callback()
{
    Log::debug() << "Called ready_callback()";

    for (auto& track : device_->get_tracks())
    {
        auto& source_metric = (*this)[track];
        source_metric.enable_chunking(config_["measurement"]["chunking"].get<int>());
        metrics_.push_back(std::ref(source_metric));
    }

    device_->start_recording();

    // TODO is this enough? or do we need more wakeups?
    // timer_.start(
    //     [this](auto error_code) {
    //         // what could possibly go wrong?
    //         (void)error_code;
    //         this->device_->fetch_data(this->metrics_);
    //         return dataheap2::Timer::TimerResult::repeat;
    //     },
    //     std::chrono::milliseconds(100));

    this->device_->fetch_data(this->metrics_);
}

} // namespace lmgd::source
