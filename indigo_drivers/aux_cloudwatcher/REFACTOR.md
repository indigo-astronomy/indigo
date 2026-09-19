# AUX CloudWatcher refactoring and validation record

Status: migrated to `indigo_generator` on 2026-09-19. The earlier partial record (simulator coverage from 2026-09-09, defect fixes `DRV-085` and `DRV-086` from 2026-09-18) is preserved below under *History*.

## Current-state audit (2026-09-19)

### Architecture and implementation

- `indigo_aux_cloudwatcher.c` is a hand-written 2062-line single-device AUX driver (`AAG CloudWatcher`), version `0x0200000B`, attached with `INDIGO_INTERFACE_AUX_SQM | INDIGO_INTERFACE_AUX_WEATHER`.
- Transport is `indigo_uni_io`: `indigo_uni_open_serial_with_speed()` at a fixed 9600 baud, or `indigo_uni_open_url()` when `DEVICE_PORT` holds a URL. `DEVICE_BAUDRATE` is initialised to `9600` and hidden.
- `aag_command()` serialises every transaction under a driver-private `pthread_mutex_t port_mutex`, discards pending input with `indigo_uni_discard()`, writes the command and reads byte by byte in 15-byte blocks until the `!\x11` handshake block, waiting up to 15 s for the first and every subsequent byte.
- The private data keeps `handle`, `firmware`, `udp`, `anemometer_black`, `cancel_reading`, the heating-algorithm state (`heating_state`, `pulse_start_time`, `wet_start_time`, `desired_sensor_temperature`, `sensor_heater_power`) and 26 `indigo_property *` fields.
- `sensors_timer_callback()` is the polling loop. It reads the full data set (`NUMBER_OF_READS` = 5 passes of `S!`, `T!`, `E!`, `C!`, `V!`, plus one `h!`/`t!`, `p!`/`q!` and `Q!`), aggregates each series with a one-sigma trimmed mean, converts the readings, publishes every weather/condition property, runs the rain-heater control algorithm, polls the relay with `F!` and reschedules itself `REFRESH_INTERVAL` (15 s) minus the measured read duration, never below 1 s.
- Connection is handled by `handle_aux_connect_property()`: open, sleep 2 s (PocketCW resets on open), `A!` identity check, `B!` firmware, `K!` serial number, `F!` relay state, reset of the runtime properties, definition of the connected properties, `M!` + `v!` constants, an informational message and the first timer run.

### Public properties

Always defined (also saved by `CONFIG.SAVE`): `AUX_OUTLET_NAMES`, `X_SKY_CORRECTION`, `AUX_DEW_THRESHOLD`, `AUX_RAIN_THRESHOLD`, `AUX_WIND_THRESHOLD`, `AUX_HUMIDITY_THRESHOLDS`, `AUX_WIND_THRESHOLDS`, `AUX_RAIN_THRESHOLDS`, `AUX_CLOUD_THRESHOLDS`, `AUX_SKY_THRESHOLDS`, `X_ANEMOMETER_TYPE`, `X_RAIN_SENSOR_HEATER_SETUP`.

Defined on connect only: `AUX_GPIO_OUTLETS`, `X_HEATER_CONTROL_STATE`, `X_AAG_CONSTANTS`, `AUX_INFO` (the sensor-readings property), `AUX_WEATHER`, `AUX_DEW_WARNING`, `AUX_RAIN_WARNING`, `AUX_WIND_WARNING`, `AUX_HUMIDITY`, `AUX_WIND`, `AUX_RAIN`, `AUX_CLOUD`, `AUX_SKY`.

All driver-specific names already carry the mandatory `X_` prefix.

### Protocol and documentation

