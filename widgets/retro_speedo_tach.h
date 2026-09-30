#pragma once

#include <stdint.h>

#include <QWidget>

#include "devices/gnss_client.h"

class QLCDNumber;
class RetroGauge;

class RetroSpeedoTach : public QWidget {
  Q_OBJECT

public:
  explicit RetroSpeedoTach(QWidget *parent = nullptr);

  void updateDisplay(const GnssPvt &gnss_state);
  void setEngineRpm(uint16_t rpm);
  void setDisconnected();

private:
  RetroGauge *speed_gauge_ = nullptr;
  RetroGauge *tach_gauge_ = nullptr;
  QLCDNumber *mileage_display_ = nullptr;
  QLCDNumber *time_display_ = nullptr;

  uint16_t dummy_speed_{0U};
  uint16_t dummy_rpm_{0U};
  float dummy_odo_{0.0f};
};
