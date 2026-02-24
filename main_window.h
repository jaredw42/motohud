#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QPushButton>
#include <QTimer>

#include "devices/gnss_client.h"
#include "widgets/speedometer_compass.h"
#include "widgets/gnss_status.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    bool event(QEvent* e) override;

private slots:
    void onUiTick();
    void showPrevPage();
    void showNextPage();

private:
    void buildUi();

private:
    QStackedWidget* pages_ = nullptr;
    QPushButton* prev_btn_ = nullptr;
    QPushButton* next_btn_ = nullptr;

    SpeedometerCompass* speedometer_compass_ = nullptr;
    GnssStatus* gnss_status_ = nullptr;

    GnssClient* gnss_ = nullptr;
    QTimer ui_timer_;

    float odo_distance_ = 0.0f;
    float fake_speed_val_ = 0.0f;
};