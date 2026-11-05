# Final MCP agent verification

Verdict: pass

Final accepted delivery is the opt-in MCP2025-11-25 agent with four tools, shared legacy-handler POST dispatch, typed dynamic property cache and URL-only BLOB reads. Owner authorizes committing this candidate without push. The final portability-only token-copy fix is verified below; earlier source/client evidence remains scoped to its actual identities and equivalent protocol behavior. Native macOS verification, real CodexCLI/ClaudeDesktopCode simulator checks and independent reviews passed. Windows/Linux runtime, DesktopChat, BLOB writes and broader hardware operation remain documented limitations, not claimed checks.

Local master and freshly fetched origin/master are67a7e06fd91254322307860b0ff4e26fda216ebb, alreadyancestor oftaskHEAD82e884b80. No merge or target change invalidates evidence. No integration/push is authorized.

## Retained incremental evidence

# Revised POST transport verification

Date: 2026-11-04. Checkout: `/Users/polakovic/Development/indigo`. Base: `67a7e06fd91254322307860b0ff4e26fda216ebb`, existing branch `task/20261103191737-ccab-mcp-agent`. Candidate is uncommitted. Owner explicitly revised the transport design to use the existing PUT-style callback and requires inspection before commit. Previous broader-design reports/logs are retained under superseded-http-api/ and do not verify this candidate.

## Baseline and environment

The existing source was restored from HEAD only after all five previous working-file identities matched their reviewed values; no unrelated owner changes were present. The removed design and its tests are retained as task-local historical evidence. A regression was added to the existing HTTP suite before the production fix.

Commands below ran from the original checkout, using its existing generated Makefile.inc and build/lib dependencies. Changed TCP and relevant core units are compiled directly by existing sanitizer targets; unchanged static archives provide the remaining symbols. Apple clang/macOS arm64; no full framework/application rebuild, Linux/Windows run, real MCP client or physical hardware execution is claimed.

1. `make -C indigo_test build/security/test_transport_http_security` against original production source with new tests: exit 0, [build log](post-baseline-build.log.gz).
2. `INDIGO_TEST_CASE_FILTER='POST JSON headers and body' indigo_test/build/security/test_transport_http_security`: exit 1, expected regression failure (`assertion failed: passed`). The original dispatcher never delivered this POST through the callback. [Baseline log](post-baseline-test.log.gz).
3. After implementing the shared dispatch, `make -C indigo_test build/security/test_transport_http_security build/integration/test_server_websocket_sanitize`: exit 0, [build log](post-build.log.gz).
4. `indigo_test/build/security/test_transport_http_security > .factory/tasks/20261103191737-ccab-mcp-agent/post-test.log 2>&1`: exit 0, **29/29 passed**, with ASan/UBSan-instrumented changed TCP/core units. [Run log](post-test.log.gz).
5. `indigo_test/build/integration/test_server_websocket_sanitize > .factory/tasks/20261103191737-ccab-mcp-agent/post-websocket-test.log 2>&1`: exit 0, **5/5 passed**, ASan/UBSan. [Run log](post-websocket-test.log.gz).
6. `./.factory/scripts/check`: passed whitespace, findings, ADR index and project boundaries. [Log](post-factory-check.log.gz).
7. `./.factory/scripts/security`: passed configured Factory boundary checks. [Log](post-factory-security.log.gz). This structural check is not a product security certification.

Local socket runs used approved sandbox escalation; no external or hardware targets. Existing compile warnings in unchanged framework code remain warnings, not suppressed tests. No native normal suite is claimed for this revised candidate; the earlier 13-test normal/sanitizer result belongs only to the removed design.

## Acceptance

POST-001/002: JSON headers/body, raw query and percent/plus values are preserved; fragmented binary body includes NUL/newline/high byte. Callback verifies the exact POST method and NULL params. POST-003: one callback consumes two consecutive POST requests over keep-alive. POST-004: registry still matches prefixes and the fixture explicitly rejects a suffix path with 404. POST-005: POST to BLOB/static paths returns 405 and missing path returns 404; BLOB value identity is unchanged. POST-006: existing retirement scenario now runs for POST, alongside unchanged GET and PUT cases, and proves withdrawal with active-callback draining. All passed within the 29-case run; no previous assertion was weakened or removed.

