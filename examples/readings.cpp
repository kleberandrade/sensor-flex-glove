#include "flex/core.hpp"
#include <cstdio>
int main() {
  flex::Calibration calibration;
  calibration.observe(300);
  calibration.observe(700);
  flex::Average filter;
  const uint16_t readings[] = {300, 320, 500, 700, 700, 700, 700, 700, 700, 700};
  std::puts("raw,filtered,angle");
  for (uint16_t raw : readings) {
    uint16_t filtered = filter.add(raw);
    uint8_t angle = 0;
    if (!calibration.angle(filtered, angle)) return 1;
    std::printf("%u,%u,%u\n", raw, filtered, angle);
  }
}
