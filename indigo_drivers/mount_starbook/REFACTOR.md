# Vixen StarBook code-generator migration

Status: simulator-backed migration completed on 2026-09-13. Baseline driver version: `0x02000004`; generated version: `0x03000005`.

## Instructions and sources reviewed

- `AGENTS.md`, `indigo_drivers/AGENTS.override.md`, `indigo_test/AGENTS.md`, `indigo_test/DRIVER_TESTING_RULES.md`, `indigo_docs/DRIVER_GENERATOR_MIGRATION.md`, and `indigo_docs/SERIAL_DEVICE_SIMULATORS.md`.
- The current driver source/header/main, driver README, host-side HTTP simulator, integration test, properties reference, migration table, Makefile and Xcode registrations.
- The untracked `vixen-starbook-protocol.md` already present in the working tree was read as a secondary reverse-engineering summary and is preserved as user-owned work. There is no bundled manufacturer protocol specification; exact behavior is therefore grounded in the existing driver, simulator traces, and deterministic response tests.

## Current-state audit

`indigo_mount_starbook.c` is a hand-written HTTP driver exposing a mount and guider logical device that share one cURL session and private-data allocation. It supports firmware discovery, old StarBook versus StarBook TEN capability gates, coordinate and status polling, GOTO/SYNC, manual movement, abort, slew speed, location, UTC and timezone, tracking/side-of-pier readback, reset, pseudo-park through STOP, and four-direction pulse guiding.

The current lifecycle is hand-written and uses reference counting, pthread mutexes, cURL global/session ownership and multiple INDIGO timers. Operations are initiated through timers rather than generator-owned handler queues. Poll callbacks perform synchronous HTTP requests every 0.5 seconds. Reset blocks its callback for one second. Guider completion is only a local timer; transport failure is ignored, the RA handler accidentally sets the DEC property BUSY, pending pulse timers are not cancelled on disconnect, and shared-session shutdown is inconsistent: mount disconnect stops motion but does not close the last session, while guider disconnect calls `starbook_close(device)` with the guider property context.

The HTTP helper accepts a timeout argument but does not apply it to cURL, constructs host/port strings with missing explicit terminators that rely on zero-filled allocation, and cannot distinguish truncated response bodies from valid replies. Open is not fully transactional after cURL initialization and tolerates failed status/time initialization. Polling failures do not consistently publish terminal property states or initiate bounded transport recovery.

The public custom properties are currently `STARBOOK_TIMEZONE` and `STARBOOK_RESET`. They violate the repository rule requiring driver-specific properties to start with `X_`; the generated migration will intentionally expose `X_STARBOOK_TIMEZONE` and `X_STARBOOK_RESET` and update tests and `indigo_docs/PROPERTIES.md`. Standard property names remain unchanged.

The current simulator is a minimal loopback HTTP server. GOTO completes immediately, coordinates are partly hard-coded, pulse duration/direction is not recorded, unsupported and malformed requests generally receive `OK`, and there is no deterministic trace or reply/drop fault injection. The existing two integration cases are smoke checks: they do not establish post-request revisions, exact command encoding, capability branches, asynchronous movement, abort/reversal, malformed replies, failures and recovery, shared mount/guider ownership, active disconnect, reconnect, or guider timing accuracy.

Unsupported or deliberately limited behavior: the protocol-backed driver exposes no home, custom tracking rate, device-side guide rate, PEC, encoder or alignment-model controls. Park is a legacy STOP operation and immediately returns to the unparked switch value; tests must document this instead of inventing physical park support. Old firmware hides tracking and side of pier. Generic astrometry is framework-owned and outside driver protocol coverage.

Build/project audit: Makefile and Xcode already register the hand-written C/H/main, simulator, and test. The new `.driver` and this record will require Xcode registration. No Visual Studio driver project is currently registered, so portable source and Windows project/solution integration must be added; native Windows execution is unavailable here. `MIGRATION_STATUS.md` currently records API 2, no Windows/generator/async/retesting, and 2/0 automated tests; its Comment column is empty and will remain unchanged.

## Baseline before production changes

