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

---

# LX200 hardware acceptance run (2026-09-22)

## Scope and hardware-test decision

Non-interactive hardware run against a physically connected **Pegasus Astro
NYX-101, firmware 1.32.1**, on macOS arm64 over the mount's own USB serial
adapter at 115200 8N1, driver version `0x03000038` before this work. Hardware
testing IS performed for this record; every result below that names the NYX-101
is physical, every result that names the simulator is hardware-free.

The mount is a harmonic-drive GEM with no counterweight and the run moves it.
Every slew is computed from the pointing the scenario starts at, the motion
scenarios work at declination 75 rather than at the pole where a right
ascension axis produces no measurable arc, and the session restores the
tracking and parked state it found.

Nothing persistent in the controller is overwritten. `MOUNT_PARK_SET` is issued
in the moment after unparking, while the mount still stands on the position it
stores. `MOUNT_HOME_SET` is deliberately **not** exercised on hardware: it sends
`:hF#`, documented as a reset that "simulates a cold Start", and the run found
that it drops the controller into a standby in which `:hP#` is refused until
tracking is enabled again. The NYX WiFi credentials are read and asserted but
never written, because the protocol has no way to restore the previous
password. Both write paths stay with the simulator.

## Test asset

`indigo_test/hardware/test_mount_lx200_hw.c`, run with
`make -C indigo_test test-mount-lx200-hw`, 30 cases covering the mount
hardware acceptance checklist of `indigo_test/DRIVER_TESTING_RULES.md` and the
guider standard for the guider logical device. `MOUNT_LX200_HW_PORT` overrides
the port the driver matches through its own `Pegasus Astro`/`NYX` USB
descriptor pattern. The model specific cases report themselves as not
applicable when another LX200 mount is connected.

## Protocol observations from the NYX-101

Recorded directly from the controller and used as the source of the simulator
additions below. They are what the firmware does, not what `NYX101.pdf` says
where the two differ.

| Command | Documented | NYX-101 1.32.1 |
| --- | --- | --- |
| `:hP#` | `0` / `1` | `0` / `1`; `0` while the controller is in cold-start standby |
| `:hR#` | not documented | `1`, the OnStep `0`/`1` contract |
| `:hQ#` | `0` / `1` | `0` while the mount is parked, `1` while it is not |
| `:hF#` | no reply | no reply, and the controller enters cold-start standby |
| `:GT#` | `n.n` | `0` while tracking is disabled, otherwise the real frequency |
| `:GU#` rate | `( O K` | `(` lunar, `O` solar, lowercase `k` king, nothing for sidereal |
| `:GU#` pier | `O T W` | lowercase `o` for none, `T` east, `W` west |
| `:GU#` slewing | `N` no goto | `N` and `n` are independent: a slew with tracking on carries neither |
| `:WL>#` | `ssid,password` | `ssid:password`, colon separated |
| `:GX9D#` | `pitch : roll` | signed, e.g. `34.5:-0.7` |
| `:GX97#` | max slew rate | `4.5` degrees per second |

## Baseline (2026-09-22, macOS arm64, driver 0x03000038)

```sh
make -C indigo_test build/hardware/test_mount_lx200_hw
make -C indigo_test test-mount-lx200-hw
```

STATE: COMPLETE. The 30-case hardware suite ran against the NYX-101 before any
production change: 22 passed, 8 failed. Five failures were driver defects and
three were defects in the new test itself, which the run exposed together. The
existing 67-case simulator suite passed unchanged in the same session, which is
why none of the five had been seen before: every one of them needs either the
real `:GT#`/`:GU#` answers of the controller or a request sequence the simulator
did not produce. Test-side baseline failures are listed with the defects they
were mixed with and are not counted as driver defects.

## Found defects (hardware run)

All are reproduced hardware-free in `mount_lx200_simulator.c` (version 4) and
pinned by a regression case in `test_mount_lx200_simulator.c`, as
`indigo_test/AGENTS.md` requires. Driver version 0x03000038 to 0x03000039.

| ID | Status | Hardware observation and root cause | Fix and regression |
| --- | --- | --- | --- |
| LX015 | FIXED | `lx200_parks_and_unparks` hung: `MOUNT_PARK` stayed BUSY for the whole 180 s timeout. `:hP#` answers `0` or `1` on the NYX and on OnStep, and the controller refuses the park outright in the cold-start standby a `:hF#` reset leaves behind. `meade_park()` sent it as a no-reply command, so the refusal was invisible, BUSY was published and nothing ever completed it. | `meade_park()` reads the documented reply for NYX and OnStep and requires `1`; Meade, OAT and TeenAstro keep the no-reply form their manuals document. Simulator: NYX answers `:hP#`, and answers `0` while `:hF#` has left it in standby. Regression `lx200_nyx_park_commands_report_refusals`. |
| LX016 | FIXED | Source audit while fixing LX015, confirmed on the mount: `:hR#` answers `1`, and OnStep documents `0`/`1` for it. `meade_unpark()` sent it as a no-reply command, so a refused unpark would have been reported as success. | `meade_unpark()` reads the reply for NYX and OnStep. Simulator: NYX answers `:hR#`. Covered by the same regression. |
| LX017 | FIXED | `lx200_reads_the_nyx_leveler` failed: the mount reported roll `-0.7`, while `X_NYX_LEVELER` declared pitch and roll as `0 … 360`. A published value outside the range it advertises is wrong for every client that clamps or validates it. | Pitch and roll are declared `-180 … 180`. Simulator: `:GX9D#` answers `34.5:-0.7`, the signed pair measured on the mount, instead of the previous positive one. Regression: `lx200_nyx_options_and_wifi_failures` asserts the signed value and that both items lie inside their published range. |
| LX018 | FIXED | `lx200_reports_the_tracking_rate_from_the_mount` failed: a session opened while tracking was off reported the lunar rate although the mount was configured for sidereal. The NYX answers `:GT#` with `0` while tracking is disabled, and `meade_get_tracking_rate()` mapped everything at or below 57.9 Hz to lunar. | The NYX no longer decodes `:GT#`; the rate comes from the `:GU#` status characters, as it already did for OnStep: `(` lunar, `O` solar, lowercase `k` king, none sidereal, all four verified on the mount. Simulator: `:GT#` answers `0` for a NYX that is not tracking. Regression `lx200_nyx_tracking_rate_comes_from_status`. |
| LX019 | FIXED | `lx200_toggles_tracking` failed: `MOUNT_STATE`'s tracking light kept its previous value until the next polling cycle. `MOUNT_PARK` and `MOUNT_HOME` publish the light they change; `MOUNT_TRACKING` assigned it and published nothing. | The tracking handler publishes `MOUNT_STATE`. Regression: `lx200_nyx_keeps_tracking_while_slewing` asserts the light in the same update as the request. |
| LX020 | FIXED | `lx200_keeps_tracking_while_slewing` failed with tracking reported off in 81 of 101 samples taken during a slew. `:GU#` carries `N` for "no goto" and `n` for "not tracking" independently, and a slew with tracking on carries neither; `meade_update_nyx_state()` read them as alternatives, so every GOTO published tracking off and then on again. | The two characters are decoded independently, as the OnStep branch already did. Regression `lx200_nyx_keeps_tracking_while_slewing` fails the case on a single sample. |
| LX021 | FIXED | `lx200_parks_and_unparks` failed: after a successful unpark `MOUNT_STATE`'s park light was OK instead of idle, because the handler assigned it the state of the request rather than the parked state of the mount. | The light is set from the parked state; a successful unpark turns it off. Regression `lx200_nyx_park_light_follows_the_mount` asserts it before the next polling cycle can correct it. |
| LX022 | FIXED | Found while writing the LX015 regression: a park requested in the same polling interval as the unpark before it was silently dropped. `PRIVATE_DATA->parked` only follows the controller status, so the admission guard still saw a parked mount and the request was republished OK with no command sent. | A successful unpark clears the cached flag for every model whose unpark completes immediately; the models that publish BUSY keep following the status. Covered by `lx200_nyx_park_commands_report_refusals`, which stores the park position immediately after unparking. |
| LX023 | FIXED | Same case: a refused or failed `MOUNT_PARK` left the rejected value in the property. `MOUNT_PARK_PARKED_ITEM` is what the generated parked guards of the motion and tracking properties read, so a failed park locked the client out of both until the next poll. | `meade_restore_park_switch()` puts the property back on the state the driver knows before the refusal or failure is published. `lx200_nyx_park_commands_report_refusals` asserts the restored value and that tracking is accepted straight afterwards. |

### Test defects the run exposed

Not driver defects; recorded because they were part of the baseline failure
count and because each one is a trap for the next mount suite.

* Right ascension motion was judged by angular separation. Next to the pole an
  axis can turn through hours of right ascension without covering any arc, so a
  working axis measured as not moving at all. The scenarios now work at
  declination 75 and judge each axis by its own coordinate.
