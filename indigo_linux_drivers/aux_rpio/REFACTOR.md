# Raspberry Pi GPIO AUX driver refactoring and validation record

Status: audit and baseline recorded on 2026-09-20. No production change has been made yet.

## Environment

All work for this record runs on the target hardware, not on a development host:

- Raspberry Pi 5 Model B Rev 1.0, `pinctrl-rp1` 40-pin header controller.
- Debian GNU/Linux 12 (bookworm), kernel `6.12.87+rpt-rpi-2712`, aarch64.
- gcc 12.2.0, GNU Make 4.3.

## Current-state audit

### Architecture and implementation

`indigo_aux_rpio.c` is a hand-written INDIGO 2.0 AUX driver. It exposes one static logical
device, `Raspberry Pi GPIO`, created at `INDIGO_DRIVER_INIT` and removed at
`INDIGO_DRIVER_SHUTDOWN`; there is no hot-plug and no `.driver` generator input. The whole
hardware boundary is the legacy sysfs GPIO and PWM interface reached with `open`/`read`/`write`
on absolute paths:

- `/sys/class/gpio/export`, `/unexport`, `/gpio<pin>/direction`, `/gpio<pin>/value`
- `/sys/class/pwm/pwmchip0/export`, `/unexport`, `/pwm<ch>/enable`, `/period`, `/duty_cycle`

Pin assignment is a pair of static tables, in BCM numbering:

- inputs `{ 19, 17, 27, 22, 23, 24, 25, 20 }`
- outputs `{ 18, 12, 13, 26, 16, 5, 6, 21 }`

Outputs 1 and 2 are driven as PWM channels 0 and 1 when `/sys/class/pwm/pwmchip0` exists,
otherwise as plain GPIO.

### Public properties

- `X_AUX_OUTLET_NAMES` (text, 8) and `X_AUX_SENSOR_NAMES` (text, 8), persistent labels.
- `AUX_GPIO_OUTLETS` (switch, 8, any-of-many), the output states.
- `AUX_OUTLET_PULSE_LENGTHS` (number, 8), per-output pulse length in milliseconds.
- `AUX_GPIO_OUTLET_FREQUENCIES` (number, 2) and `AUX_GPIO_OUTLET_DUTY` (number, 2), PWM only,
  hidden when no PWM chip is present.
- `AUX_GPIO_SENSORS` (number, 8), the input states, polled once per second.

Interface bit is `INDIGO_INTERFACE_AUX_GPIO`. `INFO` model and firmware are hardcoded to `N/A`.

### Observable behavior

Connect probes the PWM chip, exports every pin, sets directions after a fixed one second sleep,
reads back the current output states into `AUX_GPIO_OUTLETS`, programs both PWM channels from the
stored frequency/duty targets, re-arms already enabled PWM channels, defines the runtime
properties and starts the one second sensor poll. Disconnect cancels the eight pulse timers and
the sensor timer, unexports everything and deletes the runtime properties. A non-zero pulse length
turns an output on and schedules a per-output timer that turns it off again.

### Supported platforms, build and packaging

Linux only, built from `indigo_linux_drivers` by `Makefile.drvs`. Listed in `STABLE_DRIVERS`.
`MIGRATION_STATUS.md` records API 2, no generator, no async queues, not retested, 0 / 0 automated
tests, comment `RPi only`.

### Existing tests

None. There is no simulator, no fake SDK and no test source for this driver anywhere under
`indigo_test/`. This is the single largest gap.

## Baseline

Built from a clean clone of `refactoring` at `64eb95763`:

```sh
cd ~/indigo && make
```

Result: `EXIT=0`. `build/drivers/indigo_aux_rpio`, `.a` and `.so` are produced with no compiler
warnings attributable to this driver. A first attempt with `make -j4` failed in `indigo_libs` with
`indigo_ccd_driver.c:36:10: fatal error: jpeglib.h: No such file or directory`; that is a
pre-existing parallel-build ordering defect in the framework build, unrelated to this driver, and
a serial `make` succeeds. Recorded here because it blocks a parallel baseline build on a fresh
checkout.

No existing tests could be run as a baseline, because none exist.

## Hardware-test decision

Hardware testing **will** be performed, on the Raspberry Pi 5 described above, but only in a
strictly non-destructive form: reading the sysfs topology and confirming which interfaces exist.
No test in this record drives the physical header pins. The Pi is an in-service INDIGO Sky host and
unknown equipment may be attached to the 40-pin connector, so toggling real outputs is out of
scope. Every functional scenario runs against a fake sysfs tree.

