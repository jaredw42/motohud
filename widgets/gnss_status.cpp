#include "widgets/gnss_status.h"

#include <QHBoxLayout>
#include <QHostAddress>
#include <QIntValidator>
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
  fix_table_->setStyleSheet(kTableStyleSheet);



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
  baseline_->setStyleSheet(kTableStyleSheet);

  auto *top = new QGridLayout;
  top->setContentsMargins(0, 0, 0, 0);
  top->setSpacing(0);

  top->addWidget(fix_table_, 0, 0);
  top->addWidget(baseline_, 0, 1);

  auto *v = new QVBoxLayout(this);
  v->setContentsMargins(4, 2, 4, 2);
  v->setSpacing(0);
  v->addLayout(top, 2);

  auto *connection_layout = new QHBoxLayout;
  connection_layout->setContentsMargins(0, 4, 0, 0);
  connection_layout->setSpacing(6);

  auto *ip_address_label = new QLabel(tr("IP address:"), this);
  ip_address_input_ = new QLineEdit(this);
  ip_address_input_->setPlaceholderText(tr("127.0.0.1"));
  ip_address_label->setBuddy(ip_address_input_);

  auto *port_label = new QLabel(tr("Port:"), this);
  port_input_ = new QLineEdit(this);
  port_input_->setValidator(new QIntValidator(1, 65535, port_input_));
  port_input_->setMaxLength(5);
  port_input_->setInputMethodHints(Qt::ImhDigitsOnly);
  port_label->setBuddy(port_input_);

  connection_layout->addWidget(ip_address_label);
  connection_layout->addWidget(ip_address_input_, 1);
  connection_layout->addWidget(port_label);
  connection_layout->addWidget(port_input_);
  v->addLayout(connection_layout);

  connect(ip_address_input_, &QLineEdit::editingFinished, this,
          &GnssStatus::applyConnectionSettings);
  connect(port_input_, &QLineEdit::editingFinished, this,
          &GnssStatus::applyConnectionSettings);
}

void GnssStatus::setConnectionEndpoint(const QString &host, uint16_t port) {
  connection_host_ = host;
  connection_port_ = port;
  if (ip_address_input_)
    ip_address_input_->setText(host);
  if (port_input_)
    port_input_->setText(QString::number(port));
}

void GnssStatus::applyConnectionSettings() {
  const QString host = ip_address_input_->text().trimmed();
  QHostAddress address;
  if (!address.setAddress(host))
    return;

  bool port_is_valid = false;
  const uint port_value = port_input_->text().toUInt(&port_is_valid);
  if (!port_is_valid || port_value < 1 || port_value > 65535)
    return;

  const uint16_t port = static_cast<uint16_t>(port_value);
  if (host == connection_host_ && port == connection_port_)
    return;

  setConnectionEndpoint(host, port);
  emit connectionRequested(connection_host_, connection_port_);
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