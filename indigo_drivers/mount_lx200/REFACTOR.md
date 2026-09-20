# LX200 simulator test coverage

## Scope and baseline

2026-09-15, macOS arm64. Test expansion followed by user-authorized repairs
to the handwritten driver, version `0x03000033` to `0x03000034`. No generator
or original driver-local simulator changes. The four original serial compliance cases passed before expansion.
The current suite contains 67 serial cases and three explicit opt-in TCP cases,
70 unique simulated cases in total; hardware cases: 0 run, 0 passed.
The retesting column records simulator validation for this repair;
this work supplies simulator evidence only. The test count is availability,
not a claim that all acceptance tests pass or every firmware is conformant.

## Completed work

1. Read driver lifecycle/property documentation, testing rules, source and bundled protocols; establish the four-case baseline.
2. Add a test-owned simulator with 13 profiles, shared monotonic serial_motion.h motion, exact command events, one-shot reply rejection/dropout and ready-file cleanup. Existing firmware simulator remains untouched.
3. Add public bus tests for model capabilities, transport commands/readback, BUSY/completion, errors/recovery, secondary devices, shared sessions and guide timing.
4. Register both new persistent files in Xcode, compile strict test/simulator builds and ASAN/UBSAN driver/test builds, run serial/TCP validation and record results below.

## Protocol audit and simulator contract

Bundled primary references were extracted locally with pypdf: Meade-2010.10.pdf,
Meade-LX200_Classic_Manual.pdf, 10micron-2.13.2.pdf, Gemini-5-2.1.pdf,
AstroPhysics-GTOCP4.pdf, OnStep.pdf, OnStepX.pdf, ZWO.pdf and NYX101.pdf.
Checked acceptance contracts include Sr/Sd one-byte replies, MS zero success,
CM terminated replies, signed declination, west-positive longitude, OnStep
GXY0 eight-slot discovery and GXYn name/purpose replies, SXX PWM acceptance,
Te one-byte reply, and NYX GRH/GDH precision and WiFi firmware encoding.
Gemini native command checksums and AP duration/rate commands are asserted.
The ZWO GT manual has inconsistent English/Chinese rate enums; its fixture
uses the driver's explicit numeric mapping. StarGO/StarGO2, TeenAstro, OAT
and aGotino lack independent bundled protocol references; those profiles are
source-contract fixtures, not independent manufacturer conformance evidence.

The simulator uses elapsed time for GOTO, manual movement, relative focus,
park and home movement; stop freezes the current pose and SYNC sets it.
Guide commands use real driver timers, while command receipt is timestamped
by the simulator. Physical guide pulse duration is not measured.
GW's third Meade status character describes alignment stars in the manual;
its inherited simulated parked flag is not evidence of real Meade parked
readback. Meade Autostar hP is a park slew and its D reply becomes empty on completion;
the repair uses that documented completion contract. Other Meade firmware
and physical parked status remain unverified.
Reference motion speeds and poses are deterministic test values, not hardware
performance models. Host-time acceptance checks allow asynchronous polling.
Each fixture uses its own PTY, ready/event/control files and driver session.
Fresh revision waits prevent a cached OK from satisfying a new write.

## Capability and scenario mapping

Names below omit the common `lx200_` prefix. Model matrix cases additionally
assert vendor/version metadata, property visibility/counts, exact SYNC with
RA 23.5 and DEC -0.5, J2000 readback, tracking and all supported slew/rate
commands, property deletion on disconnect and successful reconnection.

