# Repository review report

- Scope: full maintained repository (initial audit)
- Task and date: 20261021204508-b596-initial-repository-audit, 21 Oct 2026 (Europe/Bratislava)
- Reviewer and independence: single agent acting sequentially as coordinator, repository-reviewer, security-reviewer and quality-reviewer. Independent implementation review was unavailable (no delegation authorized); the same agent produced the evidence and reviewed it. No production source, README, SDK or generated output was changed by this audit.
- Revision and uncommitted state: base and HEAD 48dc893491f77cddb18fc3188d400a70f67cde19. Working tree at review time: `.factory/state.json` modified, `.factory/findings/active.json` modified (FND-000045 registration), untracked `.factory/tasks/003-initial-repository-audit/`. No tracked product file differs from the base; the reviewed framework/server/protocol/I-O sources are byte-identical to the base.
- Verdict: pass
- Verdict rationale: The blocking finding FND-000045 was fixed by task 004; the remaining findings are nonblocking and tracked. The first verdict on 21 Oct 2026 was "changes required" while FND-000045 was open.

## Reviewed areas and evidence

Inventory and classification (AC-001): 6,558 tracked paths classified in inventory.tsv / inventory-summary.json. Exclusions evidenced there: 4,058 vendor, 348 generated siblings (inspect the `.driver` contract instead), 107 binary/data assets, 45 legacy/nonmaintained, 44 owner-protected driver files (ccd_apogee, ccd_sbig, mount_asi, mount_mxhd, system_ascol), 30 task/findings archive, 5 bundled UI deps, 3 catalogue data. 1,918 paths are maintained and in scope. Classification is not semantic review.

Framework public-API, ownership, I/O, protocol and image/resource boundaries (AC-002), inspected this session at the base revision:
- Core bus `indigo_libs/indigo_bus.c`: attach/detach generation tracking, detach-abort registry, BLOB cache registry, property init/resize/copy/clear lifecycle, change/update/delete/enumerate dispatch and the recursive bus mutex. No new defect confirmed beyond inherited LIB-013 (FND-000013, device visible before `attach()` completes).
- TCP/HTTP/WebSocket server `indigo_libs/indigo_server_tcp.c`: worker protocol detection (XML/JSON/HTTP), `/`, `/blob/`, resource and file handlers, BLOB GET/PUT, bundled SHA1. Confirmed finding FND-000045 (see Findings). BLOB `%p` pointer parse is validated through `indigo_validate_blob()`; `indigo_safe_malloc`/`indigo_safe_realloc` abort on failure, so unchecked-allocation patterns in this file are not independent defects.
- Wire protocols `indigo_libs/indigo_xml.c` and `indigo_libs/indigo_json.c`: parser state machines, BLOB base64 decode, device-suffix handling, escape routines and the WebSocket framing read/write. The XML parser bounds name/value/blob buffers (`BUFFER_SIZE`, `INDIGO_NAME_SIZE`) and the JSON parser bounds name/value against `INDIGO_NAME_SIZE`/`JSON_BUFFER_SIZE`; no new overflow confirmed. `ws_read` rejects frames larger than the caller buffer (`errno = ENODATA`).
- Portable I/O `indigo_libs/indigo_uni_io.c`: serial open error mapping, delimited reader, socket/file read loops. Behaviour is correct; one unit expectation is platform-fragile (see verification.md, not a code defect).
- Boundary candidate sweep (boundary-candidates.txt): every `sprintf`/`strcat`/`sscanf` in `indigo_libs`, `indigo_server` and `indigo_tools`. Framework uses are bounded or format-controlled; the `indigo_tools` fixed-buffer parsers are already captured by inherited TOOLS-001..008 (FND-000036..000043).

Tools, examples, server UI, platform/optional drivers, portable drivers/agents, tests, build/CI and documentation (AC-002): covered by the systematic review passes migrated during adoption (legacy baselines in tasks/archive/20261021193138-70b4-adopt-ai-factory/legacy-findings.md), which produced the 43 inherited findings spanning docs (FND-000001..000005), guider/filter (FND-000006, FND-000007), examples (FND-000008..000012), core/linux/optional drivers (FND-000013..000027), tests (FND-000028..000035) and tools (FND-000036..000043). This session reconciled those against current source paths; none was contradicted by the current tree.

