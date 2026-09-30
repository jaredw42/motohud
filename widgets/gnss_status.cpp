#include "widgets/gnss_status.h"

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
GnssStatus::GnssStatus(QWidget *parent) : QWidget(parent) { buildUi(); }

void GnssStatus::buildUi() {

  // fix table
  fix_table_ = new QTableWidget(6, 1, this);
  svs_used_for_nav_value_ = new QTableWidgetItem("0");
  fix_value_ = new QTableWidgetItem("0");
  correction_age_value_ = new QTableWidgetItem("0");
  estimated_position_accuracy_value_ = new QTableWidgetItem("0");
  estimated_speed_accuracy_value_ = new QTableWidgetItem("0");

  fix_table_->setItem(0, 0, svs_used_for_nav_value_);
  fix_table_->setItem(1, 0, fix_value_);
  fix_table_->setItem(2, 0, correction_age_value_);
  fix_table_->setItem(3, 0, estimated_position_accuracy_value_);
  fix_table_->setItem(4, 0, estimated_speed_accuracy_value_);

  QStringList fix_table_rows = {"sats used", "fix mode", "correction age",
                                "est horiz error [m]", "est sog error [mph] "};
  fix_table_->setVerticalHeaderLabels(fix_table_rows);
  // ned component table
  baseline_ = new QTableWidget(5, 2, this);
  baseline_distance_n_ = new QTableWidgetItem("0");
  baseline_distance_e_ = new QTableWidgetItem("0");
  baseline_distance_d_ = new QTableWidgetItem("0");
  baseline_distance_2d_ = new QTableWidgetItem("0");
  baseline_distance_3d_ = new QTableWidgetItem("0");
  velocity_n_ = new QTableWidgetItem("0");
  velocity_e_ = new QTableWidgetItem("0");
  velocity_d_ = new QTableWidgetItem("0");
  velocity_2d_ = new QTableWidgetItem("0");
  velocity_3d_ = new QTableWidgetItem("0");

  baseline_->setItem(0, 0, baseline_distance_n_);
  baseline_->setItem(1, 0, baseline_distance_e_);
  baseline_->setItem(2, 0, baseline_distance_d_);
  baseline_->setItem(3, 0, baseline_distance_2d_);
  baseline_->setItem(4, 0, baseline_distance_3d_);
  baseline_->setItem(0, 1, velocity_n_);
  baseline_->setItem(1, 1, velocity_e_);
  baseline_->setItem(2, 1, velocity_d_);
  baseline_->setItem(3, 1, velocity_2d_);
  baseline_->setItem(4, 1, velocity_3d_);

  QStringList baseline_names = {"north", "east", "down", "horizontal",
                                "spherical"};
  QStringList column_names = {"dist [m]", "velocity [m/s]"};
  baseline_->setVerticalHeaderLabels(baseline_names);
  baseline_->setHorizontalHeaderLabels(column_names);

  auto *top = new QGridLayout;
  top->setContentsMargins(0, 0, 0, 0);
  top->setSpacing(0);

  top->addWidget(fix_table_, 0, 0);
  top->addWidget(baseline_, 0, 1);

  auto *v = new QVBoxLayout(this);
  v->setContentsMargins(4, 2, 4, 2);
  v->setSpacing(0);
  v->addLayout(top, 2);
}

void GnssStatus::setDisconnected() {
  if (!label_)
    return;
  label_->setText("DISCONNECTED");
}

void GnssStatus::updateDisplay(const GnssPvt &gnss_state) {
  // if (!label_)
  //   return;

  // // Keep it dumb/simple for now. You can expand into real tiles later.
  // label_->setText(QString("SV: %1  Mode: %2")
  //                     .arg(s.num_sv)
  //                     .arg(QString::fromStdString(s.differential_mode)));

  if (svs_used_for_nav_value_) {
    svs_used_for_nav_value_->setText(QString::number(gnss_state.num_sv));
  }

  if (fix_value_) {
    const QString fixstr = QString::fromStdString(gnss_state.differential_mode);
    fix_value_->setText(fixstr);
  }

  if (correction_age_value_) {
    correction_age_value_->setText(QString::number(gnss_state.correction_age));
  }

  if (estimated_position_accuracy_value_) {
    estimated_position_accuracy_value_->setText(
        QString::number(gnss_state.estimated_horizontal_pos_accuracy));
  }

  if (baseline_distance_n_) {
    baseline_distance_n_->setText(QString::number(gnss_state.baseline_ned[0]));
    baseline_distance_e_->setText(QString::number(gnss_state.baseline_ned[1]));
    baseline_distance_d_->setText(QString::number(gnss_state.baseline_ned[2]));
    baseline_distance_2d_->setText(QString::number(gnss_state.baseline_ned[3]));
    baseline_distance_3d_->setText(QString::number(gnss_state.baseline_ned[4]));
    velocity_n_->setText(QString::number(gnss_state.velocity_n));
    velocity_e_->setText(QString::number(gnss_state.velocity_e));
    velocity_d_->setText(QString::number(gnss_state.velocity_d));
    velocity_2d_->setText(QString::number(gnss_state.velocity_2d));
    velocity_3d_->setText(QString::number(gnss_state.velocity_3d));
  }
}