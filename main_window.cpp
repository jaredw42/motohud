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

    // Top 2/3 (2 columns)
    QLabel* speed_secondary = nullptr;   // you can ignore if unused
    QLabel* heading_secondary = nullptr; 

    auto* speedTile   = makeTile("miles per hour", 64, 16, &speed_value_, &speed_secondary);
    auto* headingTile = makeTile("compass", 48, 36, &heading_value_, &heading_secondary);

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

    auto* timeTile = makeTile("time", 20, 12, &time_value_, &time_secondary);
    auto* odoTile  = makeTile("odo", 36, 12, &odo_value_, &odo_secondary);
    auto* fixTile  = makeTile("sats", 36, 12, &fix_value_, &fix_secondary);

    auto* bottom = new QGridLayout;
    bottom->setContentsMargins(0, 0, 0, 0);
    bottom->setSpacing(0);
    bottom->addWidget(timeTile, 0, 0);
    bottom->addWidget(odoTile,  0, 1);
    bottom->addWidget(fixTile,  0, 2);

    auto* v = new QVBoxLayout(root);
    v->setContentsMargins(4,2,4,2);
    v->setSpacing(0);
    v->addLayout(top, 2);
    v->addLayout(bottom, 1);
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
        fix_value_->setText(QString("DISCONNECTED"));
        return;
    }

    const GnssPvt& s = gnss_->state();

    speed_value_->setText(QString::number(s.sog_mph, 'f', 3));

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

    time_value_->setText(datetime.toLocalTime().toString("hh:mm:ss\nyyyy-MM-dd"));
    odo_value_->setText(QString::number(odo_distance_, 'f', 1));
    fix_value_->setText(QString::number(s.num_sv));

    odo_distance_ += 0.02;
}