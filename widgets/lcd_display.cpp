#include "widgets/lcd_display.h"

#include <QLCDNumber>
#include <QVBoxLayout>

LcdDisplay::LcdDisplay(QWidget *parent) : QWidget(parent) {


    static constexpr auto kLCDNumberStyleSheet = "QLCDNumber { color: deepskyblue; background-color: black; }";
	gps_towsec_ = new QLCDNumber(this);
    gps_towsec_->setDigitCount(6); 
    gps_towsec_->setStyleSheet(kLCDNumberStyleSheet);

	auto *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(gps_towsec_);
}



void LcdDisplay::updateDisplay(const GnssPvt& gnss_state) {

    static constexpr uint32_t kMillisecsPerSec{1000U};
    const uint32_t towsec = gnss_state.gps_tow_ms / kMillisecsPerSec;

    gps_towsec_->display(static_cast<int>(towsec));
    
}