| Acceptance area | Cases and assertions |
| --- | --- |
| Discovery and model branches | `meade_profile`, `onstep_profile`, `10mic_profile`, `gemini_profile`, `stargo_profile`, `stargo2_profile`, `ap_profile`, `agotino_profile`, `zwo_profile`, `nyx_profile`, `oat_profile`, `teen_profile`, `generic_profile`; autodetection, AP/StarGO2 manual selection, classic fallback |
| Metadata and base properties | `driver_metadata_and_base_properties`; INIT idempotence, public property inventory, mount type 14 items, driver INFO version and shutdown status |
| GOTO/SYNC, progress and abort | `goto_progress_abort_and_restart`, `goto_overlap_replaces_target`, `generic_goto_polling_progress`; real progress, target replacement, abort freezing pose, restart and no-op abort; model cases assert CM versus MS |
| Manual movement | `manual_reversal_and_axis_stops`, `manual_abort_completes_both_axes`; both axes/directions, reversal, independent stop and abort property completion |
| Protocol acceptance and failures | `coordinate_command_failures_recover`; Sr/Sd/MS rejection, CM empty reply, missing Sr reply and subsequent recovery |
| Invalid device readback | `malformed_coordinates_preserve_readback`; malformed RA must not replace the last valid coordinate or silently succeed |
| Time and geography | `location_and_utc_translation`, `utc_write_failures_recover`; negative latitude, east longitude, signed UTC offsets/date rollover, host time, SC/SG/SL failures and retries |
| Tracking/rates | All model cases, `initial_tracking_rate_readback`, `stargo_guide_rate_units`; sidereal/solar/lunar branches, vendor-specific command/readback, StarGO RA/DEC guide-rate scaling |
| OnStep custom options | `onstep_options_and_partial_failures`; all four preferred pier sides, automatic flip on/off, fractional meridian limits, altitude limits, accepted values retained after partial failure, tracking failure/retry, park/home set, PEC commands |
| Park/unpark | `park_meade`, `park_10micron`, `park_gemini`, `park_stargo`, `park_ap`, `park_nyx`, `park_oat`, `park_teenastro`, `park_rejects_motion_and_unpark_recovers`; available command branches, completion and parked GOTO/manual/tracking guards |
| Home/reference set | `home_onstep`, `home_stargo`, `home_zwo`, `home_nyx`, `home_teenastro`, `home_single_item_completes`, `teenastro_set_positions`; completion from an away pose, single-item behavior and hQ/hB command assertions |
| ZWO options | `zwo_rates_and_buzzer`; three rate enums, all three buzzer settings and rejected write/retry |
| NYX options | `nyx_options_and_wifi_failures`, `nyx_legacy_wifi_and_elevation`; leveler readback, firmware 1.35 base64 and 1.31 raw WiFi credentials, AP/client/reset commands, accepted truncation limits, rejected writes/retry, elevation |
| Guider timing/concurrency | `guider_directions_overlap_and_timing`, `guider_ap_duration_commands`; four directions, three durations, idle/GOTO workloads, same-axis BUSY exclusion, concurrent axes, zero-on-completion, AP duration encoding |
| Guider lifecycle | `disconnect_cancels_pulses_and_reconnects`, `shared_connection_orders_and_last_close`, `guider_transport_failure_and_recovery`; pending pulse cancellation, no stale completion after reconnect, both master/slave connection orders and surviving sibling |
| Focuser | `focuser_meade_operations`, `focuser_onstep_operations`, `focuser_ap_operations`, `focuser_oat_operations`; both directions, speed/reverse, timed relative moves, abort/restart, zero and reconnect |
| AUX mapping and writes | `aux_slot_mapping_and_failure_recovery`, `aux_heater_conversion_and_rejected_write`, `aux_power_rejected_write_recovers`; mixed/nonadjacent slots, labels/index mapping, PWM 0/50/100 to 0/128/255, failed writes/recovery |
| NYX AUX | `nyx_aux_readback`; temperature 12.5, pressure 1013.25 and voltage 12.8 |
| Shared focuser/AUX | `shared_focuser_lifecycle`, `shared_aux_lifecycle`; both connection orders, sibling operations after master disconnect, last-close and reconnect |
| Initialization and loss | `initialization_rollback_and_reconnect`, `transport_loss_and_fresh_session`, `secondary_capability_rejection`; open/detection failures, supported missing-name fallback, rollback, new session after loss, unsupported secondary capability rejection |
| Existing compliance | `mount_passes_serial_compliance_checks`, `guider_passes_serial_compliance_checks`, `focuser_passes_serial_compliance_checks`, `aux_passes_serial_compliance_checks`; original bus lifecycle/transport acceptance |
| TCP (explicit opt-in) | `tcp_redundant_guider_connection_balances_close`, `tcp_mount_commands_and_reconnect`, `tcp_guider_keepalive_and_shared_ownership`; socket command/reconnect, guider-first keepalive, shared ownership and redundant-connect last-close |

Custom property inventory audited against declarations and handlers:
X_MOUNT_MODE, X_MOUNT_TYPE, X_ZWO_BUZZER, X_NYX_WIFI_AP, X_NYX_WIFI_CL,
X_NYX_WIFI_RESET, X_NYX_LEVELER, X_ONSTEP_PREFERRED_PIER_SIDE,
X_ONSTEP_AUTOMATIC_MERIDIAN_FLIP, X_ONSTEP_MERIDIAN_LIMITS,
X_ALTITUDE_LIMITS; AUX_WEATHER and AUX_INFO are standard properties.
Standard inherited properties retain their standard names. No properties were
added/removed, so PROPERTIES.md needs no change. Visibility/count mutations
are checked through model and secondary-device cases.

Absolute focuser position, hardware limits, calibration, encoder accuracy,
physical pier behavior and physical guiding are not simulated capabilities of
this driver suite. Framework numeric validation, generic inherited config
persistence and shared motion helper unit behavior are not counted as driver
coverage. Firmware combinations outside the named profiles, arbitrary packet
fragmentation/noise, exhaustive SDK/platform behavior and every transport
failure point remain unverified. No claim of line/branch coverage percentage
or complete real-device compatibility is made.

## Found defects: baseline reproductions and production repairs

The test-only baseline retained these failing acceptance tests without skips.
The user subsequently authorized their production repairs. Each repair
is marked FIXED in the table; its listed regression passed in both ordinary and
ASAN/UBSAN simulator runs on 2026-09-15 with driver version 0x03000034. Final
regression results are recorded at the end. Source line
references in this historical table refer to baseline version 0x03000033.

