# Specification

Status: approved by owner request 2026-11-13

## Problem and outcome
See brief.md.

## Current-state audit
`agent_change_property()` queues a heap command with a property copy; `indigo_cancel_pending_handlers()` drops queued handlers without freeing their data. Reproduced: 200 requests followed by a shutdown leak 3 commands (6 allocations).

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | Requests still queued at shutdown are released; requests arriving while the agent shuts down are ignored | integration `commands_queued_at_shutdown_are_released` with the leak check in the sanitizer build |
| AC-002 | Requests outside shutdown, including the configuration restore, are handled as before | full integration suite, server acceptance run |

## Failure behavior and edge cases
A handler running during shutdown finishes before the queue is cleared; a second stop from detach is harmless.

## Permissions, data, and security requirements
None.

## Compatibility and nonfunctional requirements
None.

## Assumptions and open questions
None.

## Owner decisions and requirement changes
None.
