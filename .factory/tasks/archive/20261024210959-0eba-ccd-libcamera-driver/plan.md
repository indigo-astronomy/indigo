# Development plan

Status: active

## Approach and affected components
New generated CCD driver `indigo_linux_drivers/ccd_libcamera` with a C++ libcamera bridge behind a C interface; owner-approved generator extension `sdk { usb = false; discover {} }` in `indigo_tools/indigo_generator.c`; fake-bridge integration suite and hardware suite in `indigo_test/`; documentation (`indigo_docs/DRIVER_GENERATOR_MIGRATION.md`, `indigo_docs/PROPERTIES.md`), `MIGRATION_STATUS.md`, root `Makefile` driver list, Xcode project. Detailed driver record: `indigo_linux_drivers/ccd_libcamera/REFACTOR.md`.

## Baseline
- Commands and environment: macOS arm64 (fake bridge, generator); Raspberry Pi 3 B+, Debian 13, libcamera 0.7.2 (hardware).
- Existing failures: none relevant; `rpicam-hello` on the RPi fails with an ABI symbol error (rpicam-apps 1.13 vs libcamera 0.7.2), outside INDIGO.
- Repository revision / working-tree state: base `d25dcabf2` (origin/master), task branch `task/20261024210959-0eba-ccd-libcamera-driver`.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Generator extension, generator test | AC-007 | owner approval | 120/120 identical regeneration; test_generator_architecture | done |
| 2 | Bridge, `.driver`, generated outputs, `Makefile.inc` | AC-001..006, AC-008 | 1 | builds on macOS (fake) and RPi | in progress |
| 3 | Fake-bridge suite + sanitizer variant | AC-001..006 | 2 | 15/15 pass | done |
| 4 | Hardware suite, recorded run | AC-001..005 | 2 | `tools/run_driver_test.py ccd_libcamera --hw` | pending |
| 5 | Docs, status files, Xcode | all | 2 | review | done |
| 6 | Quality review, task archive, single commit | all | 1-5 | process checks | pending |

## Test strategy
Fake bridge replaces the C bridge (no libcamera needed); hardware suite on the RPi through the real bridge. Generator change verified by full regeneration comparison and the generator architecture test.

## Security risks and mitigations
Frame geometry and sizes from libcamera are validated against the mapped plane and destination buffer before copying. No network or credentials.

## Dependencies and architecture decisions
libcamera (optional at build time, pkg-config). The bridge isolates C++ from the generated C driver (owner discussion 2026-10-24, option "extern C shim").

## Migration, rollout, and rollback
New driver, listed in `UNTESTED_DRIVERS`; removal = delete the directory and list entry. Generator change is opt-in and leaves existing output identical.

## Deviations and owner decisions
Generator change approved 2026-10-24. The approved diff was extended by `exit(1)` on invalid `usb = false` combinations, because block parsers cannot report a fatal error otherwise.
