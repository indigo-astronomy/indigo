# INDIGO Test Suite Changes

## ToupTek shared hot-plug shutdown (2026-09-08)

The hand-written ToupTek/OEM lifecycle now follows the generated SDK sequence: connection and discovery handlers share the driver task mutex with disconnected-device verification; rejected shutdown preserves the existing callback registration; accepted shutdown deregisters, drains queued events, then detaches and deletes the queue. SDK-id discovery and multi-class camera/guider/wheel/focuser teardown remain driver-specific. The fake SDK lifecycle test asserts no deregistration/re-registration on rejection. Its pending-shutdown test verifies all 64 accepted notifications execute (96 SDK enumerations), followed by clean detach and successful reinitialization. Targeted lifecycle, pending-shutdown and hot-plug/partial-attach/active-removal tests passed without hardware.

## All generated hot-plug transports drain before detach (2026-09-08)

The generator now shares SHUTDOWN emission across libusb, SDK and HID transports. The architecture regression covers all three plus SDK discovery retries: serialized connected-device verification, rejection before deregistration, queue drain before detach/delete, and retry cancellation without holding the task mutex. The PlayerOne fake SDK queued-shutdown scenario also detects any detach while its blocked queue callback is unfinished. Validation passed: the generator suite (including 86 DSL cases and four hot-plug variants), the three targeted PlayerOne fake SDK scenarios, and full root `make all` for macOS x86_64/arm64. Hardware tests were not run.

## Shared handler-queue drain (2026-09-08)

`indigo_queue_drain()` waits for both pending and running tasks, including delayed and callback-enqueued work, without canceling tasks. Producers/recurring tasks must be stopped first and the caller must own the queue lifetime. Calls from the worker or with NULL return false. Queue notifications broadcast to wake the worker and all drain/remove waiters.

The timer unit suite covers an empty/reusable queue, a blocked running callback, delayed and callback-enqueued tasks, two concurrent drain callers, self-call rejection and wakeup after removal of pending tasks. The generator emits a single drain call after USB deregistration and before detach/delete; its regression test rejects the old generated condition variable/callback. The existing SX pending-discovery and rejected-shutdown tests exercise the integration. No hardware tests are used. Validation passed: complete timer unit suite, generator regression suite, SX pending-discovery shutdown and shared-lifecycle/rejected-shutdown scenarios, and the macOS SX driver build.

## SX complete applicable-standard fake USB suite (2026-09-08)

The SX target now contains 24 named groups covering the applicable CCD and guider standard scenarios. The coverage/N/A matrix and validation details are in `../indigo_drivers/ccd_sx/REFACTOR.md`. Normal and ASan/UBSan runs passed all 24 groups; final metadata and zero-valued guide completion assertions passed narrow reruns. The suite includes 80 monotonic USB ON/OFF pulse samples with discarded warm-ups, both idle and during acquisition. Hardware tests were not run.

Regressions fixed in the SX `.driver` include failed-open resource rollback, config-descriptor and ICX453 buffer cleanup, incomplete control packets, propagation of read/clear errors, zero-signal interlaced normalization, invalid-bin preservation, target/readback separation, unavailable cooling, and guider replacement/coalescing/error/stop handling. C is regenerated, not hand-edited. User-approved generator fixes cover duplicate arrivals, failed master attach, INIT rollback and SHUTDOWN synchronization/draining. Emitted cleanup uses INDIGO allocation helpers.

The generator test retains its 86 attribute/value/block checks and adds libusb lifecycle invariants; SX's fake tests execute those generated paths with actual failures and controlled concurrency. Configuration codecs and image containers remain framework responsibilities.

## CCD cooling and fake-boundary extensions (2026-09-08)

The camera standard explicitly requires cooler ON/OFF, separate target/measured temperatures and unit conversion, supported power readback, polling/settling, individual read/write failures and recovery, unsupported capability profiles and disconnect cleanup. Apply only controls the driver implements: SX does not report cooler power; the simulator has no SDK communication errors to inject. Hardware tests were not repeated. Altair remains covered as the shared ToupTek implementation, without a separate OEM target as requested.

- Player One: existing cooler polling, individual SDK failure and slow-initialization cases remain; added exact Bayer mapping and continuous SDK acquisition argument assertions.
- ToupTek: independent measured temperature, target conversion, temperature and both power-read failures, recovery and settling; failed initialization reads and acquisition setup, Bayer mapping, ROI/bin noise payloads, multiple-camera identity and capacity recovery.
- SX: new fake USB target covers cooled/uncooled profiles, target conversion, measured temperature, settling, ON/OFF, command/read/short-reply failure recovery, shared guiding, progressive/interlaced/ICX453 readout, ROI/bin/shutter and transfer failure recovery. Regressions exposed lost targets during polling, visible unsupported cooling controls, short cooling replies accepted as success and pixel read failures treated as success. Fixes live in `ccd_sx/indigo_ccd_sx.driver`; C is regenerated.
- CCD simulator: added RAW geometry/bin, streaming/abort/reconnect, cooler target/settling/power, camera simulation modes and file-camera generated-noise inputs. Shutdown now detaches slave devices before freeing their master.

Fake image data uses coordinate-addressable deterministic noise from `integration/ccd_test_noise.h`; no simulator image arrays are linked into fake SDK tests. New files are included in Xcode. ToupTek framework encoding/video/upload matrices were removed from the driver suite. The earlier audit below is a historical snapshot, not the current implementation inventory; these additions do not claim exhaustive branch coverage.

Validation: Player One 45/45, ToupTek 28/28, SX 5/5 and CCD simulator 16/16 normal tests pass. The final ToupTek power/initial cooler-read assertions and simulator per-property revision waits passed narrow reruns. Simulator cooling/shutdown also passes ASan/UBSan (test and driver instrumentation; prebuilt dependencies excluded). SX, ToupTek and simulator driver builds pass. Repeated SX generation produces identical C/header/main files; Xcode project lint and whitespace checks pass. No hardware tests ran.

## CCD fake-boundary coverage audit (2026-09-08)

Static coverage audit of the current working tree against the camera standard and its referenced guider/shared-lifecycle requirements. This is a scenario-coverage check, not a line/branch coverage measurement or a new passing test run. No driver or hardware tests were executed and no production code was changed.

| CCD driver with an existing test | Test boundary | Full fake SDK/USB coverage established? |
| --- | --- | --- |
| `ccd_playerone` | Production driver with fake POA SDK/USB, 44 named cases; separate hardware harness | No: concrete argument/metadata assertions are missing as listed below. |
| `ccd_touptek` | Production driver with fake Toupcam SDK/USB, 25 named groups; separate hardware harness | No: geometry, failure injection and discovery-profile coverage remain incomplete. |
| `ccd_altair` | Real Altair SDK selected by the shared ToupTek hardware harness | No: there is no Altair fake SDK build/test target. Shared source does not validate the OEM build. |
| `ccd_simulator` | Direct public-bus tests of the simulator and its logical devices | Not applicable to fake SDK/USB: this driver has neither boundary. Its direct tests are compliance/smoke coverage, not full driver-behavior coverage. |

Concrete missing assertions and scenarios:

