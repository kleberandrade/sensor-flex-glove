# Build a flex-sensor glove, step by step

This guide builds the reference design in stages: first a single sensor and USB
serial output, then a wearable mounting, then wireless control of one unloaded
servo. Each stage has a check before adding more parts. All assembly procedures
below still need validation on your actual hardware; the repository's automated
checks exercise software and compilation.

![Glove placement and signal flow](images/glove-overview.svg)

## 1. Choose your first milestone

Start with **one finger, one flex sensor, one Uno and the `reader` environment**.
That is already a useful bend-to-command demonstration. Add the second Uno and
the radios only after stable readings are visible. Five sensor channels are
supported by the transmitter, but the provided receiver controls one servo.

Have a computer with USB, a breadboard, a multimeter, short jumpers, a glove,
soft fabric loops or removable tape, and the [reference BOM](hardware/README.md#bill-of-materials).
Choose a glove that allows free movement and can carry lightweight sensor leads.
Keep the Uno and breadboard on the bench initially; the illustration's wrist
block is a lead-routing point, not a compact custom PCB supplied by this project.

Read the exact part markings before connecting power. This guide uses a passive
resistive strip, not an unspecified Grove module. If your module includes an
amplifier, a four-pin connector or a digital interface, identify it and adapt
the circuit from its own primary documentation first.

## 2. Prepare the software

Use Python 3.12 or 3.13 and a C++11 compiler with Make. From the repository root:

```sh
python3 -m venv .venv
. .venv/bin/activate
python -m pip install -r requirements-dev.txt
make test example
pio run
pio device list
```

On Windows activate `.venv\Scripts\activate`; use a Make-capable shell for the
host example. The expected host result includes `Core behavior checks passed`
and a CSV beginning with `raw,filtered,angle`. The firmware build should report
success for `transmitter`, `receiver` and `reader`. No board is needed to compile.

The example's readings are deliberately artificial. They validate calculations,
not the electrical characteristics or accuracy of a physical sensor.

## 3. Wire and check a single divider

Keep power disconnected while assembling. Follow the
[pinout and circuit](hardware/README.md#circuit-and-pinout):

1. Connect one terminal of the reference flex sensor to Uno 5V.
2. Connect its other terminal to A0 and to one end of a 10 kΩ fixed resistor.
3. Connect the resistor's other end to Uno GND.
4. Check for unintended shorts and ensure neither sensor terminal touches a
   neighboring connection. Use strain relief near the sensor's terminals.
5. Power the Uno by USB. Use the multimeter to confirm the supply and divider
   output are within the Uno ADC's 0–supply-voltage input range.

There is no extra 5 V connection to A0: it receives only the divider junction.
Leave A1–A4 inactive in configuration until their own dividers are connected.
The reference calculations are about 512 counts for a flat nominal 10 kΩ sensor
and 341 counts for 20 kΩ. Tolerances and mounting will change actual readings.

## 4. Upload the standalone reader

Substitute a real port from `pio device list`:

```sh
pio run -e reader -t upload --upload-port /dev/cu.usbmodemREADER
pio device monitor -p /dev/cu.usbmodemREADER -b 115200
```

On Linux ports often resemble `/dev/ttyACM0`; on Windows use `COM3` or the port
reported for your device. These names are examples. Close other serial programs
before opening the port or uploading. On Linux ensure your user can access the
serial device according to your distribution's permissions.

Opening the monitor can reset the Uno. Immediately move the sensor slowly
through its full intended range for five seconds, then hold still. Send lowercase
`c` to repeat calibration. Do not crease the strip or bend its terminals.

During calibration `valid` is 0. It becomes 1 only after the five-second session
has ended and the observed filtered range spans at least 20 ADC counts. If it
stays 0, check the raw readings, wiring, sensor motion and fixed resistor choice
before changing the threshold. Calibration is per channel and is lost on reset.

## 5. Understand and capture the readings

The firmware emits `ms,channel,raw,filtered,angle,valid,ack`. Status/header lines
start with `#`. For example, this **constructed reader-mode row** uses the same
last three values as the executable host example:

```csv
6000,0,700,675,160,1,-1
```

It means 6,000 ms after boot, channel 0/A0, raw ADC 700, filtered ADC 675,
command 160, calibration valid, and no radio in use. It is not a captured physical
measurement. The host example deliberately calibrates to 300–700; your glove's
endpoints will depend on the actual assembly.

- `raw` is the unscaled 10-bit ADC sample; no voltage or resistance measurement
  is transmitted directly.
- `filtered` is the integer mean of up to eight actual samples; a steady value
  should settle after the window fills. It adds roughly 70 ms of group delay at
  a nominal 50 Hz sample rate.
- `angle` is a bounded servo command. It is meaningful only if `valid=1` and
  does not represent an absolute finger-joint angle.
- `ack` is `-1` in reader mode, `0` for failed radio acknowledgement and `1` for
  a successful radio acknowledgement. An ACK is not confirmation of motion.

To save a session in a POSIX shell, first create an output directory outside the
tracked source, then use the monitor's raw/quiet mode:

```sh
mkdir -p build
pio device monitor -p /dev/cu.usbmodemREADER -b 115200 --raw --quiet | tee build/session.log
```

Use Ctrl+C to stop. Remove `#` lines before importing the seven numeric columns
into a spreadsheet or your own analysis program. Retain the calibration context,
sensor part, finger placement, session date and sampling settings alongside the
file. A serial capture/plotting application is optional; none is bundled here.

## 6. Mount the sensor on the glove

The drawing uses the index finger as channel 0 for illustration. The firmware
knows only channel numbers; you can choose another finger without changing its
single-channel behavior.

1. With electronics unpowered and the glove off the hand, place the strip along
   the back of the chosen finger following the manufacturer's permitted bend
   direction. Keep its terminal region away from the knuckle's sharp bend.
2. Use loose fabric loops or removable tape to guide the strip while allowing it
   to slide slightly as the finger bends. Do not immobilize the entire strip or
   glue directly over an active bend unless the manufacturer permits it.
3. Anchor the wires separately at the back of the hand/wrist so pulling a lead
   does not pull the sensor contacts. Avoid sharp edges and exposed conductors.
4. Wear the glove with power disconnected and check comfortable motion. Remove
   it to adjust mounting; then reconnect the bench reader and recalibrate.
5. Compare repeated open/closed poses. Record variation and drift rather than
   assuming a universal angle or resistance relationship.

Relocating the strip changes the measured range. Recalibrate whenever the glove,
mounting, sensor, divider resistor or intended travel changes. This firmware
calibrates electrical extrema, not a person's biomechanics.

## 7. Add the radio link

Use two standard, non-amplified nRF24L01+ modules. Wire each node to its own Uno
as documented in the [hardware guide](hardware/README.md); the 250 kbps setting
requires the plus variant. Add 100 nF and 10 µF capacitors locally across each
radio's supply and ground. Verify radio supply/logic conditions from that guide.
Do not connect the radio's VCC to Uno 5V.

Initially leave the servo and its external supply disconnected. Upload the
`receiver` environment to the receiver Uno and `transmitter` to the sensor Uno.
Both must use the same protocol, address, channel and data rate:

```sh
pio run -e receiver -t upload --upload-port /dev/cu.usbmodemRECEIVER
pio run -e transmitter -t upload --upload-port /dev/cu.usbmodemTRANSMITTER
```

Open separate monitors for the two real ports. Calibrate the transmitter. The
transmitter should report successful ACKs and the receiver should print valid
finger-0 commands. A `# ERROR` line indicates radio initialization failed; the
firmware then stops normal operation until reset. An unsupported 250 kbps mode
is also treated as an initialization error.

## 8. Add one unloaded servo

1. Disconnect power and leave the horn/linkage mechanically unloaded.
2. Connect receiver D2 to servo signal, and connect servo ground to both the
   external supply negative and receiver ground.
3. Connect servo V+ only to its correctly rated external supply. For the reference
   SG90 this guide budgets a regulated 4.8 V supply capable of 2 A. Check the
   actual unit's requirements. Do not use the Uno's rail for servo power.
4. Check polarity, supply voltage and pulse/travel limits before powering.
5. Run the pair and observe the response without a person or mechanism in its
   travel path. Default commands span 10–170; restrict the pulse limits in
   `servo.attach(...)` if the actual servo needs a narrower range.
6. Remove transmitter power and verify receiver timeout handling. The firmware
   detaches after 500 ms without a valid finger-0 command. Recalibrating also
   sends invalid commands and detaches the servo while the session runs.

Detaching stops pulse generation but neither disconnects servo power nor proves
that a mechanical load is safe. Validate the complete setup before adding load.
Do not infer radio range, reaction time or mechanical accuracy from host tests.

## 9. Expand to multiple fingers

Each additional channel needs a separate flex sensor and divider. An example
assignment is index=A0, middle=A1, ring=A2, little=A3, thumb=A4. This is a suggested
mapping; document your actual assignment.

Set `kActiveFingers` in `include/flex/hardware.hpp` to the number of connected
**consecutive** inputs, from 1 to 5. For example, 3 enables A0, A1 and A2; the
configuration cannot select only A0 and A4. Rebuild and upload the transmitter
or reader, then move every enabled finger during calibration. A bad channel's
validity bit stays clear independently of the others.

The packet already contains five command slots, but the receiver currently uses
only bit 0 and angle 0. Multi-servo actuation requires a deliberate receiver
extension, per-servo travel calibration, supply sizing and new behavior tests.
It is not enabled merely by changing `kActiveFingers`.

## 10. Troubleshoot by stage

| Symptom | Check | Next action |
| --- | --- | --- |
| No serial port | USB data cable, drivers, device list | Resolve USB before investigating sensors |
| Upload fails | Correct environment/port, monitor closed | Retry with the correct Uno selected |
| Raw value stays near a rail | Divider junction, open/short wiring, correct part | Measure voltage; software cannot reliably detect an unplugged sensor |
| No change when bending | Sensor direction, placement, resistor value | Return to a single loose sensor on the bench |
| `valid=0` after calibration | Filtered travel below 20 counts, incomplete motion | Correct wiring/mounting and send `c` |
| Excessive noise | Ground, jumper length, supply stability, moving contacts | Fix the electrical cause before increasing filtering |
| ACKs fail | Matching firmware settings, radio supply, SPI pin labels, plus variant | Test radios without the servo load |
| Servo jitters or radio resets | External supply capacity and ground routing | Remove load, inspect rails and return path |
| Motion is opposite to desired | Reference divider lowers ADC as bend increases | Adapt the command mapping and its tests for your mechanism |
| Motion hits a stop | Servo-specific pulse limits and linkage travel | Disconnect the linkage and reduce pulse/travel limits |

## Next steps

See [applications and extension paths](APPLICATIONS.md), the
[project layout and contribution guide](CONTRIBUTING.md), and the
[protocol/migration reference](../README.md#layout-and-migration).