Manufacturer sources are the [Lunático protocol v1.4](https://lunaticoastro.com/rs232-communication-protocol-cloudwatcher/), [v1.3](https://lunaticoastro.com/aagcw/TechInfo/Rs232_Comms_v130.pdf), [v1.1 binary constants](https://lunaticoastro.com/aagcw/TechInfo/Rs232_Comms_v110.pdf) and [v1.0 framing](https://lunaticoastro.com/aagcw/TechInfo/Rs232_Comms_v100.pdf).

### Defects, risks and lifecycle findings

- `aag_open()` tests `indigo_uni_is_url(name, "nexdome")`, a copy/paste leftover from the NexDome driver. Only a `nexdome://` URL takes the network path; every other scheme is opened as a serial device. Recorded as `DRV-121` below.
- `aag_open()` publishes `CONNECTION` itself on failure, which duplicates the framework's own connection reporting.
- The driver carries its own `DEVICE_CONNECTED_MASK` `gp_bits` flag next to the framework's `IS_CONNECTED`.
- `aux_change_property()` dispatches `AUX_GPIO_OUTLETS` through `indigo_set_timer()` rather than the device handler queue, so a relay change can run concurrently with the 15 s polling transaction; only `port_mutex` prevents interleaved protocol bytes.
- `X_AAG_CONSTANTS` is read-only but has a change branch that copies incoming values.
- `aag_populate_constants()` publishes `X_AAG_CONSTANTS` twice on failure (once with a message and once without).
- `process_data_and_update()` is not `static`.

### Platforms, build and packaging

Linux, macOS and Windows; `indigo_aux_cloudwatcher.vcxproj` exists, `Makefile.drv` builds the driver. No `Makefile.inc`.

### Test assets

- `aux_cloudwatcher_simulator/aux_cloudwatcher_simulator.c`, a PTY protocol simulator on `aux_simulator_common.h` modelling firmware 5.89 with SQ, humidity and pressure sensors and the profiles `normal`, `wrong-identity`, `relay-error` and `timeout`.
- `indigo_test/integration/test_aux_cloudwatcher_simulator.c` with five scenarios.
- Coverage gaps: optional-sensor absence, the precise humidity/temperature encodings, the binary `M!` constants, threshold classification of every condition property, the heater control state, anemometer type, outlet renaming and reconnect.

## Baseline (2026-09-19, macOS 15 arm64, universal x86_64+arm64 build)

- `make -C indigo_drivers/aux_cloudwatcher -f ../../Makefile.drv` — succeeded, no warnings.
- `make -C indigo_test build/integration/test_aux_cloudwatcher_simulator` — succeeded.
- `./build/integration/test_aux_cloudwatcher_simulator` — 5 scenarios, 5 passed, 0 failing scenarios.

## Hardware-test decision

Hardware testing will **not** be performed. No AAG CloudWatcher or PocketCW is available in this environment, and no hardware validation is claimed anywhere in this record.

## Migration plan and results

1. **Extend the simulator** with profiles for the absent optional sensors, the precise humidity/temperature encoding, a wet/overcast/dark sky data set and factory constants whose low bytes exceed 127. — **Done.** `aux_cloudwatcher_simulator.c` now serves `normal`, `no-sensors`, `precise`, `wet-overcast`, `constants-high`, `wrong-identity`, `relay-error` and `timeout`.
2. **Extend the characterization suite** against the *unchanged* driver. — **Done.** `test_aux_cloudwatcher_simulator.c` grew from 5 to 15 scenarios. Thirteen of them passed against the original driver; `constants_high_bytes` and the `AUX_WEATHER.ATMOSPHERIC_PRESSURE` metadata assertion inside `normal` are the dedicated reproducers of `DRV-124` and `DRV-123` and were recorded as expected baseline failures (see below).
3. **Capture the normalized reference trace.** — **Done.** `AUX_TEST_TRACE=1 ./build/integration/test_aux_cloudwatcher_simulator 2> trace.txt` over the fourteen pre-existing scenarios produced 509 ordered protocol lines with no timestamps, addresses or identifiers.
4. **Write `indigo_aux_cloudwatcher.driver`** and generate `.c`, `.h` and `_main.c`. — **Done.** The definition is 1574 lines; the generator emits the 26 property declarations, the connection handler, the change dispatch, detach and the entry point.
5. **Build and re-run** the suite and compare the trace. — **Done.** `make -C indigo_drivers/aux_cloudwatcher -f ../../Makefile.drv` builds with no warnings. All 15 scenarios pass, including both defect reproducers. The trace over the same fourteen scenarios is **byte-identical** to the pre-migration reference (`diff` reports no difference over 509 lines).
6. **Register** the `.driver` file and update the status documents. — **Done.** `indigo.xcodeproj/project.pbxproj` lists `indigo_aux_cloudwatcher.driver` in the `aux_cloudwatcher` group, `MIGRATION_STATUS.md` records generator `Yes`, retesting `Sim` and 15/0 tests, and `indigo_docs/PROPERTIES.md` points at the `.driver` source. The Windows project files do not list `.driver` inputs for any generated driver, so they are unchanged.
7. **Final audit.** — **Done.** See *Verification* below.

## Intentional differences from the original driver

- **`DRIVER_NAME`.** The hand-written driver logged under `indigo_aux_skywatcher`. The generator derives the name from the definition, so log lines now carry `indigo_aux_cloudwatcher`.
- **Relay dispatch.** `AUX_GPIO_OUTLETS` was dispatched with `indigo_set_timer()`, off the device handler queue, and the protocol was protected by a driver-private `port_mutex`; a relay command could therefore slip between two of the 25 transactions of a polling cycle. The generated driver dispatches it on the device handler queue like every other handler, so the mutex is gone and a relay command waits for an in-progress reading instead of interleaving with it. The added latency is bounded by one reading; the test `relay_error` and the relay assertions in `normal` cover both outcomes.
- **`X_AAG_CONSTANTS` publication.** `M!` and `v!` are still sent in the same position in the protocol stream, but the values are now read before the generated code defines the property, so clients receive one definition carrying the constants instead of a zeroed definition followed by an update. A failed read now reports `Failed reading device constants` as a message and `INDIGO_ALERT_STATE` on the definition, instead of publishing the property twice.
- **`cancel_reading` removed.** The flag was set by the connection handler, which runs on the same device queue as the polling callback and therefore cannot run while a reading is in progress. It could never shorten one.
- **Elapsed time.** The reading duration and the heater impulse/wet timers use `indigo_monotonic_time()` instead of `gettimeofday()`/`time()`, so a wall-clock adjustment cannot extend or collapse a heating cycle.
- **Write errors.** `cloudwatcher_command()` checks the result of `indigo_uni_vprintf()` and fails immediately instead of writing blindly and waiting for the read timeout.
- **`P` command formatting.** The PWM command is formatted with `"P%04d!"` through the variadic command helper instead of being assembled digit by digit. The bytes on the wire are unchanged.
- **Per-call response buffers removed.** Every helper decodes from the single `PRIVATE_DATA->response` buffer, and `cloudwatcher_aggregate_integers()` uses a fixed-size stack array instead of `indigo_safe_malloc()` per call.
- **Custom block reader retained.** `indigo_uni_read_section2()` reads up to a single-byte terminator, but a CloudWatcher reply is a whole number of 15-byte blocks ended by a handshake block whose *second* byte is XON, and the original consumes that complete block so no padding is left for the next transaction. The block reader is kept for that documented protocol requirement; it is the only custom transport loop in the driver.

## Found defects

- `DRV-121` (`cloudwatcher_open`, source audit only). Observable impact: a `cloudwatcher://host` URL was handed to the serial open path and the connection failed; `tcp://` and `udp://` URLs worked because `indigo_uni_is_url()` accepts them for any prefix. Root cause: `aag_open()` called `indigo_uni_is_url(name, "nexdome")`, a copy/paste leftover from the NexDome driver. Fix: `cloudwatcher_open()` matches `"cloudwatcher"`. No regression test: the simulator harness connects over a PTY, and the integration target must not open sockets, so the URL path has no hardware-free reproducer. Found by source audit, not reproduced.

- `DRV-123` (`AUX_WEATHER.ATMOSPHERIC_PRESSURE`, reproduced). Observable impact: the item was published with `max = 100` while the driver reports hPa, so every real reading (about 1013 hPa) lay an order of magnitude outside the published range, and the item had no format because the `INDIGO_COPY_VALUE()` line after it set `AUX_WEATHER_HUMIDITY_ITEM->number.format` a second time. Root cause: a copy/paste slip in `aag_init_properties()`. Fix: the definition declares `min = 0`, `max = 2000` and `format = "%.0f"`. Regression test: the metadata assertion in the `normal` scenario, which failed against the original driver with `expected [0, 2000] "%.0f", received [0, 100] "%g"` and passes after the fix.

- `DRV-124` (`cloudwatcher_get_electrical_constants`, reproduced). Observable impact: any factory constant whose low byte exceeds 127 was decoded 256 too low; the stock LDR max R of 1744 kΩ was reported as 1488 kΩ, and the LDR pull-up and rain thermistor constants feed the sky-brightness and rain-sensor-temperature conversions. Root cause: `aag_get_electrical_constants()` read the raw bytes of the `M!` reply through a `char` buffer, which is signed on the supported targets, while the v1.1 protocol note defines them as unsigned big-endian pairs. Fix: the reply is decoded through `const unsigned char *`. Regression test: the `constants_high_bytes` scenario over the new `constants-high` simulator profile, which failed against the original driver with `LDR_MAX_R: expected 1744, received 1488` and passes after the fix.

## History

- `DRV-085` (fixed 2026-09-18, driver version `0x0200000A`): low-resolution humidity used `(rhi * 1.7572) / 100 - 6`, a copy of the temperature coefficient. The precise branch scales the 16-bit reading by `125 / 65536`, so a reading expressed as a percentage of full scale scales by `125 / 100`, exactly as the low-resolution temperature branch uses `175.72 / 100`. With the simulator's `!h 40` the driver now reports 44 %RH instead of -5.3 %RH. The precise branch also used integer division (`(rhi * 125) / 65536`), quantising humidity to whole percent; it now uses `125.0`.
- `DRV-086` (fixed 2026-09-18, same version): a timeout before the first response byte left `index` at 0 and executed `response[index - 1] = 0`, writing one byte in front of the caller's buffer. The index is now clamped. The driver-instrumented ASan run of the `timeout` scenario reproduced the underflow before the fix and is clean after it.
- The test only relinked against a rebuilt driver after `indigo_aux_cloudwatcher.a` was added as a prerequisite of its `Makefile` rule; before that the suite silently exercised a stale archive, which is why `DRV-085` appeared to pass.

## Verification (2026-09-19, macOS 15 arm64)

- `../../build/bin/indigo_generator indigo_aux_cloudwatcher.driver` run twice: the second run reproduces the first output byte for byte.
- `make -C indigo_drivers/aux_cloudwatcher -f ../../Makefile.drv` — universal x86_64 + arm64, no errors, no warnings.
- `./build/integration/test_aux_cloudwatcher_simulator` — 15 scenarios, 15 passed.
- `AUX_TEST_TRACE=1` trace over the 14 pre-existing scenarios — identical to the pre-migration reference trace, 509 lines, no diff.
- `make -C indigo_test build/integration/test_aux_cloudwatcher_simulator_asan && ./build/integration/test_aux_cloudwatcher_simulator_asan` — 15 scenarios, 15 passed, no sanitizer report. The framework library is still not instrumented.
- No `MAX_DEVICES` override is present, `git diff` contains no build products, and the driver version rose from `0x0200000B` to `0x0300000C`.
- Linux and Windows builds were not run in this environment; only the macOS universal build is validated.

## Test coverage map

| Scenario | Profile | Covers |
| --- | --- | --- |
| `normal` | normal | Connect, `INDIGO_INTERFACE_AUX`, all 13 connection-dependent and all 12 always-defined properties, pressure and wind conversion, relay set/readback, `DRV-123` metadata |
| `constants` | normal | `M!`/`v!` decoding, per-constant divisors, firmware-independent ambient constants |
| `constants_high_bytes` | constants-high | `DRV-124`: unsigned decoding of the binary constants |
| `readings_and_conditions` | normal | Aggregated raw readings, both thermistor conversions, sky temperature correction, sky quality and Bortle class, all five condition classifications, all three warning lights, heater control state |
| `humidity_conversion` | normal | `DRV-085` regression (low-resolution humidity) |
| `precise_readings` | precise | 16-bit `!th`/`!hh` encoding and the ambient thermistor branch |
| `absent_sensors` | no-sensors | No RH/T, pressure, sky quality or anemometer: zeroed values, the IR-sensor ambient fallback, idle conditions and warnings, dark sky |
| `wet_overcast` | wet-overcast | Raining, overcast, very light, humid, calm, rain and dew warnings |
| `grey_anemometer` | normal | Anemometer type latched at connect and the grey/black wind calibration |
| `outlet_names` | normal | Relay renaming and republication of `AUX_GPIO_OUTLETS` |
| `threshold_settings` | normal | Acceptance and readback of `AUX_CLOUD_THRESHOLDS`, `X_SKY_CORRECTION` and `X_RAIN_SENSOR_HEATER_SETUP` |
| `reconnect` | normal | Withdrawal of the connection-dependent properties, survival of the always-defined ones, second connection and fresh reading |
| `wrong_identity` | wrong-identity | `A!` rejection, alert connection state, no property left defined |
| `relay_error` | relay-error | Rejected acknowledgement of `G!`/`H!` |
| `timeout` | timeout | Silent port, bounded recovery, `DRV-086` regression |

Not covered, and why:

- Hardware behavior of any kind; no device is available (see the hardware-test decision above).
- The `cloudwatcher://`, `tcp://` and `udp://` transports: the integration target must not open sockets, and the PTY harness cannot exercise the URL branch.
- Firmware older than 5.6 (no `t!`/`h!`), older than 5.0 (no anemometer) and older than 3.0 (no `M!`): the simulator models firmware 5.89 only. The driver's version gates are simple early returns, and the `no-sensors` profile already covers the absent-sensor outcome they produce.
- The heater impulse and wet cycles: they need 600 s of continuous wet readings and 60 s of impulse, far beyond the bounded polling the test harness allows. The `X_HEATER_CONTROL_STATE` normal branch and the PWM write are covered.
- `CONFIG.SAVE` persistence: the generated `indigo_save_property()` calls are framework behavior, asserted only through the presence of the twelve persistent properties.

## Final test summary

- Simulated tests: 30 executed, 30 passed (15 ordinary scenarios and the same 15 under AddressSanitizer). Two of the 15 were recorded as expected baseline failures against the original driver and pass as regression tests against the migrated driver.
- Hardware tests: 0 executed, 0 passed.