| ID | Status | Reproduction / source | Implemented fix and regression |
| --- | --- | --- | --- |
| LX001 | FIXED | `manual_abort_completes_both_axes`; lines 2500–2509 skip already-BUSY axis properties | Clear active axis switches and publish completion after successful Q; retain two-axis abort/restart test |
| LX002 | FIXED | `malformed_coordinates_preserve_readback`; lines 657–684 parse without payload validation | Validate device coordinate grammar before publishing; preserve last valid values and surface failed readback |
| LX003 | FIXED | `aux_slot_mapping_and_failure_recovery`; lines 3353–3363 overwrite the discovery bitmap with descriptor replies | Copy the bitmap before nested transactions; verify slots 1/3/4, no ghost slot 8, correct labels and writes |
| LX004 | FIXED | `onstep_options_and_partial_failures`; line 758 sends unframed `$QZ+`/`$QZ-` | Send documented colon/hash framing; assert both enable/disable commands |
| LX005 | FIXED | `initial_tracking_rate_readback`; lines 905–912 pass TRACK_RATE items to TRACKING | Select items on MOUNT_TRACK_RATE_PROPERTY; verify exactly one lunar selection from GT readback |
| LX006 | FIXED | `home_single_item_completes`, `home_stargo`, `home_teenastro`; lines 2432–2433 clear the request, while line 2073 requires it | Track pending homing independently of the momentary switch and publish final OK/ALERT |
| LX007 | FIXED | `park_meade`, `park_oat`; lines 2391–2392 clear the request, while line 2055 requires it | Complete single-item park from documented status/operation state; Meade uses D slew completion, OAT uses GX Parked; hardware remains unverified |
| LX008 | FIXED | `park_ap`; lines 2008–2032 reset parked state then generic polling loses KA state; unpark gate suppresses PO | Preserve valid AP park state or use supported status; ensure PO is sent after park |
| LX009 | FIXED | `park_teenastro`; line 2395 omits TeenAstro from asynchronous park waiting | Keep BUSY until GXI confirms completion; immediate subsequent unpark must issue hR |
| LX010 | FIXED | `tcp_redundant_guider_connection_balances_close`; lines 3096–3105 lack the connection-change ignore guard | Ignore redundant requests before incrementing shared references; exactly one CLOSE on last disconnect |
| LX011 | FIXED | Source audit: generic slew comparison used lastRA/lastDec after overwriting them (baseline lines 2005–2006) | Save the new pose after comparison with the preceding pose; `generic_goto_polling_progress` passed in ordinary and ASAN/UBSAN runs |
| LX012 | FIXED | Source audit: guider callbacks ignored transport command failure | Publish ALERT, clear pulse values and return without a success finalizer; `guider_transport_failure_and_recovery` passed for both axes, reconnect and retry in ordinary and ASAN/UBSAN runs |

LX011 and LX012 were not in the original 13 baseline failures. Driver version is
0x03000034 and test-owned simulator version is 3. Simulator SXX commands now
reach their dedicated stateful handler instead of the generic SX handler.
The malformed readback fixture waits for an actual first polling readback
before taking its preservation baseline. AUX readback avoids replacing a
switch write that became BUSY during its transport transaction.

## Commands and test-only baseline results

Build from repository root:

```sh
make -C indigo_test build/integration/test_mount_lx200_simulator build/integration/test_mount_lx200_simulator_sanitize
cd indigo_test
build/integration/test_mount_lx200_simulator
build/integration/test_mount_lx200_simulator_sanitize
```

An optional substring argument selects matching cases. TCP is never part of
the default serial suite; run `make -C indigo_test test-mount-lx200-tcp` from
the repository root when local sockets are allowed. It executes both ordinary
and sanitized tests and propagates failure from either run.

Test-only macOS arm64 baseline, before repairs (68 unique cases), full serial
run plus the separate TCP run:

| Build / transport | Run | Passed | Failed |
| --- | --- | --- | --- |
| Ordinary serial | 65 | 53 | 12 |
| ASAN/UBSAN serial | 65 | 53 | 12 |
| Ordinary TCP | 3 | 2 | 1 |
| ASAN/UBSAN TCP | 3 | 2 | 1 |
| Unique scenarios per build | 68 | 55 | 13 |

The 13 failures reproduce the ten defect groups above. Both serial
executables returned 1, and the TCP make target returned nonzero as intended.
Across both builds, 136 cases ran, 110 passed and 26 failed. All 13 model
matrix cases and all four original compliance cases passed. No failures are
skipped, suppressed or counted as successes. Hardware: 0 run, 0 passed.

Guiding completion error in the test-only baseline run (milliseconds, 48 samples each):

| Build | Min | Mean | Median | p95 | p99 / max | Stddev | Max absolute | Mean signed % | Max absolute % |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Ordinary | 55.265 | 75.714 | 58.159 | 167.558 | 170.625 | 40.142 | 170.625 | 130.933 | 300.145 |
| ASAN/UBSAN | 54.198 | 76.134 | 57.863 | 172.446 | 173.023 | 41.755 | 173.023 | 129.519 | 300.345 |

Both guide timing/concurrency cases passed. These are observations on this
host while the ordinary and sanitized suites ran concurrently; they are not
portable performance acceptance limits.
TCP: each build ran three cases, two passed and LX010 failed.
Strict -Wall/-Wextra/-Werror test/simulator compilation passed with existing
shared-helper unused/sign warnings excluded. ASAN/UBSAN instrument the test
and production driver source; linked framework libraries and the external
simulator are not instrumented. No sanitizer memory/undefined-behavior
report was observed in completed runs. Existing deprecated sprintf warnings
in production source remain. TSAN was not run; race freedom is not claimed.
Representative macOS x86_64/Rosetta checks passed metadata, protocol failure
recovery and shared guider lifecycle; sanitized x86_64 protocol recovery
also passed. These are representative checks, not the whole x86_64 matrix.
Linux and Windows execution are unavailable and unverified.

