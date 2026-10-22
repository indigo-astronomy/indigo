# Development plan

Status: executed 2026-10-22

## Approach and affected components
Characterization first, then one atomic move: the library reproduces the driver's existing
protocol code verbatim behind named functions, and the driver switches to it in one step.
The library is a static library on every platform (owner decision 2026-10-22, replacing the first
design that `#include`d the source with internal linkage). Make: `indigo_drivers/shared/primalucelib/Makefile`
builds `build/lib/libprimalucelib.a`, which the driver links through `SDK_LIB`/`MAKE_SDK` in its
`Makefile.inc`. Visual Studio: `primalucelib.vcxproj`, referenced by the driver project. Xcode: the
library source compiled once into the `indigo`/`indigo_m1` targets that hold all drivers.

Allowed paths: `indigo_drivers/shared/primalucelib/`, `indigo_drivers/focuser_primaluce/`
(`.driver`, regenerated `.c/.h/_main.c`, `.vcxproj*`, `REFACTOR.md`, recorded `README.md`
Testing line only), `indigo_test/unit/test_primalucelib.c`,
`indigo_test/integration/test_focuser_primaluce_simulator.c` (trace capture only),
`indigo_test/Makefile`, `indigo.xcodeproj/project.pbxproj`, `TEST_SUMMARY.md` (generated),
`MIGRATION_STATUS.md` (status columns only, if counts change), `.factory/` task, state and
findings records.

## Baseline
- Commands and environment: Linux x86_64, gcc 13.3.0; `make -C indigo_drivers/focuser_primaluce -f ../../Makefile.drv all`; `make -C indigo_test build/integration/test_focuser_primaluce_simulator build/integration/test_focuser_primaluce_motion`; both binaries run from `indigo_test/`.
- Existing failures: 10 of 54 simulator cases and the 1 motion case fail. 9 + 1 fail because a
  Linux PTY slave keeps `TIOCEXCL` after close, so every reconnect gets EBUSY. `status_polling` is
  timing-flaky (2 of 3 reruns failed). Details in verification.md.
- Repository revision / working-tree state: `80d073bcb` on `task/008-primalucelib`; unrelated
  untracked files of the owner are preserved and never staged.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | `REFACTOR.md`: audit, hardware decision, plan | AC-006 | None | Record written before code changes | done |
| 2 | Trace capture in `test_focuser_primaluce_simulator.c` | AC-007 | 1 | Suite result unchanged; traces written | done |
| 3 | Baseline traces of the original driver (2 runs to measure nondeterminism) | AC-007 | 2 | Traces stored in scratchpad | done |
| 4 | Library `primalucelib.{h,c}`, `jsmn.h` moved, `PROTOCOL.md` | AC-001, AC-002, AC-008 | 1 | Compiles standalone | done |
| 5 | Unit test `test_primalucelib` incl. golden requests | AC-003, AC-004 | 4 | Unit test passes | done |
| 6 | Driver refactor + regeneration | AC-005 | 4, 5 | Driver builds; generated diff reviewed | done |
| 7 | Post-change suites + trace comparison | AC-006, AC-007 | 3, 6 | Same pass/fail set; traces equal | done |
| 8 | Project registration (Xcode, vcxproj, Makefile) | AC-008 | 4, 5 | Files referenced; plist parses | done |
| 9 | Recorded run via `tools/run_driver_test.py focuser_primaluce` | AC-006 | 7 | README/TEST_SUMMARY updated | done |
| 10 | Reviews, findings, archival, single commit | all | 9 | process check passes | done |

## Test strategy
Unit (`indigo_test/unit/test_primalucelib.c`): library over a pseudo-terminal with a scripted
peer thread. Golden request bytes, reply parsing, error forms, framing, link failure, model
detection, URL parsing and every device helper group. Integration: the existing 54 + 1
simulator cases unchanged, plus trace comparison. No hardware run (owner decision).

## Security risks and mitigations
Untrusted replies: the parsing code is moved unchanged (bounded buffers, token limit). Moving it
must not drop the `"`/`\` name guard or the URL encoding; the unit tests cover both.

## Dependencies and architecture decisions
No new dependency. jsmn (MIT, already vendored) moves with the library and is compiled into it with `JSMN_STATIC`, so only the library's own `primalucelib_`/`PRIMALUCE_` symbols are exported. Drivers are loaded without `RTLD_GLOBAL`, so each driver library resolves to its own copy.

## Migration, rollout, and rollback
Single commit; rollback is reverting it. No configuration or property migration.

## Deviations and owner decisions
- 2026-10-22, owner: fix the pre-existing Linux failures (FND-000049 libindigo `TIOCNXCL`, FND-000050 and FND-000051 test races) inside this task as separate commits. Adds `indigo_libs/indigo_uni_io.c` and `indigo_test/unit/test_uni_io.c` to the allowed paths.
- `handshake_timeout` was first attributed to FND-000049; the libindigo fix left it failing, which identified FND-000051.
