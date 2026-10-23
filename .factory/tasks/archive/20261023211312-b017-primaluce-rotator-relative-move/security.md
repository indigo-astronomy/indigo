# Security review
Verdict: pass
Reviewer: security reviewer pass, same agent as the developer (Claude Code, claude-opus-5-5)
Date: 2026-10-24
Revision and working-tree state: `task/20261023211312-b017-primaluce-rotator-relative-move` at `17681f4f8` plus the uncommitted task diff
Review independence: not independent; a separate pass over the final diff.

## Scope, assets, and trust boundaries
- A new client-writable movement property.
- Controller replies, used for the angle and the acknowledgement.

## Risk coverage
| Area | Applicable? | Review evidence / reason not applicable |
| --- | --- | --- |
| Authentication and authorization | No | No change to access. |
| Input, injection, and output handling | Yes | The framework limits the offset to the item range (-180..180). The target is checked against the ROTATOR_POSITION range before any command. Reasons are formatted with `snprintf` into a fixed buffer and published through `"%s"`. |
| Data, secrets, cryptography, and logs | No | None. |
| Filesystem and outbound network access | No | None. |
| Dependencies and configuration | No | None. |
| Data model (MIGRATION_STATUS.md, PROPERTIES.md) | Yes | MIGRATION_STATUS.md: only the simulated test count changes (67 to 69, matching the recorded run). PROPERTIES.md: the PrimaLuce entry lists the property with its behaviour. |
| Abuse and failure behavior | Yes | Moves are bounded by the range check and the shared BUSY guard. An abort ends both properties. |

## Automated checks
`./.factory/scripts/security`; see verification.md.

## Findings
None.

## Accepted risks
None recorded.

## Verdict and limitations
Pass. No hardware check yet.

## Register reconciliation
No finding; this is a requested feature.
