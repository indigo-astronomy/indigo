# Security review
Verdict: pass
Reviewer: security reviewer pass, same agent as the developer (Claude Code desktop, claude-opus-5-5)
Date: 2026-10-25
Revision and working-tree state: `e49bfce27` plus the uncommitted task diff
Review independence: not independent; a separate pass over the final diff.

## Scope, assets, and trust boundaries
Camera replies and USB descriptors (untrusted); camera state and card during a test run.

## Risk coverage
| Area | Applicable? | Review evidence / reason not applicable |
| --- | --- | --- |
| Authentication and authorization | No | none involved |
| Input, injection, and output handling | Yes | USB config descriptors are read through libusb and checked for `num_altsetting`; the Bulb removal bounds `memmove` by the decoded count, the append is now bounded by `PTP_MAX_ELEMENTS` |
| Data, secrets, cryptography, and logs | Yes | records name the model only; no serials in reports |
| Filesystem and outbound network access | Yes | hardware suite writes only its private directory; no card deletion |
| Dependencies and configuration | No | none |
| Data model (MIGRATION_STATUS.md, TEST_SUMMARY.md) | Yes | ccd_ptp count 165 / 15 and the recorded runs; Comment preserved |
| Abuse and failure behavior | Yes | readback after a failed write is one bounded transaction, skipped when disconnected or detaching |

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