The earlier additional body-parser/admission/response-closure promises are withdrawn by explicit owner design correction. Existing callbacks, including the future MCP handler, must validate framing, bound their work, control content logging and check access themselves. This commit candidate does not implement MCP protocol/version/auth policy.

## Review and delivery

Parent authored production/tests and performed developer verification; independent quality/security review is assigned to the existing separate reviewer against the revised final diff. Its previous report was retained under superseded-http-api/. Final committed-range/process evidence, task completion/archive and integration/publication are deferred: task remains unfinished and owner inspection before commit is mandatory. No checkpoint resets or accepted security risks are claimed.

Final scope clarification: the owner reset the public-header comments; parent preserved that concurrent change and removed its additional DEVELOPMENT.md block as unnecessary. Final product diff is only the dispatcher and existing regression test; their reviewed/tested content is unchanged. Header, docs and Makefile equal HEAD. `git diff --check` passed after this removal; `make -C indigo_test test-clean` removed test build artifacts. No behavior retest is needed for removal of documentation-only edits.

## MCP skeleton increment — 2026-11-04

Current checkout/branch remains /Users/polakovic/Development/indigo / task/20261103191737-ccab-mcp-agent over67a7e06fd; all changes uncommitted. No additional core source/API change: the existing POST dispatcher content remains the previously reviewed cdcb139b602745970418b0cfdf2596ebc5578201. New implementation is confined to agent_mcp with required static/platform/test registration. No existing header comment or generic documentation block was reintroduced. No original agent_mcp existed, so no pre-existing driver/hardware trace applies. Hardware decision: no physical devices, SDKs or external endpoints exercised; software bus and ephemeral local sockets only.

### Builds and execution

- `cc -std=gnu11 -DINDIGO_MACOS -Iindigo_libs -Ibuild/include -Wall -Wextra -Werror -Wno-unused-parameter -fsyntax-only indigo_drivers/agent_mcp/indigo_agent_mcp.c`: passed.
- `make -C indigo_drivers/agent_mcp -f ../../Makefile.drv all`: universal x86_64/arm64 archive and dylib build passed (mcp-build.log); recorded runner rebuilt the final corrected source too.
- `make -C indigo_libs /Users/polakovic/Development/indigo/build/lib/libindigo.a /Users/polakovic/Development/indigo/build/lib/libindigo.dylib`: passed (mcp-core-build.log); rebuilt the existing POST transport and generated build identity, linked available dependencies. No off-limits drivers/vendor rebuild sweep.
- Initial new-suite executions: native14/14 and ASan/UBSan14/14 passed after fixing a test fixture to enumerate public properties before changing CONNECTION. The initial503 did not establish a production defect. Logs retained as mcp-initial-* (before present-empty-version regression).
- `make -C indigo_test build/integration/test_agent_mcp_sanitize build/security/test_transport_http_security build/integration/test_server_websocket_sanitize`: passed (mcp-final-build.log). These compile the changed TCP/agent source directly and instrument relevant core units for the HTTP regression.
- Final `python3 tools/run_driver_test.py agent_mcp`: exit0,14/14; rebuilt final source and ran hardware-free cases, updated only the new driver's README Testing line and generated TEST_SUMMARY row. See mcp-recorded-test.log. Its native execution is arm64; the driver build is universal.
- Final `indigo_test/build/integration/test_agent_mcp_sanitize`: exit0,14/14 with ASan/UBSan after empty-version fix; see mcp-sanitize-test.log.
- Final `indigo_test/build/integration/test_server_websocket_sanitize`: exit0,5/5; mcp-websocket-regression.log. Existing final HTTP29-case result is recorded separately after completion.
- `plutil -lint indigo.xcodeproj/project.pbxproj`: passed. Independent integration checks parsed the Xcode graph and confirmed source membership in indigo/indigo_m1, test navigator membership, no Factory paths; Windows XML/GUID/reference/configuration checks passed. Windows/MSBuild and Linux runtime builds unavailable/unperformed, not passes.
- Configured Factory check/security helpers and `git diff --check` passed; logs mcp-factory-check.log/mcp-factory-security.log. Structural validations are not client/hardware/security certification.

### Acceptance and assumptions