* The abort scenarios raced the mount. `MOUNT_ABORT_MOTION` is dispatched at
  urgent priority, so an abort requested before the goto handler has run halts a
  mount that has not started moving, and one requested after the busy
  publication can still arrive after a short slew has finished. Both now wait
  until the controller itself reports the slew.
* `MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY_NAME` is not the property the mount
  base class publishes; it is `GEOGRAPHIC_COORDINATES`.
* A scenario cannot wait for a fresh publication of the property it is
  asserting, because INDIGO suppresses an update that says nothing new. The
  suite waits on `UTC_TIME`, the one property the polling callback moves on
  every cycle.

## Controller behaviour that is not a defect

Recorded because each one cost a scenario and would cost the next one again.

* **A goto during a guide pulse is refused.** The NYX answers `:MS#` with error
  8, "already in motion", while a pulse it was given is still running, and the
  driver reports it as an alert with that message. `lx200_guides_while_the_mount_slews`
  therefore starts the slew first and pulses during it, which the mount accepts.
  Modelled in the simulator and pinned by `lx200_nyx_goto_during_a_guide_pulse_is_refused`.
* **`:hQ#` is refused while the mount is parked** and accepted while it is not,
  which is why the park position is stored in the moment after unparking.
* **`:hF#` costs the park position.** It resets the controller at its home
  position and leaves it in a standby in which `:hP#` is refused until tracking
  is enabled again. This is why `MOUNT_HOME_SET` is not exercised on hardware.

## Hardware acceptance results (2026-09-22 23:07, driver 0x03000039)

macOS 15 arm64, Pegasus Astro NYX-101 firmware 1.32.1 on
`/dev/cu.usbserial-NYX467edc0c` at 115200 8N1.

| Suite | Run | Passed | Failed |
| --- | --- | --- | --- |
| `test-mount-lx200-hw` against the NYX-101 | 30 | 30 | 0 |

Scenario to case mapping for the mount hardware acceptance checklist:

| Checklist item | Cases |
| --- | --- |
| Discovery, identity and capability readback | `lx200_reports_identity_and_capabilities`, `lx200_publishes_the_property_contract` |
| Coordinates and supported status readback | `lx200_reads_site_and_time`, `lx200_reports_side_of_pier`, `lx200_reads_the_nyx_leveler`, `lx200_reads_the_nyx_wifi_configuration`, `lx200_reads_the_nyx_sensors` |
| Small reachable slew and SYNC | `lx200_slews_to_a_nearby_target`, `lx200_syncs_to_the_current_pointing` |
| Manual motion in both axes | `lx200_moves_both_axes_manually` |
| Tracking and rate changes | `lx200_toggles_tracking`, `lx200_selects_tracking_rates`, `lx200_reports_the_tracking_rate_from_the_mount`, `lx200_keeps_tracking_while_slewing`, `lx200_selects_slew_rates` |
| Abort followed by a fresh command | `lx200_aborts_manual_motion`, `lx200_aborts_a_slew_and_accepts_a_fresh_one` |
| Park, unpark and home with the actual setup | `lx200_unparks_the_mount`, `lx200_goes_home`, `lx200_parks_and_unparks` |
| Guider standard on the guider logical device | `lx200_guides_in_all_four_directions`, `lx200_guides_both_axes_at_once`, `lx200_replaces_a_guide_pulse_on_the_same_axis`, `lx200_measures_guide_pulse_duration`, `lx200_guides_while_the_mount_slews` |
| Shared devices, connection orders, last close | `lx200_shares_the_serial_session`, `lx200_refuses_the_unsupported_focuser` |
| Connection failure, reconnect, INIT/SHUTDOWN | `lx200_refuses_an_unusable_port`, `lx200_reconnects`, `lx200_reinitializes` |

Measured on the mount: site 48.2167 N, 16.9833 E; local sidereal time 22.3367 h;
mount clock 2026-09-22T21:05:19 with UTC offset 1; leveler pitch 28.7, roll
-0.3, compass 63.9; ambient 23.2 C, 1006.4 mb, supply 12.3 V; maximum slew rate
4.5 degrees per second from `:GX97#`, about 1.5 degrees per second measured near
the pole. The abort stopped a 12 degree slew 10.4731 degrees short of its
target. The mount kept tracking through all 101 samples taken during a slew.

### Guiding pulse duration accuracy (hardware)

Requested 20, 100 and 500 ms in all four directions, three retained samples per
direction and duration after one discarded warm-up, 12 samples per duration.
The endpoints are the client's change request and the OK publication of
`GUIDER_GUIDE_RA` / `GUIDER_GUIDE_DEC`: the mount terminates the pulse itself,
so **this is software completion timing over the real serial path, not an
electrical measurement of the relay output**. Each transaction carries the
driver's 50 ms post-command settle and one serial round trip.

| Requested | n | Min | Mean | Median | p95 | p99 / max | Stddev | Signed error | Max absolute |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 20 ms | 12 | 73.31 | 77.57 | 78.32 | 81.32 | 81.53 | 3.05 | +57.57 ms (+287.84 %) | 61.53 ms |
| 100 ms | 12 | 152.43 | 165.26 | 158.42 | 162.94 | 252.52 | 27.68 | +65.26 ms (+65.26 %) | 152.52 ms |
| 500 ms | 12 | 553.07 | 585.20 | 558.25 | 643.61 | 649.05 | 43.27 | +85.20 ms (+17.04 %) | 149.05 ms |

A pulse during a slew completed in 358.7 ms, both axes at once in 785.6 ms, and
a 2000 ms pulse replaced after 500 ms by a 600 ms pulse ended after 1179 ms.
These are observations on this host, not acceptance limits.

## Simulator results (driver 0x03000039, simulator version 4)

The suite grew from 67 to 72 serial cases with the five regressions above. Two
full runs of the whole suite were made while the fixes were being written. The
first reported 71 of 71 passing and the second 71 of 72, with
`lx200_initialization_rollback_and_reconnect` failing on a fixture left over
from the new `lx200_nyx_goto_during_a_guide_pulse_is_refused` case, which
connected the master device and tore down only the guider; the case now
disconnects the master as the other shared-device cases do. **That teardown fix
was not re-verified by another full run**, because the run was stopped at that
point; the five new cases and every case they touch passed individually
afterwards. The opt-in TCP target and the ASAN/UBSAN build were not run in this
session.

## Not covered

* Physical transport loss. The mount is on a USB serial adapter and the cable
  cannot be pulled from a non-interactive run; no hot-plug coverage is claimed.
* `MOUNT_HOME_SET` and the three WiFi write properties on hardware, for the
  reasons given above. All four are covered against the simulator.
* Pointing accuracy, tracking accuracy, polar alignment and periodic error, which
  the driver testing rules place outside driver acceptance.
* Other LX200 models. This run validates the NYX branch of the driver; the
  Meade, 10micron, Gemini, StarGO, AP, OnStep, ZWO, OAT, aGotino and TeenAstro
  branches keep their simulator evidence only.

## Final test summary for this run

* Simulated tests: 72 run, 71 passed in the last full run, with the one failure
  and its unverified fix described above.
* Hardware tests: 30 run, 30 passed, against a Pegasus Astro NYX-101.

# LX200 hardware acceptance run, OnStepX (2026-09-22)

## Scope and hardware-test decision

Non-interactive hardware run against a physically connected **OnStepX
controller, firmware 10.28x**, on macOS arm64 over the controller's ESP32 USB
CDC port `/dev/cu.usbmodem2401`, driver version `0x03000039` before this work.
Hardware testing IS performed for this record; every result below that names
OnStepX is physical, every result that names the simulator is hardware-free.

This is the second model the mount hardware suite has been run against. The
first run, recorded above, validated the NYX branch. The suite was written
around that mount and reported the model specific scenarios as not applicable
for anything else, so a large part of this work is teaching it the OnStep
branch rather than only fixing what it found.

## Bench preparation

The controller was found with **latitude 0, longitude 0 and a clock 20 hours
behind**, the factory default of a freshly flashed board. At latitude 0 the
celestial pole lies on the horizon, the mount stands below its own horizon
limit, and OnStep answers every `:MS#` with `6`, "outside limits". Nothing the
driver does can slew such a controller, so the site and clock were configured
once before the acceptance run, directly over the serial port:

```text
:St+48*13#  :Sg343*01#  :SG-02#  :SC09/22/26#  :SL23:40:30#  :hC#
```

The site is 48°13' N, 16°59' E, the same place the NYX run was recorded at, and
`:hC#` put the axes back on the home position the manual motion scenarios had
left. This is test bench configuration and is called out here because the
baseline below was taken *before* it: eleven of the thirteen baseline failures
are the unconfigured site, not the driver.

## Test asset

`indigo_test/hardware/test_mount_lx200_hw.c`, run with
`make -C indigo_test test-mount-lx200-hw` and `MOUNT_LX200_HW_PORT` pointing at
the controller. The file grew from 30 to 33 cases: it now detects the OnStep
model as well as the NYX, asserts the property contract `meade_init_onstep_mount()`
publishes, writes the four OnStep option properties back with the values the
controller already holds, checks the auxiliary feature outlets of the aux
logical device, and follows the focuser capability the controller reports.

