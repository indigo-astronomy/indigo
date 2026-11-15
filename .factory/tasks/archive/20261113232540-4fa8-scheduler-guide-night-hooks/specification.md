# Specification

Status: approved by owner request 2026-11-14

## Problem and outcome
See brief.md.

## Current-state audit
The guide had night-hook sections from c9f5cbc5f, but the introduction, the Mount Agent dependency, the START description, the basic hooks example (IDLE without RESUME), the Alert state, the messages, the history example, the restart section and troubleshooting did not mention them, and the multi-night example named hook tasks without showing them.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | Every section that describes hooks, START, Alert, messages, restart and troubleshooting reflects the night hooks | review against the code |
| AC-002 | The multi-night example gives complete task files and commands, and a timeline of one night | private server |
| AC-003 | Statements about Sequencer steps (cooler waits, dome steps via the Mount Agent) are true | Sequencer.js, server acceptance evidence |
| AC-004 | Links and anchors resolve; no whitespace errors | script, git diff --check |

## Failure behavior and edge cases
Troubleshooting covers skipped night start, late night end, standby.

## Permissions, data, and security requirements
None.

## Compatibility and nonfunctional requirements
None.

## Assumptions and open questions
None.

## Owner decisions and requirement changes
None.
