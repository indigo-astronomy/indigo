# Transport security review

Reviewed source: `cb3c392e9f8833ddb6d29744a34cfae618b04fc8`, task branch `task/20261023112118-8e5e-server-security-audit`, working directory `/Users/polakop/Development/indigo`. Application source was unchanged during this pass. Reviewer: delegated `server_transport`, separate from the coordinator and other assigned surface reviewers. The coordinator requested the policy's strong `gpt-6-astra` / `high` configuration; actual inference settings are not independently exposed to this reviewer. This is a source-contract review, not an executed exploit, sanitizer result, or release certification.

Write scope: this report and `transport-coverage.json` only. No application, test, registry or Git modifications. Root instructions, `indigo_libs/AGENTS.md`, security-reviewer role, project/task context and findings routine were loaded. No scoped instructions exist under `indigo_server/`. `docs/security.md` is absent. The coordinator owns configured workflow/security checks and canonical finding allocation; T01–T07 below are report section labels, not competing registry IDs.

Verdict: **changes required**. Seven independently actionable defects are supported by current source contracts. Their exploitability prerequisites and unperformed validation are stated below. No residual risk is accepted.

## Boundary and coverage

The server binds all IPv4 interfaces (`indigo_uni_io.c:1617-1622`, Windows `1712-1717`). Deployment firewall/exposure is unknown. Socket bytes select XML, JSON or HTTP; a root HTTP request containing a WebSocket key selects the JSON WebSocket adapter. HTTP BLOB routes accept item addresses advertised in ordinary protocol messages, not cryptographic capabilities. Cache content can include images and uploaded application data. HTTP workers run concurrently with cache updates, device removal and resource cleanup.

Reviewed all 691 lines of `indigo_libs/indigo_server_tcp.c`, and the specific dependent cache, allocation, read, socket-accept and caller contracts listed in the coverage file. XML/JSON parser internals, device token configuration, management handlers and browser policy are assigned to the other reviewers. WebSocket framing itself is in `indigo_json.c` and was explicitly handed to the parser reviewer. Findings list contained 99 active summaries when inspected; relevant existing records were opened individually by a filtered active-record inspection. No archive was searched. The explicitly assigned `unfinished-defensive-boundaries.json` was read as an unapplied proposal and its transport claims checked against current source.

## T01 — Unvalidated BLOB lengths reach a process-fatal allocator

Severity: **high**. Confidence: **high**, source-contract confirmed.

Locations: `indigo_libs/indigo_server_tcp.c:355-370`; `indigo_libs/indigo/indigo_bus.h:1030-1037`.

Prerequisites: access to the listening service and a currently registered BLOB path. A normal client can learn paths through XML/JSON BLOB enumeration or updates (`indigo_xml.c:2090,2187`; `indigo_json.c:767,888`); pointer guessing is not required.

The handler converts `Content-Length` using `atoi`, does not check syntax, sign, overflow, duplicates or a configured maximum, and assigns it to both the shared cache size and the `size_t` argument of `indigo_safe_realloc`. A negative signed length becomes an enormous unsigned allocation request. On allocation failure the helper asserts, or calls `exit(0)` in assertions-disabled builds. The later `if (entry->content)` cannot recover from that helper's failure. A valid but very large positive length also causes an unbudgeted allocation. Missing/malformed length defaults to zero and zero-size `realloc` has implementation-dependent behavior, so a portable handler must define it explicitly rather than rely on the current helper.

Impact: remotely induced whole-server termination or severe memory pressure before a body is received. No claim of code execution. The source chain is sufficient to establish the fatal failure path; this review did not allocate a large buffer or terminate a running server.

Remediation: parse a bounded nonnegative decimal length with checked conversion; reject conflicting/duplicate framing and unsupported transfer encodings; enforce a practical per-upload and aggregate budget before allocation; use a recoverable allocation contract and return an HTTP error. Define zero-length uploads explicitly. The retained prior proposal is not a complete remedy: bounding at `INT_MAX` alone still permits excessive allocations.

## T02 — Incomplete BLOB uploads expose unpublished allocation contents

Severity: **high**. Confidence: **high** for publication of unwritten bytes; exact disclosed data depends on allocator history.

