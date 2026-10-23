# Verification report

Own acceptance results, exact executed checks, baseline comparisons and retest evidence here. Link quality-review.md for diff quality and security.md for security conclusions; do not copy those reviews or the handoff timeline.

Verdict: pass
Reviewer: verifier role, same agent as the developer (Claude Code, claude-opus-5-5)
Date: 2026-10-22
Revision and working-tree state: base `80d073bcb` on `task/008-primalucelib`, uncommitted task changes as listed in handoff.md; the owner's unrelated untracked files are untouched.
Review independence: not independent. The same agent implemented and verified. Mitigations: golden requests come from the original source, not from the new code; a mutation check; byte comparison of wire traces recorded with the original driver.

## Acceptance evidence
| Criterion | Result: pass / fail / blocked / N/A | Evidence and rationale |
| --- | --- | --- |
| AC-001 | pass | `primalucelib.h`: key/value constants, 57 request strings, 65 reply paths, 93 functions for controller/focuser/ARCO/GIOTTO/ALTO. Unit cases cover every group (C5). |
| AC-002 | pass | No `PRIVATE_DATA`, `DRIVER_NAME` or property code (the log name is a parameter). Static library: `build/lib/libprimalucelib.a` exports the 93 `primalucelib_` functions and 65 `PRIMALUCE_REPLY_` tables, no jsmn symbol (S2). The driver archive exports only `indigo_focuser_primaluce`; the driver `.so` and executable link the archive (S1, S2). Unit test links the archive (S6). Strict `-Wall -Wextra` compile of the library is clean, header compiles as C++ (S5). |
| AC-003 | pass | The golden case sends 57 requests (56 templates) through the library and compares them with the original driver templates expanded by `snprintf`: all equal. Mutation: removing one space from one macro fails the case (C6). |
| AC-004 | pass | Unit cases for path lookup, `[` lines, CR/LF, ERROR under cmd/set/get with reason, quoted and broken replies (answered, not failed link), silence (link failed and recovered), acknowledgements and stop fallback (C5). |
| AC-005 | pass | The `.driver` contains no `primaluce_command`, `get_string`, `get_number`, `getToken`, path arrays, request literals or jsmn (C10). 2309 → 1895 lines. |
| AC-006 | pass | The original driver and the library driver, with the same libindigo: identical failure set (the same 10 cases, see Baseline). With all fixes: 55/55 (C14). |
| AC-007 | pass | 54 per-case traces of the library driver are byte-identical (`diff -r`) to both reference runs of the original driver: 865 requests, every case equal at all three comparison levels (C12, C13). |
| AC-008 | pass | `indigo.xcodeproj`: `shared/primalucelib` group with the library files, `Makefile` and `primalucelib.vcxproj`, the moved `jsmn.h`, the unit test, and `primalucelib.c` in the Sources of the `indigo` and `indigo_m1` targets; brace/paren balance checked (`plutil` unavailable). `primalucelib.vcxproj` (StaticLibrary, x64/ARM64, Debug/Release) in `indigo_windows.sln` with 8 configuration mappings; the driver `.vcxproj` has a `ProjectReference` to it; all parse as XML, CRLF/BOM kept. `indigo_test/Makefile`: the archive in the unit, simulator, motion and hardware rules. `jsmn.h` is a pure rename. Library headers: Copyright (c) 2026 Rumen G. Bogdanovski, history line crediting the original protocol code by Peter Polakovic. |

## Executed checks
All commands run on Linux x86_64 (Ubuntu, gcc 13.3.0, Python 3), from the repository root unless noted.

