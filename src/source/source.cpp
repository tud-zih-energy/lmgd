#include <lmgd/source/source.hpp>

#include <lmgd/device/device.hpp>

#include <lmgd/log.hpp>

#include <nitro/lang/enumerate.hpp>

#include <memory>

namespace lmgd::source
{

Source::Source(const std::string& server, const std::string& token, bool drop_data)
: dataheap2::Source(token), signals_(io_service, SIGINT, SIGTERM), timer_(io_service),
  drop_data_(drop_data)
{
    Log::debug() << "Called lmgd::Source::Source()";

    // Register signal handlers so that the daemon may be shut down.
    signals_.async_wait([this](auto, auto signal) {
        if (!signal)
        {
            return;
        }

        stop_requested_ = true;

        Log::info() << "Caught signal " << signal << ". Shutdown.";
        if (device_)
        {
            device_->stop_recording();
        }
    });

    connect(server);
}

void Source::source_config_callback(const nlohmann::json& config)
{
    Log::debug() << "Called source_config_callback()";
    std::lock_guard<std::mutex> lock(config_mutex_);
    config_ = config;
    if (device_)
    {
        Log::info() << "Received new config. Restarting requested.";
        device_->stop_recording();
    }
    Log::debug() << "Finished source_config_callback()";
}

Source::~Source()
{
}

void Source::setup_device()
{
    std::lock_guard<std::mutex> lock(config_mutex_);

    device_ = std::make_unique<lmgd::device::Device>(io_service, config_);

    for (auto& track : device_->get_tracks())
    {
        auto& source_metric = (*this)[track.name()];
        Log::info() << "Add metric to recording: " << track.name();
        source_metric.enable_chunking(config_["measurement"]["chunking"].get<int>());
        // TODO set max_repeats dependent to sampling rate
        metrics_.emplace_back(track, source_metric);
    }

    device_->start_recording();
    device_->fetch_data([this](auto& data) {
        Log::trace() << "Called completion_callback: " << data->size();
        if (data->size() == 1)
        {
            char c = data->read_char();
            if (c != '1')
            {
                Log::error() << "Unexpected single char '" << c << "' (" << static_cast<int>(c) << ")";
            }

            if (stop_requested_)
            {
                Log::info() << "Datastream from device ended. Stop.";
                this->io_service.stop();
            }
            else
            {
                Log::info() << "Datastream from device ended unexpectedly. Restarting...";
                this->setup_device();
            }
            return network::CallbackResult::cancel;
        }

        if (this->drop_data_)
        {
            return network::CallbackResult::repeat;
        }

        auto cycle_start = data->read_date();
        auto cycle_duration = data->read_time();

        for (auto& metric : this->metrics_)
        {

            auto list = data->read_float_list();

            for (auto entry : nitro::lang::enumerate(list))
            {
                auto time_ns = cycle_start + entry.index() * cycle_duration / list.size();
                metric.send(dataheap2::TimePoint(time_ns.time_since_epoch()), entry.value());
            }
        }
        return network::CallbackResult::repeat;
    });
}

void Source::ready_callback()
{
    Log::debug() << "Called ready_callback()";
    setup_device();
    Log::debug() << "Finished ready_callback()";
}

} // namespace lmgd::source
