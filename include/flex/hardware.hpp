#ifndef FLEX_HARDWARE_HPP
#define FLEX_HARDWARE_HPP
#include <Arduino.h>
#include "core.hpp"
#include "servo_output.hpp"
namespace hardware {
// Enable only channels that are physically connected; do not sample floating pins.
constexpr uint8_t kActiveFingers = 1;
constexpr uint8_t kAnalogPins[flex::kFingers] = {A0, A1, A2, A3, A4};
// Channel order: index, middle, ring, little, thumb (suggested finger mapping).
// Tune each pulse range unloaded before coupling a mechanism.
constexpr flex::ServoSettings kServos[flex::kFingers] = {
    {2, 1000, 2000}, {3, 1000, 2000}, {4, 1000, 2000},
    {5, 1000, 2000}, {6, 1000, 2000}};
static_assert(flex::validServoSettings(kServos[0]) &&
              flex::validServoSettings(kServos[1]) &&
              flex::validServoSettings(kServos[2]) &&
              flex::validServoSettings(kServos[3]) &&
              flex::validServoSettings(kServos[4]), "Invalid servo pulse limits");
constexpr uint32_t kSampleMs = 20;
constexpr uint32_t kCalibrationMs = 5000;
static_assert(kActiveFingers > 0 && kActiveFingers <= flex::kFingers,
              "Connect between one and five sensors");
}
#endif
