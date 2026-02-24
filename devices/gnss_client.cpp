#include "gnss_client.h"

#include <cmath>
#include <iostream>
#include <array>
#include <boost/math/constants/constants.hpp>

GnssClient::GnssClient(QObject* parent)
    : QObject(parent)
{
    connect(&socket_, &QTcpSocket::readyRead,
            this, &GnssClient::onReadyRead);

    connect(&socket_,
            QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
            this,
            &GnssClient::onSocketError);
}

void GnssClient::connectTcp(const QString& host, quint16 port)
{
    last_error_.clear();

    if (socket_.state() != QAbstractSocket::UnconnectedState)
        socket_.abort();

    ublox_parser_.reset();
    state_ = GnssPvt{};

    socket_.connectToHost(host, port);
}

void GnssClient::disconnect()
{
    last_error_.clear();

    socket_.disconnectFromHost();
    if (socket_.state() != QAbstractSocket::UnconnectedState)
        socket_.abort();

    ublox_parser_.reset();
    state_ = GnssPvt{};
}

bool GnssClient::isConnected() const
{
    return socket_.state() == QAbstractSocket::ConnectedState;
}

QString GnssClient::lastErrorString() const
{
    return last_error_;
}

void GnssClient::onReadyRead()
{
    const QByteArray bytes = socket_.readAll();
    if (bytes.isEmpty())
        return;

    ublox_parser_.read_bytes(bytes); 
    updateGnssPvt();
}

void GnssClient::onSocketError(QAbstractSocket::SocketError)
{
    last_error_ = socket_.errorString();
}

void GnssClient::updateGnssPvt() {

    static constexpr float mm_to_m = 1e-3;
    static constexpr float meters_per_sec_to_miles_per_hour = 2.23694;
    static constexpr uint32_t milliseconds_in_week = 604800000U;
    state_.latitude = ublox_parser_.latitude();
    state_.longitude = ublox_parser_.longitude();
    state_.height_msl = ublox_parser_.height_msl();
    state_.velocity_n = ublox_parser_.velocity_n() * mm_to_m;
    state_.velocity_e = ublox_parser_.velocity_e() * mm_to_m;
    state_.velocity_d = ublox_parser_.velocity_d() * mm_to_m;

    state_.velocity_2d = std::sqrt(std::pow(state_.velocity_n, 2) + std::pow(state_.velocity_e, 2));
    state_.velocity_3d = std::sqrt(std::pow(state_.velocity_n, 2) + std::pow(state_.velocity_e, 2) + std::pow(state_.velocity_d, 2) );

    state_.sog_mph = state_.velocity_2d * meters_per_sec_to_miles_per_hour;
    state_.heading = ublox_parser_.heading();
    state_.gps_tow_ms = ublox_parser_.itow(); 
    // std::cout << "state gps time ms: " << state_.gps_tow_ms << "\n";
    const uint64_t time_since_epoch{2407 * milliseconds_in_week};
    std::chrono::gps_time<std::chrono::milliseconds> gps_t(std::chrono::milliseconds(state_.gps_tow_ms + time_since_epoch));
    
    state_.utc_time = std::chrono::gps_clock::to_utc(gps_t);
    state_.num_sv = ublox_parser_.numSv();
    state_.utc_datetime = ublox_parser_.utcDateTime(); 
    state_.cardinal_direction = degreesToCardinal(state_.heading);

    state_.differential_mode = ublox_parser_.differentialMode(); 
    state_.correction_age = ublox_parser_.correctionAge();

}

QString GnssClient::degreesToCardinal(const float degrees) {
    const std::array<QString, 8> directions = {
        "N", "NE", "E", "SE", "S", "SW", "W", "NW"
    };

    constexpr float sector = 360 / directions.size(); // 45 degrees

    // offset by half the sector so boundaries map correctly 
    uint8_t index = static_cast<uint8_t>((degrees + sector / 2.0) / sector) % directions.size();
    return directions[index];
}

std::array<float, 3> GnssClient::geodetic2Ned(float lat1, float lon1, float h1, float lat2, float lon2, float h2)
{
    constexpr float deg2rad =
        boost::math::constants::pi<float>() / 180.0f;

    // WGS-84
    constexpr float a = 6378137.0f;                // semi-major axis (m)
    constexpr float f = 1.0f / 298.257223563f;     // flattening.
    constexpr float e2 = f * (2.0f - f);           // eccentricity²

     lat1 = lat1 * deg2rad;
     lat2 = lat2 * deg2rad;
     lon1 = lon1 * deg2rad;
     lon2 = lon2 * deg2rad;

    const float d_lat = lat2 - lat1;
    const float d_lon = lon2 - lon1;

    const float sin_lat = std::sin(lat1);
    const float cos_lat = std::cos(lat1);

    const float w  = std::sqrt(1.0f - e2 * sin_lat * sin_lat);
    const float Rn = a / w;
    const float Rm = a * (1.0f - e2) / (w * w * w);

    return {
        d_lat * (Rm + h1),              // North
        d_lon * (Rn + h1) * cos_lat,    // East
        -(h2 - h1)                      // Down
    };
}