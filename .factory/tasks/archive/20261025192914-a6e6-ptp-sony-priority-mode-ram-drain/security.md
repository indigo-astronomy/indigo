# Security review

Verdict: pass
Reviewer: coordinator
Date: 2026-10-25
Revision and working-tree state: final candidate on the task branch
Review independence: not independent (implementer)

## Scope, assets, and trust boundaries
USB PTP camera replies (untrusted) in the Sony exposure and connect paths; JPEG data parsed by the hardware test.

## Risk coverage
| Area | Applicable? | Review evidence / reason not applicable |
| --- | --- | --- |
| Authentication and authorization | No | none |
| Input, injection, and output handling | Yes | driver: no new parsing; test EXIF reader bounds-checked against the buffer size |
| Data, secrets, cryptography, and logs | No | none |
| Filesystem and outbound network access | No | none |
| Dependencies and configuration | No | none |
| Data model (status documents) | Yes | `MIGRATION_STATUS.md` changes only the ccd_ptp counts (180 / 16); Comment column untouched |
| Abuse and failure behavior | Yes | accesses to 0xffffc001 only when a ready value is reported and not already spent (firmware-crash risk), bounded drain, expiry of pending shots |

## Automated checks
`./.factory/scripts/security` pass; ASan/UBSan pass.

## Findings
None.

## Accepted risks
None recorded.

## Verdict and limitations
Pass.

## Register reconciliation
No security findings.
