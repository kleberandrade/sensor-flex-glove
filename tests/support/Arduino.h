#ifndef FLEX_TEST_ARDUINO_H
#define FLEX_TEST_ARDUINO_H
#include <stdint.h>
constexpr uint8_t A0=14, A1=15, A2=16, A3=17, A4=18;
#define F(value) value
extern uint32_t fakeNow;
inline uint32_t millis() { return fakeNow; }
inline void delay(uint32_t) {}
struct FakeSerial {
  void begin(unsigned long) {}
  template <typename T> void print(T) {}
  template <typename T> void println(T) {}
};
extern FakeSerial Serial;
#endif
