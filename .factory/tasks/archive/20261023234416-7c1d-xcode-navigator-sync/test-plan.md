# Test and regression plan

Status: final

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001 | `findings check`, `findings sync-project --check` | working tree | pass | after each step |
| T-002 | AC-002, AC-003 | grep and count checks on `project.pbxproj` | working tree | no missing, each ID twice, balanced | final |
| T-003 | AC-004 | XML parse and GUID grep | working tree | pass | final |

## Baseline evidence
`findings check` failed with "Xcode findings references are stale" at `ff98d482b`.

## Bug reproduction
The baseline failure above, and five task folders and five completion records with no reference.

## Negative cases and permission boundaries
The script refuses ID collisions and missing records.

## Isolation and cleanup
The script stays in the session scratchpad.
