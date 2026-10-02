#include "widgets/retro_speedo_tach.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iterator>

#include <QDate>
#include <QDateTime>
#include <QFontMetricsF>
#include <QHBoxLayout>
#include <QLCDNumber>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>
#include <QTime>
#include <QVBoxLayout>
#include <QtMath>

namespace {

constexpr uint16_t kSpeedMaximumMph = 120;
constexpr uint16_t kTachMaximumRpm = 6500;
constexpr uint16_t kTachRedlineRpm = 5800;
constexpr int kCurveLengthSamples = 128;
constexpr int kBandEdgeSamples = 4;
constexpr float kMinimumCurveLength = 0.0001f;

struct GaugeCurve {
  QPointF start;
  QPointF first_control;
  QPointF second_control;
  QPointF end;
};

enum class CurveSpacing { UniformParameter, UniformDistance };
enum class ScalePlacement { CurveNormal, BandAxis };

struct GaugeConfig {
  QString title;
  uint16_t maximum = 1;
  uint16_t redline = 0;
  uint16_t tick_interval = 1;
  uint16_t tick_label_divisor = 1;
  int segment_count = 1;
  int lcd_digits = 3;

  float minimum_band_width = 36.0f;
  float band_width_fraction = 0.24f;
  float band_height_fraction = 0.20f;
  float bar_rotation_degrees = 0.0f;
  float segment_gap_fraction = 0.15f;

  // Placement belongs to this instance. BandAxis keeps the labels aligned
  // with fixed horizontal/vertical bars; CurveNormal supports rotating scales.
  ScalePlacement scale_placement = ScalePlacement::BandAxis;
  float scale_side = -1.0f;
  float tick_band_offset_fraction = 0.0f;
  float tick_offset = 0.0f;
  // Widget-coordinate adjustments: negative Y moves upward.
  QPointF tick_position_offset{0.0, 0.0};
  QPointF tick_label_position_offset{0.0, 0.0};
  float tick_half_length = 8.0f;
  float tick_rotation_degrees = 0.0f;
  float tick_label_gap = 5.0f;
  float tick_label_tangent_offset = 0.0f;
  float label_normal_side = 1.0f;
  int tick_label_font_width_divisor = 18;

  CurveSpacing curve_spacing = CurveSpacing::UniformParameter;
  GaugeCurve curve;
};

// Each factory supplies an independent, owned configuration, including its
// curve.
GaugeConfig speedGaugeConfig() {
  GaugeConfig config;
  config.title = QStringLiteral("miles per hour");
  config.maximum = kSpeedMaximumMph;
  config.tick_interval = 20;
  config.tick_label_divisor = 1;
  config.segment_count = 42;
  config.lcd_digits = 3;
  config.minimum_band_width = 36.0f;
  config.band_width_fraction = 0.24f;
  config.band_height_fraction = 0.20f;
  config.bar_rotation_degrees = 0.0f;
  config.segment_gap_fraction = 0.15f;
  config.scale_placement = ScalePlacement::BandAxis;
  config.scale_side = -1.0f;
  config.tick_band_offset_fraction = 1.0f;
  config.tick_offset = 0.0f;
  config.tick_half_length = 8.0f;
  config.tick_rotation_degrees = 0.0f;
  config.tick_label_gap = 4.0f;
  config.tick_label_tangent_offset = 0.0f;
  config.label_normal_side = 1.0f;
  config.tick_label_font_width_divisor = 20;
  config.curve_spacing = CurveSpacing::UniformDistance;
  config.curve = {QPointF(0.22, 0.88), QPointF(0.22, 0.48), QPointF(0.84, 0.22),
                  QPointF(0.84, 0.12)};
  return config;
}

GaugeConfig tachGaugeConfig() {
  static constexpr float kTickPosOffsetY{-48.0f};
  static constexpr float kLabelPosOffsetY{-6.0f};
  GaugeConfig config;
  config.title = QStringLiteral("RPMx100");
  config.maximum = kTachMaximumRpm;
  config.redline = kTachRedlineRpm;
  config.tick_interval = 1000;
  config.tick_label_divisor = 100;
  config.segment_count = 39;
  config.lcd_digits = 4;
  config.minimum_band_width = 36.0f;
  config.band_width_fraction = 0.24f;
  config.band_height_fraction = 0.20f;
  config.bar_rotation_degrees = 90.0f;
  config.segment_gap_fraction = 0.15f;
  config.scale_placement = ScalePlacement::CurveNormal;
  config.scale_side = -1.0f;
  config.tick_band_offset_fraction = 0.0f;
  config.tick_offset = 0.0f;
  config.tick_position_offset = QPointF(0.0, kTickPosOffsetY);
  config.tick_label_position_offset = QPointF(0.0, kLabelPosOffsetY);
  config.tick_half_length = 8.0f;
  config.tick_rotation_degrees = 90.0f;
  config.tick_label_gap = 4.0f;
  config.tick_label_tangent_offset = 0.0f;
  config.label_normal_side = 1.0f;
  config.tick_label_font_width_divisor = 18;
  config.curve_spacing = CurveSpacing::UniformParameter;
  config.curve = {QPointF(0.17, 0.84), QPointF(0.397, 0.55),
                  QPointF(0.623, 0.17), QPointF(0.85, 0.30)};
  return config;
}

float valueFraction(uint16_t value, const GaugeConfig &config) {
  return std::clamp(static_cast<float>(value) /
                        static_cast<float>(config.maximum),
                    0.0f, 1.0f);
}

QPointF cubicPoint(const GaugeCurve &curve, float fraction) {
  const float inverse = 1.0f - fraction;
  return curve.start * (inverse * inverse * inverse) +
         curve.first_control * (3.0f * inverse * inverse * fraction) +
         curve.second_control * (3.0f * inverse * fraction * fraction) +
         curve.end * (fraction * fraction * fraction);
}

QPointF cubicTangent(const GaugeCurve &curve, float fraction) {
  const float inverse = 1.0f - fraction;
  return (curve.first_control - curve.start) * (3.0f * inverse * inverse) +
         (curve.second_control - curve.first_control) *
             (6.0f * inverse * fraction) +
         (curve.end - curve.second_control) * (3.0f * fraction * fraction);
}

QPointF directionAtDegrees(float degrees) {
  const float angle = qDegreesToRadians(degrees);
  return QPointF(std::cos(angle), std::sin(angle));
}

} // namespace

