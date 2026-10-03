#include "widgets/retro_speedo_tach.h"

#include <algorithm>
#include <cmath>
#include <iterator>

#include <QDate>
#include <QDateTime>
#include <QFontDatabase>
#include <QFontMetricsF>
#include <QHBoxLayout>
#include <QLCDNumber>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>
#include <QTime>
#include <QtMath>

RetroSpeedoTach::RetroSpeedoTach(RpiPwmGpio *tachometer, QWidget *parent)
    : QWidget(parent), tachometer_(tachometer) {
  buildUI();
}

void RetroSpeedoTach::buildUI() {
  setStyleSheet("RetroSpeedoTach { background-color: #100D04; }");

  const auto font_id = QFontDatabase::addApplicationFont(
      ":/widgets/assets/Orbitron-VariableFont_wght.ttf");

  const auto font_family =
      QFontDatabase::applicationFontFamilies(font_id).first();

  QFont font(font_family);
  font.setPixelSize(24);

  setFont(font);

  speed_gauge_ = new RetroGauge(speed_config_, this);
  tach_gauge_ = new RetroGauge(tach_config_, this);
  speed_gauge_->setAuxiliaryValue(QStringLiteral("---.-"));
  tach_gauge_->setAuxiliaryValue(QStringLiteral("--:--"));

  auto *layout = new QHBoxLayout(this);
  layout->setContentsMargins(6, 6, 6, 6);
  layout->setSpacing(2);
  layout->addWidget(speed_gauge_, 1);
  layout->addWidget(tach_gauge_, 1);
}

void RetroSpeedoTach::updateDisplay(const GnssPvt &gnss_state) {
  // For live RPM, use tachometer_->rpm() when tachometer_->isOpen().
  tach_gauge_->setValue(tachometer_->rpm());
  dummy_speed_ = tachometer_->rpm() / 54.1667;
  speed_gauge_->setValue(dummy_speed_);

  // tach_gauge_->setValue(tachometer_->rpm());
  // tach_gauge_->setUnavailable();
  //   ++dummy_speed_;
  dummy_odo_ = 420.6;

  speed_gauge_->setAuxiliaryValue(
      //   QString::number(gnss_state.odometer_m / kMetersPerMile, 'f', 1));
      QString::number(dummy_odo_, 'f', 1));

  const auto &utc = gnss_state.utc_datetime;
  const QDate date(utc[0], utc[1], utc[2]);
  const QTime time(utc[3], utc[4], utc[5]);
  if (date.isValid() && time.isValid()) {
    const QDateTime utc_datetime(date, time, Qt::UTC);
    tach_gauge_->setAuxiliaryValue(
        utc_datetime.toLocalTime().toString("hh:mm"));
  } else {
    tach_gauge_->setAuxiliaryValue(QStringLiteral("--:--"));
  }
}

void RetroSpeedoTach::setDisconnected() {
  speed_gauge_->setUnavailable();
  speed_gauge_->setAuxiliaryValue(QStringLiteral("---.-"));
  tach_gauge_->setAuxiliaryValue(QStringLiteral("--:--"));
}

RetroSpeedoTach::RetroGauge::RetroGauge(const GaugeConfig &config,
                                        QWidget *parent)
    : QWidget(parent), config_(config) {
  Q_ASSERT(config_.maximum > 0);
  Q_ASSERT(config_.tick_interval > 0);
  Q_ASSERT(config_.tick_label_divisor > 0);
  Q_ASSERT(config_.segment_count > 0);
  Q_ASSERT(config_.tick_label_font_width_divisor > 0);
  Q_ASSERT(config_.segment_gap_fraction >= 0.0f &&
           config_.segment_gap_fraction < 1.0f);
  buildUI();
}

void RetroSpeedoTach::RetroGauge::buildUI() {
  setMinimumSize(320, 180);

  value_display_ = new QLCDNumber(this);
  value_display_->setDigitCount(config_.lcd_digits);
  value_display_->setSegmentStyle(QLCDNumber::Flat);
  value_display_->setStyleSheet(
      "QLCDNumber { color: #D8DD43; background-color: #100D04; "
      "border: 1px solid #806019; }");
  value_display_->display(QStringLiteral("---"));

  auxiliary_title_ = new QLabel(config_.auxiliary_title, this);
  auxiliary_title_->setStyleSheet("color: #E8B52B; background: transparent;");
  auxiliary_display_ = new QLCDNumber(this);
  auxiliary_display_->setDigitCount(config_.auxiliary_digits);
  auxiliary_display_->setSegmentStyle(QLCDNumber::Flat);
  auxiliary_display_->setStyleSheet(value_display_->styleSheet());
}

