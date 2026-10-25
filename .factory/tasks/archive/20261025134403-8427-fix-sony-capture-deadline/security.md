# Security review

Verdict: pass
Reviewer: coordinator
Date: 2026-10-25
Revision and working-tree state: final uncommitted candidate on the task branch
Review independence: not independent (implementer)

## Scope, assets, and trust boundaries
USB PTP camera replies (untrusted) in the Sony exposure state machine and the connection handler.

## Risk coverage
| Area | Applicable? | Review evidence / reason not applicable |
| --- | --- | --- |
| Authentication and authorization | No | no access control change |
| Input, injection, and output handling | Yes | only an existing decoded property value and the shutter value are read; decode bounds unchanged; shutter decode divides only by a non-zero denominator |
| Data, secrets, cryptography, and logs | No | one new log/error line without device data |
| Filesystem and outbound network access | No | none |
| Dependencies and configuration | No | none |
| Abuse and failure behavior | Yes | retry count and reopen wait are bounded (2 retries, 1 s); abort and capture deadline unchanged |

## Automated checks
`./.factory/scripts/security` pass; ASan/UBSan fake suites pass (verification.md).

## Findings
None.

## Accepted risks
None recorded.

## Verdict and limitations
Pass; hardware-io risk reviewed above.

## Register reconciliation
No security findings.
