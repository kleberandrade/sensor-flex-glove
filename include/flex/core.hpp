#ifndef FLEX_CORE_HPP
#define FLEX_CORE_HPP
#include <stddef.h>
#include <stdint.h>

namespace flex {
constexpr uint8_t kFingers = 5;
constexpr size_t kPacketSize = 10;
constexpr uint32_t kTimeoutMs = 500;

// An eight-sample moving average, filled only with actual readings.
class Average {
 public:
  void reset() { *this = Average(); }
  uint16_t add(uint16_t value) {
    sum_ -= values_[index_];
    values_[index_] = value;
    sum_ += value;
    index_ = (index_ + 1) % 8;
    if (count_ < 8) ++count_;
    return static_cast<uint16_t>(sum_ / count_);
  }
 private:
  uint16_t values_[8] = {};
  uint32_t sum_ = 0;
  uint8_t index_ = 0;
  uint8_t count_ = 0;
};

class Calibration {
 public:
  void reset() { *this = Calibration(); }
  bool observe(int value) {
    if (value < 0 || value > 1023) return false;
    if (value < low_) low_ = value;
    if (value > high_) high_ = value;
    return true;
  }
  bool angle(int value, uint8_t& result) const {
    if (value < 0 || value > 1023 || high_ - low_ < 20) return false;
    if (value < low_) value = low_;
    if (value > high_) value = high_;
    result = static_cast<uint8_t>(10 + static_cast<int32_t>(value - low_) * 160 /
                                 (high_ - low_));
    return true;
  }
 private:
  int low_ = 1023;
  int high_ = 0;
};

struct Packet {
  uint16_t sequence;
  uint8_t mask;
  uint8_t angles[kFingers];
};

inline void encode(const Packet& packet, uint8_t* bytes) {
  bytes[0] = 0xf1;
  bytes[1] = 1;
  bytes[2] = static_cast<uint8_t>(packet.sequence);
  bytes[3] = static_cast<uint8_t>(packet.sequence >> 8);
  bytes[4] = packet.mask;
  for (uint8_t i = 0; i < kFingers; ++i) bytes[5+i] = packet.angles[i];
}

inline bool decode(const uint8_t* bytes, size_t size, Packet& packet) {
  if (size != kPacketSize) return false;
  if (bytes[0] != 0xf1 || bytes[1] != 1 || (bytes[4] & 0xe0)) return false;
  for (uint8_t i = 0; i < kFingers; ++i) {
    if (bytes[5+i] < 10 || bytes[5+i] > 170) return false;
  }
  packet.sequence = static_cast<uint16_t>(bytes[2]) |
                    (static_cast<uint16_t>(bytes[3]) << 8);
  packet.mask = bytes[4];
  for (uint8_t i = 0; i < kFingers; ++i) packet.angles[i] = bytes[5+i];
  return true;
}

inline bool expired(uint32_t now, uint32_t last) {
  return static_cast<uint32_t>(now - last) >= kTimeoutMs;
}
} // namespace flex
#endif
