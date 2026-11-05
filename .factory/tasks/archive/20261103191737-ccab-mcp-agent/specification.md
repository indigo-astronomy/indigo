# MCP agent analysis and proposed specification

Task: 20261103191737-ccab-mcp-agent
Inspected revision: b6c0019676e43843665967d31b832304dc14891f
Date: 2026-11-03
Status: analysis delivered; first transport increment can be planned. Product policy decisions below remain open before a usable MCP endpoint is enabled. This is not implementation or acceptance evidence.

## Latest owner-directed transport scope — 2026-11-04

The owner explicitly selected POST through the existing PUT-style URI callback. This supersedes the earlier additional-API proposal and its first-increment AC-002/003 parser/routing promises below. No second registration API, immutable request structure, buffered parser or transport-level exact route registry is delivered. `indigo_server_add_handler()` stays unchanged; POST and PUT callbacks own unread headers/body and receive the raw path with query plus NULL params. Exact endpoint matching is a handler responsibility. Existing header preflight and retirement still apply; body framing, limits, deadlines, access policy and log suppression remain the future MCP handler's responsibility.

Revised step acceptance: POST-001 correct method/path/raw header/body delivery through existing callback; POST-002 JSON and fragmented binary transport preservation without form decoding; POST-003 callback-controlled keep-alive remains usable; POST-004 existing prefix routing and handler-level exact endpoint checks; POST-005 POST cannot invoke PUT-only BLOB upload or static download paths and unsupported POST terminates safely; POST-006 active POST callback retirement drains through existing removal contract. Preserve unchanged GET/PUT and WebSocket regressions. No MCP endpoint or hardware behavior is enabled. Original checkout and no-commit owner-inspection gate remain in force. Remaining sections are the historical full-feature analysis; they do not override this latest explicit owner selection.

## Owner-authorized first increment — 2026-11-04

The owner requested implementation of the first HTTP transport increment and inspection before any commit. This supersedes the original analysis-only assignment for that increment. Scope is AC-001 through AC-003 and the HTTP retirement portion of AC-004; MCP agent/control/auth policy decisions remain open for later increments. See plan.md and test-plan.md for concrete limits and API ownership. Callbacks own complete raw responses and must bound their work; generic callback preemption, response buffering/size enforcement and streaming are not promised by the first API. The server closes the new-style connection after callback return. No commit, publication, archival or feature-completion claim is authorized before owner inspection.

## Confirmed outcome and boundaries

The owner requested incremental development of an MCP server implemented as an INDIGO agent using the existing TCP server's HTTP transport. This assignment authorizes analysis and specification only. No implementation, generator modification, installation, hardware operation, merge, publication or deployment is performed.

The agent owns MCP semantics, tools, policy, property cache and service lifecycle. The TCP server owns HTTP framing and route dispatch. Drivers remain unaware of MCP. The intended users are MCP clients discovering INDIGO devices and properties and, in a later increment, issuing explicitly permitted property changes.

Do not create one tool per dynamic property. Use a small stable tool set with typed structured results. Initial proposed tools are `list_devices`, `list_properties` and `get_property`; `set_property` is a separate later increment. Domain-specific exposure/mount/sequencer commands are future work, not part of the minimum feature.

## Source evidence and architectural conclusions

Paths and lines below refer to the inspected revision; proposed names are not existing APIs.

