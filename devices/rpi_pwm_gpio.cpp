#include "rpi_pwm_gpio.h"

#include <algorithm>
#include <cstdio>
#include <limits>

namespace {
constexpr uint8_t kGpioPinId{12};
constexpr uint8_t kRevolutionsPerPulse{2};
constexpr uint64_t kNanosecondsPerMinute{60'000'000'000ULL};
constexpr qint64 kPulseTimeoutMs{1000};
} // namespace

RpiPwmGpio::RpiPwmGpio(QObject *parent) : QObject(parent) {
    chip_ = gpiod_chip_open("/dev/gpiochip0");
    if (!chip_) {
        std::perror("gpiod_chip_open");
        return;
    }

#ifdef MOTOHUD_LIBGPIOD_V2
    settings_ = gpiod_line_settings_new();
    line_config_ = gpiod_line_config_new();
    request_config_ = gpiod_request_config_new();
    if (!settings_ || !line_config_ || !request_config_) {
        std::perror("libgpiod setup");
        return;
    }

    const unsigned int gpio_offset = kGpioPinId;
    if (gpiod_line_settings_set_direction(settings_,
                                                                                GPIOD_LINE_DIRECTION_INPUT) < 0 ||
            gpiod_line_settings_set_edge_detection(settings_,
                                                                                         GPIOD_LINE_EDGE_RISING) < 0 ||
            gpiod_line_config_add_line_settings(line_config_, &gpio_offset, 1,
                                                                                    settings_) < 0) {
        std::perror("configure tachometer GPIO");
        return;
    }

    gpiod_request_config_set_consumer(request_config_, "motohud-tachometer");
    request_ =
            gpiod_chip_request_lines(chip_, request_config_, line_config_);
    if (!request_) {
        std::perror("request tachometer GPIO");
        return;
    }

    event_buffer_ = gpiod_edge_event_buffer_new(16);
    if (!event_buffer_) {
        std::perror("create tachometer event buffer");
        return;
    }
    const int event_fd = gpiod_line_request_get_fd(request_);
#else
    line_ = gpiod_chip_get_line(chip_, kGpioPinId);
    if (!line_ ||
            gpiod_line_request_rising_edge_events(line_, "motohud-tachometer") < 0) {
        std::perror("request tachometer GPIO");
        return;
    }
    const int event_fd = gpiod_line_event_get_fd(line_);
#endif

    if (event_fd < 0) {
        std::perror("get tachometer event descriptor");
        return;
    }

    notifier_ = new QSocketNotifier(event_fd, QSocketNotifier::Read, this);
    connect(notifier_, &QSocketNotifier::activated, this,
                    [this] { readEvents(); });
}

RpiPwmGpio::~RpiPwmGpio() {
    if (notifier_) {
        notifier_->setEnabled(false);
        delete notifier_;
    }

#ifdef MOTOHUD_LIBGPIOD_V2
    if (event_buffer_)
        gpiod_edge_event_buffer_free(event_buffer_);
    if (request_)
        gpiod_line_request_release(request_);
    if (request_config_)
        gpiod_request_config_free(request_config_);
    if (line_config_)
        gpiod_line_config_free(line_config_);
    if (settings_)
        gpiod_line_settings_free(settings_);
#else
    if (line_)
        gpiod_line_release(line_);
#endif
    if (chip_)
        gpiod_chip_close(chip_);
}

uint16_t RpiPwmGpio::rpm() const {
    if (!has_pulse_ || last_pulse_elapsed_.elapsed() > kPulseTimeoutMs)
        return 0;
    return rpm_;
}

void RpiPwmGpio::readEvents() {
#ifdef MOTOHUD_LIBGPIOD_V2
    const int event_count =
            gpiod_line_request_read_edge_events(request_, event_buffer_, 16);
    if (event_count < 0) {
        std::perror("read tachometer GPIO event");
        return;
    }

    for (int index = 0; index < event_count; ++index) {
        gpiod_edge_event *event =
                gpiod_edge_event_buffer_get_event(event_buffer_, index);
        recordPulse(gpiod_edge_event_get_timestamp_ns(event));
    }
#else
    gpiod_line_event event{};
    if (gpiod_line_event_read(line_, &event) < 0) {
        std::perror("read tachometer GPIO event");
        return;
    }

    const uint64_t timestamp_ns =
            static_cast<uint64_t>(event.ts.tv_sec) * 1'000'000'000ULL +
            static_cast<uint64_t>(event.ts.tv_nsec);
    recordPulse(timestamp_ns);
#endif
}

void RpiPwmGpio::recordPulse(uint64_t timestamp_ns) {
    if (previous_timestamp_ns_ != 0 &&
            timestamp_ns > previous_timestamp_ns_) {
        const uint64_t period_ns = timestamp_ns - previous_timestamp_ns_;
        const uint64_t measured_rpm =
                (kNanosecondsPerMinute * kRevolutionsPerPulse) / period_ns;
        rpm_ = static_cast<uint16_t>(std::min<uint64_t>(
                measured_rpm, std::numeric_limits<uint16_t>::max()));
    }

    previous_timestamp_ns_ = timestamp_ns;
    last_pulse_elapsed_.restart();
    has_pulse_ = true;
}