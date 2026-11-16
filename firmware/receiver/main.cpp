#include <Arduino.h>
#include <Servo.h>
#include "flex/receiver.hpp"
#include "flex/servo_output.hpp"
#include "flex/hardware.hpp"
#include "flex/radio.hpp"
RF24 radio(hardware::kCePin, hardware::kCsnPin);
Servo servos[flex::kFingers];
flex::Receiver receiver;

void setup() {
  Serial.begin(115200);
  if (!hardware::configureRadio(radio)) {
    Serial.println(F("# ERROR: nRF24L01+ not responding or 250 kbps unsupported"));
    while (true) delay(1000);
  }
  radio.openReadingPipe(1, hardware::kAddress);
  radio.startListening();
  Serial.println(F("# Waiting for valid commands; all five servos detached"));
  Serial.println(F("# angle0,angle1,angle2,angle3,angle4 (ignored when channel invalid)"));
}

void loop() {
  // Bound the drain so timeout/output handling always gets CPU time.
  for (uint8_t pending = 0; pending < 3 && radio.available(); ++pending) {
    uint8_t bytes[flex::kPacketSize];
    radio.read(bytes, sizeof(bytes));
    if (!receiver.receive(bytes, sizeof(bytes), millis())) continue;
    for (uint8_t i = 0; i < flex::kFingers; ++i) {
      Serial.print(receiver.command(i).angle);
      Serial.print(i + 1 == flex::kFingers ? '\n' : ',');
    }
  }
  receiver.tick(millis());
  for (uint8_t i = 0; i < flex::kFingers; ++i)
    flex::applyServo(servos[i], receiver.command(i), hardware::kServos[i]);
}