- Player One: `POAStartExposure` ignores its `single` argument (`integration/test_ccd_playerone_sdk.c:661`). Assert `POA_FALSE` for both single INDIGO exposure and streaming, preserving the continuous-mode workaround used by the production driver (`../indigo_drivers/ccd_playerone/indigo_ccd_playerone.driver:409`). No physical Saturn-C is needed for this assertion.
- Player One: the RAW observer checks only the presence of `BAYERPAT=` (`integration/test_ccd_playerone_sdk.c:239`); fixture initialization selects only `POA_BAYER_RG` (`:1960`). Check the actual handed-off BGGR/GRBG/GBRG/RGGB mapping and unsupported-pattern behavior. Existing RGB pixel checks and mono/no-Bayer cases do not establish that mapping.
- ToupTek: `Toupcam_PullImageV2` always returns 16×16 pixels from the beginning of the simulator fixture (`integration/test_ccd_touptek_sdk.c:1749`). The observer also expects 16×16 (`:282`). ROI/bin tests check outgoing options (`:971`) but do not validate the corresponding delivered geometry, ROI pixel mapping or Bayer value (`Toupcam_get_RawFormat` returns FourCC zero). This is driver raw-handoff coverage, not an encoder test.
- ToupTek: several relevant SDK reads/start/stop calls cannot fail in the fake: `Toupcam_get_Option` (`:488`), `Toupcam_StartPullModeWithCallback` (`:1774`), `Toupcam_Stop` (`:1782`), exposure/gain range reads and temperature reads near the end of the file. Add targeted failures with untouched output and assertions for the actual driver error/cleanup branches; existing write/trigger/pull failure tests do not cover these paths. This also limits wheel/cooler readback-failure coverage.
- ToupTek: enumeration is limited to three fixed physical records with ids `0`, `1`, `2` (`integration/test_ccd_touptek_sdk.c:461`): one camera, a wheel and a focuser. Combined-interface and duplicate-event tests exist, but two independent cameras with identical names, reordered enumeration/nontrivial ids, survivor acquisition and capacity overflow are not established by this fixture.
- Altair: `Makefile:780` links its hardware object and real SDK; the fake target builds only the ToupTek variant. Provide an OEM-specific fake boundary/build before claiming automated fake coverage for Altair. Other OEM variants have no driver-specific test target in this inventory.
- CCD simulator: `integration/test_ccd_simulator.c` checks metadata, public properties and representative logical-device actions/short exposures. Driver-specific image generation, streaming/abort races and error paths are not comprehensively covered by these checks; do not invent a vendor SDK solely to test a simulator.

The existing fake suites cover substantial lifecycle, controls, acquisition and race behavior. The gaps above are sufficient to reject a full-coverage claim; counts of named cases or earlier successful runs do not close them. CCD SX has no dedicated automated CCD test; `test_ao_sx_simulator.c` exercises AO/guider, so SX is outside the tested-CCD inventory. Framework codec/upload tests still present in the ToupTek suite do not count toward the missing driver coverage.

## Driver test scope by class (2026-09-08)

The same driver-only scope now includes AO corrections/reset/limits/shared guider ownership and GPS parsing/fix lifecycle/source selection/reader cleanup, with separate hardware acceptance. AO steps are distinguished from guider pulse durations. No implementation or hardware tests were run for this documentation update.

`DRIVER_TESTING_RULES.md` now defines shared driver-only scope and separate fake SDK/protocol and real-hardware acceptance for mounts, wheels, focusers, rotators and guiders. Camera pulse scenarios and SDK-entry timing measurements moved into the guider standard, referenced by camera and mount sections. Framework behavior and optical/electrical performance measurements are outside driver acceptance. These are coverage requirements, not claims of newly implemented tests. This documentation-only change runs no driver or hardware tests.

## Player One final plan audit (2026-09-08)

`test_ccd_playerone_sdk` now has 44 named fixture-isolated cases. The added groups cover required initialization/attribute/preset read failures with untouched outputs, optional mode failures, geometry failures, SDK identity and bounded strings, individual cooler failures, partial control writes/readback, partial guider attachment, before-handler/setup/readout aborts, queued shutdown, bounded discovery retries, invalid/aligned ROI/bin behavior and RAW16-only capability, and slow initialization crossing accelerated polling intervals. All camera SDK calls are checked for overlapping access and use after close; fixture cleanup failures affect the executable's exit code and an unmatched filter fails.

At that checkpoint images used the shared CCD simulator fixtures; the current fake tests use generated noise. RAW pixels and Bayer metadata are checked. Framework codec, container and upload-destination tests were removed; the suite now contains 44 driver-focused cases. The modified frame-type/exposure-unit, finite-stream-count and RAW/ROI/bin cases passed after scope reduction; hardware tests were not repeated. Timing retains 120 measured guide pulses without a machine-dependent error threshold. Normal, safe-readout and ASan/UBSan validation passes; final test-only additions are rerun narrowly in each configuration.

The generator suite now checks 86 attribute/value/block cases, including opt-in `discovery_retries`. The Player One integration test verifies late SDK visibility without another arrival, six-retry exhaustion, removal cancellation, and pending-retry shutdown cleanup. Reference-driver regeneration changes only the already approved registration/queue rollback paths. Hardware test modes `--acceptance` and `--suffix` isolate physical interruption and flash/name acceptance; `test-ccd-playerone-reload-hw` uses a shared bus and actual dynamic driver unload/reload, excluding configuration SAVE to avoid user settings. These modes remain outside normal integration tests and use existing Xcode-referenced source files.

The earlier gap lists below are historical checkpoints. The final migration matrix and named coverage mapping are in `ccd_playerone/REFACTOR.md`; hardware-only and platform deferrals are recorded there and in `TESTING.md`. Electrical ST4 measurements, other/colder models, multiple real cameras and unavailable OS runners remain unverified. Driver tests do not replace exhaustive shared-framework codec or every hypothetical future SDK capability test.

## Camera test standard and generator parsing (2026-09-08)

Camera fake SDK/USB and hardware scenarios now live in `DRIVER_TESTING_RULES.md`; root and test `AGENTS.md` contain references only. The standard now requires generated noise for fake images and defines guider timing at SDK entry.

The generator architecture suite checks all supported named DSL attributes and code blocks with 85 table cases, including true/false settings, generated behavior and parser traces. Separate fixtures check `name`/`name_value` in either order and `name` alone. Tests exposed `handler` being consumed as `handle`; the user-approved fix parses `handler` first. Both declaration orders now have regression coverage. The prefix matching implementation remains unchanged.

## Expanded Player One coverage (2026-09-08)

The fake SDK suite now has 32 fixture-isolated cases. Added coverage includes property metadata, repeated shared lifecycle, simulator-backed RAW pixel verification in four SDK formats with ROI/bin mapping, frame types and output encodings, controls and failures, suffix boundaries, simultaneous guide axes, registration/global-lock rollback, active removal, upload destinations and video headers, controlled final-frame/abort orders, cooled-profile polling, discovery bursts and queue failure, the real readout deadline, capability rebuilding and busy controls. Guider timing now measures 120 pulses at 20/50/100/250/500 ms, all directions, three repeats, idle and during exposure.

The temperature case exposed a polling race: an asynchronous handler read the measured value after polling could overwrite the requested value. The driver now preserves measured values and consumes the requested target. Shared open is transactional and no longer needs a private `opened` flag.

Validation: all 32 cases pass in the normal macOS build, explicit `POA_SAFE_READOUT=1` build, and arm64 ASan/UBSan build after the temperature fix. Instrumentation covers the test, production driver and separately compiled framework objects; prebuilt dependencies remain uninstrumented. The generator architecture target passes all five groups, including its 85 attribute/value/block fixtures.

Remaining standard gaps include exhaustive property/item boundaries and SDK read/partial-write failures, duplicate model names and bounded SDK strings, full payload decoding for every output encoding, queued-discovery shutdown races, and proof of every SDK call's shared-handle serialization. These remain deferred coverage, not implied passes. Hardware coverage is recorded separately in `TESTING.md`.

## Player One camera generator migration (2026-09-08)

Added `integration/test_ccd_playerone_sdk.c`, initially with three passing hardware-free cases using separately compiled production driver/framework and SDK/USB substitutes: CCD property definitions, RAW BLOB acquisition and isolated CONFIG save; guider-first shared connection, pulse completion and CCD disconnect while guider remains connected; exact three-frame streaming. Added opt-in `test-ccd-playerone-hw`, excluded from normal integration targets. Full capability/error/payload/configuration-reload/queue-interleaving coverage remains planned in `ccd_playerone/REFACTOR.md`. Physical Mars-C II baseline acquired images but failed later lifecycle validation with a libusb mutex assertion; physical output and hot-unplug tests remain pending.

The generated driver now additionally passes finite streaming, bounded SDK short-wait retry, abort/reacquire and guiding during long exposure, readout failure recovery, Open/Init/config-enumeration rollback, optional guider/capacity and SDK-id-based removal, failed master attach recovery, busy guards, configuration reload/suffix boundaries, and pulse replacement/disconnect cleanup. Guider completion uses `guider_ra_finalizer`/`guider_dec_finalizer`; regression checks explicitly observe BUSY before completion so missing initiation updates after generator suppression cannot silently pass. The real SDK lifecycle test subsequently passed, including driver shutdown/reinitialization (the SDK library remains loaded). The full adversarial matrix in REFACTOR.md is not yet claimed complete.

