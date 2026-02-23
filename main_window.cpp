#include "main_window.h"

#include <iostream>
#include <QLabel>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QWidget>
#include <QFont>
#include <QChar>
#include <QString>
#include <QDateTime>
#include <QFrame>
#include <QGestureEvent>
#include <QSwipeGesture>

#include "devices/gnss_client.h"

// A small "tile": title label, big value, optional small value.
// Returns the container; outputs pointers to the value labels you update later.
static QFrame* makeTile(const QString& title,
                        int primaryPt,
                        int secondaryPt,
                        QLabel** outPrimary,
                        QLabel** outSecondary)
{
    auto* frame = new QFrame();
    frame->setFrameShape(QFrame::StyledPanel);
    frame->setFrameShadow(QFrame::Plain);

    // This stylesheet draws the lines between widgets (tile borders)
    // and makes the title smaller/dimmer.
   frame->setStyleSheet(R"(
QFrame {
    border: 2px solid #404040;
    border-radius: 2px;
    background: #101010;
}
QLabel { padding: 0px; margin: 0px; }

QLabel#title     { color: #B0B0B0; }
QLabel#primary   { color: #F0F0F0; }
QLabel#secondary { color: #C8C8C8; }
)");
    auto* titleLabel = new QLabel(title);
    titleLabel->setObjectName("title");
    titleLabel->setAlignment(Qt::AlignCenter);

    QFont tf;
    tf.setPointSize(12);
    tf.setBold(false);
    titleLabel->setFont(tf);

    auto* primary = new QLabel("--");
    primary->setAlignment(Qt::AlignCenter);
    QFont pf;
    pf.setPointSize(primaryPt);
    pf.setBold(true);
    primary->setFont(pf);

    auto* secondary = new QLabel("");
    secondary->setAlignment(Qt::AlignCenter);
    QFont sf;
    sf.setPointSize(secondaryPt);
    sf.setBold(false);
    secondary->setFont(sf);

    // If you don't want the secondary line to take space when empty:
    secondary->setVisible(false);

    primary->setObjectName("primary");
    secondary->setObjectName("secondary");

    auto* layout = new QVBoxLayout(frame);
    layout->setContentsMargins(4,2,4,2);
    layout->setSpacing(0);
    layout->addWidget(titleLabel);
    layout->addWidget(primary, 1);
    layout->addWidget(secondary);

    if (outPrimary)   *outPrimary = primary;
    if (outSecondary) *outSecondary = secondary;

    return frame;
}



MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    buildUi();

    gnss_ = new GnssClient(this);
    gnss_->connectTcp("192.168.0.151", 8100);
    // 5 Hz UI update loop (polling)
    ui_timer_.setInterval(200); // ms
    connect(&ui_timer_, &QTimer::timeout, this, &MainWindow::onUiTick);
    ui_timer_.start();
}

void MainWindow::buildUi()
{
    auto* root = new QWidget(this);
    setCentralWidget(root);

    pages_ = new QStackedWidget(root);
    pages_->addWidget(buildPage1());
    pages_->addWidget(buildPage2());
    pages_->grabGesture(Qt::SwipeGesture);
    prev_btn_ = new QPushButton("◀", root);
    next_btn_ = new QPushButton("▶", root);

    // Hook up buttons
    connect(prev_btn_, &QPushButton::clicked, this, &MainWindow::showPrevPage);
    connect(next_btn_, &QPushButton::clicked, this, &MainWindow::showNextPage);

    // Layout: pages + small nav row
    auto* nav = new QHBoxLayout;
    nav->setContentsMargins(4,2,4,2);
    nav->setSpacing(6);
    nav->addWidget(prev_btn_);
    nav->addStretch(1);
    nav->addWidget(next_btn_);

    auto* outer = new QVBoxLayout(root);
    outer->setContentsMargins(0,0,0,0);
    outer->setSpacing(0);
    outer->addWidget(pages_, 1);
    outer->addLayout(nav);
}