Guide timing uses native monotonic command-receipt time in the simulator and
public property-OK callback time in the test. macOS uses
clock_gettime_nsec_np(CLOCK_MONOTONIC) to avoid the linked framework clock
shim; timing tests require macOS 10.12 or later. Durations 20/100/500 ms in
four directions, two retained repetitions, with discarded warmup per
direction/workload, produce 48 retained samples across idle and actual
mount GOTO/polling workloads. Statistics report signed completion error,
absolute maximum, percentage error, mean/median, population standard
deviation and nearest-rank p95/p99. The five-second wait is a liveness bound,
not a hardware pulse-accuracy threshold. Host scheduling and transaction
latency are included; results do not establish physical pulse accuracy.

Test-only baseline git diff --check and Xcode plist validation passed. test-clean completed;
process inventory confirmed no LX200 test or simulator processes remained.


## Production repair plan (2026-09-15)

User authorized repair and retest after the test-only baseline: ordinary and
ASAN/UBSAN 68 cases each, 55 passed / 13 failed. Existing driver archive and
both baseline test builds are available. No hardware testing will be performed.
Handwritten portable driver; no generation or new persistent files required.
Shared queues own serialized transport and lifecycle; polling owns HOME/PARK
completion. Preserve unrelated ongoing iOptron work and all README files.

1. COMPLETE: LX001–LX005 repairs compiled; sanitized manual abort, malformed readback, AUX slot, initial tracking and PEC regressions passed. Version raised to 0x03000034. LX010 guard implemented; TCP regression subsequently passed in both builds in step 3. Fixed a simulator SXX dispatch shadow (version 3) and waited for the first complete polling readback in the malformed fixture. AUX polling now avoids overwriting a pending switch write.
2. COMPLETE: all six sanitized HOME and nine PARK cases passed. Momentary properties complete from pending BUSY operation plus device status; TeenAstro waits for GXI; AP retains KA/PO command-owned park state. Meade Autostar hP slew completion uses documented D empty-bar reply; no GW parked flag is used. Physical completion on other Meade firmware remains unverified.
3. COMPLETE: ordinary and ASAN/UBSAN serial 67/67 each, TCP 3/3 each; representative repaired x86_64 checks and version assertions passed. Strict universal build/check passed. Final status/results recorded below; Linux/Windows unavailable. Cleanup checked below.

Source-audit follow-up: generic polling now compares with the preceding pose,
then saves the new pose; guider write failure publishes ALERT with cleared pulse
values and no success finalizer. New driver-specific regression cases verify
generic GOTO progress and both guide axes failing/recovering after transport loss.


## Repair verification (version 0x03000034)

Actual build commands from the repository root:

```sh
make -C indigo_drivers/mount_lx200 -f ../../Makefile.drv
make -C indigo_test build/integration/test_mount_lx200_simulator build/integration/test_mount_lx200_simulator_sanitize
clang -fsyntax-only -arch x86_64 -arch arm64 -DINDIGO_MACOS -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-function -Wno-sign-compare -Wno-deprecated-declarations -Iindigo_libs indigo_drivers/mount_lx200/indigo_mount_lx200.c
```

Production universal build, strict universal syntax/warnings check, and both
test builds passed. Existing unused/sign/deprecated warnings are excluded
from the strict check; the regular build retains the deprecated sprintf
warnings. No warnings introduced by these repairs were observed.
The metadata assertion now requires version 0x03000034 and passed in both
ordinary and sanitized builds after rebuilding that assertion.
Representative repaired x86_64/Rosetta tests passed single-item HOME,
sanitized mixed AUX mapping and sanitized guider loss/recovery.
Linux/Windows execution remains unavailable. No physical hardware was used.

The repair does not migrate the handwritten driver or redesign its existing
blocking focuser waits. Exhaustive firmware/platform behavior, physical
motion and physical guide pulse duration remain outside this validation.
ASAN/UBSAN cover driver/test source, not the linked framework libraries or
external simulator. TSAN was not run.

Final macOS arm64 repair acceptance matrix:

| Build / transport | Run | Passed | Failed |
| --- | --- | --- | --- |
| Ordinary serial | 67 | 67 | 0 |
| ASAN/UBSAN serial | 67 | 67 | 0 |
| Ordinary TCP | 3 | 3 | 0 |
| ASAN/UBSAN TCP | 3 | 3 | 0 |
| Unique scenarios per build | 70 | 70 | 0 |

All ten baseline defect groups and both source-audit follow-ups are fixed and
covered by passing regressions. Both serial executables and the TCP make
target returned 0. No cases were skipped or accepted as expected failures.
No ASAN/UBSAN memory or undefined-behavior diagnostic appeared.

Repair-run guide completion error, milliseconds (48 samples per build):

| Build | Min | Mean | Median | p95 | p99 / max | Stddev | Max absolute | Mean signed % | Max absolute % |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Ordinary | 52.985 | 76.177 | 58.190 | 173.045 | 177.782 | 42.417 | 177.782 | 130.273 | 300.135 |
| ASAN/UBSAN | 52.193 | 76.620 | 57.923 | 176.061 | 176.752 | 43.429 | 176.752 | 129.273 | 300.250 |

Requested durations, warmup, endpoints and GOTO/idle workloads are the same
as defined above. Both timing cases passed the finite completion/concurrency
checks. These are software completion timing observations on this host while
both suites ran concurrently; physical relay output was not measured.