- Environment: macOS, universal x86_64/arm64 clang build; runtime requires permission to bind a loopback TCP socket.
- `make -B -C indigo_drivers/mount_starbook -f ../../Makefile.drv all` — passed, producing the archive, dylib, and executable from the unmodified driver.
- `make -B -C indigo_test build/integration/test_mount_starbook_simulator` — passed, producing the unmodified simulator and two-case integration binary.
- `(cd indigo_test && ./build/integration/test_mount_starbook_simulator)` — first sandboxed run failed because `bind()` was denied; the approved loopback-enabled rerun passed both baseline cases (2/2).
- The baseline pass establishes only the current smoke behavior, not the missing protocol, lifecycle, failure, concurrency, or timing coverage listed above.

## Hardware-test decision

No physical Vixen StarBook or StarBook TEN controller was supplied or identified as available. Hardware tests run/passed are therefore 0/0 and will not be claimed. The host-side simulator can validate request formatting, response parsing, public property transitions, lifecycle, and software pulse timing, but cannot validate mechanics, pointing/tracking accuracy, physical guide output, firmware-specific HTTP fragility, or real network interruption.

## Atomic migration plan

1. **Complete — audit and baseline.** Read the applicable rules and sources, identified lifecycle/protocol/test gaps, built the original driver and test, and recorded the 2/2 approved-loopback baseline.
2. **Complete — extract and normalize the generator definition.** Reverse-extracted with `build/bin/indigo_generator -c indigo_drivers/mount_starbook/indigo_mount_starbook.driver`, inspected and replaced the incomplete skeleton, made `.driver` the source of truth, advanced the version to generated 3.0.5, preserved public device names, and corrected the driver copyright owner to Koji Tsunoda as explicitly directed by the user.
3. **Complete — establish transactional shared HTTP lifecycle.** Removed cURL and its global/session state. Each bounded HTTP/1.0 transaction now uses portable `indigo_uni_open_url()` TCP I/O, validates the HTTP status and complete HTML terminator, and reuses the private response buffer. Generator-owned reference counting handles either mount/guider connection order and final release without a persistent transport sentinel.
4. **Complete — move behavior onto generated queues.** Generated handlers own changes and shared serialization. GOTO uses a named `mount_goto_finalizer`, periodic polling remains a polling callback, guide completion uses time-priority finalizers, disconnect cancels pending work, and a 600-second GOTO deadline provides bounded failure recovery.
5. **Complete — correct property and protocol defects.** Custom names are `X_STARBOOK_TIMEZONE` and `X_STARBOOK_RESET`; numeric/key-value/status responses are validated; old and TEN firmware paths are retained; near-Sun GOTO receives the required second confirmation; RA guiding updates RA rather than DEC; command failures publish ALERT and recover on a fresh request.
6. **Complete — make the simulator deterministic.** The simulator now models multi-poll GOTO progress, manual coordinate movement, firmware selection, timestamped request traces, strict unknown-command errors, one-shot reply replacement and dropped responses, and reconnect-safe TCP operation.
7. **Complete — expand mount and guider integration coverage.** Eleven independent cases use post-request revisions and traces to cover the property contract, new/old firmware, exact high-precision signed SYNC/GOTO, BUSY/progress/readback, movement/rates/reversal/abort, location/timezone/reset/park commands, response and transport failures, near-Sun confirmation, connection rollback, shared logical-device ownership, active disconnect/reconnect, all pulse directions/durations, and timing under idle and polling load. Unsupported home, writable tracking/rates, PEC, encoders and physical park remain non-applicable.
8. **Complete — integrate and document.** Xcode contains the `.driver` and refactoring record. Added an x64/ARM64 Visual Studio driver project and filters, solution registration and server reference. `PROPERTIES.md` records the prefixed names and `.driver` source; `MIGRATION_STATUS.md` records API 3, Windows source/project support, generator/queue completion, simulator retesting and 11/0 tests without changing its Comment cell. README is unchanged.
9. **Complete — verify and clean.** Repeated unchanged generation produced identical C/H/main SHA-1 values (`c7498b…`, `f0ef9d…`, `ca1c37…`). The universal driver build and a direct `-Wall -Wextra -Werror` driver compile passed; simulator/test strict compiles passed with only shared-harness unused warnings suppressed. Normal and native arm64 ASan+UBSan suites passed 11/11 (`detect_leaks=0` because macOS leak detection is unavailable). Generator architecture, Xcode plist, Visual Studio XML and `git diff --check` passed. `make -C indigo_test test-clean`, the driver clean target and removal of named `/tmp` strict/sanitizer files removed task artifacts.

