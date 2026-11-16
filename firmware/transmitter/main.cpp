#include <Arduino.h>
#include "flex/core.hpp"
#include "flex/hardware.hpp"
#ifndef FLEX_READER_ONLY
#include "flex/radio.hpp"
RF24 radio(hardware::kCePin, hardware::kCsnPin);
#endif

flex::Average filters[hardware::kActiveFingers];
flex::Calibration calibration[hardware::kActiveFingers];
uint32_t calibrationStart = 0;
uint32_t lastSample = 0;
uint16_t sequence = 0;
bool calibrating = true;

void beginCalibration() {
  for (uint8_t i = 0; i < hardware::kActiveFingers; ++i) {
    filters[i].reset();
    calibration[i].reset();
  }
  calibrationStart = millis();
  calibrating = true;
  Serial.println(F("# Move connected fingers through full range for 5 seconds"));
}

void setup() {
  Serial.begin(115200);
#ifndef FLEX_READER_ONLY
  if (!hardware::configureRadio(radio)) {
    Serial.println(F("# ERROR: nRF24L01+ not responding or 250 kbps unsupported"));
    while (true) delay(1000);
  }
  radio.openWritingPipe(hardware::kAddress);
  radio.stopListening();
#endif
  Serial.println(F("# ms,channel,raw,filtered,angle,valid,ack (-1 = reader only)"));
  beginCalibration();
}

void loop() {
  if (Serial.available() && Serial.read() == 'c') beginCalibration();
  const uint32_t now = millis();
  if (static_cast<uint32_t>(now - lastSample) < hardware::kSampleMs) return;
  lastSample = now;
  if (static_cast<uint32_t>(now - calibrationStart) >= hardware::kCalibrationMs)
    calibrating = false;

  flex::Packet packet = {sequence++, 0, {10, 10, 10, 10, 10}};
  uint16_t raw[hardware::kActiveFingers];
  uint16_t filtered[hardware::kActiveFingers];
  for (uint8_t i = 0; i < hardware::kActiveFingers; ++i) {
    // Discard the first conversion after changing the ADC multiplexer.
    analogRead(hardware::kAnalogPins[i]);
    raw[i] = analogRead(hardware::kAnalogPins[i]);
    filtered[i] = filters[i].add(raw[i]);
    if (calibrating) calibration[i].observe(filtered[i]);
    if (!calibrating && calibration[i].angle(filtered[i], packet.angles[i]))
      packet.mask |= static_cast<uint8_t>(1U << i);
  }
  int ack = -1;
#ifndef FLEX_READER_ONLY
  uint8_t bytes[flex::kPacketSize];
  flex::encode(packet, bytes);
  ack = radio.write(bytes, sizeof(bytes)) ? 1 : 0;
#endif
  for (uint8_t i = 0; i < hardware::kActiveFingers; ++i) {
    Serial.print(now); Serial.print(',');
    Serial.print(i); Serial.print(',');
    Serial.print(raw[i]); Serial.print(',');
    Serial.print(filtered[i]); Serial.print(',');
    Serial.print(packet.angles[i]); Serial.print(',');
    Serial.print((packet.mask >> i) & 1); Serial.print(',');
    Serial.println(ack);
  }
}