S-001: INFO and failed-init rollback without bus, idempotent load/unload, disabled initial state and standard CONNECTION enable/disable/reload passed. S-002: exact path versus prefix, GET405, POST close/pipeline behavior and admission-proven unload/body draining passed. S-003: Content-Length/TE/Expect/media/Accept/version/Origin errors, partial/oversized/fragmented/stalled bodies and body64KiB/token256/depth16/admission8 bounds passed. S-004: MCP2025-11-25 init/notification/ping/emptytools, string/integer IDs, escaped keys, strict JSON grammar/UTF-8 and RPC errors passed. S-005: focused universal build and platform/project structure passed; unavailable platform execution and real clients unverified.

Owner selected Codex and Claude GUI. Official Codex docs confirm HTTP URL configuration; official Claude docs state remote connectors originate from Anthropic cloud and cannot reach localhost. A local Claude GUI stdio-to-HTTP bridge is still required and not supplied by this skeleton. No user client configuration changed or model/API call performed. No device read/write tools are exposed; no claim of real Codex/Claude GUI interoperability. Initial headless Origin rejection and disabled-by-default empty catalog do not authorize later sensitive data/control without an explicit access policy.

Sources checked: https://learn.chatgpt.com/docs/extend/mcp?surface=cli ; https://support.claude.com/en/articles/11175166-get-started-with-custom-connectors-using-remote-mcp ; https://modelcontextprotocol.io/specification/2025-11-25/basic/lifecycle ; https://modelcontextprotocol.io/specification/2025-11-25/basic/transports .

### Found defects and fixes

FND-20261103191737-ccab-mcp-agent-001 (low functional): source review found that a present empty/whitespace-only MCP-Protocol-Version was copied as empty and treated like an absent initialization header. Fixed by rejecting an empty parsed value before protocol dispatch; standalone whitespace-version fixture (without a duplicate masking it) now expects400. This was discovered by source audit, not an observed deployed failure. Final recorded native and sanitizer suites14/14 verify it. Coordinator records fixed disposition in the canonical finding lifecycle.

Implementation also corrected the tokenizer's classification of a valid top-level null: jsmn strict lexical mode requires a trailing primitive delimiter, so that flag was removed while independent full-document/grammar/UTF-8 validation remains mandatory. The null envelope is rejected as -32600, malformed/unquoted-key/numeric/UTF-8/NUL fixtures remain -32700. Test expectations were not weakened.

### Final test summary for this driver

Hardware-free integration cases:14run/14passed natively and the same14run/14passed under ASan/UBSan; these are14unique registered driver cases, not28. Physical hardware:0run/0passed. No guider interface/timing test applies. Independent review records its additional bounded grammar-corpus check separately from driver counts. Full MCP feature and committed-candidate/process/archive/integration/publication gates remain unfinished; owner inspection still precedes any commit.

Final HTTP regression completed: `indigo_test/build/security/test_transport_http_security` exit0,29/29 with sanitizer instrumentation (mcp-http-regression.log). Independent mcp_review reran the final agent sanitizer binary after verifying its timestamp newer than source/test:14/14 exit0, retained mcp-independent-sanitize-test.log. Its additional deterministic grammar-corpus harness is retained as mcp-json-review-harness.c; the differential4535-record review check is described in the independent report and is not counted as registered driver integration coverage.

Final metadata-only delta added repository-style version-history headers to the two new agent files; executable code is unchanged from the passing behavioral tests. Final universal module rebuild passed (mcp-final-driver-build.log). Independent reviewer updated current content hashes and retained scopedpass; no extra behavioral rerun was claimed for comment-only edits. Final Factory check/security and Xcode lint passed after finding archival. Generated test binaries were cleaned with test-clean after all recorded/reviewer runs; evidence remains.

## Shared jsmn relocation — 2026-11-04

Owner requested moving the existing tokenizer into a shared location. Final path: indigo_libs/externals/jsmn/jsmn.h, using the existing plural externals convention. Source bytes are identical to HEAD:indigo_drivers/shared/primalucelib/jsmn.h, SHA-256 a7811ac6efe4ba60ea6a635bb7d02c8ccb0b88ce5e8a92e06e77cef305385990; license, macros, token layout and implementation unchanged. See jsmn-relocation.json. No forwarding copy, symlink, new dependency or behavior/version change.

