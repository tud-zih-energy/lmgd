#include <lmgd/source/source.hpp>

#include <lmgd/device/device.hpp>
#include <lmgd/log.hpp>

#include <metricq/ostream.hpp>

#include <nitro/lang/enumerate.hpp>

#include <chrono>
#include <memory>

namespace lmgd::source
{

Source::Source(const std::string& server, const std::string& token, bool drop_data)
: metricq::Source(token),
  signals_(io_service, SIGINT, SIGTERM),
  timer_(io_service),
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
        else
        {
            stop();
        }
    });

    connect(server);
}

void Source::on_source_config(const nlohmann::json& config)
{
    Log::debug() << "Called on_source_config()";
    std::lock_guard<std::mutex> lock(config_mutex_);
    config_ = config;
    if (device_)
    {
        Log::info() << "Received new config. Restarting requested.";
        device_->stop_recording();
    }
    Log::debug() << "Finished on_source_config()";
}

Source::~Source()
{
}

void Source::setup_device()
{
    std::lock_guard<std::mutex> lock(config_mutex_);

    // if the device pointer is already set, then this is a reconfigure
    auto is_reconfigure = static_cast<bool>(device_);

    // When handling a reconfigure, before creating a new device, we need to make sure, the old one
    // is gone. So yes, this explicit reset is intentional.
    device_.reset(nullptr);
    device_ = std::make_unique<lmgd::device::Device>(io_service, config_);
    chunk_size_ = config_["chunk_size"].get<int>();

    // resetting internal state for reconfigure
    lmg_metrics_.clear();
    offset_metrics_.clear();
    clear_metrics();

    for (auto& track : device_->get_tracks())
    {
        auto& source_metric = (*this)[track.name()];
        source_metric.metadata.rate(device_->sampling_rate());
        Log::info() << "Add metric to recording: " << track.name();
        source_metric.chunk_size(chunk_size_);
        // TODO set max_repeats dependent to sampling rate
        lmg_metrics_.emplace_back(track, source_metric);
        if (device_->measurement_mode() == device::MeasurementMode::gapless)
        {
            source_metric.metadata.chunk_size(device_->gap_length());

            offset_metrics_.emplace_back(*this, track.name());
            offset_metrics_.back().local_offset.metadata.rate(
                device_->sampling_rate() / device_->gap_length());
            offset_metrics_.back().chunk_offset.metadata.rate(
                device_->sampling_rate() / device_->gap_length());
        }
    }

    device_->start_recording(lmgd::network::Connection::Mode::binary);

    timer_.start(
        [this](auto) {
            Log::fatal() << "LMG failed to send values within the last 10 seconds. "
                            "Assuming the connection died.";
            throw std::runtime_error("Connection to LMG timed out");
            return metricq::Timer::TimerResult::cancel;
        },
        std::chrono::seconds(10));

    device_->fetch_binary_data([this](auto& data) {
        Log::trace() << "Called completion_callback: " << data->size();

        timer_.restart();

        if (data->size() == 1)
        {
            char c = data->read_char();
            if (c != '1')
            {
                Log::error() << "Unexpected single char '" << c << "' (" << static_cast<int>(c)
                             << ")";
            }

            if (stop_requested_)
            {
                Log::info() << "Datastream from device ended. Stop.";
                this->timer_.cancel();
                this->stop();
            }
            else
            {
                Log::info() << "Datastream from device ended unexpectedly. Restarting...";
                this->timer_.cancel();
                this->setup_device();
            }
            return network::CallbackResult::cancel;
        }

        if (this->drop_data_)
        {
            return network::CallbackResult::repeat;
        }

        if (device_->measurement_mode() == device::MeasurementMode::gapless)
        {
            const auto base_cycle_start = data->read_date();
            const auto cycle_duration = data->read_time();

            auto it = this->offset_metrics_.begin();

            for (auto& metric : this->lmg_metrics_)
            {
                const auto cycle_start = metric.cycle_start(base_cycle_start);

                assert(it != this->offset_metrics_.end());
                auto& offset_metric = *it++;

                offset_metric.local_offset.send({
                    metricq::TimePoint(cycle_start.time_since_epoch()),
                    std::chrono::duration_cast<std::chrono::duration<double>>(
                        metricq::Clock::now().time_since_epoch() - cycle_start.time_since_epoch())
                        .count(),
                });

                offset_metric.chunk_offset.send({
                    metricq::TimePoint(cycle_start.time_since_epoch()),
                    std::chrono::duration_cast<std::chrono::duration<double>>(
                        base_cycle_start - cycle_start)
                        .count(),
                });

                const auto list = data->read_float_list();

                for (auto entry : nitro::lang::enumerate(list))
                {
                    auto time_ns = cycle_start + entry.index() * cycle_duration / list.size();
                    metric.send(metricq::TimePoint(time_ns.time_since_epoch()), entry.value());
                }
                if (chunk_size_ == 0)
                {
                    metric.flush();
                }
                metric.cycle_end(cycle_start + cycle_duration);
            }
        }
        else
        {
            auto now = metricq::Clock::now();
            for (auto& metric : this->lmg_metrics_)
            {
                metric.send(now, data->read_float());
                if (chunk_size_ == 0)
                {
                    metric.flush();
                }
            }
        }
        return network::CallbackResult::repeat;
    });

    if (is_reconfigure)
    {
        declare_metrics();
    }
}

void Source::on_source_ready()
{
    Log::debug() << "Called on_source_ready()";
    setup_device();
    Log::debug() << "Finished on_source_ready()";
}

void Source::on_error(const std::string& message)
{
    Log::error() << "Connection to MetricQ failed: " << message;
    if (device_)
    {
        stop_requested_ = true;
        device_->stop_recording();
    }
    signals_.cancel();
}

void Source::on_closed()
{
    Log::debug() << "Connection to MetricQ closed.";
    if (device_)
    {
        stop_requested_ = true;
        device_->stop_recording();
    }
    signals_.cancel();
}

} // namespace lmgd::source