Final simulated acceptance tests: 140 run, 140 passed (70 unique cases in
each of ordinary and ASAN/UBSAN builds). Narrow regressions and representative
x86_64 runs were additional passing checks, separate from this matrix.
Final hardware tests: 0 run, 0 passed.

Final git diff --check and Xcode plist validation passed; test-clean completed.
Process inventory confirmed no LX200 test/simulator remained. Existing unrelated
iOptron changes were preserved.

---

# LX200 generator migration (2026-09-20)

## Scope

Migrate the hand-written `indigo_mount_lx200.c` to an `indigo_generator`
`.driver` definition, keeping the existing test-owned simulator and the
70-case acceptance suite as the compatibility contract. macOS arm64,
branch `refactoring`, driver version `0x03000035` before this work.
Generator is used unchanged; no generator source is touched.

## Current-state audit

### Architecture and exposed devices

Single serial/TCP transport shared by four logical devices attached from one
`indigo_mount_lx200()` entry point with one `lx200_private_data`:

| Logical device | Name macro | Interface | Role |
| --- | --- | --- | --- |
| mount | `MOUNT_LX200_NAME` | `INDIGO_INTERFACE_MOUNT` | master, owns `DEVICE_PORT`/`DEVICE_PORTS`/`DEVICE_BAUDRATE`, polling timer |
| guider | `MOUNT_LX200_GUIDER_NAME` | `INDIGO_INTERFACE_GUIDER` | slave, pulse guiding |
| focuser | `MOUNT_LX200_FOCUSER_NAME` | `INDIGO_INTERFACE_FOCUSER` | slave, relative focus, Meade/AP/OnStep/OAT only |
| aux | `MOUNT_LX200_AUX_NAME` | `INDIGO_INTERFACE_AUX_POWERBOX \| INDIGO_INTERFACE_AUX_WEATHER` | slave, NYX weather/voltage or OnStep aux slots |

`mount_attach` sets `master_device` for all four; `device_count` is the shared
open/close reference count. `meade_open()`/`meade_close()` are the low-level
connection helpers; `meade_close()` additionally resets `device_count` to 0,
which the hand-written `--device_count <= 0` disconnect tests compensate for.

### Transport and protocol

`indigo_uni_io` serial (`indigo_uni_open_serial_with_config`, autobaud retry
over 9600/19200/115200) or `lx200://` URL (TCP 9999 for NYX/OnStep, 4030
otherwise, `TCP_NODELAY`, 5 s `keep_alive_callback` ping while the master is
not connected). Three command helpers: `meade_no_reply_command`,
`meade_simple_reply_command` (one byte, plus the two-section `:SC` progress
drain), `meade_command` (`#`-delimited section). All discard pending input
first and sleep 50 ms after the transaction. `meade_validate_handle()` closes
the handle and disconnects all slave devices when the handle dies.

Fourteen `X_MOUNT_TYPE` branches (autodetect plus Meade, 10Micron, Losmandy
Gemini, Avalon StarGO/StarGO2, Astro-Physics, OnStep, aGotino, ZWO AM,
Pegasus NYX, OpenAstroTech, TeenAstro, Generic), each with its own
`meade_init_*_mount()` capability/visibility setup and `meade_update_*_state()`
status decoding. Detection uses `:GVP#` with a classic-LX200 fallback probe.

### Public properties

Driver-defined (all `X_`-prefixed except the two standard AUX ones):
`X_MOUNT_TYPE` (always defined, persisted through `CONFIG`, `RW` while
disconnected and `RO` while connected), `X_MOUNT_MODE`, `X_ZWO_BUZZER`,
`X_NYX_WIFI_AP`, `X_NYX_WIFI_CL`, `X_NYX_WIFI_RESET`, `X_NYX_LEVELER`,
`X_ONSTEP_PREFERRED_PIER_SIDE`, `X_ONSTEP_AUTOMATIC_MERIDIAN_FLIP`,
`X_ONSTEP_MERIDIAN_LIMITS`, `X_ALTITUDE_LIMITS`, plus `AUX_WEATHER`,
`AUX_INFO`, `AUX_HEATER_OUTLET`, `AUX_POWER_OUTLET` on the aux device.
Inherited mount properties with driver change handlers: `MOUNT_PARK`,
`MOUNT_PARK_SET`, `MOUNT_HOME`, `MOUNT_HOME_SET`,
`MOUNT_GEOGRAPHIC_COORDINATES`, `MOUNT_EQUATORIAL_COORDINATES`,
`MOUNT_ABORT_MOTION`, `MOUNT_MOTION_RA`, `MOUNT_MOTION_DEC`,
`MOUNT_SET_HOST_TIME`, `MOUNT_UTC_TIME`, `MOUNT_TRACKING`,
`MOUNT_TRACK_RATE`, `MOUNT_PEC`, `MOUNT_GUIDE_RATE`. No property is added or
removed by this migration, so `indigo_docs/PROPERTIES.md` needs no change.

### Blocking, concurrency and lifecycle

Per-device handler queues serialize transport. `position_timer_callback`
reschedules itself at 0.5 s while `MOUNT_EQUATORIAL_COORDINATES` is BUSY and
1 s otherwise. `nyx_aux_timer_callback` (10 s) and `onstep_aux_timer_callback`
(2 s) are two separate aux polling callbacks selected by mount type.
`meade_focus_rel()` blocks the focuser queue for the whole relative move
(1 ms per step for Meade/AP/OAT, `:FT#` polling for OnStep) and is cancelled
through the `focus_aborted` flag, which is why `FOCUSER_ABORT_MOTION` is
dispatched synchronously from `focuser_change_property` instead of being
queued. Guider pulses use `indigo_execute_priority_handler_in` completion
callbacks named `*_finish_callback`.