Added a thirteenth case measuring guider ON-to-OFF duration at entry to fake `POASetConfig`, using `CLOCK_MONOTONIC` and a mutex-protected per-camera/per-direction edge recorder. It ignores redundant OFF writes, excludes one warm-up per phase, and reports each requested/measured duration, signed error in ms/percent, and min/mean/median/p95/p99/max/standard deviation. Three repeats of NORTH/SOUTH/EAST/WEST at 20/100/250 ms run both idle and during a 30-second exposure (72 measured pulses). Completion and acquisition coexistence are assertions; host scheduling precision is reported without a flaky fixed error threshold. This measures driver-to-SDK command timing, not physical ST4 output. On macOS arm64, 2026-09-08: idle mean +2.426 ms, max +5.039 ms; exposure mean +3.049 ms, max +5.041 ms. All 13 cases passed. Source remains in the existing Xcode integration-test reference.

This document records the automated test suite added under `indigo_test/` and the remaining follow-up work. The suite is intentionally hardware-free: it links against the built INDIGO library and simulator driver archives, then exercises public APIs through unit and in-process integration tests.

## Astroasis wheel generator migration (2026-09-08)

Added `integration/test_wheel_astroasis_sdk.c` and the normal integration target
`build/integration/test_wheel_astroasis_sdk`. The generated driver is compiled
separately against Oasis SDK/libusb stubs, with real bus, queue and wheel/base
handlers. Configuration is redirected by a test-local framework object to a
fresh temporary directory; the fixture removes it without changing HOME.

Ten fixture-isolated groups cover:

- public metadata, inherited properties/configuration/profile roundtrip,
  custom-property visibility and factory-reset confirmation hint;
- five/16-slot counts, allocation bounds, 50/51-byte filter-name limits,
  one-based positioning and actual bus clamping;
- global-lock/open/required-read failures, optional Bluetooth NOT_IMPLEMENTED,
  repeated connect/disconnect and shutdown rejection while connected;
- movement start/read failures, BUSY suppression, fractional/NaN requests,
  wrong-target/invalid status, timeout and checked recovery;
- calibration no-op/start/read errors, completion/timeout and competing
  movement/reset requests;
- suffix lengths 0/1/31/32/33, failed-write rollback, reconnect readback,
  reset no-op/failure/success/readback failure while remaining connected;
- default five-device capacity and recovery, independent USB/SDK ordering,
  multiple removals, failed/inconclusive scans and USB reference balance;
- probe open/version/model/suffix failures, attach retry, duplicate events,
  descriptor/product filtering, full-length naming and replacement SDK IDs;
- disconnect/unplug during operation and held SDK work, cancelled polling,
  no late property updates and balanced SDK/global-lock ownership;
- moving/calibrating/benchmarking initialization, invalid completion,
  and ignored direct requests to hidden Bluetooth properties.

All ten groups passed normally and with AddressSanitizer/UndefinedBehaviorSanitizer.
Instrumentation covers the test, generated driver and test-local base/framework
objects; the shared INDIGO library and vendor SDK are not instrumented (the
vendor SDK is replaced by stubs). The timeout cases accelerate callback dispatch
while retaining the production 240-poll budget. Existing ASI (5 cases) and Player
One (12 cases) SDK suites passed as regressions.

Added `integration/test_generator_architecture.c`, runnable with
`make -C indigo_test test-generator-architecture`, for the user-approved optional
`supported_architecture` attribute. The C test uses `test_runner.h` and is also
part of the normal integration suite. It creates a minimal synthetic AUX driver in
a temporary directory, independently of production drivers and SDKs. An intentionally
missing SDK header verifies that the fallback excludes implementation includes.
It verifies nine platform/CPU combinations
(including Intel-only and Apple-Silicon-only macOS examples), reverse extraction
for three expressions, a compiled/executed unsupported INFO/INIT/SHUTDOWN fallback
without SDK linkage, and omission of the attribute. Generation without the new
attribute was also compared with the prior generator for six existing drivers;
all C/header/main outputs were byte-identical.

Hardware remains unavailable. SDK readiness on initial USB arrival, actual
name payload limits and operation/reset timing remain unverified. Probe failure
is retryable on a subsequent arrival event; no new automatic discovery retry is
claimed. Bluetooth stays hidden and its activation/write/readback hardware
matrix is deferred in `wheel_astroasis/REFACTOR.md`; the normal public bus cannot
exercise its dormant setters while the properties are undefined. Normal teardown
tests do not establish arbitrary concurrent hot-plug/shutdown safety.

## Player One wheel generator migration (2026-09-08)

Added `integration/test_wheel_playerone_sdk.c` and the normal integration target
`build/integration/test_wheel_playerone_sdk`. The generated driver is compiled
as a separate object against Player One SDK/libusb stubs. Tests use public driver
entry points and bus requests, a mutex-protected multi-wheel property cache,
and real wheel/device base handlers. A test-local framework object redirects
configuration files to a temporary directory without changing HOME; files are
removed after the run.

Twelve test cases cover:

- all exposed properties (`INFO`, `CONNECTION`, `CONFIG`, `PROFILE`,
  `PROFILE_NAME`, `WHEEL_SLOT`, `WHEEL_SLOT_NAME`, `WHEEL_SLOT_OFFSET`,
  `X_RESET`, `X_CUSTOM_SUFFIX`) and hidden/disconnected property visibility;
- slot names and offsets, configuration save/load/remove, profile naming and
  selection, five/16-slot metadata and the 50-byte filter-name boundary;
- repeated lifecycle requests, global-lock/open/metadata/position/suffix errors,
  invalid slot counts, asynchronous initial motion, bounded timeout and reconnect;
- first/last slots, bus range clamping, fractional/NaN rejection, BUSY request
  suppression, SDK movement/read failures, invalid and wrong-target replies;
- reset false-switch no-op, rejection during movement, failure, successful
  completion before disconnect/property deletion, and reconnect;
- empty/one/24-byte suffixes, 25-byte rejection, failed writes, reconnect/replug
  and full-length attached names;
- default five-wheel capacity, handles above 23, reverse connection order, reordered enumeration
  of attached wheels, duplicate USB arrival, capacity/retry, descriptor/enumeration
  errors, failed attach retry and malformed SDK names;
- disconnect and unplug during movement, including a held SDK read proving close
  waits for running work, cancelled polling and ignored disconnected requests;
- reversed USB/SDK arrival order, inconclusive SDK enumeration on removal,
  several removals in one USB event and separate per-USB-pointer reference balance;
- balanced SDK close/global locks and USB references during fixture teardown.

Verification: all 12 cases passed in the universal macOS build and in a native
AddressSanitizer/UndefinedBehaviorSanitizer build. The existing ASI wheel SDK
suite (5 cases) and timer/queue suite (86 cases) also passed. Sanitizers cover the
test, driver and test-local base/framework objects; the shared INDIGO library
itself is not rebuilt with instrumentation. The initialization-timeout test
accelerates delayed dispatch while retaining the production poll count.

Physical hardware is unavailable. Real SDK readiness and physical reset behavior
remain deferred. The optional `sdk.unplug_match` generator block is used only
by Player One; SDK-handle presence determines removal even if USB and SDK
arrival order differ. No new shutdown synchronization is introduced. No claim
is made that the stubs establish real SDK readiness timing or that arbitrary
simultaneous hot-plug/shutdown races are covered. Test outputs were cleaned.

## ToupTek guide coalescing and connection serialization (2026-09-08)

Added two hardware-free cases to `test_ccd_touptek_sdk.c`: queued full-vector guide replacements on both axes must issue exactly one SDK pulse per axis (DRV-067), and an immediately due temperature task must survive held camera initialization and run after connection completes (DRV-068 false-positive check). The guide test reproduced duplicated commands before the fix. The temperature test uses the real queue/master-mutex path and shortens only the first monitoring deadline, avoiding a five-second sleep.

