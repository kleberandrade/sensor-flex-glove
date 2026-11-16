#include "flex/servo_motion.hpp"
#include "flex/servo_calibration.hpp"
#include "flex/serial_command.hpp"
#include "flex/calibration_storage.hpp"
#include <EEPROM.h>
#include <cassert>
#include <cstdio>
#include <cstring>
const flex::ServoSettings baseProfile[5]={{2,1000,2000},{3,1000,2000},{4,1000,2000},{5,1000,2000},{6,1000,2000}};
void motion() {
  flex::ServoMotion fast,slow;
  flex::ServoSettings limits={2,1000,2000,false,200};
  assert(slow.update(2000,limits,100)==1500); // unknown initial position: commanded midpoint
  assert(slow.update(2000,limits,200)==1520);
  assert(slow.update(1000,limits,300)==1500); // reverse target without overshoot
  assert(fast.update(2000,baseProfile[0],100)==2000); // zero disables ramp
  slow.observe(1995);slow.pause();
  assert(slow.update(2000,limits,400)==1995);
  assert(slow.update(2000,limits,500)==2000);
  assert(slow.update(2000,limits,10000)==2000); // no idle-time credit
  assert(slow.update(1000,limits,10001)==2000);
  assert(slow.update(1000,limits,10100)==1980);
  slow.pause();assert(slow.update(1000,limits,90000)==1980); // detach does not accumulate credit
  assert(slow.update(1000,limits,95000)==1960); // long loop gaps capped to 100ms
  flex::ServoMotion fractional;
  limits.speedUsPerSecond=1;
  assert(fractional.update(2000,limits,0)==1500);
  for(uint32_t t=1;t<1000;++t) assert(fractional.update(2000,limits,t)==1500);
  assert(fractional.update(2000,limits,1000)==1501);
  flex::ServoMotion wrap;
  limits.speedUsPerSecond=100;
  assert(wrap.update(2000,limits,0xfffffff0U)==1500);
  assert(wrap.update(2000,limits,84)==1510);
  limits.reversed=true;
  assert(wrap.update(flex::pulseFor(170,limits),limits,184)==1500);
  limits.minUs=1600;limits.maxUs=1800;
  wrap.pause();assert(wrap.update(1800,limits,200)==1600); // changed limits clamp history
  limits.speedUsPerSecond=0;assert(wrap.update(1800,limits,201)==1800);
  assert(!flex::validServoSettings({2,1000,2000,false,10001}));
  flex::ServoMotion independent[5];
  for(uint8_t i=0;i<5;++i) {
    flex::ServoSettings axis=baseProfile[i];axis.speedUsPerSecond=100*(i+1);
    assert(independent[i].update(2000,axis,0)==1500);
    assert(independent[i].update(2000,axis,100)==1500+10*(i+1));
  }
}
flex::SerialCommand parseSpeed(const char* line) {
  flex::CommandReader reader;flex::SerialCommand result;
  while(*line){auto next=reader.push(*line++);if(next.kind!=flex::CommandKind::None)result=next;}
  return result;
}
void configuration() {
  using K=flex::CommandKind;
  auto command=parseSpeed("speed 4 500\r\n");
  assert(command.kind==K::Speed && command.channel==4 && command.first==500);
  assert(parseSpeed("speed 0 0\n").kind==K::Speed);
  assert(parseSpeed("speed 0 10000\n").kind==K::Speed);
  for(const char* bad:{"speed 0 -1\n","speed 5 10\n","speed 0 10001\n","speed 0 65536\n","speed 0 1.5\n","speed 0\n","speed 0 1 extra\n"})
    assert(parseSpeed(bad).kind==K::Invalid);
  flex::ServoCalibration c(baseProfile);
  assert(!c.setSpeed(0,500));c.start();
  assert(!c.setSpeed(5,500) && !c.setSpeed(0,10001));
  assert(c.preview(4,1500,0) && c.setSpeed(4,500) && !c.previewEnabled(4));
  assert(c.setReversed(4,true) && c.set(4,1100,1900));
  assert(c.settings()[4].speedUsPerSecond==500 && c.settings()[4].reversed);
  for(uint8_t i=0;i<4;++i) assert(c.settings()[i].speedUsPerSecond==0);
  c.restoreDefaults();assert(c.settings()[4].speedUsPerSecond==0);
}
bool same(const flex::ServoSettings* a,const flex::ServoSettings* b) {
  for(uint8_t i=0;i<5;++i) if(a[i].minUs!=b[i].minUs || a[i].maxUs!=b[i].maxUs ||
    a[i].reversed!=b[i].reversed || a[i].speedUsPerSecond!=b[i].speedUsPerSecond)return false;
  return true;
}
void legacy(EEPROMClass& memory,const flex::ServoSettings* profile,uint8_t schema,uint8_t slot,uint32_t generation) {
  using namespace flex::calibration_storage;
  uint8_t bytes[kSlotSize];encodeRecord(profile,generation,bytes);
  bytes[3]=schema;if(schema==1) bytes[4]=5;
  write16(bytes+29,crc16(bytes+1,28));
  for(uint8_t i=0;i<kLegacySlotSize;++i) memory.bytes[slot*kLegacySlotSize+i]=bytes[i];
}
void storage() {
  using namespace flex::calibration_storage;
  flex::ServoSettings desired[5],loaded[5];
  std::memcpy(desired,baseProfile,sizeof desired);std::memcpy(loaded,baseProfile,sizeof loaded);
  for(uint8_t i=0;i<5;++i){desired[i].speedUsPerSecond=1+500*i;desired[i].minUs+=i*10;desired[i].reversed=i%2;}
  EEPROMClass memory;assert(save(memory,desired) && load(memory,loaded) && same(desired,loaded));
  int writes=memory.writes;assert(save(memory,desired) && memory.writes==writes);
  desired[3].speedUsPerSecond=10000;assert(save(memory,desired) && memory.writes>writes);
  assert(load(memory,loaded) && same(desired,loaded));
  for(uint8_t schema=1;schema<=2;++schema) {
    flex::ServoSettings old[5];std::memcpy(old,desired,sizeof old);
    for(uint8_t i=0;i<5;++i){old[i].speedUsPerSecond=0;if(schema==1)old[i].reversed=false;}
    EEPROMClass source;legacy(source,old,schema,0,6);legacy(source,old,schema,1,7);
    const auto oldBytes=source.bytes;
    assert(load(source,loaded) && same(old,loaded) && source.writes==0);
    for(int budget=0;budget<kSlotSize+5;++budget) {
      EEPROMClass interrupted=source;interrupted.allowed=budget;
      try {save(interrupted,desired);}catch(const PowerLoss&){}
      assert(load(interrupted,loaded) && (same(old,loaded)||same(desired,loaded)));
      for(uint16_t i=0;i<kBaseAddress;++i) assert(interrupted.bytes[i]==oldBytes[i]);
    }
    assert(save(source,old));writes=source.writes;
    assert(save(source,old) && writes==source.writes); // migrate once even with zero rates
    legacy(source,baseProfile,schema,1,1000); // a downgrade can update only the old bank
    assert(load(source,loaded) && same(old,loaded)); // valid current bank still wins
    EEPROMClass legacyOnly;legacy(legacyOnly,old,schema,1,7);legacyOnly.capacity=62;
    assert(load(legacyOnly,loaded) && same(old,loaded));
    assert(!save(legacyOnly,desired));
  }
  EEPROMClass rateBaseline;assert(save(rateBaseline,baseProfile));
  flex::ServoSettings rateOnly[5];std::memcpy(rateOnly,baseProfile,sizeof rateOnly);
  for(uint8_t i=0;i<5;++i) rateOnly[i].speedUsPerSecond=1234+1000*i;
  for(int budget=0;budget<kSlotSize+5;++budget) {
    EEPROMClass interrupted=rateBaseline;interrupted.allowed=budget;
    try {save(interrupted,rateOnly);}catch(const PowerLoss&){}
    assert(load(interrupted,loaded) && (same(baseProfile,loaded)||same(rateOnly,loaded)));
  }
  memory.bytes[kBaseAddress+kSlotSize+29]^=1;
  assert(load(memory,loaded) && loaded[3].speedUsPerSecond==1501); // previous complete profile
  uint8_t bytes[kSlotSize];encodeRecord(desired,100,bytes);
  write16(bytes+29,10001);write16(bytes+kSlotSize-2,crc16(bytes+1,kSlotSize-3));
  EEPROMClass invalid;for(uint8_t i=0;i<kSlotSize;++i) invalid.bytes[kBaseAddress+i]=bytes[i];
  assert(!load(invalid,loaded));
}
int main(){motion();configuration();storage();std::puts("Per-servo pulse slew and EEPROM v3 migration checks passed");}