### Known defects, risks

All twelve previously recorded defects (LX001–LX012) are fixed in
`0x03000035`; no open defect was carried into this migration. Residual risks
carried over unchanged: blocking focuser waits, unverified physical park
completion on non-Autostar Meade firmware, no hardware validation.
`lx200_private_data.motioned` is written in two places and never read.

### Platforms, build and packaging

Portable C, no platform-specific code. Built by `Makefile.drv` into
`indigo_mount_lx200.a` / `.dylib` / executable, registered in
`indigo_server/indigo_server.c`, `indigo.xcodeproj` and
`indigo_mount_lx200.vcxproj`. `99-indigo_lx200.rules` is installed on Linux.
No generator input existed before this change.

### Existing test assets

* `indigo_test/simulator_common/mount_lx200_simulator.c` — test-owned
  protocol simulator, version 3, 13 profiles plus an ASI profile shared with
  `test_mount_asi_simulator`, `serial_motion.h` based motion, exact command
  event log, one-shot reply injection.
* `indigo_test/integration/test_mount_lx200_simulator.c` — 67 serial cases and
  3 opt-in TCP cases, ordinary and ASAN/UBSAN targets.
* Coverage gaps before this migration: no retained reference trace to diff a
  migration against, and the test file depends on the private
  `MOUNT_LX200_*_NAME` macros exported by the hand-written public header.

## Baseline (2026-09-20, macOS arm64, Apple clang, driver 0x03000035)

```sh
make -C indigo_drivers/mount_lx200 -f ../../Makefile.drv
make -C indigo_test build/integration/test_mount_lx200_simulator build/integration/test_mount_lx200_simulator_sanitize
cd indigo_test && build/integration/test_mount_lx200_simulator
```

STATE: COMPLETE. Driver and both test binaries built; the two pre-existing
`sprintf` deprecation warnings in the hand-written source are the only
warnings. The ordinary serial suite ran 67 cases, 67 passed, 0 failed, exit
status 0, wall time 5 m 51 s. No pre-existing failure and no unavailable
prerequisite. This is the preservation contract for the migration.

## Hardware-test decision

No hardware testing will be performed. No LX200-protocol mount is available
for this work. Nothing in this record claims or implies hardware validation;
all evidence below is simulator-backed.

## Accepted generator-owned behavior differences

These follow from documented generator semantics and cannot be expressed in a
`.driver` input without changing the generator, which is not authorized here.
Each is verified by an adapted test case rather than silently dropped.

1. **Overlapping GOTO.** `INDIGO_COPY_TARGETS_PROCESS_CHANGE` refuses a new
   `MOUNT_EQUATORIAL_COORDINATES` request while the property is BUSY. The
   hand-written driver replaced the target mid-slew. Generated mount drivers
   (for example `mount_ioptron`) already behave this way.
2. **Parked guard breadth.** The generator inserts
   `!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value` ahead of
   the `MOUNT_EQUATORIAL_COORDINATES`, `MOUNT_MOTION_RA`, `MOUNT_MOTION_DEC`
   and `MOUNT_TRACKING` handlers. The hand-written `IS_PARKED` additionally
   required `MOUNT_PARK_PROPERTY->count == 2`, so single-item park profiles
   were never guarded. Only the OpenAstroTech profile is affected in practice
   (it is the one single-item profile whose park state survives polling).
3. **Connection messages.** The generator publishes its own
   `Connected to …`/`Failed to connect to …`/`Disconnected from …` messages.
4. **Dead field.** `motioned` is dropped; it was never read.

## Migration plan

1. Add opt-in reference-trace capture to the shared serial test harness and
   record the normalized original-driver trace for the whole suite.
2. Write `indigo_mount_lx200.driver` from the audited source (Approach 1),
   keeping every protocol helper verbatim and renaming only what the
   generator requires (`lx200_open`/`lx200_close`, `*_finalizer`).
3. Generate `.c`/`.h`/`_main.c`, build the driver, fix definition errors.
4. Update the test to the generated public header (device-name literals).
5. Run the serial suite, compare the normalized trace with the baseline and
   resolve every difference not covered by the accepted list above.
6. Run the ASAN/UBSAN serial suite and the opt-in TCP target.
7. Strict universal warnings check and generator reproducibility check.
8. Register the `.driver` file in the Xcode project and the Windows project.
9. Update `MIGRATION_STATUS.md`, finish this record, clean up artifacts.

## Migration execution log

1. COMPLETE — reference trace. `indigo_current_test_name` was added to
   `indigo_test/test_runner.h` and an opt-in `capture_simulator_trace()` to
   `indigo_test/integration/serial_simulator_test_common.h`: when
   `INDIGO_SIMULATOR_TRACE_DIR` names a directory, every fixture's simulator
   command event log is copied out before teardown. The original-driver trace
   was recorded with
   `INDIGO_SIMULATOR_TRACE_DIR=<dir> build/integration/test_mount_lx200_simulator`
   (72 fixture logs, 2402 normalized lines). Normalization drops the
   nondeterministic timestamp column, masks host-clock `SC`/`SL` arguments and
   collapses adjacent repeated polling blocks. Two consecutive original-driver
   runs differ by 18 lines, all of them poll-versus-command interleaving; that
   is the noise floor the migration comparison is judged against.
