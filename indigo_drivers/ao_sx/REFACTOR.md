# INDIGO 3.0 refactoring record for `ao_sx`

This record was created with the 2026-09-20 hardware acceptance work. The driver's earlier
migration to `indigo_generator` predates this file and is deliberately not reconstructed here; only
the change below is recorded, so nothing in this file is inferred history.

## Current state audit (2026-09-20)

### Architecture and implementation

`indigo_ao_sx.c`, `indigo_ao_sx.h` and `indigo_ao_sx_main.c` are generated from
`indigo_ao_sx.driver` by `indigo_generator`. The `.driver` file is the source of truth.

The driver exposes two logical devices from one serial connection:

- `SX AO`, the master, an `INDIGO_INTERFACE_AO` device that owns `DEVICE_PORT` / `DEVICE_PORTS` and
  has `additional_instances = true`.
- `SX AO (guider)`, an `INDIGO_INTERFACE_GUIDER` slave whose `master_device` is the AO. It opens the
  port through its master and shares `PRIVATE_DATA->handle`; the generator owns the `PRIVATE_DATA->count`
  reference counting, so whichever device connects first opens and whichever disconnects last closes.

All device traffic goes through one helper, `sx_command(device, command, response, ...)`, which
discards pending input, writes the formatted command with `indigo_uni_vprintf()` and, when a reply
is expected, reads exactly `response` raw bytes with `indigo_uni_read_section()` into the shared
`PRIVATE_DATA->response` buffer. The read timeout is 15 s for `K` and `R` and 1 s for everything
else.

### Protocol

Character framed, 9600-8N1, no terminators. Verified against the physical AO-L in this session:

| Command | Reply | Measured latency | Meaning |
| --- | --- | --- | --- |
| `X` | `Y` | 15 ms | handshake |
| `V` | `V` + 3 digits | 15 ms | firmware, reported as `121` by this unit |
| `L` | one status byte, base `0x30` | 15 ms | `0x08`/`0x02` RA at limit, `0x04`/`0x01` DEC at limit |
| `G<NSTW>%05d` | `G`, or `L` at the travel limit | 64 ms at 10 steps, 223 ms at 50 steps | tip/tilt correction |
| `M<NSTW>%05d` | `M` | 15 ms | ST4 guide pulse, argument in 10 ms units |
| `K` | `K` | 1.6 s to 8.2 s | centre |
| `R` | `K` | about 7 s | unjam |
| `U` | — | no reply | not implemented by this unit, and not used by the driver |

The `M` command is acknowledged immediately; the relay pulse is timed by the AO hardware, so the
public guider property completes long before the pulse ends. The `G`, `K` and `R` commands are
acknowledged only after the mechanism has finished moving.

### Public properties

| Device | Property | Notes |
| --- | --- | --- |
| `SX AO` | `AO_GUIDE_RA`, `AO_GUIDE_DEC` | `on_attach` sets every item `number.max` to 50 steps |
| `SX AO` | `AO_RESET` | `CENTER` sends `K`, `UNJAM` sends `R`, both expect `K` |
| `SX AO` | `INFO` | `count` raised to 6; `sx_open()` fills model and firmware |
| `SX AO` | `DEVICE_PORT`, `DEVICE_PORTS`, `ADDITIONAL_INSTANCES` | unhidden by the generator |
| `SX AO (guider)` | `GUIDER_GUIDE_RA`, `GUIDER_GUIDE_DEC` | milliseconds, divided by 10 at the send point |

No driver-specific `X_` properties exist, so the `X_` prefix rule has nothing to check here.

### Blocking, concurrency and lifecycle

Every command runs on the device handler queue, one at a time; the generated
`INDIGO_COPY_VALUES_PROCESS_CHANGE` / `..._PRIORITY_CHANGE` guards publish BUSY and silently drop a
second request for a property that is still BUSY. A request for a different property is queued and
runs after the one in progress, which matters because `K` and `R` occupy the queue for seconds. No
finalizers are used: `G`, `K` and `R` are bounded synchronous transactions, and `M` is acknowledged
immediately by the device.

### Supported platforms and packaging

Platform independent; built through `Makefile.drv` into `indigo_ao_sx.a`, `.dylib` and the
standalone executable, and registered in `indigo.xcodeproj` and `indigo_ao_sx.vcxproj`.

### Existing test assets

- `indigo_drivers/ao_sx/ao_sx_simulator/ao_sx_simulator.c`, a host-side pseudo-terminal protocol
  simulator refactored from the Arduino sketch of the same name. It models the character framing,
  the tip/tilt travel limit and the `L` status byte.
