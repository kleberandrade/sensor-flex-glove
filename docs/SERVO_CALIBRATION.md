# Serial servo calibration and EEPROM persistence

This calibrates the **receiver's pulse endpoints**, independently for each servo.
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

`show` reports comment-prefixed `channel,pin,min_us,max_us` rows. Channel/pin
mapping stays fixed by the firmware: 0→D2, 1→D3, 2→D4, 3→D5 and 4→D6. The suggested
fingers are index, middle, ring, little and thumb. Serial/EEPROM limits cannot
change pins or alter other channels when one channel is edited.

## Commands

Commands are lowercase, with decimal integer arguments separated by spaces/tabs.

| Command | Effect |
| --- | --- |
| `help` | Print command syntax; allowed in either mode |
| `show` | Print current RAM endpoints for all five channels |
| `cal` | Enter calibration, detach all outputs, clear radio command state |
| `set CH MIN_US MAX_US` | Change one channel's endpoints in RAM; stop manual output |
| `test CH US` | Drive only that servo at an exact pulse for at most a 1000 ms software interval |
| `stop` | Detach all outputs and stay in calibration; also enters calibration from RF mode |
| `save` | Stop manual output and save the entire five-channel profile explicitly |
| `load` | Stop manual output and reload EEPROM, discarding RAM edits; invalid EEPROM selects defaults |
| `defaults` | Stop manual output and restore firmware defaults in RAM; EEPROM is unchanged |
| `run` | Stop manual output, leave calibration and wait for fresh RF commands; does not save |

`set`, `test`, `save`, `load` and `defaults` require calibration mode. A rejected
command does not change endpoints or refresh a manual-test deadline. Changing
the preview channel detaches the previous one before activating the new one.
After a preview ends, radio control remains paused until `run`.

### Input limits and feedback

- Channel is 0–4. Every channel requires `544 ≤ MIN_US < MAX_US ≤ 2400`.
- `test` pulses must also lie within that channel's current RAM endpoints.
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

In RF mode, commands 10–170 map to each channel's min/max pulse endpoints; 90
maps to the midpoint. Verify the transmitter mapping and all five validity bits
again after changing a servo's limits.

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
saved profile. Limits follow **channel numbers**, so changing pin assignment,
servos or mechanisms requires recalibration; inspect the profile before operation.
Firmware upload/erase tools can have different EEPROM retention behavior: the
persistence guarantee here concerns normal receiver resets and power cycles,
not every programmer's erase procedure.

## Storage format and recovery

The receiver reserves **EEPROM addresses 0–61**; do not share them with another
feature. Other EEPROM addresses are untouched. The bundled
[Arduino AVR EEPROM library](https://github.com/arduino/ArduinoCore-avr/blob/1.8.6/libraries/EEPROM/src/EEPROM.h)
provides `length`, `read` and `update`. No external EEPROM component is required
on the supported Uno R3.

There are two 31-byte slots at offsets 0 and 31. Multibyte values are explicitly
little-endian; no compiler struct layout is written:

| Byte offset in slot | Meaning |
| --- | --- |
| 0 | Commit marker `0xA5`; invalidated before replacing this slot |
| 1–2 | Magic bytes `F`, `S` |
| 3 | Storage schema version 1 (independent of RF protocol version) |
| 4 | Channel count 5 |
| 5–8 | Unsigned generation counter; wrap-aware newest-record selection |
| 9–28 | Five pairs of unsigned 16-bit min/max microseconds, channel order 0–4 |
| 29–30 | CRC-16/CCITT-FALSE over bytes 1–28; polynomial `0x1021`, initial `0xFFFF` |

A changed save writes the older/unused slot: invalidate its marker, update the
payload/CRC, verify the bytes, then commit the marker last. The previous slot
remains intact. Loading checks the marker, magic, version, count, CRC and every
pulse range, then selects the newest valid complete record. Corruption of the
newer record can fall back to the older one. If neither is valid, firmware
uses defaults. Unknown schema versions are rejected rather than guessed.

An identical profile skips **all** EEPROM writes, including generation/marker
changes. Other saves use `EEPROM.update` so unchanged bytes are not programmed.
Wear is spread between two slots, not across the whole EEPROM. Save deliberately;
do not use it as a repeated heartbeat or continuous logging operation.

Host tests simulate interruption before changed-byte writes, corruption, invalid
ranges, counter wrap, capacity/write failures and reload. The journal and CRC
reduce recovery risks, but do not establish physical brownout immunity or detect
every possible corruption. Stable power and hardware-level bench checks remain
necessary; no real EEPROM endurance or power-interruption test has been performed.
