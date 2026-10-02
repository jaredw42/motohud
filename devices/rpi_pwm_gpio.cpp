#include <gpiod.h>

#include <cstdio>
#include <cstdint>

constexpr uint8_t kGpioPinId{12};
constexpr uint8_t kRevolutionsPerPulse{2};

int main() {
    const unsigned int gpio_offset = kGpioPinId;

    gpiod_chip* chip = gpiod_chip_open("/dev/gpiochip0");
    gpiod_line_settings* settings = gpiod_line_settings_new();
    gpiod_line_config* line_config = gpiod_line_config_new();
    gpiod_request_config* request_config = gpiod_request_config_new();

    if (!chip || !settings || !line_config || !request_config) {
        std::perror("libgpiod setup");
        return 1;
    }

    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
    gpiod_line_settings_set_edge_detection(settings, GPIOD_LINE_EDGE_RISING);
    gpiod_line_config_add_line_settings(line_config, &gpio_offset, 1, settings);
    gpiod_request_config_set_consumer(request_config, "tachometer");

    gpiod_line_request* request =
        gpiod_chip_request_lines(chip, request_config, line_config);
    gpiod_edge_event_buffer* buffer = gpiod_edge_event_buffer_new(1);

    if (!request || !buffer) {
        std::perror("request GPIO events");
        return 1;
    }

    uint64_t previous_ns = 0;

    for (;;) {
        if (gpiod_line_request_wait_edge_events(request, -1) < 0) {
            std::perror("wait for GPIO event");
            break;
        }

        if (gpiod_line_request_read_edge_events(request, buffer, 1) < 0) {
            std::perror("read GPIO event");
            break;
        }

        const gpiod_edge_event* event =
            gpiod_edge_event_buffer_get_event(buffer, 0);
        const uint64_t current_ns = gpiod_edge_event_get_timestamp_ns(event);

        if (previous_ns != 0 && current_ns > previous_ns) {
            const uint64_t period_ns = current_ns - previous_ns;
            const uint64_t rpm =
                (60ULL * kRevolutionsPerPulse * 1'000'000'000ULL) / period_ns;
            std::printf("RPM: %llu\n", static_cast<unsigned long long>(rpm));
        }

        previous_ns = current_ns;
    }

    gpiod_edge_event_buffer_free(buffer);
    gpiod_line_request_release(request);
    gpiod_request_config_free(request_config);
    gpiod_line_config_free(line_config);
    gpiod_line_settings_free(settings);
    gpiod_chip_close(chip);
}