- `indigo_test/integration/test_ao_sx_simulator.c`, 3 cases over the real serial simulator.
- `indigo_test/integration/test_ao_sx_transport.c`, 6 cases over a faked `indigo_uni_*` transport
  covering all four directions and both devices, unit conversion, zero requests, command,
  acknowledgement, limit and reset failures, handshake and initial-status rollback, shared-handle
  ownership in both connection orders, guider 10 ms quantisation and the queued-correction /
  reset / disconnect race.

### Coverage gaps found by this audit

- No hardware acceptance program existed; `MIGRATION_STATUS.md` recorded `9 / 0` automated cases.
- The guiding-pulse duration accuracy measurement required by `indigo_drivers/AGENTS.override.md`
  for every driver that exposes a guider interface had never been run or recorded.

## Baseline (2026-09-20)

Before any production change the checked-in driver was rebuilt and the existing hardware-free suite
was run against it.

```sh
cd indigo_drivers/ao_sx && make -f ../../Makefile.drv
make -C indigo_test build/integration/test_ao_sx_simulator build/integration/test_ao_sx_transport
cd indigo_test && ./build/integration/test_ao_sx_simulator && ./build/integration/test_ao_sx_transport
```

Result: 3 of 3 and 6 of 6 cases passed, driver version 3.0.0.12, no pre-existing failure. The
repository-wide `make all` was deliberately not run; the driver was built through `Makefile.drv`.

## Hardware test decision (2026-09-20)

Hardware testing is performed. The device is a Starlight Xpress AO-L reporting firmware `121`,
attached through its FTDI serial adapter as `/dev/cu.usbserial-FTDFZ2FW` on a macOS 15 arm64 host.
It is the only SX AO available.

Planned scenarios: the AO hardware acceptance row of `indigo_test/DRIVER_TESTING_RULES.md` —
connection and identity, the published property contract, small corrections in all four directions,
the boundary and zero correction, centre followed by a further correction, the supported unjam, the
overlapping request, the embedded guider alone and alongside the AO, both connection orders,
sibling survival, last-close ownership, the guiding-pulse duration measurement, reconnect and
driver INIT/SHUTDOWN.

Deliberately excluded, by the operator's instruction for this session: physical hot-plug and
transport-loss scenarios. The optical element is only ever moved within the ±50 step range the
driver itself advertises and every scenario returns it to centre, so the mechanism is never driven
into a jam or an end stop. The largest correction any scenario makes is 40 steps, see "Test defect
found on hardware".

## Plan

1. Write `indigo_test/hardware/test_ao_sx_hw.c` covering the AO hardware acceptance checklist and
   the guider standard for the embedded ST4 guider, wire it into `indigo_test/Makefile` as the
   opt-in `test-ao-sx-hw` target and add both to the Xcode project. — **done**, the test builds
   warning free and registers 16 cases.
2. Run the suite against the AO-L, record every failure. — **done**, see the run below. Three cases
   failed on the first run: two on real driver defects, one on a wrong assumption in the test
   itself, see "Test defect found on hardware".
3. Add a hardware-free reproducer for each driver defect to
   `indigo_test/integration/test_ao_sx_transport.c` and confirm it fails against the unchanged
   driver. — **done**, `reset_does_not_complete_a_queued_correction` and
   `identity_follows_the_shared_connection` both fail against the version 12 driver built from
   `HEAD` and pass against the fixed one.
4. Fix each defect in `indigo_ao_sx.driver`, regenerate, rerun the hardware suite and the
   hardware-free suites. — **done**, see the found defects section. Version 12 → 13.
5. Record the run in the driver `README.md`, regenerate `TEST_SUMMARY.md` and reconcile
   `MIGRATION_STATUS.md`. — **done**.

## Found defects

**ASX-001 — the AO keeps reporting a model and firmware after the port has been closed.**

Observable impact: with both logical devices connected, disconnecting the AO first and the guider
second closes the serial port, but `SX AO` keeps publishing `INFO.DEVICE_MODEL` =
"StarlightXpress AO" and `INFO.DEVICE_FW_REVISION` = "121". A client that reconnects to a different
unit, or that simply looks at a disconnected device, is shown the identity of the previous session
instead of "Unknown". The stale values survive until the AO itself is connected again.

Root cause: the generator calls `sx_open(device->master_device)` from the slave's connection
handler, so the identity is always read into the master's `INFO_PROPERTY`, but it calls
`sx_close(device)` with whichever device happens to release the last reference. When that is the
guider, `sx_close()` clears and republishes the guider's `INFO_PROPERTY` — whose `count` is the
default 4, so the model and firmware items are not even published — and never touches the master's.

