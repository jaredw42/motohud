#pragma once 


#include <QObject>

#include "ubx_types.h"

class UbloxParser {

    public:
    UbloxParser();
    void read_bytes(const QByteArray bytes); 
    void reset(); 

    float latitude();
    float longitude();
    float height_msl(); 
    float velocity_n();
    float velocity_e();
    float velocity_d();
    float heading();
    uint32_t itow(); 
    uint8_t numSv();
    std::array<uint16_t, 6> utcDateTime();
    std::string differentialMode();
    uint8_t correctionAge();

    private:
    static constexpr uint16_t kMaxPacketSize{640U};
    static constexpr float kPositionScalingFactor{1e-7};
    static constexpr float kAltitudeScalingFactor{1e-3};
    static constexpr float kHeadingScalingFactor{1e-5};

    enum class State: uint8_t {
        kUnknown,
        kIdle,
        kSyn1,
        kSyn2,
        kMsgClass,
        kMsgId,
        kPayloadLength,
        kPayload,
        kChecksumA,
        
    };

    State state_{};
    uint16_t msg_id_{};
    uint8_t checksum_a_{};
    uint8_t checksum_b_{};
    size_t payload_length_{};
    std::array<uint8_t, kMaxPacketSize> payload_{};
    uint16_t payload_received_{};

    UbxNavPvtMsg nav_pvt_data_{};

    bool processMessage(MsgClassId id, const std::array< uint8_t, kMaxPacketSize>& payload);



};