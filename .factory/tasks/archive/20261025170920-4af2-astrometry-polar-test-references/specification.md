# Specification
Status: final

## Problem and outcome
The polar alignment cases test the agent with references that define an alignment.

## In scope
`indigo_test/integration/test_agent_astrometry.c` (fake solver `polar` mode, three cases).

## Out of scope
Agent and library code.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | The three polar alignment cases pass with three distinct references and a recalculation at the third. | the cases; recorded run |
| AC-002 | The rest of the suite is unchanged (the default fake solver output stays the same). | recorded run 77/77 |

## Failure behavior and edge cases
`polar alignment failures` keeps the default mode and still expects ALERT for a refused mount start.

## Permissions, data, and security requirements
None.

## Compatibility and nonfunctional requirements
None.

## Assumptions and open questions
None.

## Owner decisions and requirement changes
2026-10-25: investigate the failures.
