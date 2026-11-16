#include "flex/servo_calibration.hpp"
#include "flex/calibration_storage.hpp"
#include "flex/serial_command.hpp"
#include <EEPROM.h>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <string>
const flex::ServoSettings defaults[5] = {{2,1000,2000},{3,1000,2000},{4,1000,2000},{5,1000,2000},{6,1000,2000}};

flex::SerialCommand parse(const char* line) {
  flex::CommandReader reader;
  flex::SerialCommand result = {};
  while (*line) {
    flex::SerialCommand next = reader.push(*line++);
    if (next.kind != flex::CommandKind::None) result = next;
  }
  return result;
}
void commands() {
  using K=flex::CommandKind;
  assert(parse("help\n").kind==K::Help);
  assert(parse("show\r\n").kind==K::Show);
  assert(parse("cal\n").kind==K::Calibrate);
  assert(parse("stop\n").kind==K::Stop);
  assert(parse("save\n").kind==K::Save);
  assert(parse("load\n").kind==K::Load);
  assert(parse("defaults\n").kind==K::Defaults);
  assert(parse("run\n").kind==K::Run);
  auto set=parse(" \tset 4 1100 1900 \r\n");
  assert(set.kind==K::Set && set.channel==4 && set.first==1100 && set.second==1900);
  assert(parse("test 0 1500\n").kind==K::Test);
  const char* bad[]={"set 5 1000 2000\n","set -1 1000 2000\n","set 0 2000 1000\n","set 0 1000 1000\n","set 0 543 2000\n","set 0 1000 2401\n","set 0 65536 2000\n","set 0 999999999999 2000\n","set 0 1e3 2000\n","set 0 1000\n","set 0 1000 2000 extra\n","save extra\n","TEST 0 1500\n","test 0 -1\n","test 5 1500\n","test 0 2401\n"};
  for (auto line:bad) assert(parse(line).kind==K::Invalid);
  flex::CommandReader reader;
  for (int i=0;i<90;++i) assert(reader.push('x').kind==K::None);
  assert(reader.push('\n').kind==K::Invalid);
  for (char c:std::string("show\n")) {
    auto result=reader.push(c);
    if(c=='\n') assert(result.kind==K::Show);
  }
  assert(parse("\t  \n").kind==K::None);
  flex::CommandReader binary;
  for(char c:std::string("save")) binary.push(c);
  binary.push('\0');
  assert(binary.push('\n').kind==K::Invalid);
  assert(parse("unknown\n").kind==K::Invalid);
  assert(parse("test 0 543\n").kind==K::Invalid);
}
void session() {
  flex::ServoCalibration c(defaults);
  assert(!c.active());
  assert(!c.set(0,1100,1900));
  assert(!c.preview(0,1500,0));
  c.start();
  assert(!c.previewEnabled(5));
  assert(c.set(4,1100,1900));
  assert(c.settings()[4].pin==6 && c.settings()[0].minUs==1000);
  assert(!c.set(5,1100,1900) && !c.set(0,2000,1000));
  assert(!c.preview(4,1901,0) && !c.preview(5,1500,0));
  assert(c.preview(4,1900,0xfffffff0U));
  c.tick(983); assert(c.previewEnabled(4) && !c.previewEnabled(0));
  c.tick(984); assert(!c.previewEnabled(4)); // 1000ms after wrap
  assert(c.preview(0,1500,1000));
  assert(c.preview(1,1600,1001));
  assert(!c.previewEnabled(0) && c.previewEnabled(1) && c.previewPulse()==1600);
  assert(c.set(1,1200,1800) && !c.previewEnabled(1));
  flex::ServoSettings invalid[5];std::memcpy(invalid,defaults,sizeof invalid);
  invalid[0].minUs=1200;invalid[4].maxUs=500;
  assert(!c.restore(invalid) && c.settings()[0].minUs==1000);
  assert(c.preview(0,1500,1010));c.stop();assert(!c.previewEnabled(0));
  flex::ServoSettings restored[5];std::memcpy(restored,defaults,sizeof restored);
  restored[2]={99,1200,1800}; // restore limits, never change firmware pin mapping
  assert(c.restore(restored) && c.settings()[2].pin==4 && c.settings()[2].minUs==1200);
  c.restoreDefaults();assert(c.settings()[4].minUs==1000);
  c.run(); assert(!c.active());
}
void storage() {
  using namespace flex::calibration_storage;
  const uint8_t vector[]={'1','2','3','4','5','6','7','8','9'};
  assert(crc16(vector,9)==0x29b1);
  EEPROMClass memory;
  flex::ServoSettings loaded[5];std::memcpy(loaded,defaults,sizeof loaded);
  assert(!load(memory,loaded));
  assert(save(memory,defaults));
  int writes=memory.writes;
  assert(save(memory,defaults) && memory.writes==writes);
  flex::ServoSettings changed[5];std::memcpy(changed,defaults,sizeof changed);
  changed[4].minUs=1100;changed[4].maxUs=1900;
  assert(save(memory,changed) && load(memory,loaded));
  assert(loaded[4].minUs==1100 && loaded[4].pin==6 && loaded[0].minUs==1000);
  // Cut before every possible changed-byte write; reboot must recover old or new.
  EEPROMClass baseline;assert(save(baseline,defaults));
  for(int budget=0;budget<kSlotSize+5;++budget) {
    EEPROMClass interrupted=baseline;interrupted.allowed=budget;
    try {save(interrupted,changed);} catch(const PowerLoss&) {}
    assert(load(interrupted,loaded));
    assert(loaded[4].minUs==1000 || loaded[4].minUs==1100);
    assert(loaded[4].maxUs==(loaded[4].minUs==1000?2000:1900));
  }
  // Damage the newer record: fall back to the older intact one.
  memory.bytes[kBaseAddress+kSlotSize+10]^=1;
  assert(load(memory,loaded) && loaded[4].minUs==1000);
  EEPROMClass corrupt;
  assert(save(corrupt,changed));corrupt.bytes[kBaseAddress+3]=99;
  assert(!load(corrupt,loaded));
  EEPROMClass shortMemory;shortMemory.capacity=kStorageBytes-1;
  assert(!save(shortMemory,defaults) && !load(shortMemory,loaded));
  EEPROMClass unwritable;unwritable.readOnly=true;
  assert(!save(unwritable,defaults));
  // Five different profiles round-trip independently and preserve firmware pins.
  flex::ServoSettings unique[5];std::memcpy(unique,defaults,sizeof unique);
  for(uint8_t i=0;i<5;++i) {unique[i].minUs=1000+50*i;unique[i].maxUs=2000-50*i;}
  assert(save(baseline,changed));
  for(int budget=0;budget<kSlotSize+5;++budget) {
    EEPROMClass interrupted=baseline;interrupted.allowed=budget;
    try {save(interrupted,unique);} catch(const PowerLoss&) {}
    assert(load(interrupted,loaded));
    const bool newest=loaded[0].maxUs==unique[0].maxUs && loaded[1].minUs==unique[1].minUs;
    for(uint8_t i=0;i<5;++i) {
      assert(loaded[i].minUs==(newest?unique[i].minUs:changed[i].minUs));
      assert(loaded[i].maxUs==(newest?unique[i].maxUs:changed[i].maxUs));
      assert(loaded[i].pin==i+2);
    }
  }
  EEPROMClass wrapped;
  uint8_t record[kSlotSize];encodeRecord(defaults,0xffffffffUL,record);
  for(uint8_t i=0;i<kSlotSize;++i) wrapped.bytes[kBaseAddress+i]=record[i];
  assert(save(wrapped,unique) && load(wrapped,loaded));
  for(uint8_t i=0;i<5;++i) assert(loaded[i].minUs==unique[i].minUs);
  for(size_t i=kStorageBytes;i<wrapped.bytes.size();++i) assert(wrapped.bytes[i]==0xff);
  // A valid CRC cannot authorize invalid bounds or an unsupported schema.
  encodeRecord(defaults,2,record);write16(record+9,2401);write16(record+kSlotSize-2,crc16(record+1,kSlotSize-3));
  EEPROMClass invalidRecord;
  for(uint8_t i=0;i<kSlotSize;++i) invalidRecord.bytes[kBaseAddress+i]=record[i];
  assert(!load(invalidRecord,loaded));
  changed[0].minUs=2401;writes=baseline.writes;
  assert(!save(baseline,changed) && baseline.writes==writes);
}
int main(){commands();session();storage();std::puts("Serial calibration/session/EEPROM checks passed (host storage model)");}
