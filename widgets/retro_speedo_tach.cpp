#include "widgets/retro_speedo_tach.h"

#include <algorithm>
#include <array>
#include <cmath>

#include <QDate>
#include <QDateTime>
#include <QFontMetricsF>
#include <QHBoxLayout>
#include <QLCDNumber>
#include <QLabel>
#include <QPainter>
#include <QRandomGenerator>
#include <QResizeEvent>
#include <QTime>
#include <QVBoxLayout>
#include <QtMath>
namespace {
constexpr int kSegmentCount = 36;
constexpr int kSpeedMaximumMph = 120;
constexpr int kSpeedTickIntervalMph = 20;
constexpr int kTachMaximumRpm = 6500;
constexpr int kTachRedlineRpm = 5500;
constexpr int kTachTickIntervalRpm = 1000;
constexpr int kCurveLengthSamples = 128;
constexpr double kMetersPerMile = 1609.344;
constexpr double kCurveNormalSample = 0.001;
constexpr double kMinimumBandWidth = 36.0;
constexpr double kBandWidthFromGaugeWidth = 0.24;
constexpr double kBandWidthFromGaugeHeight = 0.18;
constexpr double kSegmentGapFraction = 0.22;

struct GaugeCurve {
  QPointF start;
  QPointF first_control;
  QPointF second_control;
  QPointF end;
};

const GaugeCurve kSpeedCurve{QPointF(0.22, 0.88), QPointF(0.22, 0.48),
                             QPointF(0.68, 0.22), QPointF(0.68, 0.20)};
const GaugeCurve kTachCurve{QPointF(0.17, 0.84), QPointF(0.20, 0.55),
                            QPointF(0.67, 0.17), QPointF(0.85, 0.30)};

QPointF cubicPoint(const GaugeCurve &curve, double fraction) {
  const double inverse = 1.0 - fraction;
  return curve.start * (inverse * inverse * inverse) +
         curve.first_control * (3.0 * inverse * inverse * fraction) +
         curve.second_control * (3.0 * inverse * fraction * fraction) +
         curve.end * (fraction * fraction * fraction);
}

QPointF rawGaugePoint(const QSize &size, double fraction, bool tachometer) {
  const GaugeCurve &curve = tachometer ? kTachCurve : kSpeedCurve;
  const QPointF point = cubicPoint(curve, fraction);
  return QPointF(point.x() * size.width(), point.y() * size.height());
}

double speedCurveParameter(const QSize &size, double distance_fraction) {
  if (distance_fraction <= 0.0 || distance_fraction >= 1.0)
    return distance_fraction;

  std::array<double, kCurveLengthSamples + 1> cumulative_lengths{};
  QPointF previous_point = rawGaugePoint(size, 0.0, false);
  for (int sample = 1; sample <= kCurveLengthSamples; ++sample) {
    const double parameter = static_cast<double>(sample) / kCurveLengthSamples;
    const QPointF point = rawGaugePoint(size, parameter, false);
    const QPointF delta = point - previous_point;
    cumulative_lengths[sample] =
        cumulative_lengths[sample - 1] + std::hypot(delta.x(), delta.y());
    previous_point = point;
  }

  const double target_length = distance_fraction * cumulative_lengths.back();
  const auto upper = std::lower_bound(cumulative_lengths.begin(),
                                      cumulative_lengths.end(), target_length);
  const auto upper_index =
      static_cast<int>(std::distance(cumulative_lengths.begin(), upper));
  const int lower_index = upper_index - 1;
  const double segment_length =
      cumulative_lengths[upper_index] - cumulative_lengths[lower_index];
  const double segment_fraction =
      (target_length - cumulative_lengths[lower_index]) / segment_length;
  return (lower_index + segment_fraction) / kCurveLengthSamples;
}

QPointF gaugePoint(const QSize &size, double fraction, bool tachometer) {
  const double parameter =
      tachometer ? fraction : speedCurveParameter(size, fraction);
  return rawGaugePoint(size, parameter, tachometer);
}

QPointF gaugeNormal(const QSize &size, double fraction, bool tachometer) {
  const double before_fraction = qMax(0.0, fraction - kCurveNormalSample);
  const double after_fraction = qMin(1.0, fraction + kCurveNormalSample);
  const QPointF tangent = gaugePoint(size, after_fraction, tachometer) -
                          gaugePoint(size, before_fraction, tachometer);
  const double length = std::hypot(tangent.x(), tangent.y());
  if (length == 0.0)
    return QPointF(0.0, 0.0);
  return QPointF(tangent.y() / length, -tangent.x() / length);
}

} // namespace

class RetroGauge : public QWidget {
public:
  RetroGauge(const QString &title, double maximum, double redline,
             QWidget *parent = nullptr)
      : QWidget(parent), title_(title), maximum_(maximum), redline_(redline) {
    setMinimumSize(320, 180);

    value_display_ = new QLCDNumber(this);
    value_display_->setDigitCount(redline_ > 0.0 ? 4 : 3);
    value_display_->setSegmentStyle(QLCDNumber::Flat);
    value_display_->setStyleSheet(
        "QLCDNumber { color: #D8DD43; background-color: #100D04; "
        "border: 1px solid #806019; }");
    value_display_->display(QStringLiteral("---"));
  }

