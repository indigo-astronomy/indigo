# Mount Simulator queue refactoring

Status: complete on 2026-09-13. Baseline driver version: `0x0300000B`. Refactored driver version: `0x0300000C`.

## Workflow correction

This record was created after production edits had already started because `indigo_drivers/AGENTS.override.md` was initially missed. That violated the required ordering even though the implementation and tests were subsequently audited. The baseline below was therefore reconstructed from the unmodified `HEAD` sources, and the plan records completed work and its actual evidence instead of pretending that the ledger preceded it. The final verification is being repeated after this correction.

## Instructions and sources reviewed

- `AGENTS.md`, `indigo_drivers/AGENTS.override.md`, `indigo_test/AGENTS.md` and `indigo_test/DRIVER_TESTING_RULES.md`.
- Top-level `README.md`, `indigo_docs/DEVELOPMENT.md`, the handler-queue guidance in `indigo_docs/DRIVER_DEVELOPMENT_BASICS.md`, `indigo_docs/PROPERTIES.md` and `indigo_docs/SERIAL_DEVICE_SIMULATORS.md`.
- `indigo_mount_simulator.c`, its public header and standalone main, the root driver makefiles, Xcode and Visual Studio project entries, the existing mount-simulator integration test, and the shared simulator test harness.

## Current-state audit

### Architecture and observable behavior

The mount simulator is a hand-written, in-process software driver. It has no `.driver` generator input, protocol transport, vendor SDK, external simulator process, firmware, or physical-device resource. One private-data allocation is shared by two logical devices:

- `Mount Simulator` exposes the mount interface, simulated equatorial position, GOTO and SYNC, park and home positions/actions, RA/DEC manual motion, four slew rates, five tracking-rate selections, custom tracking rate, guide rate, side of pier, epoch, alignment selection and mount state.
- `Mount Simulator (guider)` exposes the guider interface, four pulse directions on independent RA/DEC axes, and RA/DEC guide rates.

The complete visible connected mount-property inventory is `MOUNT_INFO`, `GEOGRAPHIC_COORDINATES`, `MOUNT_LST_TIME`, `MOUNT_PARK`, `MOUNT_PARK_SET`, `MOUNT_PARK_POSITION`, `MOUNT_HOME`, `MOUNT_HOME_SET`, `MOUNT_HOME_POSITION`, `MOUNT_SLEW_RATE`, `MOUNT_MOTION_DEC`, `MOUNT_MOTION_RA`, `MOUNT_TRACK_RATE`, `MOUNT_CUSTOM_TRACKING_RATE`, `MOUNT_TRACKING`, `MOUNT_GUIDE_RATE`, `MOUNT_ON_COORDINATES_SET`, `MOUNT_EQUATORIAL_COORDINATES`, `MOUNT_HORIZONTAL_COORDINATES`, `MOUNT_ABORT_MOTION`, `MOUNT_ALIGNMENT_MODE`, `MOUNT_EPOCH`, `MOUNT_SIDE_OF_PIER` and `MOUNT_STATE`. The guider adds `GUIDER_GUIDE_DEC`, `GUIDER_GUIDE_RA` and `GUIDER_RATE`. Standard common connection/configuration properties remain framework-owned. No property or item is added or removed by this refactoring, so `indigo_docs/PROPERTIES.md` needs no change.

### Baseline synchronization, lifecycle and risks

Version `0x0300000B` used INDIGO timers for connection dispatch, periodic position updates, manual-motion progress and guider pulse completion. A private `pthread_mutex_t` protected position-related fields between timer callbacks. Timer cancellation and shared private state were distributed across disconnect and shutdown paths. The principal risks were timer/change-callback races, stale completion after disconnect, abort racing a queued operation start, and teardown of the master logical device while guider work could still reference shared state.