Fix, in `indigo_ao_sx.driver`: `sx_open()` and `sx_close()` now both resolve the master device
themselves, `indigo_device *master = device->master_device ? device->master_device : device;`, and
use it for the port, the `INFO` items and the `INFO` update. This keeps the identity on the device
that owns the connection regardless of which logical device the generated reference counting hands
to the helper, and it is contained entirely in the driver's `code` block; no generator change is
involved.

Regression tests: `sx_ao_clears_identity_on_last_close` in `indigo_test/hardware/test_ao_sx_hw.c`
connects both devices, disconnects the AO first and the guider last, and asserts that `SX AO`
republishes `INFO` with model and firmware back at "Unknown" and reads the same firmware back on
the next connect. The hardware-free reproducer is `identity_follows_the_shared_connection` in
`indigo_test/integration/test_ao_sx_transport.c`, which asserts the same thing over the faked
transport in both connection orders.

**ASX-002 — a completed reset reports a correction that has not been sent yet as done.**

Observable impact: reproduced on hardware by `sx_ao_queues_a_correction_behind_a_reset`. `CENTER`
occupies the device queue for one and a half to eight seconds on an AO-L. A correction requested in
that window is accepted, published `BUSY` and queued behind the reset, as it should be. When the
reset completed it published `AO_GUIDE_RA` as `INDIGO_OK_STATE` while `WEST` still held the
requested 20 steps and nothing had been sent to the unit. A client that waits for its correction to
complete is released early, with the requested value still in the property, and only afterwards does
the driver actually move the mechanism. A guiding client would treat the correction as applied one
transaction too early.

Root cause: `AO_RESET.on_change` unconditionally forced both guide properties to `INDIGO_OK_STATE`
when the unit acknowledged `K` or `R`. That publication exists so that an axis reported at its
travel limit by the initial `L` status is cleared once the mechanism has been centred, but it
overwrote the `BUSY` state of a correction that was still waiting on the queue.

Fix, in `indigo_ao_sx.driver`: the reset now clears an axis only when that axis is not `BUSY`. A
queued correction keeps its `BUSY` state and publishes its own `OK` or `ALERT` when it runs.

Regression tests: `sx_ao_queues_a_correction_behind_a_reset` on hardware asserts that the correction
is not carried out while the reset owns the queue and that its item is cleared only when it has
actually run. The hardware-free reproducer is `reset_does_not_complete_a_queued_correction` in
`indigo_test/integration/test_ao_sx_transport.c`, which gates the faked transport read by read so
the state the reset published for a queued correction can be read without racing the correction's
own completion.

## Test defect found on hardware

The first version of `sx_ao_accepts_boundary_and_zero_corrections` made a full 50 step correction,
the maximum the `AO_GUIDE_*` items allow, and required it to succeed. It failed on the first run and
passed on the second. A direct protocol probe of 30 centre-plus-50-step cycles answered `L`, the
travel limit reply, once; every other reply was `G` after 223 ms. The AO-L half range is therefore
right at 50 steps, so a full-scale correction from centre sits on the mechanical limit and the
driver is correct to report `ALERT` for it. This was a wrong assumption in the test, not a driver
defect. The scenario now uses 40 steps as its largest correction and still asserts that the
advertised item maximum is 50; the limit reply itself stays covered by the simulator and
faked-transport suites, which can produce it deterministically.

## Hardware run

```sh
make -C indigo_test test-ao-sx-hw
```

AO-L, firmware `121`, on `/dev/cu.usbserial-FTDFZ2FW`, centred at the start and at the end.

| Run | Result |
| --- | --- |
| Driver 3.0.0.12, before the fix | 16 cases, 13 passed; `sx_ao_clears_identity_on_last_close` (ASX-001), `sx_ao_queues_a_correction_behind_a_reset` (ASX-002) and `sx_ao_accepts_boundary_and_zero_corrections` (test defect) failed |
| Driver 3.0.0.13, after the fix | 16 cases, 16 passed |

The hardware-free suites were rerun against the fixed driver: `test_ao_sx_simulator` 3 of 3 passed
and `test_ao_sx_transport` 8 of 8 passed, the two added cases included.

Protocol latencies measured on this unit, for reference: `X`, `V`, `L` and `M` about 15 ms, a
correction 64 ms at 10 steps and 223 ms at 50 steps, `K` between 1.6 s and 8.2 s, `R` about 7 s. One
`K` out of about 90 did not answer within 20 s in a direct protocol probe; the driver allows it 15 s
and reports `ALERT` beyond that. This was not reproduced through the driver in any suite run and is
recorded as an observation, not a defect.

