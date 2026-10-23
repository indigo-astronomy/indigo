# Independent security review and verification

Task: 20261023121227-a609-fix-server-security-high. Reviewer role: independently assigned verifier/security-reviewer; requested strong gpt-6-astra/high, actual model settings not independently observable. Date: 2026-10-23, macOS arm64, Apple clang 21.0.0. This reviewer implemented management fixes earlier, so **management source and management regressions are excluded from this independent verdict**; coordinator owns their independent review. Reviewer did not author transport, parser, Alpaca, shared test registration or their documentation. No production/source/test/task/registry edits during this review role.

**Verdict: PASS within the independent scope stated above.** Product-source review, independent regressions and final staged documentation review are complete with no unresolved blocking defect. The final identity addendum binds this verdict to reviewed content.

## Candidate and method

Reviewed uncommitted task candidate in `/Users/polakop/Development/indigo`. HEAD, local master and origin/master all resolve to `989ec8db122f65ea3813af9db13d510d18bee90e` at review; no committed final task revision yet. Inspected actual implementation and untracked tests, surrounding ownership/locking/I/O contracts, applicable root/source/test/driver/docs rules, specification/test plan and retained targeted audit reports. Loaded verifier, security-reviewer, quality-reviewer and quality-review routine plus its evidence checklist. No archive discovery beyond explicitly assigned retained reports. Candidate manifest `/private/tmp/indigo-security-candidate-identities.json` initially matched all 18 supplied paths; three management paths excluded from independent scope. Initial manifest SHA-256: `bf604af11ad3ba542f872bec91791afce1b4f7cf19e263c1a975c5229f87ada4`. Only subsequent observed mismatch was expected Makefile EOF whitespace cleanup; product source identities stayed unchanged.

Reviewed independent identities at runtime:

| File | SHA-256 |
| --- | --- |
| `indigo_docs/INDIGO_SERVER_AND_DRIVERS_GUIDE.md` | `9a00971ace4f898ee55432d1f0d706214b6099114e703cde58b77858a0e41c10` |
| `indigo_docs/PROTOCOLS.md` | `75eb8390b40094a69c1d986d0a0b3d21ede684c67dca0a03a5bd9f89c26d175f` |
| `indigo_drivers/agent_alpaca/REFACTOR.md` | `1c37b1bc91d9d44ba2e48ba766f6b4ec23f698d70afd56917be46cebb3d46ae7` |
| `indigo_drivers/agent_alpaca/indigo_agent_alpaca.c` | `0bccaec257c22bac12acc79b66721694ecab6a6fdbb4bacac6e622f8b529ca5b` |
| `indigo_drivers/agent_alpaca/indigo_alpaca_ccd.c` | `4aa0f2d1e7d52672f123271079911da7a2fc363f9a708891fc994a7a72c36def` |
| `indigo_libs/indigo/indigo_bus.h` | `6e9dc2fe175afa77d89c10f5620add5667554b8ef2439be68df3e55597f856b6` |
| `indigo_libs/indigo_bus.c` | `6a8e7b3fef4cea0bb79d1a9c7209d991b1de9f6864986cd87fd876697a61ae3a` |
| `indigo_libs/indigo_json.c` | `809c1df6145d13b4939098c71ee0383b74001c7a69db254ae35babf088c8cc6a` |
| `indigo_libs/indigo_server_tcp.c` | `eda58fa6a40f8d43578cbca06a39f95a68bab873fac1345792febe464fac9125` |
| `indigo_libs/indigo_xml.c` | `fad9ca4f2c5ae712c6195ec7bfeb769f30e85962d47dd2b76e15e0f5d262c17b` |
| `indigo_test/Makefile` | `c577c6dc7cc2f6e1e41059b460995772a5df5e82818bb8ce367ef3b930875c5a` |
| `indigo_test/integration/test_agent_alpaca_cache.c` | `579295403214927a53697d75685cd3a96ad21388e489d1cf7d41f6123b4e25a0` |
| `indigo_test/integration/test_transport_http_security.c` | `609c97e772d268a4f6f4217362c7be71e04ea3d7dedf13b05d9383b207a8bc96` |
| `indigo_test/unit/test_protocol_security.c` | `89b6cd1b1ddfbfe14ee1da33cccb9ccd3019cd42359b64236bfe955412081e6a` |
| `indigo_test/unit/test_transport_cache_security.c` | `0e4802e194d6c82a88420cd903fff8a7debae7f96ecce779b3c7cd650dde41b6` |

## Acceptance and trust-boundary results

