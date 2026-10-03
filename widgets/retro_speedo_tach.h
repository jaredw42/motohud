#pragma once

#include <array>
#include <stdint.h>

#include <QPointF>
#include <QSize>
#include <QString>
#include <QWidget>

#include "devices/gnss_client.h"
#include "devices/rpi_pwm_gpio.h"

class QColor;
class QFontMetricsF;
class QLabel;
class QLCDNumber;
class QPainter;
class QPaintEvent;
class QResizeEvent;

class RetroSpeedoTach : public QWidget {
  Q_OBJECT

public:
  explicit RetroSpeedoTach(RpiPwmGpio *tachometer, QWidget *parent = nullptr);

  void updateDisplay(const GnssPvt &gnss_state);
  void setDisconnected();

private:
  void buildUI();

  static constexpr uint16_t kSpeedMaximumMph = 120;
  static constexpr uint16_t kTachMaximumRpm = 6500;
  static constexpr uint16_t kTachRedlineRpm = 5500;
  static constexpr float kMetersPerMile = 1609.344f;

  struct GaugeCurve {
    QPointF start;
    QPointF first_control;
    QPointF second_control;
    QPointF end;
  };

  enum class CurveSpacing { UniformParameter, UniformDistance };
  enum class ScalePlacement { CurveNormal, BandAxis };
  enum class AuxiliaryPlacement { Bottom, TopLeft };

  struct GaugeConfig {
    QString title;
    uint16_t maximum = 1;
    uint16_t redline = 0;
    uint16_t tick_interval = 1;
    uint16_t tick_label_divisor = 1;
    int segment_count = 1;
    int lcd_digits = 3;
    QString auxiliary_title;
    int auxiliary_digits = 5;
    AuxiliaryPlacement auxiliary_placement = AuxiliaryPlacement::Bottom;

    float minimum_band_width = 36.0f;
    float band_width_fraction = 0.24f;
    float band_height_fraction = 0.20f;
    float bar_rotation_degrees = 0.0f;
    float segment_gap_fraction = 0.15f;

    // BandAxis aligns the scale with the bars; CurveNormal follows the curve.
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

  static constexpr GaugeCurve speedo_curve = {
      QPointF(0.22, 0.97), // start
      QPointF(0.22, 0.59), // first cp
      QPointF(0.60, 0.28), // second cp
      QPointF(0.84, 0.05)  // end
  };

  const GaugeConfig speed_config_{
      .title = QStringLiteral("miles per hour"),
      .maximum = kSpeedMaximumMph,
      .tick_interval = 20,
      .tick_label_divisor = 1,
      .segment_count = 60,
      .lcd_digits = 3,
      .auxiliary_title = tr("odo"),
      .auxiliary_digits = 6,
      .auxiliary_placement = AuxiliaryPlacement::Bottom,
      .tick_band_offset_fraction = 1.0f,
      .tick_label_gap = 4.0f,
      .tick_label_font_width_divisor = 20,
      .curve_spacing = CurveSpacing::UniformDistance,
      .curve = speedo_curve,
  };

  static constexpr GaugeCurve tach_curve = {
      QPointF(0.03, 0.90), // start
      QPointF(0.22, 0.65), // first cp
      QPointF(0.70, 0.25), // second cp
      QPointF(0.95, 0.35)  // end
  };

  const GaugeConfig tach_config_{
      .title = QStringLiteral("revolutions per minute"),
      .maximum = kTachMaximumRpm,
      .redline = kTachRedlineRpm,
      .tick_interval = 1000,
      .tick_label_divisor = 1000,
      .segment_count = 65,
      .lcd_digits = 4,
      .auxiliary_title = tr("TIME"),
      .auxiliary_digits = 5,
      .auxiliary_placement = AuxiliaryPlacement::TopLeft,
      .bar_rotation_degrees = 90.0f,
      .tick_position_offset = QPointF(0.0, -48.0),
      .tick_label_position_offset = QPointF(0.0, -6.0),
      .tick_rotation_degrees = 90.0f,
      .tick_label_gap = 4.0f,
      .curve = tach_curve,
  };

  class RetroGauge : public QWidget {
  public:
    explicit RetroGauge(const GaugeConfig &config, QWidget *parent = nullptr);
    void setValue(uint16_t value);
    void setUnavailable();
    void setAuxiliaryValue(const QString &value);

  protected:
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

  private:
    void buildUI();
    float valueFraction(uint16_t value) const;
    static QPointF cubicPoint(const GaugeCurve &curve, float fraction);
    static QPointF cubicTangent(const GaugeCurve &curve, float fraction);
    static QPointF directionAtDegrees(float degrees);

    QPointF rawGaugePoint(float parameter) const;
    void rebuildCurveLengths();
    float curveParameter(float fraction) const;
    QPointF gaugePoint(float fraction) const;
    QPointF gaugeNormal(float fraction) const;
    void drawBand(QPainter &painter, float first_fraction, float last_fraction,
                  const QPointF &band_direction, float band_width,
                  const QColor &color) const;
    void drawColoredBand(QPainter &painter, float first_fraction,
                         float last_fraction, const QPointF &band_direction,
                         float band_width, bool active) const;
    void drawTick(QPainter &painter, const QFontMetricsF &metrics,
                  uint16_t tick_value, const QPointF &band_direction,
                  float band_width, const QPointF &tick_direction) const;

    static constexpr int kCurveLengthSamples = 128;
    static constexpr int kBandEdgeSamples = 4;
    static constexpr float kMinimumCurveLength = 0.0001f;

    const GaugeConfig config_;
    uint16_t value_ = 0;
    bool available_ = false;
    QLCDNumber *value_display_ = nullptr;
    QLCDNumber *auxiliary_display_ = nullptr;
    QLabel *auxiliary_title_ = nullptr;
    QSize curve_size_;
    std::array<float, kCurveLengthSamples + 1> cumulative_lengths_{};
  };

  RpiPwmGpio *tachometer_ = nullptr;
  RetroGauge *speed_gauge_ = nullptr;
  RetroGauge *tach_gauge_ = nullptr;

  uint16_t dummy_speed_{0U};
  float dummy_odo_{0.0f};
};