Locations: `indigo_libs/indigo_server_tcp.c:366-380,194-243`; `indigo_libs/indigo_uni_io.c:1916-1944`; `indigo_libs/indigo/indigo_bus.h:1030-1034`.

Prerequisites: a registered BLOB path and a body shorter than its positive declared size, or a read error. A following GET reaches the same cache before a legitimate producer overwrites it.

The shared `entry->size` and reallocated `entry->content` are changed before reading. `indigo_uni_read` returns a positive short count on EOF and `-1` on a read error. The HTTP handler tests only logical negation, so both positive short counts and `-1` enter its success path and return 200. Even the zero-byte failure path leaves the changed allocation and declared size in the cache: common failure cleanup merely releases the mutex. `realloc` does not initialize newly allocated bytes. GET subsequently copies and transmits the full declared cache size. Therefore the unwritten suffix can contain residual heap data, or old BLOB bytes when an allocation is reused. A fresh allocation need not contain sensitive data for the bug to exist; no particular secret or memory layout was observed or assumed.

Impact: potential process-memory disclosure plus corrupted uploaded/downloaded content. The failure is independent of excessive-length handling: a small valid positive length suffices. Source-contract evidence is the exact return convention and the missing equality check/rollback; no real data was accessed.

Remediation: receive into bounded private storage, require the exact byte count, and atomically publish size/content only after complete success. On every failure preserve the previous cache or invalidate it consistently. A return-value check alone is insufficient if changed cache state remains visible after failure.

## T03 — BLOB PUT violates the read-only cache contract

Severity: **medium**. Confidence: **high** for the read-only cache integrity violation; downstream effects depend on consumers.

Locations: `indigo_libs/indigo_server_tcp.c:355-380`; `indigo_libs/indigo_bus.c:1084-1119,1477-1484,836-854`; `indigo_libs/indigo_xml.c:420-429`.

Prerequisites: access to the service and a registered BLOB identifier, available through ordinary property publication. RO entries are explicitly cached by `indigo_update_property`.

`indigo_validate_blob` checks item-pointer membership only. PUT does not check the cached property's permission, device access token, upload ownership or session identity. It directly replaces any matching cache entry, including an `INDIGO_RO_PERM` image, without calling the token-enforcing `indigo_change_property`. A subsequent GET returns attacker-supplied cache bytes instead of the device's output. For WO uploads, the staging cache is shared between clients; the later XML change handler copies the cache and only then dispatches the property through normal authorization. A token-protected final property update does not authenticate who produced those cached bytes.

Impact: replacement of RO output and interference with another client's staged upload. The principal defect is violation of the RO property/cache integrity contract. `indigo_docs/INDIGO_DEVICE_ACCESS_CONTROL_AND_LOCKING.md` explicitly says tokens/locking are not security features, all clients may monitor values, and a master token must be configured for protection to apply. Thus this report does not claim a confidential-read boundary or an authentication bypass merely from public BLOB retrieval. With configured device and master tokens, cache mutation also bypasses the intended control-coordination check, but no unauthorized hardware command execution is established. The access-control reviewer was notified to avoid duplicate registration.

Remediation: reject uploads to non-writable BLOBs and bind writable upload staging to the intended operation/client so one upload cannot replace another client's staged data. Preserve the documented token-coordination checks for final changes, and define whether staging itself must honor them. Do not treat the advertised address as a credential or introduce a claimed confidentiality boundary without a separate design decision.

## T04 — BLOB lookup and deletion do not preserve entry/item lifetime

Severity: **high**. Confidence: **high** for invalid ownership contracts; runtime race timing and resulting symptoms were not reproduced.

Locations: `indigo_libs/indigo_bus.c:1430-1484,954-977`; `indigo_libs/indigo_server_tcp.c:197-202,227-231,358-366`.

Prerequisites: a BLOB request overlapping property release/clear, or an old WO BLOB path used after its property is released. Device removal/reconfiguration is a normal lifecycle event; no permission to initiate it is assumed.

