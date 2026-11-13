# Security review

Verdict: pass
Verdict note: no security-relevant change
Reviewer: coordinator (implementer)
Date: 2026-11-12
Revision and working-tree state: master `e0280e650` plus the uncommitted task diff
Review independence: same agent as the implementer; the separate quality review (quality-review.md) was asked to report any security issue

## Scope, assets, and trust boundaries
One SDK call argument in `qhy2_setup()` and the fake SDK in the test. The trust boundary is unchanged: data from the vendor SDK is untrusted, and the frame size check after `GetQHYCCDSingleFrame()` is unchanged.

## Risk coverage
| Area | Applicable? | Review evidence / reason not applicable |
| --- | --- | --- |
| Authentication and authorization | No | No access control change. |
| Input, injection, and output handling | No | The timeout is computed from the framework-validated exposure; no new input path. |
| Data, secrets, cryptography, and logs | No | None involved. |
| Filesystem and outbound network access | No | No change. |
| Dependencies and configuration | No | No new dependency; same SDK. |
| Abuse and failure behavior | Yes | The SDK limit stays bounded at exposure + 10 s, equal to the acquisition deadline, so a camera that never answers cannot hold the device queue indefinitely. The `uint32_t` limit cannot overflow for any exposure the camera advertises. |

## Automated checks
None beyond the build and fake-SDK suites in verification.md.

## Findings
None.

## Accepted risks
None recorded.

## Verdict and limitations
No security impact.

## Register reconciliation
No findings, so no FND IDs.
