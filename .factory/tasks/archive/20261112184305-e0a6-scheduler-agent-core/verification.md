# Verification report

Verdict: pass
Reviewer: coordinator (Claude Code, claude-opus-5-5); independent review of the diff by a fresh reviewer subagent, see quality-review.md
Date: 2026-11-12
Revision and working-tree state: branch `scheduler` at cbf8e03bf with the uncommitted changes of this task
Review independence: the coordinator implemented and verified; the diff review was independent.

## Baseline
| Command | Working directory / environment | Exit code | Result |
| --- | --- | --- | --- |
| `make -C indigo_test build/integration/test_agent_scripting && ./indigo_test/build/integration/test_agent_scripting` | repository root, Linux arm64, Debian, gcc | 1 | 36/37; pre-existing failure `change bindings forward local access tokens`, tracked as FND-20261105112837-9c3a-sequencer-simulator-suite-004 |
| `make -C indigo_test test-unit` | repository root | 2 | pre-existing failure in `test_protocol_security` (`device ACL requires device or nonzero master token`); reproduced with the test built against the committed `indigo_names.h`, so not caused by this task; recorded as FND-20261112184305-e0a6-scheduler-agent-core-001 |

## Acceptance evidence
| Criterion | Result | Evidence and rationale |
| --- | --- | --- |
| AC-001 | pass | `test_agent_scheduler_header` 9/9: every key, defaults, case, whitespace, CRLF, BOM, header end, decimal and sexagesimal targets, windows, every rejection class including hex, exponent, infinity and fractional leading sexagesimal parts, size limit |
| AC-002 | pass | `test_agent_scheduler_sky` 13/13: Sun at the 2026 March equinox, full and new moon of March 2026, Lorentzian values, Polaris altitude/airmass/hour angle, wrapped HA range, target below horizon, twilight, moon separation and illumination, moon below horizon ignored, absolute and nightly windows, next eligible time, time to set, unknown site |
| AC-003 | pass | integration `store_edit_and_delete`, `commands_and_order`, `pending_rename_reserves_the_name` |
| AC-004 | pass | integration `persistence_across_restart` (states, disabled flag, order, settings, flush on shutdown without delay), `running_task_restored_without_using_a_retry` (RUNNING restored as queued with retries intact, unstored urgent task dropped) |
| AC-005 | pass | integration `run_order_by_priority` (class/level order, a task uploaded during a run does not interrupt it), `window_wait_and_expiry`, `target_below_horizon_waits`, `repeat_retry_and_failures` |
| AC-006 | pass | integration `urgent_preempts_and_task_is_retried` (sequence and guider aborted, retry not consumed, rerun), `preempted_task_fails_when_asked`, `urgent_waits_for_eligibility`, `second_urgent_waits_for_the_first`, `urgent_submitted_while_stopped`, `hand_started_sequence_blocks_all_but_urgent`, `stop_request_blocks_preemption` |
| AC-007 | pass | integration `repeat_retry_and_failures` (thrown error, syntax error, missing `start()`), `failure_after_start_aborts_the_sequence`, `stop_lets_the_task_finish`, `abort_and_delete_running_task`, `edit_of_running_task_is_deferred`, `stop_timeout_stops_the_scheduler` |
| AC-008 | pass | agent built with `Makefile.drv`; server syntax-checked with and without `STATIC_DRIVERS`; Windows `.vcxproj`/`.filters`/`.user`, solution and server reference added; Xcode group, both library targets, `Sequencer.js.dat` phase and test files added (balanced braces/parentheses); names in `indigo_names.h`; `PROPERTIES.md` section |

