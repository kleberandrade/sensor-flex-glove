#include <cassert>
#include <cstdio>
#include "../firmware/receiver/main.cpp"
uint32_t fakeNow = 0;
FakeSerial Serial;

void enqueue(flex::Packet packet) {
  std::array<uint8_t, flex::kPacketSize> bytes = {};
  flex::encode(packet, bytes.data());
  radio.frames.push_back(bytes);
}
int main() {
  setup();
  for (uint8_t i = 0; i < 5; ++i) assert(!servos[i].attached());
  enqueue({1, 31, {10, 50, 90, 130, 170}});
  fakeNow = 100;
  loop();
  const int pulses[] = {1000, 1250, 1500, 1750, 2000};
  for (uint8_t i = 0; i < 5; ++i) {
    assert(servos[i].attached() && servos[i].pin == i + 2);
    assert(servos[i].pulse == pulses[i] && servos[i].pulseAtAttach == pulses[i]);
  }
  // A complete but malformed RF frame cannot move or refresh any servo.
  enqueue({2, 31, {90, 90, 90, 90, 90}});
  radio.frames.back()[9] = 171;
  fakeNow = 599;
  loop();
  for (uint8_t i = 0; i < 5; ++i)
    assert(servos[i].attached() && servos[i].pulse == pulses[i]);
  fakeNow = 600;
  loop();
  for (uint8_t i = 0; i < 5; ++i) assert(!servos[i].attached());
  enqueue({3, 16, {10, 10, 10, 10, 90}});
  fakeNow = 601;
  loop();
  for (uint8_t i = 0; i < 5; ++i) assert(servos[i].attached() == (i == 4));
  assert(servos[4].pin == 6 && servos[4].pulseAtAttach == 1500);
  enqueue({4, 0, {10, 10, 10, 10, 10}});
  loop();
  for (uint8_t i = 0; i < 5; ++i) assert(!servos[i].attached());
  // Artificially overfill the double to ensure the adapter drains at most 3.
  for (int i = 0; i < 4; ++i) enqueue({5, 1, {50, 10, 10, 10, 10}});
  loop();
  assert(radio.frames.size() == 1);
  assert(servos[0].attached() && servos[0].pulse == 1250);
  std::puts("Receiver firmware adapter checks passed (mock RF/Servo, not hardware)");
}
