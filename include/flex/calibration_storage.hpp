#ifndef FLEX_CALIBRATION_STORAGE_HPP
#define FLEX_CALIBRATION_STORAGE_HPP
#include "servo_output.hpp"
namespace flex { namespace calibration_storage {
constexpr uint8_t kSlotSize=41,kLegacySlotSize=31;
constexpr uint16_t kBaseAddress=64,kStorageBytes=kBaseAddress+2*kSlotSize;
constexpr uint8_t kCommitted=0xa5,kSchema=3;
static_assert(kFingers==5,"EEPROM schemas require exactly five channel slots");
inline uint16_t crc16(const uint8_t* bytes,size_t size) {
  uint16_t crc=0xffff;
  for(size_t i=0;i<size;++i) {
    crc^=static_cast<uint16_t>(bytes[i])<<8;
    for(uint8_t bit=0;bit<8;++bit)
      crc=static_cast<uint16_t>((crc&0x8000)?(crc<<1)^0x1021:crc<<1);
  }
  return crc;
}
inline uint16_t read16(const uint8_t* p) {return uint16_t(p[0])|(uint16_t(p[1])<<8);}
inline void write16(uint8_t* p,uint16_t value) {p[0]=uint8_t(value);p[1]=uint8_t(value>>8);}
struct Record {
  uint32_t generation=0;
  uint8_t schema=0,reversedMask=0;
  uint16_t minUs[kFingers]={},maxUs[kFingers]={},speed[kFingers]={};
};
inline bool validRecordHeader(const uint8_t* bytes,bool legacy) {
  if(bytes[0]!=kCommitted || bytes[1]!='F' || bytes[2]!='S') return false;
  if(!legacy) return bytes[3]==kSchema && !(bytes[4]&0xe0);
  if(bytes[3]==1) return bytes[4]==kFingers;
  return bytes[3]==2 && !(bytes[4]&0xe0);
}
inline bool decodeRecord(const uint8_t* bytes,Record& record,bool legacy=false) {
  const uint8_t size=legacy?kLegacySlotSize:kSlotSize;
  if(!validRecordHeader(bytes,legacy) || read16(bytes+size-2)!=crc16(bytes+1,size-3)) return false;
  record.schema=bytes[3];record.reversedMask=bytes[3]==1?0:bytes[4];
  for(uint8_t i=0;i<kFingers;++i) {
    record.minUs[i]=read16(bytes+9+4*i);record.maxUs[i]=read16(bytes+11+4*i);
    record.speed[i]=legacy?0:read16(bytes+29+2*i);
    if(!validServoSettings({0,record.minUs[i],record.maxUs[i],false,record.speed[i]})) return false;
  }
  record.generation=0;
  for(uint8_t i=0;i<4;++i) record.generation|=uint32_t(bytes[5+i])<<(8*i);
  return true;
}
template<typename Storage>
bool readSlot(Storage& storage,uint8_t slot,Record& record,bool legacy=false) {
  const uint8_t size=legacy?kLegacySlotSize:kSlotSize;
  const uint16_t base=legacy?slot*kLegacySlotSize:kBaseAddress+slot*kSlotSize;
  uint8_t bytes[kSlotSize]={};
  for(uint8_t i=0;i<size;++i) bytes[i]=storage.read(base+i);
  return decodeRecord(bytes,record,legacy);
}
inline bool newer(uint32_t a,uint32_t b) {
  const uint32_t delta=a-b;return delta!=0 && delta<0x80000000UL;
}
template<typename Storage>
int8_t latestPair(Storage& storage,Record& record,bool legacy) {
  Record second;
  const bool firstValid=readSlot(storage,0,record,legacy);
  const bool secondValid=readSlot(storage,1,second,legacy);
  if(secondValid && (!firstValid || newer(second.generation,record.generation))) {record=second;return 1;}
  return firstValid?0:-1;
}
template<typename Storage>
int8_t latest(Storage& storage,Record& record) {
  if(storage.length()>=kStorageBytes) {
    const int8_t slot=latestPair(storage,record,false);
    if(slot>=0) return slot; // current bank wins; never merge profile fields
  }
  if(storage.length()>=2*kLegacySlotSize) return latestPair(storage,record,true);
  return -1;
}
template<typename Storage>
bool load(Storage& storage,ServoSettings* settings) {
  Record record;if(latest(storage,record)<0) return false;
  for(uint8_t i=0;i<kFingers;++i) {
    settings[i].minUs=record.minUs[i];settings[i].maxUs=record.maxUs[i];
    settings[i].reversed=bool((record.reversedMask>>i)&1);
    settings[i].speedUsPerSecond=record.speed[i];
  }
  return true;
}
inline void encodeRecord(const ServoSettings* settings,uint32_t generation,uint8_t* bytes) {
  bytes[0]=kCommitted;bytes[1]='F';bytes[2]='S';bytes[3]=kSchema;bytes[4]=0;
  for(uint8_t i=0;i<4;++i) bytes[5+i]=uint8_t(generation>>(8*i));
  for(uint8_t i=0;i<kFingers;++i) {
    if(settings[i].reversed) bytes[4]|=static_cast<uint8_t>(1U<<i);
    write16(bytes+9+4*i,settings[i].minUs);write16(bytes+11+4*i,settings[i].maxUs);
    write16(bytes+29+2*i,settings[i].speedUsPerSecond);
  }
  write16(bytes+kSlotSize-2,crc16(bytes+1,kSlotSize-3));
}
inline bool sameProfile(const Record& record,const ServoSettings* settings) {
  for(uint8_t i=0;i<kFingers;++i)
    if(settings[i].minUs!=record.minUs[i] || settings[i].maxUs!=record.maxUs[i] ||
       settings[i].reversed!=bool((record.reversedMask>>i)&1) || settings[i].speedUsPerSecond!=record.speed[i]) return false;
  return true;
}
template<typename Storage>
bool commitRecord(Storage& storage,uint16_t base,const uint8_t* bytes) {
  storage.update(base,0);
  for(uint8_t i=1;i<kSlotSize;++i) storage.update(base+i,bytes[i]);
  for(uint8_t i=1;i<kSlotSize;++i) if(storage.read(base+i)!=bytes[i]) return false;
  storage.update(base,kCommitted);return storage.read(base)==kCommitted;
}
template<typename Storage>
bool save(Storage& storage,const ServoSettings* settings) {
  if(storage.length()<kStorageBytes) return false;
  for(uint8_t i=0;i<kFingers;++i) if(!validServoSettings(settings[i])) return false;
  Record record;const int8_t previous=latest(storage,record);
  if(previous>=0 && record.schema==kSchema && sameProfile(record,settings)) return true;
  const uint16_t base=kBaseAddress+((record.schema==kSchema && previous==0)?kSlotSize:0);
  uint8_t bytes[kSlotSize];encodeRecord(settings,previous<0?1:record.generation+1,bytes);
  return commitRecord(storage,base,bytes);
}
} }
#endif
