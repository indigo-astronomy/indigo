# POST dispatch through existing URI handlers

Owner revised the first increment on 2026-11-04: reuse `indigo_server_add_handler()` and its existing PUT callback contract for POST. This replaces the previous additive buffered API plan, retained under superseded-http-api/ as historical evidence.

1. Remove our uncommitted additional API/parser/structures, separate test and Makefile entries. Preserve all existing project behavior and any owner edits; original reviewed file hashes were checked before removing only our previous design.
2. Extend the existing PUT branch to recognize POST and pass the correct method/path offset to the same callback. Leave headers/body unread for the callback and preserve prefix routing, raw query-in-path and boolean keep-alive semantics. Do not route POST through the built-in PUT-only BLOB uploader; unsupported POSTs to built-in/static resources must close with an appropriate error.
3. Document actual shared callback ownership. No automatic JSON/form parsing, new resource type, request cache, admission/parser limits or blanket content-logging guarantee. The eventual MCP handler must check its exact path and enforce request framing, limits, deadlines and access policy before doing work.
4. Extend existing synthetic HTTP security integration suite for method delivery, headers/binary body preservation, fragmented input, prefix/exact handler checks, keep-alive and handler retirement. Reuse existing suite/build targets; no new test file or Xcode reference is needed.
5. Run baseline reproduction before production fix when feasible, focused ASan/UBSan HTTP and WebSocket regressions, then independent final-diff quality/security review. Leave changes unstaged and uncommitted in /Users/polakovic/Development/indigo for owner/Xcode inspection.

Scope: indigo_libs/indigo_server_tcp.c, its existing public header documentation, indigo_docs/DEVELOPMENT.md, existing test_transport_http_security.c and task evidence. No drivers, generator, property reference, dependencies or project metadata changes. Same task/branch and owner-prioritized transport scope; recurring audit remains due without checkpoint change. Bounded dispatch change medium network consequence, low implementation complexity; inherited host/model retained, no new costs/providers.

## Next increment: MCP agent skeleton

1. Discovery complete: new driver absent; inspected Alpaca lifecycle, portable I/O, JSON options and platform integration. No hardware tests planned; synthetic bus/socket tests only. No existing driver regression/trace exists to reproduce.
2. Implement hand-written agent_mcp.c/.h with singleton device and CONNECTION enablement, initially disconnected. Private jsmn inclusion, bounded grammar/UTF-8/envelope validation and MCP2025-11-25 skeleton (initial protocol assumption while client preference pending). No generic framework/parser changes.
3. Agent owns exact route, bounded HTTP header/body processing, no-content-logging, finite requests and route draining. Empty catalog exposes no device data/control. Browser Origin requests rejected for the first headless-only skeleton; authentication/read policy is deferred before any device tooling.
4. Add focused test_agent_mcp.c plus normal/sanitized targets. Parent owns agent/source and project integration; test subagent owns only its test and Makefile entries, independent review owns reports. Existing POST TCP/test changes preserved byte-for-byte.
5. Register new source/test in Xcode for owner inspection, static server and Windows projects; do not add Factory metadata. Do not broadly build off-limits drivers. Add only essential new-agent usage/test record per project conventions; no existing header/docs churn.
6. Build focused driver/archive and synthetic tests, run recorded driver test command, validate properties/version/project references, independent quality/security review; retain no-commit state for owner inspection.

Risk: full MCP medium/high complexity at untrusted JSON/HTTP/lifetime boundary; this skeleton cannot drive hardware. Inherited model retained; exact runtime settings unobservable, no provider switch or new paid calls. Due incremental audit remains recorded; owner's explicit continuation prioritizes this same MCP task without resetting cadence.

### Skeleton progress and exit evidence

Steps1-5 complete: discovery/baseline and hardware decision recorded; hand-written agent with private existing tokenizer/full validation implemented; focused14-case fixture and necessary static/Xcode/Windows registration added. Step6 implementation/verification/review complete for this skeleton: strict compile, universal driver build, recorded native14/14, sanitizer14/14, HTTP29/29 and WebSocket5/5 passed; fixed low header-version finding archived. Required README/MIGRATION_STATUS/TEST_SUMMARY records match14/0 hardware-free/hardware availability. Independent quality/security scopedpass. Real client GUI integration, localClaude bridge and device tooling belong to subsequent increments; owner inspection and no-commit gate remain.

