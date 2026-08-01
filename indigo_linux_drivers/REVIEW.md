# indigo_linux_drivers Review

## Status

| Field | Value |
| --- | --- |
| Last reviewed commit | `017ba602857378e4aed489c065c76eacae15924c` |
| Review state | Complete static review for in-scope Linux drivers at this commit. |

## Scope

Linux-specific INDIGO drivers and platform integration code under `indigo_linux_drivers`.

Excluded from this review:

- `externals/` and `bin_externals/` SDK/vendor trees.
- Build products, generated binaries, and simulator-only driver trees.

## Coverage Manifest

| Group | Directories | Source files reviewed |
| --- | --- | --- |
| Linux AUX GPIO drivers | `aux_asiair`, `aux_rpio` | `indigo_aux_asiair.c`, `indigo_aux_asiair.h`, `indigo_aux_asiair_main.c`, `indigo_aux_rpio.c`, `indigo_aux_rpio.h`, `indigo_aux_rpio_main.c` |

## Current Findings

| ID | Severity | File | Summary | Status |
| --- | --- | --- | --- | --- |
| LINUXDRV-001 | Medium | `aux_asiair/indigo_aux_asiair.c:853`, `aux_asiair/indigo_aux_asiair.c:875`, `aux_rpio/indigo_aux_rpio.c:1028`, `aux_rpio/indigo_aux_rpio.c:1050` | PWM change handlers ignore failed `*_pwm_get()` calls, then compute frequency or duty from `period` initialized to zero. If the PWM sysfs node disappears, permission is denied, or a read fails, this can divide by zero or push nonsensical PWM values while still updating the property as successful. Check the read result, set the changed property to `INDIGO_ALERT_STATE`, and return before doing math or writes. | Open |
| LINUXDRV-002 | Medium | `aux_asiair/indigo_aux_asiair.c:282`, `aux_asiair/indigo_aux_asiair.c:298`, `aux_rpio/indigo_aux_rpio.c:319`, `aux_rpio/indigo_aux_rpio.c:335` | PWM sysfs reads parse buffers with `atoi()` without adding a terminating NUL after `read()`. sysfs does not append NUL bytes, so parsing can read stale stack data past the bytes returned. Capture the read length, reject full-buffer reads, append `buf[n] = '\0'`, and use `strtol()` with validation. | Open |
| LINUXDRV-003 | Medium | `aux_asiair/indigo_aux_asiair.c:482`, `aux_asiair/indigo_aux_asiair.c:505`, `aux_rpio/indigo_aux_rpio.c:555`, `aux_rpio/indigo_aux_rpio.c:581` | GPIO/PWM export and unexport helpers return on the first failure without rolling back or continuing cleanup. A partial connect failure can leave some GPIOs/PWMs exported, and a disconnect can stop after one unexport error while leaving the rest of the pins exported. Track successfully exported resources during connect and perform best-effort cleanup of all resources on failure and disconnect. | Open |
| LINUXDRV-004 | Low | `aux_asiair/indigo_aux_asiair.c:694`, `aux_asiair/indigo_aux_asiair.c:941`, `aux_rpio/indigo_aux_rpio.c:859`, `aux_rpio/indigo_aux_rpio.c:1140` | `relay_mutex` and `port_mutex` are initialized but never destroyed before the private data is freed. This is unlikely to affect a single-process normal shutdown, but repeated driver load/unload cycles leak pthread mutex resources and complicate sanitizers. Destroy both mutexes during detach/delete after timers are cancelled and no callbacks can use them. | Open |
| LINUXDRV-005 | Low | `aux_asiair/indigo_aux_asiair.c:696`, `aux_rpio/indigo_aux_rpio.c:861` | If custom property allocation fails after `indigo_aux_attach()` has already succeeded, `aux_attach()` returns `INDIGO_FAILED` without releasing any properties allocated before the failed allocation and without detaching the base AUX context. Add a single cleanup path that releases partially initialized properties, destroys initialized mutexes, and calls `indigo_aux_detach()`. | Open |

## Review Focus

- Linux-only API usage, sysfs GPIO/PWM handling, and build isolation.
- Device discovery, permissions, resource ownership, and cleanup.
- Driver lifecycle consistency with portable INDIGO drivers.
- Property state handling when hardware, kernel interfaces, or permissions are unavailable.
- Timer cancellation and callback/resource lifetime.
- Standalone XML protocol adapter entry points.

## Reviewed Ranges

| From | To | Date | Notes |
| --- | --- | --- | --- |
| Repository start | `017ba602857378e4aed489c065c76eacae15924c` | 2026-08-01 | Reviewed all in-scope Linux driver source/header/main files listed in the coverage manifest. |