Safe baselines (AC-004), evidence in verification.md: Factory check/security/test and build-system suites pass; the unit subset fails one platform-fragile case (FND-000047, corrected in task 004) and skips the RAW image suite; the full native unit target cannot link because `build/drivers/indigo_system_alpaca.a` is absent (environment limitation, not a defect). No driver/hardware mode was inferred or run.

## AI slop inspection

No unintegrated generated scaffolding, fabricated API, generic filler or unsupported completion claim was confirmed in the reviewed framework/server/protocol/I-O scope. Generated driver `.c`/`.h`/`_main.c` outputs were treated as owned by their `.driver` sources and not re-flagged for style. Authorship was not inferred.

## Findings

- FND-000045 (category security, severity high, blocking): `indigo_libs/indigo_server_tcp.c:129/134/152`. Evidence: the HTTP WebSocket upgrade handler `strncpy`s the attacker-controlled `Sec-WebSocket-Key` value into `websocket_key[256]` (up to 255 bytes) and then unconditionally `strcat`s the 36-byte magic GUID; a header value of 220+ bytes overflows the stack buffer. The value reaches this path from an unauthenticated TCP client (header read into `header[1024]` via `indigo_uni_read_line`). Impact: remotely triggerable stack-buffer-overflow in the default server. Confidence: confirmed by an isolated ASan reproducer of the exact copy/append operations (control exit 0; 255-byte value aborts, WRITE of size 37). Smallest remediation: cap the copied length so the buffer always leaves room for the 36-byte suffix plus NUL (or reject over-length keys). Disposition: fixed by task 20261021203703-ee3d-fix-websocket-key-overflow and archived; see security.md.

Inherited findings FND-000001..000044 were reconciled, not reopened; FND-000044 is archived (FTDI DMG, fixed in task 002) and stays closed. No inherited finding was silently closed or duplicated by this audit.

Questions (not confirmed findings): none outstanding from the framework/server/protocol scope.

## Fix verification

No fix was applied by this audit. Baseline and reproducer evidence is in verification.md (checks) and security.md (FND-000045 reproduction). Repairs for FND-000045 and the inherited findings are deferred to scoped tasks and are not verified here.

## Cadence decision

This is the owner-deferred initial full audit (deferral recorded in maintenance.json). Coverage this session is the framework/server/protocol/portable-I-O boundary review plus baseline validation; combined with the adoption-migrated component review passes it spans every mapped maintained component class. The coordinator's checkpoint decision is recorded in handoff.md and maintenance.json. Because the audit did not independently re-inspect all 1,918 maintained files line-by-line this session and relies in part on inherited component passes, coverage is recorded as achieved-with-documented-limitations; the recurring checkpoint is advanced only per the coordinator entry in maintenance.json.

Owner decision, 21 Oct 2026: Peter Polakovic wrote "I accept the documented coverage of task 003 as the initial-audit gate." after FND-000045 was fixed by task 004. The full-review fields in maintenance.json now record this audit at revision 48dc893491f77cddb18fc3188d400a70f67cde19. Task 004 completed after that snapshot and is counted toward the next audit.

## Verdict and handoff

Pass. FND-000045, the only blocking finding, was fixed and reverified by task 004. Task 004 also registered FND-000046 and FND-000047; all open findings are nonblocking and keep their owners and next actions in the register. Limitations: no hardware or driver-mode runs; no full native unit link (missing alpaca archive); one platform-specific unit failure (FND-000047) that also kept the RAW image suite from running; semantic re-inspection of the full driver corpus this session was not performed and leans on inherited passes. Next assignment is in handoff.md.

## Register reconciliation

Canonical IDs: inherited FND-000001..000043 (open, legacy aliases DOCS/DRV/EXAMPLES/LIB/LINUXDRV/OPTDRV/TEST(S)/TOOLS preserved in their summaries), FND-000044 (archived, fixed), FND-000045 (found by this task, fixed and archived by task 004), FND-000046 and FND-000047 (registered by task 004, open). next_id is 48. Ownership/status live in .factory/findings/active.json; this report does not copy registry payloads. `./.factory/scripts/findings check`: 45 active, 2 archived, task evidence valid.
