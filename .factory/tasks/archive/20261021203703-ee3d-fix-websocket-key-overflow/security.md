# Security review

Verdict: pass
Reviewer: single agent, security-reviewer role (same agent implemented the fix; independence limited, disclosed)
Date: 21 Oct 2026 (Europe/Bratislava)
Revision and working-tree state: task/20261021203703-ee3d-fix-websocket-key-overflow on f1a28a893 plus the uncommitted task diff described in verification.md.
Review independence: no separate reviewer under the no-delegation constraint.

## Scope, assets, and trust boundaries
The unauthenticated HTTP handshake in indigo_libs/indigo_server_tcp.c, reached by any TCP client of the INDIGO server before any access-token check. Asset: integrity and availability of the server process.

## Risk coverage
| Area | Applicable? | Review evidence |
| --- | --- | --- |
| Authentication and authorization | No change | The handshake still runs before token checks; the fix removes the memory-safety exposure on that path. |
| Input, injection, and output handling | Yes | The key length is checked against the buffer before any copy; over-long keys get a fixed 400 reply that echoes no client data. |
| Data, secrets, cryptography, and logs | Partial | SHA-1/base64 unchanged; the trace message names no client data. Pre-existing SHA-1 shift UB registered as FND-000046. |
| Filesystem and outbound network access | No | Not touched. |
| Dependencies and configuration | No | No dependency or configuration change. |
| Abuse and failure behavior | Yes | A refused client is disconnected and the server keeps serving, as the regression test shows. |

## Automated checks
AddressSanitizer and UBSan build of the real server source (test-server-websocket-sanitize): no memory error after the fix; stack-buffer-overflow at indigo_server_tcp.c:152 with the base source. The release build crashed with SIGTRAP before the fix. Details in verification.md.

## Findings
- FND-000045, security, high: fixed. The key is now copied only when strlen(key) < sizeof(websocket_key) - strlen(WEBSOCKET_GUID), so the key, the 36-byte GUID and the NUL always fit in 256 bytes. Longer keys get `400 Bad Request` and the connection is closed. Retest evidence is in verification.md.
- FND-000046, functional, low, open: signed shifts in the bundled SHA-1. Not a memory-safety issue; deferred.

## Accepted risks
None.

## Verdict and limitations
Pass for FND-000045. Limitations: verified on macOS arm64 only; no fuzzing of the remaining HTTP handlers, which were outside this task.

## Register reconciliation
FND-000045 archived with resolution fixed and both task references. FND-000046 open, nonblocking.