Planned hardware scenarios:

1. Enumerate `/sys/class/gpio/gpiochip*` and record `base`, `ngpio` and `label`.
2. Enumerate `/sys/class/pwm/pwmchip*` and record `npwm` and the backing platform device.
3. Confirm, without exporting, that the driver's hardcoded numbering cannot address the header on
   this model.

Results are in the found-defects section below.

## Found defects

Numbering is stable; each entry gets a regression test before it is fixed.

### D1 Hardcoded sysfs pin numbering breaks on Raspberry Pi 5

- Impact: the driver cannot control any pin on a Pi 5. Connect fails.
- Root cause: legacy sysfs GPIO numbers are global, not per chip: the sysfs number of BCM pin `n`
  is `gpiochip.base + n`. The driver writes the bare BCM number. Observed on the target:

  | chip | base | ngpio | label |
  | --- | --- | --- | --- |
  | gpiochip512 | 512 | 17 | gpio-brcmstb@107d517c00 |
  | gpiochip529 | 529 | 6 | gpio-brcmstb@107d517c20 |
  | gpiochip535 | 535 | 32 | gpio-brcmstb@107d508500 |
  | gpiochip567 | 567 | 4 | gpio-brcmstb@107d508520 |
  | gpiochip571 | 571 | 54 | pinctrl-rp1 |

  The 40-pin header is `pinctrl-rp1` at base 571, so BCM 18 is sysfs 589. Writing `18` to
  `export` addresses nothing. On Pi 4 and earlier the header chip had base 0, which is why the
  bare number worked.
- Fix: resolve the header controller at connect by scanning `/sys/class/gpio/gpiochip*/label`,
  read its `base`, and offset every pin number by it. Base 0 reproduces the old behavior exactly.
- Regression test: fake sysfs exposing a non-zero base; assert the driver exports `base + pin`.

### D2 Hardcoded `pwmchip0`

- Impact: PWM silently binds to whatever `pwmchip0` happens to be, or is reported absent.
- Root cause: the path is a literal. On the target, `pwmchip0` resolves to
  `/sys/devices/platform/axi/1000120000.pcie/1f0009c000.pwm` with `npwm=4`, which is the RP1 PWM,
  but the index is not guaranteed and differs across models and overlays.
- Fix: resolve the PWM chip through its backing device rather than by index.
- Regression test: fake sysfs with the PWM chip at a non-zero index.

### D3 PWM channel to pin mapping is Pi 4 specific

- Impact: on a Pi 5 the PWM channel the driver enables does not correspond to the documented
  output pin, so an output reports enabled while a different pin is driven.
- Root cause: the channel/pin relationship comes from the device tree overlay and is assumed
  fixed by the driver.
- Fix: derive the mapping from the model, or document the required overlay and validate it.
- Regression test: fake sysfs for both mappings.

### D4 Export and unexport ignore the result of `write()`

- Impact: a failed export is reported as success, so failures surface later as a confusing
  direction-write error instead of a clear message. This is what a Pi 5 user actually sees.
- Root cause: `write(fd, buffer, bytes_written);` with no check in `rpio_pin_export`,
  `rpio_pin_unexport`, `rpio_pwm_export` and `rpio_pwm_unexport`.
- Fix: check the return value and report the failing pin.
- Regression test: fake sysfs whose `export` rejects a write.

### D5 Unterminated buffer passed to `atoi`

- Impact: undefined behavior; a garbage input or PWM reading is possible.
- Root cause: `rpio_pin_read` and `rpio_pwm_get_enable` declare `char value_str[3]`, `read()` at
  most 3 bytes into it and call `atoi()` without ever writing a terminator. A short read leaves
  the tail uninitialized and a full read leaves no terminator at all. `read() == 0` is also not
  treated as a failure.
- Fix: reserve space for the terminator, treat a zero-length read as failure, terminate explicitly.
- Regression test: fake sysfs returning an empty value file and a value without a newline.

### D6 Division by zero in the PWM readback paths

- Impact: `inf` or `nan` published as frequency or duty, and `rpio_pwm_set(ch, 0, 0)` written to
  the device.
- Root cause: `sensors_timer_callback` and the `AUX_GPIO_OUTLET_FREQUENCIES` and
  `AUX_GPIO_OUTLET_DUTY` change handlers compute `1 / (period / 1e9)` and
  `(double)duty_cycle / period * 100` without checking `period`, and the change handlers ignore
  the `rpio_pwm_get()` result entirely, leaving `period` at its initial `0`.