- AC-001 (audit 001): PASS within reviewed scope. HTTP lengths are parsed as bounded decimal, reject duplicate/conflicting/unsupported framing, and reserve recoverable upload memory. Empty uploads have defined behavior. Pending and published upload memory share the budget, with malloc failure releasing the charge. Runtime malformed/empty/over-limit/budget/allocation-failure cases passed using small test budgets.
- AC-002 (audit 002/005): PASS. Body receive is private, has progress and total deadlines, holds no cache lock over input, and publishes only after exact completion. Partial EOF, stalled and trickled bodies preserve prior bytes; same-cache and unrelated bus publication remain responsive. Retirement during receive fails publication safely.
- AC-003 (audit 004): PASS for maintained consumers. Registry acquisition retains entries before unlock; retirement covers all permissions; resize refreshes retained entries and shrinking retires removed items. Cache metadata copies are independent of original property/item lifetime. HTTP GET snapshots, XML upload reads and Alpaca image-array conversion use retained lifetimes and release after unlocking. Legacy borrowed APIs remain ABI-compatible with explicit external-serialization requirement. Tests include retained data after property release, permission changes, resize/shrink, 50 bounded concurrent retirement iterations, and registry rejection after retirement.
- AC-004: NOT independently evaluated here (management author exclusion).
- AC-005 (audit 011/012): PASS. XML append/terminator capacity checks survive release builds; named-entity recognition is bounded and rejects unsupported/malformed entities without logging unterminated data. Boundary fixtures exercise both quote styles, names, attributes and entities.
- AC-006 (audit 013/017): PASS within supported framing scope. JSON reserves token/frame terminators. WebSocket lengths stay uint64_t until bounded against capacity and LONG_MAX; header/extension/mask/payload reads require exact completion. Masking, opcode/flags, text continuation, control lengths and aggregate message capacity are enforced. Ping/close responses use the shared JSON output lock and output handle. Tests cover truncated frames, oversized integer lengths, masked/unmasked messages, invalid flags/opcodes, full-capacity messages, text fragments with control traffic, and exact ping/close response bytes.
- AC-007 (audit 014): PASS. Inbound clients are restricted to request verbs before response-only handlers run. All 13 response forms are rejected in inbound direction; outbound request forms likewise rejected. Ordinary existing XML request/response fixtures still pass.
- AC-008 (audit 015/016): PASS for inline pending-item ownership. Per-item allocated buffers retain independent decoded data until dispatch/cleanup; decoding validates base64 alphabet/padding, declared size and capacity, including split input buffers. Temporary inline properties have cleanup ownership. Larger→smaller and smaller→larger payloads, malformed/truncated data, both protocol versions, and nonempty→empty updates pass. Pre-existing parser-owned cached RO payload leak is separately tracked below and is not claimed fixed.
- AC-009: PASS in reviewed diff. Existing XML/JSON behavior fixtures and Alpaca image-array ordering/transaction metadata pass. Public entry layout remains intact and APIs are additive. No reviewed driver property, token policy, generator, protected driver or vendor change. Safety snapshot and new byte/deadline limits are documented. Linux/Windows runtime not available; no cross-platform runtime claim.
- New task finding FND-20261023121227-a609-fix-server-security-high-001: repair verified. Upstream XML `message` data previously became a variadic format in set/define/delete/message helpers. All five forwarding sites now use conditional literal `%s`, preserving NULL format when no message is supplied. Linked public-adapter regression observes percent-bearing define/update/message/named-delete events literally, without evaluating hostile directives. Whole-device deletion forwarding was also reinspected at source level.

## Findings raised and fixed during review

1. Preliminary WebSocket pong implementation wrote header/payload outside json_mutex, permitting interleaving with asynchronous JSON output. Developer moved the shared mutex declaration and serialized the whole control write; reviewed final json.c:111–118 and independently passed control/framing tests. No remaining issue in that path; concurrency stress is not exhaustive proof.
2. New empty inline BLOB used a non-NULL one-byte allocation but cached set_property attempted realloc(p,0), potentially triggering the process-fatal allocation helper on platforms returning NULL. Developer added explicit positive-size condition; final xml.c:521–536 frees/clears empty payloads and avoids zero-size realloc. Independent nonempty→empty regression passes.
3. Independent review found the pre-existing upstream XML format string issue and coordinator authorized its coupled repair, registering task-local -001. Reinspection and regression result above verify the repair.
4. Pre-existing cached RO XML payload retirement leak remains open as task-local -002 (medium, nonblocking, coordinator owned): generic release intentionally frees WO payloads, while the parser owns cached RO payloads. Source-contract evidence is retained by the coordinator. This differs from temporary pending-item storage repaired here. No leak-sanitizer certification, risk acceptance, or whole-repository security approval is asserted.

## Independent commands and results

Unless stated otherwise, working directory `/Users/polakop/Development/indigo`, same uncommitted product identities above. Compiler output includes existing deprecated-sprintf warnings in unchanged functions; no new compile error or sanitizer diagnostic in successful runs.

