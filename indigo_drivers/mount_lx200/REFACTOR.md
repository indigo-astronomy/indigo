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
