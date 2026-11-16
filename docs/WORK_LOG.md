# Modernization work log

## 2026-11-16

- Cloned the requested repository; `master` was clean at `7e0747e`. The only original commit is by Kleber de Oliveira Andrade. No local project instructions or third-party source notices were present.
- Applied ub-mode and test-master. Inspected both original sketches before selecting a reference board. The source identifies analog flex inputs, RF24 and Servo, but no sensor part number, Grove module or board.
- Wrote host behavior tests first; observed the expected missing-core compilation failure, then implemented the portable C++11 core. Tests pass for filter startup/full scale/wrap/reset, insufficient calibration span, ADC bounds, clamping, packet format/length/version/mask/angles, and timeout rollover.
- Replaced the 100-sample matrix and repeated summation with an eight-sample rolling sum (32-bit accumulator). Calibration is explicit and frozen after five seconds; mapping rejects spans below 20 ADC counts. One connected sensor is enabled by default, matching the original active output.
- Separated hardware configuration, radio settings, transmitter and receiver. Added a radio-free reader build and deterministic host reading example. Radio errors are visible; invalid calibration or radio timeout detaches the receiver servo.
- Checked primary Arduino, Spectra Symbol, Nordic, RF24, Servo, PlatformIO and PyPI sources. Selected Uno R3 as an explicit reference, not a claim about the original board. Pinned current stable PlatformIO 6.2.0, Atmel AVR 5.3.0, RF24 1.6.2 and Servo 1.3.0.
- Invoked installed crapper, dryer (threshold 0.8) and mutator (scan) against firmware/include. All reported no supported source files: these tools do not support C++/Arduino. No CRAP, duplication or mutation score is claimed.
