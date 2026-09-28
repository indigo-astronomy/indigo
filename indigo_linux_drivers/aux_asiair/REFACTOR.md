# ZWO ASIAIR power port AUX driver refactoring and validation record

Status: refactoring complete on 2026-09-20. Migrated to the generator, every recorded defect
fixed, hardware-free suite and physical Raspberry Pi 5 test both green.

## Environment

All work for this record ran on the target hardware, not on a development host:

- Raspberry Pi 5 Model B Rev 1.0, `pinctrl-rp1` 40-pin header controller.
- Debian GNU/Linux 12 (bookworm), kernel `6.12.87+rpt-rpi-2712`, aarch64.
- gcc 12.2.0, GNU Make 4.3.

Note that the target is a bare Raspberry Pi 5, not an ASIAIR. No ASIAIR was available, and none of
the scenarios below needs one, because the driver reaches the power ports through the ordinary
Raspberry Pi header.

## Current-state audit

### Architecture and implementation

`indigo_aux_asiair.c` was a hand-written INDIGO 2.0 AUX driver and a near copy of
`indigo_aux_rpio.c`: the same sysfs GPIO and PWM access layer, the same property handling, the
same connection lifecycle. It exposes one static logical device, `ZWO Power Ports ASIAIR`, created
at `INDIGO_DRIVER_INIT`, with no hot-plug and no generator input.

It differs from the Raspberry Pi GPIO driver in three ways:

- four outputs on BCM pins `{ 12, 13, 26, 18 }` instead of eight;
- no inputs at all, so no `AUX_GPIO_SENSORS` and no `AUX_SENSOR_NAMES`;
- the two PWM channels drive **Output #1 and Output #4**, not the first two outputs. Channel 0 is
  GPIO 12 and channel 1 is GPIO 18.

### Public properties

`AUX_OUTLET_NAMES` (4), `AUX_GPIO_OUTLETS` (4), `AUX_OUTLET_PULSE_LENGTHS` (4),
`AUX_GPIO_OUTLET_FREQUENCIES` (2) and `AUX_GPIO_OUTLET_DUTY` (2). Interface bit is
`INDIGO_INTERFACE_AUX_GPIO`.

### Existing tests

None. No simulator, no fake SDK and no test source anywhere under `indigo_test/`.

## Baseline

Built from a clean clone of `refactoring` at `64eb95763`:

```sh
cd ~/indigo && make
```

Result: `EXIT=0`, no compiler warnings attributable to this driver. No existing tests could be run
as a baseline, because none existed.

## Hardware-test decision

Hardware testing **was** performed, on the bare Raspberry Pi 5 described above, once the owner
confirmed that nothing is connected to the 40-pin header. No ASIAIR was available, but the power
ports are ordinary header pins, so a bare Pi exercises everything the driver does; only the power
circuitry of a real ASIAIR is left unverified.

`indigo_test/hardware/test_aux_asiair_hw.c` drives the four ports through the real driver and
verifies each pin with the `pinctrl` tool, which reads the pin controller registers rather than the
sysfs files the driver writes. It is opt-in, excluded from the normal integration target, and
refuses to run without `--run`:

```sh
make -C indigo_test test-aux-asiair-hw
```

## Found defects

This driver shared its hardware layer with `aux_rpio` by copy, so it inherited the same defects.
They are numbered as in `../aux_rpio/REFACTOR.md`, which carries the full analysis of each.

