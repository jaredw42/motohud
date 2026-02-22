#pragma once

#include <boost/endian/buffers.hpp> 

static constexpr uint8_t kSynByte1{0xB5U};
static constexpr uint8_t kSynByte2{0x62U};
static constexpr uint8_t kIdleByte{0xFFU};

using le_uint32_t = boost::endian::little_uint32_buf_t;
using le_int32_t = boost::endian::little_int32_buf_t;
using le_uint16_t = boost::endian::little_uint16_buf_t;
using le_int16_t = boost::endian::little_int16_buf_t;

enum class MsgClassId : uint16_t {
   kUbxNavPvt = 0x0107U,

};

struct UbxNavPvtMsg {
    // bitfield flags
    union Flags {
        // struct intentionally un-named 
        struct {
            uint8_t gnss_fix_ok: 1; 
            uint8_t diff_soln: 1;
            uint8_t reserved: 3;
            uint8_t head_veh_valid: 1;
            uint8_t carr_soln: 2;
        };
        uint8_t word;
    };
    union Flags2 {
        struct {
            uint8_t reserved: 5;
            uint8_t confirmed_avai: 1;
            uint8_t confirmed_date: 1;
            uint8_t confirmed_time: 1; 
        };
        uint8_t word;
    };

    le_uint32_t itow;
    le_uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint8_t valid; 
    le_uint32_t time_accuracy;
    le_int32_t nano;
    uint8_t fix_type;
    Flags flags;
    Flags2 flags2;
    uint8_t num_sv;
    le_int32_t lon;
    le_int32_t lat;
    le_int32_t height;
    le_int32_t height_msl;
    le_uint32_t horizontal_acc;
    le_uint32_t vertical_acc;
    le_int32_t velocity_n;
    le_int32_t velocity_e;
    le_int32_t velocity_d;
    le_int32_t ground_speed; 
    le_int32_t heading_motion; 
    le_uint32_t speed_acc;
    le_uint32_t heading_acc;
    le_uint16_t position_dop;
    std::array<uint8_t, 6> reserved;
    le_int32_t heading_vehicle;
    le_int16_t magnetic_declination;
    le_uint16_t magnetic_declination_acc; 

    
};