## Scenario-to-test mapping

- `starbook_mount_passes_http_compliance_checks`: mount interface, identity/property items and basic rate, motion, abort and timezone transitions.
- `starbook_guider_passes_http_compliance_checks`: guider interface/items and basic DEC/RA pulse completion.
- `starbook_firmware_capabilities_are_gated`: legacy 2.70 identity, traditional coordinate parser and hidden TEN-only tracking/pier properties.
- `starbook_sync_and_goto_have_exact_protocol_and_progress`: signed sub-degree SYNC, exact high-precision requests, GOTO BUSY/completion and final readback.
- `starbook_manual_motion_rate_abort_and_recovery`: exact speed/directional combinations, simultaneous axes, reversal, STOP, all-axis release and fresh movement.
- `starbook_settings_and_commands_are_translated`: western/southern location plus timezone encoding, reset and legacy park/STOP semantics.
- `starbook_command_failure_recovers`: protocol ERROR propagation and a successful fresh rate request.
- `starbook_transport_drop_and_near_sun_retry_recover`: absent HTTP reply, fresh movement recovery and required near-Sun GOTO confirmation.
- `starbook_connection_failure_recovers`: malformed version response, transactional failed connection and successful retry.
- `starbook_shared_lifecycle_survives_active_disconnect`: mount/guider shared ownership, sibling operation, disconnect during GOTO, reconnect and fresh motion.
- `starbook_guider_directions_and_timing`: exact four-direction `MOVEPULSE` requests at 20/100/500 ms and 12 samples each under idle and mount-poll workloads.

## Guider timing evidence

The final normal run measured software property completion from request submission; it does not claim physical guide-output timing. Idle: 12 samples, mean signed error 4.255 ms, maximum absolute error 6.408 ms. With mount polling active: 12 samples, mean signed error 4.739 ms, maximum absolute error 7.879 ms. The ASan+UBSan repetition measured 4.912/7.209 ms idle and 5.050/6.708 ms under polling (mean signed/max absolute).

## Final summary

Distinct simulator tests run/passed: **11/11**. Hardware tests run/passed: **0/0**. The simulator suite was repeated under ASan+UBSan. Native Windows/Linux compilation and physical firmware/mechanics/guide-output behavior remain unverified and are not claimed.

## Overlapping guide pulses (2026-09-20)

`GUIDER_GUIDE_RA` and `GUIDER_GUIDE_DEC` now declare `accept_while_busy = true` and zero both axis
items in `on_change_request`, replacing the earlier workaround that forced the property state back
to `INDIGO_OK_STATE` so the BUSY-guarded dispatch macro would let the request through. Zeroing the
items also closes a gap the workaround left open: a reversing request kept the superseded direction
set, so the handler picked the stale direction and re-sent `MOVEPULSE` the wrong way. The driver
now uses the same pattern as every other INDIGO driver that exposes a guider.

`starbook_guider_passes_http_compliance_checks` gained two duration-measuring cases: a 2000 ms
pulse replaced after 500 ms by a 600 ms pulse in the same direction, and the same pulse replaced by
a 300 ms pulse in the opposite direction. Waiting only for the property to leave BUSY passes even
when the second request is discarded, so the replacement was not covered before.

## Number and UTC targets (2026-09-27)

Version 11, finding TGT-C03 and the starbook part of TGT-B04 of `indigo_drivers/REVIEW_SWITCH_TARGETS.md`.
No hardware run for this change.

- TGT-C03: the status poll wrote the mount clock into `MOUNT_UTC_TIME` and published it OK on every pass,
  also over a pending request, so the handler sent the mount its own time back and reported OK. The change
  branch records the request with `indigo_mount_set_utc_target()`, the handler sends
  `indigo_mount_get_utc_target()` and writes it into the items once the mount accepted it (ALERT
  otherwise), and the poll still reads `/GETTIME` on every pass but writes items and state only while the
  property is not BUSY.
- TGT-B04: `GUIDER_GUIDE_RA`/`DEC` accept a pulse while one runs. A pulse copied on the bus thread while the
  previous pulse's finalizer ran was zeroed by that finalizer and its handler read 0 and reported OK
  without sending `/MOVEPULSE`. The finalizers already cleared only the values; the handlers now restore
  them from the targets, which `on_change_request` still clears before every copy.