The refactored ownership model serializes state mutation through the INDIGO handler queues. The guider is a logical child of the mount, so both use the mount's master queue while callbacks retain their originating logical-device context. Delayed queue handlers own recurring position/manual-motion work and pulse completion. Disconnect cancels the pending handlers for that logical device; shutdown detaches the child before the master. There are no mutexes or INDIGO timers left in the production source.

The driver has no hardware/transport reply path, so malformed replies, open/write/read failures, physical hot-plug, SDK rollback and protocol-command assertions are not applicable. Driver-owned operational failures remain relevant: parked-motion rejection, overlapping requests, abort, disconnect with work pending, connected-shutdown rejection and subsequent recovery.

### Build, packaging and platform scope

The simulator is a stable driver in the root makefile, with dynamic/static/standalone targets through `Makefile.drv`, an Xcode group/targets, and existing Visual Studio project/filter files. The new `REFACTOR.md` is registered in the Xcode mount-simulator group. No generated output exists. The `MIGRATION_STATUS.md` row is updated to `Async Queues = ✅ Yes` and `Retested = ✅ Sim`; its API, Windows, generator, automated-test and Comment fields are preserved.

The available host is macOS 26.6.2 on arm64 (`Darwin 25.6.0`). The repository driver/test build emits universal x86_64/arm64 Mach-O artifacts. Both arm64 native and x86_64 Rosetta executions were tested. Linux and Windows builds/runs, and Windows project loading, are unavailable in this environment and are not claimed.

### Existing tests and concrete gaps

The baseline suite contained 7 public-bus cases: metadata, mount and guider property enumeration/compliance, guider replacement/disconnect/reconnect, and mount manual-motion disconnect. It did not cover GOTO/SYNC completion, overlap, abort-before-start recovery, park/home and parked guards, both manual axes/directions, rate selections, simultaneous guider axes, connected shutdown for each logical device, or quantitative guider timing. Those gaps are addressed below except for non-applicable hardware/transport behavior and unavailable platform execution.

## Baseline evidence

Source baseline: unmodified mount-simulator production and test files from `HEAD` (`ad6240f71`, whose only change after `e94ece0fd` is repository instructions). Environment: macOS 26.6.2, arm64 execution.

The initial command used the existing driver archive and exposed a stale-build mismatch rather than a driver defect:

```text
make -C indigo_test build/integration/test_mount_simulator && indigo_test/build/integration/test_mount_simulator
```

Result: failed in `indigo_set_switch` with `property->type == INDIGO_SWITCH_VECTOR`; the test executable had linked a stale archive. This is retained as a setup failure, not counted as baseline behavior.

The baseline source and original test were then compiled separately from `git show HEAD:...` into `/tmp/indigo_mount_simulator_baseline.o`, `/tmp/indigo_mount_simulator_baseline.a` and `/tmp/test_mount_simulator_baseline`, using current repository headers and libraries, and run as:

```text
/tmp/test_mount_simulator_baseline
```

Result: all original 7/7 integration scenarios passed on arm64. This reconstruction happened after implementation began because of the workflow deviation noted above.

## Hardware-test decision

No hardware testing will be performed. This driver itself is the software simulator and has no supported hardware model, transport, relay, firmware or external manufacturer protocol. Hardware tests run/passed are therefore explicitly 0/0. Guider measurements below cover request-to-public-property software completion only and do not claim electrical ST4 pulse accuracy.

## Atomic plan and actual results

