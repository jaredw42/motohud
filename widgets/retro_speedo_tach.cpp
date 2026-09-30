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
constexpr int kSpeedSegmentCount = 40;
constexpr int kTachSegmentCount = 39;
constexpr uint16_t kSpeedMaximumMph = 120;
constexpr uint16_t kSpeedTickIntervalMph = 20;
constexpr uint16_t kTachMaximumRpm = 6500;
constexpr uint16_t kTachRedlineRpm = 6000;
constexpr uint16_t kTachTickIntervalRpm = 1000;
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

enum class CurveSpacing { UniformParameter, UniformDistance };

struct GaugeConfig {
  QString title;
  uint16_t maximum;
  uint16_t redline;
  uint16_t tick_interval;
  int segment_count;
  int lcd_digits;
  int tick_label_divisor;
  double bar_thickness_fraction;
  double minimum_tick_label_width;
  double bar_rotation_degrees;
  CurveSpacing curve_spacing;
  const GaugeCurve *curve;
};

const GaugeCurve kSpeedCurve{QPointF(0.22, 0.88), QPointF(0.22, 0.48),
                             QPointF(0.84, 0.22), QPointF(0.84, 0.12)};
const GaugeCurve kTachCurve{QPointF(0.17, 0.84), QPointF(0.20, 0.55),
                            QPointF(0.67, 0.17), QPointF(0.85, 0.30)};
const GaugeConfig kSpeedGaugeConfig{
  QStringLiteral("MPH"), kSpeedMaximumMph, 0, kSpeedTickIntervalMph,
  kSpeedSegmentCount, 3, 1, 0.014, 36.0, 0.0,
  CurveSpacing::UniformDistance, &kSpeedCurve};
const GaugeConfig kTachGaugeConfig{
  QStringLiteral("RPMx100"), kTachMaximumRpm, kTachRedlineRpm,
  kTachTickIntervalRpm, kTachSegmentCount, 4, 100, 0.018, 64.0, 90.0,
  CurveSpacing::UniformParameter, &kTachCurve};

QPointF cubicPoint(const GaugeCurve &curve, double fraction) {
  const double inverse = 1.0 - fraction;
  return curve.start * (inverse * inverse * inverse) +
         curve.first_control * (3.0 * inverse * inverse * fraction) +
         curve.second_control * (3.0 * inverse * fraction * fraction) +
         curve.end * (fraction * fraction * fraction);
}

QPointF rawGaugePoint(const QSize &size, double fraction,
                      const GaugeCurve &curve) {
  const QPointF point = cubicPoint(curve, fraction);
  return QPointF(point.x() * size.width(), point.y() * size.height());
}