- Fix: check the result, and guard the division.
- Regression test: fake sysfs whose PWM read fails and one that reports a zero period.

### D7 Failed connect leaves pins exported and the connection switch in no state

- Impact: a failed connect leaks every pin exported before the failure, and publishes a
  `CONNECTION` property with neither item set.
- Root cause: `rpio_export_all()` returns on first failure without rolling back, and the failure
  branch calls `indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, false)`,
  which clears the item it is supposed to set. This violates the transactional open contract in
  `AGENTS.md`.
- Fix: unexport on rollback and set the disconnected item to `true`.
- Regression test: fake sysfs failing partway through export; assert rollback and switch state.

### D8 Pulse length is passed as the pin value

- Impact: none observable. Found by source audit only.
- Root cause: `set_gpio_outlets()` calls
  `rpio_set_output_line(i, (int)pulse_length_item->number.value, pwm)` where the second argument is
  the pin value.
- Fix: pass `1`.
- Regression test: **not possible at the boundary.** Both sinks reduce the argument to a boolean
  (`char val = value ? '1' : '0'`), so every non-zero pulse length produces exactly the same sysfs
  write as `1`. No test can distinguish the two, so this stays an audit finding and is fixed for
  clarity, not to repair observable behavior.

### D9 One second blocking sleep in the connect path

- Impact: connect is blocked for a fixed second even when udev has already settled.
- Root cause: `indigo_usleep(1000000)` between export and direction setup in `rpio_export_all()`.
- Fix: poll for the direction file to appear with a bounded timeout.
- Regression test: fake sysfs that publishes the direction file late; assert bounded wait.

### D10 Eight duplicated pulse timer callbacks

- Impact: maintenance only.
- Root cause: `relay_1_timer_callback` through `relay_8_timer_callback` are byte-identical apart
  from the index.
- Fix: one callback driven by the index.

### D11 Vestigial serial-driver state

- Impact: misleading diagnostics. Three error messages print `PRIVATE_DATA->handle`, which is never
  assigned, so every GPIO failure logs handle `0`.
- Root cause: `handle`, `udp`, `count_open` and `port_mutex` are carried over from a serial driver
  and unused; `port_mutex` is initialized in `create_device()` and never destroyed, as is
  `relay_mutex`.
- Fix: remove the unused fields, destroy what is initialized.

### D12 `fprintf(stderr, ...)` instead of the INDIGO log

- Impact: one error bypasses INDIGO logging.
- Root cause: `rpio_set_input()` reports a failed direction write with `fprintf`.
- Fix: use `INDIGO_DRIVER_ERROR`.

### D14 The input poller survives disconnect

- Impact: severe. After a disconnect the one second poll keeps running, keeps reading pins that the
  same disconnect has already unexported, and keeps touching a device the driver is about to free.
  `indigo_cancel_timer_sync()` in the disconnect branch does not reliably stop it.
- Discovery: reproduced, not audited. Consecutive test cases interfere with each other. After a
  case that connects successfully, the framework log keeps printing
  `rpio_pin_read:483: Failed to open gpio19 value for reading` once per second while the *next*
  case is running, and `disconnect_serial_device()` then times out after its full five seconds.
  With the poller of the previous device still running, later cases fail non-deterministically:
  `outputs are written and read back`, `pulsed output returns to zero`,
  `inputs are polled and published`, `PWM channels are programmed when present` and
  `plain GPIO is used when no PWM chip is present` all fail or pass depending on timing. Those five
  failures are collateral of this one defect, not five separate defects.
- Root cause: `sensors_timer_callback()` re-arms itself with `indigo_reschedule_timer()` as its last
  statement, with no check that the device is still connected. The disconnect branch cancels the
  timer while that callback is in flight, and the re-arm at the end of the in-flight callback
  reinstates it. `indigo_reschedule_timer()` logs nothing in this case: the log contains no
  `Attempt to reschedule timer without reference!` line, so the reference was valid again by the
  time the callback re-armed it.
- Fix: gate the re-arm on the connection state the disconnect path clears before cancelling, so a
  cancelled poller cannot reinstate itself.
- Regression test: connect, disconnect, and assert that no further sysfs read is recorded and that
  the disconnect completes without waiting.

### D13 PWM properties updated when no PWM is present

