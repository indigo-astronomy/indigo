# Test and regression plan

Status: complete

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001 | `git diff -M --stat` | none | rename, one changed line | final |
| T-002 | AC-002 | link and anchor check; grep for the old path | none | none broken, no leftover | final |
| T-003 | AC-003 | pbxproj diff | none | reference moved into the new group | final |

## Baseline evidence
plan.md "Baseline".

## Bug reproduction
Not applicable.

## Negative cases and permission boundaries
Not applicable.

## Isolation and cleanup
None.

## Deferred tests and coverage limitations
The test file changes are comments only; nothing was rebuilt. The Xcode project was not opened in Xcode (Linux host).