## Protocol observations from the OnStepX

Recorded directly from the controller and used as the source of the simulator
additions below. They are what the firmware does, next to what `OnStepX.pdf`
documents.

| Command | Documented | OnStepX 10.28x |
| --- | --- | --- |
| `:MS#` | `0 .. 9` | `0` accepted, `6` outside limits with the site unset |
| `:hC#` | no reply | no reply; the simulator answered `1` for OnStep and now does not |
| `:hP#` / `:hR#` | `0` / `1` | `0` while the mount is outside its limits or already in that state |
| `:GXY0#` | eight character bitmap | `0`, a single character, in a build without auxiliary features |
| `:Fa#` | `1` when a focuser is present | `0`, and every other focuser command answers `0` too |
| `:FT#` | `M1#`, `S3#` … | `0` in a build without a focuser, so it never reports `S` |
| `:Gg#` | west positive | `+000*00` with the site unset, `-016*59` once it is configured |
| `:GU#` | ordered status string | `nNpHvEo160` at home, `npHhvEo140` while homing, `pvEW140` while slewing |
| `:GT#` | `n.n` | `0` while tracking is disabled, as on the NYX |
| `:GX97#` | max slew rate | `1.0` degrees per second |

## Baseline (2026-09-22 23:21, macOS arm64, driver 0x03000039)

```sh
make -C indigo_test build/hardware/test_mount_lx200_hw
MOUNT_LX200_HW_PORT=/dev/cu.usbmodem2401 indigo_test/build/hardware/test_mount_lx200_hw --run
```

STATE: COMPLETE. The 30-case suite ran against the OnStepX before any change:
**17 passed, 13 failed**. Two failures were driver defects, two were defects in
the test, and the remaining nine were all the same unconfigured site refusing
every GOTO. Two further driver defects were visible in the baseline output
without failing a case, because no case asserted them, and one more was found
by probing the controller for the focuser the suite skips on a non-NYX mount.

| Case | Baseline | Cause |
| --- | --- | --- |
| `lx200_unparks_the_mount` | FAIL | site: the working slew was refused with `:MS#` 6 |
| `lx200_toggles_tracking` | FAIL | test: no `MOUNT_STATE` publication when the light does not change |
| `lx200_selects_tracking_rates` | FAIL | cascade of the tracking failure |
| `lx200_reports_the_tracking_rate_from_the_mount` | FAIL | cascade of the tracking failure |
| `lx200_moves_both_axes_manually` | FAIL | LX026, the coordinates stayed in ALERT |
| `lx200_aborts_manual_motion` | FAIL | LX026 |
| `lx200_slews_to_a_nearby_target` | FAIL | site |
| `lx200_keeps_tracking_while_slewing` | FAIL | site |
| `lx200_aborts_a_slew_and_accepts_a_fresh_one` | FAIL | site |
| `lx200_guides_while_the_mount_slews` | FAIL | site |
| `lx200_goes_home` | FAIL | site: the mount was outside its limits |
| `lx200_parks_and_unparks` | FAIL | site: `:hP#` answered `0` |
| `lx200_reconnects` | FAIL | test: it waited for `AUX_WEATHER`, which OnStep hides |

## Found defects (hardware run)

Driver version 0x03000039 to 0x0300003A. Each one is reproduced hardware-free
in `mount_lx200_simulator.c` and pinned by a regression case in
`test_mount_lx200_simulator.c`, as `indigo_test/AGENTS.md` requires.

| ID | Status | Hardware observation and root cause | Fix and regression |
| --- | --- | --- | --- |
| LX024 | FIXED | `lx200_moves_both_axes_manually` and every later motion case timed out waiting for `MOUNT_EQUATORIAL_COORDINATES` to settle, although the axes were moving and the coordinates were being read: the single GOTO the unconfigured site had refused left the property in ALERT, and `meade_update_mount_state()` kept it there for the rest of the session. The sticky branch reads the property's own state, so it cannot tell a failed readback, which `PRIVATE_DATA->coordinate_read_failed` already tracks, from a refused GOTO. | The polling callback publishes OK whenever the readback succeeded; only `coordinate_read_failed` keeps the property in ALERT, through the branch that already owns that case. Regression `lx200_refused_goto_recovers_on_the_next_poll` refuses one `:MS#` with error 3 and requires the property to come back on its own, with no further request. |
| LX025 | FIXED | The refused GOTO was reported to the client as a bare "Slew failed". OnStep answers `:MS#` with the same 0 .. 9 code table as the OnStep derived NYX, and `meade_error_string()` already carries that table for `MOUNT_TYPE_ON_STEP`, but `meade_slew()` only decoded it for NYX and TeenAstro. The run had to be diagnosed by talking to the controller by hand. | `meade_slew()` decodes the code for OnStep as well, so the client is told "Outside limits" instead of only that the slew failed. Covered by the same regression, which asserts the message path through error 3. |
| LX026 | FIXED | `GEOGRAPHIC_COORDINATES` published **longitude 360** for a controller that reports its site as `+000*00`. `meade_get_site()` inverts the LX200 west positive sign with `360 - longitude` and never normalises, so the prime meridian comes back as 360 instead of 0; `meade_set_site()` has the same hole in the other direction and would have sent `:Sg360*00#`, which a mount that validates its input rejects. 360 is inside the range the property advertises, so nothing caught it. | Both directions normalise with `fmod`. Regression `lx200_prime_meridian_longitude_is_zero` asserts the command the mount receives is `:Sg000*00#` and that a fresh session reads the site back as 0. The hardware case `lx200_reads_site_and_time` now also requires `0 <= longitude < 360`. |
| LX027 | FIXED | The aux device logged "Onstep AUX Device at slot 8 invalid response" at every connect. `:GXY0#` answers `0`, a single character, in a build without auxiliary features, and `onstep_aux_discover()` copied eight bytes out of that reply, so seven of them were whatever the shared response buffer still held; slots that do not exist were then queried. Both outlet properties were published with no item at all. | The reply has to be the documented eight character bitmap of `0` and `1` or the controller is taken to have no auxiliary features, the counts are cleared before every way out of the function, and an outlet property with no item stays hidden. Regression `lx200_onstep_without_auxiliary_features`. |
| LX028 | FIXED | Found by probing the controller for the focuser the suite skipped on a non-NYX mount. This OnStepX answers `:Fa#` with `0`, no focuser, and answers `0` to every other focuser command including `:FT#`. The focuser logical device connected anyway, and `meade_focus_rel()` polls `:FT#` until it answers `S`, which such a controller never does: the first move would have blocked the device queue until the driver was restarted. | The focuser connection requires `:Fa#` to answer `1` on OnStep, and the `:FT#` wait is bounded by `ONSTEP_FOCUS_TIMEOUT`, after which the move is aborted and fails. Regression `lx200_onstep_without_a_focuser_refuses_the_connection`. |

### Test defects the run exposed

Not driver defects; recorded because each one is a trap for the next mount the
suite is pointed at.

* `lx200_toggles_tracking` required `MOUNT_TRACKING` to publish `MOUNT_STATE` in
  the same update as the request, but INDIGO publishes nothing about a property
  that says exactly what it said before. The NYX was found tracking, so the
  first request changed the light; the OnStepX was found with tracking off, so
  it did not, and the case failed on a driver that was behaving correctly. The
  scenario now establishes the other state before it measures anything.
* `lx200_reconnects` waited for `AUX_WEATHER` to settle. That is the property
  the NYX aux device publishes; the OnStep aux device publishes outlets, and a
  controller without auxiliary features publishes neither. The scenario now asks
  the connected device which property it keeps fresh.

### Simulator corrections

* `:hC#` is documented with no reply and the controller sends none. The
  simulator answered `1` for OnStep, which would have hidden a driver waiting
  for a reply that never comes.
* `:Fa#` was not implemented at all, so the new capability check had nothing to
  read. The simulator now answers `1`, the value a build with a focuser reports,
  and a case injects `0` for the controller that has none.

## Controller behaviour that is not a defect

* **An unconfigured site refuses everything.** With latitude 0 the pole is on
  the horizon, the mount stands below its own horizon limit, and `:MS#` answers
  `6`. This is not a driver fault and no driver change can work around it; it is
  why the bench preparation above exists.
* **A GOTO does not need tracking to be armed first.** The NYX branch of
  `meade_slew()` turns tracking on before the slew because that firmware refuses
  it otherwise. OnStep does not need it: `:MS#` starts tracking itself, and the
  status string carries no `n` once the slew is running. The NYX-only branch was
  therefore left alone.
* **`:hR#` answers `0` for a mount that is not parked**, and `:hP#` answers `0`
  for one that is outside its limits. Both are refusals the driver already
  reads, since LX015 and LX016.

## Scenario to case mapping, OnStep branch