Maintained-source inventory excluded archives, vendors and off-limits drivers; all known consumers and build references were updated. Current MCP/private source is included in the post-move scan despite being untracked. The scan found no maintained reference to the old path or old local include; historical task evidence is deliberately unchanged. The new Xcode group resolves the existing single header reference to the new file, preserving build/header-phase identity; plutil lint and graph checks passed. PrimaLuce Windows XML reference resolves to the existing file; no Windows runtime build claimed. The existing Windows project retains legacy CRLF for unrelated lines; only the edited line uses LF, avoiding unrelated full-file formatting churn.

Commands executed from /Users/polakovic/Development/indigo:
- `make -C indigo_test build/unit/test_primalucelib build/integration/test_agent_mcp_sanitize build/integration/giotto_simulator build/integration/alto_simulator`: exit0. Rebuilt shared PrimaLuce library and test, MCP sanitizer binary and both universal host simulators; jsmn-build.log. No unrelated driver/hardware suite.
- `indigo_test/build/unit/test_primalucelib`: exit0,38/38; jsmn-primalucelib-test.log.
- `indigo_test/build/integration/test_agent_mcp_sanitize`: exit0,14/14 ASan/UBSan; jsmn-mcp-sanitize-test.log.
- `python3 tools/run_driver_test.py agent_mcp`: exit0,14/14 recorded native run; jsmn-mcp-recorded-test.log. Updates the existing new-agent README/TEST_SUMMARY configuration line, not case counts.
- `plutil -lint indigo.xcodeproj/project.pbxproj`, direct JSON project-graph/path/unique-reference assertions, Windows XML resolved-header-path assertion, and original/moved byte comparison: passed.
- Configured Factory check/security and diff whitespace checks passed; jsmn-factory-check.log/jsmn-factory-security.log.

No fresh tests were invented for a path-only move. Earlier semantic review/test evidence is reused because header content and each consumer's configuration macros are unchanged. Independent relocation review refreshes current identities/scope. Physical hardware0/0; unavailable Windows/Linux execution unverified. Changes remain unstaged/uncommitted in the original checkout and same unfinished MCP task.

## Read-tools increment baseline and developer checks — 2026-11-04
Working directory /Users/polakovic/Development/indigo, HEAD82e884b80 plus preserved uncommitted POST/skeleton. Before production changes, make -C indigo_test build/integration/test_agent_mcp passed and indigo_test/build/integration/test_agent_mcp passed14/14 (escalated solely for synthetic local sockets). No hardware test:0/0. New read tools currently built universal macOS via make -C indigo_drivers/agent_mcp -f ../../Makefile.drv all; strict -Wall/-Wextra/-Werror syntax check passed. Final acceptance tests/review pending. Owner superseded secret denylist/base64/read-output caps before implementation; actual implementation is permission-based visibility, dynamic text and URL-only BLOBs.

### Read-tools found defects and fixes
FND-20261103191737-ccab-mcp-agent-002 (independent source review, medium functional): initial list_properties snapshot retained only the property header and omitted item schema, violating the original contract. Snapshot now retains item metadata without duplicating long-text values, and serializer lists item names/labels plus numeric min/max/step/format without values. Independent regression/retest pending; no false passing claim.

### Read-tools final acceptance evidence
Working directory /Users/polakovic/Development/indigo; task branch, HEAD82e884b80 plus uncommitted candidate. Source Git blob d4059a59cf626b707d4849e4050851a2116cb146; test bloba0bea6a7878245a76e14d6c66c2aaa872f693ccf.