class RetroGauge : public QWidget {
public:
  explicit RetroGauge(const GaugeConfig &config, QWidget *parent = nullptr)
      : QWidget(parent), config_(config) {
    Q_ASSERT(config_.maximum > 0);
    Q_ASSERT(config_.tick_interval > 0);
    Q_ASSERT(config_.tick_label_divisor > 0);
    Q_ASSERT(config_.segment_count > 0);
    Q_ASSERT(config_.tick_label_font_width_divisor > 0);
    Q_ASSERT(config_.segment_gap_fraction >= 0.0f &&
             config_.segment_gap_fraction < 1.0f);
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
    value_ = value;
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
    rebuildCurveLengths();
    const int display_width = qRound(width() * 0.43);
    const int display_height = qRound(height() * 0.15);
    value_display_->setGeometry(width() - display_width - 14,
                                height() - display_height - 14, display_width,
                                display_height);
    QWidget::resizeEvent(event);
  }

  void paintEvent(QPaintEvent *) override {
    if (curve_size_ != size())
      rebuildCurveLengths();

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

    const float band_width =
        std::max(config_.minimum_band_width,
                 std::min(width() * config_.band_width_fraction,
                          height() * config_.band_height_fraction));
    const QPointF band_direction =
        directionAtDegrees(config_.bar_rotation_degrees);
    const float fill_fraction =
        available_ ? valueFraction(value_, config_) : 0.0f;
    const float segment_gap = config_.segment_gap_fraction /
                              static_cast<float>(config_.segment_count);

    painter.setPen(Qt::NoPen);
    for (int index = 0; index < config_.segment_count; ++index) {
      const float first_fraction =
          static_cast<float>(index) / config_.segment_count +
          segment_gap * 0.5f;
      const float last_fraction =
          static_cast<float>(index + 1) / config_.segment_count -
          segment_gap * 0.5f;
      drawColoredBand(painter, first_fraction, last_fraction, band_direction,
                      band_width, false);
      const float active_end = std::min(last_fraction, fill_fraction);
      if (active_end > first_fraction)
        drawColoredBand(painter, first_fraction, active_end, band_direction,
                        band_width, true);
    }

    QFont scale_font = painter.font();
    scale_font.setPixelSize(
        qBound(16, width() / config_.tick_label_font_width_divisor, 22));
    painter.setFont(scale_font);
    const QFontMetricsF metrics(scale_font);
    const QPointF tick_direction =
        directionAtDegrees(config_.tick_rotation_degrees);

    // Tick values do not depend on segment count. Use a wider loop counter so
    // adding the interval cannot wrap a uint16_t at the end of its range.
    for (uint32_t tick_value = 0; tick_value <= config_.maximum;
         tick_value += config_.tick_interval) {
      drawTick(painter, metrics, static_cast<uint16_t>(tick_value),
               band_direction, band_width, tick_direction);
    }
  }

private:
  QPointF rawGaugePoint(float parameter) const {
    const QPointF point = cubicPoint(config_.curve, parameter);
    return QPointF(point.x() * width(), point.y() * height());
  }

