#include <cassert>
#include <cstdio>
#include "../firmware/receiver/main.cpp"
EEPROMClass EEPROM;
uint32_t fakeNow = 0;
FakeSerial Serial;

void command(const char* line) {
  Serial.feed(line);
  for(int count=0;count<20 && Serial.available();++count) loop();
  assert(!Serial.available());
}
void enqueue(flex::Packet packet) {
  std::array<uint8_t, flex::kPacketSize> bytes = {};
  flex::encode(packet, bytes.data());
  radio.frames.push_back(bytes);
}
int main() {
  setup();
  assert(EEPROM.writes==0);
  for (uint8_t i = 0; i < 5; ++i) assert(!servos[i].attached());
  enqueue({1, 31, {10, 50, 90, 130, 170}});
  fakeNow = 100;
  loop();
  const int pulses[] = {1000, 1250, 1500, 1750, 2000};
  for (uint8_t i = 0; i < 5; ++i) {
    assert(servos[i].attached() && servos[i].pin == i + 2);
    assert(servos[i].pulse == pulses[i] && servos[i].pulseAtAttach == pulses[i]);
  }
  // A complete but malformed RF frame cannot move or refresh any servo.
  enqueue({2, 31, {90, 90, 90, 90, 90}});
  radio.frames.back()[9] = 171;
  fakeNow = 599;
  loop();
  for (uint8_t i = 0; i < 5; ++i)
    assert(servos[i].attached() && servos[i].pulse == pulses[i]);
  fakeNow = 600;
  loop();
  for (uint8_t i = 0; i < 5; ++i) assert(!servos[i].attached());
  enqueue({3, 16, {10, 10, 10, 10, 90}});
  fakeNow = 601;
  loop();
  for (uint8_t i = 0; i < 5; ++i) assert(servos[i].attached() == (i == 4));
  assert(servos[4].pin == 6 && servos[4].pulseAtAttach == 1500);
  enqueue({4, 0, {10, 10, 10, 10, 10}});
  loop();
  for (uint8_t i = 0; i < 5; ++i) assert(!servos[i].attached());
  // Artificially overfill the double to ensure the adapter drains at most 3.
  for (int i = 0; i < 4; ++i) enqueue({5, 1, {50, 10, 10, 10, 10}});
  loop();
  assert(radio.frames.size() == 1);
  assert(servos[0].attached() && servos[0].pulse == 1250);
  command("set 0 1100 1900\n"); // RF mode must reject editing
  assert(Serial.output.find("ERR enter cal")!=std::string::npos);
  command("cal\n");
  for(uint8_t i=0;i<5;++i) assert(!servos[i].attached());
  command("set 4 1100 1900\n");
  command("test 4 1900\n");
  assert(servos[4].attached() && servos[4].pulse==1900);
  enqueue({6,31,{10,10,10,10,10}});loop();
  assert(servos[4].pulse==1900 && !servos[0].attached()); // RF suppressed
  fakeNow+=999;loop();assert(servos[4].attached());
  command("test 4 1901\n"); // invalid preview must not extend its deadline
  fakeNow+=1;loop();assert(!servos[4].attached());
  command("test 4 1500\n");assert(servos[4].attached());
  command("test 0 1500\n");
  assert(servos[0].attached() && !servos[4].attached());
  assert(servos[0].concurrentAtAttach==1);
  command("save\n"); // no servo remains driven during EEPROM programming
  for(uint8_t i=0;i<5;++i) assert(!servos[i].attached());
  int writes=EEPROM.writes;
  command("save\n");assert(EEPROM.writes==writes);
  command("defaults\n");
  command("test 4 2000\n");assert(servos[4].pulse==2000);
  command("load\n");
  command("test 4 2000\n");assert(!servos[4].attached()); // restored 1900 max
  command("test 4 1100\n");assert(servos[4].pulse==1100);
  command("stop\n");assert(!servos[4].attached());
  enqueue({7,16,{10,10,10,10,170}});
  command("run\n"); // queued data must not resume motion
  assert(!servos[4].attached());
  enqueue({8,16,{10,10,10,10,170}});loop();assert(servos[4].pulse==1900);
  command("cal\n");command("set 4 1200 1800\n"); // unsaved edit
  setup(); // simulated setup reload from EEPROM, not the changed RAM profile
  enqueue({9,16,{10,10,10,10,170}});loop();assert(servos[4].pulse==1900);
  command("show\n");assert(Serial.output.find("4,6,1100,1900")!=std::string::npos);
  command("save\n");assert(Serial.output.find("ERR enter cal")!=std::string::npos);
  command("cal\n");
  EEPROM.readOnly=true;
  command("set 4 1200 1800\n");command("save\n");
  assert(Serial.output.find("ERR EEPROM save failed")!=std::string::npos);
  EEPROM.readOnly=false;
  EEPROM.bytes.fill(0xff); // both records lost: explicit load falls back to defaults
  command("load\n");command("test 4 2000\n");
  assert(servos[4].attached() && servos[4].pulse==2000);
  command("help\n");assert(Serial.output.find("set CH MIN_US MAX_US")!=std::string::npos);
  command("set 5 1100 1900\n");
  assert(Serial.output.find("ERR invalid command")!=std::string::npos);
  command("stop\n");
  command("test 4 1500\n");assert(servos[4].attached());
  command("invert 4 1\n");assert(!servos[4].attached());
  command("set 4 1100 1900\n"); // endpoint edits must preserve orientation
  command("test 4 1100\n");assert(servos[4].pulse==1100); // raw preview stays literal
  command("save\n");command("show\n");
  assert(Serial.output.find("channel,pin,min_us,max_us,reversed")!=std::string::npos);
  assert(Serial.output.find("4,6,1100,1900,1")!=std::string::npos);
  command("run\n");enqueue({10,31,{10,10,10,10,10}});loop();
  for(uint8_t i=0;i<4;++i) assert(servos[i].pulse==1000);
  assert(servos[4].pulse==1900);
  command("invert 4 0\n"); // rejected outside calibration
  enqueue({11,16,{10,10,10,10,170}});loop();assert(servos[4].pulse==1100);
  command("cal\n");command("invert 4 0\n");command("load\n");
  command("defaults\n");command("load\n");
  setup();enqueue({12,16,{10,10,10,10,10}});loop();assert(servos[4].pulse==1900);
  command("cal\n");command("invert 4 0\n");command("save\n");
  setup();enqueue({13,16,{10,10,10,10,10}});loop();assert(servos[4].pulse==1100);
  command("cal\n");command("test 4 1500\n");
  command("speed 4 200\n");assert(!servos[4].attached());
  command("set 4 1100 1900\n");command("invert 4 1\n");command("invert 4 0\n");
  command("save\n");command("show\n");
  assert(Serial.output.find("4,6,1100,1900,0,200")!=std::string::npos);
  command("run\n");enqueue({14,31,{170,170,170,170,170}});loop();
  assert(servos[0].pulse==2000 && servos[4].pulse==1500);
  fakeNow+=100;enqueue({15,31,{170,170,170,170,170}});loop();assert(servos[4].pulse==1520);
  fakeNow+=100;enqueue({16,16,{10,10,10,10,10}});loop();assert(servos[4].pulse==1500);
  fakeNow+=500;loop();assert(!servos[4].attached()); // no wait for a ramp to finish
  enqueue({17,16,{10,10,10,10,10}});loop();assert(servos[4].pulse==1500);
  fakeNow+=100;enqueue({18,16,{10,10,10,10,10}});loop();assert(servos[4].pulse==1480);
  command("speed 4 10000\n"); // rejected in RF mode
  assert(calibration.settings()[4].speedUsPerSecond==200);
  command("cal\n");command("test 4 1800\n");assert(servos[4].pulse==1800);
  command("run\n");enqueue({19,16,{10,10,10,10,10}});loop();assert(servos[4].pulse==1800);
  setup();enqueue({20,16,{10,10,10,10,10}});loop();assert(servos[4].pulse==1500);
  fakeNow+=100;enqueue({21,16,{10,10,10,10,10}});loop();assert(servos[4].pulse==1480);
  command("cal\n");command("speed 4 0\n");command("run\n");
  enqueue({22,16,{10,10,10,10,170}});loop();assert(servos[4].pulse==1900);
  command("cal\n");command("defaults\n");command("load\n");command("show\n");
  assert(calibration.settings()[4].speedUsPerSecond==200);
  std::puts("Receiver firmware + serial limits/inversion/speed/EEPROM checks passed (host doubles)");
}
