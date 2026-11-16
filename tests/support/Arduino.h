#ifndef FLEX_TEST_ARDUINO_H
#define FLEX_TEST_ARDUINO_H
#include <stdint.h>
#include <string>
#include <deque>
constexpr uint8_t A0=14, A1=15, A2=16, A3=17, A4=18;
#define F(value) value
extern uint32_t fakeNow;
inline uint32_t millis() { return fakeNow; }
inline void delay(uint32_t) {}
struct FakeSerial {
  std::deque<char> input;
  std::string output;
  void begin(unsigned long) {}
  int available() {return static_cast<int>(input.size());}
  int read() {char c=input.front();input.pop_front();return static_cast<unsigned char>(c);}
  void feed(const std::string& text) {for(char c:text) input.push_back(c);}
  void print(const char* value) {output+=value;}
  void print(char value) {output+=value;}
  template <typename T> void print(T value) {output+=std::to_string(value);}
  template <typename T> void println(T value) {print(value);output+='\n';}
};
extern FakeSerial Serial;
#endif
