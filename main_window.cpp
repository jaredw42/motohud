#include "main_window.h"

#include <cstdlib>

#include <QGestureEvent>
#include <QHBoxLayout>
#include <QSwipeGesture>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  buildUi();

  gnss_ = new GnssClient(this);
  const QString gnss_host = QStringLiteral("127.0.0.1");
  constexpr uint16_t gnss_port = 8100;
  gnss_status_->setConnectionEndpoint(gnss_host, gnss_port);
  gnss_->connectTcp(gnss_host, gnss_port);

  ui_timer_.setInterval(200); // 5 Hz
  connect(&ui_timer_, &QTimer::timeout, this, &MainWindow::onUiTick);
  ui_timer_.start();
}

void MainWindow::buildUi() {
  auto *root = new QWidget(this);
  setCentralWidget(root);

  pages_ = new QStackedWidget(root);

  speedometer_compass_ = new SpeedometerCompass(pages_);
  speedometer_config_ =
      new SpeedometerCompassConfig(pages_, speedometer_compass_);
  gnss_status_ = new GnssStatus(pages_);
  lcd_display_ = new LcdDisplay(pages_);
  connect(gnss_status_, &GnssStatus::connectionRequested, this,
          &MainWindow::connectGnss);

  pages_->addWidget(speedometer_compass_);
  pages_->addWidget(speedometer_config_);
  pages_->addWidget(gnss_status_);
  pages_->addWidget(lcd_display_);

  pages_->grabGesture(Qt::SwipeGesture);

  prev_btn_ = new QPushButton("◀", root);
  next_btn_ = new QPushButton("▶", root);
  exit_btn_ = new QPushButton("EXIT APP");

  connect(prev_btn_, &QPushButton::clicked, this, &MainWindow::showPrevPage);
  connect(next_btn_, &QPushButton::clicked, this, &MainWindow::showNextPage);
  connect(exit_btn_, &QPushButton::clicked, this, &MainWindow::exitApplication);

  auto *nav = new QHBoxLayout;
  nav->setContentsMargins(4, 2, 4, 2);
  nav->setSpacing(6);

  nav->addWidget(prev_btn_);
  nav->addStretch(1);        // left flexible space
  nav->addWidget(exit_btn_); // centered
  nav->addStretch(1);        // right flexible space
  nav->addWidget(next_btn_);

  auto *outer = new QVBoxLayout(root);
  outer->setContentsMargins(0, 0, 0, 0);
  outer->setSpacing(0);
  outer->addWidget(pages_, 1);
  outer->addLayout(nav);
}

void MainWindow::onUiTick() {
  if (gnss_ == nullptr)
    return;

  if (!gnss_->isConnected()) {
    if (speedometer_compass_)
      speedometer_compass_->setDisconnected();
    if (gnss_status_)
      gnss_status_->setDisconnected();
    return;
  }

  const GnssPvt &gnss_state = gnss_->state();

  if (speedometer_compass_)
    speedometer_compass_->updateDisplay(gnss_state);

  if (gnss_status_)
    gnss_status_->updateDisplay(gnss_state);

  if (lcd_display_) {
    lcd_display_->updateDisplay(gnss_state);
  }
}

void MainWindow::connectGnss(const QString &host, uint16_t port) {
  if (gnss_)
    gnss_->connectTcp(host, port);
}

void MainWindow::showPrevPage() {
  if (!pages_)
    return;
  const int n = pages_->count();
  const int i = pages_->currentIndex();
  pages_->setCurrentIndex((i - 1 + n) % n);
}

void MainWindow::showNextPage() {
  if (!pages_)
    return;
  const int n = pages_->count();
  const int i = pages_->currentIndex();
  pages_->setCurrentIndex((i + 1) % n);
}

bool MainWindow::event(QEvent *e) {
  if (e->type() == QEvent::Gesture) {
    auto *ge = static_cast<QGestureEvent *>(e);
    if (auto *swipe =
            static_cast<QSwipeGesture *>(ge->gesture(Qt::SwipeGesture))) {
      if (swipe->state() == Qt::GestureFinished) {
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

void MainWindow::exitApplication() { std::exit(EXIT_SUCCESS); }