- Impact: `AUX_GPIO_OUTLET_FREQUENCIES` and `AUX_GPIO_OUTLET_DUTY` are published on every poll even
  when hidden, and their change handlers run unconditionally.
- Fix: skip when the chip is absent.

## Plan

Each step is independently verifiable and is marked here with its result as soon as it runs.

1. **Record this audit and baseline.** — done, this file.
2. **Fake sysfs harness.** — done. A mount namespace was evaluated first and does work on the
   target (`unshare --map-root-user --mount` plus `mount --bind` needs no root), but the repository
   already has a better idiom for exactly this shape of driver: `test_aux_joystick_hid` recompiles
   the driver with `-Dopen=..._test_open` and friends and supplies the system calls from the test.
   `indigo_test/integration/test_aux_rpio_sysfs.c` follows that idiom and models the kernel side of
   sysfs: export creates a pin node that defaults to direction `in`, direction and value behave
   like their real counterparts, PWM channels are exported and programmed, and every interaction is
   appended to an ordered, timestamp-free trace. `stat` is replaced through a function-like macro so
   that `struct stat` is left alone. The harness needs no privileges, no namespace and no real
   hardware, and it runs on any platform the rest of the suite runs on.
3. **Characterization suite against the original driver.** — done, 18 cases. Baseline run against
   the unmodified driver, on the target:

   ```sh
   make -C indigo_test build/integration/test_aux_rpio_sysfs
   cd indigo_test && ./build/integration/test_aux_rpio_sysfs
   ```

   Result: **6 passed, 12 failed**.

   | Case | Baseline | Meaning |
   | --- | --- | --- |
   | driver metadata, interface and property contract | pass | behavior to preserve |
   | connect exports and configures every pin | pass | behavior to preserve |
   | pins are offset by the controller base | fail | D1 |
   | outputs are written and read back | fail | collateral of D14 |
   | pulsed output returns to zero | fail | collateral of D14 |
   | inputs are polled and published | fail | collateral of D14 |
   | PWM channels are programmed when present | fail | collateral of D14 |
   | plain GPIO is used when no PWM chip is present | fail | collateral of D14 |
   | failed PWM readback is not written back | pass | D6 not reachable through this path |
   | rejected export fails the connection cleanly | pass | D4 and D7 not reached on the first export |
   | failed connect rolls back exported pins | fail | D7 |
   | empty value read is an error | fail | D5 |
   | connect does not sleep for a fixed second | fail | D9, measured 1000000 us in one call |
   | connect tolerates a late direction file | fail | collateral of D14 |
   | names are applied to labels | pass | behavior to preserve |
   | disconnect unexports and reconnect works | fail | D14 |
   | shutdown is rejected while connected | pass | behavior to preserve |
   | repeated initialization and shutdown | pass | behavior to preserve |

   The five collateral failures are all caused by D14 and are expected to clear once it is fixed;
   they are not counted as separate defects. Two cases that were written as reproducers pass
   against the original driver, which is recorded honestly above rather than presented as coverage
   of D4, D6 and D7.

   Three harness defects were found and fixed while establishing this baseline, and none of them
   were driver defects: the fake first left an exported pin in direction `out` instead of the
   kernel default `in`; the cleanup path tore a driver down twice when a connection was expected to
   fail, because `start_serial_driver()` already tears down on failure; and a blanket rename made
   the cleanup helper call itself. They are recorded here so the baseline numbers above are not
   read as driver behavior.
4. **Reference trace.** Record the ordered sysfs interactions of the original driver through the
   fake and normalize it for later comparison. — pending
5. **Fix D1 to D3**, the numbering and PWM resolution defects. — pending
6. **Fix D4 to D9**, the correctness defects, one at a time. — pending
7. **Clean up D10 to D13.** — pending
8. **Generator migration.** Reverse-extract `indigo_aux_rpio.driver`, regenerate, and confirm the
   characterization suite and reference trace still match. — pending
9. **Project and status registration.** Xcode groups, `MIGRATION_STATUS.md`, `PROPERTIES.md`. — pending
10. **Final verification.** Strict build, sanitizer run, full suite, driver version bump, diff
    audit, test records. — pending

## Final test summary

- Simulated tests: 18 executed, 6 passed. This is the pre-migration baseline against the unmodified
  driver, not a completion claim. The 12 failures are the recorded expected baseline failures for
  D1, D5, D7, D9 and D14 above.
- Hardware tests: 3 executed, 3 passed. Non-destructive topology inspection only, as decided
  above; no physical pin was driven.
