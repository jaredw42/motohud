#pragma once

#include <QWidget>
#include <QLabel>

#include "devices/gnss_client.h"

class GnssStatus : public QWidget
{
    Q_OBJECT

public:
    explicit GnssStatus(QWidget* parent = nullptr);

    void setDisconnected();
    void updateFromGnss(const GnssPvt& s);

private:
    void buildUi();

private:
    QLabel* label_ = nullptr;
};