1. **Audit instructions, architecture, properties, baseline and test gaps — completed with workflow deviation.** The required driver override was discovered after production editing began. The audit was reconstructed from repository sources and the unmodified `HEAD` baseline. The first stale-archive attempt failed; the isolated baseline then passed 7/7.
2. **Replace timer/mutex position processing with queue-owned state — completed.** `position_handler` now reschedules itself with `indigo_execute_handler_in`; `manual_motion_finalizer` provides delayed manual progress. All timer members/calls and the `pthread_mutex_t` lifecycle/lock calls were removed. Static source scan finds no `pthread`, `mutex`, `indigo_set_timer`, `indigo_cancel_timer` or `indigo_reschedule_timer` reference.
3. **Queue mount connection and writable operations — completed.** Connection, park, home, GOTO/SYNC, tracking and manual motion have named handlers. Urgent abort cancels pending starts and finalizers, publishes terminal states and permits a fresh GOTO. A new test exposed abort-before-GOTO-start as a race; recognizing the already-published BUSY coordinate property as pending fixed it. A pre-final ASan repeat then exposed nondeterministic RA/DEC completion state from one shared start handler; separate RA and DEC handlers removed the cross-property completion and three normal plus three ASan arm64 repeats passed afterward.
4. **Queue guider work and establish shared ownership — completed.** RA/DEC starts and finalizers use time-critical queue priority, same-axis replacement cancels stale completion, axes complete independently, and guide-rate changes are queued. Setting `guider->master_device = mount` serializes shared private state; guider-first detach prevents child work after master teardown.
5. **Extend deterministic public-bus functional coverage — completed.** The suite grew from 7 to 16 named cases. It covers completion and readback, state transitions, overlap/replacement, stale-finalizer suppression, abort and fresh-operation recovery, disconnect/reconnect, both logical-device connection orders and sibling survival, queue cleanup, logical-device shutdown rejection, zero-pulse semantics and the supported property/rate branches. Final normal and ASan validation passed on both built architectures.
6. **Add the required guider timing measurement — completed.** The separate opt-in `bench_mount_simulator_timing` target measures all four directions at 20, 100 and 500 ms, with one discarded warm-up and four retained samples per combination, under idle polling and simultaneous mount motion. It reports every required signed and absolute statistic without imposing a host-dependent precision assertion on the normal suite. Per benchmark rules it reports incomplete setup/sample acquisition but always exits zero; functional pass/fail remains in the integration suite. The recorded measurement completed; detailed results follow.
7. **Update integration, migration status and versioning — completed.** `DRIVER_VERSION` increased from `0x0300000B` to `0x0300000C`; normal, ASan and timing make targets are present; `REFACTOR.md` is in the Xcode group. `MIGRATION_STATUS.md` now records async queues and simulator retesting while preserving its Comment. No README, public property documentation or generator output change is required.
8. **Run strict, sanitizer and available-platform validation — completed.** The universal repository driver build and direct `-Wall -Wextra -Werror` compile passed. Three normal and three driver/test-instrumented ASan arm64 repeats passed, followed by normal and ASan x86_64 Rosetta runs. Shared `libindigo` was not sanitizer-instrumented. Linux and Windows remain unavailable.
9. **Final diff audit, repeated validation and cleanup — completed.** Static scan confirmed that the production source contains no mutex, pthread or INDIGO timer calls. `plutil -lint indigo.xcodeproj/project.pbxproj` and `git diff --check` passed, no README diff exists, version/project registration were verified, and `make -C indigo_test test-clean` removed generated test and benchmark outputs. Explicit baseline/strict objects in `/tmp` were also removed.

## Scenario-to-test coverage

| Area | Named tests and result |
| --- | --- |
| Metadata and property contract | `driver_info_reports_simulator_metadata`, `mount_exposes_expected_properties`, `mount_guider_exposes_expected_properties`, `mount_passes_mount_compliance_checks`, `mount_guider_passes_guider_compliance_checks`: passed |
| Connection, lifecycle and cleanup | `guider_pending_disconnect_and_replacement`, `mount_manual_motion_disconnect`, `mount_shutdown_is_rejected_while_connected`, `guider_shutdown_is_rejected_while_connected`, `logical_devices_survive_both_connection_orders`: passed; covers each logical device alone, both orders, sibling survival and public INIT/SHUTDOWN |
| GOTO, overlap, abort and recovery | `mount_goto_runs_on_queue_and_rejects_overlap`, `mount_abort_allows_fresh_goto`: passed with BUSY/progress/terminal state and fresh revisions |
| Park, home and guards | `mount_park_home_and_parked_guards`: passed for initial parked rejection, unpark, home, park, state lights and parked tracking rejection |
| Manual motion and rates | `mount_manual_axes_reverse_and_abort`, `mount_supports_all_rate_modes`: passed for both axes/directions, reversal, urgent abort, four slew rates, five track rates, custom rate and guide rate |
| Guider directions, axes and replacement | `mount_guider_passes_guider_compliance_checks`, `guider_axes_complete_independently`, `guider_pending_disconnect_and_replacement`: passed for all directions, simultaneous axes, independent completion, same-axis replacement, disconnect/reconnect and a fresh pulse |
| Timing under shared work | `guider_software_timing_under_mount_workload`: passed as a separate measurement scenario under idle polling and active mount motion |

