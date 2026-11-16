#include "flex/core.hpp"
#include <cassert>
#include <cstdio>

int main() {
  flex::Average filter;
  assert(filter.add(1023) == 1023); // no zero-filled startup bias
  for (int i = 0; i < 100; ++i) assert(filter.add(1023) == 1023);
  for (int i = 0; i < 8; ++i) filter.add(0);
  assert(filter.add(0) == 0);
  filter.reset();
  assert(filter.add(100) == 100);
  assert(filter.add(200) == 150);

  flex::Calibration calibration;
  uint8_t angle = 99;
  assert(!calibration.angle(500, angle));
  calibration.observe(500);
  assert(!calibration.angle(500, angle));
  calibration.observe(519);
  assert(!calibration.angle(510, angle)); // minimum span = 20 counts
  calibration.observe(520);
  assert(calibration.angle(510, angle) && angle == 90);
  assert(calibration.angle(0, angle) && angle == 10);
  assert(calibration.angle(1023, angle) && angle == 170);
  assert(!calibration.observe(-1));
  assert(!calibration.observe(1024));
  calibration.reset();
  assert(!calibration.angle(500, angle));
  calibration.observe(0);
  calibration.observe(1023);
  assert(calibration.angle(1023, angle) && angle == 170);
  assert(!calibration.angle(1024, angle));
  assert(!calibration.angle(-1, angle));

  flex::Packet packet = {65535, 1, {10, 90, 170, 10, 10}};
  uint8_t bytes[flex::kPacketSize];
  flex::encode(packet, bytes);
  assert(bytes[2] == 255 && bytes[3] == 255);
  flex::Packet decoded = {};
  assert(flex::decode(bytes, sizeof(bytes), decoded));
  assert(decoded.sequence == 65535 && decoded.mask == 1);
  assert(decoded.angles[0] == 10 && decoded.angles[2] == 170);
  assert(!flex::decode(bytes, sizeof(bytes)-1, decoded));
  assert(!flex::decode(bytes, sizeof(bytes)+1, decoded));
  bytes[0] = 0;
  assert(!flex::decode(bytes, sizeof(bytes), decoded));
  flex::encode(packet, bytes);
  bytes[1] = 2;
  assert(!flex::decode(bytes, sizeof(bytes), decoded));
  flex::encode(packet, bytes);
  bytes[4] = 32;
  assert(!flex::decode(bytes, sizeof(bytes), decoded));
  flex::encode(packet, bytes);
  bytes[5] = 9;
  assert(!flex::decode(bytes, sizeof(bytes), decoded));
  bytes[5] = 171;
  assert(!flex::decode(bytes, sizeof(bytes), decoded));
  packet.mask = 0;
  flex::encode(packet, bytes);
  assert(flex::decode(bytes, sizeof(bytes), decoded));
  assert(flex::expired(500, 0));
  assert(!flex::expired(499, 0));
  assert(!flex::expired(100, 0xfffffff0U)); // millis wrap
  assert(flex::expired(484, 0xfffffff0U));
  std::puts("Core behavior checks passed");
}
