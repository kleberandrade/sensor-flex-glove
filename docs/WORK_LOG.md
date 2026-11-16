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

## Glove guide and applications expansion (2026-11-16)

- Confirmed that the project is a glove and added an original code-native SVG to the README: back-of-hand sensor placement, active A0 channel, optional strips, serial/RF flow and synthetic 700 → 675 → 160° data from `make example`. No physical measurements or finger-angle accuracy are claimed.
- Added a staged build guide covering preparation, divider checks, single-channel readings, calibration, CSV capture, wearable mounting, RF, external servo power, loss behavior, five-channel expansion and troubleshooting. Verified monitor flags against the installed CLI help.
- Reviewed folder responsibilities and retained the existing firmware/include/tests/examples split. Added `docs/images` for conceptual illustrations and a contribution guide that documents the full tree and shared reader source, avoiding copied sketches or disruptive directory churn.
- Documented educational sensing, remote actuation, gesture datasets, game/music controls and wearable interaction applications, explicitly distinguishing existing demonstrations from extensions requiring implementation and validation.
- Host tests/example and local Markdown link/SVG XML checks pass. Rendered the glove illustration in an isolated headless Chrome profile and inspected placement, labels and data cards.
- Hosted CI passed both jobs after the cache-path fix: https://github.com/kleberandrade/sensor-flex-glove/actions/runs/43799681424 (`fa16a26`). The documentation update is also followed through CI after publication.

## Repository naming and electronics illustration (2026-11-16)

- Renamed the GitHub repository to `sensor-flex-glove` at the user's request; preserved repository history, About text and default branch `main`. Updated the origin, clone command, CI badge and all repository URLs, including links to earlier workflow runs.
- Added a separate, original electronics-only SVG illustration to the README and hardware guide: discrete A0 divider, two Uno/nRF24L01+ nodes, decoupling and an independently powered receiver servo. The artwork is explicitly a reference illustration rather than a photograph or PCB layout.
- Retained the detailed circuit and its electrical constraints as the wiring source of truth. No firmware behavior or hardware specification changed.
- Aligned the local checkout directory with the new repository name and recreated only the task-created Python virtual environment, preventing stale executable paths after the directory move. Verified documentation links/anchors, renamed URLs and SVG XML; rendered and inspected the electronics illustration.
- After relocating the checkout, `make test example`, `pip check` and all three real Uno builds passed with the pinned environment. No tracked source/documentation retains the previous repository name.

## Breadboard reproduction diagram (2026-11-16)

- Added an original SVG with numbered rows/columns and exact occupied holes for the single-sensor divider. Distinct wires connect Uno 5V/GND/A0, the flex strip e5–e10, the fixed resistor c10–c15 and the rail jumpers.
- Included named-pin wireless additions for each Uno, parallel local radio capacitors with polarity, and receiver servo wiring with an external supply/common-ground return. Kept servo current out of the breadboard. No unidentified 2×4 module header orientation is assumed.
- Embedded the image in the README, hardware reference and staged build guide. Added the hole-by-hole table, strip/gap explanation, rail-continuity checks and the primary breadboard-vendor reference. Existing electrical limits and firmware pin assignments remain unchanged.
- Validated Markdown links/anchors and SVG XML, compared all 19 labeled wiring endpoints with the documented circuit, and checked CE=D7, CSN=D8 and servo=D2 against the firmware configuration. Rendered and visually inspected the final image, including hole positions, resistor bands, capacitor/supply polarity and label legibility. This is documentation-only; physical continuity/electrical checks remain a builder responsibility.

## Five-finger breadboard expansion (2026-11-16)

- Added a separate original SVG for five independent sensor/resistor dividers and suggested index/middle/ring/little/thumb mapping to A0/A1/A2/A3/A4. Shared rails are explicit; ADC junctions remain separate.
- Added a complete hole table and README/build steps for enabling `kActiveFingers=5`, rebuilding, observing CSV channels 0–4 and calibrating every finger. Warned that this is an alternative layout requiring removal of the earlier jumpers, since some row functions change.
- Preserved the one-channel quick-start default and clarified that the receiver continues to actuate channel 0 only. No five-servo behavior or physical validation is claimed.
- Validated all 32 labeled connections in the SVG and resolved breadboard strips to confirm that the five ADC junctions are distinct and not directly tied to either rail. Checked local Markdown links/anchors and SVG XML, then rendered/inspected the final diagram.
- Built both `reader` and `transmitter` successfully for Uno with `kActiveFingers=5` in an isolated temporary project using the pinned libraries/toolchain. Removed that temporary project automatically; the tracked default remains 1 and no firmware source was changed. These are compilation and diagram checks, not a physical five-sensor test.

## Five-servo receiver implementation (2026-11-16)

- Applied ub-mode and test-master. Added receiver policy/output tests first and observed the expected missing-header failure. Implemented five independent commands with per-channel validity, malformed-frame rejection without deadline refresh, 500 ms rollover-safe expiry and recovery.
- Added a portable Servo output adapter with per-channel pulse settings. Commands 10–170 map to explicit 1000–2000 µs default endpoints, clamped before output; pulse targets are primed before attach/re-attach. Verified actual pinned Servo library behavior before choosing default attach bounds (avoids AVR custom-bound int8_t offset limitations).
- Receiver now maps channels 0–4 to D2–D6. The one-sensor transmitter default is preserved: inactive mask bits leave other outputs detached. Wire format v1 is unchanged.
- Added host tests of the actual receiver entry points using explicit Arduino/RF24/Servo doubles, after observing the missing host-Arduino-header failure. These check boot, all five distinct pin/pulse outputs, malformed frame behavior, exact timeout, partial recovery, zero-mask detach and bounded FIFO drain. Doubles do not simulate electrical hardware or AVR interrupt timing.
- `make test example`, all three host test executables under AddressSanitizer/UndefinedBehaviorSanitizer, and all three real Uno builds passed. Corrected a binary literal in the test to hexadecimal to retain strict C++11 compatibility. CI now runs every host test in both normal and sanitizer builds.
- Invoked crapper, dryer (0.8) and mutator scan again; C++ remains unsupported and all returned no supported source files. No tool metric is claimed.
