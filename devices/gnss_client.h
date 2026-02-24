#pragma once

#include <chrono>
#include <QObject>
#include <QTcpSocket>
#include <QString>

#include "ublox_parser.h"

class GnssPvt {
    public:
    float latitude; // decimal degrees
    float longitude; // decimal degrees
    float height_ellipsoid; // meters
    float height_msl; // meters
    float velocity_n; // meters per second
    float velocity_e; // meters per second
    float velocity_d; // meters per second
    float velocity_2d; // meters per second
    float velocity_3d; // meters per second

    float sog_mph; // miles per hour
    float heading; // degrees 
    QString cardinal_direction; 

    uint32_t gps_tow_ms;
    std::chrono::utc_time<std::chrono::milliseconds> utc_time;
    std::array<uint16_t, 6> utc_datetime;

    uint8_t num_sv;
    uint8_t correction_age; 
    std::string differential_mode; 


};

class GnssClient final : public QObject
{
    Q_OBJECT
public:
    explicit GnssClient(QObject* parent = nullptr);

    void connectTcp(const QString& host, quint16 port);
    void disconnect();

    bool isConnected() const;
    QString lastErrorString() const;

    // UI polls at 5 Hz
    const GnssPvt& state() const { return state_; }

private slots:
    void onReadyRead();
    void onSocketError(QAbstractSocket::SocketError);

private:
    QTcpSocket socket_;
    QString last_error_;

    UbloxParser ublox_parser_;
    GnssPvt state_;

    void updateGnssPvt();
    QString degreesToCardinal(const float degrees);
    std::array<float, 3> geodetic2Ned(float lat1, float lon1, float h1, float lat2, float lon2, float h2);
};