  void rebuildCurveLengths() {
    curve_size_ = size();
    cumulative_lengths_.fill(0.0f);
    QPointF previous_point = rawGaugePoint(0.0f);
    for (int sample = 1; sample <= kCurveLengthSamples; ++sample) {
      const float parameter = static_cast<float>(sample) / kCurveLengthSamples;
      const QPointF point = rawGaugePoint(parameter);
      const QPointF delta = point - previous_point;
      cumulative_lengths_[sample] = cumulative_lengths_[sample - 1] +
                                    std::hypot(static_cast<float>(delta.x()),
                                               static_cast<float>(delta.y()));
      previous_point = point;
    }
  }

  float curveParameter(float fraction) const {
    fraction = std::clamp(fraction, 0.0f, 1.0f);
    if (config_.curve_spacing == CurveSpacing::UniformParameter ||
        fraction <= 0.0f || fraction >= 1.0f ||
        cumulative_lengths_.back() <= kMinimumCurveLength)
      return fraction;

    const float target_length = fraction * cumulative_lengths_.back();
    const auto upper =
        std::lower_bound(cumulative_lengths_.begin() + 1,
                         cumulative_lengths_.end(), target_length);
    if (upper == cumulative_lengths_.end())
      return 1.0f;
    const int upper_index =
        static_cast<int>(std::distance(cumulative_lengths_.begin(), upper));
    const int lower_index = upper_index - 1;
    const float segment_length =
        cumulative_lengths_[upper_index] - cumulative_lengths_[lower_index];
    const float segment_fraction =
        segment_length > kMinimumCurveLength
            ? (target_length - cumulative_lengths_[lower_index]) /
                  segment_length
            : 0.0f;
    return (static_cast<float>(lower_index) + segment_fraction) /
           kCurveLengthSamples;
  }

  QPointF gaugePoint(float fraction) const {
    return rawGaugePoint(curveParameter(fraction));
  }

  QPointF gaugeNormal(float fraction) const {
    const QPointF derivative =
        cubicTangent(config_.curve, curveParameter(fraction));
    const QPointF tangent(derivative.x() * width(), derivative.y() * height());
    const float length = std::hypot(static_cast<float>(tangent.x()),
                                    static_cast<float>(tangent.y()));
    if (length <= kMinimumCurveLength)
      return QPointF(-config_.label_normal_side, 0.0);
    return QPointF(tangent.y() / length, -tangent.x() / length) *
           config_.label_normal_side;
  }

  void drawBand(QPainter &painter, float first_fraction, float last_fraction,
                const QPointF &band_direction, float band_width,
                const QColor &color) const {
    if (last_fraction <= first_fraction)
      return;
    const QPointF half_band = band_direction * (band_width * 0.5f);
    QPainterPath path;
    path.moveTo(gaugePoint(first_fraction) - half_band);
    for (int sample = 1; sample <= kBandEdgeSamples; ++sample) {
      const float fraction = first_fraction + (last_fraction - first_fraction) *
                                                  static_cast<float>(sample) /
                                                  kBandEdgeSamples;
      path.lineTo(gaugePoint(fraction) - half_band);
    }
    for (int sample = kBandEdgeSamples; sample >= 0; --sample) {
      const float fraction = first_fraction + (last_fraction - first_fraction) *
                                                  static_cast<float>(sample) /
                                                  kBandEdgeSamples;
      path.lineTo(gaugePoint(fraction) + half_band);
    }
    path.closeSubpath();
    painter.setBrush(color);
    painter.drawPath(path);
  }