double curveParameterAtDistance(const QSize &size, double distance_fraction,
                                const GaugeCurve &curve) {
  if (distance_fraction <= 0.0 || distance_fraction >= 1.0)
    return distance_fraction;

  std::array<double, kCurveLengthSamples + 1> cumulative_lengths{};
  QPointF previous_point = rawGaugePoint(size, 0.0, curve);
  for (int sample = 1; sample <= kCurveLengthSamples; ++sample) {
    const double parameter = static_cast<double>(sample) / kCurveLengthSamples;
    const QPointF point = rawGaugePoint(size, parameter, curve);
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

QPointF gaugePoint(const QSize &size, double fraction,
                   const GaugeConfig &config) {
  const double parameter =
      config.curve_spacing == CurveSpacing::UniformDistance
          ? curveParameterAtDistance(size, fraction, *config.curve)
          : fraction;
  return rawGaugePoint(size, parameter, *config.curve);
}

QPointF gaugeNormal(const QSize &size, double fraction,
                    const GaugeConfig &config) {
  const double before_fraction = qMax(0.0, fraction - kCurveNormalSample);
  const double after_fraction = qMin(1.0, fraction + kCurveNormalSample);
  const QPointF tangent = gaugePoint(size, after_fraction, config) -
                          gaugePoint(size, before_fraction, config);
  const double length = std::hypot(tangent.x(), tangent.y());
  if (length == 0.0)
    return QPointF(0.0, 0.0);
  return QPointF(tangent.y() / length, -tangent.x() / length);
}

} // namespace

class RetroGauge : public QWidget {
public:
  explicit RetroGauge(const GaugeConfig &config, QWidget *parent = nullptr)
      : QWidget(parent), config_(config) {
    setMinimumSize(320, 180);

    value_display_ = new QLCDNumber(this);
    value_display_->setDigitCount(config_.lcd_digits);
    value_display_->setSegmentStyle(QLCDNumber::Flat);
    value_display_->setStyleSheet(
        "QLCDNumber { color: #D8DD43; background-color: #100D04; "
        "border: 1px solid #806019; }");
    value_display_->display(QStringLiteral("---"));
  }

  void setValue(uint16_t value) {
    value_ = std::min(value, config_.maximum);
    available_ = true;
    value_display_->display(value_);
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
    painter.drawText(QRectF(0.0, 12.0, width(), 30.0), Qt::AlignCenter,
             config_.title);

    const double band_width =
        qMax(kMinimumBandWidth, qMin(width() * kBandWidthFromGaugeWidth,
                                     height() * kBandWidthFromGaugeHeight));
    const double bar_thickness =
        qMax(5.0, height() * config_.bar_thickness_fraction);
    const double filled_fraction =
        available_ ? static_cast<double>(value_) / config_.maximum : 0.0;
    const double segment_gap =
      kSegmentGapFraction / config_.segment_count;

    for (int index = 0; index < config_.segment_count; ++index) {
      const double first_fraction =
          static_cast<double>(index) / config_.segment_count + segment_gap * 0.5;
      const double last_fraction =
          static_cast<double>(index + 1) / config_.segment_count - segment_gap * 0.5;
      const double middle_fraction = (first_fraction + last_fraction) * 0.5;
      const bool active = available_ && middle_fraction <= filled_fraction;
      const bool redline_segment =
          config_.redline > 0 &&
          middle_fraction * config_.maximum > config_.redline;

      QColor segment_color("#34320F");
      if (redline_segment)
        segment_color = active ? QColor("#D83A1E") : QColor("#522018");
      else if (active)
        segment_color = QColor("#D8DD43");

      const QPointF point = gaugePoint(size(), middle_fraction, config_);
      painter.setPen(Qt::NoPen);
      painter.setBrush(segment_color);
      painter.save();
      painter.translate(point);
      painter.rotate(config_.bar_rotation_degrees);
      painter.drawRect(QRectF(-band_width * 0.5, -bar_thickness * 0.5,
                              band_width, bar_thickness));
      painter.restore();
    }

    QFont scale_font = painter.font();
    scale_font.setPixelSize(qBound(16, width() / 18, 22));
    painter.setFont(scale_font);
    painter.setPen(QColor("#E8B52B"));
    const int maximum_value = config_.maximum;
    const int tick_step = config_.tick_interval;
    auto drawTick = [&](int tick_value) {
      const double fraction = static_cast<double>(tick_value) / maximum_value;
      const QPointF point = gaugePoint(size(), fraction, config_);
      const QPointF normal = gaugeNormal(size(), fraction, config_);
      const double tick_start = band_width * 0.5 + 4.0;
      const QPointF tick_inner = point + normal * tick_start;
      const QPointF tick_outer = point + normal * (tick_start + 8.0);
      painter.setPen(QPen(QColor("#E8B52B"), 2.0));
      painter.drawLine(tick_inner, tick_outer);

      const QPointF label_position = point + normal * (tick_start + 22.0);
        const QString label =
            QString::number(tick_value / config_.tick_label_divisor);
      const QFontMetricsF metrics(scale_font);
        const double label_width =
            qMax(config_.minimum_tick_label_width,
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

    for (int tick_value = 0; tick_value <= maximum_value;
         tick_value += tick_step)
      drawTick(tick_value);
  }

private:
  const GaugeConfig &config_;
  uint16_t value_ = 0;
  bool available_ = false;
  QLCDNumber *value_display_ = nullptr;
};

RetroSpeedoTach::RetroSpeedoTach(QWidget *parent) : QWidget(parent) {
  setStyleSheet("RetroSpeedoTach { background-color: #100D04; }");

  speed_gauge_ = new RetroGauge(kSpeedGaugeConfig, this);
  tach_gauge_ = new RetroGauge(kTachGaugeConfig, this);

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
  dummy_odo_ += 0.1 ; 
  speed_gauge_->setValue(dummy_speed_);
  setEngineRpm(dummy_rpm_);

  mileage_display_->display(
    //   QString::number(gnss_state.odometer_m / kMetersPerMile, 'f', 1));
       QString::number(dummy_odo_, 'f', 1));

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