## Shared tokenizer relocation

Inventory complete: PrimaLuce c/h, Giotto/Alto host simulators, MCP agent, shared/test Makefiles, PrimaLuce Windows ClInclude and existing Xcode header reference. Header is private third-party MIT code, not altered. Move to indigo_libs/externals/jsmn/jsmn.h (existing plural convention), use the existing indigo_libs include root, retain configuration macros and stable Xcode file/build identity. Validate byte identity and all maintained references; build/run focused PrimaLuce unit and MCP cases, compile both affected host simulators, inspect Windows/XML/Xcode references. No hardware or broad unrelated driver sweep; no generated-driver change, version bump or new dependency/API. Final recorded MCP run and affected sanitizer checks retained, reviewer scope refresh, no commit.

Shared tokenizer move implemented and verified: identical header bytes, all maintained includes/prerequisites/project references updated, PrimaLuce38/38 and MCP14/14 native+sanitizer pass, Giotto/Alto host simulator builds pass. Independent bounded relocation review passed; owner inspection/no-commit gate remains.

## Read-tools increment preparation — 2026-11-04

Owner requested continuation after relocation checkpoint82e884b80. Next bounded increment is list_devices/list_properties/get_property in the agent only, with no writes or further framework API changes. No production changes started: D-003 read visibility and the dependent access boundary remain unresolved. An asynchronous owner question offers explicit allowlisting or visible-property access with sensitive-value exclusions; silence is not a policy decision.

1. Confirm read visibility and caller access policy before exposing data; record exact exclusions and configuration contract.
2. Add an owned bounded bus-client cache, explicit per-item observed updates (including RO values and number targets), redefinition/deletion, and transactional attach/drain/detach. Never retain callback pointers or BLOB storage. Do not use indigo_property_copy_values(), which implements write semantics.
3. Add bounded JSON responses and strict tool-argument validation, with the same visibility filter across all three tools and errors. Snapshot under a private lock; perform network I/O and bus calls outside it.
4. Extend synthetic tests for cache ownership, partial updates, dynamic text, RO/WO/hidden/sensitive properties, deletion/reappearance, large output, invalid arguments and concurrent unload/read. Hardware tests remain zero; no device changes.
5. Run focused driver/native/sanitizer checks and the recorded driver runner, then independent quality/security review. Leave original checkout changes uncommitted for owner inspection.

Progress: read-only bus/ownership discovery complete; implementation awaits the access-policy decision. No new application check or real-client compatibility result is claimed.

### Read-policy decision received
Owner explicitly authorizes all visible readable properties, including token text and BLOB payloads, and requests no password handling beyond WO permissions. No heuristic/allowlist/configuration will be added. Implement cache/client lifecycle, bounded typed JSON/base64 tools and independent synthetic tests/review. Parent owns agent and task/docs; test agent owns only test_agent_mcp.c. Cache limits and caller scope follow the new specification amendment. BLOB URLs are not fetched. Step1 resolved; steps2-5 pending.

Owner superseded BLOB/base64 and output caps before production implementation: use linked owned cache, dynamic JSON text, URL-only BLOBs. Baseline existing14/14 passed natively before source changes; no hardware. Tests updated by scoped test writer.

Read-tools steps2-3 implemented: owned linked cache, definition/partial updates/deletion and deep text ownership, BLOB URLs only, attach rollback and HTTP-drain/client-detach/cache-free shutdown; dynamic JSON and three typed read tools. List snapshots copy metadata only, get snapshots deep-copy text outside producer lifetime. UTF-8/escape handling and exact escaped Unicode target matching retained; no new properties/framework changes. Strict compile and universal driver build pass. Step4 independent test implementation and step5 final verification/review in progress; no commit.