## Executed checks
| Command | Working directory / environment | Exit code | Result and evidence |
| --- | --- | --- | --- |
| `make -C indigo_drivers/agent_scheduler -f ../../Makefile.drv all` | repository root, Linux arm64 | 0 | no warnings |
| `gcc -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -fsyntax-only` on the three agent sources | `indigo_drivers/agent_scheduler` | 0 | no warnings |
| `./indigo_test/build/unit/test_agent_scheduler_header` | repository root | 0 | 9/9 |
| `./indigo_test/build/unit/test_agent_scheduler_sky` | repository root | 0 | 13/13 |
| `./indigo_test/build/integration/test_agent_scheduler` (three consecutive runs) | repository root | 0 | 22/22 each |
| `ASAN_OPTIONS=detect_leaks=1 ./indigo_test/build/integration/test_agent_scheduler_asan` | repository root, `-fsanitize=address,undefined` | 0 | 22/22, no sanitizer report |
| header and sky unit tests built with `-fsanitize=address,undefined` | scratch build | 0 | all passed, no report |
| `python3 tools/run_driver_test.py agent_scheduler` | repository root | 0 | final recorded run, see below |
| unit binaries after `test_protocol_security` run individually | `indigo_test` | 0 | all passed |
| `./.factory/scripts/findings check` | repository root | 0 | see handoff |
| sky grid benchmark (scratch program) | Linux arm64 | 0 | 48 h grid 0.088 s, 128 targets 0.016 s |

## Baseline comparison and regressions
No existing test changed result. The two pre-existing failures are unrelated to the scheduler.

## Found defects
| Defect | Impact | Cause | Fix | Regression test | Found by |
| --- | --- | --- | --- | --- | --- |
| Request results invisible | uploads and commands never acknowledged; config restore would time out per task | INDIGO drops updates that change neither values nor state | requests acknowledged Busy, completed Ok/Alert by the handler | every integration case | reproduced |
| Empty list stopped the scheduler | urgent tasks could not run on a scheduler without tasks | "all finished" was true for zero tasks | stop only with at least one task | `second_urgent_waits_for_the_first`, `hand_started_sequence_blocks_all_but_urgent` | reproduced |
| Grid build 0.8 s on the handler queue | handler queue blocked | Moon theory evaluated at 577 points | hourly Sun/Moon samples, linear interpolation | sky unit tests; benchmark 0.088 s | measured |
| Use-after-free between enumeration/acknowledgement and task edits | crash or garbage under concurrent clients | properties touched on the caller thread while the handler replaced long text values | only the handler changes properties; clients enumerate copies under a property mutex | ASan run | review |
| Pending save lost at shutdown | finished work repeated after restart | delayed save not flushed in detach | flush in detach | `persistence_across_restart` without delay | review |
| Body failing after `start()` left its sequence running | scheduler blocked, task could run twice | error result finished the task without stopping the sequence | stop through `STOP_FAILURE` | `failure_after_start_aborts_the_sequence` | review |
| Stop during start could use a stale idle state | next task counted as started and failed | Busy update still queued in the Scripting Agent | stop waits for the start or the result; no new start in the pass that ended the stop | `abort_and_delete_running_task`, `urgent_preempts_and_task_is_retried` | review |
| Handlers used the client during shutdown | use-after-free on unload | client freed before handlers stopped | handlers stopped first | lifecycle of every case, ASan | review |
| Pending rename allowed duplicate names | two tasks with one name | name check ignored pending edits | `name_taken()` checks pending names | `pending_rename_reserves_the_name` | review |
| STOP did not block preemption | running task aborted, urgent task never run | missing check | preemption skipped after STOP | `stop_request_blocks_preemption` | review |
| Running task never failed when the Scripting Agent vanished | scheduler stuck | presence ignored while running | task fails | source audit only (no test unloads the Scripting Agent mid-run) | review |
| Parser accepted hex, exponents, infinity, fractional leading sexagesimal parts | unexpected values | `strtod` leniency | decimal-only check | `rejected_headers` | review |

## Manual checks and limitations
- Hardware-test decision: no hardware testing; all tests are hardware-free. No hardware validation is claimed.
- Windows and macOS were not built here (Linux arm64 only); project files were added by pattern.
- The Scripting Agent losing the library mid-run is covered by source audit only.
- Remaining nonblocking findings: FND-20261112184305-e0a6-scheduler-agent-core-001 to -005.

## Verification verdict rationale
All acceptance criteria pass; the defects found during the task are fixed with regression coverage except the one noted as source audit.

## Final test summary
- Simulated (hardware-free) tests: 44 run, 44 passed (22 integration in the final recorded run, 9 + 13 unit cases).
- Hardware tests: 0 run, 0 passed.

## Register reconciliation
New: FND-20261112184305-e0a6-scheduler-agent-core-001 to -005 (see handoff.md). Referenced existing: FND-20261105112837-9c3a-sequencer-simulator-suite-004.
