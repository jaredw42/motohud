#include "speedometer_compass_config.h"

#include <QGridLayout>
#include <QVBoxLayout>

SpeedometerCompassConfig::SpeedometerCompassConfig(
    QWidget *parent, SpeedometerCompass *speedo_compass)
    : QWidget(parent), speedometer_compass_(speedo_compass) {
  buildUi();
}

void SpeedometerCompassConfig::buildUi() {

  static constexpr auto kCheckBoxStyle = R"(
QCheckBox {
    spacing: 10px;
    color: #F0F0F0;
}
QCheckBox::indicator {
    width: 28px;
    height: 28px;
}
QCheckBox::indicator:unchecked {
    border: 2px solid #888;
    background: #101010;
}
QCheckBox::indicator:checked {
    border: 2px solid rgb(27, 86, 30);
    background: #2e7d32;
}
)";

  reset_odometer_btn_ = new QPushButton("RESET ODOMETER");
  show_date_ = new QCheckBox("SHOW DATE");
  show_estimated_speed_err_ = new QCheckBox("Show Estimated Speed Error");

  show_estimated_speed_err_->setStyleSheet(kCheckBoxStyle);
  show_date_->setStyleSheet(kCheckBoxStyle);

  QFont f = show_estimated_speed_err_->font();
  f.setPointSize(18);
  f.setBold(true); // optional
  show_estimated_speed_err_->setFont(f);

  connect(reset_odometer_btn_, &QPushButton::clicked, speedometer_compass_,
          &SpeedometerCompass::resetOdometer);
  connect(show_estimated_speed_err_, &QCheckBox::toggled, speedometer_compass_,
          &SpeedometerCompass::setShowEstimatedSpeedError);
  connect(show_date_, &QCheckBox::toggled, speedometer_compass_,
          &SpeedometerCompass::setShowDate);

  auto *top = new QGridLayout;
  top->setContentsMargins(2, 4, 2, 4);
  top->setSpacing(3);
  top->addWidget(reset_odometer_btn_, 0, 0);
  top->addWidget(show_date_, 0, 1);
  top->addWidget(show_estimated_speed_err_);

  auto *v = new QVBoxLayout(this);
  v->setContentsMargins(4, 2, 4, 2);
  v->setSpacing(0);
  v->addLayout(top, 2);
  //   v->addLayout(bottom, 1);
}

void SpeedometerCompassConfig::updateDisplay() {}

void SpeedometerCompassConfig::toggleDateDisplay() {}
