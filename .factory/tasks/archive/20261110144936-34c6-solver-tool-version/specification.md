# Specification

Status: approved by the owner request of 2026-11-10

## Problem and outcome
The help of `indigo_solver` starts with the version, build and build date like indigo_prop_tool.

## In scope
First help line `INDIGO Solver v.<major>.<minor>-<build> built on <__DATE__> <__TIME__>.`, then
`Part of the INDIGO project, https://www.indigo-astronomy.org`, then the description.

## Out of scope
A `--version` option.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | `indigo_solver -h` starts with `INDIGO Solver v.3.0-<INDIGO_BUILD> built on ` followed by the build date and time | tool suite `help_lists_switches_and_defaults` |
| AC-002 | the next line says `Part of the INDIGO project, https://www.indigo-astronomy.org` | same test |

## Failure behavior and edge cases
None.

## Permissions, data, and security requirements
None.

## Compatibility and nonfunctional requirements
Same macros as indigo_prop_tool.

## Assumptions and open questions
None.

## Owner decisions and requirement changes
None.
