# Test and regression plan

Status: complete

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001, AC-002 | Inspection of `indigo_docs/SCHEDULER.md` | None | All sections present, owner decisions reflected | Before delivery |
| T-002 | AC-003 | Independent review against source | None | No inaccurate code claims | Before delivery |
| T-003 | AC-004 | `git diff indigo.xcodeproj/project.pbxproj`, ID grep | None | One file reference and one group entry, unique ID | Before delivery |

## Baseline evidence
Not applicable: documentation only.

## Bug reproduction
Not applicable.

## Negative cases and permission boundaries
Not applicable.

## Isolation and cleanup
No temporary files.

## Deferred tests and coverage limitations
Scheduler tests are specified in the document and belong to the implementation task.
