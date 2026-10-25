# Findings dispositions

| Finding | Disposition | Evidence |
| --- | --- | --- |
| FND-000014 | fixed (before this task), regression test added | `rpio_sysfs_get_pwm()` rejects a failed read and a zero period, callers check the result; `zero PWM period is reported` in both suites |
| FND-000015 | fixed (before this task) | `rpio_sysfs_read_text()` reads `size - 1` and terminates at the returned count; `empty value read is an error` |
| FND-000016 | fixed here | per-session ownership bits in `rpio_sysfs`; `pre-exported pins stay exported`, `failed connect keeps pre-exported pins`, rpio hardware case on both Pis |
| FND-000017 | fixed (before this task) | the generated drivers own no `relay_mutex`/`port_mutex`; no pthread mutex is created in either driver |
| FND-000018 | not a bug | property allocation uses `indigo_safe_malloc()`, which asserts and exits instead of returning NULL; the NULL branch is generator-emitted and unreachable |
| FND-20261023065605-60d4-repository-audit-011 | fixed here | `relay_pulse_finalizer()` keeps the item ON and publishes ALERT; `failed pulse end is reported` in both suites |
| FND-20261024202957-6bd9-fix-ccd-rpi-001 | stays open | needs an autofocus camera; see verification.md |

New defect found and fixed in this task without a separate record: aux_rpio D16 (RP1 inputs without pin function), recorded in `indigo_linux_drivers/aux_rpio/REFACTOR.md`.
