# Test and regression plan

Status: complete

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001 | unit `indigo_test/unit/test_agent_scheduler_header.c` | literal task texts | valid headers parsed, every invalid class rejected with line | before integration |
| T-002 | AC-002 | unit `indigo_test/unit/test_agent_scheduler_sky.c` | fixed dates 2026-03 and 2026-11, site 48.15 N 17.11 E | known Sun/Moon events and constraint results | before integration |
| T-003 | AC-003..AC-007 | integration `indigo_test/integration/test_agent_scheduler.c` (22 cases) | real Scripting Agent and Sequencer.js, stand-in Mount and Guider agents, private HOME per case | see verification.md acceptance table | after implementation |
| T-004 | memory safety | `test_agent_scheduler_asan` | ASan + UBSan, leak detection | no report | after fixes |
| T-005 | preserved Scripting Agent behaviour | `test_agent_scripting` | unchanged | same result as baseline | baseline and final |

## Baseline evidence
See verification.md "Baseline".

## Bug reproduction
Defects found during the task and their regression tests: verification.md "Found defects".

## Negative cases and permission boundaries
Invalid headers, duplicate names, unknown task commands, deletes of unknown tasks, script errors, missing `start()`, stop timeout.

## Isolation and cleanup
Every integration case runs in its own process with a private HOME (`parallel_case_runner.h`); `make -C indigo_test test-clean` afterwards.

## Deferred tests and coverage limitations
- Loss of the Scripting Agent during a run: source audit only.
- DELETE during the short STARTING phase: covered indirectly by the stop logic tests.
- 128-task night-plan performance test: phase 3 (with the plan preview), FND-20261112184305-e0a6-scheduler-agent-core-003.
- Windows and macOS builds: not available on this host.