- make -C indigo_drivers/agent_mcp -f ../../Makefile.drv all: universal macOS archive/dylib pass (also rebuilt by final runner).
- cc -std=gnu11 -DINDIGO_MACOS -Iindigo_libs -Ibuild/include -Wall -Wextra -Werror -Wno-unused-parameter -fsyntax-only indigo_drivers/agent_mcp/indigo_agent_mcp.c: pass.
- Independent test writer built normal/sanitized Makefile targets and ran both:23/23 native and23/23 ASan/UBSan. Retained read-tools-logs/native.log and sanitize.log.
- python3 tools/run_driver_test.py agent_mcp: exit0, final recorded23/23, README and generated TEST_SUMMARY updated; read-tools-logs/recorded.log.
- New regressions cover item schema, RO/WO/hidden and readable PASSWORD/token values, partial numbers and targets,600KiB dynamic text with >1MiB complete response and structured/text JSON equality, BLOB remote URL nofetch and local source-pointer URL with actual HTTP GET, switch/light/redefine, delete/reload, Unicode escaped names,20 controlled concurrent update/read rounds and invalid arguments.
- git diff --check, plutil -lint indigo.xcodeproj/project.pbxproj, configured Factory check/security: pass. Existing TCP source and public header blobs remain cdcb139b602745970418b0cfdf2596ebc5578201 /6bb7957b4993bd0c845d1e20aebfd7dc3245c717; no further framework delta.

FND002 item-schema regression now passes natively and under sanitizers; independent source reviewer confirmed the fix. Initial expanded-run failures also exposed fixture errors: automatic base CONFIG load affected the synthetic change counter, remote URL-only BLOB with nonzero size/NULLvalue was invalid for bus caching on update (use definition), and the numeric test assumed target0 was absent. The direct C bus has no target-presence flag, so explicit0 must replace prior22; independent reviewer confirmed correcting that expectation to assert distinct33 then explicit0. No production workaround/test weakening; actual untouched items/targets remain covered. Native JSON update emits targets only for non-RO, so that wire serializer is not evidence of a target-presence flag on direct bus items.

Final read-tools test summary:23 simulated cases run/passed; same23 passed under ASan/UBSan; hardware0 run/0 passed. Real Codex/Claude GUI and Linux/Windows execution remain unverified. This is an uncommitted read-tools increment for owner review, not full-task delivery/archival or a maintenance audit.

Final-candidate note: test writer finalized the explicit-zero target assertion after the first recorded runner completed. Final source/test binaries and both writer logs include that correction. The mandatory recorded runner was repeated after test ownership release and passed23/23 (read-tools-logs/recorded.log replaced by this final run). No production change or expanded test scope occurred.

## Write-tools increment — baseline and found defects
Original checkout /Users/polakovic/Development/indigo, HEAD82e884b80 plus uncommitted prior read/POST candidate. Existing23/23 native baseline passed before source edits (make -C indigo_test build/integration/test_agent_mcp; indigo_test/build/integration/test_agent_mcp, local sockets escalated). No hardware planned/run. New scalarwrite source strict -Wall/-Wextra/-Werror compile and universal archive/dylib build pass.

FND-20261103191737-ccab-mcp-agent-003: independent numeric audit found indigo_atod intermediate scaling can reject valid finite0.1e309 or silently turn a tiny-mantissa/e400 value into0 instead of1. Testwriter reproduced29/30 failure at the first finite-regression vector; baseline log retained in write-tools-logs/fnd003-baseline.log. Fix uses standard-C strtod with JSON decimal-point normalized to the current process locale, complete-consumption and finite checks. No process/thread locale mutation or OS-dependent driver code. Finite underflow/subnormal rounded results remain allowed. Assumes no concurrent external setlocale mutation, consistent with existing libc locale use. Final regressions pending.

### Write-tools final acceptance
Working directory /Users/polakovic/Development/indigo, same uncommitted task candidate.

- Independent writer: make -C indigo_test build/integration/test_agent_mcp build/integration/test_agent_mcp_sanitize, then both binaries:30/30 native and30/30 ASan/UBSan, exit0, no diagnostics. Retained write-tools-logs/build.log, native.log, sanitize.log.
- Final mandatory python3 tools/run_driver_test.py agent_mcp after test/source ownership release: exit0,30/30 recorded; write-tools-logs/recorded.log. README/generated TEST_SUMMARY reflect final run, MIGRATION_STATUS30/0.
- Production strict compile and make -C indigo_drivers/agent_mcp -f ../../Makefile.drv all passed; final runner rebuilds universal macOS module. No framework changes beyond the preserved earlier POST delta.
- New7 cases cover WO metadata discovery with value denial; typed partial text/number/switch dispatch; BUSY/later observed OK rather than completion claim;24KiB Unicode text; malformed/unknown/duplicate/empty/RO/hidden/light/BLOB writes with zero target effects; explicit64bit device/master-token native checks with no automaticinheritance; own CONNECTION disable; same-stack unload BUSY then healthy endpoint.
- FND003 regressions passed:0.1e309 produces1e308, a tiny mantissa scaled bye400 produces1, nonfinite values rejected. A comma-decimal locale was available and the supplemental1.25 assertion executed. Independent source reviewer confirmed strtod normalization/full-consumption/finite fix; finding fixed/retested.

