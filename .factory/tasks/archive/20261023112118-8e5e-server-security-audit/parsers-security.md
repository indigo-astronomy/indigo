# Parser and protocol-adapter security review

Reviewed revision: `cb3c392e9f8833ddb6d29744a34cfae618b04fc8`, task `20261023112118-8e5e-server-security-audit`. Reviewer: delegated `server_parsers`, assigned security-reviewer, independent of product implementation. Strong review settings were requested by the coordinator (`gpt-6-astra/high`); provider telemetry and task cost are unavailable. Product source remained unchanged. Only this report and `parsers-coverage.json` were written by this reviewer.

Verdict: **changes required**. The following findings are confirmed by current source and public allocation/I/O contracts, not by execution against a running server. Aliases P01–P08 are reviewer-local; the coordinator assigns canonical findings. No risk is accepted here.

## Scope and trust boundaries

Reviewed XML/JSON parser dispatch, token buffers, property item allocation, XML BLOB decode ownership, WebSocket frame reads, serializer escaping and adapter lifecycle. Inbound TCP XML/JSON is selected before property authorization (`indigo_server_tcp.c:100–114`); WebSocket JSON enters the same parser. Outbound remote-server connections call `indigo_xml_parse(device, NULL)` (`indigo_client.c:454–478`), whereas inbound clients call `indigo_xml_parse(NULL, client)`. Remote server metadata, BLOB bodies and network client requests are untrusted. Data affected includes device control properties, images, and process memory. No real data or credentials were used.

## Confirmed findings

### P01 — XML attribute accumulation has no enforced bound (high)

Evidence: `indigo_libs/indigo_xml.c:1349–1351,1398–1401,1705–1723`. `value_buffer` has 524289 bytes; `ATTRIBUTE_VALUE_STATE` appends every non-closing byte without checking capacity. The loop assertion is not an input-validation branch. A client reaching XML parsing can supply an overlong attribute, including an unknown attribute on an otherwise accepted tag, before any token check. With assertions enabled, advancing beyond 524288 causes process-wide assertion failure. With assertions disabled, sufficiently long input writes beyond the heap allocation. The terminating NUL also becomes out of range. This is an availability and memory-integrity defect; code execution has not been demonstrated.

Fix: reject before every append that would consume the reserved terminator byte, close only the offending session, and use a checked limit that survives release builds. Regression: lengths one below, at and above the accepted limit, split across read buffers, both quote styles, and valid ordinary attributes; verify under ASan and with `NDEBUG`.

### P02 — XML name and entity termination writes outside exact-size buffers (high)

Evidence: `indigo_libs/indigo_xml.c:1351,1363,1420–1438,1482–1489,1691–1699`; `indigo_libs/indigo/indigo_bus.h:65`. The name buffer allocates 128 bytes while tag and attribute scanners accept 128 alphabetic bytes, then store a NUL at index 128. The eight-byte stack entity buffer similarly accepts eight letters and appends a ninth byte for NUL when `;` arrives. Assertions permit the name pointer to equal its capacity, so they do not prevent these writes. The invalid-entity trace can also read the unterminated entity as `%s` when tracing is compiled/enabled. Any inbound XML session or connected remote XML server can reach these paths. Impact is heap/stack memory corruption and possible process termination; no stronger exploitability claim is made.

Fix: reserve terminator capacity, reject overlong/unknown entities instead of continuing, and never log an unterminated buffer as a string. Regression: tag/attribute names of 127/128/129 characters; named entities at seven/eight/nine letters, missing terminators, and the five supported entities. Preserve valid name and entity behavior.

### P03 — JSON token and frame terminators exceed allocations (high)

Evidence: `indigo_libs/indigo_json.c:333–335,349–361,405–415,451–491`; `indigo_libs/indigo/indigo_json.h:49` (256 KiB constant). Buffers allocate exactly the advertised token/frame capacity. A 128-character field name is accepted before NUL is written at index 128. Text, number and logical token buffers accept 262144 bytes and then write NUL at index 262144. The WebSocket reader can return exactly 262144 and `buffer[count] = 0` also writes one byte past allocation. Token accumulation spans successive input reads, so a single-frame limit does not eliminate the token issue. These are pre-authorization heap out-of-bounds writes for clients reaching raw JSON or WebSocket JSON parsing.

Fix: define separate content and allocation capacities, reserve the terminator consistently, and reject unsupported oversized tokens before dispatch. The archived proposal's additional byte addresses these particular terminators but is not a complete framing/schema fix. Regression: exact boundaries and multi-read tokens, including a full-capacity frame; test valid small number/switch/text requests against observable bus dispatch.

### P04 — XML dispatch accepts response-only handlers on inbound clients (high)

