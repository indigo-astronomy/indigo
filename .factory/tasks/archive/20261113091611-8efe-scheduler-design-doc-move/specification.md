# Specification

Status: approved by owner request 2026-11-13

## Problem and outcome
See brief.md.

## Current-state audit
`indigo_docs/SCHEDULER.md` is linked from the user guide (3 links), INDIGO_AGENTS.md, PROPERTIES.md and the agent README, named in four test file comments, listed in the Xcode `indigo_docs` group; it links to the user guide.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | The design document is at `indigo_drivers/agent_scheduler/doc/SCHEDULER.md`, content unchanged except its relative link | `git diff -M` |
| AC-002 | Every link and reference to it, and its own link, resolve | link and anchor check; grep for the old path |
| AC-003 | The Xcode project lists it in a `doc` subgroup of `agent_scheduler`, not in `indigo_docs` | diff |

## Failure behavior and edge cases
None.

## Permissions, data, and security requirements
README link change approved by the owner.

## Compatibility and nonfunctional requirements
None.

## Assumptions and open questions
None.

## Owner decisions and requirement changes
- 2026-11-13: README link update approved.
