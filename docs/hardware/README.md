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

## Breadboard assembly

![Numbered breadboard and pin-by-pin radio/servo wiring](../images/breadboard-wiring.svg)

[Open the full-size SVG](../images/breadboard-wiring.svg) to zoom into the wires
and hole labels. The top panel is a reproducible **one-sensor reader/transmitter
divider**. The lower panels add the radios and receiver servo. This is an original
reference drawing; no physical breadboard test is claimed.

### Hole-by-hole divider connections

Use a conventional breadboard with separate a–e and f–j strips per numbered row.
Only the a–e side is used. The positions below are examples on the drawing: match
the actual board's markings and verify continuity with power disconnected.

| Part / jumper | First endpoint | Second endpoint | Purpose |
| --- | --- | --- | --- |
| Red supply jumper | Uno 5V | Positive rail, beside row 1 | Supply for the passive divider only |
| Black return jumper | Uno GND | Negative rail, beside row 2 | Common local ground |
| Red row jumper | Positive rail, beside row 5 | a5 | Connect the row-5 strip to 5 V |
| Flex sensor, using flying leads | e5 | e10 | Variable resistance between supply and signal |
| 10 kΩ fixed resistor | c10 | c15 | Bottom leg of the divider; two different strips |
| Green signal jumper | Uno A0 | a10 | Read the row-10 signal junction |
| Black row jumper | a15 | Negative rail, beside row 15 | Return the fixed resistor to ground |

**Why this works:** a10, c10 and e10 share the same metal strip, forming the A0
junction. Row 5 is the sensor's supply, row 10 its output, and row 15 the fixed
resistor's ground return. Do not put both resistor leads into one strip: that
would short the resistor. Holes f10–j10 are separate from a10–e10 across the gap.
The highlighted strips illustrate existing internal connections, not extra wires.

The diagram shows continuous rails only for its illustrated section. Some boards
split rails midway or leave opposite-side rails isolated. Check that the supply
and row jumpers use a continuous section; bridge only confirmed breaks with
same-net jumpers. Do not assume rail color means the board is already powered.
See the breadboard manufacturer's [strip and rail explanation](https://learn.sparkfun.com/tutorials/how-to-use-a-breadboard/).

### Add radios by pin name

The lower-left panel applies independently to the transmitter and receiver.
Match the radio's printed **VCC, GND, CE, CSN, MOSI, MISO and SCK** labels to the
Uno pins shown in the panel and the pinout table above. The terminal arrangement
is a named breakout, **not a physical 2×4 pin-order diagram**; header orientation
varies and the original module is unidentified. Use suitable female jumpers or
a labeled breakout. Inserting a bare 2×4 header into ordinary connected five-hole
strips can join pins that must remain separate.

Radio VCC connects directly to **Uno 3.3V**, never to this breadboard's 5 V rail.
C1 is 100 nF ceramic and C2 is 10 µF, both across radio VCC/GND and placed next to
the module with short leads. If C2 is polarized, its positive lead goes to VCC
and its negative lead to GND. Verify its polarity markings rather than relying
on lead length. Leave IRQ disconnected. The electrical limits below still apply.

### Add the receiver servo and check before powering

The lower-right panel identifies D2, servo signal/V+/GND and the external supply
terminals. Route servo power and return directly to its rated external supply,
with a separate common-ground connection to the receiver. **Do not route servo
motor current through the breadboard or Uno supply rail.** Do not connect the
external supply positive to Uno 5V/VIN. The transmitter and receiver are wireless
nodes and do not need a ground wire between them.

Before powering, check row connections and rail continuity, confirm that 5 V and
GND are not shorted, and verify the actual radio pin labels and capacitor polarity.
Test the single-sensor reader first, then the radios without the servo load,
then an unloaded servo as described in the [build guide](../BUILD_GUIDE.md).

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