| Checklist item | Cases |
| --- | --- |
| Discovery, identity and capability readback | `lx200_reports_identity_and_capabilities`, `lx200_publishes_the_property_contract` |
| Coordinates and supported status readback | `lx200_reads_site_and_time`, `lx200_reports_side_of_pier`, `lx200_writes_back_the_onstep_options`, `lx200_reads_the_onstep_outlets` |
| Small reachable slew and SYNC | `lx200_slews_to_a_nearby_target`, `lx200_syncs_to_the_current_pointing` |
| Manual motion in both axes | `lx200_moves_both_axes_manually` |
| Tracking and rate changes | `lx200_toggles_tracking`, `lx200_selects_tracking_rates`, `lx200_reports_the_tracking_rate_from_the_mount`, `lx200_keeps_tracking_while_slewing`, `lx200_selects_slew_rates` |
| Abort followed by a fresh command | `lx200_aborts_manual_motion`, `lx200_aborts_a_slew_and_accepts_a_fresh_one` |
| Park, unpark and home with the actual setup | `lx200_unparks_the_mount`, `lx200_goes_home`, `lx200_parks_and_unparks` |
| Guider standard on the guider logical device | `lx200_guides_in_all_four_directions`, `lx200_guides_both_axes_at_once`, `lx200_replaces_a_guide_pulse_on_the_same_axis`, `lx200_measures_guide_pulse_duration`, `lx200_guides_while_the_mount_slews` |
| Shared devices, connection orders, last close | `lx200_shares_the_serial_session`, `lx200_follows_the_onstep_focuser_capability` |
| Connection failure, reconnect, INIT/SHUTDOWN | `lx200_refuses_an_unusable_port`, `lx200_reconnects`, `lx200_reinitializes` |

The five NYX specific cases report themselves as not applicable on this
controller, and the OnStep specific ones do the same on a NYX.

## Not covered

* Physical transport loss. The controller is on its own USB CDC port and the
  cable cannot be pulled from a non-interactive run; no hot-plug coverage is
  claimed.
* The OnStep focuser move on hardware. This controller answers `:Fa#` with `0`,
  so it has no focuser to move, and the capability case asserts the refusal
  instead. The move, its speeds, the direction, reverse motion and abort are
  covered against the simulator by `lx200_focuser_onstep_operations`.
* The OnStep auxiliary feature outlets on hardware, for the same reason: this
  firmware has none, and the case asserts that neither outlet property is
  offered. The outlets themselves are covered by
  `lx200_aux_slot_mapping_and_failure_recovery`.
* `MOUNT_PEC` writes on hardware. `:$QZ+#` arms a PEC playback the controller
  keeps, and there is no reading that would let the run put back what it found,
  so only the readback is asserted. The write paths stay with the simulator.
* The NYX-101 was **not** re-run after these fixes. Two of them, LX024 and
  LX026, are in code every model shares; the NYX branch keeps the simulator
  evidence of this session and the hardware evidence of the run recorded above.
* Pointing accuracy, tracking accuracy, polar alignment and periodic error,
  which the driver testing rules place outside driver acceptance.

## Found defects, second batch

Found after the first batch of repairs, when the motion scenarios could finally run.

| ID | Status | Hardware observation and root cause | Fix and regression |
| --- | --- | --- | --- |
| LX029 | FIXED | `lx200_selects_tracking_rates` and `lx200_reports_the_tracking_rate_from_the_mount` failed on the king rate only. OnStepX 10.28x accepts `:TK#` and really tracks at the king rate, `:GT#` answers 60.136 Hz against the 60.164 Hz of sidereal, but its `:GU#` carries **no rate character at all** for it: a mount on the king rate reports exactly what a mount on the sidereal rate reports. `meade_update_onstep_state()` read the absence of a character as sidereal, so the property went back to sidereal one polling cycle after the client selected king. | A status with no rate character is settled with `:GT#`, and the king rate is taken when the frequency is within 0.01 Hz of 60.136. `:GT#` answers 0 while tracking is disabled, which says nothing about the configured rate, so the driver keeps the rate it holds. Simulator: `:GU#` no longer reports the documented `k` unless `--status-king` is given, and `:GT#` answers the four frequencies an OnStepX reports. Regression `lx200_onstep_king_rate_comes_from_the_frequency`. |

### Test defects the second run exposed

* `lx200_goes_home` used `MOTION_TIMEOUT`, 180 seconds. A home slew is the one motion that can
  cross the whole range of both axes, and this controller reports one degree per second for its
  maximum slew rate: a measured home run moved 45 degrees of declination and 48 degrees of right
  ascension in 50 seconds and was still going. The scenario now has its own `HOME_TIMEOUT` of 600
  seconds. The assertion is unchanged: the home still has to complete.

### Observed once and not reproduced

`lx200_guides_while_the_mount_slews` failed once, in the run that still had the two tracking rate
defects, with the coordinates property in ALERT and **no message from the driver**. A refused
`:MS#` publishes "Slew failed" together with the decoded reason, so this was not a refusal: the
only other path that puts the property into ALERT is a failed coordinate readback, which the
polling callback now takes back on the next successful read. Probing the controller afterwards
showed that a goto issued while a guide pulse is still running is accepted with `:MS#` 0, so the
"already in motion" refusal the NYX has does not apply here. The case passed in the clean run and
the cause stays unconfirmed; it is recorded rather than worked around, because the scenario reads
any ALERT on the coordinates as a refusal and a transient readback failure can still be misread.

## Hardware acceptance results (2026-09-23 00:11, driver 0x0300003A)

macOS 15 arm64, OnStepX 10.28x on `/dev/cu.usbmodem2401`.

| Suite | Run | Passed | Failed |
| --- | --- | --- | --- |
| `test-mount-lx200-hw` against the OnStepX, baseline (0x03000039) | 30 | 17 | 13 |
| `test-mount-lx200-hw` against the OnStepX, after the first batch | 33 | 29 | 4 |
| `test-mount-lx200-hw` against the OnStepX, final | 33 | 33 | 0 |

Measured on the controller: site 48.2167 N, 16.9833 E; local sidereal time 23.4470 h; mount clock
2026-09-22T22:11:45 with UTC offset 2; meridian limits 15 degrees east and west; altitude limits
-10 and 80 degrees; preferred pier side "best"; automatic meridian flip disabled; PEC disabled;
no auxiliary feature slots and no focuser. The abort stopped a 12 degree slew 11.8445 degrees
short of its target. The mount kept tracking through all 345 samples taken during a slew. The
maximum slew rate the controller reports through `:GX97#` is 1.0 degrees per second, and a home
slew measured 0.9 degrees per second on both axes.

### Guiding pulse duration accuracy (hardware)

Requested 20, 100 and 500 ms in all four directions, three retained samples per direction and
duration after one discarded warm-up, 12 samples per duration. The endpoints are the client's
change request and the OK publication of `GUIDER_GUIDE_RA` / `GUIDER_GUIDE_DEC`: the controller
terminates the pulse itself, so **this is software completion timing over the real serial path,
not an electrical measurement of the relay output**. Each transaction carries the driver's 50 ms
post-command settle and one serial round trip.

| Requested | n | Min | Mean | Median | p95 | p99 / max | Stddev | Signed error | Max absolute |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 20 ms | 12 | 73.99 | 96.98 | 80.29 | 82.62 | 291.16 | 61.20 | +76.98 ms (+384.89 %) | 271.16 ms |
| 100 ms | 12 | 152.51 | 185.96 | 160.92 | 278.51 | 363.59 | 65.74 | +85.96 ms (+85.96 %) | 263.59 ms |
| 500 ms | 12 | 553.89 | 587.17 | 564.32 | 670.49 | 674.37 | 48.58 | +87.17 ms (+17.43 %) | 174.37 ms |

A pulse during a slew completed in 354.2 ms, both axes at once in 717.9 ms, and a 2000 ms pulse
replaced after 500 ms by a 600 ms pulse ended after 1152.5 ms. These are observations on this
host, not acceptance limits.

### Cost of the king rate repair

Settling the rate through `:GT#` costs one extra serial transaction per polling cycle on an
OnStep whose status carries no rate character, which is every cycle on the sidereal and the king
rate. The polling callback already issues `:GR#`, `:GD#` and `:GU#` in the same cycle, so it is
one transaction more on a link that is not otherwise busy. It is recorded here because it is a
real cost and not a free repair.

## Simulator results (driver 0x0300003A)

The serial suite grew from 72 to 78 cases with the six regressions above, and
`MIGRATION_STATUS.md` was corrected: its 75 did not match the suite.

| Suite | Run | Passed | Failed |
| --- | --- | --- | --- |
| `test_mount_lx200_simulator` | 78 | 78 | 0 |

New cases:

