# Verification report

Verdict: pass
Reviewer: coordinator (Claude Code, claude-opus-5-5); independent diff review by a fresh reviewer subagent (quality-review.md)
Date: 2026-11-13
Revision and working-tree state: branch `scheduler` at 3f2a6fb28 with this task's uncommitted changes
Review independence: implementer verified; the diff review was independent.

## Acceptance evidence
| Criterion | Result | Evidence and rationale |
| --- | --- | --- |
| AC-001 | pass | `commands_queued_at_shutdown_are_released` in the sanitizer build: before the fix 6 leaked allocations (200 requests) and 76-426 leaked commands per run (2000 requests, 3 runs; reviewer: 14 of 14 runs failing); after the fix no leak, 3 runs |
| AC-002 | pass | full suite 36/36 (regular and sanitizer), server acceptance 7/7, recorded run 43/43 |

## Executed checks
| Command | Working directory / environment | Exit code | Result and evidence |
| --- | --- | --- | --- |
| `ASAN_OPTIONS=detect_leaks=1 ./indigo_test/build/integration/test_agent_scheduler_asan commands_queued` before the fix | repository root, Linux arm64 | 1 | leaks from `agent_change_property` |
| the same after the fix (3 times) | repository root | 0 | 1/1 each |
| current test compiled against the unfixed agent (3 times) | scratchpad, removed afterwards | 1 | 152, 588 and 852 leaked allocations |
| `ASAN_OPTIONS=detect_leaks=1 ./indigo_test/build/integration/test_agent_scheduler_asan` | repository root | 0 | 36/36 |
| per-case leak check over the whole suite (experiment, reverted) | repository root | 1 | only `repeat_retry_and_failures`: 1183 bytes in the indigo_libs thread-local escape buffer of `indigo_xml_escape_b()`; recorded as FND-20261113001457-32cc-scheduler-shutdown-command-leak-001 |
| `python3 tools/run_driver_test.py agent_scheduler` | repository root | 0 | 43/43 OK (36 integration + 7 server), recorded |
| `./.factory/scripts/findings check` | repository root | 0 | pass |

## Baseline comparison and regressions
Baseline 42/42; one case added, no expectation changed.

## Found defects
| Defect | Impact | Cause | Fix | Regression test | Found by |
| --- | --- | --- | --- | --- | --- |
| Requests queued at shutdown leaked | memory lost on every unload with pending requests | cancelled handlers do not free their data | list of queued commands released by `stop_commands()` | commands_queued_at_shutdown_are_released | FND-…-core-004 |
| Sanitizer runs of forked case runners never check leaks | leaks unnoticed | children end with `_exit()` | explicit check in the new case; general fix tracked | — | this task |

## Manual checks and limitations
Hardware-test decision: none. The leak check runs only in the new case, and only where LeakSanitizer is available (not with Apple clang). The branch for requests arriving during the shutdown itself is covered by review only.

## Verification verdict rationale
All acceptance criteria pass.

## Final test summary
- Simulated tests: scheduler 43 run / 43 passed (36 integration, 7 server acceptance); sanitizer build 36/36.
- Hardware tests: 0 run, 0 passed.

## Register reconciliation
FND-20261112184305-e0a6-scheduler-agent-core-004 fixed and archived; FND-20261113001457-32cc-scheduler-shutdown-command-leak-001 opened.
