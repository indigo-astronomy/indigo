# Security review

Verdict: pass
Reviewer: coordinator (Claude Code, claude-opus-5-5)
Date: 2026-11-14
Revision and working-tree state: branch `scheduler` at 2646b7afa with this task's changes
Review independence: not independent for security; the independent quality review also checked bounds and threading.

## Scope, assets, and trust boundaries
New client inputs: one switch property (one-of-many, fixed items), two number items clamped by the framework and again in the night module (±240), three text items holding task names resolved by lookup. No new file, network or script path: hooks run through the existing wrapper with JSON-encoded names.

## Risk coverage
| Area | Applicable? | Review evidence / reason not applicable |
| --- | --- | --- |
| Authentication and authorization | No | Existing bus access unchanged. |
| Input, injection, and output handling | Yes | Hook names only compared with task names; labels cut to INDIGO_VALUE_SIZE; offsets clamped. |
| Data, secrets, cryptography, and logs | No | No data. |
| Filesystem and outbound network access | No | Only the existing configuration save. |
| Dependencies and configuration | No | No new dependency. |
| Data model and migrations | Yes | The process policy flags `MIGRATION_STATUS.md` by name. The change updates only the automated-test count of the `agent_scheduler` row (36 / 0 to 52 / 0); the Comment column is unchanged. The agent's saved configuration gains the `AGENT_SCHEDULER_NIGHT` property; older configurations load with the defaults. |
| Abuse and failure behavior | Yes | Fixed arrays (8 nights, 2 plan hooks), per-call allocation, ASan/UBSan clean, failed preparation runs the shutdown hook. |

## Automated checks
ASan/UBSan integration suite (verification.md).

## Findings
None.

## Accepted risks
None recorded.

## Verdict and limitations
Pass.

## Register reconciliation
No security findings.
