# Test and regression plan

Status: complete

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001 | `./.factory/scripts/findings check` | none | pass | final |

## Baseline evidence
plan.md "Baseline".

## Bug reproduction
Recorded in the evidence of the shutdown command leak task.

## Negative cases and permission boundaries
Not applicable.

## Isolation and cleanup
Not applicable.

## Deferred tests and coverage limitations
The framework fix and its test are deferred to a separate task.
