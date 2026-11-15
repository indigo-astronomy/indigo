# Development plan

Status: complete

## Approach and affected components
Capture a running sequencer and an enabled flipper before the definitions, restore them after; skip deleting loop properties when a sequence is kept.

## Baseline
- `test_agent_scripting_sequencer`: 63/63 before the change.
- `test_agent_scripting`: 36/37 (pre-existing FND-20261105112837-9c3a-sequencer-simulator-suite-004).
- Repository revision: dfa68ee2f on branch `scheduler`.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Regression test in `indigo_test/integration/test_agent_scripting_sequencer.c` | AC-001..003 | None | fails before the fix | done: failed with "indigo_sequencer.sequence != null" |
| 2 | Fix in `Sequencer.js`, doc in `Sequencer.md` | AC-001..003 | 1 | test passes | done: passes after waiting for the asynchronously defined LOOP_0 in the test |
| 3 | Regenerate `Sequencer.js.dat`, bump Scheduler (3.0.0.2) and Scripting (3.0.0.16) versions, scheduler case, design text | AC-004 | 2 | scheduler suite | done: 23/23, ASan clean |
| 4 | Recorded runs and regression comparison | AC-005 | 3 | verification.md | done |

## Test strategy
Characterization-first regression test, end-to-end scheduler case, full suites.

## Security risks and mitigations
None.

## Dependencies and architecture decisions
None.

## Migration, rollout, and rollback
Reverting the library change restores the old behaviour.

## Deviations and owner decisions
None.
