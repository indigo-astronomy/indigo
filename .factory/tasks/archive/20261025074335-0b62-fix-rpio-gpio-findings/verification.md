# Verification

Verdict: pass

Tested candidate: `task/20261025074335-0b62-fix-rpio-gpio-findings` at master `64380bf9e` plus the uncommitted diff, the same tree on both Pis (`git diff` of the Raspberry Pi 3B+ checkout is byte-identical to the development tree after the records were copied back). Mode: non-interactive hardware run (owner request: fix the RPi findings, verify aux_rpio on both Pis).

## Hosts
| Host | Board | OS / kernel | Header controller |
| --- | --- | --- | --- |
| indigosky.local | Raspberry Pi 5 Model B Rev 1.0 | Debian 12, 6.12.87+rpt-rpi-2712, aarch64 | pinctrl-rp1, base 571 |
| rpi.local | Raspberry Pi 3 Model B Plus Rev 1.3 | 6.18.50+rpt-rpi-v8, aarch64 | pinctrl-bcm2835, base 512 |

Nothing connected to either 40-pin header; the Raspberry Pi 3B+ carries an OV5647 camera on CSI only.

## Acceptance
| ID | Result | Evidence |
| --- | --- | --- |
| AC-001 | pass | fake: `pre-exported pins stay exported`, `failed connect keeps pre-exported pins` (both drivers, both Pis); hardware: `a pin exported before the connection stays exported` 5/5 on both Pis |
| AC-002 | pass | `failed pulse end is reported` (both drivers, both Pis) |
| AC-003 | pass | `zero PWM period is reported` (both drivers, both Pis) |
| AC-004 | pass | full recorded runs below |

## Recorded runs (final, `python3 tools/run_driver_test.py`, TZ=Europe/Bratislava, 09:10-09:14 after the quality-review test fixes)
| Host | Command | Result |
| --- | --- | --- |
| Raspberry Pi 5 | `aux_rpio` | 28/28 OK |
| Raspberry Pi 5 | `aux_rpio --hw --type "Raspberry Pi 5"` | 5/5 OK (started with all inputs at function `none`) |
| Raspberry Pi 5 | `aux_asiair` | 24/24 OK |
| Raspberry Pi 5 | `aux_asiair --hw --type "Raspberry Pi 5"` | 3/3 OK |
| Raspberry Pi 3B+ | `aux_rpio --type "fake SDK (Raspberry Pi 3B+)"` | 28/28 OK |
| Raspberry Pi 3B+ | `aux_rpio --hw --type "Raspberry Pi 3B+"` | 5/5 OK |
| Raspberry Pi 3B+ | `aux_asiair --type "fake SDK (Raspberry Pi 3B+)"` | 24/24 OK |
| Raspberry Pi 3B+ | `aux_asiair --hw --type "Raspberry Pi 3B+"` | 3/3 OK |

The same full scope had also passed at 09:01-09:05 before the review fixes (test-only changes). Source and test files on both Pis are hash-identical to the development tree.

Other checks: `make -C indigo_test test-aux-rpio-sysfs-asan` passes on the Raspberry Pi 5. On the Raspberry Pi 3B+ the ASan runtime aborts before `main` (`sanitizer_allocator_primary64.h:131` CHECK, kernel address space layout): no ASan result for that host. `make -j4` of the tree: exit 0 on both Pis. Regeneration of both `.driver` files reproduces the checked-in output.

## Defect found during verification
The first hardware run of the candidate on the Raspberry Pi 5 failed 3/5 recorded (`connect claims the header pins`, `inputs are configured and readable`): pin 19 stayed at function `none`. Reproduced by hand (`export` then `direction` reads `in` while `pinctrl` shows `none`; writing `in` sets `ip`) and in the fake (`inputs are configured on the RP1` fails against the unfixed `rpio_sysfs_set_direction()`). Fixed (aux_rpio D16), then the full scope was rerun on both Pis as above. The first recorded lines (type detected as the device name) were discarded and rerun with `--type`.

## Not verified
ccd_rpi autofocus (FND-20261024202957-6bd9-fix-ccd-rpi-001): neither Pi has an autofocus camera; the OV5647 tuning file has no `rpi.af` and the IPA exposes no AF or lens controls. ASIAIR power circuitry (no ASIAIR). PWM output on a header pin (not enabled on either Pi).
