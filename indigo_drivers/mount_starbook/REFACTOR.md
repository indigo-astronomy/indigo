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
