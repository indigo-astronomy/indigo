# Specification
Status: final

## Problem and outcome
The GPIO drivers release only what they exported themselves, and never report an output as off when it could not be switched off.

## In scope
`indigo_linux_drivers/aux_rpio/shared/rpio_sysfs.[ch]`; `indigo_aux_rpio` 12, `indigo_aux_asiair` 8.

## Out of scope
ccd_rpi autofocus (FND-20261024202957-6bd9-fix-ccd-rpi-001).

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | A pin or PWM channel exported before the connection is used, and stays exported after a disconnect and after a failed connect; pins the session exported are released. | `pre-exported pins stay exported`, `failed connect keeps pre-exported pins` (both suites); rpio hardware case on both Pis |
| AC-002 | When the pulse end write fails, AUX_GPIO_OUTLETS goes ALERT with the output still on; switching it off again works. | `failed pulse end is reported` (both suites) |
| AC-003 | A zero PWM period is reported as ALERT with the previous finite values and nothing is written. | `zero PWM period is reported` (both suites) |
| AC-004 | The rest of the suites and the hardware tests are unchanged. | recorded runs |

## Failure behavior and edge cases
Pins left exported by a crashed earlier run of the driver are no longer unexported by the next session; they are owned by nobody and stay. This is the ownership rule FND-000016 asks for.

## Permissions, data, and security requirements
None.

## Compatibility and nonfunctional requirements
Linux only, as before.

## Assumptions and open questions
None.

## Owner decisions and requirement changes
2026-10-25: fix the RPi findings; verify aux_rpio on both Pis.