Initial30-case run29/30 also found an invalid test expectation:9hex digits are valid for uint64 indigo_token; overflow fixture corrected to17digits after independent source review, and full64bit token accepted. This was a test design correction, no production workaround. Dedicated escaped-item-alias duplicate runtime case not added; source decoded comparison reviewed, literal duplicate and otherUnicode/escapedkey runtime coverage retained.

Native bus semantics retained: INDIGO_OK means dispatch attempt, not token acceptance, target existence or physical completion. Explicit token0 may be treated as the master when globalmaster is unset, according to existing bus code; no new authorization guarantee is claimed. Agent suppresses HTTP content logging and adds no write-value log, while opt-in native BUS_TRACE may log request values/tokens. No change to frameworklogging/auth/native numericvalidation. Driver callbacks remain synchronous according to existing bus semantics, so legacy blocking callbacks can delay the response. Same-stack self-unload guard is not a proof against arbitrary cross-thread cycles.

Final write increment summary:30 simulated cases run/passed, same30 passed under ASan/UBSan; hardware0/0. No real Codex/Claude GUI, Linux/Windows runtime, allocation-fault injection or ThreadSanitizer claim. BLOB write transfer remains explicitly unsupported; read URLs remain supported. Scoped quality/security review retained separately; full task remains active/uncommitted for owner inspection.

## Installed-client interoperability — 2026-11-04
Owner explicitly requested Codex and Claude GUI verification without commit, then required actual INDIGO simulators. Parent manualverifier previouslyauthored implementation; separate reviewer inspects evidence/docs, no independentGUIexecution claimed. Source remains c484697a32924607c0861625e6999ed7ff98eea4, test75da8e53e9102ff7f1f91ae2c2a4ff1c3f9e2a50; same originalcheckout/taskbranch/HEAD82e884b80. No production/test change was necessary.

### Fixture and safety scope
Actual indigo_ccd_simulator and indigo_mount_simulator archives, current MCPagent/TCP sources and libindigo linked into temporary native helper. Focused simulator archive builds passed/up-to-date. Commands/build/source: client-verification/build.sh, build.log, ccd-build.log, mount-build.log, simulator-fixture.c. The initially prepared fake fixture was stopped before either client ran; all reportedclientresults use actualsimulators. Privatehome via existing testhelper, Bonjourdisabled, ephemeralport56512, requested URLhttp://127.0.0.1:56512/mcp. Default existing server binding reused; no public tunnel/hosting/deployment. Only setupchange enabled MCP.CONNECTION; simulator devices remaineddisconnected. No hardware/privatecredential data was exposed or physical driver loaded.

### Codex
Installed codex-cli0.166.0-alpha.5.1, invoked via normal authenticated client, inheritedconfiguredmodel/settings with no forcedmodel/provider switch or separateAPIkey. Exactargv/prompt in client-verification/codex-command.json and codex-prompt.txt. Session-only HTTP MCP configuration override and3-read-tool allowlist, ephemeral/read-only execution, unrelated localMCP serversdisabled only for invocation. CLIexit0. Actual item.completed mcp_tool_call events verified: list_devices({}), list_properties(device=CCD Imager Simulator), get_property(device=CCD Imager Simulator,property=INFO). All3 statuscompleted/errornull, and decodedcontentJSON==structured_content. Retained actualcallresults: client-verification/codex-calls.json; finalclientresponse: codex-result.txt.13device names include camera/mount simulatorfamilies, camera INFO driverindigo_ccd_simulator/version3.0.0.46/interface2. No shell/files/browser/set_property call in clienttrace. Codex test is CLI, not its GUI; native CUA blocked access to the currentCodexapp for safety, and no UIbypass attempted.

