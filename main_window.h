#pragma once

#include <QMainWindow>
#include <QTimer>
#include <QStackedWidget>
#include <QPushButton>
class QLabel;
class GnssClient;  

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;
protected:
bool event(QEvent* e) override;
private slots:
    void onUiTick();   
    void showPrevPage();
    void showNextPage();

private:
    void buildUi();
    QWidget* buildPage1();   
    QWidget* buildPage2();  

    QStackedWidget* pages_ = nullptr;
    QPushButton* prev_btn_ = nullptr;
    QPushButton* next_btn_ = nullptr;
    
    QLabel* speed_value_ = nullptr;
    QLabel* heading_value_ = nullptr;
    QLabel* heading_degrees_value_ = nullptr;
    QLabel* time_value_ = nullptr;
    QLabel* date_value_ = nullptr; 
    QLabel* odo_value_ = nullptr;
    QLabel* fix_value_ = nullptr;
    QLabel* sv_value_ = nullptr; 

    QTimer ui_timer_;

    GnssClient* gnss_ = nullptr;

    float odo_distance_{};
    float fake_speed_val_{};
};