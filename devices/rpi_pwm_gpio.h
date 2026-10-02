#pragma once

#include <stdint.h>

#include <gpiod.h>

#include <QElapsedTimer>
#include <QObject>
#include <QSocketNotifier>

class RpiPwmGpio final : public QObject {
public:
  explicit RpiPwmGpio(QObject *parent = nullptr);
  ~RpiPwmGpio() override;

  bool isOpen() const { return notifier_ != nullptr; }
  uint16_t rpm() const;

private:
  void readEvents();
  void recordPulse(uint64_t timestamp_ns);

  gpiod_chip *chip_ = nullptr;
  QSocketNotifier *notifier_ = nullptr;
  QElapsedTimer last_pulse_elapsed_;
  uint64_t previous_timestamp_ns_ = 0;
  uint16_t rpm_ = 0;
  bool has_pulse_ = false;

#ifdef MOTOHUD_LIBGPIOD_V2
  gpiod_line_settings *settings_ = nullptr;
  gpiod_line_config *line_config_ = nullptr;
  gpiod_request_config *request_config_ = nullptr;
  gpiod_line_request *request_ = nullptr;
  gpiod_edge_event_buffer *event_buffer_ = nullptr;
#else
  gpiod_line *line_ = nullptr;
#endif
};