The simulator reads a runtime control `delay <path> <ms>` from `<ready-file>.control`: the next matching
request removes the file and is answered late. Regression tests in
`integration/test_mount_starbook_simulator.c`, each failing against version 10 and passing with version 11:

- `starbook_utc_request_survives_poll`: the poll's `/GETTIME` is answered 1 s late and a time is requested
  meanwhile. Before: the mount got `/SETTIME?TIME=2026+08+24+22+15+30` (its own clock) instead of the
  requested `2026+09+13+14+34+56`, and the poll published OK before the handler ran.
- `starbook_guider_pulse_survives_previous_finalizer`: only the guider is connected, a 100 ms pulse is
  sent and the opposite 300 ms pulse is requested from the queue's trace line (`Executing task ...`) of the
  next TIME task after the pulse handler, i.e. the finalizer, before it runs; on both axes. Before: no
  second `/MOVEPULSE`; now `DIRECT=3`/`DIRECT=1` with `DURATION=300` is sent.

The finalizer clears the values before any I/O or driver log line, and the guide handlers are TIME tasks
without delay, so they run ahead of an overdue finalizer and cancel it: holding the queue with a late
`/GETSTATUS2` does not reproduce the window (checked, the case passed against version 10). The queue's
trace of the task it is about to run is the only line inside the window; the test lowers the log level to
debug before it sends the request from that line, so the request path does not log while the trace holds
the log lock.

Recorded run `TZ=Europe/Bratislava python3 tools/run_driver_test.py mount_starbook` on Linux x64:
13/13. MIGRATION_STATUS.md hardware-free count 11 -> 13.

### Final test summary for this change

Simulated tests: **13 run, 13 passed** (recorded run). Hardware tests: **0 run, 0 passed**.

## Mount testing rules coverage (2026-10-04)

Version 12. The test was checked against the extended "Mount Driver Test Standard" and the guider standard in
`indigo_test/DRIVER_TESTING_RULES.md`. No hardware run for this change.

### Driver defects fixed

- Coordinate encoding truncated instead of rounding, so the largest sub-unit values were sent as `60.000` minutes
  (RA `7h59.9996m` became `7+60.000`), RA just below 24 h was never wrapped, and the low precision DEC dropped a
  minute to floating point error (`48 08'` was sent as `+48+07`). Coordinates are now rounded in the unit sent with
  carry into the hour/degree, 24 h wraps to 0 h, DEC is clamped to the pole and a value that rounds to zero is sent
  with `+`.
- `/SETPLACE` truncated minutes the same way (`N48+08` read from the mount was written back as `N48+7`), and an INDIGO
  longitude above 180 degrees (INDIGO is 0 to 360 east) was sent as `E289+30` instead of `W70+30`. The readback
  published negative longitudes; it now publishes 0 to 360 degrees.
- A malformed `/GETPLACE` reply at connect failed the whole connection. It now marks only `GEOGRAPHIC_COORDINATES` and
  `X_STARBOOK_TIMEZONE` ALERT; the next site or time zone write reads the site again first, so the combined
  `/SETPLACE` never resends unknown values.
- `X_STARBOOK_TIMEZONE` sent `/SETPLACE` on an old StarBook outside INIT, which the site write already refused; it
  now ends ALERT without a command as well.
- `MOUNT_PARK_POSITION` and `MOUNT_PARK_SET` were defined although park is only `/STOP` and never uses a position;
  they are hidden now.
- A refused GOTO/SYNC (and the refused START before it) published ALERT with the requested target as the position,
  because the change copied it into the values. The handler restores the mount's last read position, and a SYNC
  publishes OK with the position read back from the mount.
- `MOUNT_TRACKING` and `MOUNT_SIDE_OF_PIER` were republished on every poll although nothing changed:
  `indigo_set_switch()` (indigo_libs/indigo_bus.c) compares the previous value of every other item of a one-of-many
  switch with the value being set instead of with false, so it marks them for update each time. The poll now sets the
  switch only when the reported item is not already on. The framework function itself is unchanged.
- Disconnecting during manual motion left the motion items ON (and a GOTO BUSY) in the next session, although the
  disconnect had stopped the mount. The disconnect now clears them.

### Simulator additions

