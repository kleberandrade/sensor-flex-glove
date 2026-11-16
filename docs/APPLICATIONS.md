# Applications and extension paths

This project provides calibrated flex readings, CSV telemetry and a wireless
one-servo demonstration. The ideas below distinguish what you can run now from
additional systems you would need to build. None of these applications has been
physically validated by the modernization work.

## Teach embedded sensing and signal processing

**Available now:** wire one divider, run `reader`, and compare `raw` with
`filtered`. Use `make example` to see deterministic smoothing without hardware.

A workshop can ask learners to hold a pose, move through calibration, compare
noise before/after filtering, and investigate the trade-off between an eight-
sample window and response delay. Record the actual sample interval and observed
ADC range; the intended 50 Hz schedule is not a guaranteed end-to-end latency.
This needs no radio, servo or five-finger assembly.

## Demonstrate remote actuation

**Available now:** one calibrated finger channel controls one receiver servo via
nRF24L01+. Use an unloaded pointer or a low-force tabletop demonstrator after
checking the actual servo's travel and supply.

**Extension:** a multi-finger robotic-hand prototype would need more servos,
mechanical design, individual pulse/travel limits, an appropriately sized power
system and receiver code/tests for all channels. The packet's five slots alone
do not implement a five-servo hand. Evaluate communication loss and mechanical
behavior on the complete assembly before operating it under load.

## Capture datasets for gesture experiments

**Available now:** the reader emits up to five consecutive ADC/command channels;
follow the build guide's CSV capture instructions. Label recordings with the
actual finger mapping, pose, sensor position and calibration session.

**Extension:** add synchronized labels, a dataset format, feature extraction and
a classifier on a host computer. Evaluate on separate recording sessions to
expose mounting drift and sensor hysteresis. Gesture recognition, training code
and model inference are not included in this repository.

Flex sensors capture bending; they do not measure hand translation, orientation,
finger contact force or all finger-joint angles. Add suitable independently
validated sensors if the experiment needs those quantities.

## Prototype game, music or visual controls

**Available now:** serial CSV can be consumed by a program you write to map bend
values to an animation, an audio parameter or a game action.

**Extension:** build a serial bridge and define thresholds, dead zones, smoothing
and application behavior when `valid=0`. Standard Uno R3 firmware in this project
does not enumerate as a USB HID glove, MIDI controller, Bluetooth device or VR
tracker. Those interfaces need separate host software, different firmware or
suitable hardware. The radio protocol is also not a standard game-controller API.

## Explore wearable interaction and accessibility ideas

**Available now:** use the standalone reader to explore which comfortable finger
motions produce repeatable electrical ranges for a particular glove mounting.

**Extension:** define an opt-in control mapping in a host application, with clear
feedback and a way to cancel commands. Evaluate comfort, false activations and
recalibration with the intended users. This prototype has not been validated as
an assistive product, rehabilitation instrument or medical measurement device.
It reports a control signal rather than a diagnostic angle or clinical metric.

## Choose a practical starting point

Start with the [single-sensor build guide](BUILD_GUIDE.md). Use the reader for
teaching, logging and host interaction; add radio and one servo for remote
actuation. Keep calibration quality, mounting repeatability, latency and loss
behavior as explicit evaluation questions for every new application.
