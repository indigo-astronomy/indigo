# Security review

Verdict: pass
Reviewer: coordinator (Claude Code, claude-opus-5-5)
Date: 2026-10-26
Revision and working-tree state: c13c69594
Review independence: same agent as coordinator; the independent quality reviewer also checked memory-safety aspects of the library changes.

## Scope, assets, and trust boundaries
Device replies (serial/USB) entering drivers, image files read by the file camera, HTTP BLOB responses, client-supplied SESSION_FEATURES and server DRIVERS/UNLOAD requests, and library buffers.

## Risk coverage
| Area | Applicable? | Review evidence / reason not applicable |
| --- | --- | --- |
| Authentication and authorization | No | No ACL or auth code changed; static UNLOAD keeps the existing request path |
| Input, injection, and output handling | Yes | Device-reported wheel counts validated against allocation (bc8d-023); exact read lengths for RAW files (3bd2-020) and DSI planes (bc8d-044); falcon ack validation; registry capacity `>=` and slave query bound (bc8d-035); Bayer edge reads in-bounds (bc8d-010); PSF map bound (bc8d-007); firmware simulator terminators |
| Data, secrets, cryptography, and logs | No | No secrets or logging of sensitive data added |
| Filesystem and outbound network access | Yes | Incomplete local image saves are removed and ALERTed (bc8d-004); HTTP gzip partial stream no longer served (60d4-009) |
| Dependencies and configuration | No | Only existing zlib is linked additionally into unit tests |
| Data model and process policy | Yes | MIGRATION_STATUS.md: only the hardware-free test count column of changed drivers was raised by the number of added cases; the Comment column is untouched. indigo_test/AGENTS.md: one line documenting the new `test-indigo-js-formatting` target; no rule or permission changed |
| Abuse and failure behavior | Yes | Bounded deadlines for endless device replies (bc8d-024, 3bd2-018, bc8d-025, bc8d-017/018); legacy printf over-read (bc8d-016); HID close of an aliased descriptor (bc8d-003) |

## Automated checks
`make -C indigo_test test-server-security-sanitize` and `test-raw-image-sanitize` (AddressSanitizer/UBSan), exit 0, macOS arm64; see verification.md.

## Findings
No new security finding. The out-of-bounds read/write findings in scope are fixed and covered by the sanitizer runs or unit regressions listed in verification.md.

## Accepted risks
None recorded.

## Verdict and limitations
Scoped to this diff; no whole-server or hardware security certification. Windows/Linux not built.

## Register reconciliation
Repaired findings are closed with verification.md as evidence; no security finding remains open from this task.