Validation: native arm64 SDK suite 25/25 passed, including both new scenarios; universal ToupTek production build passed. No Rosetta or hardware run. Test build artifacts cleaned.

## ToupTek review regressions (2026-09-08)

Extended `integration/test_ccd_touptek_sdk.c` for DRV-061–DRV-066: opposite-direction replacement and zero-vector cancellation on both guide axes; replay of actual recurring callbacks after disconnect; stale ERROR/NOFRAMETIMEOUT callbacks during acquisition setup followed by successful same-mode exposures; combined camera/ST4/wheel/focuser discovery; manual relative motion after switching out of automatic compensation while still moving; and all-off CONFIG followed by SAVE on CCD, wheel and focuser. The guide, focuser-transition and CONFIG tests reproduced failures before their respective fixes. The recurring-task test deliberately replays the survivor task after disconnect; it does not force the queue's internal cancellation interleaving. Combined flags are synthetic and do not establish support for untested physical hardware.

Validation: native arm64 23/23 SDK scenarios and 86/86 timer/queue cases passed. Native AddressSanitizer passed the three existing races and all three new standalone scenarios (prebuilt libraries uninstrumented; leak detection disabled). No Rosetta or hardware execution. Test artifacts cleaned with `make -C indigo_test test-clean`.

## ToupTek SDK queues and finalizers (2026-09-08)

`integration/test_ccd_touptek_sdk.c` builds as `build/integration/test_ccd_touptek_sdk`, with separately compiled production driver, simulator image fixture and framework dispatcher objects. Twenty-five hardware-free scenarios use public bus requests, the real handler queues and SDK/USB replacements. The observer, test cases and config-path replacement are consolidated into `test_ccd_touptek_sdk.c`; no auxiliary ToupTek test source/header files are needed. After consolidation, the universal binary rebuilt without warnings and the configuration persistence/upload scenario passed.

Coverage:

- Lifecycle: CCD+guider connection in both orders, shared handle ownership, wheel/focuser teardown, queue/thread affinity, failed Open/queue creation/registration, unload/reload, rejected shutdown and callback re-registration, duplicate arrival, partial attach rollback and unplug during active acquisition, guiding and wheel initialization.
- Properties: enumerate all four logical devices, validate published vectors/items and round-trip passive writable properties through the real base/driver dispatch. Dedicated scenarios cover active workflows. CCD coverage includes all nine advanced controls, gain, offset, fan, heater, cooler, temperature, LED, conversion gain, all advertised RAW depths and RGB8, ROI, binning policies and frame types. Tests preserve gain fall-through versus offset early return.
- Acquisition: exposure completion, admission while BUSY, abort, watchdog, Trigger/PullImage failures, obsolete Stop notifications, reconnect/recovery, finite/infinite streaming, all seven image formats and SDK error/no-frame/no-packet events. The image callback runs on a separate thread. RAW pixels are compared with a 16 × 16 sample from `ccd_simulator/indigo_ccd_simulator_data.c`; FITS headers are checked. Local SER/AVI files are finalized and their signatures checked.
- Guider: all four directions, SDK direction/duration arguments, replacement while BUSY, all-zero cancellation, subsequent requests, reconnect and pulse completion timing.
- Wheel: 5/7/8-slot models, endpoint requests, calibration, SDK errors and cancellation. Focuser: absolute/relative motion, sync, limits, backlash, reverse, beep, abort, automatic/manual property permissions, actual temperature compensation and fallback to the internal sensor.
- Persistence/output: real CONFIG SAVE/LOAD for all logical devices, restored CCD/wheel settings, CLIENT/LOCAL/BOTH/NONE upload modes and temporary image/video files. The separately compiled framework dispatcher redirects only its config-directory lookup; serialization and parsing remain real. The test does not change HOME or use the user's config directory.

Three additional deterministic race scenarios close the previously deferred step 6 cases:

- Final frame versus abort: test-only gates force both orders on a one-frame stream. Abort first produces no frame and the standard finite-stream ALERT state; frame first delivers exactly one frame, completes streaming and reports ALERT for the now-inapplicable abort. Both orders allow a subsequent successful exposure.
- Disconnect inside the real SDK callback: pause its enqueue call after capturing the old generation, start disconnect, and assert SDK Stop is waiting to join that callback. Release it, verify no stale frame is pulled/published and the guider remains connected, then reconnect the CCD and acquire a new image.
- Rapid hot-plug/pending shutdown: eight full reconnect cycles with 1,024 alternating arrival/removal notifications, followed by shutdown with 64 explicitly pending USB events. Assert cancellation without further enumeration, balanced handles/global locks, no attached devices, and successful reload/connect/disconnect/shutdown.

The synchronization gates exist only in the harness and have 10-second deadlines. The production callback enqueue API is redirected through a gate that delegates to the real API; no production queue implementation or driver logic is modified.

Guider timing uses `CLOCK_MONOTONIC` at the fake SDK ST4 command and at the matching zero-valued OK property update. Each run measures 20 pulses: EAST/WEST/NORTH/SOUTH at 20, 50, 100, 250 and 500 ms. Output includes each measured duration and signed error, plus mean signed error, mean absolute error, p95 absolute error and maximum absolute error. A broad -5 to +250 ms completion bound catches gross regressions; these measurements characterize software completion latency, not electrical ST4 signal duration or an accuracy guarantee under arbitrary system load.

Only watchdog waits of at least 25 seconds are shortened by the test hook, while their original duration is checked. Guiding and normal completion delays are not accelerated. Injected SDK/queue errors and watchdog log messages are expected. `INDIGO_TEST_FILTER` selects cases by a substring of their displayed title.

Initial validation: the universal test binary compiled without compiler warnings; the full native macOS arm64 run passed all 17 scenarios. Its 20 pulse samples measured mean +3.703 ms, mean absolute 3.703 ms, p95 absolute 5.048 ms and maximum absolute 5.066 ms. The full x86_64/Rosetta run also passed all 17 scenarios: mean +2.947 ms, mean absolute 2.947 ms, p95 absolute 5.048 ms and maximum absolute 5.063 ms. Both runs exited 0; `git diff --check` passed. Test build artifacts were removed with `make -C indigo_test test-clean`.

Race-extension validation: the full 20-scenario suite passed on native macOS arm64 and x86_64/Rosetta, both exiting 0. All three new race cases passed without gate timeouts. The universal build emitted no warnings; `git diff --check` passed and `make -C indigo_test test-clean` removed build artifacts. Repeat guiding mean errors were +3.443 ms (arm64) and +3.899 ms (Rosetta).

The tests retain current behavior: reverse-motion SDK failure is reported as OK by the driver; after cancelling a guide pulse by disconnect, the reconnect test submits both axis items to replace the retained direction value. The fake SDK cannot validate real USB transport, vendor SDK races, electrical timing, physical Intel hardware or sustained multi-camera load. Those remain hardware validation work.

## Opt-in ToupTek physical camera test (2026-09-08)

`hardware/test_ccd_touptek_hw.c` is deliberately outside the hardware-free integration suite. Run it only with `make -C indigo_test test-ccd-touptek-hw`; this target builds `build/hardware/test_ccd_touptek_hw` and invokes it with `--run`. Direct execution without `--run` exits 2 before initializing INDIGO or USB. Neither `make test` nor `test-integration` builds or runs this executable. Set `INDIGO_TEST_DEVICE` to an exact discovered camera name if multiple cameras are attached; otherwise exactly one camera with its matching guider is required.

The test compiles the production driver separately and links real INDIGO, libtoupcam and libusb, with no SDK/USB replacements. It connects the guider before the camera, acquires three 0.1-second RAW exposures, aborts a 5-second exposure, captures a five-frame stream, aborts an indefinite stream after at least three frames, sends 100 ms pulses in all four directions, verifies guider operation after CCD disconnect, closes the shared handle, reconnects camera first and acquires another image. RAW signatures, dimensions, payload sizes and frame counts are checked. Only CCD_IMAGE updates count as new frames; reconnect property definitions are metadata. Cleanup disconnects both logical devices and shuts down the driver. The test does not save configuration or image files, but it does operate the physical ST4 outputs.