Evidence: `indigo_libs/indigo_server_tcp.c:101–106`; `indigo_libs/indigo_xml.c:436–445,882–899,1267–1339,1372–1395`. The inbound server supplies a NULL device context, but `top_level_handler` accepts `switchProtocol`, `def*`, `set*`, `delProperty` and `message` without checking the connection direction. `switch_protocol_handler` asserts a non-NULL device and writes `device->version`; definition/update handlers also dereference `device->version` for ordinary name/state attributes. Thus a small response-form XML element received from an unauthenticated inbound client aborts or dereferences NULL before bus authorization. An inbound definition without attributes can also reach a zero-capacity property registry (`def_property:809–855`). Reverse-direction `enableBLOB` requires a non-NULL client but is likewise dispatched without that guard.

Fix: enforce a direction-specific top-level message allowlist and reject invalid structures before selecting a handler. Do not merely remove assertions. Regression: every allowed top-level request/response in its proper direction plus each response form on an inbound client and each request-only form on an outbound device adapter; process must survive and no unintended dispatch occurs.

### P05 — Legacy XML BLOB decode trusts declared size without bounding decoded bytes (high)

Evidence: `indigo_libs/indigo_xml.c:739–759,1630–1648,1667–1679`; `indigo_libs/indigo_base64.c:62–117`. A connected remote XML server using the legacy protocol controls `size`, parsed by `atol`. The parser allocates only `size + 3` but the legacy branch decodes all base64 characters until the closing tag, including successive chunks, without comparing output bytes against that allocation. A positive small declared size with a longer syntactically valid base64 body therefore writes beyond the heap buffer. It does not require a cached property: decode precedes `set_property` lookup. Modern-protocol arithmetic and accepted sizes also lack explicit overflow/resource limits, but those are not needed for this legacy proof.

Fix: validate numeric size conversion/range, maintain checked encoded/decoded byte budgets, reject oversized or malformed base64, and require decoded length consistent with the advertised size. Regression: exact valid one/two/three-byte padding, shorter/longer bodies, multiple chunks, malformed/empty base64, and size overflow. Legacy newline support must remain valid. This finding applies to the remote-server/client parsing side, not ordinary inbound `newBLOBVector` uploads.

### P06 — Multiple inline XML BLOB items share a buffer that is resized before consumption (high)

Evidence: `indigo_libs/indigo_xml.c:758–759,766–792,1669–1679,501–512`. Each decoded item stores the same parser-owned `blob_buffer` pointer. The next item reallocates that buffer and overwrites it; the vector is consumed only at its closing tag. Earlier item pointers may now point to freed storage when realloc moves. Even if realloc stays in place, shrinking the second item's buffer below the first item's declared size makes the first copy read beyond the current allocation; equal-sized items overwrite earlier content. A remote server can define a matching read-only multi-item BLOB property and send a vector with at least two inline BLOB items. This creates use-after-free/out-of-bounds reads and image corruption in the receiving process. Actual allocator movement or memory disclosure was not exercised.

Fix: give each pending item owned decoded storage, or copy it into stable per-item state before reusing scratch space; release all partial-item buffers on errors/disconnects. Regression: two distinct payloads, larger then smaller and smaller then larger, both protocols, forced moving realloc under a test allocator, and incomplete second-item cleanup.

### P07 — WebSocket reads do not validate lengths in the correct integer domain or require complete frames (high)

Evidence: `indigo_libs/indigo_json.c:61–116`; `indigo_libs/indigo_uni_io.c:247–327,1916–1944`. Frame length is uint64_t but the capacity check casts it to signed `long`. On Windows, where `long` is 32 bits, an extended length greater than 2^32 whose low bits are a small positive value passes the check and is read using the truncated length. For a masked frame the following loop still uses the original uint64_t length, so it XORs memory beyond the 256 KiB buffer. On LP64, high-bit lengths instead convert negative and reach the I/O API incorrectly; platform-dependent read rejection should not be treated as validation. No live or dynamic exploitation was performed.

Separately, every header/extension/mask/payload read checks only `<= 0`. The I/O implementation explicitly returns a positive short count on EOF. Truncated headers/extensions then consume uninitialized stack bytes; a short positive payload is unmasked and returned as its full advertised length, processing old buffer bytes as if received. Header opcode/FIN/RSV and client masking constraints are not validated either. Do not overstate the short-read issue alone as arbitrary memory disclosure: normal-size buffer tail is allocated and contains zeroed or prior same-session bytes.

Fix: validate uint64_t length against the content capacity and LONG_MAX before conversion; require exact read lengths; enforce supported frame opcodes, masking and fragmentation/control-frame handling. Regression: truncated 2/8-byte extension, truncated mask/payload, oversized uint64 lengths with small low 32 bits, full capacity, valid masked messages and control frames on Windows and LP64. Source-contract evidence confirms the narrowing defect; Windows execution remains unverified.

### P08 — Streaming property items have no total request budget (medium)

