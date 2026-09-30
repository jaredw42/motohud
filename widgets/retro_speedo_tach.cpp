#include "widgets/retro_speedo_tach.h"

#include <algorithm>
#include <cmath>

#include <QDate>
#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QLCDNumber>
#include <QPainter>
#include <QPolygonF>
#include <QResizeEvent>
#include <QTime>
#include <QtMath>
#include <QVBoxLayout>
#include <QRandomGenerator>
namespace {
constexpr int kSegmentCount = 36;
constexpr double kMetersPerMile = 1609.344;

QPointF cubicPoint(const QPointF &start, const QPointF &control1,
							 const QPointF &control2, const QPointF &end, double fraction) {
	const double inverse = 1.0 - fraction;
	return start * (inverse * inverse * inverse) +
			 control1 * (3.0 * inverse * inverse * fraction) +
			 control2 * (3.0 * inverse * fraction * fraction) +
			 end * (fraction * fraction * fraction);
}

QPointF gaugePoint(const QSize &size, double fraction, bool tachometer) {
	QPointF point;
	if (tachometer) {
		point = cubicPoint(QPointF(0.17, 0.84), QPointF(0.27, 0.47),
																 QPointF(0.67, 0.17), QPointF(0.85, 0.30),
																 fraction);
	} else {
		point = cubicPoint(QPointF(0.22, 0.88), QPointF(0.27, 0.48),
																 QPointF(0.70, 0.24), QPointF(0.84, 0.20),
																 fraction);
	}
	return QPointF(point.x() * size.width(), point.y() * size.height());
}

QPointF gaugeNormal(const QSize &size, double fraction, bool tachometer) {
	constexpr double kStep = 0.001;
	const double before_fraction = qMax(0.0, fraction - kStep);
	const double after_fraction = qMin(1.0, fraction + kStep);
	const QPointF tangent = gaugePoint(size, after_fraction, tachometer) -
												 gaugePoint(size, before_fraction, tachometer);
	const double length = std::hypot(tangent.x(), tangent.y());
	if (length == 0.0)
		return QPointF(0.0, 0.0);
	return QPointF(tangent.y() / length, -tangent.x() / length);
}

QPolygonF makeSegment(const QSize &size, double first_fraction,
												 double last_fraction, double band_width,
												 bool tachometer) {
	QPolygonF segment;
	constexpr int kCurveSteps = 4;
	for (int step = 0; step <= kCurveSteps; ++step) {
		const double local_fraction = static_cast<double>(step) / kCurveSteps;
		const double fraction = first_fraction +
														 (last_fraction - first_fraction) * local_fraction;
		const QPointF point = gaugePoint(size, fraction, tachometer);
		const QPointF normal = gaugeNormal(size, fraction, tachometer);
		segment << point + normal * (band_width * 0.5);
	}
	for (int step = kCurveSteps; step >= 0; --step) {
		const double local_fraction = static_cast<double>(step) / kCurveSteps;
		const double fraction = first_fraction +
														 (last_fraction - first_fraction) * local_fraction;
		const QPointF point = gaugePoint(size, fraction, tachometer);
		const QPointF normal = gaugeNormal(size, fraction, tachometer);
		segment << point - normal * (band_width * 0.5);
	}
	return segment;
}
} // namespace

class RetroGauge : public QWidget {
public:
	RetroGauge(const QString &title, double maximum, double redline,
						 QWidget *parent = nullptr)
			: QWidget(parent), title_(title), maximum_(maximum), redline_(redline) {
		setMinimumSize(160, 180);

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
		title_font.setPixelSize(qBound(14, width() / 18, 24));
		painter.setFont(title_font);
		painter.setPen(QColor("#E8B52B"));
		painter.drawText(QRectF(0.0, 12.0, width(), 30.0), Qt::AlignCenter, title_);

		const double band_width = qMax(18.0, qMin(width() * 0.12, height() * 0.09));
		const bool tachometer = redline_ > 0.0;
		const double filled_fraction = available_ ? value_ / maximum_ : 0.0;
		const double segment_gap = 0.22 / kSegmentCount;

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

			painter.setPen(Qt::NoPen);
			painter.setBrush(segment_color);
			painter.drawPolygon(makeSegment(size(), first_fraction, last_fraction,
															 band_width, tachometer));
		}

