#pragma once

#include <array>
#include <cstdint>

#include <boost/endian/buffers.hpp>

static constexpr uint8_t kSynByte1{0xB5U};
static constexpr uint8_t kSynByte2{0x62U};
static constexpr uint8_t kIdleByte{0xFFU};

using le_uint32_t = boost::endian::little_uint32_buf_t;
using le_int32_t = boost::endian::little_int32_buf_t;
using le_uint16_t = boost::endian::little_uint16_buf_t;
using le_int16_t = boost::endian::little_int16_buf_t;
using le_int8_t = boost::endian::little_int8_buf_t;

enum class MsgClassId : uint16_t {
  kUbxNavPvt = 0x0107U,
  kUbxNavSat = 0x0135U,
  kUbxNavRelPosNed = 0x013C,

};

struct UbxNavPvtMsg {
  // bitfield flags
  union Flags {
    // struct intentionally un-named
    struct {
      uint8_t gnss_fix_ok : 1;
      uint8_t diff_soln : 1;
      uint8_t reserved : 3;
      uint8_t head_veh_valid : 1;
      uint8_t carr_soln : 2;
    };
    uint8_t word;
  };
  union Flags2 {
    struct {
      uint8_t reserved : 5;
      uint8_t confirmed_avai : 1;
      uint8_t confirmed_date : 1;
      uint8_t confirmed_time : 1;
    };
    uint8_t word;
  };

  union Flags3 {
    struct {
      uint16_t invalid_llh : 1;
      uint16_t last_correction_age : 4;
      uint16_t reserved : 11;
    };
    uint16_t word;
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
  Flags3 flags3;
  std::array<uint8_t, 4> reserved;
  le_int32_t heading_vehicle;
  le_int16_t magnetic_declination;
  le_uint16_t magnetic_declination_acc;
};

struct UbxNavRelPosNed {
  uint8_t version;
  uint8_t reserved1;
  uint16_t ref_station_id;
  le_uint32_t itow;     // ms
  le_int32_t rel_pos_n; // cm
  le_int32_t rel_pos_e; // cm
  le_int32_t rel_pos_d; // cm
  // high-precision components of relative postion vector, scaled by 1e-2
  // full component is rel_pos_n + ( rel_pos_hp_n * 1e-2)
  le_int8_t rel_pos_hp_n; // mm
  le_int8_t rel_pos_hp_e; // mm
  le_int8_t rel_pos_hp_d; // mm

  uint8_t reserved2;

  // accuracy components scaled by 1e-2
  le_uint32_t acc_n; // mm
  le_uint32_t acc_e; // mm
  le_uint32_t acc_d; // mm

  union Flags {
    // struct intentionally un-named
    struct {
      uint32_t gnss_fix_ok : 1;
      uint32_t diff_soln : 1;
      uint32_t ref_pos_valid : 2;
      uint32_t carr_soln : 1;
      uint32_t is_moving : 1;
      uint32_t ref_pos_miss : 1;
      uint32_t ref_obs_missing : 1;
    };
    le_uint32_t word;
  };

  Flags flags;
};

struct UbxNavCov {};

struct UbxNavSat {
  le_uint32_t itow;
  uint8_t version;
  uint8_t num_svs;
  uint16_t reserved1;
};

struct UbxNavSatSv {
  uint8_t gnss_id;
  uint8_t sv_id;
  uint8_t cno;
  int8_t elev;
  le_int16_t azim;
  le_int16_t pr_res;

  union Flags {
    struct {
      uint32_t quality_ind : 3;
      uint32_t sv_used : 1;
      uint32_t health : 2;
      uint32_t diff_corr : 1;
      uint32_t smoothed : 1;
      uint32_t orbit_source : 3;
      uint32_t eph_avail : 1;
      uint32_t alm_avail : 1;
      uint32_t ano_avail : 1;
      uint32_t aop_avail : 1;
      uint32_t reserved : 1;
      uint32_t sbas_corr_used : 1;
      uint32_t rtcm_corr_used : 1;
      uint32_t slas_corr_used : 1;
      uint32_t spartn_corr_used : 1;
      uint32_t pr_corr_used : 1;
      uint32_t cr_corr_used : 1;
      uint32_t do_corr_used : 1;
      uint32_t clas_corr_used : 1;
      uint32_t reserved2 : 8;
    };
    le_uint32_t word;
  };

  Flags flags;
};

static_assert(sizeof(UbxNavSat) == 8);
static_assert(sizeof(UbxNavSatSv) == 12);
