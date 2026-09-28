# CG-USB-ST4 guider refactoring and validation record

Status: partial simulator coverage recorded on 2026-09-09. Production refactoring is not complete. `DRV-095` remains open pending protocol confirmation.

## Current-state audit

- The guider is tested through the public INDIGO bus and real PTY I/O against a standalone simulator. Each bounded test case owns and cleans up a fresh simulator process and event log.
- The historical manufacturer page could not be retrieved. [Upstream PHD2 at commit c406cf2b2de51cbb7e3ed76165c9accf33edad2d](https://github.com/OpenPHDGuiding/phd2/blob/c406cf2b2de51cbb7e3ed76165c9accf33edad2d/src/scope_GC_USBST4.cpp) is supplementary evidence: it uses handshake byte `0x06`, reply `A`, numeric directions 0–3 and a four-character millisecond field. INDIGO uses direction letters `n/s/e/w`.
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

### DRV-095 after the hardware run: still open, and the adapter cannot settle it

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
