# Security review
Verdict: pass
Reviewer: security reviewer pass, same agent as the developer (Claude Code, claude-opus-5-5)
Date: 2026-10-24
Revision and working-tree state: `task/20261024092953-7ee4-primaluce-giotto-alto-drivers` at `83899ad9b` plus the uncommitted task diff
Review independence: not independent; a separate pass over the final diff.

## Scope, assets, and trust boundaries
- Controller replies over USB serial or HTTP (untrusted).
- Client property requests (validated by the framework).

## Risk coverage
| Area | Applicable? | Review evidence / reason not applicable |
| --- | --- | --- |
| Authentication and authorization | No | No change to access. |
| Input, injection, and output handling | Yes | Replies are parsed by the existing jsmn-based library with bounded buffers. Refusal texts are copied with `INDIGO_COPY_VALUE` and published through `"%s"`. Preset names containing `"` or `\` are not sent (no escaping in the protocol). Numbers come from the framework within the item ranges. |
| Data, secrets, cryptography, and logs | Yes | The panel's full state contains its WiFi password; the driver does not publish it. The requester's raw probe transcripts stay in the session scratchpad, not in the repository. |
| Filesystem and outbound network access | No | The simulator writes only to the test fixture directory; the driver opens only the configured port. |
| Dependencies and configuration | Yes | No new dependency: `jsmn.h` (MIT) is already in `primalucelib`; the simulator includes it with `JSMN_STATIC`. |
| Data model (MIGRATION_STATUS.md, PROPERTIES.md) | Yes | MIGRATION_STATUS.md: one new row (25 / 8, matching the recorded runs; Comment empty). PROPERTIES.md: one new driver section listing every property it defines; names use the `X_` prefix. |
| Abuse and failure behavior | Yes | Requests are bounded by the library timeouts; failures end ALERT without stale BUSY; disconnect switches the light off and completes on a dead port. |

## Automated checks
`./.factory/scripts/security`; see verification.md.

## Findings
None open (the format-string issue was fixed before review; see quality-review.md).

## Accepted risks
None recorded.

## Verdict and limitations
Pass.

## Register reconciliation
No finding.
