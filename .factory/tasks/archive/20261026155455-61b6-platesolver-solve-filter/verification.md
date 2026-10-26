# Verification report

Own acceptance results, exact executed checks, baseline comparisons and retest evidence here. Link quality-review.md for diff quality and security.md for security conclusions; do not copy those reviews or the handoff timeline.

Verdict: pass
Reviewer: verifier role, same agent as the developer (Claude Code, claude-opus-5-5)
Date: 2026-10-26
Revision and working-tree state: `task/20261026155455-61b6-platesolver-solve-filter` at `24319f3e6` (= master), uncommitted task changes (simplified version after the requester's review).
Review independence: not independent; separate pass, plus a mutation check of the new cases.

## Acceptance evidence
| Criterion | Result: pass / fail / blocked / N/A | Evidence and rationale |
| --- | --- | --- |
| AC-001 | pass | V2, V3, V6 |
| AC-002 | pass | V2, V3, V6 |
| AC-003 | pass | V2, V3 |
| AC-004 | pass | V2, V3, V6 |
| AC-005 | pass | V2, V3 |
| AC-006 | pass | V3, V4, V5, V7 |

## Executed checks
Linux x86_64 (Ubuntu), `libindigo` (`make -C indigo_libs libindigo`) and the agents (`make -C indigo_drivers/<agent> -f ../../Makefile.drv all`) built without new warnings.

| Command | Working directory / environment | Exit code | Result and evidence |
| --- | --- | --- | --- |
| V1 `gcc -Wall -Wextra ... -c indigo_platesolver.c` | indigo_libs | 0 | only the usual `INDIGO_COPY_*` truncation and function-cast warnings of the existing pattern |
| V2 `build/integration/test_agent_astrometry filter`, `"polar alignment switches"`, `abort`, each 3 times (simplified version) | indigo_test | 0 | all PASS every time (10/10, 1/1, 9/9); the first simplified version hung in `abort during filter change` (blocking return inside a bus callback), fixed by deferring the return to the snoop |
| V2m mutation: `restore_filter()` removed from `process_failed()` and `solve()` | indigo_test | 1 | 8 of 10 new cases fail; restored, 10/10 |
| V3 `python3 tools/run_driver_test.py agent_astrometry` | root | 0 | 87/87, recorded `2026-10-26 20:06 2.0.0.33 linux x64 simulator 87/87 OK` |
| V4 `make test-agent-imager` | indigo_test | 0 | 39/39 (uses the platesolver library with a fake solver device) |
| V5 `make test-agent-astrometry-solver` (real solve-field, CCD simulator, real Imager/Mount agents) | indigo_test | 0 | 33/33 (first version); the same 33 cases are part of V3 for the simplified version |
| V6 end to end: `indigo_server` with `indigo_ccd_simulator`, `indigo_agent_imager`, `indigo_agent_astrometry`, private HOME, `indigo_prop_tool` (scratch script) | root | 0 | list `CURRENT, 1..5` (labels `Filter #1..#5`); with slot 3: wheel 1 → 3 (OK) before `AGENT_IMAGER_CAPTURE` BUSY, image OK, wheel BUSY to 1 while solving, OK on 1; process ALERT because the private HOME has no index (failure path). Abort 5 s into a 6 s exposure: capture ALERT, wheel back to 1 OK |
| V7 `valgrind --trace-children=yes --leak-check=full` on `"solve with filter"`, `"abort during filter change"`, `"filter list follows"` | indigo_test | — | no loss from the new code; 120 bytes definitely lost per solver command in `execute_command()` (`indigo_agent_astrometry.c:435`), pre-existing → FND-20261026155455-61b6-platesolver-solve-filter-001 |
| V8 `./.factory/scripts/security`, `git -c core.whitespace=cr-at-eol diff --check HEAD`, `findings check`, process check | root, Git wrapper | 0 | pass |

## Baseline comparison and regressions
77/77 before (2026-10-25), 87/87 now (10 new cases); imager and real-solver suites unchanged.

## Findings
FND-20261026155455-61b6-platesolver-solve-filter-001 (low, open): `execute_command()` leaks the last `getline()` buffer.

## Manual checks and limitations
Only Linux. `agent_solver` has no automated suite (`run_driver_test.py`: no tests named test_agent_solver); it runs the same library code. The server leak check (`test-server-leaks`) loads no agents and was not rerun; V7 covers the new code. No physical wheel.

## Verification verdict rationale
All acceptance criteria are covered by passing cases that fail without the change, and by the end-to-end run.

## Register reconciliation
One finding opened (out of scope, low).
