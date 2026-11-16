# Third-party software and references

The MIT license covers the repository author's code and original documentation,
including `hardware/circuit.svg`. No supplier schematic or datasheet is copied
into this repository.

Build dependencies are downloaded separately and retain their own licenses:

- [RF24 1.6.2](https://github.com/nRF24/RF24/tree/v1.6.2): GPL-2.0.
- [Servo 1.3.0](https://github.com/arduino-libraries/Servo/tree/1.3.0): LGPL-2.1-or-later.
- [Arduino AVR core](https://github.com/arduino/ArduinoCore-avr): LGPL-2.1-or-later, with file-specific notices and exceptions.
- [PlatformIO Core](https://github.com/platformio/platformio-core): Apache-2.0.

The repository's MIT license does not relicense these dependencies or the linked
hardware documents. In particular, firmware linked with RF24 is subject to its
GPL requirements when distributed. Preserve applicable source and notices when
sharing compiled firmware.
