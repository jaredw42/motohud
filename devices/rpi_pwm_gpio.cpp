#include <gpiod.h>
#include <poll.h>

#include <cstdio>
#include <ctime>

int main() {
    constexpr unsigned int bcm_pin = 17;
    constexpr double pulses_per_revolution = 1.0;

    gpiod_chip* chip = gpiod_chip_open("/dev/gpiochip0");
    if (!chip) {
        std::perror("gpiod_chip_open");
        return 1;
    }

    gpiod_line* line = gpiod_chip_get_line(chip, bcm_pin);
    if (!line ||
        gpiod_line_request_rising_edge_events(line, "tachometer") < 0) {
        std::perror("request GPIO edge events");
        gpiod_chip_close(chip);
        return 1;
    }

    pollfd descriptor{gpiod_line_event_get_fd(line), POLLIN, 0};
    timespec previous{};
    bool have_previous = false;

    for (;;) {
        if (poll(&descriptor, 1, -1) < 0) {
            std::perror("poll");
            break;
        }

        gpiod_line_event event{};
        if (gpiod_line_event_read(line, &event) < 0) {
            std::perror("read GPIO event");
            break;
        }

        if (have_previous) {
            const double seconds =
                (event.ts.tv_sec - previous.tv_sec) +
                (event.ts.tv_nsec - previous.tv_nsec) / 1e9;
            if (seconds > 0) {
                const double rpm = 60.0 / (seconds * pulses_per_revolution);
                std::printf("RPM: %.0f\n", rpm);
            }
        }

        previous = event.ts;
        have_previous = true;
    }

    gpiod_line_release(line);
    gpiod_chip_close(chip);
    return 0;
}