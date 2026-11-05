# Final MCP help and automatic lifecycle security review

Verdict: pass
Date: 2026-11-05
Reviewer: delivery_review, fresh non-author subagent; separate security pass by the same reviewer as quality-review.md. Reviewed revision, source identities, scope and independence limits are recorded there. Earlier security-drafts.md, security-embedding.md and security-autostart.md remain preserved.

## Boundaries and assessment

Reviewed the final added help routes, existing request parsing/decoding/serialization helpers, generated static help, HTTP admission and load/unload lifetime, and documented native bus scope. External request strings are untrusted. The help source is maintained repository content compiled at build time; it is not a request-selected filesystem resource. No third-party probes, live user server or hardware operations were performed. The referenced indigo_docs/security.md remains absent and is not claimed as an inspected input.

Decoded topics/URIs compare only against18 fixed entries after fragment removal. There is no request-derived file open, shell command, path resolution or outbound fetch. Unknown bases, traversal, queries and external schemes cannot select arbitrary content. JSON validation rejects duplicate/wrong-type topic and URI fields, duplicate/nonobject resource metadata and unknown fields. Embedded NUL decoding is rejected by the existing decoder. Standard metadata is ignored as data. Output uses existing JSON escaping; failed allocations produce errors and release owned buffers. The immutable arrays avoid compiler encoding and string-literal portability issues.

The owner-authorized service-on-load change removes an explicit toggle, not an authentication check. Existing listener/native-token scope, Origin rejection, bounded input and eight-request admission remain. Admission starts after attachment; shutdown disables it before route drain/cache destruction, while same-stack self-unload returns BUSY before disabling the active service. Help adds no native property dispatch or script execution. Existing readable-token/native trace and unauthenticated-listener limitations remain documented; this review does not certify those established contracts as authentication.

No credentials, sensitive runtime data, new runtime dependency or cryptographic mechanism were introduced. Fixed generator inputs are local maintenance/build data; generated replacement is atomic. Manual prose distinguishes dispatch from completion, stale observations from fresh results, and documentation from executable Sequencer bootstrap. Known recipe gaps remain explicit.

## Evidence and disposition

Independent final freshness, generated UTF-8/NUL/206-link whitelist, whitespace, Factory security-boundary, Xcode syntax and Windows XML checks passed; exact commands are in quality-review.md. Python3.9.6 is the local security-validator runtime. Factory validation covers repository boundaries, not complete application security. Existing native34/34 and sanitizer34/34 restoration evidence was reviewed; this pass does not claim an independent native/sanitizer execution. Coordinator final-run evidence remains authoritative for subsequent recorded runs.

No new security finding in this final delta. Fixed findings003/004 retain their regression coverage. Open semantic findings001/002 remain separately tracked; no owner risk acceptance was inferred. Remote master additions are outside these runtime boundaries and do not overlap MCP sources.

Pass is limited to this offline-help and automatic-service feature with its documented existing authority. No universal vulnerability absence, hardware safety, hostile-load/fault injection, Linux/Windows execution or GUI/model workflow acceptance is claimed. Recurring repository audit, process gates, archival and delivery remain coordinator responsibilities.
