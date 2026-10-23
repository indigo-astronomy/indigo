# Security review

Verdict: pass

Reviewer: same Codex session as implementation, separate security-reviewer pass; exact model/effort unavailable under owner-selected host, no independent/cross-provider review claimed. Base master 3b48146ca with uncommitted repair. Scope is caller-supplied RAW bytes and metadata crossing the public indigo_raw_to_fits memory boundary, used by the command-line converter. No network, authentication, authorization, credential, path, dependency or deployment changes.

Inspected short-header rejection before memcpy/signature access, nonzero dimensions and division-first full-component payload validation before multiplication, allocation/card arithmetic bounded by int output ABI, complete record-aligned temporary header and final data offset, bytewise 16-bit output with memcpy input to avoid alignment UB, and output retention on allocation failure. All fallible parsing/header assembly occurs before output realloc; no error return follows output mutation. Local synthetic ASan/UBSan tests and five allocator failure sites are evidenced in verification.md. No new production fault hooks.

Embedded parsing uses memchr with remaining-length bounds; values/names are bounded before formatting. Invalid recognized extension fields/cards fail rather than writing past a card. Caller-owned keyword arrays still require a sentinel and valid C string pointers, as the unchanged C API does not provide lengths. Input/output aliasing is explicitly excluded; prior realloc-based implementation did not support it. Semantic FITS reserved-keyword policy, quote escaping and comprehensive FITS schema validation are not newly claimed by this memory boundary repair.

Configured structural security check passes; exact command/result and coverage limits are in verification.md/security-check.log. No newly confirmed security finding in scoped review, no risk acceptance asserted. Owner's audit deferral permits only ordered repair work and does not certify unrelated code. Reinspect synchronized changes and final identities before final verdict.

## Final sign-off

Date: 2026-10-23. Reviewed target a1beecfdb8ad4e71af9df6670ef7b047e72ab107 plus final candidate content recorded in process.json. Reinspected arithmetic/pointer/card bounds and all allocation failure exits after synchronization; no production overlap with predecessor fixes. The source no longer uses unbounded sprintf, an aligned RAW header/pixel cast or unbounded extension strlen/copy. The generic numeric locale fix is applied only to a terminated temporary numeric buffer, avoiding an unterminated header scan. Temporary private header assembly precedes all caller output mutations.

All relevant local synthetic boundary/sanitizer/fault/consumer checks pass; see verification.md. Scoped defect 005 is repaired and retested; no new confirmed security finding or owner risk acceptance. No third-party probing or network/hardware activity. No authentication, secret, dependency or deployment change. Maintenance changes retain explicit human deferral authority, not agent audit acceptance. Archival/navigation/manifest checks preserve evidence and introduce no execution hooks. No whole-FITS schema, whole-server/repository security, fresh independent model or platform certification claimed.
