#ifndef FLEX_SERVO_CALIBRATION_HPP
#define FLEX_SERVO_CALIBRATION_HPP
#include "servo_output.hpp"
namespace flex {
class ServoCalibration {
 public:
  explicit ServoCalibration(const ServoSettings* defaults) {
    for(uint8_t i=0;i<kFingers;++i) defaults_[i]=defaults[i];
    restoreDefaults();
  }
  bool active() const {return active_;}
  void start() {active_=true;stop();}
  void run() {active_=false;stop();}
  void stop() {previewChannel_=kFingers;}
  const ServoSettings* settings() const {return settings_;}
  bool set(uint8_t channel,uint16_t minUs,uint16_t maxUs) {
    if(!active_ || channel>=kFingers) return false;
    ServoSettings value={settings_[channel].pin,minUs,maxUs,settings_[channel].reversed};
    if(!validServoSettings(value)) return false;
    settings_[channel]=value;stop();return true;
  }
  bool setReversed(uint8_t channel,bool reversed) {
    if(!active_ || channel>=kFingers) return false;
    settings_[channel].reversed=reversed;stop();return true;
  }
  bool preview(uint8_t channel,uint16_t pulse,uint32_t now) {
    if(!active_ || channel>=kFingers) return false;
    if(pulse<settings_[channel].minUs || pulse>settings_[channel].maxUs) return false;
    previewChannel_=channel;previewPulse_=pulse;previewStart_=now;return true;
  }
  void tick(uint32_t now) {
    if(static_cast<uint32_t>(now-previewStart_)>=1000) stop();
  }
  bool previewEnabled(uint8_t channel) const {return active_ && channel<kFingers && channel==previewChannel_;}
  uint16_t previewPulse() const {return previewPulse_;}
  void restoreDefaults() {
    for(uint8_t i=0;i<kFingers;++i) settings_[i]=defaults_[i];
    stop();
  }
  bool restore(const ServoSettings* values) {
    for(uint8_t i=0;i<kFingers;++i)
      if(!validServoSettings(values[i])) return false;
    for(uint8_t i=0;i<kFingers;++i) {
      settings_[i].minUs=values[i].minUs;settings_[i].maxUs=values[i].maxUs;
      settings_[i].reversed=values[i].reversed;
    }
    stop();return true;
  }
 private:
  ServoSettings defaults_[kFingers];
  ServoSettings settings_[kFingers];
  bool active_=false;
  uint8_t previewChannel_=kFingers;
  uint16_t previewPulse_=1500;
  uint32_t previewStart_=0;
};
}
#endif