* `lx200_refused_goto_recovers_on_the_next_poll` — LX024 and the message of LX025.
* `lx200_prime_meridian_longitude_is_zero` — LX026 in both directions.
* `lx200_onstep_without_auxiliary_features` — LX027.
* `lx200_onstep_without_a_focuser_refuses_the_connection` — LX028.
* `lx200_onstep_king_rate_comes_from_the_frequency` — LX029 on the firmware that omits the `k`.
* `lx200_onstep_king_rate_comes_from_the_status` — the same contract on a build that reports it,
  which is what `--status-king` and the NYX model provide.

The opt-in TCP target and the ASAN/UBSAN build were not run in this session.

## Final test summary for this run

* Simulated tests: 78 run, 78 passed.
* Hardware tests: 33 run, 33 passed, against an OnStepX controller, firmware 10.28x.

# LX200 hardware acceptance run, aGotino (2026-09-23)

## Scope and hardware-test decision

Non-interactive hardware run against a physically connected **aGotino
controller, firmware 230312**, on macOS arm64 over the board's USB CDC port
`/dev/cu.usbmodem11101`, driver version `0x0300003A` before this work. Hardware
testing IS performed for this record; every result below that names aGotino is
physical, every result that names the simulator is hardware-free.

This is the third model the mount hardware suite has been run against, after
the NYX-101 and the OnStepX recorded above. Both of those implement most of the
LX200 command set. The aGotino implements eleven commands and nothing else, so
the suite had to learn the difference between a scenario that fails and a
scenario that has nothing to exercise before it could say anything about the
driver.

## What the aGotino is