Generic numeric clamping, alignment-model mathematics and generic configuration persistence are framework-owned and intentionally not duplicated. Protocol/SDK errors, physical transport loss, relay masks/edges and hardware timing are not applicable because no such boundary exists. The test observes real production queues and public property revisions; it does not replace queue behavior with synchronous stubs.

## Guider software-completion timing

Measurement endpoint: monotonic time immediately before public guide-property request dispatch through the INDIGO bus to observation of that same public property returning `OK`. This includes bus, queue scheduling and polling latency; it is not the duration of a physical relay output. Idle workload still includes the mount's recurring position polling. Active workload additionally holds simultaneous RA and DEC manual-movement switches while the guider shares the mount master queue.

For each row, `n=4` retained samples after one discarded warm-up. Values are milliseconds. `error = actual - requested`; `%` is mean signed error/requested. With four samples, the nearest-rank p95 and p99 equal the maximum.

| Workload | Direction | Request | Actual mean | Error min | Error mean | Error median | Error p95/p99/max | Stddev | Max abs | Error % |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| idle-polling | EAST | 20 | 23.261 | 1.700 | 3.261 | 3.044 | 5.256 | 1.276 | 5.256 | 16.305 |
| idle-polling | EAST | 100 | 102.921 | 1.651 | 2.921 | 2.841 | 4.352 | 1.190 | 4.352 | 2.921 |
| idle-polling | EAST | 500 | 503.495 | 1.595 | 3.495 | 3.590 | 5.206 | 1.595 | 5.206 | 0.699 |
| idle-polling | WEST | 20 | 24.186 | 1.534 | 4.186 | 4.773 | 5.664 | 1.615 | 5.664 | 20.929 |
| idle-polling | WEST | 100 | 103.539 | 1.946 | 3.539 | 3.528 | 5.152 | 1.424 | 5.152 | 3.539 |
| idle-polling | WEST | 500 | 502.968 | 1.148 | 2.968 | 3.111 | 4.500 | 1.435 | 4.500 | 0.594 |
| idle-polling | NORTH | 20 | 22.602 | 1.695 | 2.602 | 2.311 | 4.091 | 0.928 | 4.091 | 13.010 |
| idle-polling | NORTH | 100 | 103.122 | 1.270 | 3.122 | 3.442 | 4.335 | 1.285 | 4.335 | 3.122 |
| idle-polling | NORTH | 500 | 501.840 | 0.857 | 1.840 | 1.897 | 2.710 | 0.770 | 2.710 | 0.368 |
| idle-polling | SOUTH | 20 | 23.302 | 1.259 | 3.302 | 3.379 | 5.191 | 1.495 | 5.191 | 16.510 |
| idle-polling | SOUTH | 100 | 104.432 | 0.987 | 4.432 | 5.414 | 5.911 | 2.020 | 5.911 | 4.432 |
| idle-polling | SOUTH | 500 | 502.076 | 0.663 | 2.076 | 2.013 | 3.617 | 1.057 | 3.617 | 0.415 |
| mount-motion | EAST | 20 | 23.588 | 1.802 | 3.588 | 3.453 | 5.642 | 1.459 | 5.642 | 17.939 |
| mount-motion | EAST | 100 | 103.738 | 1.598 | 3.738 | 3.871 | 5.612 | 1.856 | 5.612 | 3.738 |
| mount-motion | EAST | 500 | 502.964 | 1.500 | 2.964 | 3.019 | 4.319 | 1.072 | 4.319 | 0.593 |
| mount-motion | WEST | 20 | 22.819 | 0.348 | 2.819 | 2.756 | 5.415 | 2.037 | 5.415 | 14.093 |
| mount-motion | WEST | 100 | 103.892 | 1.360 | 3.892 | 4.159 | 5.891 | 1.967 | 5.891 | 3.892 |
| mount-motion | WEST | 500 | 503.155 | 0.806 | 3.155 | 2.940 | 5.935 | 2.354 | 5.935 | 0.631 |
| mount-motion | NORTH | 20 | 21.982 | 0.366 | 1.982 | 1.782 | 3.999 | 1.300 | 3.999 | 9.910 |
| mount-motion | NORTH | 100 | 102.385 | 0.769 | 2.385 | 1.716 | 5.338 | 1.752 | 5.338 | 2.385 |
| mount-motion | NORTH | 500 | 502.603 | 1.094 | 2.603 | 2.431 | 4.457 | 1.397 | 4.457 | 0.521 |
| mount-motion | SOUTH | 20 | 25.129 | 3.899 | 5.129 | 5.479 | 5.660 | 0.722 | 5.660 | 25.646 |
| mount-motion | SOUTH | 100 | 101.762 | 0.962 | 1.762 | 1.796 | 2.496 | 0.544 | 2.496 | 1.762 |
| mount-motion | SOUTH | 500 | 502.898 | 1.829 | 2.898 | 2.450 | 4.863 | 1.216 | 4.863 | 0.580 |