void RetroSpeedoTach::RetroGauge::setValue(uint16_t value) {
  value_ = value;
  available_ = true;
  value_display_->display(value_);
  update();
}

void RetroSpeedoTach::RetroGauge::setUnavailable() {
  available_ = false;
  value_display_->display(QStringLiteral("---"));
  update();
}

void RetroSpeedoTach::RetroGauge::setAuxiliaryValue(const QString &value) {
  auxiliary_display_->display(value);
}

void RetroSpeedoTach::RetroGauge::resizeEvent(QResizeEvent *event) {
  rebuildCurveLengths();
  const int display_width = qRound(width() * 0.43);
  const int display_height = qRound(height() * 0.15);
  const int auxiliary_height = qMax(24, qRound(height() * 0.10));
  int value_bottom = height() - 14;
  QFont auxiliary_font = font();
  auxiliary_font.setBold(true);
  auxiliary_font.setPixelSize(qBound(12, width() / 25, 16));
  auxiliary_title_->setFont(auxiliary_font);

  if (config_.auxiliary_placement == AuxiliaryPlacement::Bottom) {
    const int auxiliary_y = height() - auxiliary_height - 14;
    const int display_x = width() - display_width - 14;
    const int title_width =
        qCeil(QFontMetricsF(auxiliary_font)
                  .horizontalAdvance(config_.auxiliary_title));
    auxiliary_title_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    auxiliary_title_->setGeometry(display_x - title_width - 8, auxiliary_y,
                                  title_width, auxiliary_height);
    auxiliary_display_->setGeometry(display_x, auxiliary_y, display_width,
                                    auxiliary_height);
    value_bottom = auxiliary_y - 10;
  } else {
    const int clock_width = qRound(width() * 0.34);
    const int title_height = qCeil(QFontMetricsF(auxiliary_font).height()) + 4;
    // Keep the clock above the scale when the gauge is especially short.
    const bool compact_clock = height() < 240;
    auxiliary_title_->setVisible(!compact_clock);
    auxiliary_title_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    auxiliary_title_->setGeometry(14, 14, clock_width, title_height);
    const int clock_y = compact_clock ? 10 : 14 + title_height + 4;
    auxiliary_display_->setGeometry(14, clock_y, clock_width, auxiliary_height);
  }
  value_display_->setGeometry(width() - display_width - 14,
                              value_bottom - display_height, display_width,
                              display_height);
  QWidget::resizeEvent(event);
}