| Defect | Reproduced here | Status |
| --- | --- | --- |
| D1 hardcoded sysfs pin numbering breaks on a Raspberry Pi 5 | yes | fixed |
| D2 hardcoded `pwmchip0` | by audit | fixed |
| D3 PWM channel to pin mapping is overlay dependent | by audit | still assumed, but no longer reached unless PWM is asked for |
| D4 export and unexport ignore the result of `write()` | yes | fixed |
| D5 unterminated buffer passed to `atoi` | by audit | fixed |
| D6 division by zero in the PWM readback paths | yes | fixed |
| D7 failed connect leaks pins and leaves the switch in no state | yes | fixed |
| D8 pulse length passed as the pin value | by audit, not observable | fixed |
| D9 one second blocking sleep in the connect path | yes | fixed |
| D10 duplicated pulse timer callbacks | by audit | fixed |
| D11 vestigial serial-driver state | by audit | fixed |
| D12 `fprintf(stderr, ...)` instead of the INDIGO log | by audit | fixed |
| D13 PWM properties updated when no PWM is present | by audit | fixed |
| D14 the input poller survives disconnect | not applicable | not applicable |
| D9a the bounded wait waited for readability instead of writability | shared fix | fixed |
| D15 PWM routing inferred from chip existence | yes, on hardware | fixed, PWM is opt-in through `X_AUX_PWM` |

D14 does not apply: this driver has no inputs and therefore no input poller. Its PWM settings
timer is the only periodic work, and the generated poll callback returns as soon as the device is
no longer connected.

### D15 measured on this driver

Two of the four ports are silently dead on a Raspberry Pi 5, and they are exactly the two the
driver assigns to PWM channels:

```
output #1 (GPIO 12): DEAD
output #2 (GPIO 13): switches
output #3 (GPIO 26): switches
output #4 (GPIO 18): DEAD
2 of 4 outputs reach the header
```

The cause is shared with `aux_rpio` and the full analysis is in `../aux_rpio/REFACTOR.md` under
D15. For this driver the consequence was worse in proportion: half of the power ports did nothing
while the driver reported them as switched. The fix is the same `X_AUX_PWM` switch, defaulting to
disabled, and after it the same hardware test measures four of four ports switching.

`README.md` has the same documentation defect: it tells the user to add `dtoverlay=pwm-2chan`,
which is correct up to a Raspberry Pi 4 and insufficient on a Raspberry Pi 5.

### One defect specific to this driver

The `asiair_export_all()` and `asiair_read_output_lines()` helpers hardcoded the assumption that
the PWM backed outputs are outputs 1 and 4 by indexing `output_pins[1]` and `output_pins[2]`
explicitly and looping from index 1 in some places and from 0 in others. The shared layer now
takes the channel-to-line mapping as an explicit table, so the ASIAIR mapping `{ 0, 3 }` is data
rather than interleaved special cases. A dedicated test asserts that Output #1 and Output #4 are
not exported as plain GPIO while Output #2 and Output #3 are, and that switching Output #4 reaches
PWM channel 1 rather than channel 3.

## Work done

1. **Characterization suite.** `indigo_test/integration/test_aux_asiair_sysfs.c`, 15 cases,
   sharing the fake sysfs boundary in `indigo_test/integration/sysfs_gpio_fake.h` with `aux_rpio`.
   Baseline against the unmodified driver: **8 passed, 7 failed**, the failures being D1, D4, D6,
   D7 twice, D9 and one collateral failure of the case that follows a failed connection.
2. **Shared sysfs layer.** The driver now includes `../aux_rpio/shared/rpio_sysfs.h` and `.c`, the
   way `dome_dragonfly` includes `aux_dragonfly/shared`. The duplicated hardware layer is gone.
3. **Generator migration.** `indigo_aux_asiair.driver` is the source of truth; the checked-in
   `.c`, `.h` and `_main.c` are regenerated from it. Driver version raised from `0x02000002` to
   `0x03000003`.
4. **Suite green.** 15 of 15 pass against the refactored driver.
5. **Sanitizers.** `make -C indigo_test test-aux-asiair-sysfs-asan` reports nothing; exit 0.
6. **Registration.** `MIGRATION_STATUS.md` updated and every new file registered in
   `indigo.xcodeproj/project.pbxproj`. No property was added or removed, so `PROPERTIES.md` needs
   no change.

### Scenario to test mapping