  void setValue(double value) {
    value_ = std::clamp(value, 0.0, maximum_);
    available_ = true;
    value_display_->display(QString::number(qRound(value_)));
    update();
  }

  void setUnavailable() {
    available_ = false;
    value_display_->display(QStringLiteral("---"));
    update();
  }

protected:
  void resizeEvent(QResizeEvent *event) override {
    const int display_width = qRound(width() * 0.43);
    const int display_height = qRound(height() * 0.15);
    value_display_->setGeometry(width() - display_width - 14,
                                height() - display_height - 14, display_width,
                                display_height);
    QWidget::resizeEvent(event);
  }

  void paintEvent(QPaintEvent *) override {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), QColor("#100D04"));

    const QRectF panel = QRectF(rect()).adjusted(2.0, 2.0, -2.0, -2.0);
    painter.setPen(QPen(QColor("#806019"), 2.0));
    painter.setBrush(QColor("#171306"));
    painter.drawRoundedRect(panel, 12.0, 12.0);

    QFont title_font = painter.font();
    title_font.setBold(true);
    title_font.setPixelSize(qBound(20, width() / 14, 32));
    painter.setFont(title_font);
    painter.setPen(QColor("#E8B52B"));
    painter.drawText(QRectF(0.0, 12.0, width(), 30.0), Qt::AlignCenter, title_);

    const double band_width =
        qMax(kMinimumBandWidth, qMin(width() * kBandWidthFromGaugeWidth,
                                     height() * kBandWidthFromGaugeHeight));
    const double bar_thickness = qMax(6.0, height() * 0.018);
    const bool tachometer = redline_ > 0.0;
    const double filled_fraction = available_ ? value_ / maximum_ : 0.0;
    const double segment_gap = kSegmentGapFraction / kSegmentCount;

    for (int index = 0; index < kSegmentCount; ++index) {
      const double first_fraction =
          static_cast<double>(index) / kSegmentCount + segment_gap * 0.5;
      const double last_fraction =
          static_cast<double>(index + 1) / kSegmentCount - segment_gap * 0.5;
      const double middle_fraction = (first_fraction + last_fraction) * 0.5;
      const bool active = available_ && middle_fraction <= filled_fraction;
      const bool redline_segment =
          redline_ > 0.0 && middle_fraction * maximum_ > redline_;

      QColor segment_color("#34320F");
      if (redline_segment)
        segment_color = active ? QColor("#D83A1E") : QColor("#522018");
      else if (active)
        segment_color = QColor("#D8DD43");

      const QPointF point = gaugePoint(size(), middle_fraction, tachometer);
      const QRectF bar = tachometer ? QRectF(point.x() - bar_thickness * 0.5,
                                             point.y() - band_width * 0.5,
                                             bar_thickness, band_width)
                                    : QRectF(point.x() - band_width * 0.5,
                                             point.y() - bar_thickness * 0.5,
                                             band_width, bar_thickness);
      painter.setPen(Qt::NoPen);
      painter.setBrush(segment_color);
      painter.drawRect(bar);
    }

    QFont scale_font = painter.font();
    scale_font.setPixelSize(qBound(16, width() / 18, 22));
    painter.setFont(scale_font);
    painter.setPen(QColor("#E8B52B"));
    const int maximum_value = qRound(maximum_);
    const int tick_step =
        tachometer ? kTachTickIntervalRpm : kSpeedTickIntervalMph;
    const bool has_partial_endpoint = maximum_value % tick_step != 0;
    const int last_regular_tick = has_partial_endpoint
                                      ? maximum_value / tick_step * tick_step
                                      : maximum_value;
    auto drawTick = [&](int tick_value, bool draw_label) {
      const double fraction = static_cast<double>(tick_value) / maximum_value;
      const QPointF point = gaugePoint(size(), fraction, tachometer);
      const QPointF normal = gaugeNormal(size(), fraction, tachometer);
      const double tick_start = band_width * 0.5 + 4.0;
      const QPointF tick_inner = point + normal * tick_start;
      const QPointF tick_outer = point + normal * (tick_start + 8.0);
      painter.setPen(QPen(QColor("#E8B52B"), 2.0));
      painter.drawLine(tick_inner, tick_outer);

      if (!draw_label)
        return;

      const QPointF label_position = point + normal * (tick_start + 22.0);
      const QString label = QString::number(tick_value);
      const QFontMetricsF metrics(scale_font);
      const double label_width = qMax(tachometer ? 64.0 : 36.0,
                                      metrics.horizontalAdvance(label) + 8.0);
      QRectF label_rect(label_position.x() - label_width * 0.5,
                        label_position.y() - metrics.height() * 0.5,
                        label_width, metrics.height());
      label_rect.moveLeft(
          qBound(2.0, label_rect.left(), width() - label_width - 2.0));
      label_rect.moveTop(
          qBound(2.0, label_rect.top(), height() - label_rect.height() - 2.0));
      painter.setPen(QColor("#E8B52B"));
      painter.drawText(label_rect, Qt::AlignCenter, label);
    };

    for (int tick_value = 0; tick_value <= last_regular_tick;
         tick_value += tick_step) {
      const bool skip_label_for_endpoint_spacing =
          has_partial_endpoint && tick_value == last_regular_tick;
      drawTick(tick_value, !skip_label_for_endpoint_spacing);
    }
    if (has_partial_endpoint)
      drawTick(maximum_value, true);
  }