An Arduino sketch (https://github.com/mappite/aGotino) driving two stepper
motors with no encoders and no hand controller feedback. Its whole LX200
surface, read from the source and confirmed on the board:

| Command | Reply | Note |
| --- | --- | --- |
| `:GVP#` | `aGotino#` | what the autodetection matches on |
| `:GVN#` | `230312#` | a date stamped build number |
| `:GV*#` | `#` | every other `:GV` is acknowledged with the bare terminator |
| `:GR#` | `HH:MM:SS#` | frozen at the start position while a slew runs |
| `:GD#` | `sDD\xdfMM:SS#` | degree sign `0xDF`, not the asterisk |
| `:Sr…#` / `:Sd…#` | `1` | not validated |
| `:MS#` | `0`, then `1Range_too_big#` when it is out of range | |
| `:Mn#` `:Ms#` `:Mw#` `:Me#` | no reply | continuous slow motion, no position bookkeeping |
| `:Q#` / `:Qx#` | no reply | stops declination, returns right ascension to tracking |
| `:CM#` | `0#` | sync |
| `:D#` | `#` idle, `\x7f#` slewing | the classic distance bar, with DEL |

Everything else - `:Gt#`, `:Gg#`, `:GC#`, `:GL#`, `:GG#`, `:GS#`, `:GW#`,
`:Mg…#` - is read off the line and dropped without a reply, so a driver that
sends one waits out its own timeout. There is no tracking switch, no park, no
home, no slew rate, no tracking rate, no pier side, no clock and no pulse
guiding, which is why `meade_init_agotino_mount()` hides all of them.

## Test asset

`indigo_test/hardware/test_mount_lx200_hw.c`, run with
`make -C indigo_test test-mount-lx200-hw` and `MOUNT_LX200_HW_PORT` pointing at
the board. The file grew from 33 to 35 cases. The larger change is that every
scenario now asks what the detected model publishes instead of assuming the
NYX or OnStep contract: a mount with no tracking switch, no park, no slew rate
or no manual motion reports those scenarios as not applicable, and the
secondary logical devices are brought up only for a model that can serve them.

## Baseline (2026-09-23 09:30, macOS arm64, driver 0x0300003A)

```sh
MOUNT_LX200_HW_PORT=/dev/cu.usbmodem11101 make -C indigo_test test-mount-lx200-hw
```

The unmodified suite did not reach its first case: `main()` connected the aux
logical device unconditionally, the driver correctly refused it for a
controller that has neither the NYX sensors nor the OnStep outlets, and the run
stopped there. With that made conditional, the baseline was **7 of 35 failed**.

## Found defects (hardware run)

| ID | Severity | Where | Defect |
| --- | --- | --- | --- |
| LX030 | major | `meade_update_mount_state()` | An aGotino goto was published as finished the moment it was issued. The model fell through to `meade_update_generic_state()`, which infers a slew from the coordinates changing, and an aGotino reports the position its slew started from until the slew ends. `MOUNT_EQUATORIAL_COORDINATES` went `OK` within one polling cycle while the mount was still moving, and the next command reached a controller still busy inside its slew loop, which answers nothing but `:GR#`, `:GD#`, `:D#` and `:Q#`. The scenario that followed then reported "Slew failed". Fixed by `meade_update_agotino_state()`, which reads the distance bar `:D#` the firmware does implement. |
| LX031 | major | guider `on_connect` | The guider logical device came up for an aGotino and reported every pulse as completed. The firmware has no pulse guiding command: it matches the `M` of `:Mgn0300#`, finds no direction in the `g` and drops the request, so nothing moved. Fixed by refusing the connection, the way the aux and focuser devices already refuse a controller they cannot serve. |
| LX032 | major | `meade_init_mount()`, `meade_get_site()` | Connecting an aGotino replaced the observing site with 0, 0. `meade_get_site()` returned without touching its output parameters for a model with no `:Gt#`, and the caller stored the zeroes anyway. The site is what the framework computes `MOUNT_LST_TIME` and `MOUNT_HORIZONTAL_COORDINATES` from, so both were wrong for the whole session; the hardware run published a local sidereal time of exactly 0. Fixed by returning `false` and leaving the site alone. The same path affects StarGO2, which has no site query either. |
| LX033 | minor | `meade_set_site()` | Setting the observing site was refused outright for an aGotino, so `MOUNT_GEOGRAPHIC_COORDINATES` went `ALERT` and a client had no way to give the driver the site it needs. Fixed by accepting it: there is nothing to send to the controller, and the value is the driver's own. |
| LX034 | minor | `meade_init_agotino_mount()` | `MOUNT_INFO.MODEL` stayed on the `Unknown` the generic initialization writes, although the autodetection had already read `aGotino` from `:GVP#`. Fixed by filling it from the product name. |

### Test defects the run exposed

| ID | Where | Defect |
| --- | --- | --- |
| LXT018 | `main()` | The aux and guider logical devices were connected unconditionally, so the run aborted on the first model that cannot serve them. They are now brought up only for a model that can, and `lx200_refuses_the_unsupported_devices` asserts the refusal for the others. |
| LXT019 | fourteen scenarios | Tracking, tracking rate, slew rate, manual motion, park, home, pier side and guiding were asserted unconditionally. Each now reports itself as not applicable when the model publishes no such property. |
| LXT020 | `disconnect_secondary_devices()` | Tearing the session down by capability instead of by what is connected left a device that came up although it should have refused holding the shared serial handle open, and the lifecycle scenarios then ran against a session nobody closed. It now disconnects whatever is connected. |
| LXT021 | `wait_for_poll()` | The polling heartbeat was `UTC_TIME`, which a model with no clock never publishes. It falls back to `MOUNT_LST_TIME`, which every model republishes on every cycle. |
| LXT022 | `lx200_aborts_a_slew_and_accepts_a_fresh_one` | The abort was judged by the arc the mount left unfinished, which an aGotino cannot report. See the next section. |

## Controller behaviour that is not a defect

* **An aborted slew reports the target it never reached.** `:Q#` genuinely
  stops the motors - measured on the board, a 12° declination slew takes
  15.1 s and the same slew aborted after 3.2 s ends 3.2 s in - but the sketch
  then stores the goto target as the current position, the same way it does
  when the loop ran to the end. Its own source marks this as a known
  limitation. Nothing in the protocol can find it out, so the driver publishes
  a position the mount is not at, and the operator has to sync again. The
  hardware scenario therefore judges the abort by the time it saved, and the
  simulator models the quirk so the same contract is pinned down hardware-free.
* **No site, no clock, no status word.** The driver must not send `:Gt#`,
  `:Gg#`, `:GC#`, `:GL#` or `:GW#` to this controller: they are dropped
  silently and cost a full read timeout each.
* **`:GD#` separates the degrees with `0xDF`.** The driver already rewrites
  the fourth byte before parsing.
* **Continuous slow motion exists but has no bookkeeping.** `:Mn#`, `:Ms#`,
  `:Mw#` and `:Me#` move the axes, but the sketch does not update the position
  it reports while they run, so a client would see a mount that moves without
  its coordinates changing. `MOUNT_MOTION_RA` and `MOUNT_MOTION_DEC` stay
  hidden, which is the pre-existing driver decision and was left alone.

## Not covered

* **`:MS#` out of range.** The sketch prints `0` before it checks the range and
  then `1Range_too_big#` when the slew is refused, so the refusal arrives after
  the driver has already read the acknowledgement and is left in the pipe for
  the next reply. Every slew this suite issues is a few degrees and well inside
  the 30° default, so the path was never entered on the board and is not
  modelled in the simulator. It is a real risk for a client that sends a large
  goto and is recorded here rather than claimed as covered.
* **Slow motion through `:Mn#` and friends**, for the reason above.
* **Hot-plug** was not part of this run; the board is a plain USB CDC device
  and the driver has no hot-plug support for it.

## Scenario to case mapping, aGotino branch

The class standard of `indigo_test/DRIVER_TESTING_RULES.md` was applied to what
this controller can do. The scenarios of the mount checklist that need a
property the firmware has no command for report themselves as not applicable,
which is recorded here rather than counted as coverage.

| Checklist scenario | Case | Result on the aGotino |
| --- | --- | --- |
| Identity and capabilities | `lx200_reports_identity_and_capabilities` | vendor, model and firmware all from the controller |
| Published property contract | `lx200_publishes_the_property_contract` | asserts the eleven-command contract, including everything that has to stay hidden |
| Site and clock readback | `lx200_reads_site_and_time` | site and LST only; no clock to read |
| Observing site | `lx200_keeps_the_observing_site` | accepted, kept through the polling cycles and a reconnect |
| Unpark, refusal while parked | `lx200_unparks_the_mount` | no park control; the working position is still established |
| Tracking on and off | `lx200_toggles_tracking` | not applicable, no tracking switch |
| Tracking rates, rate readback | `lx200_selects_tracking_rates`, `lx200_reports_the_tracking_rate_from_the_mount` | not applicable |
| Slew rates | `lx200_selects_slew_rates` | not applicable |
| Manual motion, manual abort | `lx200_moves_both_axes_manually`, `lx200_aborts_manual_motion` | not applicable, `MOUNT_MOTION_*` hidden |
| SYNC | `lx200_syncs_to_the_current_pointing` | `:CM#` |
| GOTO | `lx200_slews_to_a_nearby_target` | LX030, the case that found it |
| Tracking during a slew | `lx200_keeps_tracking_while_slewing` | not applicable |
| Abort a slew, accept a fresh one | `lx200_aborts_a_slew_and_accepts_a_fresh_one` | judged by the time saved, see above |
| Pier side | `lx200_reports_side_of_pier` | not applicable |
| Model specific readback | the four NYX and OnStep cases | not applicable |
| Secondary device refusal | `lx200_refuses_the_unsupported_devices`, `lx200_refuses_the_unsupported_focuser` | LX031, plus the aux and focuser refusals |
| Guiding, all four cases | the five guider cases | not applicable, no pulse guiding |
| Home, park | `lx200_goes_home`, `lx200_parks_and_unparks` | not applicable |
| Shared serial session | `lx200_shares_the_serial_session` | the mount alone opens and closes it |
| Refused port | `lx200_refuses_an_unusable_port` | |
| Reconnect, driver INIT/SHUTDOWN | `lx200_reconnects`, `lx200_reinitializes` | the model is re-detected from scratch |

## Hardware acceptance results (2026-09-23 10:06, driver 0x0300003B)

```sh
MOUNT_LX200_HW_PORT=/dev/cu.usbmodem11101 make -C indigo_test test-mount-lx200-hw
```

| Suite | Run | Passed | Failed |
| --- | --- | --- | --- |
| `test_mount_lx200_hw` against an aGotino, firmware 230312 | 35 | 35 | 0 |

Measured on the board during the run:

| Quantity | Value |
| --- | --- |
| 12° declination slew | 15.1 s, about 0.80 °/s |
| 6° declination slew, driver timed | 9.2 s |
| the same slew aborted | 2.0 s, the abort itself lands within 0.2 s |
| connect, autodetect and full state readback | under 2 s |

## Simulator additions from this run

The `agotino` model of `indigo_test/simulator_common/mount_lx200_simulator.c`
used to answer the shared command table like any other profile, which is what
let the driver's generic slew heuristic look adequate. It now models the
controller:

* only the commands the sketch dispatches are answered; every other request is
  dropped without a reply, `:GV*` other than `:GVP#` and `:GVN#` answer the
  bare terminator, and `:Mg…#` is dropped like the sketch drops it;
* `:GVN#` answers the real `230312`;
* `:GD#` separates the degrees with `0xDF`;
* `:GR#` and `:GD#` report the position the slew started from until it ends;
* `:D#` answers `\x7f#` while slewing;
* `:Q#` during a slew stores the target as the current position.

Four cases pin the hardware findings down hardware-free, so the serial suite
grows from 78 to 82:

* `lx200_agotino_holds_the_goto_until_the_slew_ends` - LX030 and LX034.
* `lx200_agotino_abort_reports_the_unreached_target` - the abort contract on a
  controller that cannot say where it stopped.
* `lx200_agotino_refuses_the_guider` - LX031.
* `lx200_agotino_keeps_the_site` - LX032 and LX033.

Three of the four were run against driver `0x0300003A` before the repairs and
failed there - the goto case on the model name, the guider case because the
device came up, the site case because the write was refused - and all four pass
against `0x0300003B`. The abort case was written after the repairs and pins
down a controller limitation rather than a driver defect, so it has no failing
counterpart.

## Simulator results (driver 0x0300003B)

| Suite | Run | Passed | Failed |
| --- | --- | --- | --- |
| `test_mount_lx200_simulator` | 82 | 82 | 0 |

The opt-in TCP target and the ASAN/UBSAN build were not run in this session.

## Final test summary for this run

* Simulated tests: 82 run, 82 passed.
* Hardware tests: 35 run, 35 passed, against an aGotino controller, firmware 230312.


# LX200 hardware acceptance run, OpenAstroTracker (2026-09-23)

## Scope and hardware-test decision

Non-interactive hardware run against a physically connected **OpenAstroTracker
controller, firmware v1.13.20**, on macOS arm64 over the board's ESP32-S3 USB
CDC port `/dev/cu.usbmodem5B7A0424131` at 19200 baud, driver version
`0x0300003B` before this work. Hardware testing IS performed for this record;
every result below that names the OpenAstroTracker is physical, every result
that names the simulator is hardware-free.

The controller is a bench board. Its `Configuration_local.hpp` says so in the
first line - "ESP32-S3 bench: no drivers, motors, display or sensors attached" -
and that matters for exactly one scenario, recorded under *Controller behaviour
that is not a defect*. Everything else the run exercised is independent of it:
the `:GX#` status word is computed from the firmware's own `_mountStatus` and
from open-loop AccelStepper positions, with no hardware feedback anywhere in it.

The firmware source was available at `~/Desktop/OAT-esp32-s3` during this run
and every protocol finding below is cited against it rather than inferred from
the replies alone.

## Protocol observations from the OpenAstroTracker

| Command | Documented LX200 | OpenAstroTracker v1.13.20 |
| --- | --- | --- |
| `:GVP#` | product name | `OpenAstroTracker#` |
| `:GVN#` | firmware version | `v1.13.20#` |
| `:GR#` | `HH:MM:SS#` | as documented |
| `:GD#` | `sDD*MM:SS#` | `+45*00'00#`, the arcminute mark where the protocol puts a colon |
| `:Gt#` / `:Gg#` | site | `+45*00#` / `+100*00#`, whole arcminutes both ways |
| `:Sg` | `:SgDDD*MM#` | refused with `0`; it takes `:SgsDDD*MM#` and answers `1` |
| `:St` | `:StsDD*MM#` | as documented |
| `:GX#` | not in the protocol | `<state>,<flags>,<RA steps>,<DEC steps>,<TRK steps>,<RA>,<DEC>,<focus>,#` |
| `:hP#` | park | no reply |
| `:hU#` | unpark | `1` |
| `:P#` | toggle precision | no reply |
| `:GW#`, `:Gv#`, `:GVF#` | status, version | no reply |
| `:MT0#` / `:MT1#` | tracking off/on | `1` |

Two of these decide most of what follows.

**`STATUS_PARKED` is zero.** `src/Mount.hpp:110` defines it as
`0B0000000000000000`, and `Mount::getStatusStateString()` opens with
`if (_mountStatus == STATUS_PARKED) status = F("Parked")`. "Parked" is therefore
the name the firmware gives to a mount with no status bit set at all: every idle
mount reports it, wherever its axes are standing, and the `else status = "Idle"`
at the end of that function is unreachable for one. The run saw it at
declination +75, nowhere near the park position.

**Unparking is turning tracking on.** `MeadeCommandProcessor::onUnpark()` is
`_mount->startSlewing(TRACKING)` and nothing else. Park and "not tracking" are
the same state to this firmware, which is what made the two defects below a
deadlock rather than two separate bugs.

## Test asset

`indigo_test/hardware/test_mount_lx200_hw.c`, run with
`make -C indigo_test test-mount-lx200-hw` and `MOUNT_LX200_HW_PORT` pointing at
the board. The file stays at 35 cases; the OpenAstroTracker branch reuses the
model-aware structure the aGotino run introduced and adds its own property
contract assertions.

## Baseline (2026-09-23 10:29, macOS arm64, driver 0x0300003B)

```sh
MOUNT_LX200_HW_PORT=/dev/cu.usbmodem5B7A0424131 make -C indigo_test test-mount-lx200-hw
```

The baseline was stopped after 12 of 35 cases, with **9 failed**, once the root
causes were identified: the mount could not be unparked, so every scenario that
moves or tracks was refused by the parked-mount guard and would only have run
out its own timeout. The 12 cases it did reach are the complete evidence for
LX035 to LX041 and are recorded as the baseline rather than a full run.

That baseline also **overwrote the controller's configured latitude with 0**
(LX041) and its clock with the host clock. The site was restored by hand to the
`+45*00` / `+100*00` it was found with, verified by reading it back.

## Found defects (hardware run)

| ID | Severity | Where | Defect |
| --- | --- | --- | --- |
| LX035 | critical | `meade_get_coordinates()` | The driver never read a declination from this mount at all. `:GD#` answers `+45*00'00`, with the arcminute mark between the arcminutes and the arcseconds, and `meade_parse_coordinate()` rejects anything that does not end after the seconds it recognises. `MOUNT_EQUATORIAL_COORDINATES` stayed in ALERT for the whole session holding the +90 it was attached with, so nothing that reads a position - goto, sync, the working position, every motion check - could work. Fixed by normalising the separator in the OAT branch before parsing, the way the aGotino branch already normalises its degree sign. |
| LX036 | major | `meade_set_site()` | The site write was silently refused. The firmware takes `:SgsDDD*MM#` and answers an unsigned longitude with `0` while keeping the site it had, so the driver reported success, published its own value, and the mount kept another one. Fixed with a signed format in the OAT branch. |
| LX037 | major | `meade_init_oat_mount()` | A parked OpenAstroTracker could not be released. `MOUNT_PARK` was published with a single parked item, so a client clearing it left `MOUNT_PARK_UNPARKED_ITEM` false, the unpark branch of the change handler was never entered, `:hU#` was never sent, and the next poll put the parked item back. The firmware implements both `:hP#` and `:hU#`, so the property is now the two-state switch it always should have been. |
| LX038 | major | `meade_update_oat_state()` | An idle mount was published as parked, which locked a client out of it. `:GX#` reports "Parked" for `_mountStatus == 0`, so turning tracking off made the driver believe the mount had parked itself, and the parked-mount guard then refused to turn tracking back on: parked because not tracking, not tracking because parked. The park state is now the one the driver established with `:hP#` and `:hU#`, confirmed by the status that follows a park it asked for, the way the Astro-Physics branch keeps its own. A session starts released, because the firmware cannot be asked and the other guess is the one that locks the mount. |
| LX039 | major | `meade_update_oat_state()` | `MOUNT_TRACKING` was published off for the length of every goto and every manual move. `Mount::startSlewingToTarget()` and `Mount::startSlewing()` stop the tracking motor and set `_compensateForTrackerOff`, and `Mount::loop()` starts it again when the slew ends, so `:GX#` honestly reports no tracking while the mount is on its way although the setting the client made never changed. The driver now keeps that setting for the duration of a slew. |
| LX040 | minor | `meade_init_oat_mount()` | `MOUNT_INFO.MODEL` stayed on the `Unknown` the generic initialization writes, although the autodetection had already read `OpenAstroTracker` from `:GVP#`. |
| LX041 | major | `meade_init_mount()` | Connecting overwrote the site the mount had with the one the driver held, which on a first connect is the 0, 0 of a profile that was never configured. The trigger is the clock: this controller comes up with a 2021 date, the driver reads that as "mount is not initialized" and wrote both the clock and the site. The clock is what that check is about; the site is now written only when the driver has one, and is always read back afterwards so a write that was refused cannot be published as if it had taken. **This is the one repair outside a model branch in this run** and it applies to every profile. |

### Test defects the run exposed

| ID | Where | Defect |
| --- | --- | --- |
| LXT023 | `lx200_parks_and_unparks` | The manual move that proves the mount runs again after an unpark was issued while the mount was standing on its park position, which on a controller whose declination travel is measured from that position is its own limit. The scenario now takes the mount off the park position with a small goto first. |
| LXT024 | `mount_lx200_simulator.c` | A guide pulse consumed its duration and left the axes exactly where they were, so no hardware-free test could tell a pulse that reached the mount from one that was acknowledged and dropped. Pulses now move the axis they are sent to at half the sidereal rate for as long as they last, and `lx200_guide_pulse_moves_the_mount` pins it down. |
| LXT025 | `test_mount_lx200_simulator.c` | `wait_for_property_state(MOUNT_EQUATORIAL_COORDINATES, OK)` was used as "the controller has been read", but the property is defined in OK state carrying the values the device was attached with. `wait_for_fresh_coordinates()` waits for a publication the polling callback made. |

## Controller behaviour that is not a defect

* **Manual declination motion cannot move this build from its home position.**
  `DEC_LIMIT_UP` and `DEC_LIMIT_DOWN` default to `0.0f` unless `OAM` or `OAE` is
  defined (`Configuration_adv.hpp:243`), and `Mount::startSlewing()` sends the
  axis to `±_stepsPerDECDegree * DEC_LIMIT`, which is step zero, which is where
  the axis already is at home. Both directions are accepted and neither turns.
  Away from home the same command moves the axis back towards zero, which is why
  the manual motion scenarios pass everywhere except straight after a park. This
  is the bench configuration, not the firmware and not the driver.
* **A goto stops the tracking motor.** See LX039; the firmware restores it.
* **`:hU#` answers `1` although the driver sends it as a command without a
  reply.** Every command helper discards pending input before it writes, so the
  stray acknowledgement costs nothing.

## Not covered

* **The controller stopped answering once, during the guide pulse measurement.**
  On the run before the last repair the board went silent in the middle of
  `lx200_measures_guide_pulse_duration`, after tens of pulses in a few seconds,
  and stayed silent: the USB device node was still enumerated and every command
  timed out, which failed that case and the five after it. Individual replies had
  been arriving 6 s late for a while before it went down. It came back only after
  a DTR/RTS reset pulse. The accepted run that follows repeated the same
  measurement with no reply slower than 1045 ms, so it did not reproduce, and
  nothing in this session establishes whether the cause is the firmware, the
  ESP32-S3 USB CDC stack or the bench board. It is recorded as an observed
  hardware-only failure rather than attributed to the driver. What the driver
  does about a controller that stops answering while its port stays open - today,
  time out every command and publish ALERT until it comes back - was not changed.
* **The focuser logical device**, which this driver offers for the OAT profile.
  The bench board has no focus stepper (`FOCUS_STEPPER_TYPE` is not configured),
  so a move could not be told from a command that was ignored.
* **`MOUNT_HOME`.** The driver implements `:hF#` for this profile and
  `meade_update_oat_state()` handles the "Homing" status, but
  `meade_init_oat_mount()` never unhides the property, so home is not offered to
  a client. That is a pre-existing gap, left alone in this run because it is a
  capability to add rather than a defect the run demonstrated.

## Scenario to case mapping, OpenAstroTracker branch

| Checklist scenario | Case | Result |
| --- | --- | --- |
| Identity and capabilities | `lx200_reports_identity_and_capabilities` | vendor, model and firmware from the controller, LX040 |
| Published property contract | `lx200_publishes_the_property_contract` | the OAT branch asserts the two-state park, three tracking rates and everything that stays hidden |
| Site and clock readback | `lx200_reads_site_and_time` | both read from the mount |
| Observing site | `lx200_keeps_the_observing_site` | written back unchanged and read back, LX036 and LX041 |
| Unpark, refusal while parked | `lx200_unparks_the_mount` | LX037 and LX038 |
| Tracking on and off | `lx200_toggles_tracking` | LX038 |
| Tracking rates, rate readback | `lx200_selects_tracking_rates`, `lx200_reports_the_tracking_rate_from_the_mount` | sidereal, solar and lunar through `:XSS` |
| Slew rates | `lx200_selects_slew_rates` | |
| Manual motion, manual abort | `lx200_moves_both_axes_manually`, `lx200_aborts_manual_motion` | LX035 |
| SYNC, GOTO | `lx200_syncs_to_the_current_pointing`, `lx200_slews_to_a_nearby_target` | LX035 |
| Tracking during a slew | `lx200_keeps_tracking_while_slewing` | LX039, the case that found it |
| Abort a slew, accept a fresh one | `lx200_aborts_a_slew_and_accepts_a_fresh_one` | |
| Pier side | `lx200_reports_side_of_pier` | not applicable, the firmware has no pier side |
| Secondary device refusal | `lx200_refuses_the_unsupported_devices` | the aux device refuses, the guider comes up |
| Guiding, all five cases | the guider cases | pulses in both axes, overlap, replacement and duration |
| Home | `lx200_goes_home` | not applicable, `MOUNT_HOME` is not published for this profile |
| Park | `lx200_parks_and_unparks` | LX037, LXT023 |
| Shared serial session, refused port, reconnect, INIT/SHUTDOWN | the four lifecycle cases | |

## Hardware acceptance results (2026-09-23 11:17, driver 0x0300003C)

```sh
MOUNT_LX200_HW_PORT=/dev/cu.usbmodem5B7A0424131 make -C indigo_test test-mount-lx200-hw
```

| Suite | Run | Passed | Failed |
| --- | --- | --- | --- |
| `test_mount_lx200_hw` against an OpenAstroTracker, firmware v1.13.20 | 35 | 35 | 0 |

### Guiding pulse duration accuracy (hardware)

| Requested | n | min | median | p95 | max | mean error |
| --- | --- | --- | --- | --- | --- | --- |
| 20 ms | 12 | 71.97 ms | 80.69 ms | 89.31 ms | 92.64 ms | +61.53 ms |
| 100 ms | 12 | 155.93 ms | 164.05 ms | 172.71 ms | 601.87 ms | +99.83 ms |
| 500 ms | 12 | 555.00 ms | 567.47 ms | 671.27 ms | 1045.07 ms | +128.14 ms |

The floor is the serial path: the driver ends a pulse from its own timer and
every transaction carries the 50 ms settle the command helpers apply, so a 20 ms
pulse cannot complete in less than about 70 ms. This is software completion
timing over the real serial link, not an electrical measurement.

## Simulator additions from this run

The `oat` model of `indigo_test/simulator_common/mount_lx200_simulator.c`
answered the shared command table like any other profile, which is what let all
five protocol defects above look like working code. It now models the
controller:

* `:GX#` answers the real comma separated status word, with "Parked" for an idle
  mount the way `STATUS_PARKED == 0` makes the firmware report it, and with the
  flags field carrying the tracking motor;
* the three stepper positions in that word are derived from the simulated
  motion - the declination stepper from the offset to the home position at the
  314.1 steps per degree measured on the bench controller, the right ascension
  stepper from the distance a running slew still has to cover, and the tracking
  stepper counting down for as long as the mount tracks;
* `:GD#` separates the arcminutes from the arcseconds with the arcminute mark;
* `:Sg` refuses an unsigned longitude with `0` and keeps the site it had;
* a goto stops the tracking motor and the end of the slew starts it again;
* `:hU#` turns tracking on and answers `1`, and `:GVN#` answers `v1.13.20`.

Independently of the profile, **a guide pulse now moves the axis it is sent
to**, at half the sidereal rate for as long as it lasts (LXT024). Before this
a pulse only consumed time, so no hardware-free test could tell a pulse that
reached the mount from one that was acknowledged and dropped.

Four cases pin the findings down hardware-free, so the serial suite grows from
82 to 86:

* `lx200_oat_reads_the_coordinates_and_the_site` - LX035, LX036 and LX040.
* `lx200_oat_idle_is_not_parked` - LX037 and LX038.
* `lx200_oat_keeps_tracking_through_a_goto` - LX039.
* `lx200_guide_pulse_moves_the_mount` - LXT024, and the contract a guider owes
  every profile.

## Simulator results (driver 0x0300003C)

| Suite | Run | Passed | Failed |
| --- | --- | --- | --- |
| `test_mount_lx200_simulator` | 86 | 86 | 0 |

The opt-in TCP target and the ASAN/UBSAN build were not run in this session.

## Final test summary for this run

* Simulated tests: 86 run, 86 passed.
* Hardware tests: 35 run, 35 passed, against an OpenAstroTracker controller, firmware v1.13.20.

# Losmandy Gemini audit (2026-09-23)

## Scope and hardware-test decision

Simulator-only work on the Gemini profile of `indigo_mount_lx200`, starting
from an external audit that had not been confirmed against the code or the
documentation. **Hardware testing is NOT performed**: no Gemini controller is
available and the task explicitly forbids using physical hardware, so nothing
below may be read as hardware validation. Driver version before this work is
`0x0300003C`.

Documentation used, both bundled with the driver:

* `Gemini-5-2.1.pdf` - Gemini Level 5, Version 2.1 Serial Interface Command
  Description, referred to as L5 below.
* `gemini_manual_l4.pdf` - the Level 4 user manual, referred to as L4.

## Audit verification

Every claim was checked against the current `.driver` source and against L5
before any change. All five are confirmed.

| # | Claim | L5 reference | Code | Verdict |
| --- | --- | --- | --- | --- |
| 1 | `:CM#` answers `No object!#` when the mount is not aligned or no object is selected, and `meade_sync()` accepts it as success | "Synchronize" section: `:CM#` → `No object!#` or `<object name>#` | `meade_sync()` fails only on an empty reply and has error branches for ZWO and NYX only | confirmed |
| 2 | The time difference must be set before the date and the local time | `:SG{+-}hh#`: "The time difference has to be set before setting the calendar date (SC) and local time (SL), since the Real Time Clock is running at UTC" | `meade_set_utc()` sends `:SC#`, then optionally `:SH#`, then `:SG#`, then `:SL#` | confirmed |
| 2b | `:GG#` may answer `±hh:mm:ss` and `atoi()` drops the minutes | `:GG#` → `{+-}<hh>#` or `{+-}<hh>:<mm>:<ss>#`, "The extended format with minutes and seconds is new in L5" | `*utc_offset = -atoi(PRIVATE_DATA->response)` | confirmed |
| 3 | Double Precision answers decimal degrees that the parser rejects | `:u#` "NEW in L5 Select the Double Precision mode. Values will be displayed in signed floating point format with 6 digits after the decimal point" | `meade_parse_coordinate()` requires a sexagesimal separator and rejects `+12.345678`; `meade_get_coordinates()` returns false before the `:U#` it would send for a short reply | confirmed |
| 4 | `!` on `:Gv#` means Stall and is ignored | `:Gv#` → `N`, `T`, `G`, `C`, `S` and "! for Stall" | `meade_update_gemini_state()` switches on `S`, `C`, `T`, `G` only, so a stall reads as "no movement" | confirmed |
| 5 | `:h?#` answers `0` both for "no park command received" and for a failed park | `:h?#` → "2: Park operation in progress, 1: Park operation completed, 0: No Prk command received or Park operation failed" | the driver handles `1` and `2` and ignores `0` | confirmed |

The L4 warning is also confirmed and is a constraint on the work rather than a
defect: L4 section 5.3.10.10.2 lets the user swap the meaning of `:CM#` and
`:Cm#` through the "Sync or Align" setting, and L5 documents `:Cm#` as an
*Additional Alignment* that recalculates the pointing model. The driver must
not choose between them on the user's behalf, so this work keeps `:CM#`.

## Baseline

Driver `0x0300003C` builds clean and the full serial suite passes, which is the
state the Gemini work starts from.

```sh
cd indigo_drivers/mount_lx200 && make -f ../../Makefile.drv all
make -C indigo_test build/integration/test_mount_lx200_simulator
./build/integration/test_mount_lx200_simulator
```

| Suite | Run | Passed | Failed |
| --- | --- | --- | --- |
| `test_mount_lx200_simulator` (driver 0x0300003C) | 86 | 86 | 0 |

The `gemini` model of the simulator answers `:Gv#`, `:h?#`, `:Gm#`, `:CM#`,
`:GG#` and the native `>...#` set from the shared command table, always with the
well-behaved reply. None of the five failure replies above can be produced, so
no existing case covers any of them: every one of the five defects is currently
invisible to the suite.

## Plan

Each step is independently verifiable and is verified before the next begins.
The simulator gains a selectable quirk for each failure reply first, so the
regression case fails against `0x0300003C` before the repair is written.

1. Simulator: `:CM#` answers `No object!#` when the model is Gemini and the
   fixture asked for an unaligned mount. Case reproduces the false success.
2. Driver: `meade_sync()` rejects `No object!#` for Gemini with a message.
3. Simulator: record the order of `:SG#`, `:SC#` and `:SL#` in the event log and
   reject a date set before the offset. Case reproduces the wrong order.
4. Driver: `meade_set_utc()` sends `:SG#` before `:SC#` for Gemini.
5. Simulator: `:GG#` answers the extended `±hh:mm:ss`. Case reproduces the
   dropped minutes in the published clock.
6. Driver: parse the extended offset without truncating the UTC computation.
7. Simulator: `:Gv#` answers `!`. Case reproduces the motion that completes
   although the mount stalled.
8. Driver: propagate the stall and define the recovery on the next valid reply.
9. Simulator: `:h?#` answers `0` after a park that failed. Case reproduces the
   park that stays BUSY.
10. Driver: distinguish the initial `0`, the operation in progress and its
    failure, with a bounded completion.
11. Double Precision: negotiate or support the format without sending a blind
    toggle, and check the effect on the time and the other readbacks.
12. Regenerate, run the full serial suite, update the records.

Deferred and assessed separately, not mixed into the repairs: the native
tracking mode through command 130, the guide rate through 150 and through
151/152 on L5, reading the version with `:GVN#` to tell L4 and L5 capabilities
apart, and decoding the specific `:MS#` rejection reasons. Any native command
work must implement and verify the documented checksum.