RO deletion removes and frees entries under `blob_mutex`, but `indigo_validate_blob` neither holds that mutex nor pins the entry. A worker can read a pointer, lose the race to release, and then lock/dereference freed storage. Holding the entry mutex after lookup does not close that gap. Buffered GET also unlocks the entry and subsequently reads `entry->format` at line 231, even though it already copied the format locally. Separately, WO definitions create entries in the same registry, but `release_dependencies` frees only the WO item's value and does not remove its registry entry before the property is freed. A stale path therefore continues to pass validation; a zero-size GET copies the freed item at line 202. The latter is not merely a narrow race.

Impact: use-after-free/data race, potential server crash or unintended data access during ordinary device lifecycle; no code-execution claim. WO cache slots/content also remain retained. These are related lifecycle failures requiring an acquisition/release contract, not proof that any arbitrary address passes validation.

Remediation: acquire a live entry under the registry lock, retain/refcount it until the request finishes, and consistently retire entries for all permissions before property memory is released. Do not use entry/item/property fields after releasing their lifetime protection. Copy all response metadata while protected. The XML upload consumer also calls `indigo_find_blob` and copies `entry->size/content` without the entry mutex (`indigo_xml.c:421-425`), allowing a concurrent PUT reallocation to invalidate its source; the parser reviewer independently identified this same ownership boundary. Include that consumer in the fix. Add controlled lifecycle/concurrent-reader regressions under a sanitizer in the repair task.

## T05 — A stalled BLOB body can block bus-wide publication

Severity: **high**. Confidence: **high**, source lock/deadline chain confirmed; no indefinite-blocking test was run.

Locations: `indigo_libs/indigo_server_tcp.c:366-375`; `indigo_libs/indigo_uni_io.c:1668-1675,1753-1760,1916-1935`; `indigo_libs/indigo_bus.c:1026-1030,1084-1119`.

Prerequisites: a registered RO BLOB that a device will update, access to PUT its path, and an upload whose body remains unfinished while the connection stays open.

PUT locks `entry->mutext` before the body read. Accepted sockets explicitly have an unlimited receive timeout. The body reader waits for all bytes or disconnect/error, without a total deadline. A producer updating that RO image takes global `client_mutex`, then `blob_mutex`, and blocks on the same entry mutex. Consequently unrelated property publication/client operations needing the global client mutex stop too. This is more than one occupied download worker. The ability to PUT an RO cache is T03; even after rejecting RO writes, holding shared cache locks over untrusted I/O remains an unsafe contract for other consumers.

Impact: sustained service-wide availability loss from a stalled upload, without sending a large body. No claim that every driver/hardware operation stops immediately; the demonstrated affected boundary is bus publication/client locking.

Remediation: receive into private bounded storage outside shared cache locks, enforce body progress and absolute deadlines, then take the cache lock briefly to publish after authorization and exact-length validation. Ensure disconnect/error releases both temporary allocation and any retained entry reference.

## T06 — File-download errors leak opened file handles

Severity: **medium**. Confidence: **high**, source cleanup path confirmed.

Locations: `indigo_libs/indigo_server_tcp.c:312-345,426-436`; maintained caller `indigo_server/indigo_server.c:1936-1938`.

Prerequisites: a registered file resource and a client write failure after the resource file is opened. The shipped `/log` registration is conditional on `RPI_MANAGEMENT`; this is not an unconditional endpoint in every build.

After `indigo_uni_open_file`, any `INDIGO_PRINTF` failure or the body write failure jumps to function-wide `failure`. The file handle is local to the resource branch; common cleanup closes only the client handle and does not close this file. Its sole close is on the ordinary branch completion. Repeated interrupted downloads thus retain descriptors and handle allocations. No early-disconnect network check was run.

Impact: descriptor exhaustion and failure to open further files/sockets in builds using file resources. Remediation: give the opened file a single cleanup owner reachable on every exit, or close it before every failure jump. Test a bounded injected output-write failure and assert resource-count recovery.

## T07 — Pre-protocol connections retain unbounded workers without a deadline

Severity: **medium**. Confidence: **high** for missing bounds; exhaustion threshold is environment dependent and unmeasured.

Locations: `indigo_libs/indigo_server_tcp.c:95-100`; `indigo_libs/indigo_uni_io.c:1657-1697,1742-1781,1888-1907`; `indigo_libs/indigo_bus.c:2297-2303`.

Prerequisites: ability to establish TCP connections. No BLOB, driver or token is needed.