Native arm64 validation passed on Touptek GPCMOS01200KMB (camera suffix 04E69B): 12 valid RAW frames at 1280 × 960, 1,228,812 bytes each, including the post-reconnect frame. Both camera and guider disconnected and driver shutdown succeeded. The sandbox could see USB inventory but the SDK could not enumerate the camera; the successful run used explicitly approved execution outside the sandbox. Rosetta was not run. Compilation/linking succeeded; the linker reported that the bundled SDK targets macOS 11.0 while the repository build flags target 10.10. `make -n test` confirmed absence of the hardware executable, direct execution without `--run` returned 2, and `git diff --check` passed. Build artifacts were removed with `make -C indigo_test test-clean`.

Step 7 extension: `HW_DRIVER=altair` selects the real Altair SDK; `HW_HOTPLUG=1` additionally requires operator-assisted cable removal/reconnection during streaming (180-second deadlines per phase). The test verifies deletion of camera/guider properties, rediscovers the same devices and captures a new image plus guiding pulse after replug. All normal hardware runs now also verify complete driver unload/reload and a new exposure in the same process. The executable remains excluded from default tests.

Altair ALTAIRGP224C #E61F50 passed the native physical hot-plug run with 1280 × 960 RGB RAW payloads (3,686,412 bytes), then passed a separate native run including the newly added SDK unload/reload check. Both exited 0. No Rosetta run was performed. The earlier ToupTek run preceded the additional unload/reload check; no physical ToupTek cable-removal test is claimed.

Step 7 software verification: all 20 native SDK cases and all 86 native timer/queue cases passed. The three deterministic SDK race cases also passed under AddressSanitizer with instrumented driver/harness/framework dispatcher/image fixture; prebuilt libraries are not fully instrumented. All eleven branded drivers compiled and linked for arm64/x86_64. Full initialization-call comparison against the refactor baseline found no public schema changes. `git diff --check` passed; test and ASan artifacts were cleaned.

Physical wheel/focuser hardware, simultaneous multiple physical cameras, Linux/Windows toolchains and native Intel hardware remain unverified. Hardware ST4 electrical pulse duration is not measured.

## Goals

- Provide repeatable tests that run without physical astronomy hardware.
- Keep the harness dependency-free and compatible with the existing make build.
- Separate fast unit tests from simulator and bus-level integration tests.
- Validate driver lifecycle, property enumeration, property changes, and protocol adapters through public APIs.
- Preserve manual hardware validation in `TESTING.md` for real devices and vendor SDK behavior.

## Current Layout

```text
indigo_test/
  CHANGES.md
  Makefile
  test_runner.h
  fixtures/
    protocol/
  unit/
  integration/
  benchmark/
```

Unit tests live in `indigo_test/unit/`. Integration tests live in `indigo_test/integration/`. Protocol parser fixtures live in `indigo_test/fixtures/protocol/`. Timing benchmarks live in `indigo_test/benchmark/`.

## Build Targets

- `make -C indigo_test test` runs unit and integration tests.
- `make -C indigo_test test-unit` runs pure unit tests.
- `make -C indigo_test test-integration` runs bus and simulator-driver integration tests.
- `make -C indigo_test benchmark` builds and runs the timing benchmarks.
- `make -C indigo_test test-clean` removes generated test binaries and dSYM files.

Run `make all` from the repository root first if `build/lib/libindigo` or the required simulator driver archives are missing.

## Benchmarks

`indigo_test/benchmark/` holds measurement tools rather than tests. They report numbers, never assert, and always exit `0`, so they are deliberately excluded from the `test` target: results depend on machine load and on kernel timer behavior, and are meaningful only when compared against another run on the same machine.

All benchmarks report min, mean, median, p95, p99, max, and standard deviation, discard warm-up samples, and repeat the whole scenario set several times so that one-time costs and outliers stay visible. Timings come from a monotonic clock read inside the benchmark, so they are independent of the clock the library uses internally. Because they call only long-stable API, the same sources can be built against a second `libindigo` to compare two implementations on one machine.

`benchmark/bench_timer.c` measures per-fire latency and jitter of `indigo_set_timer()` and `indigo_reschedule_timer()`: a one-shot timer, a zero-delay timer that isolates dispatch cost, a self-rescheduling 10 ms timer, the interval between its fires, and a burst of timers that all come due at the same instant.

Comparison results are written up beside the sources. `benchmark/TIMERS_AND_QUEUES_MERGE_REVIEW.md` records the `master` against `refactoring` measurements for both benchmarks, the correctness differences behind them, and the open items before that branch merges.

`benchmark/bench_queue.c` measures the same properties for handler queues: the cost of `indigo_queue_add()` against a backlog of pending tasks, dispatch latency on an idle queue, lateness of a delayed task, a task that re-adds itself every 10 ms with the interval between its fires, and the per-task cost of draining a batch of ready tasks. The drain is measured both on an otherwise idle library and while another thread continuously creates and cancels timers, which exposes any lock shared between the timer and queue subsystems.

## Test Harness

`indigo_test/test_runner.h` provides a small dependency-free C test runner with local assertion macros. Each test executable returns `0` on success, returns non-zero on failure, and prints the failing test and assertion location.

## Unit Coverage

Implemented unit suites:

- `test_align_math.c`: parallactic angle against IAU SOFA/ERFA reference values and its meridian behavior, and the derotation rate against the measured rate of change of the parallactic angle (the quantity the mount agent derotates with), including reference values, the zero crossings due east and west, and the growth towards the zenith.
- `test_aux_math.c`: dewpoint and Bortle-scale helper behavior.
- `test_base64.c`: known vectors, binary round trips, padding, and newline-tolerant decoding.
- `test_bus_helpers.c`: numeric/string conversion, sexagesimal conversion, pixel scale, local service trimming, switch helpers, and property value/target copying.
- `test_bus_property.c`: text, number, switch, light, and BLOB property initialization, matching, copying, resizing, and release behavior, with every test case exercising all vector types.
- `test_dome_azimuth.c`: hour wrapping, azimuth distance, dome azimuth range checks, mirror symmetry of the pivot offsets across the equator, agreement with an independent vector model of the same geometry over a latitude, hour angle, declination and OTA offset sweep, and side of pier handling (the OTA staying on the reported side, no azimuth jump when a mount tracks past the meridian, agreement with counterweight down for a normal mount).
- `test_md5.c`: known MD5 vectors, partial MD5, and file-prefix MD5.
- `test_polynomial_fit.c`: polynomial value, derivative, extrema, minimum search, string output, and exact line fitting.
- `test_protocol_json.c`: JSON escaping, device/server adapter serialization, parser routing for number and switch changes, BLOB URL output, and malformed input handling.
- `test_protocol_xml.c`: XML escaping, device and client adapter serialization, parser routing for text changes and BLOB URL mode, remote property events, and malformed input handling.
- `test_raw_image.c`: RAW type constants, Bayer extension detection, Bayer channel equalization, saturation masks, and contrast.
- `test_timer.c`: timer delay conversion, callback execution, data callbacks, mutex-wrapped callbacks, cancel/reschedule behavior including data-to-plain callback transitions, device-wide timer cancellation, fork-time scheduler restart and inherited-timer cleanup while a callback is active, signed queue priority ordering, queue handler run-time limit adjustment, queue pending-task limit configuration/reporting, queue removal, and queue deletion.
- `test_token.c`: token parsing, device token add/update/remove, and master-token fallback.

## Integration Coverage

Implemented bus and simulator integration suites:

