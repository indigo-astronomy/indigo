# Security review

Verdict: changes required (one open high-severity, network-reachable memory-safety finding)
Reviewer: single agent, security-reviewer role (independence limited; same agent audited and reviewed)
Date: 21 Oct 2026 (Europe/Bratislava)
Revision and working-tree state: base/HEAD 48dc893491f77cddb18fc3188d400a70f67cde19; reviewed sources identical to base; only task evidence, findings register and coordinator state changed.
Review independence: reviewer also performed the audit; no separate implementer under the no-delegation constraint.

## Scope, assets, and trust boundaries
In scope this session: the network trust boundary of the INDIGO server — unauthenticated TCP clients reaching `indigo_libs/indigo_server_tcp.c` (XML/JSON/HTTP/WebSocket) and the wire-protocol parsers `indigo_xml.c` / `indigo_json.c`, plus the portable I/O layer `indigo_uni_io.c`. Primary asset: the long-lived server process and host memory safety. Tools and driver trust boundaries (local files, device protocols) are captured by inherited findings and were not re-triaged here.

## Risk coverage
| Area | Applicable? | Review evidence / reason not applicable |
| --- | --- | --- |
| Authentication and authorization | Partial | Server access-token gate reviewed in indigo_bus.c change dispatch; the WebSocket upgrade path runs before any token check, so FND-000045 is pre-auth. |
| Input, injection, and output handling | Yes | HTTP header parsing, WebSocket key handling and XML/JSON parsers inspected; FND-000045 confirmed; XML/JSON buffers bounded. |
| Data, secrets, cryptography, and logs | Partial | Bundled SHA1 and base64 reviewed for the handshake only; no secret handling defect found in scope. |
| Filesystem and outbound network access | Partial | `/blob/` and file-resource handlers reviewed; pointer parse validated via indigo_validate_blob; path copies bounded by snprintf. popen() uses in server/tools noted but not re-triaged (inherited TOOLS findings cover tool paths). |
| Dependencies and configuration | No | Vendored SDKs and build config excluded by scope. |
| Abuse and failure behavior | Partial | Allocation failures abort via indigo_safe_* (defined behavior); the handshake overflow is the confirmed abuse case. |

## Automated checks
`./.factory/scripts/security` → exit 0, Factory local security boundaries valid (baseline-security.log). This validates Factory boundaries only, not application memory safety. Targeted AddressSanitizer reproduction was run outside the repository (see below); no server was started and no network or device was contacted.

## Findings
- FND-000045 — category security, severity high, status open, blocking.
  - Location: `indigo_libs/indigo_server_tcp.c:129` (`char websocket_key[256]`), `:134` (`strncpy(websocket_key, header + 19, sizeof(websocket_key) - 1)`), `:152` (`strcat(websocket_key, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11")`).
  - Prerequisites: an unauthenticated TCP client sends an HTTP `GET /` with a `Sec-WebSocket-Key` header whose value is 220+ bytes. The worker reads the header line into `header[1024]` via `indigo_uni_read_line`, so values up to ~1004 bytes reach the copy.
  - Evidence/impact: `strncpy` fills up to 255 bytes and NUL-terminates; the subsequent unconditional 36-byte `strcat` then writes past the 256-byte stack buffer. Isolated ASan reproducer of the exact operations: control (24-byte value) exits 0; a 255-byte value aborts with `stack-buffer-overflow, WRITE of size 37`. The reproducer source repro-websocket-key.c is committed; its raw ASan and control logs are ignored by .gitignore and kept locally, and their outcome is recorded here. This is a remotely triggerable stack overflow in the default server; the handshake runs before any access-token check.
  - Remediation (for a separate repair task): bound the copy so the buffer always retains room for the 36-byte suffix plus NUL — e.g. copy at most `sizeof(websocket_key) - 37` bytes, or reject keys longer than the RFC 6455 24-byte base64 nonce — then reverify with the reproducer and a server integration test. Not fixed by this audit (out of write scope).
  - Status: open; owner decision required because the defect is network-reachable.

## Accepted risks
None. No agent self-acceptance. Only the owner may accept this risk, with identity, date, rationale, scope and follow-up.

## Verdict and limitations
Changes required. One confirmed pre-auth stack overflow (FND-000045). Limitations: this was a source-and-isolated-reproducer review, not a running-server penetration test; tool and driver trust boundaries were not re-triaged beyond inherited findings; no fuzzing was performed. A clean Factory scan does not complete this review.

## Register reconciliation
FND-000045 is registered open/high/blocking in .factory/findings/active.json (task 003-initial-repository-audit, security.md, null revision while the report lives in the checkout). Pin the report revision before any task-record pruning. No archive payloads are included here.