| Command | Working directory / environment | Exit code | Result and evidence |
| --- | --- | --- | --- |
| C1 `make -C indigo_drivers/focuser_primaluce -f ../../Makefile.drv all` (original) | root | 0 | baseline build; one pre-existing `-Wformat-truncation` warning on the "Move refused" message |
| C2 `./build/integration/test_focuser_primaluce_simulator` and `test_focuser_primaluce_motion` (original) | indigo_test | 1 / 1 | 44/54 and 0/1; failures listed under Baseline |
| C3 `INDIGO_SIMULATOR_TRACE_DIR=… <private copy of the original suite>` ×2 | indigo_test | 1, 1 | same 10 failures each time; traces identical to each other at all levels (deterministic) |
| C4 `make build/unit/test_primalucelib TEST_CFLAGS='… -Wall -Wextra …'` | indigo_test | 0 | no library warnings (only unused `test_runner.h` helpers) |
| C5 `./build/unit/test_primalucelib` | indigo_test | 0 | 19/19 PASS, 7.4 s |
| C6 mutation of `PRIMALUCE_REQUEST_GET_ENVIRONMENT`, rebuild, run, restore | indigo_test | — | golden case FAIL with the exact differing request; 19/19 again after restore |
| C7 driver build with the library | root | 0 | same single pre-existing warning (the original source compiled in isolation shows it too) |
| C8 generated `.c` with `//+`…`//-` user blocks stripped, old vs new; `indigo_generator` rerun + md5 | root | 0 | generator-owned code identical; `.h`/`_main.c` unchanged; regeneration reproducible |
| C9 `nm -g --defined-only build/drivers/indigo_focuser_primaluce.a` | root | 0 | only `indigo_focuser_primaluce` |
| C10 `grep` for old helpers/literals in the `.driver` | root | 1 (no match) | none left |
| C11 `INDIGO_SIMULATOR_TRACE_DIR=… <private copy of the library suite>` (original libindigo) | indigo_test | 1 | same 10 failures as C2/C3 |
| C12 `python3 -I tools/compare_traces.py before_1 after_1` (session tool) | scratchpad | 0 | 54/54 cases equal: distinct, command order, collapsed sequence |
| C13 `diff -r trace_before_1 trace_after_1`, `diff -r trace_before_2 trace_after_1` | scratchpad | 0, 0 | byte-identical |
| C14 `python3 tools/run_driver_test.py focuser_primaluce` | root | 0 | 55/55 OK, recorded `2026-10-22 16:44 3.0.0.26 linux x64 simulator 55/55 OK`; TEST_SUMMARY regenerated |
| C15 `make -C indigo_test test-unit` (with the libindigo fix) | root | 0 | 17 suites pass, including `indigo_uni_io` and the library suite |
| C16 `./build/unit/test_uni_io` before / after the libindigo fix | indigo_test | 1 / 0 | `serial_port_reopens_after_close` fails (`reopened`), then passes |
| C17 `PRIMALUCE_TEST_FILTER=status_polling` ×3 and `=handshake_timeout` ×3 after the test fixes | indigo_test | 0 ×6 | all PASS |
| C18 `./.factory/scripts/security` | root, Git wrapper (see handoff) | 0 | Factory security boundaries valid |
| C19 `./.factory/scripts/check` | root, Git wrapper | 1 | `git diff --check` flags only the 5 added CRLF lines of `indigo_focuser_primaluce.vcxproj` (the file and 96 other driver projects are CRLF). `git -c core.whitespace=cr-at-eol diff --check` and `--cached --check` exit 0. Disposition: line-ending convention, not whitespace. |

| C20 `./.factory/scripts/test` | root; with and without the Git wrapper | 1 / 1 | Not passable on this host. Without the wrapper, Git 2.43 rejects `--no-lazy-fetch`: 38 failures, 54 errors. With it, 140/144 pass; the 4 failures are 3 tests that prove the lazy-fetch protection (which the wrapper removes) and 1 that sees Python cache files in its own temporary repository. This task changes no Factory script. |
| C21 `python3 .factory/scripts/findings.py check`, `decision_index.py --check`, `validate_project.py` | root, Git wrapper | 0, 0, 0 | findings valid, decision index current, project boundaries valid |

Static library round (2026-10-22, owner decision; same host):

