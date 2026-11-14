# Development plan

Status: complete

## Approach and affected components
Parser change in indigo_prop_tool with type checks at send time; parser unit test; harness build fixes; a Python sandbox tool with a checked-in night scenario and an opt-in make target; docs.

## Baseline
- Branch `scheduler` at ebc39919d. `ITEM=` rejected, `ITEM=""` accepted; test_tool_input_safety.py did not build on Linux.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Reproduce empty values against a private server | AC-001 | None | `IDLE=""` clears, `IDLE=` rejected | done |
| 2 | Parser and send-time checks in indigo_prop_tool.c; help note | AC-001, AC-002 | 1 | parser test, private server | done |
| 3 | Parser test expectations and empty_values case; harness flags | AC-001 | 2 | parser suite passes | done; 3 unrelated tool tests fail (finding 002) |
| 4 | tools/scheduler_sandbox.py, night scenario, make target, Xcode | AC-003..AC-005 | None | scripted runs | done; Sun computation switched to the INDIGO libraries after a 0.375 deg mismatch (finding 001) |
| 5 | Guide and PROPERTY_MANIPULATION.md | AC-006 | 2, 4 | review, real output | done |
| 6 | Independent review and fixes | all | 5 | verification.md | done |

## Test strategy
Parser unit test; private servers for the tool; scripted sandbox scenarios including failure paths.

## Security risks and mitigations
The sandbox refuses a busy port so that it can never drive another server; scratch HOME.

## Dependencies and architecture decisions
No new dependency; Python standard library and ctypes with the built INDIGO libraries.

## Migration, rollout, and rollback
`ITEM=` changes from an error to an empty value (owner request), documented.

## Deviations and owner decisions
Finding records written directly on `scheduler` as in the earlier scheduler task; the helper requires a task/ branch.
