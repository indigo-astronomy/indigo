# Test and regression plan

Status: executed

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001, AC-002 | diff review of `AGENTS.md` | none | one added bullet with the required content | After the change |
| T-002 | AC-003 | `./.factory/scripts/check` | Git wrapper | exit 0 | After the change |

## Baseline evidence
See plan.md.

## Bug reproduction
Not applicable.

## Negative cases and permission boundaries
Not applicable.

## Isolation and cleanup
None needed.

## Deferred tests and coverage limitations
A prose rule has no executable test; compliance is observed in later sessions.