2. COMPLETE — `indigo_mount_lx200.driver` written (3123 lines). The protocol
   layer (`compare_versions` through `meade_update_mount_state`) moved into the
   shared `code` block unchanged except for the three generator-required edits
   listed under found defects. Four device blocks (`mount`, `guider`,
   `focuser`, `aux`) carry the connect/disconnect/timer bodies and every
   property. `serial { configurable_speed = true; pattern … }` reproduces the
   three original match patterns including the empty catch-all pattern.
3. COMPLETE — generated, built, strict universal syntax check passed.
4. COMPLETE — `test_mount_lx200_simulator.c` now spells out the four logical
   device names, because the generated public header exposes only the entry
   point.
5. COMPLETE — first generated build: 61 of 67 serial cases passed. Five
   failures were one real migration defect (LX013), the sixth was the accepted
   overlapping-GOTO contract change. After the LX013 and LX014 repairs the
   serial suite passes 67/67 and the trace comparison is clean.
6. COMPLETE — ASAN/UBSAN serial 67/67, opt-in TCP 3/3 in both builds.
7. COMPLETE — regeneration reproducibility, Xcode registration,
   `MIGRATION_STATUS.md`, cleanup.

## Found defects (migration)

| ID | Status | Reproduction / source | Implemented fix and regression |
| --- | --- | --- | --- |
| LX013 | FIXED | `lx200_home_onstep`, `lx200_home_zwo`, `lx200_home_nyx`, `lx200_home_teenastro` failed with "Missing LX200 command hC". The generated change branch publishes BUSY before the queued handler runs, so a polling cycle could observe `MOUNT_EQUATORIAL_COORDINATES` BUSY while the GOTO was still queued and publish a premature OK. The test then requested HOME while the mount state still described the pre-GOTO pose, and the HOME admission guard rejected it. | Added `PRIVATE_DATA->goto_issued`, set by the accepted slew command and by an observed slew, and a polling branch that leaves the property BUSY while a GOTO is accepted but not yet sent — the `mount_ioptron` pattern. `lx200_home_onstep/zwo/nyx/teenastro` and the whole model matrix pass in both builds. |
| LX014 | FIXED | Source audit while fixing LX013: the park and home admission conditions read `parked`/`parking`/`homing`/`homed`, and several profiles derive exactly those flags from `MOUNT_PARK`/`MOUNT_HOME` being BUSY. Evaluating them inside the queued handler, after the generated BUSY publication, lets a poll make the condition reject the request that caused the BUSY state. | The conditions moved to `on_change_request`, which runs in the bus change branch before the property becomes BUSY — the same point the hand-written `change_property` used. Captured in `park_allowed`, `unpark_allowed` and `home_allowed`. All nine park and six home cases pass in both builds. |

Both defects are migration-specific: they are created by the generator's
earlier BUSY publication and could not be reproduced against the hand-written
driver. No pre-existing defect was found during this migration, and no
previously fixed defect (LX001–LX012) regressed.

## Intentional differences from the hand-written driver

Beyond the four generator-owned items listed earlier:

* `meade_open`/`meade_close` renamed to `lx200_open`/`lx200_close` for the
  generator's connection wiring, and `lx200_close()` no longer resets the
  shared reference count — the generator owns `PRIVATE_DATA->count`. The
  hand-written `device_count = 0` in close existed only to compensate for the
  `--device_count <= 0` disconnect tests and is not needed.
* `lastUTC` and `motioned` were dropped from the private data; neither was ever
  read.
* `MOUNT_PARK_SET`, `MOUNT_HOME_SET`, `MOUNT_EQUATORIAL_COORDINATES` and
  `MOUNT_ABORT_MOTION` carried their outcome text on the property update. The
  generator owns the final update, so the texts are now sent with
  `indigo_send_message()` and `MOUNT_ABORT_MOTION`'s "Aborted"/"Failed to
  abort" texts are dropped in favour of the published property state.
* The two aux polling callbacks became one generated `aux_timer_callback` that
  dispatches to `nyx_aux_update()` (10 s) or `onstep_aux_update()` (2 s); the
  OnStep slot discovery moved to `onstep_aux_discover()`.
* `guider_guide_dec_finish_callback` / `guider_guide_ra_finish_callback` were
  renamed `guider_guide_dec_finalizer` / `guider_guide_ra_finalizer`, which is
  semantically significant to the generator. Guide dispatch now runs at
  `INDIGO_TASK_PRIORITY_TIME` instead of normal priority.
* The aux device now resets the visibility of its four properties at every
  connect, so a session against one mount model cannot leave a property from a
  previous model's session visible.
* Disconnect cancels pending handlers before `on_disconnect` runs, so the
  mount's `:Q#` is now sent after cancellation rather than before it.
* `MOUNT_PEC`'s parked guard is now a `reject_change` block, which also marks
  the items for update so the rejected value cannot stay visible in a client.
* `FOCUSER_ABORT_MOTION` keeps synchronous dispatch
  (`asynchronous_change = false`). The relative focus move still blocks its
  device queue, so a queued urgent abort could not reach the `focus_aborted`
  flag. Redesigning that blocking wait is outside this migration.

## Trace comparison

```sh
INDIGO_SIMULATOR_TRACE_DIR=<dir> build/integration/test_mount_lx200_simulator
```

