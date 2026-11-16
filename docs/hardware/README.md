# Reference hardware

For an ordered assembly walkthrough, start with the [glove build guide](../BUILD_GUIDE.md).

## What is known

The original code uses five analog inputs, an RF24 radio with CE=D7/CSN=D8,
and one Servo output on D2. It contains no schematic, part list or photos.
Arduino Uno R3, Spectra Symbol FS-L-0095-103-ST and TowerPro SG90 below are
**reference choices**, not claims about the unidentified original hardware.
No Grove connector, I2C interface or amplified Grove flex module is assumed.

## Bill of materials

Quantities are for the default one-sensor wireless demonstration.

| Qty | Part | Specification / reason |
| --- | --- | --- |
| 2 | Arduino Uno R3 | ATmega328P, 5 V logic, 10-bit ADC, hardware SPI |
| 1 | Resistive flex sensor | Reference Spectra Symbol FS-L-0095-103-ST, 10 kΩ flat ±30% |
| 1 | Fixed resistor | 10 kΩ, 1%, at least 0.125 W, bottom leg of divider |
| 2 | nRF24L01+ module | Standard PCB antenna, no PA/LNA; 250 kbps capable |
| 2 each | Radio bypass capacitors | 100 nF ceramic + 10 µF, ≥6.3 V, at each radio VCC/GND |
| 1 | Positional hobby servo | Reference TowerPro SG90 digital; verify actual unit's supply/travel |
| 1 | Regulated servo supply | 4.8 V, budget 2 A for this reference servo; match actual load |
| 2 | USB cables and power sources | Power the Unos separately; servo current bypasses Uno |
| As needed | Breadboard, short jumpers, strain relief | Mount sensor without bending its terminals |

For the standalone `reader`, omit the second Uno, both radios, servo and servo
supply. For five sensors, add four identical sensor/resistor dividers and set
`kActiveFingers=5`. The included receiver still drives only finger 0.

## Circuit and pinout

![Electronics-only component overview](../images/electronics-overview.svg)

[Open original SVG circuit](circuit.svg). This is a labeled electrical reference,
not a PCB manufacturing file. Connections are identified by module pin names;
check the orientation of the module in hand rather than assuming a header order.

| Node | Uno pin | Connection | Electrical domain |
| --- | --- | --- | --- |
| Transmitter | 5V | Flex sensor top terminal | 5 V USB-powered Uno rail |
| Transmitter | A0 | Flex bottom terminal + 10 kΩ resistor top | Divider output, 0–5 V |
| Transmitter | GND | 10 kΩ resistor bottom | Local signal ground |
| Transmitter optional | A1–A4 | One additional divider per input | Never enable floating inputs |
| Both nodes | 3.3V | Radio VCC, bypassed locally | Radio supply only; never connect radio VCC to 5 V |
| Both nodes | GND | Radio GND, bypass capacitor negatives | Local common ground |
| Both nodes | D7 | Radio CE | Uno output → radio input |
| Both nodes | D8 | Radio CSN | Uno output → radio input |
| Both nodes | D11 | Radio MOSI | Uno SPI output → radio input |
| Both nodes | D12 | Radio MISO | Radio output → Uno SPI input |
| Both nodes | D13 | Radio SCK | Uno SPI output → radio input |
| Both nodes | — | Radio IRQ | Unconnected; firmware polls |
| Receiver | D2 | Servo signal | Servo control pulses |
| Receiver | GND | Servo GND + external supply negative | Common return, route motor current directly to supply |
| Receiver | — | External supply positive → servo V+ | Regulated 4.8 V; do not join to Uno 5V/VIN |

Power off before wiring. Verify polarity, module labels and supply voltage before
connecting USB. The two wireless nodes do not require a ground wire between
them; each node needs its own local common ground. Avoid routing servo return
current through the radio or breadboard signal-ground wiring.

## Electrical rationale and constraints

The [Spectra Symbol stock sensor datasheet](https://cdn.sparkfun.com/datasheets/Sensors/ForceFlex/FLEX%20SENSOR%20DATA%20SHEET%202014.pdf)
identifies a 10 kΩ nominal flat sensor (7–13 kΩ) whose resistance rises when bent.
It is a passive resistance, not a digital sensor. This reference divider is:
`5V → flex → A0 → 10kΩ → GND`. With the default Uno ADC reference at its supply,
`ADC ≈ 1023 × 10000 / (Rflex + 10000)`. At nominal flat resistance it reads about
512, and at 20 kΩ about 341. These are calculations, not measured results.
At any positive flex resistance, the divider's Thevenin resistance is below
10 kΩ. This matches the [ATmega328P ADC source-impedance guidance](https://ww1.microchip.com/downloads/aemDocuments/documents/MCU08/ProductDocuments/DataSheets/ATmega48A-PA-88A-PA-168A-PA-328-P-DS-DS40002061B.pdf).
Firmware also discards the first conversion after selecting each channel.
Use a buffer if adapting a substantially higher-impedance circuit. A 2.2-inch
sensor can have a different nominal resistance: check its own datasheet and
reselect the fixed resistor rather than assuming all flex sensors are 10 kΩ.

The [Uno R3 pinout and manual](https://docs.arduino.cc/hardware/uno-rev3/)
identify the analog and SPI pins. The Uno 3.3 V output is rated at 50 mA;
this reference uses one standard non-amplified radio per Uno. PA/LNA modules
require a separately sized supply and are outside this BOM.

The [Nordic nRF24L01+ product specification v1.0](https://docs.nordicsemi.com/r/bundle/pdf_ps_nrf24l01p/)
rates VDD at 1.9–3.6 V and describes 5 V tolerant digital inputs, not a 5 V
supply. When input signals exceed 3.6 V, its specified VDD condition is
2.7–3.3 V. Direct Uno signals in this reference therefore require that the radio
supply stays within that condition. Verify the rail; use appropriate level
translation if it does not, or if the actual module/clone has different limits.
The 3.3 V MISO signal is sufficient for the Uno's normal digital input high
threshold at a 5 V supply. Place both bypass capacitors next to each radio and
keep SPI leads short. Check module-specific documentation before substituting
unidentified clones or regulated adapter boards.

[TowerPro's SG90 digital page](https://towerpro.com.tw/product/sg90-7/)
specifies 4.8 V operation and an external adapter; its manufacturer response
indicates current can reach 2 A. The supply budget above follows that reference,
but actual stall current and travel need validation on the actual servo.
The library's default pulse range (544–2400 µs) may exceed a particular servo's
mechanical range even when software angles are limited to 10–170. Adjust the
`Servo::attach` pulse limits for that unit before coupling a mechanism.

## Bench validation still required

The firmware was compiled and its pure core tested on a host. No assembled
hardware was available. Before using the wireless demonstration:

1. Run the serial-only reader; verify rest/bent ADC values and recalibration.
2. Confirm radio VCC and signal levels under load; check for brownouts.
3. Establish ACKs with the servo supply disconnected, then power the unloaded servo.
4. Verify travel and polarity; confirm pulses stop when the transmitter is removed
   or a calibration packet is invalid. Detach does not guarantee removal of torque.
5. Evaluate RF reliability, latency, power supply behavior and mechanical safety
   for the intended setup. This is a hobby reference, not a validated control system.
