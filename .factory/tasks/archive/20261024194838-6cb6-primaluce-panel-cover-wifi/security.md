# Security review
Verdict: pass
Reviewer: security reviewer pass, same agent as the developer (Claude Code, claude-opus-5-5)
Date: 2026-10-24
Revision and working-tree state: `task/20261024194838-6cb6-primaluce-panel-cover-wifi` at `1a0252d5e` plus the uncommitted task diff
Review independence: not independent; a separate pass over the final diff.

## Scope, assets, and trust boundaries
Client-supplied WiFi credentials sent to the device; device replies; the device's network reachability.

## Risk coverage
| Area | Applicable? | Review evidence / reason not applicable |
| --- | --- | --- |
| Authentication and authorization | No | INDIGO has no per-property authorization; unchanged. |
| Input, injection, and output handling | Yes | Credentials are inserted unescaped into the JSON request; `"` and `\` are now refused before sending, so a client cannot inject JSON members. Lengths are bounded (32/63). Messages from the device are published with `"%s"`. |
| Data, secrets, cryptography, and logs | Yes | Passwords are shown in clear text in `X_WIFI_AP`/`X_WIFI_STA`, as the focuser already did and as the device's own web page does; they are not logged by the driver. Not saved by CONFIG. |
| Filesystem and outbound network access | No | None. |
| Dependencies and configuration | No | None. |
| Data model (MIGRATION_STATUS.md, PROPERTIES.md) | Yes | PROPERTIES.md documents the three properties for all three drivers. MIGRATION_STATUS.md unchanged (no migration status change). |
| Abuse and failure behavior | Yes | A WPA2 password the access point cannot use is refused, so the panel cannot be left with an unusable access point by a typo; a refused mode restarts nothing; the ALTO refuses a restart during a move or calibration. |

## Automated checks
`./.factory/scripts/security`; see verification.md.

## Findings
None registered.

## Accepted risks
None recorded.

## Verdict and limitations
Pass. The HTTP path was reviewed, not run.

## Register reconciliation
No finding.
