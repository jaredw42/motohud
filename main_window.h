#pragma once

#include <QMainWindow>
#include <QTimer>

class QLabel;
class GnssClient;   // forward declare your existing class

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void onUiTick();   // 5 Hz UI refresh

private:
    void buildUi();

private:
    // UI labels (Run 1 layout)
    QLabel* speed_value_ = nullptr;
    QLabel* heading_value_ = nullptr;
    QLabel* heading_degrees_value_ = nullptr;
    QLabel* time_value_ = nullptr;
    QLabel* odo_value_ = nullptr;
    QLabel* fix_value_ = nullptr;

    QTimer ui_timer_;

    GnssClient* gnss_ = nullptr;

    float odo_distance_{};
};