1. `make -C indigo_test SECURITY_BUILD=build/security-review test-server-security-sanitize > /private/tmp/indigo-independent-sanitize.log 2>&1`: rebuilt the four changed core units with ASan/UBSan, frame pointers and small upload limits/deadlines, linked unchanged dependencies from source-current libindigo archive. Protocol 13/13 and cache 7/7 passed. Initial aggregate exit 2 because network sandbox denied listener bind (`Operation not permitted`); that HTTP attempt was not a passing test.
2. `./indigo_test/build/security-review/test_transport_http_security > /private/tmp/indigo-independent-http-sanitize.log 2>&1`, explicit owner-authorized sandbox escalation for temporary local test listener: exit 0, 7/7 passed. Synthetic bytes and loopback clients only, no third-party requests or real management operations.
3. `make -C indigo_test build/unit/test_protocol_xml build/unit/test_protocol_json build/unit/test_protocol_security build/unit/test_transport_cache_security test-agent-alpaca-cache > /private/tmp/indigo-independent-normal-build.log 2>&1`: exit 0; normal binaries current, Alpaca 1/1 passed.
4. Initial direct XML binary invocation from repository root failed 7 fixture/path assertions because that suite requires `indigo_test/` working directory. Logged in `/private/tmp/indigo-independent-normal-tests.log`; corrected without source or expectation changes. This is a verifier invocation error, not a product regression.
5. From `/Users/polakop/Development/indigo/indigo_test`, ran `./build/unit/test_protocol_xml`, `./build/unit/test_protocol_json`, `./build/unit/test_protocol_security`, `./build/unit/test_transport_cache_security` through Python subprocess with per-process 60-second bounds. Each exit 0; counts 8/8, 6/6, 13/13 and 7/7. Output `/private/tmp/indigo-independent-normal-tests-corrected.log`.
6. `make -C indigo_test SECURITY_BUILD=build/security-review build/security-review/test_transport_cache_alloc_failure > /private/tmp/indigo-independent-allocation.log 2>&1`: exit 0. From indigo_test, `./build/security-review/test_transport_cache_alloc_failure > /private/tmp/indigo-independent-allocation-run.log 2>&1`: exit 0, 8/8 passed, including recoverable malloc failure and restored aggregate charge.
7. `git diff --check`: first observation found a new terminal empty line in Makefile; coordinator removed it. Repeated check exit 0. No source behavior change was needed.
8. `make -C indigo_test SECURITY_BUILD=build/security-review-ndebug SECURITY_ASSERT_FLAGS=-DNDEBUG build/security-review-ndebug/test_protocol_security > /private/tmp/indigo-independent-ndebug-build.log 2>&1`: exit 0. From indigo_test, `./build/security-review-ndebug/test_protocol_security > /private/tmp/indigo-independent-ndebug-run.log 2>&1`: exit 0, 13/13 passed with assertions disabled and ASan/UBSan instrumentation. No sanitizer diagnostics.
9. Final SHA-256 comparison of candidate manifest and staged source used Python hashlib over `git show :<path>` for every supplied final path. Product source identities remain the same as runtime evidence. Final identity addendum below records completion.

## Limits and follow-up

No complete fuzzer, exhaustive scheduler/race exploration, TSan, leak check, Linux/Windows runtime, real RPI helper or hardware execution. Sanitizers instrument repaired core units and test harnesses; unchanged archive dependencies are not completely instrumented. Full native universal library and Alpaca rebuild success are coordinator evidence, not independently repeated build claims. Configured Factory checks/process gating and final commit/integration remain coordinator responsibilities; metadata validation alone cannot establish application safety. No deployment, push, merge, third-party contact or risk waiver was performed. Pre-existing lower-priority audit findings remain open, including writable staging policy, idle connection admission, download-file cleanup and total parser message budgets; this verdict is scoped to repaired high findings and the coupled XML format repair.

## Final staged candidate identity and verdict

**Verdict: PASS, scoped to independently reviewed transport/cache, parsers, Alpaca, build registration and their documentation; management author exclusion remains in force.** No unresolved blocking defect was found in this scope. This is not risk acceptance of separately tracked lower-priority findings.

Final staged review used `git diff --cached` and SHA-256 of `git show :<path>` from `/Users/polakop/Development/indigo`. Manifest `/private/tmp/indigo-security-candidate-identities-final.json` SHA-256 is `e9f2db59ed7f5baec02c3129c9a74738190f9599a55dd4ecb0c628f4c21b7845`. All 22 paths were checked. The only manifest delta is the reviewed REFACTOR wording/task-link correction recorded below; all behavioral source and regression identities remain unchanged from independently tested content. The following 19 path identities define this independent final review; the three excluded management paths were checked for identity only.

