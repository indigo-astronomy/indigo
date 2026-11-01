# Security review

Verdict: pass
Reviewer: the implementing session (Claude in the Claude Desktop Code tab, model claude-fable-5-1 after a mid-task switch from claude-opus-5-5)
Date: 2026-11-01
Revision and working-tree state: uncommitted diff on top of 90e8424bd (origin/master merged into the task branch)
Review independence: not independent, the same session implemented the change. The quality review was performed by a separate subagent.

## Scope, assets, and trust boundaries
The Alpaca Agent bridges HTTP Alpaca clients to INDIGO devices. The change adds one boolean per device record, set from the existing `PUT connected` handler and cleared from the device's own CONNECTION updates, and makes the automatic guider connection conditional on it. No new input is parsed, no new property, file, socket or credential is introduced.

## Risk coverage
| Area | Applicable? | Review evidence / reason not applicable |
| --- | --- | --- |
| Authentication and authorization | Yes, reduced | The agent no longer claims a device no Alpaca client asked for, so an idle agent cannot block other agents' device selection. Alpaca access control is unchanged. |
| Input, injection, and output handling | No | No new parsing; the `connected` parameter handling is unchanged. |
| Data, secrets, cryptography, and logs | No | None involved. |
| Filesystem and outbound network access | No | None added. |
| Dependencies and configuration | No | None added. |
| Abuse and failure behavior | Yes | A failed or externally ended mount connection clears the flag (AC-004). A client that exits without `connected=false` leaves the flag set while the mount stays connected; that is narrower than the previous unconditional behavior and is recorded in verification.md. |

## Automated checks
`./.factory/scripts/check` passed on 2026-11-01 (Factory boundaries). No security scanner is configured for C drivers in this project. Build and tests: see verification.md.

## Findings
None.

## Accepted risks
None recorded.

## Verdict and limitations
Pass. The change shrinks the agent's unrequested actions and adds no attack surface. Reviewed by the implementer, diff and its call paths only.

## Register reconciliation
FND-20261101163708-4785-fix-alpaca-guider-autoconnect-001 (functional) records the defect and its fix; no security finding is registered.
