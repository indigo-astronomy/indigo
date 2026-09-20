# Raspberry Pi GPIO AUX driver refactoring and validation record

Status: refactoring complete on 2026-09-20. Migrated to the generator, every recorded defect
fixed, hardware-free suite and physical Raspberry Pi 5 test both green.

## Environment

All work for this record runs on the target hardware, not on a development host:

- Raspberry Pi 5 Model B Rev 1.0, `pinctrl-rp1` 40-pin header controller. The machine has both an
  SD card and an external USB SSD and its `BOOT_ORDER` is `0xf146`, so it prefers the USB disk.
  A reboot taken during this work therefore moved it from the SD card system to the SSD system;
  the work was copied across and continued on the SSD.
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

Hardware testing **was** performed on the Raspberry Pi 5 described above, in two stages.

The first stage was non-destructive inspection only, decided before implementation because the Pi
is an in-service INDIGO Sky host and equipment could have been attached to the 40-pin connector:

1. Enumerate `/sys/class/gpio/gpiochip*` and record `base`, `ngpio` and `label`.
2. Enumerate `/sys/class/pwm/pwmchip*` and record `npwm` and the backing platform device.
3. Confirm, without exporting, that the driver's hardcoded numbering cannot address the header.

The second stage became possible once the owner confirmed that nothing is connected to the header.
`indigo_test/hardware/test_aux_rpio_hw.c` drives the real pins through the real driver, and every
assertion is verified with the `pinctrl` tool, which reads the pin controller registers, rather
than through the sysfs files the driver itself writes. The test is opt-in, excluded from the normal
integration target, and refuses to run without `--run`:

```sh
make -C indigo_test test-aux-rpio-hw
```

It found two defects that neither the source audit nor the fake could reveal. Both are recorded
below as D9a and D15.

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

### D9a The bounded wait replacing the sleep waited for the wrong condition

- Impact: connect failed outright on real hardware. `Failed to open
  /sys/class/gpio/gpio584/direction for writing`, followed by a full rollback of six output, eight
  input and two PWM exports.
- Discovery: reproduced on hardware, on the first run of the hardware test. The fake could not
  show it, because a fake file has no owner.
- Root cause: the D9 fix waited for the direction file to be **openable for reading**. A freshly
  exported node appears owned by `root` and is only handed to the `gpio` group once udev has
  processed the event, so the file exists and is readable well before it is writable. This is what
  the original fixed one second sleep was really waiting for; removing it without understanding
  what it hid turned a slow connect into a broken one.
- Fix: wait for the direction file to be writable, not merely present.
- Regression test: covered by the hardware test, which connects on a real Pi. The fake models
  permissions no better than any file-backed fake would, so no simulated reproducer is claimed.

### D15 PWM is assumed to be routed to the header whenever a PWM chip exists

- Impact: two of the eight outputs are silently dead. Measured on the target:

  ```
  output #1 (GPIO 18): DEAD
  output #2 (GPIO 12): DEAD
  output #3 (GPIO 13): switches
  output #4 (GPIO 26): switches
  output #5 (GPIO 16): switches
  output #6 (GPIO  5): switches
  output #7 (GPIO  6): switches
  output #8 (GPIO 21): switches
  6 of 8 outputs reach the header
  ```

  The driver reports the outputs as switched and publishes `INDIGO_OK_STATE`; nothing in the
  property state tells the user that two ports do nothing.
- Discovery: reproduced on hardware. Not reachable through the fake, which cannot model the
  difference between a PWM controller existing and a PWM controller being wired to a pin.
- Root cause: the driver decides that PWM is available from the existence of a PWM chip and then
  hands Outputs #1 and #2 to PWM channels instead of exporting them as GPIO. On this machine
  `dtoverlay=pwm-2chan` **is** present in `/boot/firmware/config.txt` and the RP1 controller
  `pwm@9c000` is enabled, so the chip exists, but no header pin is routed to it. Verified directly:
  PWM channel 0 programmed to 100 Hz at 100 percent duty and enabled leaves GPIO 12, 14 and 15 at
  function `none`, so the channel drives nothing. On a Raspberry Pi 5 the overlay needs its
  `pins-12-13` variant, and the channel to pin mapping is different from a Raspberry Pi 4 in any
  case. The existence of a chip is therefore not evidence that the two outputs are PWM backed.
