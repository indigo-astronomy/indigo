# Security review
Verdict: pass
Reviewer: security reviewer pass, same agent as the developer (Claude Code, claude-opus-5-5)
Date: 2026-10-25
Revision and working-tree state: `task/20261025004416-0995-xml-client-named-request-protocol` at `5aa5660f3` plus the uncommitted task diff
Review independence: not independent; a separate pass over the final diff.

## Scope, assets, and trust boundaries
XML client requests to a server.

## Risk coverage
| Area | Applicable? | Review evidence / reason not applicable |
| --- | --- | --- |
| Authentication and authorization | No | Tokens are unaffected. |
| Input, injection, and output handling | Yes | The added attributes are fixed text and numbers; the name handling is unchanged apart from skipping the legacy translation, and it was not escaped before either. |
| Data, secrets, cryptography, and logs | No | None. |
| Filesystem and outbound network access | No | None. |
| Dependencies and configuration | No | None. |
| Data model (MIGRATION_STATUS.md, PROPERTIES.md) | No | No property change. |
| Abuse and failure behavior | No | A server that ignores `switch` behaves as before. |

## Automated checks
`./.factory/scripts/security`; see verification.md.

## Findings
None registered.

## Accepted risks
None recorded.

## Verdict and limitations
Pass.

## Register reconciliation
No finding.
