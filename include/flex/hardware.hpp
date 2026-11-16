#ifndef FLEX_HARDWARE_HPP
#define FLEX_HARDWARE_HPP
#include <Arduino.h>
#include "core.hpp"
namespace hardware {
// Enable only channels that are physically connected; do not sample floating pins.
constexpr uint8_t kActiveFingers = 1;
constexpr uint8_t kAnalogPins[flex::kFingers] = {A0, A1, A2, A3, A4};
constexpr uint8_t kServoPin = 2;
constexpr uint32_t kSampleMs = 20;
constexpr uint32_t kCalibrationMs = 5000;
static_assert(kActiveFingers > 0 && kActiveFingers <= flex::kFingers,
              "Connect between one and five sensors");
}
#endif
