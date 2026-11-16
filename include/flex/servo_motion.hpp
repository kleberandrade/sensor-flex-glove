#ifndef FLEX_SERVO_MOTION_HPP
#define FLEX_SERVO_MOTION_HPP
#include "servo_output.hpp"
namespace flex {
// Limits the commanded pulse, not measured physical motor speed.
class ServoMotion {
 public:
  void pause() {running_=false;fraction_=0;}
  void observe(uint16_t pulse) {pulse_=pulse;known_=true;pause();}
  uint16_t update(uint16_t target,ServoSettings settings,uint32_t now) {
    target=clampPulse(target,settings);
    if(!known_) {pulse_=settings.minUs+(settings.maxUs-settings.minUs)/2;known_=true;}
    pulse_=clampPulse(pulse_,settings);
    if(!running_) {last_=now;fraction_=0;running_=true;}
    uint32_t elapsed=now-last_;last_=now;
    if(!settings.speedUsPerSecond) {pulse_=target;fraction_=0;return pulse_;}
    if(target==pulse_) {fraction_=0;return pulse_;}
    return advance(target,settings.speedUsPerSecond,elapsed);
  }
 private:
  uint16_t advance(uint16_t target,uint16_t speed,uint32_t elapsed) {
    // No catch-up burst after a long loop stall; unsigned subtraction handles wrap.
    if(elapsed>100) elapsed=100;
    const uint32_t budget=uint32_t(speed)*elapsed+fraction_;
    uint16_t step=static_cast<uint16_t>(budget/1000);
    fraction_=static_cast<uint16_t>(budget%1000);
    const uint16_t distance=target>pulse_?target-pulse_:pulse_-target;
    if(step>=distance) {step=distance;fraction_=0;}
    pulse_=target>pulse_?pulse_+step:pulse_-step;
    return pulse_;
  }
  uint16_t pulse_=1500,fraction_=0;
  uint32_t last_=0;
  bool known_=false,running_=false;
};
}
#endif
