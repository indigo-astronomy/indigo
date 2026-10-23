# Specification
Status: final

## Problem and outcome
Every tracked Factory record added by the PrimaLuce tasks, and this task's own records, is referenced in the Xcode navigator.

## In scope
`indigo.xcodeproj/project.pbxproj` navigator groups; the task records.

## Out of scope
Build targets, Windows changes, other contributors' entries.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | `findings check` and `findings sync-project --check` pass. | commands |
| AC-002 | Every archived task folder and completion record is referenced; every new object ID appears exactly once as a definition and once as a child. | grep checks |
| AC-003 | The project keeps balanced braces and parentheses, and no existing line changes. | diff review |
| AC-004 | The PrimaLuce Windows projects parse as XML and their GUID references match the solution. | XML parse, grep |

## Failure behavior and edge cases
The script stops without writing when an ID collides, a group cannot be identified, or a record is missing.

## Permissions, data, and security requirements
None.

## Compatibility and nonfunctional requirements
Object IDs follow the `indigo-navigator:` SHA-256 scheme of the earlier repair, so later repairs stay deterministic.

## Assumptions and open questions
Xcode itself was not available to open the project.

## Owner decisions and requirement changes
2026-10-24: the requester asked for the fix.
