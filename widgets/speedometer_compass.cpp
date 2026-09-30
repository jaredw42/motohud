#include "widgets/speedometer_compass.h"

#include <QChar>
#include <QDate>
#include <QDateTime>
#include <QFont>
#include <QFrame>
#include <QGridLayout>
#include <QTime>
#include <QVBoxLayout>

static QFrame *makeTile(const QString &title, int primaryPt, int secondaryPt,
                        QLabel **outPrimary, QLabel **outSecondary) {
  auto *frame = new QFrame();
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

  auto *titleLabel = new QLabel(title);
  titleLabel->setObjectName("title");
  titleLabel->setAlignment(Qt::AlignCenter);

  QFont tf;
  tf.setPointSize(12);
  tf.setBold(false);
  titleLabel->setFont(tf);

  auto *primary = new QLabel("--");
  primary->setObjectName("primary");
  primary->setAlignment(Qt::AlignCenter);

  QFont pf;
  pf.setPointSize(primaryPt);
  pf.setBold(true);
  primary->setFont(pf);

  auto *secondary = new QLabel("");
  secondary->setObjectName("secondary");
  secondary->setAlignment(Qt::AlignCenter);

  QFont sf;
  sf.setPointSize(secondaryPt);
  sf.setBold(false);
  secondary->setFont(sf);

  secondary->setVisible(false);

  auto *layout = new QVBoxLayout(frame);
  layout->setContentsMargins(4, 2, 4, 2);
  layout->setSpacing(0);
  layout->addWidget(titleLabel);
  layout->addWidget(primary, 1);
  layout->addWidget(secondary);

  if (outPrimary)
    *outPrimary = primary;
  if (outSecondary)
    *outSecondary = secondary;

  return frame;
}

SpeedometerCompass::SpeedometerCompass(QWidget *parent) : QWidget(parent) {
  buildUi();
}

void SpeedometerCompass::buildUi() {
  // --- top row ---

  speed_tile_ = new Tile("miles per hour", this);
  speed_tile_->setPrimaryPointSize(speed_tile_cfg_.primary_point_size, true);
  speed_tile_->setSecondaryPointSize(speed_tile_cfg_.secondary_point_size,
                                     false);
  speed_tile_->setSecondaryVisible(speed_tile_cfg_.show_secondary);

  speed_value_ = speed_tile_->primaryLabel();
  estimated_speed_accuracy_value_ = speed_tile_->secondaryLabel();

  heading_tile_ = new Tile("compass", this);
  heading_tile_->setPrimaryPointSize(heading_tile_cfg_.primary_point_size,
                                     true);
  heading_tile_->setSecondaryPointSize(heading_tile_cfg_.secondary_point_size,
                                       false);
  heading_tile_->setSecondaryVisible(heading_tile_cfg_.show_secondary);

  heading_value_ = heading_tile_->primaryLabel();
  heading_degrees_value_ = heading_tile_->secondaryLabel();

  auto *top = new QGridLayout;
  top->setContentsMargins(0, 0, 0, 0);
  top->setSpacing(0);
  top->addWidget(speed_tile_, 0, 0);
  top->addWidget(heading_tile_, 0, 1);

  // --- bottom row ---
  time_tile_ = new Tile("time", this);
  time_tile_->setPrimaryPointSize(time_tile_cfg_.primary_point_size, true);
  time_tile_->setSecondaryPointSize(time_tile_cfg_.secondary_point_size, false);
  time_tile_->setSecondaryVisible(time_tile_cfg_.show_secondary);

  time_value_ = time_tile_->primaryLabel();
  date_value_ = time_tile_->secondaryLabel();

  odo_tile_ = new Tile("odo", this);
  odo_tile_->setPrimaryPointSize(odo_tile_cfg_.primary_point_size, true);
  odo_tile_->setSecondaryPointSize(odo_tile_cfg_.secondary_point_size, false);
  odo_tile_->setSecondaryVisible(odo_tile_cfg_.show_secondary);

  odo_value_ = odo_tile_->primaryLabel();
  // (odo secondary currently unused; stays hidden)

  sv_tile_ = new Tile("sats", this);
  sv_tile_->setPrimaryPointSize(sv_tile_cfg_.primary_point_size, true);
  sv_tile_->setSecondaryPointSize(sv_tile_cfg_.secondary_point_size, false);
  sv_tile_->setSecondaryVisible(sv_tile_cfg_.show_secondary);

  sv_value_ = sv_tile_->primaryLabel();
  fix_value_ = sv_tile_->secondaryLabel();

  auto *bottom = new QGridLayout;
  bottom->setContentsMargins(0, 0, 0, 0);
  bottom->setSpacing(0);
  bottom->addWidget(time_tile_, 0, 0);
  bottom->addWidget(odo_tile_, 0, 1);
  bottom->addWidget(sv_tile_, 0, 2);

  auto *v = new QVBoxLayout(this);
  v->setContentsMargins(4, 2, 4, 2);
  v->setSpacing(0);
  v->addLayout(top, 2);
  v->addLayout(bottom, 1);
}

