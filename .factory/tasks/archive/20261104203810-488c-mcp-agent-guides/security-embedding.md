# Embedded MCP help security review

Verdict: pass
Date: 2026-11-04
Reviewer and independence: `guides_review`, no implementation/test/manual authorship. Separate security pass by the same reviewer as the embedding quality pass; not two independent reviewers.
Reviewed candidate: uncommitted task branch based on `323a560ff37d036c34bd7984a5e38abed9036c6a`; final agent/generator/header/test identities are recorded in [quality-review-embedding.md](quality-review-embedding.md).

## Scope and trust boundaries

Reviewed the newly advertised initialization instructions, resources/list, resources/read and get_help; static generated documentation and its build lifecycle; relevant existing JSON decoding/serialization, HTTP admission and driver unload paths. Assets are server process availability, native bus authority and device data/configuration. External request strings are untrusted; the generated manuals are maintained repository content, not request-selected files. No third-party probes or hardware operations were performed.

## Risk assessment

- Files/path/network access: exactly 18 compiled entries are reachable. Decoded resource/topic strings are compared against fixed topic/URI metadata after optional fragment removal. No fopen/path resolution, shell call or outbound request is introduced at runtime. Traversal, external schemes, query aliases and unknown bases cannot select arbitrary content. Generator source paths are fixed build-time inputs, and generated output replacement is atomic.
- Input/injection: existing bounded UTF-8/JSON parsing applies to the added routes. Duplicate topic/URI and resource metadata validation is explicit; unknown fields and wrong types fail. Decoding rejects embedded NUL. Standard metadata is ignored as data, not executed. Help never dispatches native property writes and adds no script/file execution. Fragment support changes only lookup navigation, not the compiled whitelist.
- Output/portability: maintained UTF-8 is compiled as numeric byte arrays, with ASCII-escaped metadata. Existing JSON escaping is reused for instructions, titles, URIs and Markdown. Dynamic allocation failure paths return errors and release owned buffers; static help data lives for the module lifetime. The final arrays and full retrieval matrix preserve Unicode and JSON strings without embedding binary/BLOB payloads.
- Authentication/authorization: no new listener, authentication method or token inheritance is introduced. Existing enabled/admission/protocol gates remain in front of all routes. The four bus tools and their native authorization scope are unchanged. Read-only help does not imply permission for the operations it describes. Existing transport/authentication limitations remain documented rather than certified by this review.
- Resource/lifecycle behavior: immutable help requires no bus cache locks or selected science agent. No mutex/drain/unload order changes are introduced. Existing request concurrency/input limits and unload draining remain covered by regression tests. Help reads have bounded compiled content; no unbounded runtime file expansion is added. A full exhaustion/fault-injection audit was outside this increment.
- Secrets, logs, dependencies: no credentials/private data or new runtime dependencies in the help bundle. The Python generator is a maintainer/build tool, not a server runtime requirement. Help is explicit about native trace exposure, script/config/file side effects, BLOB URL fetches, and the difference between dispatch, state and completion. No new cryptographic claim.

## Evidence and findings

Reviewed primary MCP 2025-11-25 resource metadata and result contracts; protocol sources are linked in the quality report. The two concrete interoperability findings 003/004 were fixed and reverified, with canonical archived records linked there. Neither was a file-access or authorization bypass. No unresolved security finding in the changed embedding scope. Existing functional/semantic findings 001/002 remain open; embedding does not accept their risk or fix their behavior.

Independent checks: `python3 -B .factory/scripts/validate_project.py --security` passed with Python 3.9.6; generated-header freshness/UTF-8/link-whitelist checks and strict C99 syntax passed; final 34-case integration binary independently passed with isolated loopback permission. The configured Factory check validates local repository boundaries, not native application security. Sanitizer/build results and exact execution evidence are retained in [verification-embedding.md](verification-embedding.md); the reviewer inspected those results but did not independently rerun sanitizer. The suggested canonical indigo_docs/security.md reference remains absent; no absent reference is claimed to have been read.

## Scoped verdict

Pass for this additive immutable-help implementation, with no owner risk acceptance requested or inferred. No Linux/Windows execution, external client GUI acceptance, hardware safety, complete agent recipe correctness, allocation-fault injection or universal absence of vulnerabilities is claimed. The coordinator retains final delivery and follow-up ownership. Commit, publication and integration remain unauthorized by this review.
