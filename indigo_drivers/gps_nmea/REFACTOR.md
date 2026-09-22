# Generic NMEA 0183 GPS validation record

The driver was migrated to the generated INDIGO 3.0 form before this record existed, so this file
starts as the validation record of the 2026-09-22 hardware acceptance run and carries the audit,
the baseline, the found defects and the coverage map that `indigo_drivers/AGENTS.override.md`
requires.

## Current-state audit (2026-09-22)

### Architecture and implementation

- Generated driver. `indigo_gps_nmea.driver` is the source of truth, `indigo_gps_nmea.c`,
  `indigo_gps_nmea.h` and `indigo_gps_nmea_main.c` are the checked-in outputs of
  `indigo_generator`.
- One logical device, `NMEA GPS`, of class `gps`, with `additional_instances = true`.
- The transport is either a serial port opened through `indigo_uni_open_serial_with_config()` with
  the rate `DEVICE_BAUDRATE` names, or a TCP socket when `DEVICE_PORT` holds a `gps://host:port`
  URL. `nmea_open()` and `nmea_close()` are the generator-wired connection helpers.
- There is no command channel. The receiver streams, and the driver only reads: `on_timer` reads
  one line with `indigo_uni_read_line()` (a 5 s section read), parses it and reschedules itself.
  A read that returns nothing is treated as the receiver being gone, and the timer disconnects the
  device by requesting `CONNECTION.DISCONNECTED` and queueing the connection handler.
- `nmea_parse()` validates the sentence before any field is used: the `$G?` prefix, the optional
  `*hh` checksum, a token count that depends on the sentence, and per-sentence field ranges through
  `nmea_number()` and `nmea_coordinate()`. An invalid sentence is dropped, which is what keeps a
  malformed record from becoming a position.
- `RMC`, `GGA`, `GSA` and `GSV` are interpreted; every other sentence the receiver sends is parsed
  and discarded.

### Protocol

NMEA 0183. The talker id is the third character of the sentence (`$GPxxx` is `P` for GPS, `L` for
GLONASS, `A` for Galileo, `B` for BeiDou, `I` for NavIC, `Q` for QZSS, `N` for a combined solution).
`X_GPS_SELECTED_SYSTEM` selects which talker the driver accepts, `AUTO` latching onto the talker
that first reports a fix.

### Public properties

- Inherited and visible after connection: `GEOGRAPHIC_COORDINATES` (LATITUDE, LONGITUDE,
  ELEVATION), `UTC_TIME` (TIME only), `GPS_STATUS`, `GPS_ADVANCED`, and `GPS_ADVANCED_STATUS` while
  `GPS_ADVANCED.ENABLED` is set.
- Driver-defined: `X_GPS_SELECTED_SYSTEM`, always defined, one of AUTO, MULTIPLE, GPS, GALILEO,
  GLONASS, BEIDOU, NAVIC, QZSS.

### Defects, risks and lifecycle findings

Recorded in the found-defects section below.

### Platforms, build and packaging

Platform independent. Built into `build/drivers/indigo_gps_nmea.a` on macOS and Linux and through
`indigo_gps_nmea.vcxproj` on Windows.

### Test assets

- `gps_nmea_simulator/gps_nmea_simulator.c`, a pseudo-terminal simulator that streams one fixed
  3D-fix cycle.
- `indigo_test/integration/test_gps_nmea_simulator.c`, the serial compliance case.
- `indigo_test/integration/test_gps_nmea_transport.c`, three cases over a fake transport covering
  framing, checksum, token bounds, coordinate and time mapping, read loss, open failure,
  constellation selection, fix transitions, DOP and UTC rollover.

## Baseline (2026-09-22, macOS 15 arm64, universal x86_64+arm64 build)

```
make -C indigo_test build/integration/test_gps_nmea_simulator build/integration/test_gps_nmea_transport
indigo_test/build/integration/test_gps_nmea_transport     # 3/3 passed
indigo_test/build/integration/test_gps_nmea_simulator     # 1/1 passed
```

Driver version at baseline: `3.0.0.19`.

## Hardware-test decision

Hardware testing is performed. Device: **u-blox 7 GPS/GNSS receiver** (USB, `1546:01a7`, CDC ACM at
`/dev/cu.usbmodem111401`), reporting `HW UBX-G70xx 00070000`, `ROM CORE 1.00 (59842)`,
`PROTVER 14.00`.