		QFont scale_font = painter.font();
		scale_font.setPixelSize(qBound(11, width() / 27, 18));
		painter.setFont(scale_font);
		painter.setPen(QColor("#E8B52B"));
		const int label_count = 6;
		for (int index = 0; index <= label_count; ++index) {
			const double fraction = static_cast<double>(index) / label_count;
			const QPointF point = gaugePoint(size(), fraction, tachometer);
			const QPointF normal = gaugeNormal(size(), fraction, tachometer);
			const double tick_start = band_width * 0.5 + 4.0;
			const QPointF tick_inner = point + normal * tick_start;
			const QPointF tick_outer = point + normal * (tick_start + 8.0);
			painter.setPen(QPen(QColor("#E8B52B"), 2.0));
			painter.drawLine(tick_inner, tick_outer);

			const QPointF label_position = point + normal * (tick_start + 22.0);
			const int label_value = qRound(maximum_ * fraction);
			const QString label = QString::number(label_value);
			painter.setPen(QColor("#E8B52B"));
			painter.drawText(QRectF(label_position.x() - 24.0,
															label_position.y() - 12.0, 48.0, 24.0),
											 Qt::AlignCenter, label);
		}
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

	speed_gauge_ = new RetroGauge(tr("MPH"), 120.0, 0.0, this);
	tach_gauge_ = new RetroGauge(tr("RPM"), 6500.0, 5500.0, this);

	auto *center = new QWidget(this);
	center->setStyleSheet("QWidget { background-color: #100D04; }");
	auto *center_layout = new QVBoxLayout(center);
	center_layout->setContentsMargins(2, 0, 2, 0);
	center_layout->setSpacing(8);

	auto *mileage_title = new QLabel(tr("MILES"), center);
	mileage_title->setAlignment(Qt::AlignCenter);
	mileage_title->setStyleSheet("color: #E8B52B; font-weight: bold;");
	mileage_display_ = new QLCDNumber(center);
	mileage_display_->setDigitCount(6);
	mileage_display_->setSegmentStyle(QLCDNumber::Flat);
	mileage_display_->setStyleSheet(
			"QLCDNumber { color: #D8DD43; background-color: #100D04; }");
	mileage_display_->display(QStringLiteral("---.-"));

	auto *time_title = new QLabel(tr("TIME"), center);
	time_title->setAlignment(Qt::AlignCenter);
	time_title->setStyleSheet("color: #E8B52B; font-weight: bold;");
	time_display_ = new QLCDNumber(center);
	time_display_->setDigitCount(5);
	time_display_->setSegmentStyle(QLCDNumber::Flat);
	time_display_->setStyleSheet(
			"QLCDNumber { color: #D8DD43; background-color: #100D04; }");
	time_display_->display(QStringLiteral("--:--"));

	center_layout->addStretch(2);
	center_layout->addWidget(mileage_title);
	center_layout->addWidget(mileage_display_);
	center_layout->addSpacing(12);
	center_layout->addWidget(time_title);
	center_layout->addWidget(time_display_);
	center_layout->addStretch(2);

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
        dummy_rpm_ =0;
    }
    dummy_speed_++;
    dummy_rpm_ +=137;
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

void RetroSpeedoTach::setEngineRpm(uint16_t rpm) {
	tach_gauge_->setValue(rpm);
}

void RetroSpeedoTach::setDisconnected() {
	speed_gauge_->setUnavailable();
	tach_gauge_->setUnavailable();
	mileage_display_->display(QStringLiteral("---.-"));
	time_display_->display(QStringLiteral("--:--"));
}