- `test_bus_lifecycle.c`: `indigo_start()` / `indigo_stop()`, client and device attach/detach, enumeration, property definition/update/delete delivery, change routing, and invalid lifecycle calls.
- `test_ccd_simulator.c`: CCD simulator driver metadata, imager device lifecycle, full visible/hidden imager property sets, connection/disconnection, short exposure checks, and compliance checks for exposed imager, wheel, focuser, guider camera, guider, AO, Bahtinov camera, DSLR, and file-camera devices.
- `test_dome_baader_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the Baader Classic Dome driver.
- `test_dome_beaver_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the NexDome Beaver driver, including azimuth motion, shutter, park/home, calibration, abort, and Beaver safety/failure properties.
- `test_dome_nexdome_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the NexDome driver, including shutter, azimuth motion, abort, reverse-direction, and NexDome status/control properties.
- `test_dome_nexdome3_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the NexDome 3 firmware driver, including shutter, azimuth motion, abort, slaving threshold, and NexDome 3 status/settings properties.
- `test_dome_skyroof_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the Interactive Astronomy SkyRoof driver, including shutter, abort, and heater control.
- `test_dome_talon6ror_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the Talon6 ROR driver, including roof open/close/abort handling and Talon6 sensor/configuration properties.
- `test_dome_simulator.c`: dome simulator metadata, lifecycle, property enumeration, connection/disconnection, visible and hidden properties, dome property item/range checks, shutter/slaving/park commands, azimuth motion, and abort handling.
- `test_gps_simulator.c`: GPS simulator metadata, lifecycle, property enumeration, connection/disconnection, GPS property item/range checks, status lights, and advanced-status coverage.
- `test_gps_nmea_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free NMEA 0183 stream parsing for the Generic NMEA GPS driver, including selected positioning system, coordinates, UTC time, 3D fix status, satellite counts, and advanced DOP/status updates.
- `test_mount_ioptron_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, one configurable iOptron protocol simulator covering HC 8406, HC 8407, protocol 1.0, 2.0, 2.5, and 3.0 dialect connection paths, plus protocol 3.0 mount and guider property/action coverage.
- `test_mount_lx200_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, configurable LX200/OnStep protocol simulator coverage for the LX200 mount, guider, focuser, and OnStep AUX logical devices.
- `test_mount_nexstar_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, configurable NexStar protocol simulator coverage for both Celestron and Sky-Watcher dialect connection paths, plus Celestron mount and guider property/action coverage.
- `test_mount_nexstaraux_simulator.c`: external TCP simulator launch, ready-file discovery using a `nexstar://` loopback URL, and hardware-free NexStar AUX binary protocol coverage for mount and guider logical devices.
- `test_mount_pmc8_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, optional loopback TCP/UDP simulator endpoints, and hardware-free PMC-Eight protocol coverage for mount and guider logical devices, including disconnected connection-mode changes, serial/TCP/UDP mount connection paths, disconnected default `MOUNT_TYPE=AUTO`, `MOUNT_TYPE=AUTO` autodetection, manual mount-type override, basic mount operations, abort during coordinate tracking, manual RA/DEC motion, and guider pulses on both axes.
- `test_mount_rainbow_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, and hardware-free RainbowAstro serial protocol coverage for mount lifecycle, identity, guide-rate, tracking, track-rate, coordinate, and abort paths.
- `test_mount_simulator.c`: mount simulator metadata, main mount and guider-device lifecycle, property enumeration, connection/disconnection, mount compliance checks, and guider compliance checks.
- `test_mount_starbook_simulator.c`: external loopback HTTP simulator launch, ready-file discovery, and hardware-free Vixen StarBook protocol coverage for mount and guider logical devices.
- `test_mount_temma_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, and hardware-free Takahashi Temma serial protocol coverage for mount and guider logical devices.
- `test_polaralign_simulator.c`: polar aligner simulator metadata, lifecycle, property enumeration, connection/disconnection, property item/range checks, direction commands, offset no-op handling, reset commands, and abort command handling.
- `test_rotator_simulator.c`: rotator simulator metadata, lifecycle, property enumeration, connection/disconnection, direction reversal position mapping, shortest-path movement across 0/360, and rotator compliance checks.
- `test_rotator_optec_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, and hardware-free Optec Pyxis rotator serial protocol coverage for direction, absolute position, home, rate, and relative rotate controls.
- `test_rotator_wa_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, and hardware-free WandererAstro rotator serial protocol coverage for direction, backlash, absolute/sync/relative position, zero position, and abort controls.
- `test_focuser_astromechanics_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the ASTROMECHANICS focuser driver, including position/aperture property coverage and the aperture command path.
- `test_focuser_askar_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, class property enumeration completeness, and compliance checks for the Askar-WAF focuser driver.
- `test_focuser_efa_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the Celestron / PlaneWave EFA focuser driver, including position sync/goto, relative steps, fan control, temperature, and abort.
- `test_focuser_dmfc_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the PegasusAstro DMFC focuser driver, including speed, backlash, reverse motion, motor type, encoder/LED controls, position sync/goto, relative steps, temperature, and abort.
- `test_focuser_dsd_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the Deep Sky Dad AF focuser driver, including AF2 speed, backlash, reverse motion, step mode, coils mode, current/timing controls, temperature, position sync/goto, relative steps, and abort.
- `test_focuser_focusdreampro_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the AstroGadget FocusDreamPro focuser driver, including speed, duty cycle, position sync/goto, relative steps, temperature, and abort.
- `test_focuser_ioptron_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the iOptron iEAF focuser driver, including status polling, reverse motion, absolute and relative moves, abort, zero sync, and temperature.
- `test_rotator_falcon2_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, class property enumeration completeness, and compliance checks for the Falcon2 rotator driver, including that a goto completes only once the rotator is idle and that a goto to the current position finishes instead of staying busy.
- `test_wheel_xagyl_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, wheel property enumeration completeness, and compliance checks for the Xagyl filter wheel driver.
- `test_wheel_indigo_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, wheel property enumeration completeness, and compliance checks for the Pegasus Indigo filter wheel driver.
- `test_wheel_quantum_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, wheel property enumeration completeness, and compliance checks for the Brightstar Quantum filter wheel driver.
- `test_wheel_trutek_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, wheel property enumeration completeness, and compliance checks for the Trutek filter wheel driver.
- `test_wheel_qhy_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, wheel property enumeration completeness, and compliance checks for the QHY CFW1, CFW2, and CFW3 filter wheel driver modes.
- `test_wheel_optec_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, wheel property enumeration completeness, and compliance checks for the Optec filter wheel driver.
- `test_focuser_fc3_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, class property enumeration completeness, and compliance checks for the FocusCube 3 focuser driver.
- `test_focuser_qhy_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, class property enumeration completeness, and compliance checks for the QHY Q-Focuser driver, including mode/compensation settings, position sync/goto, relative moves, speed, reverse motion, temperature, limits, and abort.
- `test_focuser_optecfl_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, class property enumeration completeness, and compliance checks for both Optec FocusLynx focuser channels, including focuser type, reverse motion, position sync/goto, relative moves, temperature, and abort.
- `test_focuser_lacerta_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, class property enumeration completeness, and compliance checks for the LACERTA Motorfocus focuser driver.
- `test_focuser_lakeside_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the LakesideAstro focuser driver, including temperature polling, relative moves, abort, backlash, mode, compensation settings, and active slope.
- `test_focuser_mjkzz_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and binary-frame serial compliance checks for the MJKZZ Rail focuser driver, including speed, absolute position, relative movement, and abort.
- `test_focuser_moonlite_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the MoonLite focuser driver, including speed, stepping mode, compensation, mode, temperature polling, absolute and relative moves, and abort.
- `test_focuser_nfocus_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the Rigel Systems nFOCUS focuser driver, including speed, temperature, relative moves, and abort.
- `test_focuser_nstep_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the Rigel Systems nSTEP focuser driver, including speed, stepping mode, phase wiring, backlash, compensation mode, temperature, relative moves with position polling, and abort.
- `test_focuser_optec_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the Optec TCF-S focuser driver, including manual/automatic mode changes, compensation, reverse-motion state, temperature, and relative moves with position polling.
- `test_focuser_primaluce_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free JSON serial driver connection, and per-device compliance for both PrimaLuceLab logical devices — the focuser device (configuration/state/WiFi/LED/preset/hold-current controls, backlash, speed, absolute and relative moves, temperature, and abort) and the connection-sharing rotator device (property coverage and abort).
- `test_focuser_prodigy_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and per-device compliance for both PegasusAstro Prodigy logical devices — the focuser device (speed, backlash, park, temperature, position sync/goto, relative moves, and abort) and the powerbox device (power outlets, USB ports, outlet naming, and reboot).
- `test_focuser_robofocus_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free fixed-frame serial driver connection, and serial compliance checks for the RoboFocus driver, including power channels, configuration/backlash, temperature, limits, absolute and relative moves, reverse motion, and abort.
- `test_focuser_steeldrive2_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free SteelDrive II serial driver connection with CRC responses, and per-device compliance for both logical devices — the focuser device (saved values, temperature-compensation controls, end-stop/zeroing controls, position sync/goto, relative moves, temperature, and abort) and the connection-sharing AUX heater device (heater output, auto-dew/PID controls, PID settings, and sensor selection).
- `test_focuser_usbv3_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free compact-command serial driver connection, and serial compliance checks for the USB_Focus v3 driver, including step size, compensation, mode, speed, temperature, limits, absolute and relative moves, and abort.
- `test_focuser_wemacro_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free binary-frame serial driver connection, and serial compliance checks for the WeMacro Rail driver, including rail configuration, shutter fire, relative movement, batch execution, speed, reverse motion, and abort.
- `test_aux_svbpowerbox_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, class property enumeration completeness, and compliance checks for the SVBONY PowerBox AUX driver.
- `test_aux_wbplusv3_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, class property enumeration completeness, and compliance checks for the WandererBox Plus V3 AUX driver.
- `test_aux_wbprov3_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, class property enumeration completeness, and compliance checks for the WandererBox Pro V3 AUX driver.
- `test_aux_wcv4ec_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, lightbox property enumeration completeness, and compliance checks for the WandererCover V4-EC AUX lightbox driver.
- `test_aux_arteskyflat_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, lightbox property enumeration completeness, and compliance checks for the Artesky Flat Box AUX lightbox driver.
- `test_aux_astromechanics_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, SQM weather property enumeration completeness, and timer-driven `V#` reading compliance for the ASTROMECHANICS LPM AUX driver.
- `test_aux_fbc_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection through the `: I #`/`: P #`/`: V #` handshake, lightbox property enumeration completeness, and light-intensity change compliance for the Lacerta FBC AUX driver.
- `test_aux_flatmaster_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection through the `#`/`V` handshake, lightbox property enumeration completeness, and light switch and intensity change compliance for the Pegasus Astro FlatMaster AUX driver.
- `test_aux_flipflat_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection through the `>POOO`/`>VOOO` handshake, lightbox and cover property enumeration completeness, and light, intensity, and timed cover-open (BUSY-to-OK via `>SOOO` status polling) compliance for the Optec/Alnitak Flip-Flat AUX driver.
- `test_ao_sx_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection through the character-framed `X`/`V` handshake, and per-device compliance for both logical devices of the StarlightXpress AO driver — the AO device (tip/tilt pulse and reset) and the connection-sharing guider device (guider pulse).
- `test_mount_synscan_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and per-device compliance for the SynScan EQ8 driver's logical devices — the mount device (property completeness including `MOUNT_STATE`, representative changes, model-code identification, aux-encoder coordinate sourcing, capability-gated autohome property visibility, park completion after initialized axis status replies, coordinate updates and coordinate BUSY states during slew/park/home/autohome, home-state busy/completion after home/autohome, tracking stop after home, serial-loss disconnect, and tracking/state publication after coordinate slews that request tracking), the connection-sharing guider device (guide pulse), and the connection-sharing AUX shutter device (snap-port exposure and abort).
- `test_aux_upb3_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and per-device compliance for both logical devices of the Ultimate Powerbox 3 driver — the AUX device (power, USB, heater, dew control) and the connection-sharing focuser device (position move).
- `test_aux_ppb_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and per-model compliance for all three PegasusAstro Pocket Powerbox variants — PPB (power outlets, heater outlets, dew control), PPBA (same plus DSLR power selection), and SPB (single power outlet, heater outlets, dew control) — using the `--model ppb|ppba|spb` flag.
- `test_aux_skyalert_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection through the `\r`-terminated `send` command, and property enumeration completeness for the Interactive Astronomy SkyAlert AUX weather driver.
- `test_aux_sqm_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection through the `x`-terminated `ix` handshake, timer-driven `rx` reading compliance, and property enumeration completeness for the Unihedron SQM AUX driver.
- `test_aux_uch_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection through the `P#`/`PV`/`PL:1`/`PA` handshake, USB port toggle compliance, and property enumeration completeness for the PegasusAstro USB Control Hub AUX driver.
- `test_aux_usbdp_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection through the `SWHOIS`/`SGETAL` handshake (6-byte fixed-length commands, `\n`-terminated responses), and per-version compliance for both USB Dewpoint variants — V2 (three controllable heater outlets, dew control mode, calibration, thresholds, channel linking, aggressivity, weather, and dual temperature sensors) and V1 (weather and single temperature sensor only) — selected via `--model v1|v2`.

`integration/simulator_test_common.h` provides the shared in-process client, property cache, lifecycle helpers, and compliance-style assertions used by simulator tests.

## Compliance Model

The simulator compliance checks are derived from the shell-based routines in `indigo_tests/`. They verify interface bitmasks, mandatory class properties and items, numeric ranges, connection/disconnection paths, representative state transitions, and restoration of changed values where practical.

Coverage currently includes:

- GPS compliance on the GPS simulator.
- Mount compliance on the mount simulator.
- Guider compliance on the mount simulator guider and CCD simulator guider.
- Rotator compliance on the rotator simulator.
- Wheel, focuser, guider, and AO compliance through devices exposed by the CCD simulator.
- CCD camera compliance through the CCD simulator's imager, guider camera, Bahtinov camera, DSLR, and file-camera devices.
- Dome compliance on the dome simulator.
- Polar-aligner compliance on the polar-aligner simulator.
- External serial simulator compliance on Baader Classic Dome, NexDome Beaver, NexDome, NexDome 3, Interactive Astronomy SkyRoof, Talon6 ROR, Generic NMEA 0183 GPS, iOptron mount/guider, ASTROMECHANICS focuser, Askar-WAF focuser, Celestron / PlaneWave EFA focuser, AstroGadget FocusDreamPro focuser, Falcon2 rotator, Optec Pyxis rotator, WandererAstro rotator, Xagyl filter wheel, Pegasus Indigo filter wheel, Brightstar Quantum filter wheel, Trutek filter wheel, QHY CFW1/CFW2/CFW3 filter wheel modes, Optec filter wheel, FocusCube 3 focuser, QHY Q-Focuser, Optec FocusLynx focuser, Optec TCF-S focuser, PrimaLuceLab focuser/rotator, PegasusAstro Prodigy focuser/powerbox, RoboFocus focuser, SteelDrive II focuser/AUX, USB_Focus v3 focuser, WeMacro Rail focuser, LACERTA Motorfocus focuser, MoonLite focuser, myFocuserPro2 focuser, Rigel Systems nFOCUS focuser, Rigel Systems nSTEP focuser, SVBONY PowerBox AUX, WandererBox Plus V3 AUX, WandererBox Pro V3 AUX, WandererCover V4-EC AUX, Artesky Flat Box AUX, ASTROMECHANICS LPM AUX, Lacerta FBC AUX, Pegasus Astro FlatMaster AUX, Optec/Alnitak Flip-Flat AUX, StarlightXpress AO, SynScan EQ8 mount, Ultimate Powerbox 3 AUX, PegasusAstro Pocket Powerbox PPB/PPBA/SPB AUX, Interactive Astronomy SkyAlert AUX, Unihedron SQM AUX, PegasusAstro USB Control Hub AUX, and USB Dewpoint v1/v2 AUX drivers through pseudo terminals exported with `INDIGO_SIMULATOR_PORT`.

Standalone simulator archives are not present for every device class in this checkout, so some class coverage intentionally uses multi-device simulator drivers.

## Current Behavior

- The suite is hardware-free.
- Tests do not launch `indigo_server` or open network sockets.
- Protocol tests use fixed fixtures and temporary files through `indigo_uni_io`.
- Integration tests exercise public driver entry points and bus APIs; they do not include production `.c` files directly.

## Verification

Last verified locally on macOS:

- `make -C indigo_test test-unit`
- `make -C indigo_test test-integration`
- `make -C indigo_test test`
- `make -C indigo_test test-clean`

All tests passed at the time this document was cleaned up.

## Deferred Work

- Add binary fixtures for `indigo_dslr_raw.c`, `indigo_fits.c`, `indigo_tiff.c`, `indigo_avi.c`, and `indigo_ser.c`.
- Add carefully sourced expected values for `indigocat` coordinate, precession, and time transforms.
- Add slower live `indigo_server` socket tests with process management, port allocation, and strict timeouts.
- Add driver-specific fake I/O tests for hardware drivers with complex parsing or command sequencing.
- Wire a root-level `make test` target once the suite is accepted as part of the normal project workflow.
- Update `README.md` and `TESTING.md` with the finalized automated test commands and their relationship to manual hardware testing.

## 2026-09-07 focuser_asi refactoring verification

- Added `integration/test_focuser_asi_sdk.c` to the integration suite. It compiles the generated driver separately, exercises the public driver entry point and bus APIs, and replaces SDK calls, USB events and hardware locks without linking the vendor SDK or accessing USB hardware.
- Six regression groups cover failed initialization cleanup and lock release; confirmed position/step limits and failed limit/backlash readback; termination of failed polling and SDK preflight before retry; abort switch reset and delayed stop confirmation; accepting compensation settings, retaining the temperature baseline and recovery after a transient move error; five devices plus retry after failed attach and capacity exhaustion.
- Narrow run: `make -C indigo_test build/integration/test_focuser_asi_sdk` then `indigo_test/build/integration/test_focuser_asi_sdk`. All six groups passed on macOS. Fresh x86_64/arm64 compilation and archive/dynamic-library/executable linking with the real SDK also passed; existing vendor deployment-target warnings remain.
- Real EAF firmware timing, physical USB enumeration order, hand-controller behavior and end-to-end configuration persistence still require hardware validation.

## 2026-09-07 — ASI EFW hot-plug regression

Added `integration/test_wheel_asi_sdk.c` for DRV-051. It compiles the production driver separately with SDK/USB stubs and exercises its public lifecycle. It verifies retry after a failed attach and after exhausting all five slots then freeing one, without reinitializing the driver. No vendor SDK or hardware is needed for this test. The regression fails on the original driver at failed-attach retry and passes with the fix.

Run `make -C indigo_test build/integration/test_wheel_asi_sdk` then `indigo_test/build/integration/test_wheel_asi_sdk`. Real USB enumeration and vendor SDK behavior still require hardware validation.

The ASI EFW SDK suite also covers connection, changing slot 1 to slot 3, and disconnection through the public property API. It checks SDK open/close calls, lock acquisition/release, conversion from INDIGO slot 3 to SDK index 2, and the slot property's BUSY-to-OK transition with the confirmed final value. Both integration cases passed with the SDK stubs.

## 2026-09-07 — ASI EAF functional scenarios

Extended `integration/test_focuser_asi_sdk.c` with four public-bus functional cases: connect/absolute move/disconnect, relative moves in both directions, position synchronization without an SDK move, and limits/backlash/reverse/beep settings with readback after reconnect. Stateful SDK stubs record requested positions and settings. The tests verify SDK open/close and lock release, motion BUSY-to-OK completion, and confirmed positions. Existing abort and compensation regressions remain in the same suite (ten cases total).

Run `make -C indigo_test build/integration/test_focuser_asi_sdk` followed by `indigo_test/build/integration/test_focuser_asi_sdk`. These tests require no hardware; physical motion and vendor SDK behavior are not covered.

## 2026-09-07 — ASI EFW SDK error regressions

The wheel SDK suite now has five cases. Added required initialization read failures and oversized SDK slot counts with close/lock-release checks; motion polling failure preserving the confirmed slot and stopping retries; calibration read failure/reset/retry; START=false completion; and interrupted calibration followed by reconnect while the wheel is still moving. Existing normal motion and attach/capacity retry cases remain. All five cases passed, and production universal compilation/linking passed separately.

## 2026-09-07 — ASI CAA hardware-free regression (rotator refactoring Step 8)

Added `integration/test_rotator_asi_sdk.c` and registered `test_rotator_asi_sdk` in `INTEGRATION_TESTS`, including the standard `test-integration` and `test` targets. As with EAF/EFW, the generated production C is compiled into a separate test object with local preprocessor replacements for USB discovery, attach/detach and global locks. Stateful CAA functions are supplied by the test executable, which links the real INDIGO library without the vendor CAA binary. No production source or generator changes are needed.

Each of the 11 named scenarios starts a fresh driver lifecycle and uses public driver/bus APIs, `test_runner.h` and `simulator_test_common.h`. Cleanup runs even after a failed assertion. Per-device counters detect SDK calls on closed handles, duplicate opens, unbalanced global locks and leaked USB references; successful SDK opens and closes must balance after every scenario.

Implemented coverage:

- Metadata/version, interface, custom schemas and connected-only visibility, hidden backlash, SDK version and repeated connect/disconnect.
- Failed SDK open and all four required connection reads (maximum, position, reverse, beep), followed by a successful retry; failed initialization must leave CONNECTION alert/disconnected and release handles/locks.
- Fractional absolute and signed relative motion, separate current value/target, overlapping requests, computed relative targets outside effective limits, and continued BUSY while the SDK motor flag remains active even at the target angle.
- Failed move start, status/position polling failures, preservation of confirmed position, termination of failed polling and safe retry while the motor remains active.
- Sync command failure, failure of readback after successful sync, and successful sync without a move command.
- Failed stop, delayed motor-stop confirmation, abort switch reset and hand-controller movement that cannot be stopped through the SDK.
- Maximum writes and failed write/readback consistency, effective motion-limit rejection, reverse/beep writes and failures, and settings readback on reconnect.
- Beep persistence selection through CONFIG_SAVE. Test-local save/base-dispatch hooks observe the driver's save request and bypass base CONFIG handling to avoid writing user configuration; all other base property handling delegates to the real rotator base driver. This checks selection for persistence, not disk serialization.
- Empty/eight-byte suffixes, overlength rejection without an SDK write, failed-write rollback, and the resulting device name after replug.
- Disconnect/reconnect and connected unplug with pending movement polling; no further SDK polling after close. Wrong USB vendor/product, failed/invalid SDK IDs, failed open/property probes, failed attach/retry, duplicate arrival, default five-device capacity, slot reuse, and removal of one connected device while another stays open.
- Unplug queued behind a deliberately held SDK probe, using a bounded gate; normal driver shutdown and resource balance after every scenario.

Validation: `make -C indigo_test build/integration/test_rotator_asi_sdk` compiled the test and separate production object for x86_64/arm64. `indigo_test/build/integration/test_rotator_asi_sdk` passed all 11 scenarios on native macOS arm64. Expected SDK error logs are from injected failures. The final run includes suffix replug and motor-active-at-target checks. Dynamic dependencies contain INDIGO/libusb and system libraries, with no CAA vendor binary. `git diff --check` passed; `make -C indigo_test test-clean` removes the generated test artifacts.

Remaining coverage: forced overlap of an already executing device handler with disconnect, shutdown with queued/in-flight hot-plug work, USB callback registration/queue allocation failures, invalid startup product-count responses, and vendor timing/physical USB-to-SDK identity. The connection helper waits for the initial delayed SDK read before issuing ordinary command assertions; commands racing that initial read are not covered. No physical CAA, Linux execution, x86_64 execution or full-suite run was performed in this step. Hardware validation remains refactoring Step 9.

Player One follow-up validation (2026-09-08): all 13 cases also passed with `POA_SAFE_READOUT=1` in an isolated build tree and with arm64 AddressSanitizer/UndefinedBehaviorSanitizer. The test, driver and test-local framework objects were instrumented; bundled static dependencies were not. After finalizer naming changes, pulse error was idle mean +1.928 ms / max +5.060 ms and exposure mean +2.684 ms / max +5.054 ms. Physical streaming unplug/replug, reacquisition, guider command and subsequent unload/reload passed separately (TESTING.md); electrical ST4 output remains unmeasured.
