#ifndef FLEX_TEST_SERVO_H
#define FLEX_TEST_SERVO_H
// Captures requested pulses and attachment order; does not model AVR timers.
struct Servo {
  bool active = false;
  int pin = -1;
  int pulse = 1500;
  int pulseAtAttach = 0;
  bool attached() { return active; }
  void writeMicroseconds(int value) { pulse = value; }
  void attach(int value) { pin=value; pulseAtAttach=pulse; active=true; }
  void detach() { active=false; }
};
#endif