| Command | Working directory / environment | Exit code | Result and evidence |
| --- | --- | --- | --- |
| S1 `make -C indigo_drivers/focuser_primaluce -f ../../Makefile.drv all` after removing `build/lib/libprimalucelib.a` | root | 0 | library built through `MAKE_SDK`, driver `.so` and executable link it; a second `make` rebuilds nothing; touching `primalucelib.c` rebuilds the archive and relinks `.so` and executable only |
| S2 `nm` of `libprimalucelib.a`, `indigo_focuser_primaluce.a`, `indigo_focuser_primaluce.so` | root | 0 | 93 `T primalucelib_`, `D PRIMALUCE_REPLY_*`, no `jsmn_`; driver archive only `indigo_focuser_primaluce`; drivers are loaded without `RTLD_GLOBAL` (`indigo_driver.c`), so copies in two driver libraries do not interpose |
| S3 two concurrent `make all` in `indigo_drivers/shared/primalucelib` after a touch | library dir | 0 | one valid archive (member `primalucelib.o`, 93 functions), no temporary directories left |
| S4 `primaluce` regenerated by the driver build | root | 0 | only the include block of the generated `.c` changed (no `primalucelib.c` include) |
| S5 `gcc -std=gnu11 -Wall -Wextra -c primalucelib.c`; `g++ -std=gnu++11 -Wall` on a file including the header | library dir | 0, 0 | clean (C++ only reports the ignored `#pragma mark` lines) |
| S6 `./build/unit/test_primalucelib` linked against the archive | indigo_test | 0 | 19/19 |
| S7 `make -C indigo_test test-unit` | root | 0 | 17 suites pass |
| S8 simulator suite with `INDIGO_SIMULATOR_TRACE_DIR` | indigo_test | 0 | 54/54; of the 44 cases that passed with the original driver, 43 traces byte-identical to the reference; `unsupported_device` has one extra `MODNAME` because its second connect is no longer refused with EBUSY (FND-000049 fix) |
| S9 `python3 tools/run_driver_test.py focuser_primaluce` (fresh library, final tree) | root | 0 | 55/55 OK, recorded `2026-10-22 23:18 3.0.0.26 linux x64 simulator 55/55 OK` |

## Baseline comparison and regressions
Original driver at `80d073bcb`, Linux x86_64 (C2, C3): 44 of 54 simulator cases pass, and the motion
case fails. Earlier records were all made on macOS or with hardware.

- `repeated_init_shutdown`, `reconnect`, `disconnect_during_motion`, `controller_settings`,
  `led_middle`, `stored_positions`, `wifi_station_mode`, `rotator_lifecycle` and the motion case:
  `Failed to open /dev/pts/N (Device or resource busy)` at the second open (FND-000049).
- `handshake_timeout`: `!context.connected` at line 605 (FND-000051; initially misread as
  FND-000049, corrected after the libindigo fix left it failing).
- `status_polling`: no ALERT within 15 s (FND-000050).

The library driver with the same libindigo reproduces exactly this set (C11), with identical traffic
(C12, C13). After the three fixes the full scope passes (C14). No regression was found.

## Findings
- **FND-000049, fixed:** Linux PTY reopen `EBUSY` in `indigo_uni_io`. Reproduction: C16 and a
  standalone `TIOCEXCL` program (EBUSY without `TIOCNXCL`, ok with it). Retest: C14, C15, C16.
- **FND-000050, fixed:** `status_polling` window. Retest: C17, C14.
- **FND-000051, fixed:** `handshake_timeout` race. Retest: C17, C14.
- **FND-000052, open, nonblocking:** unescaped WiFi credentials, found by source audit; behaviour
  preserved by this task.

Ownership and status are kept in `.factory/findings/`.

## Manual checks and limitations
- **Platforms:** only Linux x86_64. Windows (`.vcxproj` registration, MSVC build) and macOS (Xcode
  project) were not built. `plutil` was not available to lint the Xcode project.
- **HTTP transport:** moved verbatim but not executed; no socket tests in these targets and no
  hardware run (owner chose a simulator run).
- **No hardware run:** hardware evidence remains the earlier records.
- **GIOTTO, ALTO and SESTO SENSO 3 extras:** checked only against INDI-derived reply shapes.
- **Shared library change:** the libindigo fix affects every serial driver. Only the unit suite and
  this driver's suites were run; other drivers' simulator suites were not rerun.

## Verification verdict rationale
Every acceptance criterion has direct evidence. The strongest is the byte-identical wire traffic
across all 54 simulator cases against two reference runs of the original driver, plus golden
request bytes that are proven sensitive by mutation. Remaining limits are platform coverage and the
unexercised HTTP path, both recorded above.

## Register reconciliation
FND-000049, FND-000050 and FND-000051 are archived as fixed, with this report as evidence.
FND-000052 is active (open, owner coordinator, nonblocking). `next_id` is 53.