The receiver is indoors and has no sky view. A raw 3-minute capture taken before the run shows the
`$GP` talker only, `GSA` reporting fix mode 1 with `99.99` DOP throughout, `GSV` occasionally
reporting a single satellite at SNR 28, and `RMC`/`GGA` with every field empty. **The run therefore
does not wait for a fix and does not treat the absence of one as a failure**; it verifies the
communication path, the parsing of the sentences the receiver actually sends, the published
property contract and the connection lifecycle, and it asserts fix-dependent values only if a fix
happens to appear.

Planned scenarios: identity and port selection, the property contract, sentence reception and the
no-fix contract, the fix state lights, advanced status visibility and values, constellation
selection including a constellation the receiver does not send, a refused port, disconnect and
reconnect, and driver SHUTDOWN/INIT.

## Plan

1. Record this audit, the baseline and the hardware decision. — **done**
2. Add `indigo_test/hardware/test_gps_nmea_hw.c` and its `test-gps-nmea-hw` target, registered in
   the Xcode project. — **done**, nine scenarios, built clean.
3. Run it against the u-blox 7, record the result and every defect it finds. — **done**, the first
   run found `DRV-137` and `DRV-138` below and three wrong expectations of the new test itself.
4. Fix each defect in `indigo_gps_nmea.driver`, regenerate, reproduce the behaviour in the
   simulator with a regression test, and rerun the full hardware scope. — **done**, driver
   `3.0.0.19` → `3.0.0.20`, simulator `--no-fix` mode added, hardware scope 9/9.
5. Record the run in `README.md`, regenerate `TEST_SUMMARY.md` and update `MIGRATION_STATUS.md`. —
   **done**

## Found defects

### DRV-137: `nmea_reset()` published properties the client did not have

- **Observed on hardware**, 2026-09-22, u-blox 7, driver `3.0.0.19`. With `HW_TRACE=1` the connect
  sequence is `CONNECTION` busy, then updates of `GEOGRAPHIC_COORDINATES`, `GPS_STATUS`, `UTC_TIME`
  and `GPS_ADVANCED_STATUS`, and only then the definitions of the first three. The fourth is never
  defined at all, because `GPS_ADVANCED` defaults to `DISABLED`.
- **Impact.** Four `set...Vector` messages precede the matching `def...Vector`, and
  `GPS_ADVANCED_STATUS` is published to clients that were never told it exists. A client that tracks
  definitions either drops the update or, as the hardware harness did, believes a hidden property is
  visible.
- **Root cause.** `nmea_reset()` ends with four unconditional `indigo_update_property()` calls. It
  runs from the `on_connect` block, which the generator executes before
  `indigo_gps_change_property(device, NULL, CONNECTION_PROPERTY)` defines the connected properties,
  and it ignores `GPS_ADVANCED_ENABLED_ITEM` although every other publication site in the driver
  checks it.
- **Fix.** `indigo_gps_nmea.driver`, `nmea_reset()`: publish only under `IS_CONNECTED`, which is
  false for the whole of `on_connect` because `CONNECTION_PROPERTY->state` is still busy there, and
  publish `GPS_ADVANCED_STATUS` only while `GPS_ADVANCED_ENABLED_ITEM->sw.value`. Nothing is lost
  during connect: the definitions that follow carry the values the reset has just set.
- **Regression test.** `nmea_gps_reports_a_receiver_without_a_fix` in
  `indigo_test/integration/test_gps_nmea_simulator.c` asserts `updates_without_define() == 0` after
  connect and again after a selection change, and that `GPS_ADVANCED_STATUS` is not defined while
  the advanced status is disabled. Against the unfixed driver it reports `expected 0, got 4`.
  Detecting this needed a new counter in `integration/simulator_test_common.h`, because the property
  cache silently drops an update for a property it has no definition for.

### DRV-138: no-fix `GSA` fought the `RMC` and `GGA` of the same cycle

- **Observed on hardware**, 2026-09-22, u-blox 7, driver `3.0.0.19`. The trace repeats, once per
  second for as long as the receiver has no fix: `UTC_TIME` alert, `GEOGRAPHIC_COORDINATES` alert,
  `GEOGRAPHIC_COORDINATES` busy, `UTC_TIME` busy.
- **Impact.** Both properties oscillate between alert and busy twice a second and never settle. The
  alert half lasts the few milliseconds between the `RMC` and the `GSA` of the same cycle, so a
  client polling either property sees busy essentially always while the driver keeps publishing the
  contradiction. It also defeats INDIGO's update suppression: a receiver with nothing to report
  produces four property updates per second per client indefinitely.
