# Security review
Verdict: pass
Reviewer: security reviewer pass, same agent as the developer (Claude Code desktop, claude-opus-5-5), plus the independent read-only diff review in quality-review.md
Date: 2026-10-25
Revision and working-tree state: `task/20261025093224-2d50-fix-ptp-findings` at `93bb23d76` plus the uncommitted task diff
Review independence: not independent; a separate pass over the final diff.

## Scope, assets, and trust boundaries
Camera replies over USB or ImageCaptureCore are untrusted input to the driver process (indigo_server or a client host). Assets: driver memory, published BLOBs and saved image files.

## Risk coverage
| Area | Applicable? | Review evidence / reason not applicable |
| --- | --- | --- |
| Authentication and authorization | No | No access control involved. |
| Input, injection, and output handling | Yes | DeviceInfo, descriptors, vendor code lists, ObjectInfo and Canon event records are length- and capacity-checked; BLOB and file sizes come from the received length; the Canon text request is terminated and bounded. Covered by 13 fake cases under ASan/UBSan. |
| Data, secrets, cryptography, and logs | No | New log lines carry handles, sizes and codes only. |
| Filesystem and outbound network access | Yes | Saved image files now contain exactly the received bytes. No network. |
| Dependencies and configuration | No | None. |
| Data model (MIGRATION_STATUS.md, TEST_SUMMARY.md) | Yes | Only the ccd_ptp test count (159 / 4) and the recorded run changed; Comment column preserved. |
| Abuse and failure behavior | Yes | Malformed replies are rejected or truncated without stopping the connection; the camera keeps capturing in every new case. |

## Automated checks
`./.factory/scripts/security`, sanitizer suites; see verification.md.

## Findings
Four findings fixed; residual low-severity FND-20261025093224-2d50-fix-ptp-findings-001 (reads without reply length on three paths, writes already capacity-checked) remains open.

## Accepted risks
None recorded.

## Verdict and limitations
Pass for the scoped findings. No physical camera; macOS arm64 execution only.

## Register reconciliation
See findings.md.
