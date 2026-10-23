# Security review
Verdict: pass
Reviewer: security reviewer pass, same agent as the developer (Claude Code, claude-opus-5-5)
Date: 2026-10-23
Revision and working-tree state: `task/20261022235021-bcb6-primaluce-sestosenso3` at `4a32f088d` plus the uncommitted task changes listed in handoff.md
Review independence: not independent; this was a separate review pass over the final diff after implementation.

## Scope, assets, and trust boundaries
- Controller replies over USB serial or HTTP, which are untrusted device input.
- Client switch requests on `X_CALIBRATE` and `FOCUSER_ABORT_MOTION`.
- The test-only simulator.
- Asset: physical safety of the focuser mechanics. A calibration run must stay stoppable.

## Risk coverage
| Area | Applicable? | Review evidence / reason not applicable |
| --- | --- | --- |
| Authentication and authorization | No | No change to bus access; the controller has none on USB. |
| Input, injection, and output handling | Yes | `primalucelib_vrequest` formats into a fixed 8192-byte buffer with `vsnprintf` (truncation, no overflow). `primalucelib_request_verb` copies at most 15 bytes with a length check. Replies are parsed as before (jsmn, bounded tokens). The stale-reply loop is bounded by `PRIMALUCE_MAX_STALE_REPLIES` and the read timeout. Requests built from client data are unchanged (FND-000052 stays open, out of scope). |
| Data, secrets, cryptography, and logs | Yes | New log lines carry no payload. The simulator's WiFi strings are synthetic. No secrets are added. |
| Filesystem and outbound network access | No | No new access. The simulator queue is in memory. |
| Dependencies and configuration | No | No new dependency. |
| Data model (MIGRATION_STATUS.md) | Yes | Only the simulated test count of the focuser_primaluce row changed (55 to 60, matching the recorded run); the Comment column is untouched. No schema or other row changed. |
| Abuse and failure behavior | Yes | FND-000057, a calibration run that END could not stop, was a physical-safety defect. Fixed and covered: extra requests are refused, END is accepted during the run, and abort sends the SESTO SENSO 3 stop commands (FND-000058). The restart wait and the range poll are bounded (10 s, 5 s), and no motion runs during them. |

## Automated checks
- `./.factory/scripts/security` with the Git wrapper from the handoff: see verification.md.
- `python3 .factory/scripts/findings.py check`: passes.

## Findings
- No new security-category finding.
- FND-000057 and FND-000058 are functional findings with a safety impact. Both are fixed; their retest evidence is in verification.md.

## Accepted risks
None recorded.

## Verdict and limitations
- Pass. The SESTO SENSO 3 abort during calibration is verified with the simulator only, not on the unit.

## Register reconciliation
FND-000053 to FND-000059 are archived as fixed with this task's verification.md. FND-000052 stays open.
