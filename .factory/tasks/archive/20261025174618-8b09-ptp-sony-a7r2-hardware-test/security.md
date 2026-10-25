# Security review

Verdict: pass
Reviewer: coordinator
Date: 2026-10-25
Revision and working-tree state: final candidate on the task branch
Review independence: not independent (implementer)

## Scope, assets, and trust boundaries
USB PTP camera replies (untrusted) in the Sony exposure, event, live view and property-write paths.

## Risk coverage
| Area | Applicable? | Review evidence / reason not applicable |
| --- | --- | --- |
| Authentication and authorization | No | none |
| Input, injection, and output handling | Yes | discarded objects use the existing bounded ObjectInfo/GetObject decode; no new parsing |
| Data, secrets, cryptography, and logs | No | log lines carry file names only |
| Filesystem and outbound network access | No | discarded objects are freed, not stored |
| Dependencies and configuration | No | none |
| Data model (status documents) | Yes | `MIGRATION_STATUS.md` changes only the ccd_ptp counts (177 / 15); Comment column untouched |
| Abuse and failure behavior | Yes | drain at most 4 objects, retries at most 2, waits bounded (30 s live view, 15 s write confirmation), pending shots expire; 0xffffc001 touched only when reported ready or announced |

## Automated checks
`./.factory/scripts/security` pass; ASan/UBSan fake suites pass.

## Findings
None.

## Accepted risks
None recorded.

## Verdict and limitations
Pass.

## Register reconciliation
No security findings.
