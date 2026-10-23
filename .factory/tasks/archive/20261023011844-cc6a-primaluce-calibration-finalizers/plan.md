# Development plan
Status: complete

## Approach and affected components
`indigo_focuser_primaluce.driver`, regenerated into the `.c`:
- three calibration finalizers and a shared failure helper;
- the X_CALIBRATE `on_change` publishes all of its own states, which the generator requires once a
  finalizer is referenced;
- the abort branch for a calibration that is still starting.

Also changed:
- the library (`primalucelib.c/.h`): the restarting gate, a short restart probe, and a drain of late
  replies;
- the simulator: a 5 s restart that replays up to 16 requests;
- `test_primalucelib.c`, `test_focuser_primaluce_simulator.c`;
- PROTOCOL.md, REFACTOR.md, MIGRATION_STATUS.md, and the test record.

## Baseline
- Commands and environment: Linux x64 at `d37d4392d`. The task 011 record is 60/60.
- Existing failures: none.
- Repository revision / working-tree state: `task/20261023011844-cc6a-primaluce-calibration-finalizers` from
  `task/20261022235021-bcb6-primaluce-sestosenso3` at `d37d4392d`. Tasks 010 and 011 are not integrated.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Finalizer chain in the `.driver`, regenerate | AC-001 to AC-004 | None | build; generated handler owns its updates | done |
| 2 | Regression cases | AC-001 to AC-003 | 1 | pass, and fail against 3.0.0.27 | done |
| 3 | Library restarting gate, probe, drain; driver poll skip; simulator replay; tests | AC-006, AC-007 | 2 | unit case; simulator cases fail without the drain | done |
| 4 | Recorded run, hardware check, docs, reviews | AC-004 to AC-007 | 3 | recorded run passes; hardware calibration works | done |

## Test strategy
- Simulator cases that observe queue availability while X_CALIBRATE is BUSY.
- An abort during the restart.
- A run of the new cases against the previous driver.
- A supervised hardware check of the real restart timing.

## Security risks and mitigations
None new; see security.md.

## Dependencies and architecture decisions
No new dependency. This follows the AGENTS.md handler + finalizer rule.

## Migration, rollout, and rollback
Not applicable beyond reverting the task commit.

## Deviations and owner decisions
None.