The migrated normalized trace differs from the original-driver trace in 46
lines over 2402, against a same-driver noise floor of 18. Every difference was
inspected: 40 lines are poll-versus-command interleaving of the same commands
(the noise class), three lines are one extra polling cycle between the two
`MS` commands of the rewritten overlapping-GOTO case, and three lines are a
`GXX1`/`GXX3` aux polling cycle that now falls after rather than before the
`SXX` writes in `lx200_shared_aux_lifecycle`. No command, argument, ordering
constraint, error path or lifecycle event is missing or reordered relative to
its neighbouring commands. No unexplained difference remains.

## Test changes

The case count is unchanged at 67 serial plus 3 opt-in TCP. Two cases were
rewritten against the migrated contract:

* `lx200_goto_overlap_replaces_target` became `lx200_goto_busy_refuses_overlap`
  and now asserts that a second GOTO during a slew sends no further `MS`, that
  the first target is still reached, and that the next request after completion
  is accepted and executed.
* `lx200_guider_directions_overlap_and_timing` waits for the master slew to end
  before issuing the next workload GOTO. The guide samples still run while the
  mount slews and polls; only the GOTO-on-GOTO overlap was removed. The wait
  uses a new shared-client observation of the master device's
  `MOUNT_EQUATORIAL_COORDINATES` state, because the property cache follows the
  guider during that test.

## Migration verification (version 0x03000036)

Commands from the repository root:

```sh
build/bin/indigo_generator indigo_drivers/mount_lx200/indigo_mount_lx200.driver
make -C indigo_drivers/mount_lx200 -f ../../Makefile.drv
make -C indigo_test build/integration/test_mount_lx200_simulator build/integration/test_mount_lx200_simulator_sanitize
clang -fsyntax-only -arch x86_64 -arch arm64 -DINDIGO_MACOS -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-function -Wno-sign-compare -Wno-deprecated-declarations -Iindigo_libs indigo_drivers/mount_lx200/indigo_mount_lx200.c
make -C indigo_test test-mount-lx200-tcp
```

Regenerating from the checked-in `.driver` reproduces the checked-in `.c`,
`.h` and `_main.c` byte for byte. The strict universal syntax check passed.
The only build warnings are the two pre-existing `sprintf` deprecations in
`gemini_set()` and the UTC offset formatting, carried over unchanged. Every
other integration test target in `indigo_test` still builds.

| Build / transport | Run | Passed | Failed |
| --- | --- | --- | --- |
| Ordinary serial | 67 | 67 | 0 |
| ASAN/UBSAN serial | 67 | 67 | 0 |
| Ordinary TCP | 3 | 3 | 0 |
| ASAN/UBSAN TCP | 3 | 3 | 0 |
| Unique scenarios per build | 70 | 70 | 0 |

No case was skipped or accepted as an expected failure, and no ASAN/UBSAN
memory or undefined-behavior diagnostic was reported. TSAN was not run, so
race freedom is not claimed. Representative macOS x86_64/Rosetta runs of driver
metadata, OnStep homing, shared connection ordering and the overlapping-GOTO
contract passed; that is a spot check, not the whole x86_64 matrix. Linux and
Windows execution are unavailable and unverified.

Migration-run guide completion error, milliseconds (48 samples per build,
20/100/500 ms in four directions across idle and mount GOTO/polling workloads,
warm-up discarded, endpoints as defined earlier in this file):

| Build | Min | Mean | Median | p95 | p99 / max | Stddev | Mean signed % | Max absolute % |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Ordinary | 53.798 | 79.559 | 58.041 | 191.830 | 198.754 | 48.190 | 132.902 | 345.735 |
| ASAN/UBSAN | 52.239 | 79.768 | 58.962 | 204.737 | 208.198 | 49.395 | 132.375 | 300.035 |

These are software completion-timing observations on this host, comparable to
the pre-migration figures recorded above; physical relay output was not
measured and no hardware pulse accuracy is claimed.

The `Retested` column in `MIGRATION_STATUS.md` was lowered from `HW` to `Sim`:
the earlier hardware validation applied to the hand-written implementation,
and the generated implementation has only simulator evidence.

Final simulated acceptance tests for this migration: 140 run, 140 passed
(70 unique cases in each of the ordinary and ASAN/UBSAN builds). The four
representative x86_64 runs and the narrow per-case reruns are additional
passing checks, separate from that matrix.
Final hardware tests: 0 run, 0 passed.

## Overlapping guide pulses (2026-09-20)

A guide pulse requested while another pulse on the same axis was still running was silently
discarded. `GUIDER_GUIDE_RA` and `GUIDER_GUIDE_DEC` now declare `accept_while_busy = true` and zero
both axis items in `on_change_request`, and each handler drops the finaliser of the pulse it
replaces so the superseded deadline cannot end the new pulse early. The cancellation sits in
`on_change`, where it runs on the device queue thread and `indigo_queue_remove()` skips its blocking
wait.

`lx200_guider_directions_overlap_and_timing` carried the comment "Same-axis BUSY requests are
ignored" and asserted that `Mgs0100` was never sent, which recorded the discard as expected
behaviour. It now asserts the command is sent and adds two duration-measuring cases: a 2000 ms pulse
replaced after 500 ms by a 600 ms pulse in the same direction (1446 ms measured) and by a 300 ms
pulse in the opposite direction (917 ms).
