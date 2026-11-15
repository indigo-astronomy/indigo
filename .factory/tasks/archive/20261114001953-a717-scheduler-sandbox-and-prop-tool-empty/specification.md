# Specification

Status: approved by owner request 2026-11-14

## Problem and outcome
See brief.md.

## Current-state audit
- indigo_prop_tool: `ITEM=` rejected by parse_set_property_string() ("Invalid property string format"); `ITEM=""` already worked because quotes are removed after the emptiness check; the guide wrongly said empty values cannot be sent. The parser test asserted that `D.P.I=` and `D.P.I=  ` are invalid.
- test_tool_input_safety.py could not build on Linux (no -D_GNU_SOURCE for test_runner.h's nftw, no Avahi libraries for libindigo_client.a).
- No hand-testing tool; the automated tests move the site longitude to produce dusk and dawn.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | `ITEM=`, `ITEM=  ` and `ITEM=""` set a text item to an empty string; other items unchanged; the item name is still required | parser unit test, private server |
| AC-002 | An empty value for a number or switch item exits with an error message | private server |
| AC-003 | The sandbox starts a private server, chooses the time of day, makes waiting tasks, sets hooks (also empty), controls and shows the scheduler, and prints its messages | scripted runs |
| AC-004 | `dusk N`/`dawn N` agree with the agent's night computation | comparison with indigo_scheduler_sun_altitude, timing of the hooks |
| AC-005 | Scripts end with exit code 1 when an `until` times out; the night scenario passes through an opt-in make target | runs |
| AC-006 | The guide and PROPERTY_MANIPULATION.md describe the tool and empty values correctly | review, private server |

## Failure behavior and edge cases
Missing build: clear message. Server exit: message with the log path. Unknown command: message.

## Permissions, data, and security requirements
The sandbox binds a local port, uses a scratch HOME and removes it unless --dir is given.

## Compatibility and nonfunctional requirements
Changed contract: `ITEM=` was rejected and is now an empty value (owner request); the parser test expectations were updated with this reason.

## Assumptions and open questions
None.

## Owner decisions and requirement changes
- 2026-11-14: tool and empty values requested.
