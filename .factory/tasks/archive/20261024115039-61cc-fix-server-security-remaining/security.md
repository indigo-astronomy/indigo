# Scoped server security review

Verdict: pass
Reviewer: independent read-only subagent /root/transport_review, security-reviewer role explicitly loaded
Date: 2026-10-24
Reviewed revision: working diff against f967c0032969361164be91f97fbe1400646407fb; final content identities retained in process.json.
Independence: root implemented and executed tests; reviewer advised on contracts and independently inspected final source. No external provider/tool deployment or third-party probing.

## Boundaries and conclusions

Reviewed network input, cache ownership/permission transitions, recoverable allocation budgets, file descriptors, admission before asynchronous thread creation, failed-spawn rollback, idle protocol detection and established-session compatibility. No dependencies, secrets or authentication model changes. Public item/property pointers are not dereferenced by retained upload permission checks; private metadata is protected by the entry mutex. Registry-to-entry lock order matches existing retirement paths.

First pass requested changes for an AC-002 permission edge within existing finding 003. Final `sync_blob_permissions` covers both define APIs and all update states before snapshot/hidden/dont_update filtering. Permission transitions clear obsolete charged staging; publication and consumption recheck under the same lock. Reinspection passed after tests for define/targeted define/BUSY/ALERT/IDLE and writable restoration. No further concrete issue identified within scope; reviewer independently ran `git diff --check` successfully.

## Selected findings

- FND-20261023121227-a609-fix-server-security-high-002: parser-local retirement frees parser-owned cached RO payloads, leaving driver memory ownership intact. Repeated lifetimes plus macOS leak inspector pass.
- FND-20261023112118-8e5e-server-security-audit-018: finite item and wire budgets reject over-limit requests without dispatch; partial state releases. Upstream image contract preserved.
- FND-20261023112118-8e5e-server-security-audit-007: shared admission bounds threads, failed creation rolls back ownership/reservation, initial detection waits at most five seconds. Established astronomy idle sessions retained.
- FND-20261023112118-8e5e-server-security-audit-006: common cleanup closes opened resource handles after header/read/write/EOF failures; EOF cannot spin.
- FND-20261023112118-8e5e-server-security-audit-003: RO upload/publication denied, permission transitions revoke staging, matching writable operation consumes fresh staging once. This does not add client authentication to existing item identifiers.

## Executed evidence and limits

See verification.md for actual commands, 50 instrumented cases, 315 native unit cases, zero-leak inspection and 188 Factory regressions. Reviewers did not independently rerun those tests. Configured Factory security check passed; it validates local boundaries rather than certifying C code. Windows and Linux received source-level portability consideration, no runtime validation; no hardware/release/whole-server certification or owner risk acceptance is claimed. Unselected finding audit-010 and other registry work remain open. The full repository audit remains due under the owner's narrowly scoped deferral.
