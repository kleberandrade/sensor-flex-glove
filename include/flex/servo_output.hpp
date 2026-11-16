#ifndef FLEX_SERVO_OUTPUT_HPP
#define FLEX_SERVO_OUTPUT_HPP
#include "receiver.hpp"
namespace flex {
struct ServoSettings {
  uint8_t pin;
  uint16_t minUs;
  uint16_t maxUs;
  bool reversed;
  constexpr ServoSettings(uint8_t outputPin=0,uint16_t minimum=1000,
                          uint16_t maximum=2000,bool reverse=false)
      :pin(outputPin),minUs(minimum),maxUs(maximum),reversed(reverse) {}
};
constexpr bool validServoSettings(ServoSettings settings) {
  return settings.minUs >= 544 && settings.maxUs <= 2400 &&
         settings.minUs < settings.maxUs;
}
// Settings must be validated before use. Commands 10..170 span these pulse limits.
inline uint16_t pulseFor(uint8_t angle, ServoSettings settings) {
  if (angle < 10) angle = 10;
  if (angle > 170) angle = 170;
  const uint8_t offset=settings.reversed?170-angle:angle-10;
  return settings.minUs + static_cast<uint32_t>(offset) *
         (settings.maxUs - settings.minUs) / 160;
}

template <typename ServoType>
void applyPulse(ServoType& servo, bool enabled, uint16_t pulse, ServoSettings settings) {
  if (!enabled) {
    if (servo.attached()) servo.detach();
    return;
  }
  // Prime before enabling pulses, including reattachment after a timeout.
  if (pulse < settings.minUs) pulse = settings.minUs;
  if (pulse > settings.maxUs) pulse = settings.maxUs;
  servo.writeMicroseconds(pulse);
  // Default library bounds avoid its AVR int8_t custom-attach offset limits.
  // Our own per-servo pulse limits are applied explicitly above.
  if (!servo.attached()) servo.attach(settings.pin);
}
template <typename ServoType>
void applyServo(ServoType& servo, ServoCommand command, ServoSettings settings) {
  applyPulse(servo, command.enabled, pulseFor(command.angle, settings), settings);
}
}
#endif
