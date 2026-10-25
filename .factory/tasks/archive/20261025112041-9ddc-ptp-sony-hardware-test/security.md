# Security review
Verdict: pass
Reviewer: security reviewer pass, same agent as the developer (Claude Code desktop, claude-opus-5-5)
Date: 2026-10-25
Revision and working-tree state: `a89afe87d` plus the uncommitted task diff
Review independence: not independent; a separate pass over the final diff.

## Scope, assets, and trust boundaries
Camera replies (untrusted) and camera state; the user's camera card and settings during a test run.

## Risk coverage
| Area | Applicable? | Review evidence / reason not applicable |
| --- | --- | --- |
| Authentication and authorization | No | none involved |
| Input, injection, and output handling | Yes | the new refresh path reuses the bounded Sony decoder; no new parsing |
| Data, secrets, cryptography, and logs | Yes | INFO now shows the camera serial number, as other drivers do; the run record names the model only |
| Filesystem and outbound network access | Yes | hardware suite writes only its private directory and removes it; no card deletion |
| Dependencies and configuration | No | none |
| Data model (MIGRATION_STATUS.md, TEST_SUMMARY.md) | Yes | ccd_ptp count 162 / 15 and the recorded runs; Comment preserved |
| Abuse and failure behavior | Yes | dropped writes now end ALERT instead of a false OK; busy retry bounded to 10 s after connect |

## Automated checks
`./.factory/scripts/security`; sanitizer suites; see verification.md.

## Findings
None.

## Accepted risks
None recorded.

## Verdict and limitations
Pass. Physical camera on one host only.

## Register reconciliation
No change.
