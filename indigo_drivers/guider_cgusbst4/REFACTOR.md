# CG-USB-ST4 guider refactoring and validation record

Status: numeric-protocol defect `DRV-095` / `FND-000006` fixed in version 3.0.0.10 and verified on CGUSB-ST4 + NYX-101 on 2026-10-24. Broader lifecycle/refactoring gaps below remain separate; no global DEC orientation change was made.

## Current-state audit

- The guider is tested through the public INDIGO bus and real PTY I/O against a standalone simulator. Each bounded test case owns and cleans up a fresh simulator process and event log.
- The historical manufacturer page could not be retrieved. [Upstream PHD2 at commit c406cf2b2de51cbb7e3ed76165c9accf33edad2d](https://github.com/OpenPHDGuiding/phd2/blob/c406cf2b2de51cbb7e3ed76165c9accf33edad2d/src/scope_GC_USBST4.cpp) is supplementary evidence: it uses handshake byte `0x06`, reply `A`, numeric directions 0–3 and a four-character millisecond field. The historical INDIGO version used direction letters `n/s/e/w`; version 3.0.0.10 uses the numeric mapping.
- The simulator exposes explicit `phd2`, `indigo`, `wrong-identity` and `silent` profiles. Its automatic same-axis replacement is a model assumption, not independently verified firmware behavior.
- Current coverage includes all four directions, expiration and public-value reset in the explicit INDIGO dialect, wrong identity, silent identity and reconnect.
- Remaining gaps are authoritative protocol confirmation, overlapping/replaced pulses, transport loss, additional instances and a full timing benchmark.

## Validation and next steps

- The historical test-change log recorded this driver as `Partial` with PTY protocol simulator plus existing fake transport: the PTY run passed 4/5 scenarios; the explicit INDIGO dialect worked, while the numeric PHD2 dialect rejected the emitted commands as `DRV-095`.
- Five ordinary simulator scenarios ran on 2026-09-09: four passed. The PHD2-dialect scenario rejected INDIGO's letter command while INDIGO later reported timer completion, reproducing `DRV-095`.
- Hardware testing was not performed. No hardware protocol compatibility or relay behavior is claimed.
- Before production changes: obtain authoritative protocol evidence or an identified device for hardware testing; record the exact model and hardware plan if available; then plan atomic protocol, overlap, transport-loss, instance and timing work and verify each step through the simulator and applicable hardware.

## Automated-test wiring (2026-09-18)

`phd2_dialect_directions` is a known-defect reproducer for `DRV-095`, but it ran in the default
suite and failed it, which stopped `make -C indigo_test test` before roughly half the integration
tests. It is now opt-in behind `--known-defects`, matching the `dome_baader` convention:

```sh
make -C indigo_test test-guider-cgusbst4-simulator-known-defects
```

The remaining four scenarios run in the default suite and pass. `DRV-095` itself is unchanged and
still needs authoritative protocol evidence or hardware before the dialect can be altered. The
test's `Makefile` rule also now lists `indigo_guider_cgusbst4.a` as a prerequisite, so a rebuilt
driver relinks the test instead of leaving a stale archive in place.

## Final test summary

- Simulated tests: 5 executed, 4 passed (1 known-defect reproducer excluded from the default suite).
- Hardware tests: 0 executed, 0 passed.

## Overlapping guide pulses (2026-09-20)

`GUIDER_GUIDE_RA` and `GUIDER_GUIDE_DEC` now declare `accept_while_busy = true`, replacing the
earlier workaround that forced the property state back to `INDIGO_OK_STATE` in `on_change_request`
so the BUSY-guarded dispatch macro would let the request through. `on_change_request` is now only
the two item-zeroing lines, matching every other INDIGO driver that exposes a guider.

The `indigo_cancel_pending_handler(device, guider_guide_<axis>_handler)` call that used to sit in
`on_change_request` was removed because it runs on the bus thread, where `indigo_queue_remove()`
blocks until a running handler finishes. The finaliser cancellation in `on_change` runs on the
device queue thread, where that wait is skipped, and is what actually implements the replacement.

## Hardware acceptance run (2026-09-21)

A physical CG-USB-ST4 adapter (`04d8:ffe5`, USB strings `Astrogene1000_stuff` / `USB to ST4
Astrogene_1000`, `/dev/cu.usbmodem000000043B1`) was connected to a Mac mini (macOS, arm64) and
driven through the real driver by the new opt-in suite
`indigo_test/hardware/test_guider_cgusbst4_hw.c`:

```sh
make -C indigo_test test-guider-cgusbst4-hw
```

All ten cases passed and the driver is unchanged by this record. The driver's USB match pattern
picked the adapter out of two CDC devices on the same machine (the other one is the USB_Focus v3),
and the `0x06`/`A` handshake confirmed the identity over the link.

### Scenario to test mapping

| Hardware acceptance area | Scenario |
| --- | --- |
| Port detection, identity handshake, connection | `cgusbst4_reports_identity_and_capabilities` |
| Published property contract | `cgusbst4_publishes_the_property_contract` |
| All four directions, BUSY/completion | `cgusbst4_pulses_in_all_four_directions` |
| Overlapping axes | `cgusbst4_pulses_both_axes_at_once` |
| Same-axis replacement | `cgusbst4_replaces_a_pulse_on_the_same_axis` |
| Zero request | `cgusbst4_stops_on_a_zero_request` |
| Pulse duration measurement | `cgusbst4_measures_guide_pulse_duration` |
| Disconnect during a pulse, handshake and pulse after reconnect | `cgusbst4_survives_a_disconnect_during_a_pulse` |
| Reconnect and repeated disconnect | `cgusbst4_reconnects` |
| INIT/SHUTDOWN, shutdown refused while connected | `cgusbst4_reinitializes` |

### Guiding-pulse duration measurement

Requested 20, 50, 100, 200 and 500 ms in all four directions, four samples each with the first
discarded, 60 samples in total, on an idle adapter with no other workload. Measured endpoints are
the public `GUIDER_GUIDE_RA` / `GUIDER_GUIDE_DEC` request and the OK completion the client
observes, so this is **software completion timing of the real serial command path**, not an
electrical measurement of the relay output: the driver times the pulse itself and the adapter
gives no feedback. Every direction ran long by a roughly constant amount, the one serial write
plus the handler queue latency:

| Requested | Mean error | Error in percent |
| --- | --- | --- |
| 20 ms | +3.4 … +5.9 ms | +17 … +29 % |
| 50 ms | +4.0 … +5.9 ms | +8 … +12 % |
| 100 ms | +3.8 … +5.8 ms | +4 … +6 % |
| 200 ms | +3.5 … +4.7 ms | +2 % |
| 500 ms | +2.7 … +5.4 ms | +0.5 … +1.1 % |

Worst absolute error over all 60 samples: 7.4 ms, standard deviation per cell 0.4 to 2.9 ms. The
overhead is about a third of the GPUSB's, which needs two HID writes per edge.

### Historical DRV-095 status after the 2026-09-21 run: still open

The adapter was probed directly over the port, outside the driver, with both dialects:

| Probe | Result |
| --- | --- |
| `0x06` on an idle adapter | `A` in under 1 ms |
| `:Mgn3000#` (INDIGO letters) | no reply at all within 1 s |
| `0x06` while that pulse would be running | `A` in under 1 ms |
| `:Mg03000#` (PHD2 digits) | no reply at all within 1 s |
| `0x06` while that pulse would be running | `A` in under 1 ms |

The firmware acknowledges neither dialect and refuses neither, and it does not block while a pulse
is timing out, so **a host cannot tell an accepted guide command from an ignored one**. This run
therefore neither confirms nor refutes DRV-095, and the driver's letter dialect was deliberately
left unchanged.

What the evidence still says: upstream PHD2 `scope_GC_USBST4.cpp` sends `:Mg%c%4d#` with the
digits 0–3 for north, south, east and west against this device class, while INDIGO sends the
letters `n`, `s`, `e` and `w` in the same frame. If the firmware only understands digits, every
INDIGO guide pulse is silently dropped and every scenario above still passes, because the driver
times the pulse itself. Settling this needs an observation the adapter does not provide: a mount
wired to the ST4 port that visibly moves, or the adapter's own direction indicator, while a long
pulse runs in each dialect. Until then the numeric dialect stays modelled by the simulator's
default `phd2` profile and the reproducer stays behind `--known-defects`.

### Simulator alignment

The hardware facts above were carried back into
`guider_cgusbst4_simulator/guider_cgusbst4_simulator.c`, which already models them: the handshake
is answered at any time, including during a pulse, and an unparsable command is recorded as a
silent `REJECT` with no reply, exactly as the adapter behaves. The new
`reconnect_during_pulse` scenario in `indigo_test/integration/test_guider_cgusbst4_simulator.c`
mirrors the hardware scenario of the same shape: the link is dropped while the adapter keeps
timing the abandoned pulse, the handshake of the reconnect has to succeed and the axis has to come
back idle. Still modelled rather than verified: the automatic same-axis replacement, the
space-padded `%4d` duration field and the numeric dialect itself.

- Simulated tests run: 5; passed: 5 (plus the `--known-defects` reproducer, unchanged).
- Hardware tests run: 10; passed: 10, against a physical CG-USB-ST4 on macOS arm64.

## FND-000006 numeric-protocol repair (2026-10-24)

Scope: bounded protocol correction, not a lifecycle migration. Owner requests PHD2 N/S/E/W = 0/1/2/3 without N/S swapping. The prior hardware experiment (task 20261024064954-2f11-verify-cgusbst4-protocol) confirmed numeric acceptance and silent letter rejection with NYX-101 firmware 1.32.1. DEC orientation on that setup remains qualified, not a reason for a global swap.

Current source is generator-owned (.driver, .c/.h/_main.c), portable serial I/O, unchanged RA/DEC properties and timed finalizers. No property addition/removal, dependency, OS API or generator change is needed. Version before repair is 9. Public properties and lifecycle remain unchanged.

Baseline: `python3 tools/run_driver_test.py guider_cgusbst4 --no-record` on mac arm64 builds successfully, five PTY cases pass; both fake transport cases fail because the compile-time replacement still intercepts open_serial instead of open_serial2. Exact output: task 20261024070358-5d13-fix-cgusbst4-protocol/baseline.log. The existing opt-in PHD2 direction reproducer is being run before the production edit.

Hardware decision: yes, physical CGUSB-ST4 04d8:ffe5 connected to NYX-101 over ST4 on indigosky. Existing owner authorization covers temporary unpark/tracking, at most two-second pulses and repark. Final external motor-response probe uses 500 ms pulses at temporary R1, restores R6 and parked/tracking-off state. Existing hardware acceptance/timing suite runs with NYX parked and does not establish physical relay precision. No hot-plug or away-from-pole slew planned.

Atomic plan:
1. Complete: stale fake-open interception repaired; both original letter-wire transport cases pass. Numeric PTY reproducer failed as expected before production edit. Original real-driver hardware suite passed its ten adapter cases but the new independent NYX-response case failed at missing active ST4 status, giving 11/10 Failed; cleanup restored parked/not tracking/R6. The simulator runner needed an explicit capture call before its manual event-file unlink; reference traces are retained with task evidence.
2. Complete: N/S/E/W = 0/1/2/3, version 10; unchanged generator reproduces .c/.h/_main.c. Mandatory numeric PTY regression retains all four wire mappings; all lifecycle profiles now use the observed numeric dialect. Test parent records the complete plan and captures event logs before deletion. Normalized successful/lifecycle event sequences match the baseline; the wire alphabet is the sole intended protocol difference.
3. Complete: recorded mac arm64 simulator/transport 7/7 and Linux arm64 CGUSB-ST4 with NYX-101 ST4 11/11. ASan/UBSan transport 2/2 (driver and test instrumented, framework library not instrumented); strict hardware-test syntax check has no warnings with unused helper/parameter warnings suppressed. Windows and native Intel runtime unavailable. Two hardware-test iterations exposed its incorrect lowercase-only status assumption: NYX reports G at R1 and g at R6, both documented active modes. Bounded sampling accepts either and still requires independent motor movement. No additional production behavior changed.
4. Test records updated through the recorded runner, final hardware runner used TZ=Europe/Bratislava, and summary regenerated. Final diff/process review and archival are owned by the task evidence; no merge or push authorized.

Found defects: FND-000006 is reproduced by numeric-dialect PTY rejection and independent physical mount observation; root cause is letter direction encoding. Planned fix is the four numeric constants, regression is phd2_dialect_directions plus exact transport frames. Stale open_serial interception is a test defect predating this repair, requiring an open_serial2 wrapper; no production I/O change.

### Repaired-driver hardware response and timing

At R1, after an idle baseline of 151.319 RA steps/s and zero DEC steps, 500 ms public guide requests through the real driver yielded DEC -17/+17 steps for NORTH/SOUTH. EAST/WEST produced idle-corrected RA -30.476/+28.522 steps. The test deliberately does not assert a global celestial DEC mapping from a near-pole position. Every run restored parked/not tracking/R6 (`nNPzET160`). The optional physical-response case is enabled only by `CGUSBST4_NYX_PORT=/dev/ttyUSB0`, in addition to the adapter port; it requires initially parked/not tracking/R6 and is excluded from ordinary adapter-only runs. No hot-plug or optical/electrical guiding-accuracy certification.

The hardware suite measured request-to-public-OK software completion on the actual serial driver path, with NYX parked: all four directions at 20/50/100/200/500 ms, one discarded warm-up and three measured samples per direction/duration (60 samples total), idle workload. Cell means exceeded requested time by +0.53 to +1.39 ms, maximum absolute sample error 1.39 ms, reported cell standard deviations 0.00–0.03 ms. Full min/mean/median/p95/p99/max, signed percent errors and standard deviations are retained in the task hardware log. These are host/public-property timing statistics, not relay pulse accuracy. Standalone adapter has no imaging workload; imaging concurrency is not applicable.

The NYX coordinate transform/pier-side inversion from the prior experiment is outside this adapter simulator: it does not simulate the attached mount. Numeric acceptance, silent letter rejection and firmware pulse expiration were already modelled; current regressions use that hardware-confirmed profile. Same-axis replacement, zero cancellation and physical transport loss remain broader coverage limitations; adapter-only timer tests do not prove physical relay semantics.

Evidence and exact commands: [repair verification](../../.factory/tasks/archive/20261024070358-5d13-fix-cgusbst4-protocol/verification.md).

## Final test summary — numeric protocol repair

- Simulated tests: 7 executed, 7 passed; additional sanitizer variant: 2 executed, 2 passed.
- Hardware tests: 11 executed, 11 passed, Linux arm64 CGUSB-ST4 + NYX-101 ST4; no physical hot-plug test.
