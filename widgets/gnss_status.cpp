#include "widgets/gnss_status.h"

#include <QVBoxLayout>

GnssStatus::GnssStatus(QWidget* parent)
    : QWidget(parent)
{
    buildUi();
}

void GnssStatus::buildUi()
{
    auto* layout = new QVBoxLayout(this);

    label_ = new QLabel("PAGE 2");
    label_->setAlignment(Qt::AlignCenter);

    layout->addWidget(label_, 1);
}

void GnssStatus::setDisconnected()
{
    if (!label_) return;
    label_->setText("DISCONNECTED");
}

void GnssStatus::updateFromGnss(const GnssPvt& s)
{
    if (!label_) return;

    // Keep it dumb/simple for now. You can expand into real tiles later.
    label_->setText(QString("SV: %1  Mode: %2")
                        .arg(s.num_sv)
                        .arg(QString::fromStdString(s.differential_mode)));
}