| Evidence | Observed contract and implication |
| --- | --- |
| `indigo_docs/DEVELOPMENT.md:5-23` | Agents have both device and client roles; client callbacks observe property definitions, updates and deletions. A bridge agent fits the existing bus model. |
| `indigo_drivers/agent_alpaca/indigo_agent_alpaca.c:980-1022,1296-1376` | Alpaca registers HTTP handlers and attaches a device plus client. Its shutdown removes handlers before detaching/freeing client/device/code. This is the closest architectural precedent, not a template to copy wholesale. |
| `indigo_libs/indigo/indigo_server_tcp.h:79-92` | Existing handler receives handle/method/path/query only. Removal waits for active handlers and must occur outside handlers without locks they need. |
| `indigo_libs/indigo_server_tcp.c:345-411,523-524,582-611` | Dispatch recognizes XML, INDIGO JSON and HTTP; actual HTTP branches are GET and PUT. GET consumes headers before its callback, while PUT callbacks own headers/body. A POST branch alone does not provide a consistent request API. |
| `indigo_libs/indigo_server_tcp.c:93-127,667-726` | Resource lookup uses prefix matching, reference counts and draining retirement. New exact `/mcp` matching must reject `/mcp-extra`; old prefix handlers must retain their semantics. Registration currently returns void and permits duplicate paths, so new registration needs an observable conflict/failure result. |
| `indigo_libs/indigo_server_tcp.c:132-204` | HTTP header progress/total deadlines and bounded preflight already exist. Preserve these protections; extend framing deliberately without an unrelated HTTP rewrite. |
| `indigo_libs/indigo_uni_io.c:1683-1700,1795` | Listener binds all IPv4 interfaces on both platform branches. Sharing port 7624 is not local-only exposure. Existing worker data does not provide a trusted peer-policy context (`indigo_uni_io.h:106-109`). |
| `indigo_libs/indigo_bus.c:905-977,1003-1027,1068-1122,1204,1353` | Bus synchronizes client callbacks and device routing. `indigo_change_property()` can return OK after skipping a token-protected device, so dispatch return is not device acceptance or completion. Cache mutexes must never be held while entering the bus or performing network I/O. |
| `indigo_libs/indigo/indigo_bus.h:666-669` | Property copy helpers exist, but payload lifetime and partial updates still require careful per-type ownership. Do not retain callback pointers or shallow-copy dynamic text/BLOB data. |
| `indigo_libs/indigo_json.c:409,661-712` | Existing parser implements the INDIGO wire protocol, not a generic JSON-RPC document API. Existing escaping helper is not evidence of complete arbitrary JSON encoding. A bounded JSON document/parser and serializer strategy must be reviewed before MCP implementation; do not silently add a new dependency or modify the protocol parser. |
| `indigo_libs/indigo/indigo_names.h:97-99,1683-1687` | Authentication and Wi-Fi password properties exist. Generic read access can disclose secrets; read-only is not a confidentiality boundary. |
| `Makefile.drvs:32-37`, `Makefile.drv`, `Makefile:75`, `indigo_server/indigo_server.c:168,232`, `indigo_windows.sln:91` | Driver-directory wildcard builds, package selection, built-in server registration and platform project registration are separate concerns. New agent registration must cover intended dynamic/static and package variants rather than assuming directory creation suffices. |
| `indigo_test/integration/test_transport_http_security.c`, `test_server_websocket.c`, `test_agent_alpaca_cache.c`, `test_agent_alpaca_mapping.c` | Existing synthetic HTTP, WebSocket and bridge-cache tests provide focused regression patterns. Test naming for new driver-specific cases must use `agent_mcp`. |

A hand-written agent is consistent with existing bridge agents. This analysis does not require changing the generator. If later implementation finds a generator limitation, concrete generator changes require separate owner approval.

## Transport boundary proposed for the first increment

Add an HTTP handler API alongside the existing API; retain public ABI and legacy callback behavior. Its request context should give the callback immutable parsed method/path/query, relevant header access, exact-length body access and caller-owned registration context. Lifetime is the callback duration. A registration result reports conflicts/failure. No HTTP structure should embed MCP concepts.

The new route has explicit exact-match semantics and method handling. Parsing is performed once; agent code must not parse headers from a socket which the server already consumed. Define ownership of the response and connection before coding. Prefer a bounded buffered request and bounded JSON response for the minimum feature; no indefinite SSE worker in the first increment.

Limits must be concrete implementation constants/configuration and tested at boundaries: request line/header bytes and counts, body bytes, nesting/tokens, response bytes, cache entries/bytes, per-request total/progress deadlines, network write deadline and concurrent new-handler calls. Numeric defaults are engineering choices to record in the plan, not owner decisions. A small rejection status/body must remain possible after the normal response cap is hit.