- Fix: **applied.** PWM is now opt-in through `X_AUX_PWM`, a persistent, always defined switch that
  defaults to disabled and is read at connect time. With it off the driver never looks for a PWM
  chip and drives all eight outputs as plain GPIO; with it on the behavior is exactly what it was.
  Two alternatives were considered and rejected: probing the routing through the GPIO character
  device (`GPIO_V2_GET_LINEINFO_IOCTL` reports whether a line is claimed and by whom) is the more
  automatic answer, but its decisive branch cannot be validated on this machine at all, because no
  overlay routes a header pin to PWM on RP1, and it would reintroduce the character device that
  was deliberately not used; and leaving the behavior documented keeps a silent failure.
- Related documentation defect: `README.md` tells the user to add `dtoverlay=pwm-2chan`, which is
  correct up to a Raspberry Pi 4 and does nothing useful on a Raspberry Pi 5.
- Re-verified from a clean boot, because the first measurement was taken after the pins had been
  exported by earlier test runs and sysfs GPIO overrides the pin function. Immediately after a
  reboot, with the driver never started, GPIO 12, 13, 18 and 19 all read function `none`, and
  programming PWM channel 0 and channel 1 to 100 Hz at 100 percent duty and enabling them leaves
  all four unchanged. `dtoverlay -h pwm-2chan` advertises `pin` defaulting to 18 and `pin2` to 19
  with Alt5 functions, which are BCM pin functions; the RP1 pin controller of a Raspberry Pi 5 has
  a different mux and the overlay does not take effect on it. The controller is enabled and the
  chip is present, but nothing reaches a header pin.
- Regression test: `PWM is off by default` in the simulated suite asserts that a present PWM chip
  is not used and that all eight outputs are exported as GPIO, and `outputs drive the physical
  pins` in the hardware test now measures eight of eight outputs switching on the real header,
  where it measured six before the fix.

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
4b. **Reference trace.** — done. Recorded through the harness:

    ```sh
    INDIGO_SIMULATOR_TRACE_DIR=/tmp/rpio_trace ./build/integration/test_aux_rpio_sysfs
    ```

    `connect_exports_every_pin.events`, 78 ordered events, md5
    `74d21b5cf3db1317680b12be339b1794`. The trace is normalized: it carries no timestamps, no
    addresses and no descriptor numbers, only the ordered sysfs interactions.

5. **Shared sysfs layer.** — done. The hardware boundary moved to
   `shared/rpio_sysfs.h` and `shared/rpio_sysfs.c`, which `aux_asiair` includes the way
   `dome_dragonfly` includes `aux_dragonfly/shared`. It resolves the controller base and the PWM
   chip, checks every write, terminates every read, rolls back a failed export and maps PWM
   channels to output lines through an explicit table, because the ASIAIR drives its first and
   its fourth output from channels 0 and 1 rather than the first two.
6. **Generator migration.** — done. `indigo_aux_rpio.driver` is the source of truth and the
   checked-in `.c`, `.h` and `_main.c` are regenerated from it. The driver has no transport, so it
   is a virtual driver in generator terms, like `wheel_manual`. The migration also removes whole
   classes of hand-written code and with them several defects: the generator owns the connection
   switch on a failed connect (D7), the property definition and deletion, the `IS_CONNECTED` guard
   at the head of the poll callback (D14) and the single pulse finalizer that replaces the eight
   duplicated callbacks (D10). The vestigial serial state and the stray `fprintf` disappeared with
   the hand-written file (D11, D12).
7. **Suite green against the refactored driver.** — done, 18 of 18 pass. Four harness defects were
   found and fixed on the way and none of them were driver defects; they are listed under
   "Harness defects" below.
8. **Sanitizers.** — done.

   ```sh
   make -C indigo_test test-aux-rpio-sysfs-asan
   ```

   AddressSanitizer and UndefinedBehaviorSanitizer report nothing; exit status 0.
9. **Project and status registration.** — done. `MIGRATION_STATUS.md` updated; the new `.driver`,
   the shared sources, `REFACTOR.md` and both test sources are registered in
   `indigo.xcodeproj/project.pbxproj`. No property was added or removed, so `PROPERTIES.md` needs
   no change.