| Class standard area | Scenario |
| --- | --- |
| Identity and property contract | `driver metadata, interface and property contract` |
| Concrete driver properties, absence of inputs | `driver metadata, interface and property contract` |
| Connection and resource acquisition | `connect exports and configures every pin` |
| Hardware topology | `pins are offset by the controller base` |
| Representative writable property | `outputs are written and read back` |
| Driver-specific timed behavior | `pulsed output returns to zero` |
| Driver-specific capability mapping | `PWM drives the first and the fourth output` |
| Capability absence | `plain GPIO is used when no PWM chip is present` |
| Readback failure injection | `failed PWM readback is not written back` |
| Open failure injection | `rejected export fails the connection cleanly` |
| Initialization rollback and balanced resources | `failed connect rolls back exported pins` |
| Non-blocking entry points | `connect does not sleep for a fixed second` |
| Persistent names | `names are applied to labels` |
| Disconnect, reconnect and cleanup | `disconnect unexports and reconnect works` |
| INIT/SHUTDOWN lifecycle | `shutdown is rejected while connected`, `repeated initialization and shutdown` |

### Gaps

- The ports were switched on a bare Raspberry Pi 5, not on an ASIAIR, so the ASIAIR power
  circuitry, its connectors and its current handling are unverified.
- PWM output was never observed on a pin, because no pin is routed to the controller on this
  machine, so the PWM path is covered by the fake only. See D15.
- The driver exposes no guider interface, so the guiding pulse accuracy measurement does not apply.

## Switch target adoption: PWM requests and the pulse finalizer (3.0.0.6, 2026-09-27)

Findings TGT-018 and the aux_asiair part of TGT-B03 in `indigo_drivers/REVIEW_SWITCH_TARGETS.md`.
Linux x64 (Ubuntu 24.04, gcc 13.3), hardware-free suite only; no hardware run.

- **Baseline on Linux x64 (reproduced, fixed in the test build):** the unchanged 3.0.0.5 suite
  failed 14 of 16 cases. Ubuntu's gcc enables `_FORTIFY_SOURCE` by default, so glibc's inlined
  fortified `open()` and `read()`, renamed by the `-Dopen=sysfs_test_open` / `-Dread=...`
  replacements, called the real functions through their asm aliases (`nm -u` of the test object
  showed `open` and `read`, not the fakes); every connect then failed on
  `/sys/class/gpio/export`. `AUX_RPIO_SYSFS_TEST_REPLACEMENTS` in `indigo_test/Makefile` now starts
  with `-U_FORTIFY_SOURCE` (shared with `aux_rpio`). The contract case pinned `0x03000004` while
  the driver was already 3.0.0.5; it now asserts the API generation, as `indigo_test/AGENTS.md`
  requires.
- **TGT-B03 (reproduced):** the 1 s poll `asiair_update_pwm()` wrote value and target of
  `AUX_GPIO_OUTLET_FREQUENCIES` and `AUX_GPIO_OUTLET_DUTY` from the channels and published them OK
  without a BUSY check. Polls are `INDIGO_TASK_PRIORITY_TIME` tasks and run ahead of a queued
  change handler, so a request copied while the poll came due was shown OK with the old value
  before it was applied, and the handler, which programs the channels from `number.target` in
  `asiair_apply_pwm()`, programmed the old setting again. **Fix:** the poll leaves value, target
  and state of a BUSY PWM property alone and publishes only the one that is not BUSY; the handlers
  already program the channels from the targets and are unchanged. A property that is not BUSY
  keeps being refreshed with value and target, so a handler of the other property still programs
  the setting the channel reports.
- **Regression tests:** `PWM duty request survives the poll` and `PWM frequency request survives
  the poll` hold the device queue with a gate handler, send the request for Output #1, change the
  other setting of channel 0 in the fake (as a reprogramming outside the driver would) so the poll
  publishes the other PWM property as a witness, let the poll come due behind the gate, and check
  that the first result after the request is OK with the requested value, published after the
  poll ran, and that the channel runs at the requested setting with the external one kept. Against
  3.0.0.5 both failed (first result OK with the old 100, channel left at 50 Hz / 100 % and
  100 Hz / 50 %); both pass with 3.0.0.6, also in the ASan build.
