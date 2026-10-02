#pragma once

#include <stdint.h>

#include <QMainWindow>
#include <QPushButton>
#include <QStackedWidget>
#include <QTimer>

#include "devices/gnss_client.h"
#include "devices/rpi_pwm_gpio.h"
#include "widgets/gnss_status.h"
#include "widgets/lcd_display.h"
#include "widgets/retro_speedo_tach.h"
#include "widgets/speedometer_compass.h"
#include "widgets/speedometer_compass_config.h"

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);

protected:
  bool event(QEvent *e) override;

private slots:
  void onUiTick();
  void connectGnss(const QString &host, uint16_t port);
  void showPrevPage();
  void showNextPage();
  void toggleFullscreen();

private:
  void buildUi();
  void exitApplication();

private:
  QStackedWidget *pages_ = nullptr;
  QPushButton *prev_btn_ = nullptr;
  QPushButton *next_btn_ = nullptr;
  QPushButton *exit_btn_ = nullptr;
  QPushButton *fullscreen_btn_ = nullptr;

  SpeedometerCompass *speedometer_compass_ = nullptr;
  GnssStatus *gnss_status_ = nullptr;
  LcdDisplay *lcd_display_ = nullptr;
  RetroSpeedoTach *retro_speedo_tach_ = nullptr;
  SpeedometerCompassConfig *speedometer_config_ = nullptr;
  GnssClient *gnss_ = nullptr;
  RpiPwmGpio *tachometer_ = nullptr;

  QTimer ui_timer_;

  float fake_speed_val_ = 0.0f;
};