### Claude GUI
Installed ClaudeDesktop4.16336.0, local Code tab, existingOpus5.5Medium selection unchanged. Opened isolated temporaryworkspace with typehttp project .mcp.json. No installation/stdio bridge/globalMCPentry needed. Actual nativeCUA GUI showed1connectedconnector and4executed MCP calls: same3readcalls plus get_property(Mount Simulator,INFO). Expandedtool trace displayed actual Input Device=Mount Simulator,Property=INFO and JSON output driverindigo_mount_simulator/version3.0.0.26/interface1, stateOk/permissionro. Camera result matches Codex. Parent inspected executedtool list/results in GUI, beyond the model's summary. Scoped observation: client-verification/claude-gui-observation.md; configuration: claude-project-config.json. All4 calls succeeded; no othertool/property action shown. This verifies DesktopCodeGUI, not DesktopChat or cloud connector.

### Documentation and cleanup
Corrected README's overly broadDesktopbridge claim: localCode .mcp.json supportsHTTP, while Chat localprocess integration needs compatibletransport; primarysource https://code.claude.com/docs/en/desktop#shared-configuration and https://code.claude.com/docs/en/mcp#project-scope . Codex HTTP configuration confirmed against https://learn.chatgpt.com/docs/extend/mcp?surface=app . Manualclienttestrecords3/3 and4/4 added to README and TEST_SUMMARY regenerated; automatedportablecasecount stays30/0 because these are manualclient checks, not addedINTEGRATION_TESTS. Native/sanitizer30/30 evidence retained from unchangedcandidate; no redundantapplication/simulator/hardware suite run.

Helper gracefullystoppedexit0, PID62638/listener56512 absent, privatehome removed (shutdown.log). Removed temporaryproject .mcp.json; no persistentindigo_smoke entry added to either userclientconfig. CompletedClaudeGUItestsession remains available for ownerinspection; no unrelatedsession/navigation/Xcode edits. cleanup.json records actual cleanup.

Finalclientchecksummary: Codex3/3 and ClaudeDesktopCodeGUI4/4 readcalls passed against actualsoftware simulators. Hardware0/0. Realclientset_property/BLOBdownload/DesktopChat/otherOS remain untested; no fulltask/archive/auditcompletion, commit or push claimed.

### Real-client text-write/readback/restore extension
To verify newlyadded set_property with the actualclients, added a reversible textcycle on real CCDsimulator after initialreadphasecleanup. Newhelper PID76091 reused nowfree56512 with freshprivatehome; productsource/test unchanged. No otherpropertytarget, connect/exposure/motion or explicitCONFIG request.

Codex additional5/5 actualcompleted/errornull calls: getPROFILE_NAME initiallyProfile #0; setNAME_0=MCP Codex verification; getconfirmed; setNAME_0=Profile #0; getconfirmed. Parent parsed actual JSONevents, verified text==structured_content for allresults, full first/finalitems arrays equal, no command_execution. Exactcommand/prompt/result/calls: client-verification/codex-write-*. Bothsetresults dispatch-only, readbacks prove appliedchange/restoration. Ephemeralclientoverride approved only its set_property against the isolatedsimulator endpoint; no persistentapproval/configchange.

ClaudeGUI additional5/5 calls in same localCode session: getinitialProfile #0, setMCP Claude verification, getconfirmed, setProfile #0, getrestored. ExpandednativeGUItrace shows initialget then set/get/set/get. Parent inspected actualintermediate/final JSON values, not only modelsummary; NAME_1..4 unchanged. See appendedwritephase in claude-gui-observation.md.

Total realclientcampaign: Codex8/8 and ClaudeDesktopCodeGUI9/9, all4 MCPtools exercised; textwrites only. ManualREADMErecords replaced perconfiguration with8/8 and9/9, TESTSUMMARY regenerated. Portableautomatedcount remains30/0. Source c484697a/test75da8e53 unchanged, so no redundantnative/sanitizer rerun after these runtime-only checks.

PROFILE_NAME nativebasehandler automaticallycallsindigo_save_property (indigo_driver.c:1666-1684). Therefore 'no CONFIGsave request' must not be read as zero filesystemwrites: nativeprofiledata was saved in the privatefixturehome. Childpositivelyidentified /private/tmp/indigo-test-home.2ccwb8 (created15:49:45.551459), .indigo/CCD_Imager_Simulator.common size303/mtime15:52:08.483609, without reading unrelatedhomecontent/environmentsecrets. Actualhome/process/listener removed by cleanstop0. No observeduserconfigurationwrite.