QWidget* MainWindow::buildPage1()
{
    auto* page = new QWidget();

    // Top 2/3 (2 columns)
    QLabel* speed_secondary = nullptr;
    QLabel* heading_secondary = nullptr;

    auto* speedTile   = makeTile("miles per hour", 108, 16,
                                 &speed_value_, &speed_secondary);

    auto* headingTile = makeTile("compass", 96, 36,
                                 &heading_value_, &heading_secondary);

    heading_degrees_value_ = heading_secondary;
    
    auto* top = new QGridLayout;
    top->setContentsMargins(0, 0, 0, 0);
    top->setSpacing(0);
    top->addWidget(speedTile,   0, 0);
    top->addWidget(headingTile, 0, 1);

    // Bottom 1/3 (3 columns)
    QLabel* time_secondary = nullptr;
    QLabel* odo_secondary  = nullptr;
    QLabel* fix_secondary  = nullptr;

    auto* timeTile = makeTile("time", 36, 18,
                              &time_value_, &time_secondary);

    auto* odoTile  = makeTile("odo", 36, 12,
                              &odo_value_, &odo_secondary);

    auto* fixTile  = makeTile("sats", 36, 18,
                              &sv_value_, &fix_secondary);
    
    fix_value_ = fix_secondary; 
    date_value_ = time_secondary; 
    auto* bottom = new QGridLayout;
    bottom->setContentsMargins(0, 0, 0, 0);
    bottom->setSpacing(0);
    bottom->addWidget(timeTile, 0, 0);
    bottom->addWidget(odoTile,  0, 1);
    bottom->addWidget(fixTile,  0, 2);

    auto* v = new QVBoxLayout(page);
    v->setContentsMargins(4, 2, 4, 2);
    v->setSpacing(0);
    v->addLayout(top, 2);
    v->addLayout(bottom, 1);

    return page;
}

QWidget* MainWindow::buildPage2()
{
    auto* page = new QWidget();
    auto* layout = new QVBoxLayout(page);

    auto* label = new QLabel("PAGE 2");
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label, 1);

    return page;
}


void MainWindow::onUiTick()
{
    // Poll GNSS state at 5 Hz. No UI updates from socket callbacks.
    // This function is the only place the screen is updated.

    if (gnss_ == nullptr)
        return;

    if (!gnss_->isConnected())
    {
        speed_value_->setText("--");
        heading_value_->setText("--");
        time_value_->setText("--:--:--");
        odo_value_->setText("--");
        sv_value_->setText(QString("DISCONNECTED"));
        return;
    }

    const GnssPvt& s = gnss_->state();

    // speed_value_->setText(QString::number(s.sog_mph, 'f', 0));
    speed_value_->setText(QString::number(fake_speed_val_, 'f', 0));

    heading_value_->setText(s.cardinal_direction);

    if (heading_degrees_value_ != nullptr)
    {
        heading_degrees_value_->setText(QString::number(s.heading, 'f', 1) + QChar(0x00B0));
        heading_degrees_value_->setVisible(true);
    }

    
    const auto dt = s.utc_datetime;
    QDate date(dt[0], dt[1], dt[2]);
    QTime time(dt[3], dt[4], dt[5]);

    QDateTime datetime(date, time, Qt::UTC);

    time_value_->setText(datetime.toLocalTime().toString("hh:mm:ss"));
    odo_value_->setText(QString::number(odo_distance_, 'f', 1));
    sv_value_->setText(QString::number(s.num_sv));

    if (fix_value_ != nullptr) {
        QString agestr = QString::number(s.correction_age);
        QString fixstr = QString::fromStdString(s.differential_mode);
        // fixstr.append(QString::fromStdString(" - "));
        // fixstr.append(agestr);
        fix_value_->setText(fixstr);
        fix_value_->setVisible(true);
    }

    if (date_value_ != nullptr) {
        date_value_->setText(datetime.toLocalTime().toString("yyyy-MM-dd"));
        date_value_->setVisible(true);
    }

    odo_distance_ += 0.02;
    fake_speed_val_ += 0.02;
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