- **Root cause.** A migration regression. The `RMC` and `GGA` branches publish
  `hasFix ? INDIGO_OK_STATE : INDIGO_ALERT_STATE`, while `nmea_reset()` and the no-fix `GSA` branch
  use busy for the same condition. In the original hand-written driver the `GSA` branch was guarded
  by `fix == 1 && GPS_STATUS_NO_FIX_ITEM->light.value != INDIGO_ALERT_STATE`, so it ran only on the
  transition into no fix and the steady state settled on alert. The generated driver dropped that
  guard while adding the `position_valid`/`time_valid` invalidation.
- **Fix.** `indigo_gps_nmea.driver`, `GSA` branch: keep the unconditional invalidation the migration
  added and restore the original guard around the light, state and publication work, so only the
  transition into no fix forces busy.
- **Regression test.** The same `nmea_gps_reports_a_receiver_without_a_fix` case waits out the first
  cycle, then requires `GEOGRAPHIC_COORDINATES` and `UTC_TIME` to stand at alert and to be published
  no further over three more cycles. Against a driver carrying only this defect the case fails
  earlier still, at `wait_for_property_state(GEOGRAPHIC_COORDINATES, INDIGO_ALERT_STATE)`, because
  the alert half of the oscillation is too short for a poll to catch.

### Wrong expectations of the new hardware test, not driver defects

The first hardware run also failed three of its own assertions. They are recorded here because each
is a fact about the framework that the next test in this class has to know:

- `INFO` publishes four items by default (`INFO_PROPERTY->count = 4`), so a driver with no identity
  to report has no `DEVICE_MODEL` item. The scenario now asserts `DEVICE_NAME` and `DEVICE_DRIVER`
  and that `DEVICE_MODEL` is absent.
- `indigo_update_property()` suppresses an update that changes neither the state nor any item, so a
  receiver repeating the same sentences publishes nothing once it has settled. Counting publications
  cannot prove the link is alive. The scenario now uses the driver's own 5 s read timeout — staying
  connected past it means lines keep arriving — and a forced reset that only freshly parsed
  sentences can undo.
- `DEVICE_PORT` is validated by the framework, which reports alert for a path it cannot read but
  keeps the value, so the refused-port scenario expects alert from `DEVICE_PORT` and alert from the
  connection attempt that follows.
- A state a receiver cycle undoes within a second cannot be polled. `nmea_reconnects` asserted that
  no status light is on straight after the reconnect, and passed twice before failing on three runs
  in a row: the driver does clear the lights on connect, but the first `GSA` of the new session had
  already restored `NO_FIX` by the time the assertion read them. The scenario now asserts the busy
  reset and the return to OK through the state history the property's deletion cleared, which has no
  such race. This is a defect of the test, not of the driver.

### Observations recorded, not changed

- Without a fix the driver writes `2000-00-00T00:00:00` into `UTC_TIME.TIME` and `0, 0` into
  `GEOGRAPHIC_COORDINATES`, because `RMC` carries empty fields and `atoi`/`indigo_atod` map them to
  zero. Both properties are published in the alert state, so neither is offered as valid, and the
  original driver behaved the same way. Left as is; changing which placeholder a receiver without a
  fix shows is not a malfunction this run demonstrated.

## Simulator corrections taken from the hardware (2026-09-22)

The simulator streamed one fixed 3D-fix cycle and had no way to reproduce either defect, both of
which only appear without a fix. It gained a `--no-fix` mode carrying what the u-blox 7 actually
sent during a three-minute capture taken before the run:

- the `$GPTXT` power-up banner, six sentences the driver has to read and discard;
- `RMC` with every field empty and the `V` void flag, `VTG` and `GLL` likewise;
- `GGA` with fix quality `0` and `00` satellites used;
- `GSA` with fix mode `1` and the `99.99` no-solution dilution on all three axes;
- a `GSV` naming one satellite the receiver can hear at SNR 28 with no elevation or azimuth. The
  capture sent one about every twenty-fourth cycle; the mode sends one every fifth so a test does
  not have to wait out that cadence.

The mode is opt-in, so the existing fixed-fix cases keep their meaning.

## Verification (2026-09-22, macOS 15 arm64, universal x86_64+arm64 build)

```
indigo_generator indigo_gps_nmea.driver          # regenerated, diff explained by the two fixes
make -f ../../Makefile.drv all                   # driver archive, no warnings
make -C indigo_test build/hardware/test_gps_nmea_hw   # no warnings
indigo_test/build/integration/test_gps_nmea_simulator # 2/2 passed
indigo_test/build/integration/test_gps_nmea_transport # 3/3 passed
make -C indigo_test test-gps-nmea-hw                  # 9/9 passed
```