The timing run issued 120 software pulses: 24 discarded warm-ups and 96 retained measurements. Functional direction/completion/cancellation assertions remain in the normal integration suite; timing values are observational and host-dependent.

## Final validation evidence

- Driver build: `make -C indigo_drivers/mount_simulator -f ../../Makefile.drv all` — passed; universal x86_64/arm64 archive, dynamic library and executable produced.
- Normal arm64 integration: `indigo_test/build/integration/test_mount_simulator` — three consecutive final runs, 48/48 test-case executions passed.
- AddressSanitizer arm64: `indigo_test/build/integration/test_mount_simulator_asan` — three consecutive final runs, 48/48 passed; test and production driver source instrumented, shared `libindigo` not instrumented.
- Normal x86_64 under Rosetta: `arch -x86_64 indigo_test/build/integration/test_mount_simulator` — 16/16 passed.
- AddressSanitizer x86_64 under Rosetta: `arch -x86_64 indigo_test/build/integration/test_mount_simulator_asan` — 16/16 passed with the same instrumentation limit.
- Timing: `make -C indigo_test build/benchmark/bench_mount_simulator_timing && indigo_test/build/benchmark/bench_mount_simulator_timing` — all 120 requested software pulses completed and produced the table above; the benchmark is measurement-only and is not counted as a pass/fail test.
- Strict compile: production source compiled for both x86_64 and arm64 with the repository include/define flags plus `-Wall -Wextra -Werror` — passed.
- Static/integration audit: source scan found zero mutex/pthread/timer references; Xcode project lint and `git diff --check` passed; `MIGRATION_STATUS.md` agrees with the queue/simulator evidence; no README change exists.
- Linux and Windows compilation/execution were unavailable.

## Final test summary

Final simulated test-case executions run: **128**. Final simulated test-case executions passed: **128**. These totals are 64 normal plus 64 ASan executions across arm64 and x86_64 Rosetta; they exclude the separate 120-pulse measurement benchmark. Hardware tests run: **0**. Hardware tests passed: **0**.
