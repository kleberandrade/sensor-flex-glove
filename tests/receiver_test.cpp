#include "flex/receiver.hpp"
#include "flex/servo_output.hpp"
#include <cassert>
#include <cstdio>

namespace {
bool deliver(flex::Receiver& receiver, const flex::Packet& packet, uint32_t now) {
  uint8_t bytes[flex::kPacketSize];
  flex::encode(packet, bytes);
  return receiver.receive(bytes, sizeof(bytes), now);
}

void independent_commands_and_masks() {
  flex::Receiver receiver;
  assert(receiver.enabledMask() == 0);
  for (uint8_t i = 0; i < 5; ++i) assert(!receiver.command(i).enabled);
  flex::Packet packet = {1, 31, {10, 50, 90, 130, 170}};
  assert(deliver(receiver, packet, 100));
  assert(receiver.enabledMask() == 31);
  for (uint8_t i = 0; i < 5; ++i) {
    assert(receiver.command(i).enabled);
    assert(receiver.command(i).angle == packet.angles[i]);
  }
  packet.mask = 0x15;
  packet.angles[2] = 110;
  assert(deliver(receiver, packet, 200));
  assert(receiver.enabledMask() == 0x15);
  for (uint8_t i = 0; i < 5; ++i)
    assert(receiver.command(i).enabled == ((packet.mask >> i) & 1));
  assert(receiver.command(2).angle == 110);
  packet.mask = 0;
  assert(deliver(receiver, packet, 201));
  for (uint8_t i = 0; i < 5; ++i) assert(!receiver.command(i).enabled);
  packet.mask = 2;
  packet.angles[1] = 170;
  assert(deliver(receiver, packet, 202));
  assert(receiver.command(1).enabled && receiver.command(1).angle == 170);
  assert(!receiver.command(0).enabled);
}

void malformed_packets_do_not_change_or_refresh_outputs() {
  flex::Receiver receiver;
  flex::Packet packet = {1, 31, {90, 90, 90, 90, 90}};
  assert(deliver(receiver, packet, 100));
  uint8_t bytes[flex::kPacketSize];
  flex::encode(packet, bytes);
  bytes[9] = 171; // invalid fifth channel must reject the whole frame
  assert(!receiver.receive(bytes, sizeof(bytes), 599));
  assert(!receiver.receive(bytes, sizeof(bytes)-1, 599));
  for (uint8_t i = 0; i < 5; ++i)
    assert(receiver.command(i).enabled && receiver.command(i).angle == 90);
  receiver.tick(599);
  for (uint8_t i = 0; i < 5; ++i) assert(receiver.command(i).enabled);
  receiver.tick(600);
  for (uint8_t i = 0; i < 5; ++i) assert(!receiver.command(i).enabled);
}

void timeout_wrap_and_recovery() {
  flex::Receiver receiver;
  flex::Packet packet = {65535, 31, {10, 50, 90, 130, 170}};
  assert(deliver(receiver, packet, 0xfffffff0U));
  receiver.tick(483); // 499 ms after the last accepted frame
  for (uint8_t i = 0; i < 5; ++i) assert(receiver.command(i).enabled);
  receiver.tick(484);
  assert(receiver.enabledMask() == 0);
  for (uint8_t i = 0; i < 5; ++i) assert(!receiver.command(i).enabled);
  packet.sequence = 0;
  packet.mask = 16;
  assert(deliver(receiver, packet, 500));
  assert(receiver.command(4).enabled && receiver.command(4).angle == 170);
  assert(!receiver.command(0).enabled);
  receiver.tick(999);
  assert(receiver.command(4).enabled);
  receiver.tick(1000);
  assert(!receiver.command(4).enabled);
}

struct FakeServo {
  bool active = false;
  int pulse = 1500;
  int pulseAtAttach = 0;
  int pin = -1;
  int detachCount = 0;
  bool attached() { return active; }
  void writeMicroseconds(int value) { pulse = value; }
  void attach(int value) { pin = value; pulseAtAttach = pulse; active = true; }
  void detach() { active = false; ++detachCount; }
};

void pulse_limits_and_output_lifecycle() {
  const flex::ServoSettings settings = {6, 1000, 2000};
  assert(flex::validServoSettings(settings));
  assert(!flex::validServoSettings({6, 2000, 1000}));
  assert(!flex::validServoSettings({6, 1000, 1000}));
  assert(!flex::validServoSettings({6, 543, 2000}));
  assert(!flex::validServoSettings({6, 1000, 2401}));
  assert(flex::pulseFor(10, settings) == 1000);
  assert(flex::pulseFor(90, settings) == 1500);
  assert(flex::pulseFor(170, settings) == 2000);
  assert(flex::pulseFor(0, settings) == 1000);
  assert(flex::pulseFor(255, settings) == 2000);
  assert(flex::pulseFor(90, {2, 1100, 1900}) == 1500);
  FakeServo servo;
  flex::applyServo(servo, {false, 10}, settings);
  assert(!servo.active && servo.detachCount == 0);
  flex::applyServo(servo, {true, 170}, settings);
  assert(servo.active && servo.pin == 6 && servo.pulseAtAttach == 2000);
  flex::applyServo(servo, {true, 10}, settings);
  assert(servo.pulse == 1000);
  flex::applyServo(servo, {false, 90}, settings);
  assert(!servo.active && servo.detachCount == 1);
  flex::applyServo(servo, {false, 90}, settings);
  assert(servo.detachCount == 1);
  flex::applyServo(servo, {true, 90}, settings);
  assert(servo.active && servo.pulseAtAttach == 1500);
}

void five_outputs_follow_receiver_state() {
  flex::Receiver receiver;
  FakeServo servos[5];
  const uint8_t angles[] = {10, 50, 90, 130, 170};
  const int pulses[] = {1000, 1250, 1500, 1750, 2000};
  flex::Packet packet = {1, 31, {10, 50, 90, 130, 170}};
  assert(deliver(receiver, packet, 0));
  for (uint8_t i = 0; i < 5; ++i) {
    flex::applyServo(servos[i], receiver.command(i), {uint8_t(i+2), 1000, 2000});
    assert(servos[i].active && servos[i].pin == i+2);
    assert(servos[i].pulseAtAttach == pulses[i]);
    assert(receiver.command(i).angle == angles[i]);
  }
  packet.mask = 1;
  assert(deliver(receiver, packet, 100));
  for (uint8_t i = 0; i < 5; ++i) {
    flex::applyServo(servos[i], receiver.command(i), {uint8_t(i+2), 1000, 2000});
    assert(servos[i].active == (i == 0));
  }
  receiver.tick(600);
  for (uint8_t i = 0; i < 5; ++i) {
    flex::applyServo(servos[i], receiver.command(i), {uint8_t(i+2), 1000, 2000});
    assert(!servos[i].active);
  }
}
}
int main() {
  independent_commands_and_masks();
  malformed_packets_do_not_change_or_refresh_outputs();
  timeout_wrap_and_recovery();
  pulse_limits_and_output_lifecycle();
  five_outputs_follow_receiver_state();
  std::puts("Five-servo receiver behavior checks passed (host doubles, not hardware)");
}
