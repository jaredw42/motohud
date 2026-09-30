#pragma once

#include <QWidget>

#include "devices/gnss_client.h"
class QLCDNumber;

class LcdDisplay : public QWidget {
  Q_OBJECT

public:
  explicit LcdDisplay(QWidget *parent = nullptr);
  void updateDisplay(const GnssPvt &gnss_pvt);

private:
  QLCDNumber *gps_towsec_ = nullptr;
};
