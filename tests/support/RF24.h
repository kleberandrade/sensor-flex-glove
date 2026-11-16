#ifndef FLEX_TEST_RF24_H
#define FLEX_TEST_RF24_H
#include <array>
#include <deque>
#include <cstring>
#include <cassert>
#include <stdint.h>
constexpr uint8_t RF24_PA_LOW=1, RF24_250KBPS=2, RF24_CRC_16=2;
struct RF24 {
  std::deque<std::array<uint8_t,10>> frames;
  RF24(uint8_t, uint8_t) {}
  bool begin() { return true; }
  void setChannel(uint8_t) {}
  void setPALevel(uint8_t) {}
  bool setDataRate(uint8_t) { return true; }
  void setCRCLength(uint8_t) {}
  void setAutoAck(bool) {}
  void setRetries(uint8_t,uint8_t) {}
  void setPayloadSize(uint8_t) {}
  void openReadingPipe(uint8_t,const uint8_t*) {}
  void startListening() {}
  bool available() { return !frames.empty(); }
  void read(void* data, uint8_t size) {
    assert(!frames.empty() && size==frames.front().size());
    std::memcpy(data,frames.front().data(),size);
    frames.pop_front();
  }
};
#endif
