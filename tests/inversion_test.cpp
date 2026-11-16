#include "flex/servo_calibration.hpp"
#include "flex/calibration_storage.hpp"
#include "flex/serial_command.hpp"
#include <EEPROM.h>
#include <cassert>
#include <cstring>
#include <cstdio>
const flex::ServoSettings normal[5]={{2,1000,2000},{3,1000,2000},{4,1000,2000},{5,1000,2000},{6,1000,2000}};
flex::SerialCommand parseInversion(const char* text) {
  flex::CommandReader reader;
  flex::SerialCommand result;
  while(*text) {auto next=reader.push(*text++);if(next.kind!=flex::CommandKind::None) result=next;}
  return result;
}
void mapping_and_session() {
  flex::ServoSettings inverted={2,1100,1901,true};
  flex::ServoSettings straight=inverted;straight.reversed=false;
  assert(flex::pulseFor(10,inverted)==1901);
  assert(flex::pulseFor(170,inverted)==1100);
  assert(flex::pulseFor(90,inverted)==flex::pulseFor(90,straight));
  for(unsigned int value=0;value<256;++value) {
    const uint8_t clamped=value<10?10:(value>170?170:value);
    assert(flex::pulseFor(value,inverted)==flex::pulseFor(180-clamped,straight));
  }
  flex::ServoCalibration c(normal);
  assert(!c.setReversed(0,true));
  c.start();assert(!c.setReversed(5,true));
  assert(c.preview(0,1500,0));
  assert(c.setReversed(4,true) && !c.previewEnabled(0));
  assert(c.settings()[4].reversed && !c.settings()[0].reversed);
  assert(c.set(4,1200,1800) && c.settings()[4].reversed);
  assert(c.preview(4,1200,1) && c.previewPulse()==1200); // raw-us test is NOT inverted
  flex::ServoSettings restored[5];std::memcpy(restored,normal,sizeof restored);
  restored[2].reversed=true;restored[2].pin=99;
  assert(c.restore(restored) && c.settings()[2].reversed && c.settings()[2].pin==4);
  assert(c.setReversed(2,false) && !c.settings()[2].reversed);
  c.restoreDefaults();for(uint8_t i=0;i<5;++i) assert(!c.settings()[i].reversed);
  for(uint8_t selected=0;selected<5;++selected) {
    c.restoreDefaults();assert(c.setReversed(selected,true));
    for(uint8_t i=0;i<5;++i) assert(c.settings()[i].reversed==(i==selected));
  }
}
void serial_commands() {
  using K=flex::CommandKind;
  auto on=parseInversion("invert 4 1\r\n");
  assert(on.kind==K::Invert && on.channel==4 && on.first==1);
  assert(parseInversion("invert 0 0\n").kind==K::Invert);
  const char* invalid[]={"invert 0 2\n","invert 0 -1\n","invert 5 1\n","invert 0 true\n","invert 0\n","invert 0 1 extra\n","invert 0 65536\n","INVERT 0 1\n"};
  for(auto text:invalid) assert(parseInversion(text).kind==K::Invalid);
}
void legacyRecord(EEPROMClass& storage,const flex::ServoSettings* profile,uint32_t generation=7) {
  using namespace flex::calibration_storage;
  uint8_t bytes[kSlotSize];encodeRecord(profile,generation,bytes);
  bytes[3]=1;bytes[4]=5;write16(bytes+29,crc16(bytes+1,28));
  for(uint8_t i=0;i<kSlotSize;++i) storage.bytes[i]=bytes[i];
}
bool equalProfile(const flex::ServoSettings* a,const flex::ServoSettings* b) {
  for(uint8_t i=0;i<5;++i)
    if(a[i].minUs!=b[i].minUs || a[i].maxUs!=b[i].maxUs || a[i].reversed!=b[i].reversed) return false;
  return true;
}
void persistence_and_migration() {
  using namespace flex::calibration_storage;
  flex::ServoSettings profile[5];std::memcpy(profile,normal,sizeof profile);
  for(uint8_t i=0;i<5;++i) {profile[i].minUs+=i*10;profile[i].reversed=(i%2==0);}
  // Exercise every five-channel direction mask against the public v2 byte format.
  for(uint8_t mask=0;mask<32;++mask) {
    flex::ServoSettings combination[5];std::memcpy(combination,normal,sizeof combination);
    for(uint8_t i=0;i<5;++i) combination[i].reversed=bool((mask>>i)&1);
    EEPROMClass bits;assert(save(bits,combination) && bits.bytes[4]==mask);
    flex::ServoSettings roundtrip[5];std::memcpy(roundtrip,normal,sizeof roundtrip);
    assert(load(bits,roundtrip) && equalProfile(roundtrip,combination));
  }
  EEPROMClass storage;assert(save(storage,profile));
  flex::ServoSettings loaded[5];std::memcpy(loaded,normal,sizeof loaded);
  assert(load(storage,loaded) && equalProfile(profile,loaded));
  const int writes=storage.writes;assert(save(storage,profile) && storage.writes==writes);
  profile[0].reversed=false;assert(save(storage,profile) && storage.writes>writes);
  assert(load(storage,loaded) && equalProfile(profile,loaded));
  EEPROMClass legacy;legacyRecord(legacy,normal);
  std::memcpy(loaded,profile,sizeof loaded);
  assert(load(legacy,loaded) && equalProfile(loaded,normal) && legacy.writes==0);
  assert(save(legacy,normal)); // explicit save upgrades v1, even with identical limits
  assert(legacy.bytes[kSlotSize+3]==2);
  int migratedWrites=legacy.writes;assert(save(legacy,normal) && legacy.writes==migratedWrites);
  for(int budget=0;budget<36;++budget) {
    EEPROMClass interrupted;legacyRecord(interrupted,normal);interrupted.allowed=budget;
    try {save(interrupted,profile);} catch(const PowerLoss&) {}
    assert(load(interrupted,loaded));
    assert(equalProfile(loaded,normal) || equalProfile(loaded,profile));
  }
  // Interrupted writes in the new schema must keep orientation and limits coherent.
  EEPROMClass baseline;assert(save(baseline,normal));
  for(int budget=0;budget<36;++budget) {
    EEPROMClass interrupted=baseline;interrupted.allowed=budget;
    try {save(interrupted,profile);} catch(const PowerLoss&) {}
    assert(load(interrupted,loaded));
    assert(equalProfile(loaded,normal) || equalProfile(loaded,profile));
  }
  // A real v1 profile can have non-default endpoints in its second/latest slot.
  flex::ServoSettings oldProfile[5];std::memcpy(oldProfile,normal,sizeof oldProfile);
  oldProfile[0].minUs=1120;oldProfile[0].maxUs=1880;
  oldProfile[4].minUs=1160;oldProfile[4].maxUs=1840;
  EEPROMClass legacyBoth;legacyRecord(legacyBoth,normal,7);
  EEPROMClass secondSlot;legacyRecord(secondSlot,oldProfile,8);
  for(uint8_t i=0;i<kSlotSize;++i) legacyBoth.bytes[kSlotSize+i]=secondSlot.bytes[i];
  assert(load(legacyBoth,loaded) && equalProfile(loaded,oldProfile));
  for(int budget=0;budget<36;++budget) {
    EEPROMClass interrupted=legacyBoth;interrupted.allowed=budget;
    try {save(interrupted,profile);} catch(const PowerLoss&) {}
    assert(load(interrupted,loaded));
    assert(equalProfile(loaded,oldProfile) || equalProfile(loaded,profile));
  }
  EEPROMClass invalid;uint8_t bytes[kSlotSize];encodeRecord(profile,10,bytes);
  bytes[4]|=0x20;write16(bytes+29,crc16(bytes+1,28)); // CRC-valid reserved direction bit
  for(uint8_t i=0;i<kSlotSize;++i) invalid.bytes[i]=bytes[i];
  assert(!load(invalid,loaded));
  legacy.bytes[kSlotSize+4]^=1; // corrupt newest direction mask without updating CRC
  assert(load(legacy,loaded) && equalProfile(loaded,normal));
  for(size_t i=kStorageBytes;i<legacy.bytes.size();++i) assert(legacy.bytes[i]==0xff);
}
int main(){mapping_and_session();serial_commands();persistence_and_migration();std::puts("Individual inversion + EEPROM v1/v2 migration checks passed");}
