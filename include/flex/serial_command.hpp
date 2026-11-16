#ifndef FLEX_SERIAL_COMMAND_HPP
#define FLEX_SERIAL_COMMAND_HPP
#include "servo_output.hpp"
#include <string.h>
namespace flex {
enum class CommandKind {None,Invalid,Help,Show,Calibrate,Set,Test,Stop,Save,Load,Defaults,Run};
struct SerialCommand {
  CommandKind kind=CommandKind::None;
  uint8_t channel=0;
  uint16_t first=0,second=0;
};
inline bool parseUnsigned(const char* text,uint16_t& value) {
  if(!*text) return false;
  uint32_t number=0;
  while(*text) {
    if(*text<'0' || *text>'9') return false;
    number=number*10+(*text++-'0');
    if(number>65535) return false;
  }
  value=static_cast<uint16_t>(number);return true;
}
inline uint8_t splitCommand(char* line,char** tokens) {
  uint8_t count=0;
  while(*line) {
    if(*line==' ' || *line=='\t') {++line;continue;}
    if(count==4) return 5;
    tokens[count++]=line;
    while(*line && *line!=' ' && *line!='\t') ++line;
    if(*line) *line++='\0';
  }
  return count;
}
inline CommandKind simpleCommand(const char* name) {
  const char* names[]={"help","show","cal","stop","save","load","defaults","run"};
  const CommandKind kinds[]={CommandKind::Help,CommandKind::Show,CommandKind::Calibrate,
    CommandKind::Stop,CommandKind::Save,CommandKind::Load,CommandKind::Defaults,CommandKind::Run};
  for(uint8_t i=0;i<8;++i) if(!strcmp(name,names[i])) return kinds[i];
  return CommandKind::Invalid;
}
inline SerialCommand numericCommand(char** tokens,uint8_t count) {
  SerialCommand result;result.kind=CommandKind::Invalid;
  const bool set=count==4 && !strcmp(tokens[0],"set");
  const bool test=count==3 && !strcmp(tokens[0],"test");
  if(!set && !test) return result;
  uint16_t channel;
  if(!parseUnsigned(tokens[1],channel) || channel>=kFingers ||
     !parseUnsigned(tokens[2],result.first)) return result;
  result.channel=static_cast<uint8_t>(channel);
  if(set) {
    if(!parseUnsigned(tokens[3],result.second) ||
       !validServoSettings({0,result.first,result.second})) return result;
    result.kind=CommandKind::Set;
  } else if(result.first>=544 && result.first<=2400) result.kind=CommandKind::Test;
  return result;
}
inline SerialCommand parseCommand(char* line) {
  char* tokens[4];const uint8_t count=splitCommand(line,tokens);
  SerialCommand result;
  if(count==0) return result;
  if(count==1) {result.kind=simpleCommand(tokens[0]);return result;}
  if(count>4) {result.kind=CommandKind::Invalid;return result;}
  return numericCommand(tokens,count);
}
class CommandReader {
 public:
  SerialCommand push(char c) {
    SerialCommand result;
    if(c=='\r' || c=='\n') {
      buffer_[used_]='\0';
      if(overflow_) result.kind=CommandKind::Invalid;
      else result=parseCommand(buffer_);
      used_=0;overflow_=false;return result;
    }
    // Reject control/non-ASCII bytes instead of allowing embedded-NUL truncation.
    if((c<' ' && c!='\t') || static_cast<uint8_t>(c)>126) overflow_=true;
    if(static_cast<size_t>(used_)+1<sizeof buffer_) buffer_[used_++]=c;
    else overflow_=true;
    return result;
  }
 private:
  char buffer_[64]={};
  uint8_t used_=0;
  bool overflow_=false;
};
}
#endif
