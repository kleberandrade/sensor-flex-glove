#ifndef FLEX_TEST_EEPROM_H
#define FLEX_TEST_EEPROM_H
#include <array>
#include <cassert>
#include <stdint.h>
struct PowerLoss {};
struct EEPROMClass {
  std::array<uint8_t,128> bytes;
  int writes=0,allowed=-1;
  uint16_t capacity=128;
  bool readOnly=false;
  EEPROMClass() {bytes.fill(0xff);}
  uint16_t length() {return capacity;}
  uint8_t read(int address) {assert(address>=0 && address<capacity);return bytes[address];}
  void update(int address,uint8_t value) {
    assert(address>=0 && address<capacity);
    if(readOnly || bytes[address]==value) return;
    if(allowed==0) throw PowerLoss{};
    if(allowed>0) --allowed;
    bytes[address]=value;++writes;
  }
};
extern EEPROMClass EEPROM;
#endif