`--tracking`, `--pierside`, `--place`, `--init` (INIT state that refuses GOTO/ALIGN/MOVE until START) and
`--goto-polls`; a GOTO now moves the position towards the target on each status poll, so an abort lands mid-slew.
Runtime control commands `fault <path> <body>` (repeatable to fail several requests), `drop <path>`, `track <n>`
and `pierside <n>` next to `delay`.

### New cases

- `starbook_connect_publishes_device_state_and_capabilities`: non-default device state at connect (tracking off, west
  of pier, south-western site, time zone), identity fields, negative capability contract (no track/guide rate, home,
  park position/set, legacy unprefixed names; two coordinate-set items; one park item; read-only epoch, tracking and
  pier side), read-only requests send nothing and change no revision, hand-controller changes published once,
  disconnect/reconnect with no update of an undefined property.
- `starbook_firmware_thresholds_select_commands`: 2.70/2.71/4.19 on both sides of both thresholds, commands a firmware
  lacks never sent while polling, low precision GOTO string and readback, old firmware refuses site, time zone, UTC
  and host time outside INIT without a command.
- `starbook_init_state_start_precedes_motion`: START?INIT=OFF precedes the GOTO, a refused START sends no GOTO and keeps
  the position, plain START on 2.70 before motion, UTC accepted in INIT on 2.70.
- `starbook_goto_refusals_report_reason_and_recover`: BELOW HORIZON, ILLEGAL STATE, FORMAT and an unknown error each
  end ALERT with the reason in the message after one GOTO, position kept, dropped reply, double NEAR SUN, SYNC not
  repeated on NEAR SUN, next GOTO accepted.
- `starbook_goto_busy_poll_and_abort_mid_slew`: a poll in flight does not complete a GOTO, a second GOTO while BUSY is
  not sent, abort mid-slew sends one STOP, ends ALERT, two equal fresh readbacks short of the target.
- `starbook_poll_faults_publish_alert_and_recover`: malformed status, out-of-range DEC, tracking and clock replies
  mark only their property ALERT with the last values and recover; a reply lost during a GOTO does not leave it BUSY.
- `starbook_guider_zero_failure_axes_and_disconnect`: zero pulses send nothing, refused pulse ALERT and recovery,
  simultaneous RA/DEC with independent completion, guider-only session never polls the mount, disconnect with a
  pulse pending and reconnect without stale BUSY.
- `starbook_goto_during_guide_pulse`: GOTO accepted and completed while a guide pulse runs.
- `starbook_disconnect_during_motion_stops_mount`: release and STOP before the session ends, no request after
  disconnect, motion items off and nothing BUSY after reconnect, for manual motion and a GOTO.
- `starbook_manual_motion_released_when_owner_detaches`: motion of a detached client is released with the all-off
  `/MOVE`.
- `starbook_momentary_switches_and_host_time`: reset, host time, abort and park return to off; host time sent as host
  local time; refused park ALERT and retry; park never latches.
- `starbook_coordinate_encoding_rounds_and_carries`: carries, 24 h wrap, negative zero degree, both poles, both
  precisions.
- `starbook_site_write_keeps_combined_values`: malformed site at connect, re-read before the time zone write,
  minute carry, prime meridian on both sides, west sign, reconnect readback in INDIGO units, refused write and retry.

Existing cases extended: SYNC readback asserted on later polls without a GOTO command; manual motion checks that
north raises DEC and east raises RA on the polled readback.

### Not covered, with reason

- A GOTO the controller reports done short of the target ends OK: the driver ends on `GOTO=0` without comparing the
  position, and the tolerance a real StarBook keeps after its pointing model is unknown without hardware.
- The 600 s GOTO deadline is not exercised (too long for the suite).
- Abort and park send `/STOP`, which stops tracking on the simulator; whether the real StarBook keeps tracking after
  `/STOP` is not documented, so "abort while idle keeps tracking" is not asserted.
- An unknown refusal reaches the client as "UNKNOWN", not the controller's text, because the message is used as a
  format string.
- Not applicable: track/guide rate, home, PEC, alignment controls, pier-side writes (read-only), transports other than
  HTTP/TCP, meridian options.

Recorded run `python3 tools/run_driver_test.py mount_starbook` on macOS arm64: 26/26 OK (13 existing + 13 new cases).
Hardware tests: 0 run, 0 passed.
