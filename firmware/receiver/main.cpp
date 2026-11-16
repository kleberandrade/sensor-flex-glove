#include <Arduino.h>
#include <Servo.h>
#include <EEPROM.h>
#include "flex/servo_calibration.hpp"
#include "flex/calibration_storage.hpp"
#include "flex/serial_command.hpp"
#include "flex/receiver.hpp"
#include "flex/servo_output.hpp"
#include "flex/servo_motion.hpp"
#include "flex/hardware.hpp"
#include "flex/radio.hpp"
RF24 radio(hardware::kCePin, hardware::kCsnPin);
Servo servos[flex::kFingers];
flex::ServoMotion motion[flex::kFingers];
flex::Receiver receiver;
flex::ServoCalibration calibration(hardware::kServos);
flex::CommandReader commandReader;

void detachAll() {
  for(uint8_t i=0;i<flex::kFingers;++i) {
    if(servos[i].attached()) servos[i].detach();
    motion[i].pause();
  }
}

void showSettings() {
  Serial.println(F("# channel,pin,min_us,max_us,reversed,speed_us_per_s"));
  for(uint8_t i=0;i<flex::kFingers;++i) {
    const flex::ServoSettings& settings=calibration.settings()[i];
    Serial.print(F("# "));Serial.print(i);Serial.print(',');Serial.print(settings.pin);
    Serial.print(',');Serial.print(settings.minUs);Serial.print(',');Serial.print(settings.maxUs);
    Serial.print(',');Serial.print(settings.reversed?1:0);
    Serial.print(',');Serial.println(settings.speedUsPerSecond);
  }
}

void loadCalibration() {
  flex::ServoSettings values[flex::kFingers];
  for(uint8_t i=0;i<flex::kFingers;++i) values[i]=hardware::kServos[i];
  const bool loaded=flex::calibration_storage::load(EEPROM,values);
  calibration.restore(values);
  Serial.println(loaded?F("# OK loaded EEPROM"):F("# EEPROM invalid/empty; using defaults"));
}

bool handleSessionCommand(flex::CommandKind kind) {
  using K=flex::CommandKind;
  switch(kind) {
    case K::None:return true;
    case K::Invalid:Serial.println(F("# ERR invalid command or values"));return true;
    case K::Help:
      Serial.println(F("# help | show | cal | set CH MIN_US MAX_US | invert CH 0|1 | speed CH US_PER_S | test CH US | stop | save | load | defaults | run"));
      return true;
    case K::Show:showSettings();return true;
    case K::Calibrate:case K::Stop:
      calibration.start();detachAll();receiver=flex::Receiver();radio.flush_rx();
      Serial.println(F("# OK cal: radio ignored; servos detached"));return true;
    case K::Run:
      detachAll();receiver=flex::Receiver();radio.flush_rx();calibration.run();
      Serial.println(F("# OK run: waiting for fresh radio commands"));return true;
    default:return false;
  }
}

void handleCommand(flex::SerialCommand command) {
  using K=flex::CommandKind;
  if(handleSessionCommand(command.kind)) return;
  if(!calibration.active()) {Serial.println(F("# ERR enter cal first"));return;}
  switch(command.kind) {
    case K::Set:
      if(calibration.set(command.channel,command.first,command.second)) {
        detachAll();Serial.println(F("# OK limits changed in RAM; save explicitly"));
      } else Serial.println(F("# ERR invalid limits"));
      break;
    case K::Invert:
      if(calibration.setReversed(command.channel,command.first!=0)) {
        detachAll();Serial.println(F("# OK direction changed in RAM; save explicitly"));
      } else Serial.println(F("# ERR invalid channel"));
      break;
    case K::Speed:
      if(calibration.setSpeed(command.channel,command.first)) {
        detachAll();Serial.println(F("# OK speed changed in RAM; save explicitly"));
      } else Serial.println(F("# ERR invalid speed or channel"));
      break;
    case K::Test:
      if(calibration.preview(command.channel,command.first,millis())) {
        detachAll(); // break before make, even when switching to a lower channel
        Serial.println(F("# OK test: one servo, 1000 ms maximum"));
      } else Serial.println(F("# ERR test outside channel limits"));
      break;
    case K::Save:
      calibration.stop();detachAll();
      Serial.println(flex::calibration_storage::save(EEPROM,calibration.settings())?
        F("# OK saved (unchanged current profiles do not write)"):F("# ERR EEPROM save failed"));
      break;
    case K::Load:calibration.stop();detachAll();loadCalibration();showSettings();break;
    case K::Defaults:calibration.restoreDefaults();detachAll();
      Serial.println(F("# OK defaults in RAM; save explicitly"));showSettings();break;
    default:break;
  }
}

void readCommands() {
  // Bounded, nonblocking input: no String allocation or read-until timeout.
  for(uint8_t i=0;i<16 && Serial.available();++i)
    handleCommand(commandReader.push(static_cast<char>(Serial.read())));
}

void setup() {
  Serial.begin(115200);
  detachAll();receiver=flex::Receiver();calibration.run();commandReader=flex::CommandReader();
  for(uint8_t i=0;i<flex::kFingers;++i) motion[i]=flex::ServoMotion();
  loadCalibration();
  if (!hardware::configureRadio(radio)) {
    Serial.println(F("# ERROR: nRF24L01+ not responding or 250 kbps unsupported"));
    while (true) delay(1000);
  }
  radio.flush_rx();
  radio.openReadingPipe(1, hardware::kAddress);
  radio.startListening();
  Serial.println(F("# Waiting for valid commands; all five servos detached"));
  Serial.println(F("# Send help for serial servo calibration; type cal before editing"));
  Serial.println(F("# valid_mask,angle0,angle1,angle2,angle3,angle4"));
}

void loop() {
  readCommands();
  // Bound the drain so timeout/output handling always gets CPU time.
  for (uint8_t pending = 0; pending < 3 && radio.available(); ++pending) {
    uint8_t bytes[flex::kPacketSize];
    radio.read(bytes, sizeof(bytes));
    if (calibration.active() || !receiver.receive(bytes, sizeof(bytes), millis())) continue;
    Serial.print(receiver.enabledMask());
    Serial.print(',');
    for (uint8_t i = 0; i < flex::kFingers; ++i) {
      Serial.print(receiver.command(i).angle);
      Serial.print(i + 1 == flex::kFingers ? '\n' : ',');
    }
  }
  receiver.tick(millis());
  calibration.tick(millis());
  for (uint8_t i = 0; i < flex::kFingers; ++i) {
    const bool wasAttached = servos[i].attached();
    const flex::ServoSettings settings=calibration.settings()[i];
    if(calibration.active()) {
      const bool enabled=calibration.previewEnabled(i);
      flex::applyPulse(servos[i],enabled,calibration.previewPulse(),settings);
      if(enabled) motion[i].observe(calibration.previewPulse());
      else motion[i].pause();
    } else {
      const bool enabled=receiver.command(i).enabled;
      uint16_t pulse=0;
      if(enabled) pulse=motion[i].update(flex::pulseFor(receiver.command(i).angle,settings),settings,millis());
      else motion[i].pause();
      flex::applyPulse(servos[i],enabled,pulse,settings);
    }
    if (wasAttached != servos[i].attached()) {
      Serial.print(servos[i].attached() ? F("# attached channel ") :
                                        F("# detached channel "));
      Serial.println(i);
    }
  }
}