| File | Final staged SHA-256 |
| --- | --- |
| `MIGRATION_STATUS.md` | `6663667ea7dd9baf4e41b7136013722cc9e9018c5514690def66605f48a023a2` |
| `TEST_SUMMARY.md` | `b3f8b7f1aaa9e531a284b51eac4468edbfcf513d1964db3ce1d55055e301d7ab` |
| `indigo.xcodeproj/project.pbxproj` | `55970215dbfba52ee07dceaa3f27998070fd5dd973b3f2c8c11f9ac20f28373f` |
| `indigo_docs/INDIGO_SERVER_AND_DRIVERS_GUIDE.md` | `9a00971ace4f898ee55432d1f0d706214b6099114e703cde58b77858a0e41c10` |
| `indigo_docs/PROTOCOLS.md` | `75eb8390b40094a69c1d986d0a0b3d21ede684c67dca0a03a5bd9f89c26d175f` |
| `indigo_drivers/agent_alpaca/README.md` | `ea1454f3a6b5e1860b1264fd962ca660760f8022ae408b497632b279be69ab24` |
| `indigo_drivers/agent_alpaca/REFACTOR.md` | `1e9ba6588695d8b8681e06be53f8f7674c12fba00d367c3ab0ab262be38f05f3` |
| `indigo_drivers/agent_alpaca/indigo_agent_alpaca.c` | `0bccaec257c22bac12acc79b66721694ecab6a6fdbb4bacac6e622f8b529ca5b` |
| `indigo_drivers/agent_alpaca/indigo_alpaca_ccd.c` | `4aa0f2d1e7d52672f123271079911da7a2fc363f9a708891fc994a7a72c36def` |
| `indigo_libs/indigo/indigo_bus.h` | `6e9dc2fe175afa77d89c10f5620add5667554b8ef2439be68df3e55597f856b6` |
| `indigo_libs/indigo_bus.c` | `6a8e7b3fef4cea0bb79d1a9c7209d991b1de9f6864986cd87fd876697a61ae3a` |
| `indigo_libs/indigo_json.c` | `809c1df6145d13b4939098c71ee0383b74001c7a69db254ae35babf088c8cc6a` |
| `indigo_libs/indigo_server_tcp.c` | `eda58fa6a40f8d43578cbca06a39f95a68bab873fac1345792febe464fac9125` |
| `indigo_libs/indigo_xml.c` | `fad9ca4f2c5ae712c6195ec7bfeb769f30e85962d47dd2b76e15e0f5d262c17b` |
| `indigo_test/Makefile` | `9cfc5f14eca00f291048f0937de79234520bc156b399ea3e3787f7a9555c0bfc` |
| `indigo_test/integration/test_agent_alpaca_cache.c` | `579295403214927a53697d75685cd3a96ad21388e489d1cf7d41f6123b4e25a0` |
| `indigo_test/integration/test_transport_http_security.c` | `609c97e772d268a4f6f4217362c7be71e04ea3d7dedf13b05d9383b207a8bc96` |
| `indigo_test/unit/test_protocol_security.c` | `89b6cd1b1ddfbfe14ee1da33cccb9ccd3019cd42359b64236bfe955412081e6a` |
| `indigo_test/unit/test_transport_cache_security.c` | `0e4802e194d6c82a88420cd903fff8a7debae7f96ecce779b3c7cd650dde41b6` |

Documentation reinspection confirmed the Alpaca automated count is 1/0 with its manual Comment preserved, README/TEST_SUMMARY record the coordinator's actual 1/1 runner result, and REFACTOR describes completed steps and 1/1 synthetic versus 0/0 hardware evidence without claiming a migration or broad conformance. Its corrected task link points to the active brief for the archiver to rewrite. The Xcode review used only the index's six added file references/group entries; the owner's pre-existing working-tree reorder was excluded. Final `git diff --cached --check` passed.

Final evidence reconciliation, archive placement/link rewriting, process gating and final committed-candidate identity confirmation remain coordinator responsibilities. No further production/test changes are anticipated; any such change invalidates the affected scope until reinspected/retested.

## Archive/documentation-only reinspection

2026-10-23: read-only reinspection of the final working candidate confirmed `.factory/project.md` states 13 repaired original high findings plus the coupled XML format repair, with five original lower-priority findings and the new medium cached-RO XML leak still open. The 14 targeted archived records are closed with fixed resolutions and retained origin/repair references; task-local -002 remains active, medium, nonblocking, with no resolution. Central security/quality wrappers preserve reviewer independence, management exclusion, scoped limits and pending committed-candidate gating. Relative Markdown links in those wrappers, the project snapshot and Alpaca REFACTOR resolve; REFACTOR now links to the archived brief. Archived copies of the frozen independent reports matched before this addendum. All source/test C, header and Python identities remain identical to the final tested manifest. No actual inconsistency found in this documentation/archive delta; scoped PASS is unchanged. No runtime rerun was warranted for these non-behavioral changes. Coordinator retains responsibility for staging, final process/commit identity and integration/publication gates.
