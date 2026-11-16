# Serial servo calibration and EEPROM persistence

This calibrates the **receiver's pulse endpoints, direction and command slew rate**, independently
for each servo.
It is separate from the transmitter's five-second ADC calibration (`c`), which
still lives only in RAM. No wiring or RF packet-format change is required.

## Connect and enter calibration

Use the receiver Uno's serial port at **115200 baud**. Keep the transmitter off
initially, leave horns/linkages unloaded, and follow the
[five-servo power guide](hardware/README.md#five-servo-receiver-and-power). A working
receiver radio module is still required to initialize the firmware normally.

```sh
pio device list
pio device monitor -p /dev/cu.usbmodemRECEIVER -b 115200 --eol LF
```

Substitute the real port (`COM3`, `/dev/ttyACM0`, etc.). In another serial monitor,
select newline, carriage return or CRLF line endings; commands execute when a line
ends. Opening the monitor may reset the Uno and reload EEPROM. **Do not assume
opening the monitor pauses radio control.** Send `cal` and wait for its response
before editing limits or enabling servo power for a calibration session.

```text
help
cal
show
```

`cal` detaches every output, clears pending receiver commands and pauses radio
actuation. The radio FIFO is drained but its commands are ignored while this mode
is active. RF ACKs can still occur and do not indicate servo movement.

`show` reports comment-prefixed `channel,pin,min_us,max_us,reversed,speed_us_per_s` rows. Channel/pin
mapping stays fixed by the firmware: 0→D2, 1→D3, 2→D4, 3→D5 and 4→D6. The suggested
fingers are index, middle, ring, little and thumb. The `reversed` field is `0` for
normal direction and `1` for reversed; `speed_us_per_s` is the commanded pulse
rate limit (0 disables it). Serial/EEPROM settings cannot change pins or alter other channels when one channel is edited.

## Commands

Commands are lowercase, with decimal integer arguments separated by spaces/tabs.

| Command | Effect |
| --- | --- |
| `help` | Print command syntax; allowed in either mode |
| `show` | Print current RAM endpoints, direction and speed for all five channels |
| `cal` | Enter calibration, detach all outputs, clear radio command state |
| `set CH MIN_US MAX_US` | Change one channel's endpoints in RAM; stop manual output |
| `invert CH 0\|1` | Set one channel to normal (`0`) or reversed (`1`) in RAM; stop manual output |
| `speed CH US_PER_S` | Set one channel's RF pulse slew limit, 0–10000 µs/s; stop manual output |
| `test CH US` | Drive only that servo at an exact pulse for at most a 1000 ms software interval |
| `stop` | Detach all outputs and stay in calibration; also enters calibration from RF mode |
| `save` | Stop manual output and save the entire five-channel profile explicitly |
| `load` | Stop manual output and reload EEPROM, discarding RAM edits; invalid EEPROM selects defaults |
| `defaults` | Stop manual output and restore firmware defaults in RAM; EEPROM is unchanged |
| `run` | Stop manual output, leave calibration and wait for fresh RF commands; does not save |

`set`, `invert`, `speed`, `test`, `save`, `load` and `defaults` require calibration mode. A rejected
command does not change endpoints or refresh a manual-test deadline. Changing
the preview channel detaches the previous one before activating the new one.
After a preview ends, radio control remains paused until `run`.

### Input limits and feedback

- Channel is 0–4. Every channel requires `544 ≤ MIN_US < MAX_US ≤ 2400`.
- `test` pulses must also lie within that channel's current RAM endpoints. They
  are literal microseconds and are **not** transformed by direction inversion or slew limiting.
- Values are microseconds, not servo degrees. These software bounds do not
  establish safe mechanical travel for a particular servo.
- Commands longer than 63 bytes, extra/missing arguments, signed/fractional/
  overflowing numbers, unknown words and invalid control bytes are rejected.
  An overlong line is discarded through its ending; the next line works normally.
- Firmware reads at most 16 serial bytes per loop and uses a fixed buffer, without
  blocking line-read timeouts or Arduino `String` allocation.
- Replies and settings start with `#`; successful operations use `# OK`, errors
  use `# ERR`. Empty/invalid EEPROM reports that defaults are being used.
  A failed save reports `# ERR EEPROM save failed`; do not assume it persisted.

The preview is a software timeout checked by the loop; serial/EEPROM processing
and loop scheduling add latency. It is not an independent emergency power cutoff.
Stopping pulses neither disconnects servo power nor guarantees torque release.

## Calibrate one channel, then repeat

Start with a narrow range justified by the actual servo specifications and an
unloaded assembly. The numbers below illustrate the interface; they are **not
measured or universally safe endpoints** for your hardware.

```text
cal
show
set 0 1400 1600
test 0 1500
stop
```

Observe the unloaded servo and let each test stop. Adjust the endpoints gradually
only after verifying actual travel. Test the low and high values separately:

```text
test 0 1400
test 0 1600
stop
```

Use `set 1 ...`, `set 2 ...`, `set 3 ...` and `set 4 ...` for the other servos.
Do not paste literal ellipses; supply actual verified integer endpoints. This
workflow never automatically sweeps an unknown mechanical range.

When all desired settings have been checked:

```text
show
save
run
```

Wait for `# OK saved ...` before removing receiver power. `save` detaches all
servos before programming. The settings take effect in RAM immediately, so `run`
uses unsaved edits too; without `save`, reset/power loss restores the last valid
saved profile. `run` clears cached RF state/FIFO so an old queued command cannot
resume motion; newly arriving valid packets are required. Normal per-channel
500 ms RF loss handling remains in effect outside calibration.

In normal RF direction, command 10 maps to min and 170 to max. For a reversed
channel, command 10 maps to max and 170 to min; command 90 stays at the same
integer midpoint. The logical command is clamped before reversing. Incoming
packet angles and receiver angle CSV are unchanged; inversion affects only the
final servo pulse. `test CH US` always applies raw microseconds within the limits.
Verify the transmitter mapping and all five validity bits
again after changing a servo's limits.

## Reverse an individual servo

Keep the mechanism unloaded and enter calibration before changing orientation:

```text
cal
invert 2 1
show
save
run
```

This reverses channel 2 / D4 (suggested ring finger). Use `invert 2 0` to restore
normal direction, followed by `save` to persist it. The explicit 0/1 setting is
idempotent; repeating `invert 2 1` does not toggle the direction. Values such as
`2`, `-1`, `true`, or a channel outside 0–4 are rejected.

Only that channel changes. Its min/max endpoints and pin are preserved; changing
limits with `set` also preserves its current direction. Successful direction
changes stop any manual test and detach outputs, and RF remains paused until
`run`. A direction change is a RAM edit until `save`; `load` restores saved limits, directions
**and speeds**, while `defaults` restores firmware defaults (all normal in
the shipped configuration). Do not swap MIN_US/MAX_US to reverse travel: the
limits still require min < max.

Raw `test` commands are intentionally unaffected, so endpoints can be calibrated
in real microseconds. To verify reversed finger motion, leave the linkage
unloaded, finish calibration, send `run`, and use fresh transmitter commands.
Keep clear of travel: the same input can target the opposite endpoint after
inversion. No automatic sweep or physical direction verification is performed.

## Smooth motion with an individual speed setting

The receiver can limit the rate of change of each **commanded pulse** in normal
RF mode. It does not measure shaft position, physical degrees/second, motor RPM
or load. It is a linear slew limiter, not an acceleration/easing controller.

```text
cal
speed 2 500
show
save
run
```

This limits channel 2 / D4 to 500 **microseconds of pulse change per second**.
Valid settings are integers 0–10000. `speed 2 0` restores immediate target updates
(the shipped and legacy-profile default). A lower positive value gives a slower
command ramp. For example, changing a known commanded pulse from 1000 to 2000 µs
at 500 µs/s nominally takes 2 seconds with continuous valid RF data and regular
loop updates. That is not a measured travel time or a physical speed guarantee.

The setting affects only the selected channel, and endpoint/direction edits
preserve it. Successful `speed` changes stop manual output; explicit `save`
persists the complete profile, `load` restores it, and `defaults` restores the
firmware profile (all rates 0 in this repository). The `show` rate is configuration,
not measured speed or the current intermediate pulse. Incoming angle CSV remains
the target command and therefore does not describe the ramp's intermediate output.

### Ramp start, interruption and interaction with calibration

- Inversion and endpoint mapping select the target pulse **before** slew limiting.
  Targets can change or reverse while moving; the ramp follows them without
  overshooting the commanded target. Fractional progress is retained so even
  1 µs/s settings work with frequent loop updates.
- After a cold boot, a rate-limited channel has no known physical position. Its
  first emitted command is the configured midpoint, then it ramps toward the
  target. This initial midpoint can itself move the mechanism abruptly: the
  feature does **not** guarantee a physical soft start. Rate 0 instead preserves
  the old immediate target behavior.
- After a detach, the receiver remembers only its last **emitted command**, not
  the actual position. Resume starts from that remembered pulse, clamped to the
  current endpoints, without credit for paused time. Endpoint edits can thus
  cause a clamp on reattachment. Manually moving or back-driving an unpowered
  servo can make the actual position differ from this remembered command.
- Raw `test CH US` previews bypass the ramp and remain exact, short calibration
  tests. Their emitted pulse becomes the remembered starting command when RF
  resumes. Never use those raw tests expecting the configured speed to protect
  a linkage.
- Invalid mask bits, `cal`/`stop`, manual preview expiry and the 500 ms RF timeout
  detach immediately in the control loop; none waits for a ramp to finish. A
  slow ramp needs continued valid RF packets, not one packet followed by silence.
- Each update accounts for at most 100 ms of elapsed time to avoid a catch-up
  burst after a long loop stall. Slow loops can therefore take longer than the
  nominal duration. The limiter uses unsigned elapsed time across `millis()` wrap,
  integer fractional accumulation and no blocking motion delays.

Verify mapping, endpoints, first attachment, timeout and recovery unloaded before
coupling a mechanism. Current, torque and actual speed still depend on the servo,
supply and mechanics; no physical motion measurements were performed here.

## Reload, reset and migration

On startup the receiver attempts to load a complete valid profile. If no valid
record exists, it uses the compile-time `hardware::kServos` defaults (normally
1000–2000 µs). Startup/loading never automatically writes those defaults to EEPROM.

To discard unsaved changes, use `cal` then `load`. If both saved records are
invalid, `load` selects firmware defaults and reports the fallback. To intentionally
replace the stored profile with defaults:

```text
cal
defaults
show
save
run
```

`defaults` alone changes RAM only; it does not erase the saved profile. Pins
remain firmware-owned. Changing source-code defaults will not override an intact
saved profile. Limits, direction and speed follow **channel numbers**, so changing pin assignment,
servos or mechanisms requires recalibration; inspect the profile before operation.
Firmware upload/erase tools can have different EEPROM retention behavior: the
persistence guarantee here concerns normal receiver resets and power cycles,
not every programmer's erase procedure.

## Storage format and recovery

The supported receiver now reserves **EEPROM addresses 0–145**. Addresses 0–61
hold the legacy v1/v2 bank and are read-only to this firmware; 62–63 are an
untouched gap. Current **v3** records occupy two disjoint 41-byte slots at
addresses **64 and 105** (ending at 145). No other EEPROM addresses are written.
Do not share this reservation with another feature. The Uno's built-in
[Arduino AVR EEPROM library](https://github.com/arduino/ArduinoCore-avr/blob/1.8.6/libraries/EEPROM/src/EEPROM.h)
provides `length`, `read` and `update`; no external EEPROM is required.

All multibyte fields are explicitly little-endian, not compiler struct layouts.
Each current slot has:

| Byte offset in v3 slot | Meaning |
| --- | --- |
| 0 | Commit marker `0xA5`; invalidated before replacing this slot |
| 1–2 | Magic bytes `F`, `S` |
| 3 | Storage schema version 3, independent of the RF protocol |
| 4 | Five-bit reversal mask, bits 0–4; bits 5–7 must be zero |
| 5–8 | Unsigned generation counter with wrap-aware selection |
| 9–28 | Five unsigned 16-bit min/max pulse pairs, channel order 0–4 |
| 29–38 | Five unsigned 16-bit speed values in µs/s, each 0–10000 |
| 39–40 | CRC-16/CCITT-FALSE over bytes 1–38, polynomial `0x1021`, initial `0xFFFF` |

A save invalidates the older/unused current slot, updates and verifies its bytes,
then commits its marker last. The previous complete current record remains
intact. Both current slots must fit to save (at least 146 EEPROM bytes). A valid
current record takes priority over the legacy bank; within a bank, loading uses
the newest valid generation. CRC, header/version, reversal bits, pulse endpoints
and all speed limits are validated as one complete profile.

### Legacy migration and recovery

Legacy records remain 31 bytes at offsets 0 and 31. Their CRC is at offsets
29–30 over bytes 1–28. Schema v1 byte 4 is channel count 5, with all directions
normal; schema v2 byte 4 is the reversal mask. Both retain their original pulse
endpoints and load with **speed 0**, preserving immediate-output behavior.

Loading never writes or upgrades EEPROM. The next explicit `save` writes v3 to
the separate current bank, even if settings were unchanged. The legacy bank is
not overwritten, so interrupted migration can fall back to a complete old
profile instead of damaging overlapping records. Tests exercise both legacy
versions and byte-boundary interruptions. Unknown schemas and invalid settings
are rejected rather than guessed.

If one current record is corrupt, the other valid current record can be used.
If neither current record is valid, the loader tries the legacy bank; if no
record is valid, it uses firmware defaults. Recovery restores a **whole earlier
profile**, potentially including earlier direction or an immediate speed of 0.
Inspect `show` before operating after recovery. The legacy bank becomes a frozen
pre-migration fallback; it is not kept in sync with later v3 saves.

Older firmware cannot read the new bank or speeds. Downgrading can reactivate
older legacy limits/directions or defaults, not the latest current profile. Do
not rely on safe profile reuse on downgrade; record and reverify settings with
compatible firmware. These migrations do not change RF protocol v1 or pin order.

An identical **current-schema** profile skips all EEPROM writes, including
marker/generation changes. Limits-, direction- or speed-only edits count as
profile changes. Other saves use `EEPROM.update` for changed bytes only. Wear is
spread between two current slots, not the whole EEPROM; save deliberately rather
than as a heartbeat.

Host tests simulate interrupted writes, legacy migration, corruption, invalid
fields, counter wrap, small/unwritable storage and reload. Journaling/CRC do not
establish physical brownout immunity or detect every possible corruption. No
real EEPROM endurance, power-interruption or physical servo-speed test has been
performed; stable power and bench validation remain necessary.