private:
  QString title_;
  double maximum_;
  double redline_;
  double value_ = 0.0;
  bool available_ = false;
  QLCDNumber *value_display_ = nullptr;
};

RetroSpeedoTach::RetroSpeedoTach(QWidget *parent) : QWidget(parent) {
  setStyleSheet("RetroSpeedoTach { background-color: #100D04; }");

  speed_gauge_ = new RetroGauge(tr("MPH"), kSpeedMaximumMph, 0.0, this);
  tach_gauge_ =
      new RetroGauge(tr("RPM"), kTachMaximumRpm, kTachRedlineRpm, this);

  auto *center = new QWidget(this);
  center->setStyleSheet("QWidget { background-color: #100D04; }");
  auto *center_layout = new QVBoxLayout(center);
  center_layout->setContentsMargins(2, 0, 2, 0);
  center_layout->setSpacing(8);

  auto *mileage_title = new QLabel(tr("MILES"), center);
  mileage_title->setAlignment(Qt::AlignCenter);
  mileage_title->setStyleSheet(
      "color: #E8B52B; font-size: 20px; font-weight: bold;");
  mileage_display_ = new QLCDNumber(center);
  mileage_display_->setDigitCount(6);
  mileage_display_->setMinimumHeight(64);
  mileage_display_->setSegmentStyle(QLCDNumber::Flat);
  mileage_display_->setStyleSheet(
      "QLCDNumber { color: #D8DD43; background-color: #100D04; }");
  mileage_display_->display(QStringLiteral("---.-"));

  auto *time_title = new QLabel(tr("TIME"), center);
  time_title->setAlignment(Qt::AlignCenter);
  time_title->setStyleSheet(
      "color: #E8B52B; font-size: 20px; font-weight: bold;");
  time_display_ = new QLCDNumber(center);
  time_display_->setDigitCount(5);
  time_display_->setMinimumHeight(64);
  time_display_->setSegmentStyle(QLCDNumber::Flat);
  time_display_->setStyleSheet(
      "QLCDNumber { color: #D8DD43; background-color: #100D04; }");
  time_display_->display(QStringLiteral("--:--"));

  center_layout->addStretch(1);
  center_layout->addWidget(mileage_title);
  center_layout->addWidget(mileage_display_, 1);
  center_layout->addSpacing(12);
  center_layout->addWidget(time_title);
  center_layout->addWidget(time_display_, 1);
  center_layout->addStretch(1);

  auto *layout = new QHBoxLayout(this);
  layout->setContentsMargins(6, 6, 6, 6);
  layout->setSpacing(2);
  layout->addWidget(speed_gauge_, 5);
  layout->addWidget(center, 2);
  layout->addWidget(tach_gauge_, 5);
}

void RetroSpeedoTach::updateDisplay(const GnssPvt &gnss_state) {
  // speed_gauge_->setValue(gnss_state.sog_mph);
  // setEngineRpm(QRandomGenerator::global()->bounded(6501));
  if (dummy_speed_ > 125) {
    dummy_speed_ = 0;
  }

  if (dummy_rpm_ > 6600) {
    dummy_rpm_ = 0;
  }
  dummy_speed_++;
  dummy_rpm_ += 81;
  speed_gauge_->setValue(dummy_speed_);
  setEngineRpm(dummy_rpm_);

  mileage_display_->display(
      QString::number(gnss_state.odometer_m / kMetersPerMile, 'f', 1));

  const auto &utc = gnss_state.utc_datetime;
  const QDate date(utc[0], utc[1], utc[2]);
  const QTime time(utc[3], utc[4], utc[5]);
  if (date.isValid() && time.isValid()) {
    const QDateTime utc_datetime(date, time, Qt::UTC);
    time_display_->display(utc_datetime.toLocalTime().toString("hh:mm"));
  } else {
    time_display_->display(QStringLiteral("--:--"));
  }
}

void RetroSpeedoTach::setEngineRpm(uint16_t rpm) { tach_gauge_->setValue(rpm); }

void RetroSpeedoTach::setDisconnected() {
  speed_gauge_->setUnavailable();
  tach_gauge_->setUnavailable();
  mileage_display_->display(QStringLiteral("---.-"));
  time_display_->display(QStringLiteral("--:--"));
}
