# Development plan

Status: complete

## Approach and affected components
Run the existing hardware suite on the ILCE-7RM2, add stepping support to the suite, trace failures with Pi-only diagnostics, fix the Sony path, model each observation in the fake old-API profile.

## Baseline
- Pi 5 indigosky.local, ILCE-7RM2 (054c:0a6b), master 70d34909b, driver 3.0.0.59: hardware suite 2/15 (stepping controls not selectable).

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Stepping helpers in the hardware suite | AC-003 | None | suite selects/restores | done |
| 2 | Time-based Sony live view tolerance | AC-001 | 1 | live view after capture on hardware; fake case | done |
| 3 | Old-API stale shot accounting with expiry | AC-002 | 1 | aborted shot discarded on hardware; fake cases | done |
| 4 | Review fixes, version 3.0.0.60, recorded runs | AC-004, AC-005 | 2, 3 | script runs | see verification.md |

## Test strategy
Fake regressions with negative controls; full recorded fake and hardware runs; sanitizer runs.

## Security risks and mitigations
No new parsing; bounded waits and counters with expiry.

## Dependencies and architecture decisions
None.

## Migration, rollout, and rollback
Revert the commit.

## Deviations and owner decisions
None.
