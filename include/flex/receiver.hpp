#ifndef FLEX_RECEIVER_HPP
#define FLEX_RECEIVER_HPP
#include "core.hpp"
namespace flex {
struct ServoCommand {
  bool enabled;
  uint8_t angle;
};

class Receiver {
 public:
  bool receive(const uint8_t* bytes, size_t size, uint32_t now) {
    Packet packet = {};
    if (!decode(bytes, size, packet)) return false;
    for (uint8_t i = 0; i < kFingers; ++i) {
      commands_[i] = {bool((packet.mask >> i) & 1), packet.angles[i]};
      if (commands_[i].enabled) lastValid_[i] = now;
    }
    return true;
  }
  void tick(uint32_t now) {
    for (uint8_t i = 0; i < kFingers; ++i) {
      if (commands_[i].enabled && expired(now, lastValid_[i]))
        commands_[i].enabled = false;
    }
  }
  // index is a trusted channel index in [0, kFingers).
  const ServoCommand& command(uint8_t index) const { return commands_[index]; }
 private:
  ServoCommand commands_[kFingers] = {};
  uint32_t lastValid_[kFingers] = {};
};
}
#endif
