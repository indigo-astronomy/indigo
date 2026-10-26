# Development plan

Status: complete

## Approach and affected components
Pi-only probes with temporary tracing (no shot for PriorityMode; 2 shots per leftover probe), then fixes, fake regressions, one review, one recorded run.

## Baseline
Pi 5 indigosky.local, ILCE-7RM4 fw 1.20, master 24319f3e6, driver 3.0.0.60.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Probe PriorityMode and leftover behaviour | all | None | traces, EXIF | done |
| 2 | pending_shot_until across reconnect, generic drain, PriorityMode write | AC-001..004 | 1 | fake cases with negative controls | done |
| 3 | Hardware case "Reconnect during exposure" with EXIF check | AC-001 | 2 | parser checked on the probe image | done |
| 4 | Review, version 3.0.0.61, recorded runs | AC-005 | 3 | script runs | see verification.md |

## Test strategy
Fake regressions with negative controls, hardware case in the recorded suite, sanitizer runs.

## Security risks and mitigations
Bounded EXIF reads; bounded drain and waits.

## Dependencies and architecture decisions
None.

## Migration, rollout, and rollback
Revert the commit.

## Deviations and owner decisions
See specification.md.
