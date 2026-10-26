# Specification
Status: final

## Problem and outcome
The plate-solver agents capture their frames through a filter chosen by the user and return the wheel to the previous
filter afterwards.

## In scope
`indigo_libs/indigo_platesolver.c`, `indigo_libs/indigo/indigo_platesolver.h`, `indigo_libs/indigo/indigo_names.h`,
`indigo_drivers/agent_astrometry`, `indigo_drivers/agent_solver` (version), `indigo_docs/PROPERTIES.md`,
`indigo_test/integration/test_agent_astrometry.c`.

## Out of scope
Imager Agent behaviour, guider-only capture, passive solving (`AGENT_PLATESOLVER_SOLVE_IMAGES`) and uploaded images.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | `AGENT_PLATESOLVER_FILTER` (ONE_OF_MANY) lists `CURRENT` "Current filter" first and selected by default, followed by the slots of the related Imager Agent's `AGENT_WHEEL_FILTER` (same item names and labels). | `solver filter list mirrored` |
| AC-002 | With a filter chosen, every exposure a process (SOLVE, SYNC, CENTER, PRECISE_GOTO, polar alignment and recalculation) requests from the Imager Agent is preceded by a change to that filter, and the wheel is asked to return to the previous filter as soon as the frame has arrived, while it is solved. | `solve with filter`, `polar alignment switches filter per exposure`, end-to-end check |
| AC-003 | `CURRENT`, or a chosen filter already in the wheel, changes nothing. | `current or same filter keeps wheel` |
| AC-004 | On capture failure, solve failure and abort the wheel returns to the previous filter exactly once; a failed filter change ends the process in ALERT with "Failed to select filter '<name>'" and without a capture. | `capture failure restores filter`, `solve failure restores filter once`, `abort restores filter once`, `abort during filter change`, `filter change failure` |
| AC-005 | The list follows the Imager Agent: renamed slots keep the selected slot, a selected slot that disappears falls back to `CURRENT`, and without a related Imager Agent only `CURRENT` is listed. | `filter list follows Imager Agent` |
| AC-006 | The rest of the agents' behaviour is unchanged. | recorded `agent_astrometry` run; Imager Agent suite; leak check |

## Failure behavior and edge cases
- The selection waits up to 120 s for the wheel (abortable); a wheel that does not end OK on the slot gives ALERT.
- The return never blocks: wheel drivers ignore a slot change while they move, so a return requested during a
  selection (abort) is sent when the wheel reports that it stopped; the next selection waits for the wheel.
- The choice is not saved in the configuration (owner review 2026-10-26: keep it simple).

## Permissions, data, and security requirements
None beyond the existing agent permissions.

## Compatibility and nonfunctional requirements
New property only; the default leaves the existing behaviour unchanged.

## Assumptions and open questions
None.

## Owner decisions and requirement changes
2026-10-26: switch per exposure in multi-exposure processes; plan approved. After the first implementation the requester found it overcomplicated; it was reduced to a slot mirror with the slot to return to (no choice by name, no persistence, no queued return).