Each accepted socket receives a detached native thread before any protocol input. Its receive timeout is zero (unlimited), and the initial `MSG_PEEK` blocks without a deadline. There is no admission limit in the accept loop or HTTP worker; `client_count` is an unsynchronized informational count, not a gate. The bus's protocol-client table limit is later and cannot protect silent pre-protocol connections. If `indigo_async` eventually fails, the accept loop logs and retains the accepted socket, worker_data and handle instead of cleaning them up.

Impact: cumulative thread/socket/memory exhaustion from idle connections, with additional permanent leaks when thread creation fails. This review did not run a connection flood or measure capacity.

Remediation: bound admission before starting workers, time-limit initial protocol detection and HTTP request/header progress, and close/free all resources when thread creation fails. Preserve intentional long-lived protocol sessions after successful establishment, with appropriate bounded session policy.

## Existing findings, rejected leads and limits

- Existing gzip-capacity defect is `FND-20261023065605-60d4-repository-audit-009`. Lines 212-223 still allocate input-sized gzip output and ignore completion. No new duplicate and no sanitizer rerun.
- Existing SHA-1 signed-shift issue is `FND-000046`, still present at lines 665, 673 and 675. SHA-1's use here is the mandated WebSocket handshake transform, not evidence of a password/hash design flaw. No duplicate.
- The previously repaired WebSocket key overflow is not reintroduced: accepted key length is at most 219 bytes, plus 36-byte GUID and NUL fits 256 bytes.
- Static file-resource routing does not concatenate a request suffix into a filesystem path; `file_name` comes from registration. `/log/../...` matching the `/log` prefix does not alone provide directory traversal. Handler-specific paths remain the access/UI reviewer's scope.
- An arbitrary `/blob/` address is compared against registered pointers before dereference. No arbitrary-address access is claimed independently of T04's stale/live-entry ownership defects.
- Resource-list lookup releases its mutex before using a resource and deletion frees nodes. The maintained server removes all resources during shutdown; a concurrent shutdown/request lifetime issue remains possible, but no additional remote-trigger finding is allocated without establishing the intended worker-drain contract and reachable lifecycle. This is a follow-up review lead, not silently accepted risk.
- HTTP framing accepts truncated lines, unchecked duplicate lengths and unsupported transfer encodings. T01 records the actionable BLOB length consequence. No reverse-proxy request-smuggling claim is made without a concrete proxy/parser boundary. General RFC noncompliance alone is not treated as an exploit.
- Header fields derived from BLOB format are emitted without HTTP-specific encoding. This pass did not establish an independently authorized untrusted producer/control path for a response-splitting finding; parser/upstream data validation remains follow-up.

## Evidence and unperformed checks

Executed read-only commands included `git status --short`, `git rev-parse HEAD`, `wc -l indigo_libs/indigo_server_tcp.c`, `sed -n '1,260p' indigo_libs/indigo_server_tcp.c`, `nl -ba indigo_libs/indigo_server_tcp.c | sed -n '250,700p'`, numbered bounded reads of each dependency above, `./.factory/scripts/findings list`, filtered standard-library reads of `.factory/findings/active/*.json`, `git ls-files` for test/security-document inventory, `find indigo_server -name AGENTS.md -print`, and `python3 --version` (3.9.6). No `rg`, vendor traversal, prohibited-driver inspection, archived-record search, hardware, external service or live server was used. The sole archived input was the exact coordinator-assigned unapplied proposal.

The findings are proven to the source-contract level stated in each section. No compiler, sanitizer, unit/integration suite, HTTP exchange, race reproducer or allocation-failure injection was run by this reviewer. Existing test inventory includes `indigo_test/integration/test_server_websocket.c` and `test_server_driver_stress.c`; those were not executed or counted as passing. `.factory/checks.json` configures a Factory security validator, not an application vulnerability scanner; its execution is assigned to the coordinator. Report JSON/whitespace checks do not validate application behavior. Final report checks parsed `transport-coverage.json`, confirmed all reviewed application blobs match HEAD, and found no trailing whitespace or missing final newline in either report.

Next assignment: coordinator independently reconcile these sections with parser/access findings, allocate canonical IDs, retain report provenance, and schedule focused fixes with synthetic regression checks. The audit identifies required changes and does not claim they are implemented or risk-accepted.