Read-tools steps4-5 complete for this increment:23 cases pass native/ASan/UBSan, final recorded driver runner23/23 after test ownership release, universal agent build/strict compile and boundary/project checks pass. Independent quality/security scoped pass, FND002 schema issue fixed/retested/archived. Required records updated to23/0; reports and exact limits in verification.md. Owner inspection remains next; no commit, whole-task completion, archive, integration or publication.

## set_property increment
1. Read bus/token/validation/server unload conventions; existing23-case native baseline before source change. No hardware. Owner scope allvisibleRW/WO; type scope text/number/switch, explicit unsupported BLOB/light.
2. Retain WO schemas without values and expose discovery metadata; keep get_property WO rejection. Add strict scalar/Unicode/finite/token argument decoding and partial-property dispatch with no inherited mastertoken.
3. Add per-dispatch thread guard to reject same-stack self-unload with BUSY; never hold agent locks across bus, preserve external drain. Result reports dispatch only.
4. Separate test writer owns only test_agent_mcp.c, production/task/docs parent-owned; extend lock/dispatch/WO/invalid/noeffects/self-unload/concurrency scenarios. Independent reviewer checks oldread expectation amendments and final quality/security.
5. Strict/universal build, native/ASan/UBSan and finalrecorded runner; required records then owner inspection, no commit/push. Inherited hostmodel retained, strong security/lifetime review recommended, no newprovider/budget.

Write steps1-5 complete for the scalar increment. Existing23-case baseline passed. Text/number/switch tool, WO schema discovery/scrub and same-stack unload guard implemented. Independent writer30/30 native+ASan/UBSan, mandatory final recorded runner30/30; strict/universal build and boundary checks pass. FND003 numeric defect fixed with portable locale-normalizedstrtod and independently reverified including comma locale. New reports scopedpass; records30/0. Owner inspection remains next, no commit/push/task archive or audit checkpoint change. BLOB write transfer and realclient integration remain outside this increment.

## Real-client verification increment
Owner explicitly requested Codex and Claude GUI verification, no commit. Parent verifier uses installed nativeapps/client only; existing model/settings retained. Prepare isolated syntheticfixture/private testhome and ephemeralserverport with agent plus read-only synthetic property; no hardware/realcredential/propertydata shared. Temporary client-specific config entries only, preserve all existing settings and remove afterverification. Prefer direct HTTP, investigate actual installed Claude Code vs chat transport; do not claim UIinteroperability from raw protocol tests alone. Capture actual nativeclient handshake/catalog/read result in task evidence, fixes only for concrete compatibility defects with focused regressions/review. No install/new paidAPI/publicexposure/commit/push.

Client verification extension: readsmoke completed; also verify newlyadded set_property through both realclients against actualsimulators so userrequest is notlimited todiscovery. Restartprivatefixture on same nowfree56512 to preserveGUIconnection; safePROFILE_NAME.NAME_0 text change/readback/restore only, no connect/configsave/exposure/motion. Initialreadphasecleanup is retained; finalwritephasecleanup follows. Source/test unchanged, no newregressionsuite scope; realclientwrite compatibility was previouslyunexercised.

Client verification complete for requested CodexCLI and installedClaudeGUIlocalCode. All4tools exercised: discovery, reading and reversible textwrites/restoration,8/8+9/9actualcalls. No agentcompatibility fix needed. README mode/setup claim corrected and manualrecords/summary current; automated30/0 unchanged. FND004 safe-reusablefixture guard reviewed with successfulstartup/cleanup, actualrunisolationproven, canonicalclosed. Allownedruntimes/configentries/privatehomes cleaned; GUIresultsession retainedforownerinspection. Independentclientreviewscopedpass. No commit/push/taskarchive or maintenancecheckpointreset.

Finaldelivery gates complete for currentacceptedcontract: ownerauthorizeslocalcommit/no push; latestmainref included; portabilityfix independentlyreviewed withfresh30/30native/sanitizer/recorded evidence; canonicalreports finalpass, all5taskfindings closed. Archive/stageexactassignedscope, recordv2contentidentities, createoutcomecommit, runimmutablecommitted-rangeprocessgate andreportexternally. Integration/publication/branchcleanup remain unauthorized/pending, nottaskimplementation blockers.
