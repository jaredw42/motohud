#include "main_window.h"

#include <cstdlib>

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGestureEvent>
#include <QSwipeGesture>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    buildUi();

    gnss_ = new GnssClient(this);
    gnss_->connectTcp("192.168.0.151", 8100);

    ui_timer_.setInterval(200); // 5 Hz
    connect(&ui_timer_, &QTimer::timeout, this, &MainWindow::onUiTick);
    ui_timer_.start();
}

void MainWindow::buildUi()
{
    auto* root = new QWidget(this);
    setCentralWidget(root);

    pages_ = new QStackedWidget(root);

    speedometer_compass_ = new SpeedometerCompass(pages_);
    gnss_status_ = new GnssStatus(pages_);

    pages_->addWidget(speedometer_compass_);
    pages_->addWidget(gnss_status_);

    pages_->grabGesture(Qt::SwipeGesture);

    prev_btn_ = new QPushButton("◀", root);
    next_btn_ = new QPushButton("▶", root);
    exit_btn_ = new QPushButton("EXIT APP");

    connect(prev_btn_, &QPushButton::clicked, this, &MainWindow::showPrevPage);
    connect(next_btn_, &QPushButton::clicked, this, &MainWindow::showNextPage);
    connect(exit_btn_, &QPushButton::clicked, this, &MainWindow::exitApplication);

    auto* nav = new QHBoxLayout;
    nav->setContentsMargins(4, 2, 4, 2);
    nav->setSpacing(6);

    nav->addWidget(prev_btn_);
    nav->addStretch(1);        // left flexible space
    nav->addWidget(exit_btn_); // centered
    nav->addStretch(1);        // right flexible space
    nav->addWidget(next_btn_);

    auto* outer = new QVBoxLayout(root);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);
    outer->addWidget(pages_, 1);
    outer->addLayout(nav);
}

void MainWindow::onUiTick()
{
    if (gnss_ == nullptr)
        return;

    if (!gnss_->isConnected())
    {
        if (speedometer_compass_) speedometer_compass_->setDisconnected();
        if (gnss_status_) gnss_status_->setDisconnected();
        return;
    }

    const GnssPvt& s = gnss_->state();

    if (speedometer_compass_)
        speedometer_compass_->updateFromGnss(s, odo_distance_);

    if (gnss_status_)
        gnss_status_->updateFromGnss(s);

   
}

void MainWindow::showPrevPage()
{
    if (!pages_) return;
    const int n = pages_->count();
    const int i = pages_->currentIndex();
    pages_->setCurrentIndex((i - 1 + n) % n);
}

void MainWindow::showNextPage()
{
    if (!pages_) return;
    const int n = pages_->count();
    const int i = pages_->currentIndex();
    pages_->setCurrentIndex((i + 1) % n);
}

bool MainWindow::event(QEvent* e)
{
    if (e->type() == QEvent::Gesture)
    {
        auto* ge = static_cast<QGestureEvent*>(e);
        if (auto* swipe = static_cast<QSwipeGesture*>(ge->gesture(Qt::SwipeGesture)))
        {
            if (swipe->state() == Qt::GestureFinished)
            {
                if (swipe->horizontalDirection() == QSwipeGesture::Left)
                    showNextPage();
                else if (swipe->horizontalDirection() == QSwipeGesture::Right)
                    showPrevPage();
            }
            return true;
        }
    }

    return QMainWindow::event(e);
}

void MainWindow::exitApplication() {

    std::exit(EXIT_SUCCESS);
}