void RetroSpeedoTach::RetroGauge::paintEvent(QPaintEvent *) {
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
  QFont font = QFontDatabase::font("Orbitron", "Bold", 12);
  // title_font.setBold(true);
  // title_font.setPixelSize(qBound(20, width() / 14, 32));
  painter.setFont(font);
  painter.setPen(QColor("#E8B52B"));
  painter.drawText(
      QRectF(14.0, value_display_->y() - 38.0, width() - 28.0, 30.0),
      Qt::AlignRight | Qt::AlignVCenter, config_.title);

  const float band_width =
      std::max(config_.minimum_band_width,
               std::min(width() * config_.band_width_fraction,
                        height() * config_.band_height_fraction));
  const QPointF band_direction =
      directionAtDegrees(config_.bar_rotation_degrees);
  const float fill_fraction = available_ ? valueFraction(value_) : 0.0f;
  const float segment_gap =
      config_.segment_gap_fraction / static_cast<float>(config_.segment_count);

  painter.setPen(Qt::NoPen);
  for (int index = 0; index < config_.segment_count; ++index) {
    const float first_fraction =
        static_cast<float>(index) / config_.segment_count + segment_gap * 0.5f;
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

QPointF RetroSpeedoTach::RetroGauge::rawGaugePoint(float parameter) const {
  const QPointF point = cubicPoint(config_.curve, parameter);
  return QPointF(point.x() * width(), point.y() * height());
}

void RetroSpeedoTach::RetroGauge::rebuildCurveLengths() {
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

float RetroSpeedoTach::RetroGauge::curveParameter(float fraction) const {
  fraction = std::clamp(fraction, 0.0f, 1.0f);
  if (config_.curve_spacing == CurveSpacing::UniformParameter ||
      fraction <= 0.0f || fraction >= 1.0f ||
      cumulative_lengths_.back() <= kMinimumCurveLength)
    return fraction;

  const float target_length = fraction * cumulative_lengths_.back();
  const auto upper = std::lower_bound(cumulative_lengths_.begin() + 1,
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
          ? (target_length - cumulative_lengths_[lower_index]) / segment_length
          : 0.0f;
  return (static_cast<float>(lower_index) + segment_fraction) /
         kCurveLengthSamples;
}

QPointF RetroSpeedoTach::RetroGauge::gaugePoint(float fraction) const {
  return rawGaugePoint(curveParameter(fraction));
}

QPointF RetroSpeedoTach::RetroGauge::gaugeNormal(float fraction) const {
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

void RetroSpeedoTach::RetroGauge::drawBand(QPainter &painter,
                                           float first_fraction,
                                           float last_fraction,
                                           const QPointF &band_direction,
                                           float band_width,
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

void RetroSpeedoTach::RetroGauge::drawColoredBand(
    QPainter &painter, float first_fraction, float last_fraction,
    const QPointF &band_direction, float band_width, bool active) const {
  const QColor normal_color(active ? "#D8DD43" : "#34320F");
  const QColor redline_color(active ? "#D83A1E" : "#522018");
  // Split at the actual redline value, including when it crosses a segment.
  const float redline_fraction =
      config_.redline > 0 ? valueFraction(config_.redline) : 1.0f;
  drawBand(painter, first_fraction, std::min(last_fraction, redline_fraction),
           band_direction, band_width, normal_color);
  drawBand(painter, std::max(first_fraction, redline_fraction), last_fraction,
           band_direction, band_width, redline_color);
}

void RetroSpeedoTach::RetroGauge::drawTick(
    QPainter &painter, const QFontMetricsF &metrics, uint16_t tick_value,
    const QPointF &band_direction, float band_width,
    const QPointF &tick_direction) const {
  const float fraction = valueFraction(tick_value);
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
  const QPointF tangent(-scale_direction.y(), scale_direction.x());
  // The label follows the tick, including the tick's instance offset.
  const QPointF label_position =
      tick_center +
      scale_direction *
          (tick_half_extent + label_half_extent + config_.tick_label_gap) +
      tangent * config_.tick_label_tangent_offset +
      config_.tick_label_position_offset;
  const QRectF label_rect(label_position.x() - label_width * 0.5,
                          label_position.y() - label_height * 0.5, label_width,
                          label_height);
  painter.drawText(label_rect, Qt::AlignCenter, label);
}

float RetroSpeedoTach::RetroGauge::valueFraction(uint16_t value) const {
  return std::clamp(static_cast<float>(value) /
                        static_cast<float>(config_.maximum),
                    0.0f, 1.0f);
}

QPointF RetroSpeedoTach::RetroGauge::cubicPoint(const GaugeCurve &curve,
                                                float fraction) {
  const float inverse = 1.0f - fraction;
  return curve.start * (inverse * inverse * inverse) +
         curve.first_control * (3.0f * inverse * inverse * fraction) +
         curve.second_control * (3.0f * inverse * fraction * fraction) +
         curve.end * (fraction * fraction * fraction);
}

QPointF RetroSpeedoTach::RetroGauge::cubicTangent(const GaugeCurve &curve,
                                                  float fraction) {
  const float inverse = 1.0f - fraction;
  return (curve.first_control - curve.start) * (3.0f * inverse * inverse) +
         (curve.second_control - curve.first_control) *
             (6.0f * inverse * fraction) +
         (curve.end - curve.second_control) * (3.0f * fraction * fraction);
}

QPointF RetroSpeedoTach::RetroGauge::directionAtDegrees(float degrees) {
  const float angle = qDegreesToRadians(degrees);
  return QPointF(std::cos(angle), std::sin(angle));
}
