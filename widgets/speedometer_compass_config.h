#pragma once

#include "speedometer_compass.h"
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QWidget>

class SpeedometerCompassConfig : public QWidget {
  Q_OBJECT

public:
  explicit SpeedometerCompassConfig(
      QWidget *parent = nullptr, SpeedometerCompass *speedo_compass = nullptr);

  void updateDisplay();

private:
  void buildUi();
  void toggleDateDisplay();

  QPushButton *show_elapsed_time_ = nullptr;
  QPushButton *reset_odometer_btn_ = nullptr;
  QCheckBox *show_date_ = nullptr;
  QCheckBox *show_estimated_speed_err_ = nullptr;
  QComboBox *background_color_ = nullptr;
  QComboBox *text_color = nullptr;
  SpeedometerCompass *speedometer_compass_ = nullptr;
};
// reset odometer value
// display date
// toggle current/elapsed time
// set background / text colors