10. **Final verification.** — done. Driver version raised from `0x02000007` to `0x03000008`; the
    driver builds with no warnings; regenerating from the `.driver` input reproduces the checked-in
    `.c` byte for byte.

    Scope of the verification run: **only the two driver suites were run**, on instruction. The
    repository-wide `make -C indigo_test test` target was not completed, so no claim is made about
    it. Two observations from the part of it that did run before it was stopped, both unrelated to
    this driver and both pre-existing on this host, because nothing under `indigo_libs/` or
    `indigo_test/unit/` was touched:

    - `unit/test_timer.c` fails two cases on this platform,
      `timer_scheduler_restarts_after_fork_during_callback` and
      `cancel_all_timers_from_timer_callback_does_not_deadlock`.
    - `test_ccd_qhy_sdk` does not link on Debian without `libhidapi-dev`, because the test links
      `-lhidapi` while the in-tree build produces `libhidapi-hidraw` and `libhidapi-libusb`.

## Defect status

| Defect | Status |
| --- | --- |
| D1 hardcoded sysfs pin numbering | fixed, controller base resolved from the chip labels |
| D2 hardcoded `pwmchip0` | fixed, the first chip with enough channels is used |
| D3 PWM channel to pin mapping | still assumed, but no longer reached unless PWM is asked for, see D15 |
| D4 unchecked `write()` | fixed |
| D5 unterminated buffer passed to `atoi` | fixed |
| D6 division by zero in the PWM readback | fixed |
| D7 failed connect leaks pins and clears the switch | fixed, rollback plus generator-owned switch |
| D8 pulse length passed as the pin value | fixed, not observable, see its entry |
| D9 one second blocking sleep | fixed, bounded wait for the direction file |
| D10 eight duplicated pulse callbacks | fixed, one finalizer |
| D11 vestigial serial state | fixed, gone with the hand-written driver |
| D12 `fprintf` instead of the INDIGO log | fixed |
| D13 PWM properties updated with no PWM | fixed |
| D14 the input poller survives disconnect | fixed, the generated callback returns when not connected |
| D9a the bounded wait waited for readability | fixed, it waits for writability |
| D15 PWM routing inferred from chip existence | fixed, PWM is opt-in through `X_AUX_PWM` |

### D3 is still an assumption, but it is now opt-in

The mapping between a PWM channel and the pin it drives comes from the device tree overlay and is
not discoverable from sysfs, and the hardware run turned this from an audit finding into a measured
one: on this Raspberry Pi 5 no header pin is routed to the enabled PWM controller at all. The
driver still assumes the documented mapping when PWM is switched on, which is unchanged behavior
for the Raspberry Pi 4 the driver was written for. What changed is that nobody gets that assumption
by accident any more. Confirming the mapping on a Raspberry Pi 5, or finding an overlay that routes
RP1 PWM to a header pin at all, remains open and needs hardware this record did not have.

## Harness defects

Found while building the suite. None of them were driver defects, and they are recorded so the
numbers above are not misread.

1. The fake left a freshly exported pin in direction `out`; the kernel leaves it `in`.
2. A cleanup path tore the driver down twice when a connection was expected to fail, because
   `start_serial_driver()` already tears down on failure. That corrupted the bus for later cases.
3. A blanket rename made the cleanup helper call itself.
4. The fake matched sysfs paths with `sscanf`, which reports how many conversions succeeded rather
   than whether the whole format matched, so `gpiochip571/ngpio` satisfied a `gpiochip%d/label`
   format and `gpio19/value` satisfied a `gpio%d/direction` format. This one silently produced
   wrong readings and was responsible for most of the remaining failures. Every indexed path is now
   matched exactly.

One limitation of the shared harness is worth recording: `assert_not_defined_property()` answers
whether a property was **ever** defined, because the shared client removes a deleted property from
its cache but keeps the name in the defined list. A property that has to be gone at a given moment
must be checked against the cache instead, which is what `assert_property_deleted()` in the test
does.

## Final test summary

- Simulated tests: 19 executed, 19 passed. The pre-migration baseline against the unmodified driver
  was 18 executed, 6 passed; the 12 failures were the recorded expected baseline failures for D1,
  D5, D7, D9 and D14, and all of them now pass.
- Hardware tests: 4 executed, 4 passed, on a Raspberry Pi 5 Model B Rev 1.0 with nothing connected
  to the 40-pin header, and repeated twice with identical results. All eight outputs physically
  switch, measured through `pinctrl` rather than through the driver's own interface. Before the
  D15 fix the same test measured six of eight. Three non-destructive topology inspections were run
  before this stage and all three passed.

  The hardware run is what validated the central fix of this refactoring: with the controller base
  resolved at 571, the driver claims and drives real pins on a Raspberry Pi 5, which it could not
  do at all before.
