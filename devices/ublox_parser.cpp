#include "ublox_parser.h"

#include <ios>
#include <iostream>



UbloxParser::UbloxParser() {}

void UbloxParser::read_bytes(const QByteArray bytes) {

  for (const uint8_t b : bytes) {
    switch (state_) {
    case State::kUnknown:
    // intentional fall-through
    case State::kIdle: {

      if (b == kIdleByte) {
        state_ = State::kIdle;
      }
      if (b == kSynByte1) {
        state_ = State::kSyn1;
      }
    } break;

    case State::kSyn1: {
      state_ = b == kSynByte2 ? State::kSyn2 : State::kUnknown;
    } break;

    case State::kSyn2: {
      msg_id_ = static_cast<uint16_t>(b) << 8;
      state_ = State::kMsgClass;
      checksum_a_ = b;
      checksum_b_ = b;
    } break;

    case State::kMsgClass: {
      msg_id_ |= static_cast<uint16_t>(b);
      state_ = State::kMsgId;
      checksum_a_ += b;
      checksum_b_ += checksum_a_;

    } break;

    case State::kMsgId: {
      payload_length_ = b;
      state_ = State::kPayloadLength;
      checksum_a_ += b;
      checksum_b_ += checksum_a_;
    } break;

    case State::kPayloadLength: {
      payload_length_ |= static_cast<size_t>(b) << 8;

      if (payload_length_ > payload_.size()) {
        std::cout << "ubx packet exceeds max size, id:" <<msg_id_ << " len: " << payload_length_
                  << " size: " << payload_.size() << "\n";
        state_ = State::kUnknown;
      } else {
        payload_received_ = 0U;
        state_ = State::kPayload;
      }
      checksum_a_ += b;
        checksum_b_ += checksum_a_;
    } break;

    case State::kPayload: {
      if (payload_received_ < payload_length_) {
        payload_[payload_received_++] = b;
        checksum_a_ += b;
        checksum_b_ += checksum_a_;
      } else if (checksum_a_ == b) {
        state_ = State::kChecksumA;
      } else {
        std::cout << "failed to decode msg.\n";
        state_ = State::kUnknown;
      }
    } break;

    case State::kChecksumA:
      if (checksum_b_ == b) {
        const bool success = processMessage(static_cast<MsgClassId>(msg_id_), payload_);
        // payload_.clear(); 
        if (!success) {
          state_ = State::kUnknown;
          std::cout << "ubx packet failed parsing.\n";
        } else {

          state_ = State::kIdle;
        }
      }

    default:
      state_ = State::kUnknown;
    }
  }

}

bool UbloxParser::processMessage(
    MsgClassId id, const std::array< uint8_t, kMaxPacketSize>& payload) {

  bool status = true;
  switch (id) {
  case MsgClassId::kUbxNavPvt: {

    
    if (payload_length_ == sizeof(UbxNavPvtMsg)) {
        nav_pvt_data_ = *reinterpret_cast<const UbxNavPvtMsg*>(payload.data());
        status = true;
    }
  }break;

  }

  return status;
}

float UbloxParser::latitude() {
    return nav_pvt_data_.lat.value() * kPositionScalingFactor;
}

float UbloxParser::longitude() {
    return nav_pvt_data_.lon.value() * kPositionScalingFactor;
}

float UbloxParser::height_msl() {
    return nav_pvt_data_.height_msl.value() * kPositionScalingFactor;
}

float UbloxParser::velocity_n() {
    return nav_pvt_data_.velocity_n.value();
}

float UbloxParser::velocity_e() {
    return nav_pvt_data_.velocity_e.value();
}

float UbloxParser::velocity_d() {
    return nav_pvt_data_.velocity_d.value();
}

float UbloxParser::heading() {
    return nav_pvt_data_.heading_motion.value() * kHeadingScalingFactor;
}

uint32_t UbloxParser::itow() {
    return nav_pvt_data_.itow.value();
}

uint8_t UbloxParser::numSv() {
    return nav_pvt_data_.num_sv;
}

std::array<uint16_t, 6> UbloxParser::utcDateTime() {

    
    return std::array<uint16_t,6>{nav_pvt_data_.year.value(), nav_pvt_data_.month, nav_pvt_data_.day, nav_pvt_data_.hour, nav_pvt_data_.min, nav_pvt_data_.sec};
}

std::string UbloxParser::differentialMode(){

    const std::array<std::string, 4> modes = {"SPS", "DGNSS", "FLOAT", "INTEGER"};

    if (nav_pvt_data_.flags.diff_soln !=1) {
        return modes[0];
    }
    std::string mode;
    const uint16_t carrier_soln = static_cast<uint16_t>(nav_pvt_data_.flags.carr_soln);
    switch(carrier_soln) {
        case 0:
        mode = modes[1];
        break;
        case 1:
        mode = modes[2];
        break;
        case 2:
        mode = modes[3];
        break;
        default:
        mode = modes[1];
        break;

    }

    return mode; 
}

uint8_t UbloxParser::correctionAge() {

    uint8_t age;

    std::cout << "flags3.word = 0x"
          << std::hex << nav_pvt_data_.flags3.word
          << std::dec << "\n";
    switch(nav_pvt_data_.flags3.last_correction_age){
        case 0:
        // flag 0 means "not available", so return max value
        age = 255U;
        break;

        case 1:
        age = 0U;
        break;

        case 2:
        age = 1U;
        break;

        case 3:
        age = 2U;
        break;

        case 4: 
        age = 5U;
        break;

        case 5: 
        age = 10U;
        break;

        case 6: 
        age = 15U;
        break; 

        case 7:
        age = 20U;
        break;

        case 8: 
        age = 30U;
        break;

        case 9:
        age = 45U;
        break; 

        case 10:
        age = 60U;
        break;

        case 11:
        age = 90U;
        break;

        case 12: //intentional fall-through
        default: 
        age = 120U;
        break;

    }
    return age; 
}

void UbloxParser::reset() {}