Evidence: XML `indigo_xml.c:283–285,324–326,365–367`; JSON `indigo_json.c:190–193,234–237,268–269`; `indigo_bus.c:1393–1407`; allocation helpers `indigo/indigo_bus.h:1004–1037`. Every item extends the in-progress property, with no item-count or total-message allocation cap. Input buffers are reused, so their fixed capacities do not bound total property size. An unauthenticated connected client can keep one request open while adding items; allocation grows before the final request/token dispatch. Exhaustion hits assertion/exit in the safe allocation wrappers, terminating the whole process rather than only the connection. No exhaustion test was run. Severity is medium because it requires sustained resource consumption; this is distinct from the small-input corruption findings.

Fix: enforce documented item/message/depth/allocation budgets per session and reject over-budget requests with ordinary error handling. Regression: accepted maximum and next item, multi-buffer inputs, cleanup after rejection, and an unaffected second session. Select limits that accommodate legitimate large properties.

## Positive contracts and rejected overclaims

- `indigo_safe_malloc` zeroes exactly the requested allocation; it does not secretly allocate an extra terminator. `indigo_resize_property` grows items and zeroes new slots before updating count. Ordinary item additions are therefore not an immediate preallocated-array overflow; P08 concerns missing total budgets.
- `INDIGO_COPY_NAME` clears 128 bytes and copies at most 127 (`indigo_bus.h:931`). Long attribute **values** copied into property names are not themselves unbounded destination copies. P01 concerns the earlier parser scratch buffer; P02 concerns tag/attribute **names**.
- XML ordinary text append is bounded and its buffer includes a terminator (`xml.c:1350,1539–1557`). It truncates overlong text rather than growing indefinitely; this is not the same overflow as attributes.
- Normal non-WebSocket JSON line reads request `JSON_BUFFER_SIZE - 1`, so their immediate input-buffer NUL is within allocation. P03 separately covers full-size WebSocket frames and accumulated tokens.
- WebSocket `indigo_uni_read` normally loops until its requested length is satisfied; a short successful OS read alone does not prove a defect. P07 depends on EOF returning a positive partial total and on unchecked narrowing.
- The parser has no external-entity/DTD resolver. Conventional XXE file-fetch/network-fetch claims were not substantiated. The named-entity scratch overflow is a different issue.
- JSON define/update output uses `append_json` with growth and `va_copy` (`json.c:637–656`); treating all serialization as a fixed 256 KiB sprintf overflow would be incorrect. XML/JSON text-value escaping uses dedicated long buffers for long values.
- Property changes normally reach the bus dispatcher, which is the authorization boundary. This review does not claim that omitting authentication in every parser helper independently bypasses authorization. Access-control findings belong to the coordinator's separate review.

## Leads and coverage limits

Serializer property names, number formats and BLOB metadata are not uniformly escaped (`xml.c:1929–1960,2049–2092,2187–2194`; `json.c:683,712,767–769`). Remote re-shared metadata is a possible protocol-injection boundary; end-to-end security impact and relevant name contracts were not fully traced, so this is an unconfirmed follow-up, not a canonical confirmed finding here. Cached XML updates also do not compare property vector types before interpreting item unions (`xml.c:452–525`); a dedicated schema/type review remains appropriate.

`new_blob_vector_handler` copies an entry returned by `indigo_find_blob` without taking the entry mutex (`xml.c:421–425`); the transport reviewer was notified to reconcile concurrent HTTP PUT/delete ownership. Avoid duplicating their canonical BLOB lifetime finding. `indigo_xml_client_adapter` formats an unbounded caller-supplied name at `indigo_client_xml.c:44`; caller/config/discovery provenance and actual overwrite extent were not fully established in this bounded assignment, so it remains a lead.

No complete grammar compliance certification, live service test, sanitizer run, fuzz campaign, cross-platform build, exhaustive concurrency test or security fix retest was performed. Existing `build/lib/libindigo` was explicitly not used as source-current evidence because prior aborted work may have left stale objects. Existing protocol test sources were inspected for conventions; they were not executed. The repository's configured security command validates Factory metadata and cannot establish application memory safety. `docs/security.md` is absent from the tracked file inventory. Coordinator owns running final configured gates and canonical registration.

## Evidence collection and reconciliation

Working directory for all reads: repository root `/Users/polakop/Development/indigo`. Commands used: `git status --short`, `git rev-parse HEAD`, scoped `git ls-files`, `nl -ba FILE | sed -n RANGE`, `sed -n RANGE FILE`, focused `grep -nE PATTERN FILE`, and `./.factory/scripts/findings list`. These confirmed the reviewed source revision, current source locations and existing active findings. No `rg`, external service, prohibited driver or bulk archive search was used. The only retained task proposal opened was the explicitly assigned `20261023101753-bc8d-complete-repository-audit/proposals/unfinished-defensive-boundaries.json`; its XML and JSON suggestions remain unapplied and were independently checked. Active summaries have no existing equivalent XML/JSON parser-boundary record; FND-000036 concerns different command-line property parsers. BLOB HTTP compression and SHA-1 findings are existing, separately owned records.

Next action: coordinator independently reconcile P01–P08, assign canonical IDs and repair owners, retain open status, and authorize/plan implementation with the regression cases above. No product changes, commits or risk waivers are supplied by this report.
