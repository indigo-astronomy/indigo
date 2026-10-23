# Security review

Status: focused semantic inspection complete; final synchronized candidate pending.

Reviewer: same Codex session as implementation, separate security-reviewer pass; exact model/effort unavailable under owner-selected host, no independent/cross-provider review claimed. Base master 3b48146ca with uncommitted repair. Scope is caller-supplied RAW bytes and metadata crossing the public indigo_raw_to_fits memory boundary, used by the command-line converter. No network, authentication, authorization, credential, path, dependency or deployment changes.

Inspected short-header rejection before memcpy/signature access, nonzero dimensions and division-first full-component payload validation before multiplication, allocation/card arithmetic bounded by int output ABI, complete record-aligned temporary header and final data offset, bytewise 16-bit output with memcpy input to avoid alignment UB, and output retention on allocation failure. All fallible parsing/header assembly occurs before output realloc; no error return follows output mutation. Local synthetic ASan/UBSan tests and five allocator failure sites are evidenced in verification.md. No new production fault hooks.

Embedded parsing uses memchr with remaining-length bounds; values/names are bounded before formatting. Invalid recognized extension fields/cards fail rather than writing past a card. Caller-owned keyword arrays still require a sentinel and valid C string pointers, as the unchanged C API does not provide lengths. Input/output aliasing is explicitly excluded; prior realloc-based implementation did not support it. Semantic FITS reserved-keyword policy, quote escaping and comprehensive FITS schema validation are not newly claimed by this memory boundary repair.

Configured structural security check passes; exact command/result and coverage limits are in verification.md/security-check.log. No newly confirmed security finding in scoped review, no risk acceptance asserted. Owner's audit deferral permits only ordered repair work and does not certify unrelated code. Reinspect synchronized changes and final identities before final verdict.