FND-20261103191737-ccab-mcp-agent-004 (mediumsecurity, temporaryfixture reuse): helperignoredfalse fromindigo_test_use_private_home. A futureisolation-setup failure could let nativeprofileautosave use realHOME. As-executedread/writefixture artifacts preservedunchanged; separatelyretained guarded-simulator-fixture.c nowchecks helperresult beforebusinit andprintsprivatehome. Rebuiltguardedhelper positiveboot on56512 printed /tmp/indigo-test-home.BPRoa2, thenimmediatequitexit0; PID92952/listener/homeabsent. No client/modelcycle repeated solelyfor this startupguard, productsource unaffected. Exactfixture/guardcleanup proof write-shutdown.log; independentreviewpendingfinaldisposition.

Final cleanup replaced the initialreadphase-only cleanup record with bothphases andguardcheck in cleanup.json; temporaryproject .mcp.json/README removed, no persistentuserMCPentriesadded. CompletedGUItestsession retainedforownerinspection. Finalrealclientwrites/BLOB/hardwarelimits are stated in README/report; BLOBdownload/numeric/switchwrite/Chat/otherOS remain untestedthroughtheseclients.

## Final commit candidate — 2026-11-04
Finaltoken-copy portabilityfix replacesPOSIXstrndup with checkedmalloc(length+1), memcpy andNUL termination. FND-20261103191737-ccab-mcp-agent-005 is a mediumfunctional source-audit Windowsportability finding: v143/stdc11 target has no strndupcompatibility declaration/implementation. No MSVC execution claimed. Agent is a newinitialrelease in thiscommit, retains initial3.0.0.0; no previouslycommitteddriverbehavior changed. Independentreview confirmed exactbytecopy/failure/lifetime equivalence.

- make -C indigo_test build/integration/test_agent_mcp build/integration/test_agent_mcp_sanitize: compiledsuccessfully; commit-logs/build.log.
- Final native30/30 and ASan/UBSan30/30, exit0, no diagnostics; commit-logs/native.log andsanitize.log.
- Mandatory python3 tools/run_driver_test.py agent_mcp: exit0,30/30; commit-logs/recorded.log, universalagent rebuild and finalREADME/generatedsummary record. Existing manualclient8/8 and9/9 records retained.
- Strictcc -std=gnu11 -DINDIGO_MACOS -Iindigo_libs -Ibuild/include -Wall -Wextra -Werror -Wno-unused-parameter -fsyntax-only indigo_drivers/agent_mcp/indigo_agent_mcp.c: exit0.

Finalsource Gitblob13693451ba2243c6f5493b17062a34b7112b9672; test75da8e53e9102ff7f1f91ae2c2a4ff1c3f9e2a50. Onlyequivalentnumeric-tokenallocation changed after actualclientchecks, so no unrelatedGUI/hardware rerun. FinalREADME3500e9b42c0a9173f7141ce050b140ca91fff337 andTESTSUMMARY6640eb662f21c7f10194a57291109b309e1639b5 before archival. Portableautomatedcasecount30/0 remains unchanged.

Owner requests localcommit only. Wholeacceptedcurrentfeature reports supersede earlierincremental pending statements, which remain historical evidence; alltaskfindings resolved and finalreview refresh precedes staging/archival. ConfiguredFactory check/security and committed-rangeprocessgate are still required on finalretainedcandidate; postcommitresult recorded externally ratherthan inserting a self-referential commitSHA. Factory unittest suite not repeated because no workflowhelper/testlogic changes; requiredapplicationregressions and configuredboundarychecks are the relevant coverage.

## Evidence packaging for Git retention
Raw execution logs are losslessly stored as .log.gz snapshots; historical command lines and plain logical log names remain unchanged. evidence-retention.json maps all original names to storedpaths and SHA-256 of exact uncompressed bytes. Every gzip decompression was byte-compared before rawfile removal. Markdown log links nowtarget .log.gz. The superseded unifieddiff is similarly retained as source.patch.gz with its uncompressed SHA recorded in handoff.md. This avoids treating output/context whitespace as sourceformatting, without weakening sourcechecks or changing executionevidence.
