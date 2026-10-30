# Development plan

Status: complete

## Approach and affected components
Replace the `indigo_send_message` call with `INDIGO_DRIVER_DEBUG`; bump the agent version. In the agent integration
test, install `indigo_log_message_handler` and raise the log level to debug only around the two search-order solves,
appending lines that contain "Solving with index" to the message buffer marked "log:", in order with the client
messages; the order assertions are unchanged, and no client message from the agent may contain the line.

## Baseline
- Commands and environment: see verification.md, Baseline
- Existing failures: see verification.md
- Repository revision / working-tree state: master `a5d9ebd4e`, task branch created from it; unrelated untracked files in the checkout are not touched

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Baseline: build agent, run `make -C indigo_test test-agent-solver` | — | None | result recorded | done: 21/21 (verification.md C1) |
| 2 | Test: debug log capture, order assertions on it, client messages without the line (`indigo_test/integration/test_agent_solver.c`) | AC-001, AC-002 | 1 | the two tests fail before step 3 (client messages still carry the line) | done: 19/21, both fail as expected (C2) |
| 3 | Agent: `INDIGO_DRIVER_DEBUG`, `DRIVER_VERSION` 0x03000019 (`indigo_drivers/agent_solver/indigo_agent_solver.c`) | AC-001, AC-002, AC-004 | 2 | agent tests pass | done: 21/21 (C3) |
| 4 | Final run `python3 tools/run_driver_test.py agent_solver`, solver unit tests, Factory check/security | AC-003 | 3 | all pass, README test line and TEST_SUMMARY.md regenerated | done: 154/154, tool and unit suites pass, check/security pass; Factory self-test limitation C9/C10 |

## Test strategy
Integration tests of the agent; the solver tool tests are unaffected (not changed) but run as a regression.

## Security risks and mitigations
None new; the log handler is test-only and restored after use.

## Dependencies and architecture decisions
No new dependency; the public `indigo_log_message_handler` hook of indigo_bus.

## Migration, rollout, and rollback
Revert the single commit.

## Deviations and owner decisions
See specification.md.
