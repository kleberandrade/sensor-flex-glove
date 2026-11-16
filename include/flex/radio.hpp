#ifndef FLEX_RADIO_HPP
#define FLEX_RADIO_HPP
#include <RF24.h>
#include "core.hpp"
namespace hardware {
constexpr uint8_t kCePin = 7;
constexpr uint8_t kCsnPin = 8;
constexpr uint8_t kAddress[6] = "flex1";
inline bool configureRadio(RF24& radio) {
  if (!radio.begin()) return false;
  // 2476 MHz; unlike the original channel 115, within the 2.4 GHz ISM band.
  radio.setChannel(76);
  radio.setPALevel(RF24_PA_LOW);
  if (!radio.setDataRate(RF24_250KBPS)) return false;
  radio.setCRCLength(RF24_CRC_16);
  radio.setAutoAck(true);
  radio.setRetries(3, 5);
  radio.setPayloadSize(flex::kPacketSize);
  return true;
}
}
#endif
