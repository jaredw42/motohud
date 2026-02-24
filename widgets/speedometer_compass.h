#pragma once

#include <QWidget>
#include <QLabel>

#include "devices/gnss_client.h"

class SpeedometerCompass : public QWidget
{
    Q_OBJECT

public:
    explicit SpeedometerCompass(QWidget* parent = nullptr);

    void setDisconnected();
    void updateFromGnss(const GnssPvt& s, float odo_miles);

private:
    void buildUi();

private:
    QLabel* speed_value_ = nullptr;

    QLabel* heading_value_ = nullptr;
    QLabel* heading_degrees_value_ = nullptr;

    QLabel* time_value_ = nullptr;
    QLabel* date_value_ = nullptr;

    QLabel* odo_value_ = nullptr;

    QLabel* sv_value_ = nullptr;
    QLabel* fix_value_ = nullptr;
};