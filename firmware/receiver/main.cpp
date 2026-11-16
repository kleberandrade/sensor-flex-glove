#include <Arduino.h>
#include <Servo.h>
#include "flex/core.hpp"
#include "flex/hardware.hpp"
#include "flex/radio.hpp"
RF24 radio(hardware::kCePin, hardware::kCsnPin);
Servo servo;
uint32_t lastValid = 0;

void setup() {
  Serial.begin(115200);
  if (!hardware::configureRadio(radio)) {
    Serial.println(F("# ERROR: nRF24L01+ not responding or 250 kbps unsupported"));
    while (true) delay(1000);
  }
  radio.openReadingPipe(1, hardware::kAddress);
  radio.startListening();
  Serial.println(F("# Waiting for valid finger 0; servo detached"));
}

void loop() {
  // Drain at most the hardware FIFO capacity so timeout always gets CPU time.
  for (uint8_t pending = 0; pending < 3 && radio.available(); ++pending) {
    uint8_t bytes[flex::kPacketSize];
    radio.read(bytes, sizeof(bytes));
    flex::Packet packet = {};
    if (!flex::decode(bytes, sizeof(bytes), packet)) continue;
    if (!(packet.mask & 1)) {
      servo.detach();
      continue;
    }
    lastValid = millis();
    // Set the target before attach, avoiding the Servo library's default pulse.
    servo.write(packet.angles[0]);
    if (!servo.attached()) servo.attach(hardware::kServoPin);
    Serial.println(packet.angles[0]);
  }
  if (servo.attached() && flex::expired(millis(), lastValid)) {
    servo.detach();
    Serial.println(F("# TIMEOUT: servo detached"));
  }
}
