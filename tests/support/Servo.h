#ifndef FLEX_TEST_SERVO_H
#define FLEX_TEST_SERVO_H
// Captures requested pulses and attachment order; does not model AVR timers.
struct Servo {
  bool active = false;
  int pin = -1;
  int pulse = 1500;
  int pulseAtAttach = 0;
  int concurrentAtAttach = 0;
  static int& activeCount() {static int count=0;return count;}
  bool attached() { return active; }
  void writeMicroseconds(int value) { pulse = value; }
  void attach(int value) { pin=value; pulseAtAttach=pulse; if(!active) ++activeCount();active=true;concurrentAtAttach=activeCount(); }
  void detach() {if(active) --activeCount();active=false;}
};
#endif
