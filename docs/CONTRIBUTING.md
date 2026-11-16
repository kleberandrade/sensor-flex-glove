# Project layout and contribution guide

## Folder conventions

The repository is intentionally small. Keep one source of truth for shared
behavior rather than copying sketches for each example:

```text
.github/workflows/       Hosted host and firmware checks
firmware/
  transmitter/main.cpp   Uno acquisition, serial calibration and optional RF
  receiver/main.cpp      RF commands and five-servo hardware adapter
include/flex/
  core.hpp               Portable C++11 processing and protocol; no Arduino API
  receiver.hpp           Five-channel command validity and timeout state
  servo_output.hpp       Pulse limits and Servo-like output adapter
  servo_motion.hpp       Nonblocking per-channel pulse slew state
  servo_calibration.hpp  Runtime endpoint profile and one-servo test session
  serial_command.hpp     Bounded serial line reader and validated commands
  calibration_storage.hpp  Versioned two-slot EEPROM journal and CRC
  hardware.hpp           Shared Uno pins, active channels and timing
  radio.hpp              Shared nRF24L01+ adapter settings
examples/readings.cpp    Executable synthetic reading example using the core
tests/core_test.cpp      Host behavior tests of the portable core
tests/receiver_test.cpp  Five-channel state and pulse/output lifecycle tests
tests/calibration_test.cpp  Parser/session/journal interruption and recovery tests
tests/inversion_test.cpp  Direction mapping, persistence and legacy fixtures
tests/speed_test.cpp     Slew timing/state and v1/v2→v3 journal migration tests
tests/receiver_firmware_test.cpp  Actual receiver entry points with host doubles
tests/support/          Explicit host Arduino/RF24/Servo/EEPROM substitutes
docs/
  hardware/              Electrical reference, BOM, pinout and original circuit
  images/                Original conceptual illustrations embedded in guides
  BUILD_GUIDE.md         Staged physical assembly and operation guide
  SERVO_CALIBRATION.md   Serial command reference, workflow and EEPROM format
  APPLICATIONS.md        Current uses and explicitly scoped extensions
  CONTRIBUTING.md        This layout and development workflow
  THIRD_PARTY.md         Dependency licenses and distribution notes
  WORK_LOG.md            Implementation decisions, checks and limitations
platformio.ini           Reproducible firmware targets and dependency versions
requirements-dev.txt     Pinned Python tooling and its dependencies
Makefile                 Host test/example entry points
```

`reader` deliberately builds the transmitter source with `FLEX_READER_ONLY`:
calibration, filtering and CSV behavior stay shared, while radio code is
excluded. It does not need its own copied firmware directory. Public shared
headers live under `include/flex`; hardware-specific code belongs in the adapter
headers or firmware entry points. Keep host example code outside the firmware
build source tree. Generated `.pio`, `.platformio`, `.venv` and `build` folders
are ignored and are not project source.

## Make and verify a change

1. Establish a clean Git state and inspect the relevant source and documentation.
2. For portable behavior changes, write a focused failing test first, observe the
   failure, then implement the smallest fix. Cover invalid and boundary inputs.
3. Keep ADC acquisition, radio I/O and servo calls out of `core.hpp`. Do not mock
   a hardware test and describe it as a real sensor or radio experiment.
4. Run `make test example`. Use compiler warnings and host sanitizers when changing
   memory, indexing or arithmetic behavior:

   ```sh
   make test BUILD_DIR=build/sanitized \
     CXXFLAGS="-std=c++11 -Wall -Wextra -Werror -pedantic -Iinclude -fsanitize=address,undefined -fno-omit-frame-pointer"
   ```

5. Activate the project's virtual environment and run `pio run` for all three
   environments. Compilation proves target compatibility, not physical function.
6. Update protocol, pinout, examples and calibration documentation if behavior
   changes. Keep both wireless nodes compatible or document a coordinated migration.
7. Check local Markdown links, render any changed SVG, and run `git diff --check`.
8. Keep commits focused and describe the observed failure, fix and validation.
   Follow the CI run on `main` or your pull request through completion.

The CI uses Ubuntu 24.04, Python 3.12 and pinned official Actions. Its jobs test
host behavior/sanitizers and compile the three Uno targets. It cannot test
physical electrical limits, RF range or a mechanism. Package updates should
re-resolve the tooling lock file, preserve third-party licenses and pass the
same checks before merging. EEPROM schema changes also need byte-level migration/rejection
tests, reserved-address documentation and interruption/recovery coverage. Keep
serial processing bounded and detach manual outputs before EEPROM writes.