void SpeedometerCompass::setDisconnected() {
  if (speed_value_)
    speed_value_->setText("--");
  if (heading_value_)
    heading_value_->setText("--");
  if (time_value_)
    time_value_->setText("--:--:--");
  if (odo_value_)
    odo_value_->setText("--");
  if (sv_value_)
    sv_value_->setText("DISCONNECTED");

  if (heading_degrees_value_) {
    heading_degrees_value_->setText("");
    heading_degrees_value_->setVisible(false);
  }

  if (fix_value_) {
    fix_value_->setText("");
    fix_value_->setVisible(false);
  }

  if (date_value_) {
    date_value_->setText("");
    date_value_->setVisible(false);
  }
}

void SpeedometerCompass::updateDisplay(const GnssPvt &gnss_state) {
  if (speed_value_) {
    speed_value_->setText(QString::number(gnss_state.sog_mph, 'f', 0));
    speed_tile_->setPrimaryPointSize(speed_tile_cfg_.primary_point_size);
  }

  if (speed_tile_cfg_.show_secondary) {

    const auto est_speed_acc =
        gnss_state.estimated_speed_accuracy * kMillimetersPerSecToMilesPerHour;
    if (estimated_speed_accuracy_value_) {

      estimated_speed_accuracy_value_->setText(QString::number(est_speed_acc));
      estimated_speed_accuracy_value_->setVisible(true);
    }
  }

  if (heading_value_)
    heading_value_->setText(gnss_state.cardinal_direction);

  if (heading_tile_cfg_.show_secondary) {
    if (heading_degrees_value_) {
      heading_degrees_value_->setText(
          QString::number(gnss_state.heading, 'f', 1) + QChar(0x00B0));
      heading_degrees_value_->setVisible(true);
    }
  }

  const auto dt = gnss_state.utc_datetime;
  QDate date(dt[0], dt[1], dt[2]);
  QTime time(dt[3], dt[4], dt[5]);
  QDateTime datetime(date, time, Qt::UTC);
  const QDateTime local = datetime.toLocalTime();

  if (time_value_)
    time_value_->setText(local.toString("hh:mm:ss"));

  if (date_value_) {
    date_value_->setText(local.toString("yyyy-MM-dd"));
    date_value_->setVisible(time_tile_cfg_.show_secondary);
  }
  current_gnss_odo_val_ = gnss_state.odometer_m;
  const auto dist =
      (current_gnss_odo_val_ - start_gnss_odo_val_) / kMilesToMeters;
  if (odo_value_)

    odo_value_->setText(QString::number(dist, 'f', 1));

  if (sv_value_)
    sv_value_->setText(QString::number(gnss_state.num_sv));

  if (fix_value_) {

    const QString fixstr = QString::fromStdString(gnss_state.differential_mode);
    fix_value_->setText(fixstr);
    fix_value_->setVisible(true);
  }
}

void SpeedometerCompass::resetOdometer() {
  start_gnss_odo_val_ = current_gnss_odo_val_;
}

void SpeedometerCompass::setShowEstimatedSpeedError(bool show) {
  static constexpr uint8_t kDefaultPrimaryPointSize = 108;
  static constexpr uint8_t kPrimaryPointSizeNoSecondary = 132;

  speed_tile_cfg_.primary_point_size =
      show ? kDefaultPrimaryPointSize : kPrimaryPointSizeNoSecondary;
  speed_tile_cfg_.show_secondary = show;

  estimated_speed_accuracy_value_->setVisible(show);
}

void SpeedometerCompass::setShowDate(bool show) {
  static constexpr uint8_t kDefaultPrimaryPointSize = 36;
  static constexpr uint8_t kPrimaryPointSizeNoSecondary = 48;

  time_tile_cfg_.primary_point_size =
      show ? kDefaultPrimaryPointSize : kPrimaryPointSizeNoSecondary;
  time_tile_cfg_.show_secondary = show;
  date_value_->setVisible(show);
}

void SpeedometerCompass::setShowHeadingDegrees(bool show) {
  static constexpr uint8_t kDefaultPrimaryPointSize = 96;
  static constexpr uint8_t kPrimaryPointSizeNoSecondary = 128;

  heading_tile_cfg_.primary_point_size =
      show ? kDefaultPrimaryPointSize : kPrimaryPointSizeNoSecondary;
  heading_tile_cfg_.show_secondary = show;

  heading_degrees_value_->setVisible(show);
}