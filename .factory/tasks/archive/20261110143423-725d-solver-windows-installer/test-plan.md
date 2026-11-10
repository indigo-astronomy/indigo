# Test and regression plan

Status: executed

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-002 | `diff` of HEAD's `[Code]` of indigo.iss and path.iss | none | identical | after change |
| T-002 | AC-003 | `bash -n tools/build_windows_installer.sh` | none | no syntax error | after change |
| T-003 | AC-004 | brace/parenthesis balance of project.pbxproj | none | balanced as HEAD | after change |
| T-004 | AC-001 | review of indigo_solver.iss against indigo.iss | none | consistent | after change |

## Baseline evidence
Not applicable.

## Bug reproduction
Not applicable.

## Negative cases and permission boundaries
Not applicable.

## Isolation and cleanup
None.

## Deferred tests and coverage limitations
ISCC compile and a Windows install/uninstall (PATH added/removed, side by side with INDIGO) not run here.