## Guiding-pulse duration accuracy

Required by `indigo_drivers/AGENTS.override.md` for every driver exposing a guider interface, and
measured by `sx_ao_measures_guide_pulse_duration`.

**What is measured.** The AO-L acknowledges the `M` command in about 15 ms and times the ST4 relay
pulse in its own firmware, so the public `GUIDER_GUIDE_RA` / `GUIDER_GUIDE_DEC` completion does not
track the pulse. The measurement therefore reports the driver command path: the time from the bus
request to the property reaching `INDIGO_OK_STATE`, against the requested duration. This is **not**
physical relay-output accuracy; establishing that needs an instrument on the ST4 port and is out of
scope for driver acceptance.

Requested durations 100, 250, 500 and 1000 ms, 5 samples each on both axes, 40 samples in total,
with no other driver work running. The measured endpoints are `indigo_monotonic_time()` immediately
before `indigo_change_number_property_1()` and the arrival of the `OK` revision in the test client.

Result, driver 3.0.0.13 on the AO-L:

| Statistic | Value |
| --- | --- |
| Samples | 40 |
| Command path latency, mean | 16.0 ms |
| Command path latency, maximum | 17.7 ms |
| Signed error against the requested duration, mean | -446.5 ms |
| Absolute error against the requested duration, mean | 446.5 ms |
| Worst error against the requested duration | -985.1 ms |

The signed error is simply minus the requested duration, because the driver completes as soon as the
unit acknowledges. That is the intended behaviour for this device and the case asserts it: a
completion that tracked the pulse would mean the driver blocked its queue for the whole guide
exposure. The 16 ms command path is one 9600 baud transaction, and it is the only part of the pulse
timing this measurement covers.

## Scenario to test mapping

| AO class standard area | Scenario |
| --- | --- |
| Discovery and connection, identity | `sx_ao_reports_identity_and_capabilities` |
| Property contract and item ranges | `sx_ao_publishes_the_property_contract` |
| Initial status readback | `sx_ao_reports_initial_limit_status` |
| Corrections east, west, north, south | `sx_ao_corrects_in_all_four_directions` |
| Step magnitudes and zero request | `sx_ao_accepts_boundary_and_zero_corrections` |
| BUSY then OK publication and item reset | `sx_ao_publishes_busy_then_ok` |
| Overlapping and queued requests | `sx_ao_drops_an_overlapping_request`, `sx_ao_queues_a_correction_behind_a_reset` |
| Centre, then correct again | `sx_ao_centers_and_corrects_again` |
| Unjam, then correct again | `sx_ao_unjams_and_corrects_again` |
| Embedded guider, independent operation | `sx_guider_pulses_in_all_four_directions` |
| Guider pulse duration accuracy | `sx_ao_measures_guide_pulse_duration` |
| Shared connection, both orders, sibling survival | `sx_ao_shares_the_connection_in_both_orders` |
| Last-close ownership | `sx_ao_clears_identity_on_last_close` |
| Reconnect after interruption | `sx_ao_reconnects`, `sx_ao_reports_initial_limit_status` |
| INIT/SHUTDOWN and shutdown refusal | `sx_ao_reinitializes` |

## Not covered

- Physical hot-plug and transport loss, excluded by the operator for this session. The driver's
  behaviour when the port disappears is therefore only covered by the faked-transport suite.
- The travel-limit reply `L` and the `AO_GUIDE_*` `ALERT` state it produces are exercised only in
  the simulator and the faked transport. Reproducing it on hardware means driving the optical
  element into its end stop, which the class standard explicitly forbids.
- Optical stabilisation quality, correction accuracy in arcseconds and closed-loop guiding
  performance are outside driver acceptance by the class standard.
- Physical ST4 relay-output timing, see the pulse accuracy section above.
- Windows and Linux were not exercised; the run was macOS 15 arm64 only.
- Only one SX AO was available, so nothing multi-unit was exercised. `ADDITIONAL_INSTANCES` is
  generator-owned and untested on hardware.

## Final test summary

- Simulated tests run: 11; passed: 11. (`test_ao_sx_simulator` 3 and `test_ao_sx_transport` 8, both
  against the fixed driver. The two cases added by this work are counted here and both were
  confirmed to fail against the version 12 driver.)
- Hardware tests run: 16; passed: 16. (Driver 3.0.0.13 against an SX AO-L; the pre-fix run of the
  same 16 cases had 3 failures, two driver defects and one wrong assumption in the test, all
  resolved.)
