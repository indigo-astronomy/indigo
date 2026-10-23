# Security review
Verdict: pass
Reviewer: security reviewer pass, same agent as the developer (Claude Code, claude-opus-5-5)
Date: 2026-10-23
Revision and working-tree state: `task/20261023011844-cc6a-primaluce-calibration-finalizers` at `d37d4392d` plus the uncommitted task diff
Review independence: not independent; a separate pass over the final diff.

## Scope, assets, and trust boundaries
- The X_CALIBRATE and FOCUSER_ABORT_MOTION handlers, and the new calibration finalizers.
- Controller replies, which are untrusted device input, are read through the unchanged library calls.
- Asset: the focuser mechanics, which must stay stoppable.

## Risk coverage
| Area | Applicable? | Review evidence / reason not applicable |
| --- | --- | --- |
| Authentication and authorization | No | No change to access. |
| Input, injection, and output handling | Yes | No new parsing. The step table is a fixed array indexed only up to its NULL terminator, and the index is reset at each START. The library drain reads into the fixed response buffer with a bounded length and at most `PRIMALUCE_RESTART_POLLS` lines. The restarting gate copies a constant reason with `INDIGO_COPY_VALUE`. |
| Data, secrets, cryptography, and logs | No | No new data or logs. |
| Filesystem and outbound network access | No | None. |
| Dependencies and configuration | No | None. |
| Data model (MIGRATION_STATUS.md) | Yes | Only the simulated test count of the focuser_primaluce row changes (60 to 61, matching the recorded run); the Comment column is untouched. |
| Abuse and failure behavior | Yes | Abort is now served during the start: it cancels the pending step or restart poll before any motion. Each finalizer polls a bounded number of times (restart 20, range 20). Disconnect cancels them through the generated `indigo_cancel_pending_handlers()`. |

## Automated checks
`./.factory/scripts/security` and `findings.py check`; see verification.md.

## Findings
None in the security category. FND-000060 (quality) is fixed.

## Accepted risks
None recorded.

## Verdict and limitations
Pass. Only Linux is tested.

## Register reconciliation
FND-000060 is archived as fixed with this task's verification.md.
