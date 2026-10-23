# Test and regression plan

Status: executed (results in verification.md)

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-003 | unit `indigo_test/unit/test_primalucelib.c` "golden requests match the original driver" | PTY peer, empty replies | 57 requests byte-identical to the original driver templates (56 templates, MOT_ABORT→MOT_STOP fallback sends two) | Before driver change |
| T-002 | AC-004 | same file: reply paths, diagnostic lines, refusals, garbage, silence | Scripted replies | Values by path; `[` lines skipped; ERROR under cmd/set/get fails with reason; quoted/broken JSON fails without link failure; silence sets link failure | Before driver change |
| T-003 | AC-001 | same file: acknowledgements, stop fallback, settings, stored-position guards, positions/angles, environment, restart, SESTO SENSO 3, GIOTTO, ALTO, models, URLs | Scripted replies in hardware or INDI shape | Each helper's success condition and request bytes | Before driver change |
| T-004 | AC-003 (meta) | mutation: one space removed from one request macro | temporary edit, reverted | golden case fails | Once |
| T-005 | AC-002 | `nm` of `build/lib/libprimalucelib.a`, `build/drivers/indigo_focuser_primaluce.a` and the driver `.so`; rebuild behaviour after touching a library source | built artifacts | archive exports the library API, no jsmn; driver archive only `indigo_focuser_primaluce`; a library change rebuilds the archive and relinks the driver | After driver change |
| T-006 | AC-005 | `grep` for old helpers, path arrays and request literals in the `.driver` | | none left | After driver change |
| T-007 | AC-006 | `integration/test_focuser_primaluce_simulator.c` (54 cases), `test_focuser_primaluce_motion` (1) | original vs library driver, same libindigo | identical pass/fail set | Before and after |
| T-008 | AC-007 | trace comparison (`INDIGO_SIMULATOR_TRACE_DIR`) | 2 original runs, 1 library run with the same libindigo | per case: same distinct requests, same command order, same collapsed sequence | Before and after |
| T-009 | generator semantics | generated `.c` with user blocks stripped, regeneration md5 | | identical; reproducible | After driver change |
| T-010 | FND-000049 regression | unit `test_uni_io.c` "serial_port_reopens_after_close" | PTY, master stays open | reopen succeeds; fails before the fix | Before and after the fix |
| T-011 | full scope after fixes | `python3 tools/run_driver_test.py focuser_primaluce` | final tree | 55/55 recorded | Final |

## Baseline evidence
See verification.md "Baseline".

## Bug reproduction
- FND-000049: T-010 fails on the unfixed library (`assertion failed: reopened`); 10 driver cases fail with EBUSY.
- FND-000050: `status_polling` failed 2 of 3 isolated reruns and in all three full baseline runs.

## Negative cases and permission boundaries
Refusal forms, silence, malformed JSON, stored-position guards (zero position, quote and backslash in names), malformed URLs and too-long host names are covered by T-002/T-003.

## Isolation and cleanup
Every unit case uses its own pseudo-terminal and peer thread. The simulator suite keeps its forked-case fixtures. Traces and private binaries live in the session scratchpad only; `make -C indigo_test test-clean` runs at the end.

## Deferred tests and coverage limitations
- HTTP transport: not testable without sockets in the unit or integration targets. The code moved
  verbatim and keeps its hardware coverage from 3.0.0.22 (no rerun in this task).
- GIOTTO, ALTO and parts of the SESTO SENSO 3 command set: unit-tested against INDI-shaped
  replies only; no hardware or driver yet.
- Windows and macOS builds were not run in this environment (Linux x86_64 only).