- **TGT-018 (analysis, Won't fix):** `relay_pulse_finalizer()` writes `sw.value = false` only for
  an output whose pulse has elapsed, switches that output off in the same step and publishes; the
  handler compares every item with a fresh read of the outputs. An ON request for that output
  copied before the finalizer ran is ignored exactly like the same request a moment earlier while
  the pulse ran (the value was already ON), so no request is lost. The target cannot replace the
  write, the same way as in `aux_dragonfly` TGT-016: items an any-of-many request does not carry
  keep their previous target, so a handler reading targets cannot tell a new ON from the ON of the
  finished pulse. A temporary case (not kept) held the queue past the end of a 500 ms pulse on
  Output #3 and queued a request for Output #2 only. Unchanged driver: `value 26 0` from the
  finalizer, then only `value 13 1`, Output #3 off. With the finalizer skipping a BUSY property and
  the handler reading targets: Output #3 was pulsed again (`value 26 1`, 500 ms later
  `value 26 0`), and after its pulse length had been set to 0 it was switched on for good. No code
  change for TGT-018.
- **Verification:** `TZ=Europe/Bratislava python3 tools/run_driver_test.py aux_asiair`, 18/18.
  Regeneration reproduces the checked-in output.

```sh
cd indigo_test && INDIGO_TEST_CASE_FILTER="request survives the poll" ./build/integration/test_aux_asiair_sysfs
```

## Failed PWM writes (3.0.0.7, 2026-09-27)

The aux_asiair part of TGT-D11 in `indigo_drivers/REVIEW_SWITCH_TARGETS.md` (unrelated to D11 in
this file), fixed the same way as aux_rpio 3.0.0.11. Linux x64 (Ubuntu 24.04, gcc 13.3),
hardware-free suite only; no hardware run.

- **TGT-D11 (reproduced):** both PWM handlers ignored the result of `asiair_apply_pwm()`, so a
  setting the kernel rejected was reported OK with the rejected value while the channel kept its
  old setting. **Fix:** the channel readback of the poll moved into `asiair_read_pwm()` (one read
  of each channel per poll, the BUSY guard of 3.0.0.6 unchanged); on a failed `asiair_apply_pwm()`
  the handler reads the channels back into value and target of its property with
  `asiair_read_pwm()` and publishes ALERT. The generator still owns the OK prologue and the final
  update (checked in the regenerated handlers). The connect-time `asiair_apply_pwm()` is unchanged.
- **Regression tests:** `PWM duty write failure is reported` and `PWM frequency write failure is
  reported` set `pwm_write_fails` in the shared `sysfs_gpio_fake.h` (the kernel rejects a period
  or duty cycle with `EINVAL` and keeps the previous setting), request 50 % / 200 Hz on Output #1
  and check that the first result after the request is ALERT showing the 100 the channel still
  runs at, with the channel unchanged. Against 3.0.0.6 both failed (first result OK with the
  rejected 50 / 200); both pass with 3.0.0.7, also in the ASan build.
- **Test build:** the ASan rule of `test_aux_rpio_sysfs` now lists `sysfs_gpio_fake.h` as a
  prerequisite like the other three sysfs rules; `make -n -W integration/sysfs_gpio_fake.h`
  relinks both ASan tests.
- **Verification:** `TZ=Europe/Bratislava python3 tools/run_driver_test.py aux_asiair`, 20/20.
  Regeneration reproduces the checked-in output.

```sh
cd indigo_test && INDIGO_TEST_CASE_FILTER="write failure is reported" ./build/integration/test_aux_asiair_sysfs
```

## Final test summary

- Simulated tests: 20 executed, 20 passed (Linux x64, 3.0.0.7). Earlier: 18 executed, 18 passed
  (Linux x64, 3.0.0.6) and 16 executed, 16 passed on
  the Raspberry Pi 5. The pre-migration baseline against the unmodified driver was 15 executed,
  8 passed.
- Hardware tests: 3 executed, 3 passed, on a Raspberry Pi 5 Model B Rev 1.0 with nothing connected
  to the 40-pin header, and repeated twice with identical results. All four ports physically
  switch, measured through `pinctrl` rather than through the driver's own interface. Before the
  D15 fix the same test measured two of four. No ASIAIR was available, so the ASIAIR power
  circuitry itself is still unverified.