  void drawColoredBand(QPainter &painter, float first_fraction,
                       float last_fraction, const QPointF &band_direction,
                       float band_width, bool active) const {
    const QColor normal_color(active ? "#D8DD43" : "#34320F");
    const QColor redline_color(active ? "#D83A1E" : "#522018");
    // Split at the actual redline value, including when it crosses a segment.
    const float redline_fraction =
        config_.redline > 0 ? valueFraction(config_.redline, config_) : 1.0f;
    drawBand(painter, first_fraction, std::min(last_fraction, redline_fraction),
             band_direction, band_width, normal_color);
    drawBand(painter, std::max(first_fraction, redline_fraction), last_fraction,
             band_direction, band_width, redline_color);
  }

  void drawTick(QPainter &painter, const QFontMetricsF &metrics,
                uint16_t tick_value, const QPointF &band_direction,
                float band_width, const QPointF &tick_direction) const {
    const float fraction = valueFraction(tick_value, config_);
    const QPointF point = gaugePoint(fraction);
    const QPointF scale_direction =
        config_.scale_placement == ScalePlacement::BandAxis
            ? band_direction * config_.scale_side
            : gaugeNormal(fraction);
    const qreal band_half_extent =
        std::abs(scale_direction.x() * band_direction.x() +
                 scale_direction.y() * band_direction.y()) *
        band_width * 0.5f;
    const qreal tick_distance =
        band_half_extent * config_.tick_band_offset_fraction +
        config_.tick_offset;
    const QPointF tick_center =
        point + scale_direction * tick_distance + config_.tick_position_offset;
    const QPointF tick_half_length = tick_direction * config_.tick_half_length;
    painter.setPen(QPen(QColor("#E8B52B"), 2.0));
    painter.drawLine(tick_center - tick_half_length,
                     tick_center + tick_half_length);

    const QString label =
        QString::number(tick_value / config_.tick_label_divisor);
    const qreal label_width = metrics.horizontalAdvance(label) + 4.0;
    const qreal label_height = metrics.height();
    const qreal label_half_extent =
        std::abs(scale_direction.x()) * label_width * 0.5 +
        std::abs(scale_direction.y()) * label_height * 0.5;
    const qreal tick_half_extent =
        std::abs(scale_direction.x() * tick_direction.x() +
                 scale_direction.y() * tick_direction.y()) *
        config_.tick_half_length;
    const qreal scale_outer_extent =
        std::max(band_half_extent, tick_distance + tick_half_extent);
    const QPointF tangent(-scale_direction.y(), scale_direction.x());
    const QPointF label_position =
        point +
        scale_direction *
            (scale_outer_extent + label_half_extent + config_.tick_label_gap) +
        tangent * config_.tick_label_tangent_offset +
        config_.tick_label_position_offset;
    const QRectF label_rect(label_position.x() - label_width * 0.5,
                            label_position.y() - label_height * 0.5,
                            label_width, label_height);
    painter.drawText(label_rect, Qt::AlignCenter, label);
  }

  const GaugeConfig config_;
  uint16_t value_ = 0;
  bool available_ = false;
  QLCDNumber *value_display_ = nullptr;
  QSize curve_size_;
  std::array<float, kCurveLengthSamples + 1> cumulative_lengths_{};
};

RetroSpeedoTach::RetroSpeedoTach(RpiPwmGpio *tachometer, QWidget *parent)
    : QWidget(parent), tachometer_(tachometer) {
  setStyleSheet("RetroSpeedoTach { background-color: #100D04; }");

  speed_gauge_ = new RetroGauge(speedGaugeConfig(), this);
  tach_gauge_ = new RetroGauge(tachGaugeConfig(), this);

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
  // Preserve the supplied demo values while validating the gauge layout.
  // For live RPM, use tachometer_->rpm() when tachometer_->isOpen().
  tach_gauge_->setValue(4281);
  dummy_speed_ = 81;
  speed_gauge_->setValue(dummy_speed_);
  dummy_odo_ = 420.6f;
  mileage_display_->display(QString::number(dummy_odo_, 'f', 1));

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

void RetroSpeedoTach::setDisconnected() {
  speed_gauge_->setUnavailable();
  mileage_display_->display(QStringLiteral("---.-"));
  time_display_->display(QStringLiteral("--:--"));
}
