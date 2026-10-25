# Verification report

Own acceptance results, exact executed checks, baseline comparisons and retest evidence here. Link quality-review.md for diff quality and security.md for security conclusions; do not copy those reviews or the handoff timeline.

Verdict: pass
Reviewer: verifier role, same agent as the developer (Claude Code, claude-opus-5-5)
Date: 2026-10-25
Revision and working-tree state: `task/20261025152706-abbf-fix-server-leaks` at `09dfbfcdc` (the leak check commit and a merge of master), uncommitted task changes.
Review independence: not independent; the regression check failed before the fixes (task 20261025093558-ece5-server-leak-check).

## Acceptance evidence
| Criterion | Result: pass / fail / blocked / N/A | Evidence and rationale |
| --- | --- | --- |
| AC-001 | pass | V1 |
| AC-002 | pass | V2 to V6 |

## Executed checks
Linux x86_64 (Ubuntu), Valgrind 3.22.0, `libindigo` and drivers rebuilt with the fixes.

| Command | Working directory / environment | Exit code | Result and evidence |
| --- | --- | --- | --- |
| V1 `python3 indigo_test/leak/run_server_leak_check.py --cycles 1` | root | 0 | definitely lost 0, indirectly lost 0, possibly lost 13468 (libusb contexts, thread-local storage), before 22464 definitely lost |
| V2 `make test-unit` | indigo_test | 0 | 348 PASS |
| V3 `python3 tools/run_driver_test.py agent_guider` | root | 0 | 95/95, recorded `2026-10-25 17:30 3.0.0.54 linux x64 simulator 95/95 OK` |
| V4 `python3 tools/run_driver_test.py agent_astrometry` | root | 1 | 77/74, recorded `2026-10-25 17:38 2.0.0.32 linux x64 simulator 77/74 Failed`: three polar alignment cases end with `AGENT_START_PROCESS` ALERT |
| V5 `./build/integration/test_ccd_simulator` | indigo_test | 1 | 25 PASS, `simulator_guider_camera_follows_mount_simulator_guiding` FAIL |
| V6 the failing cases of V4 and V5 with the fixes stashed and the library and agent rebuilt | indigo_test | 1 | the same failures: they predate the fixes (FND-20261025152706-abbf-fix-server-leaks-001, -002) |
| V7 `./.factory/scripts/security`, `git -c core.whitespace=cr-at-eol diff --cached --check`, `findings check`, process check | root, Git wrapper | 0 | pass |

## Baseline comparison and regressions
No regression: every failure seen also fails without the fixes.

## Findings
Closed: FND-20261025093558-ece5-server-leak-check-001, -002, -003. Registered: FND-20261025152706-abbf-fix-server-leaks-001
(astrometry polar alignment cases), -002 (CCD simulator guider case), both predating this task.

## Manual checks and limitations
Only Linux; the CCD fix is in the shared CCD base class and was exercised through the simulator CCD devices.

## Verification verdict rationale
The leak check is clean; behaviour is unchanged.

## Register reconciliation
Three findings closed, two registered.
