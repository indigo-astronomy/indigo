# Security review
Verdict: pass
Reviewer: security reviewer pass, same agent as the developer (Claude Code, claude-opus-5-5)
Date: 2026-10-23
Revision and working-tree state: `task/20261023132326-3447-primaluce-field-errors` at `82a071af5` plus the uncommitted task diff
Review independence: not independent; a separate pass over the final diff.

## Scope, assets, and trust boundaries
- Controller reply text (untrusted device input) is now shown to clients as a reason.
- The calibration acknowledgements.
- The test-only simulator.

## Risk coverage
| Area | Applicable? | Review evidence / reason not applicable |
| --- | --- | --- |
| Authentication and authorization | No | No change. |
| Input, injection, and output handling | Yes | The reply text is copied into `last_error` with `INDIGO_COPY_VALUE`, which is bounded by `INDIGO_VALUE_SIZE`. It is formatted into the reason with `snprintf` and a fixed buffer, and published through `"%s"`, never as a format string. A field without text gives no reason. |
| Data, secrets, cryptography, and logs | No | The text is a device reply; no secrets. |
| Filesystem and outbound network access | No | None. |
| Dependencies and configuration | No | None. |
| Data model (MIGRATION_STATUS.md) | Yes | The merge with origin/master combined upstream rows with this file. This task changes only the simulated test count of the focuser_primaluce row (65 to 66, matching the recorded run). The Comment column is untouched. |
| Abuse and failure behavior | Yes | A refused calibration step now stops the calibration instead of continuing, which is safer. |

## Automated checks
`./.factory/scripts/security` and `findings check`; see verification.md.

## Findings
None in the security category. FND-20261023132326-3447-primaluce-field-errors-001 is functional and fixed.

## Accepted risks
None recorded.

## Verdict and limitations
Pass.

## Register reconciliation
FND-20261023132326-3447-primaluce-field-errors-001 is archived as fixed with this task's verification.md.
