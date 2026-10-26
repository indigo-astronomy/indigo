# Specification
Status: final

## Problem and outcome
No memory is lost per solver command.

## In scope
`indigo_drivers/agent_astrometry/indigo_agent_astrometry.c`.

## Out of scope
Other agents and the library.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | Valgrind reports no definitely lost block from `execute_command()` for a solve. | valgrind on `solve with filter` before/after |
| AC-002 | The agent suite passes unchanged. | recorded run |

## Failure behavior and edge cases
`free(NULL)` when the loop freed the last line is harmless.

## Permissions, data, and security requirements
None.

## Compatibility and nonfunctional requirements
None.

## Assumptions and open questions
None.

## Owner decisions and requirement changes
2026-10-26: the requester asked for the one-line fix.