Reject malformed/overflowing/contradictory Content-Length and ambiguous Content-Length plus Transfer-Encoding. Either implement supported transfer coding correctly or reject it with an explicit HTTP error and close; never interpret leftover bytes as another request. Reject oversized requests before allocation. Read bodies by length, not newline. For the first new-handler implementation, closing the connection after each complete response is an acceptable simplification if clients pass interoperability checks; legacy keep-alive behavior is preserved.

## MCP version strategy: explicit compatibility required

The official `latest` transport page resolved to revision 2026-07-28 during this analysis. That revision uses POST and per-request metadata, requires header/body consistency checks, removes protocol sessions and the standalone GET stream, and uses request-scoped JSON/SSE results. Missing/mismatched required headers and unsupported versions have distinct errors. GET/DELETE may return 405. [Current Streamable HTTP](https://modelcontextprotocol.io/specification/2026-07-28/basic/transports/streamable-http)

Modern revision negotiation has no initialize handshake: requests state their version, `server/discover` is required, and legacy interoperability is an additional behavior. An implementation that only supports modern requests cannot claim support for handshake-only clients. [Current versioning](https://modelcontextprotocol.io/specification/2026-07-28/basic/lifecycle)

In contrast, 2025-11-25 starts with `initialize` followed by `notifications/initialized`. Version agreement and advertised capabilities govern subsequent operations. [Legacy lifecycle](https://modelcontextprotocol.io/specification/2025-11-25/basic/lifecycle)

Its Streamable HTTP permits JSON responses, optional protocol session IDs, and GET returning 405 when no SSE stream is provided. A stateless JSON-response implementation need not implement persistent SSE or mint sessions merely to support that revision. [Legacy transport](https://modelcontextprotocol.io/specification/2025-11-25/basic/transports)

**Recommendation, not an accepted requirement:** keep transport increment version-neutral; choose a named real acceptance client before the MCP increment. If no client constraint emerges, propose 2026-07-28 as the initial pinned revision and add 2025-11-25 compatibility only for a demonstrated client need. A dual-era implementation must have separate version adapters/tests rather than mixing envelopes. Do not implement the deprecated 2024 HTTP+SSE transport, stdio, WebSocket MCP, sampling, prompts, subscriptions, task extensions or resumability in the minimum feature. Never claim any excluded capability in discovery.

## Agent lifecycle and concurrency

1. Initialize all private state and locks transactionally, attach device/client, populate the property view, then expose the endpoint. If registration or attachment fails, release everything acquired and report failure.
2. Default service enablement is proposed to be off until endpoint access policy is configured. Agent device publishes enabled/disabled/error state without exposing credentials. New custom property names start with `X_`; document every added property in `indigo_docs/PROPERTIES.md` when implemented.
3. Bus callbacks copy only policy-visible metadata/values into owned storage. Merge updates by item name; preserve schema on partial updates; replace a redefined property's schema; remove one property or all device properties on the corresponding deletion. Device identity is the full bus name, including remote suffix, not a truncated display label.
4. Under the cache mutex, take a bounded owned snapshot. Release the mutex before JSON encoding, network writes, bus calls or resource removal. Cache entries carry a monotonically increasing view revision/generation so clients can recognize snapshots and page invalidation. A snapshot is observed state, not an atomic snapshot of hardware.
5. Shutdown first marks stopping, withdraws endpoint admission and drains finite callbacks outside bus/cache locks; then detach client, release snapshots/cache/device and destroy state. Repeated shutdown/removal must be safe. No callback can outlive agent code unloading. Lifecycle tests must cover slow request bodies/writes, not just a callback waiting on a condition variable.
6. Endpoint registration is singleton at `/mcp`; a second instance must fail clearly rather than hide another handler. Multiple logical agent devices must not compete silently for a global route.

## Proposed observable tool contract

| Tool | Inputs and result |
| --- | --- |
| `list_devices` | Optional bounded paging cursor. Returns policy-visible devices, full stable bus names and safe available interface metadata. No implicit connection or enumeration round-trip per HTTP request. |
| `list_properties` | Exact device name plus optional bounded paging cursor. Returns names, type, permission, state, labels and item schema (number min/max/step/format, switch rule where applicable). It does not disclose excluded values. |
| `get_property` | Exact device/property. Returns the current owned snapshot, state, view revision and type-appropriate values/targets allowed by policy. Missing/deleted/unavailable objects return explicit errors. |
| `set_property` (later) | Exact device/property and nonempty unique item/value changes, restricted initially to number/switch/text. Return dispatch status and separately labeled observed property state. Never imply completion or exactly-once execution. |

The first tool list is fixed and deterministic. Tool inputs have explicit JSON schemas. Invalid JSON-RPC envelopes, unknown methods/tools, invalid arguments and runtime device failures are distinguishable. Structured results also include a bounded textual JSON representation for clients needing text; both representations describe identical data. [Tools and structured results](https://modelcontextprotocol.io/specification/2025-11-25/server/tools)

Write operations validate untrusted JSON at the MCP boundary, including identifiers, types, duplicate items and allowed properties, then use normal bus change APIs and framework validation. Do not add duplicated numeric-validation handlers to drivers. RO/light properties, unknown items, hidden/excluded properties and unsupported BLOB writes are refused. An empty device name must never broadcast a change.

Long operations return promptly after dispatch with a truthful status such as `dispatched` and an explicitly separate observed snapshot; clients poll `get_property`. A cached state change cannot reliably correlate completion to one writer among concurrent clients. Do not invent operation IDs or claim device acceptance from the bus return. Disconnection or an MCP timeout does not imply physical abort or permission to repeat a command; retrying a write can repeat a side effect. Dedicated operation tracking/idempotency/abort semantics require a later contract.

## Access policy, confidentiality and BLOBs

A shared listener cannot become loopback-only for just one path through an agent setting. Do not globally change INDIGO binding behavior. A proposed safe local mode requires trusted peer metadata/checking implemented in the shared portable I/O layer, with exact Origin/Host policy. Forwarded headers are not peer identity. Local TCP clients are still a trust class; a local mode is not authenticated user identity.

LAN/remote access needs an explicit authentication strategy before enabling the endpoint. An INDIGO device access token is not an MCP HTTP credential. Never automatically give the agent the master token or silently bypass a lock. If a static bearer secret is chosen for a local/private first version, identify it as a constrained deployment profile and verify the intended client's support; do not advertise OAuth discovery compliance. Secrets must not appear in ordinary INDIGO text properties, JSON outputs, saved public configuration, debug logs or URLs. TLS termination/deployment is separate scope.

MCP authorization is optional at the protocol level; implementations adopting its HTTP authorization mechanism should conform to the associated resource-server/discovery contract. This analysis does not equate a single token comparison with that contract. [MCP authorization](https://modelcontextprotocol.io/specification/2026-07-28/basic/authorization)

Use separate read/write visibility policy with explicit device/property selection; mere read-only access is insufficient because properties can contain credentials, filesystem paths and scripts. The proposed initial mode exposes an explicit safe allowlist, excludes authentication/security/configuration-management/scripting surfaces and does not grant the MCP path access to alter its own policy or unload its own agent. Re-evaluate policy for each request/snapshot, and discard excluded data after a policy change. Metadata/labels are untrusted data, never instructions to the model or executable input.

For the minimum feature, BLOB properties expose safe metadata only (format/size/state when available), never raw data, pointer-based URLs, local file paths or externally supplied download URLs. Do not enable bulk BLOB delivery just to maintain the cache. Authorized binary retrieval, secure resource identifiers, data lifetimes and image conversion are separate future work. No generic URL fetching or file access is added.

## Acceptance criteria and verification mapping

All criteria below are proposed feature requirements pending the decisions affecting their scope. None is claimed implemented or passed.

| ID | Observable criterion | Required verification |
| --- | --- | --- |
| AC-001 | With no new handler/agent, existing XML, INDIGO JSON, HTTP static/BLOB, WebSocket and Alpaca behavior is preserved. | Baseline and after-change focused regression runs; legacy handler compatibility cases. |
| AC-002 | A registered exact new-style test route receives method, headers, query and exact body once; `/mcp-extra` cannot invoke `/mcp`. Duplicate route ownership fails observably. | Synthetic HTTP requests including split packets, binary/newline body and route conflicts. |
| AC-003 | Invalid lengths, over-limit input, unsupported framing/methods, stalled reads/writes and resource exhaustion terminate with bounded memory/time and no second-request confusion. | At/over-limit tests, partial body EOF, conflicting headers, slow clients, admission saturation; sanitizer where available. |
| AC-004 | Agent load/enable/disable/unload and failed initialization release routes, state and client references; reload works; no callback executes after unload. | Repeated lifecycle and barrier-based in-flight/slow-I/O race tests; no sleep-only race assertions. |
| AC-005 | Endpoint implements only explicitly pinned MCP versions/capabilities, with correct discovery, request/notification and error behavior. | Versioned JSON fixtures plus a named real client smoke test; modern header/body mismatch or legacy initialization tests according to selected scope. |
| AC-006 | Three read tools show only allowed devices/properties and correct typed values/targets; updates, redefinitions, property/device deletion and remote names are reflected. | Synthetic bus devices with partial item updates, dynamic text, duplicate display labels, deletion/reappearance and concurrent HTTP reads. |
| AC-007 | Hidden, WO-value, excluded secret, policy configuration and BLOB-payload content cannot leak through any result or log; policy changes revoke visibility. | Sentinel secret fixtures across normal/error/list/get/log paths; bounded snapshot and cache tests. |
| AC-008 | Untrusted/disallowed callers and invalid Origins are rejected before data disclosure; selected local/remote authentication mode behaves as documented. | Loopback/non-loopback, Origin/Host, credentials and forwarded-header spoof cases applicable to selected policy. |
| AC-009 | Later writes respect exact targeting, permissions and device locks without master privilege; unsupported changes produce zero device side effects. | Synthetic device records invocation counts; wrong token, RO, unknown/duplicate item, empty device, self-unload and forbidden policy target cases. |
| AC-010 | Later long-running changes return within a bounded request interval and report dispatch separately from BUSY/OK/ALERT observation; disconnect does not secretly abort/retry. | Synthetic delayed operations, concurrent writers, token-denied route, disconnect and timeout cases. |
| AC-011 | Intended Unix/macOS/Windows dynamic/static/package configurations register and link the agent; no Factory metadata enters Xcode. | Focused builds and project-registration review; unavailable environments explicitly unverified. |
| AC-012 | Added properties, setup, limits, selected protocol versions and deployment restrictions are accurately documented. | PROPERTIES mapping review and reproducible client configuration example without real credentials. |

Tests must run using temporary/private configuration and synthetic devices by default. No hardware testing is proposed for the first transport/read-only increments; no hardware certification is implied. For later driver-specific test execution, follow the existing runner/recording requirements. Do not run broad off-limits drivers, vendor builds or shell_compliance as a side effect of a transport change.

## Recommended staged implementation

1. **Transport contract and characterization.** Record the actual baseline, finite limits, additive API design and response ownership in plan/test-plan. Add focused old/new handler fixtures, then implement exact new-style routing, POST framing/context and safe draining. No MCP agent/tool logic yet. Exit: AC-001 through AC-003 and new-handler retirement portion of AC-004 pass. The generic transport step is independent of the product decisions below.
2. **Agent lifecycle and protocol skeleton.** Resolve version/client and endpoint access mode; implement opt-in `agent_mcp`, private state, registration/rollback/unload and selected protocol adapters with empty tool catalog. Update build/platform integration and new-property documentation. Exit: AC-004, AC-005, AC-008 and applicable AC-011/012 pass with no device control.
3. **Policy-filtered read tools.** Resolve visibility policy, implement bounded cache/snapshots/paging and three read tools. Exit: AC-006/007, concurrency/lifetime checks and real-client read interoperability pass. This is the first useful MCP release candidate.
4. **Controlled writes.** Resolve write authority/allowlist, implement `set_property`, truthful dispatch response and asynchronous polling semantics. Exit: AC-009/010 plus all affected regressions and specialist security review pass.
5. **Optional future features.** Authorized BLOB resources, semantic astronomy tools or subscriptions only from separately specified demonstrated needs. Not prerequisites for steps 1-4.

After each implementation step, record actual results and deviations; keep one task/branch through role transitions. The due incremental repository audit must be completed or explicitly owner-deferred before implementation proceeds; this analysis does not qualify as that audit. Every final task diff still needs independent quality/security/verification gates and committed-range evidence. No implementation is authorized merely by this document.

## Material decisions and completion status

The coordinator should resolve these before the affected increment, without blocking independent transport work:

- **D-001, before step 2:** Which concrete MCP client/version is the first acceptance target? Recommended proposal: pinned modern revision initially; legacy adapter only where needed by the chosen client. Client support has not been tested.
- **D-002, before step 2:** Is the initial deployment local-only, trusted LAN or remote? This determines peer checking/authentication/TLS responsibilities. Recommended proposal: disabled-by-default, local-only first with explicit disclosure that local applications share the trust boundary; remote access later.
- **D-003, before step 3:** Which device/property set may be read, and how is that policy configured without exposing secrets? Recommended proposal: explicit allowlist and deny-by-default for values. No universal safe-property heuristic is assumed.
- **D-004, before step 4:** Which property changes are permitted, and are all writes opt-in? Recommended proposal: read-only initial release, explicit separate write selection later; never unrestricted access to server management or scripting.

Assessment: high complexity for the full feature because version-era compatibility, existing HTTP framing, callback lifetime, bus/cache lock ordering, secret-bearing generic properties and platform registration cross contracts. First isolated transport step is medium complexity. Consequence risk is high for enabled writes and remote exposure; reversible analysis has low execution risk. Recommend balanced for the narrow transport planning assignment and reassessment/strong specialist review for auth, lifetime and write security. Current model/effort was inherited and not observable; no model change or cost estimate is claimed.

Inspection performed: read-only source/documentation inspection and official web references; repository status and active finding summaries read. No build, automated test, real MCP client, network listener, physical hardware or sanitizer run was performed. Existing active findings were not altered or represented as repaired. No newly confirmed implementation defect is asserted; listed gaps are requirements for a feature that does not yet exist.

The analysis deliverable is complete. The whole-feature specification gate remains open for D-001 through D-004 as applicable; the task remains unfinished. The coordinator owns maintenance disposition, state transition, plan/test design and the next authorized implementation increment.

## MCP skeleton increment — 2026-11-04

Baseline: no agent_mcp exists. Previous POST dispatch increment is verified and remains uncommitted. This is a new software-only agent, not a migration of an existing hardware driver; no original-driver trace, SDK or hardware baseline applies. Reference architecture: agent_alpaca shared HTTP registration and drain-before-detach. Existing parsed-JSON choices inspected: native bus parser is protocol-specific; in-tree MIT jsmn can be included privately with strict full-document validation and no new library/framework API.

S-001: load one MCP Agent device transactionally; duplicate init/shutdown are safe. Standard CONNECTION enables the service, disconnected by default. No custom properties or property cache/client until read tools need them.
S-002: own exact /mcp handling through existing handler; other prefix matches404, GET405 (no stream), POST response closes connection. Unload drains route before device/state release.
S-003: bounded HTTP framing, one decimal Content-Length, JSON media type and required Accept types, no transfer coding/Expect, finite body/progress deadlines, bounded concurrent processing, content-log suppression inside agent. Reject Origin-bearing browser requests in this first headless-only skeleton; no device data or control is exposed, authentication policy remains for the read increment.
S-004: initially pin MCP2025-11-25 for initialize/notifications/initialized, ping and empty tools/list; JSON-RPC IDs/errors preserved and malformed JSON/envelopes rejected. No SSE, session ID issuance, resources/prompts/control or unsupported capabilities claimed. Concrete client version remains an optional owner input; this initial pin is an engineering assumption, not broad interoperability evidence.
S-005: no new framework/library/generator change; necessary agent files, static server/project/test registration only. Portable code, bounded parsing and meaningful synthetic failure/lifecycle/concurrency tests. Original checkout and owner pre-commit gate remain.

## Owner read policy and read-tools increment — 2026-11-04

Owner selected all visible properties, then explicitly removed password/token/BLOB exclusions: do not implement a secret-name heuristic; passwords are handled by WO permissions. This supersedes D-003 allowlist recommendation. Expose all nonhidden RO/RW text/number/switch/light/BLOB properties through list_devices/list_properties/get_property; exclude WO properties entirely. Preserve full device/property/item names, number values and targets, and typed values. BLOB value is base64; do not fetch remote URLs as a side effect. A URL-only BLOB reports data unavailable instead of implying an empty payload. Bounded data/cache/response failures must be explicit, never stale/truncated successes. No write tools, extra framework API, access configuration or new properties. Existing explicitly enabled listener trust boundary remains; no authentication is added by this increment, and all reachable headless clients share its read access. README must state that actual scope.

Use an internal client with owned cache and partial updates, drained before client/cache destruction. Limits:1024 properties,256 items/property,64KiB variable text/BLOB bytes per property,16MiB total owned cache,1MiB encoded response. Cache overflow/unknown updates fail closed with a tool error until reload rather than returning an incomplete list. No hardware tests; synthetic bus/socket fixtures only. Real Codex/Claude GUI compatibility remains separately unverified.

### Owner BLOB/output clarification
Owner requires URL-based BLOB results only; no base64 support or payload copying. BLOB callbacks supply pointer/URL; use existing INDIGO HTTP BLOB URL convention and never dereference/store payloads. This supersedes the prior base64/cache-size proposal. Use dynamic text/cache/results without arbitrary output/cache caps; existing inbound HTTP/JSON limits remain unchanged. Ordinary allocation failure produces an explicit error. All tools remain read-only.

## Owner-authorized write increment — 2026-11-04
Owner requests continuation and explicitly authorizes all visible RW/WO properties including server/agents according to bus rules (D-004 resolved). Add set_property(device, property, items, optional token), with exact nonempty full names, known unique item names and typed values. First write increment supports text/number/switch; light/RO/hidden/missing and BLOB writes fail explicitly without dispatch. URL-only BLOB input cannot generically implement native remote upload (URL is upload destination, requires payload); no fetch/base64/core change is added. Parent disclosed this bounded type scope.

To make WO writes discoverable, list_devices/list_properties now include visible WO schemas, without values. get_property continues rejecting WO, and cache must retain only their schema. This supersedes earlier whole-WO omission for discovery, preserving the no-WO-values contract.

Default token0, optionally explicit caller hex token; never inherit process device/master tokens. Build a partial property and call indigo_change_property with internal client, outside cache/service locks. Report dispatched:true/completion:not_confirmed only after dispatch call returns OK; busOK neither proves token acceptance nor device completion. No operation/retry/abort/polling abstraction; later state read via get_property. Same-thread synchronous self-unload during dispatch must return INDIGO_BUSY before service/route/lifecycle mutation; external unload continues draining. Server queued driver operations retain existing behavior. No unasked restrictions on server/agent targets.

Validate JSON types, finite numbers, duplicate/unknown items, empty requests, exact targeting and token syntax at MCP boundary; framework owns numeric ranges/steps and device operation rules. No actual hardware/actions in verification. Original checkout/no commit gate retained.

## Final delivery contract accepted for commit
Owner accepts the implemented/tested current candidate and explicitly requests committing all MCPchanges without push. This final contract follows owner corrections over the original analytical recommendations: existing handlerPOST only, no publicHTTPAPI change; disconnected-by-default MCP2025-11-25 endpoint; visibleRO/RW reads and visibleWO schema-only discovery; no secret/token heuristic; BLOBreads viaURL only; dynamicoutput/cache without artificialresultsize caps; allvisibleRW/WO text/number/switch writes using nativebus permissions/explicittokens and truthfuldispatch-only result; unsupportedBLOB/lightwrites explicitlyrejected; realCodexCLI andClaudeDesktoplocalCode simulatorverification. No mastertokeninheritance, newauth/configprops/generator changes, physicalhardwarevalidation or ClaudeChatbridge promised. Required platformprojectregistration is included; unavailableOSruntime checks remain documented. All confirmedtaskfindings fixed/reviewed; finalsourceportability token-copy correction covered by focused tests. This is a localcommit-only delivery; no merge/push/publication authorized.