The hardware scope was run five times in total: twice against the first version of the test, then,
after `nmea_reconnects` was found to be racy, three more consecutive clean runs against the current
one.

Not run: Linux and Windows builds, and sanitizer variants; no such environment was available in this
session.

## Hardware run (2026-09-22, macOS 15 arm64, u-blox 7, driver 3.0.0.20)

| Scenario | Result |
| --- | --- |
| `nmea_reports_identity_and_capabilities` | pass — GPS interface bit, single device `NMEA GPS`, port `auto:///dev/cu.usbmodem111401` |
| `nmea_publishes_the_property_contract` | pass — coordinates without `ACCURACY`, `UTC_TIME`, three status lights, `GPS_ADVANCED`, eight selectable systems, `GPS_ADVANCED_STATUS` hidden |
| `nmea_receives_sentences_from_the_receiver` | pass — connected through an 8 s window past the driver's 5 s read timeout, and a forced reset was undone by freshly parsed sentences |
| `nmea_reports_the_fix_state` | pass — **no fix**; `NO_FIX` alert, and neither the position nor the time was ever published as valid |
| `nmea_publishes_the_advanced_status` | pass — 0 satellites in use, DOP 99.99 on all three axes, property appears and disappears with `GPS_ADVANCED` |
| `nmea_selects_the_positioning_system` | pass — `GPS` and `MULTIPLE` keep the status, `GLONASS` leaves the driver waiting because the receiver sends only `$GP` |
| `nmea_refuses_an_unusable_port` | pass — alert connection, no connected property left defined, real port works again |
| `nmea_reconnects` | pass — connected properties withdrawn and redefined, status republished busy and re-derived from live data |
| `nmea_reinitializes` | pass — `SHUTDOWN` removes the device, `INIT` brings it back and it connects and parses again |

The receiver was indoors for the whole session and never acquired a fix, so the fix-dependent half
of the class checklist — parsed latitude, longitude, elevation and UTC, 2D and 3D transitions, loss
and reacquisition — was **not** established on hardware. Those paths are covered hardware-free by
`test_gps_nmea_simulator.c` and `test_gps_nmea_transport.c`, and the scenarios above assert them
against hardware automatically whenever a run does have a fix.

## Test coverage map

| Test | Fixture | Covers |
| --- | --- | --- |
| `nmea_gps_passes_serial_compliance_checks` | simulator, 3D fix | GPS class property contract, parsed position and elevation, advanced status, system selection |
| `nmea_gps_reports_a_receiver_without_a_fix` | simulator, `--no-fix` | `DRV-137`, `DRV-138`, no-fix status and DOP, advanced status visibility, reset and recovery |
| `Malformed framing, checksum, token bounds, coordinate/time mapping and read-loss recovery` | fake transport | Framing, checksum, token limits, hemisphere mapping, read loss |
| `Open failure and reconnect` | fake transport | Failed open, open/close balance, reconnect |
| `Constellation selection, fix transitions, DOP, satellites and UTC rollover` | fake transport | Every talker id, fix 1/2/3 lights, DOP, satellites in view, date rollover |
| `test_gps_nmea_hw.c`, nine scenarios | u-blox 7 | The hardware acceptance table above |

Not covered, and why:

- A fix on hardware: the receiver had no sky view for the whole session, which
  `DRIVER_TESTING_RULES.md` says to record rather than wait out.
- The `gps://host:port` network transport: it needs a second host or a bridge and no run in this
  session exercised it.
- `DEVICE_BAUDRATE` against a real RS-232 receiver: the unit is USB CDC, where the rate is ignored.

## Final test summary

- Simulated tests: **5 executed, 5 passed** (2 simulator integration cases, 3 fake-transport cases),
  against driver `3.0.0.20`. The same five passed against `3.0.0.19` except
  `nmea_gps_reports_a_receiver_without_a_fix`, which is the regression test of `DRV-137` and
  `DRV-138` and fails against it by design.
- Hardware tests: **9 executed, 9 passed** (2026-09-22, u-blox 7, driver `3.0.0.20`), three
  consecutive runs with the same result after the racy `nmea_reconnects` assertion was corrected.
  The first run, against `3.0.0.19`, executed the same 9 and failed 6: two driver defects and three
  wrong expectations of the test itself, all listed above.
