# Modernization work log

## 2026-11-16

- Cloned the requested repository; `master` was clean at `7e0747e`. The only original commit is by Kleber de Oliveira Andrade. No local project instructions or third-party source notices were present.
- Applied ub-mode and test-master. Inspected both original sketches before selecting a reference board. The source identifies analog flex inputs, RF24 and Servo, but no sensor part number, Grove module or board.
- Wrote host behavior tests first; observed the expected missing-core compilation failure, then implemented the portable C++11 core. Tests pass for filter startup/full scale/wrap/reset, insufficient calibration span, ADC bounds, clamping, packet format/length/version/mask/angles, and timeout rollover.
- Replaced the 100-sample matrix and repeated summation with an eight-sample rolling sum (32-bit accumulator). Calibration is explicit and frozen after five seconds; mapping rejects spans below 20 ADC counts. One connected sensor is enabled by default, matching the original active output.
- Separated hardware configuration, radio settings, transmitter and receiver. Added a radio-free reader build and deterministic host reading example. Radio errors are visible; invalid calibration or radio timeout detaches the receiver servo.
- Checked primary Arduino, Spectra Symbol, Nordic, RF24, Servo, PlatformIO and PyPI sources. Selected Uno R3 as an explicit reference, not a claim about the original board. Pinned current stable PlatformIO 6.2.0, Atmel AVR 5.3.0, RF24 1.6.2 and Servo 1.3.0.
- Invoked installed crapper, dryer (threshold 0.8) and mutator (scan) against firmware/include. All reported no supported source files: these tools do not support C++/Arduino. No CRAP, duplication or mutation score is claimed.
- Built `transmitter`, `receiver` and `reader` successfully with the real AVR compiler (7.3.0), Arduino AVR package 5.4.0 and pinned libraries. Compiler/framework packages are pinned explicitly in addition to the platform version. The first attempted tool installation was 6.1.18; the official PyPI check identified 6.2.0 and it replaced that version before validation.
- Ran `make test example`, AddressSanitizer/UndefinedBehaviorSanitizer host checks, and LLVM source coverage. The portable `include/flex/core.hpp` reports 47/47 executable lines, 8/8 functions and 38/38 branches exercised. This does not measure hardware adapters, RF behavior or mechanical behavior. No mutation score is available.
- Firmware size: transmitter 4,828 flash / 270 static RAM bytes; receiver 4,896 / 279; reader 2,788 / 224. Stack usage and runtime hardware behavior are not measured.
- Added an English README, migration/protocol guide, original SVG circuit, source-backed BOM and electrical pinout. Checked local Markdown targets and parsed SVG XML. Quick Look cropped its preview; rendered the SVG in an isolated headless Chrome profile and inspected the complete image for legibility and connections.
- Added MIT for author-owned source/docs and documented separately downloaded third-party licenses, including RF24 GPL-2.0. No third-party code or diagrams were relicensed or vendored.
- Added host/sanitizer and firmware CI jobs on `master`/pull requests; verified and pinned current official checkout v7.0.1 and setup-python v7.1.0 commit SHAs. Hosted Actions have not run because these commits have not been pushed.
- Updated GitHub About through the authenticated API and checked the returned metadata: “Arduino flex-sensor glove with calibrated readings, nRF24L01+ wireless control, and host-tested signal processing.” Default branch confirmed as `master`. No Git push, deployment, upload to a board or package publication performed.
- Environment notes: clone/downloads required approved network access; the initial system Python 3.9.6 emitted urllib3's LibreSSL warning, so the task-created virtual environment was replaced with the installed Python 3.13 for final validation. The Arduino core emits upstream unused-parameter warnings with `-Wextra`. CI uses Python 3.12; that hosted environment has not been executed here.
- Remaining validation requires actual hardware: sensor identification, rail measurements, ADC travel, radio/ACK reliability, servo pulse/travel limits and timeout behavior. The provided wiring is a reference design because the original module is unidentified.
- Final toolchain check used Python 3.13 with a fresh virtual environment. Locked the complete resolved Python tool dependency set in `requirements-dev.txt`; checked each package's declared Python requirement against CI Python 3.12 and ran `pip check`. Kept the local virtual environment and compiler cache ignored for continued project use.

## Branch migration and publication (2026-11-16)

- The user explicitly authorized publishing the four modernization commits and changing the default branch from `master` to `main`.
- Verified that the remote has only the unprotected `master` at `7e0747e`, that no `main` exists, and that the authenticated account can administer this repository.
- Updated the workflow push trigger and README badge to `main`; historical entries above retain the branch names and validation state at the time of their execution.
- Publication uses GitHub's branch rename operation followed by a normal fast-forward push, preserving history without a force push. The active chat follows the hosted CI to completion and handles any failures.

- The first hosted run passed host tests/sanitizers but failed in setup-python before compilation: its pip cache searched default dependency filenames and did not find `requirements-dev.txt`. Set `cache-dependency-path` explicitly to the existing lock file; no firmware workaround or cache removal is needed.
