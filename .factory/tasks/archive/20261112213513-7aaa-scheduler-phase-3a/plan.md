# Development plan

Status: complete

## Approach and affected components
Server acceptance runner with simulators; agent hooks, history, estimate and measurement; `Sequencer.js` `estimate()`; tests and docs.

## Baseline
- Revision 6898c7094 on branch `scheduler`; scheduler suite 23/23, Sequencer suite 64/64, scripting 36/37 (pre-existing FND-20261105112837-9c3a-sequencer-simulator-suite-004), opt-in simulator suite with the pre-existing "restore private configuration" failure (FND-20261105112837-9c3a-sequencer-simulator-suite-005).

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | `run_scheduler_simulators.py` against a private server | AC-001 | None | 5/5 | done: all failures were in the new client (JSON heartbeat, standalone messages, acknowledgement read, explicit site, restart after the automatic stop); none in the agent |
| 2 | `Sequence.prototype.estimate()` + test | AC-005 | None | sequencer case | done |
| 3 | Agent hooks, history, estimate, measurement, names | AC-002..AC-004 | 2 | 29 integration cases | done: found and fixed the history trimming on restore, the estimate race at selection, the automatic-stop race |
| 4 | Server cases for estimate and shutdown hook | AC-001, AC-002, AC-004 | 3 | 6/6 | done |
| 5 | Independent review, fixes | all | 3 | 30 cases, ASan | done: 1 blocking, 5 should-fix fixed |
| 6 | Docs, MCP help, versions, counts, recorded runs | AC-006 | 5 | verification.md | done |

## Test strategy
Integration cases with the real Scripting Agent; server acceptance with simulators; sanitizer build; inventory tests of the Sequence API updated for the new public method.

## Security risks and mitigations
History text sanitised; history restore accepted only from the configuration reader; estimate values checked for finiteness and clamped.

## Dependencies and architecture decisions
None new.

## Migration, rollout, and rollback
Older configurations load (missing `measured=` reads as 0, missing settings keep defaults).

## Deviations and owner decisions
- Script errors are found by the asynchronous estimate and mark the task SCRIPT ERROR instead of rejecting the upload (SCHEDULER.md updated).
- A task waits for its pending estimate before selection.
- An automatic stop is cancelled by a task added before its shutdown hook starts; START during the shutdown hook continues scheduling.
- The Sequence API inventory tests now expect 110 prototype names and 109 public methods (estimate() added on purpose).
