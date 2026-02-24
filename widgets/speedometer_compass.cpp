#include "widgets/speedometer_compass.h"

#include <QFrame>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QFont>
#include <QChar>
#include <QDateTime>
#include <QDate>
#include <QTime>

static QFrame* makeTile(const QString& title,
                        int primaryPt,
                        int secondaryPt,
                        QLabel** outPrimary,
                        QLabel** outSecondary)
{
    auto* frame = new QFrame();
    frame->setFrameShape(QFrame::StyledPanel);
    frame->setFrameShadow(QFrame::Plain);

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
    primary->setObjectName("primary");
    primary->setAlignment(Qt::AlignCenter);

    QFont pf;
    pf.setPointSize(primaryPt);
    pf.setBold(true);
    primary->setFont(pf);

    auto* secondary = new QLabel("");
    secondary->setObjectName("secondary");
    secondary->setAlignment(Qt::AlignCenter);

    QFont sf;
    sf.setPointSize(secondaryPt);
    sf.setBold(false);
    secondary->setFont(sf);

    secondary->setVisible(false);

    auto* layout = new QVBoxLayout(frame);
    layout->setContentsMargins(4, 2, 4, 2);
    layout->setSpacing(0);
    layout->addWidget(titleLabel);
    layout->addWidget(primary, 1);
    layout->addWidget(secondary);

    if (outPrimary)   *outPrimary = primary;
    if (outSecondary) *outSecondary = secondary;

    return frame;
}

SpeedometerCompass::SpeedometerCompass(QWidget* parent)
    : QWidget(parent)
{
    buildUi();
}

void SpeedometerCompass::buildUi()
{
    QLabel* speed_secondary = nullptr;
    QLabel* heading_secondary = nullptr;

    auto* speedTile = makeTile("miles per hour", 108, 16,
                               &speed_value_, &speed_secondary);

    auto* headingTile = makeTile("compass", 96, 36,
                                 &heading_value_, &heading_secondary);

    heading_degrees_value_ = heading_secondary;

    auto* top = new QGridLayout;
    top->setContentsMargins(0, 0, 0, 0);
    top->setSpacing(0);
    top->addWidget(speedTile,   0, 0);
    top->addWidget(headingTile, 0, 1);

    QLabel* time_secondary = nullptr;
    QLabel* odo_secondary  = nullptr;
    QLabel* fix_secondary  = nullptr;

    auto* timeTile = makeTile("time", 36, 18,
                              &time_value_, &time_secondary);

    auto* odoTile = makeTile("odo", 36, 12,
                             &odo_value_, &odo_secondary);

    auto* fixTile = makeTile("sats", 36, 18,
                             &sv_value_, &fix_secondary);

    fix_value_ = fix_secondary;
    date_value_ = time_secondary;

    auto* bottom = new QGridLayout;
    bottom->setContentsMargins(0, 0, 0, 0);
    bottom->setSpacing(0);
    bottom->addWidget(timeTile, 0, 0);
    bottom->addWidget(odoTile,  0, 1);
    bottom->addWidget(fixTile,  0, 2);

    auto* v = new QVBoxLayout(this);
    v->setContentsMargins(4, 2, 4, 2);
    v->setSpacing(0);
    v->addLayout(top, 2);
    v->addLayout(bottom, 1);
}

void SpeedometerCompass::setDisconnected()
{
    if (speed_value_)   speed_value_->setText("--");
    if (heading_value_) heading_value_->setText("--");
    if (time_value_)    time_value_->setText("--:--:--");
    if (odo_value_)     odo_value_->setText("--");
    if (sv_value_)      sv_value_->setText("DISCONNECTED");

    if (heading_degrees_value_)
    {
        heading_degrees_value_->setText("");
        heading_degrees_value_->setVisible(false);
    }

    if (fix_value_)
    {
        fix_value_->setText("");
        fix_value_->setVisible(false);
    }

    if (date_value_)
    {
        date_value_->setText("");
        date_value_->setVisible(false);
    }
}

void SpeedometerCompass::updateFromGnss(const GnssPvt& s, float odo_miles)
{
    if (speed_value_)
        speed_value_->setText(QString::number(s.sog_mph, 'f', 0));

    if (heading_value_)
        heading_value_->setText(s.cardinal_direction);

    if (heading_degrees_value_)
    {
        heading_degrees_value_->setText(QString::number(s.heading, 'f', 1) + QChar(0x00B0));
        heading_degrees_value_->setVisible(true);
    }

    const auto dt = s.utc_datetime;
    QDate date(dt[0], dt[1], dt[2]);
    QTime time(dt[3], dt[4], dt[5]);
    QDateTime datetime(date, time, Qt::UTC);
    const QDateTime local = datetime.toLocalTime();

    if (time_value_)
        time_value_->setText(local.toString("hh:mm:ss"));

    if (date_value_)
    {
        date_value_->setText(local.toString("yyyy-MM-dd"));
        date_value_->setVisible(true);
    }

    if (odo_value_)
        odo_value_->setText(QString::number(odo_miles, 'f', 1));

    if (sv_value_)
        sv_value_->setText(QString::number(s.num_sv));

    if (fix_value_)
    {
        // NO AGE. Only differential mode text.
        const QString fixstr = QString::fromStdString(s.differential_mode);
        fix_value_->setText(fixstr);
        fix_value_->setVisible(true);
    }
}