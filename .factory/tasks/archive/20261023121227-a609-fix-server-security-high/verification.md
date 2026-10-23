# Verification evidence

Verdict: pass

Coordinator baseline verification, source base 989ec8db122f65ea3813af9db13d510d18bee90e, working directory /Users/polakop/Development/indigo. macOS arm64, Apple clang 21.0.0; native Makefile builds universal x86_64/arm64 binaries. Coordinator did not implement management/parser/transport patches, but owns the small Alpaca consumer adaptation and test integration; final independent security review remains required.

## Baseline before product changes

- `make -C indigo_libs -j4 libindigo`: exit 0, source-current native library and client variant rebuilt. No prohibited driver build performed. Full log /private/tmp/indigo-security-baseline-build.log (temporary working evidence).
- `make -C indigo_test test-unit` in sandbox: exit 2; nine uni_io cases failed due to restricted socket/serial operations. Retried with approved unrestricted execution: exit 2 with one pre-existing failure, `serial_open_reports_not_a_serial_port`, expected `Not a serial port`, actual `Serial device is not connected` from /dev/null exclusive-open ENOTSUP on this macOS. Protocol XML/JSON, bus and preceding unit suites passed. This is baseline broader-suite failure, not a passing check or a security repair regression; no test expectation changed.
- `make -C indigo_drivers/agent_alpaca -f ../../Makefile.drv`: exit 0. Synthetic linked image-array 2×2 MONO8 characterization compiled against baseline bridge/library and passed 1/1 after harness setup enabled BLOB caching. Initial harness compilation errors (incorrect property signature/assert macro) and first missing-cache setup failure were corrected before production edits; they are test development issues, not reproduced product defects.
- `./.factory/scripts/check`: exit 0, findings/decisions/project boundaries valid.
- `./.factory/scripts/test`: exit 0, 175 Factory tests passed in 142.777 seconds.
- `./.factory/scripts/security`: exit 0, local Factory boundaries valid. This is not application vulnerability coverage.

## Management integration

Integrated delegated management patch; `python3 indigo_test/security/run_server_management_security.py --sanitize`: exit 0, 12/12 checks pass, no sanitizer diagnostics. No actual helper executed. `clang -std=gnu11 -fsyntax-only -DINDIGO_MACOS -Duint=unsigned -DRPI_MANAGEMENT -Iindigo_libs -Iindigo_drivers -Iindigo_mac_drivers -Ibuild/include indigo_server/indigo_server.c`: exit 0, RPI branch parsed. Native full server build (`make -C indigo_server`) also passed; final literal-helper suite through `make -C indigo_test test-server-management-security-sanitize` passed 12/12 after integration.

## Acceptance and combined candidate

Reviewed synchronized base: a final `git fetch origin` confirmed local master and origin/master still equal 989ec8db122f65ea3813af9db13d510d18bee90e; no base merge was needed. Product candidate is the staged diff (22 source/test/doc paths), identified by final-candidate-identities.json and later process.json Git identities. The working Xcode file additionally contains the owner's original unrelated reorder; the task's index contains only 20 lines adding new test file references/groups. Both plist versions passed `plutil -lint`. The original owner bytes were saved before registration; applying the same six-reference additions to that snapshot exactly reproduces the current working file. Owner changes are not staged or reviewed as task changes.

Coordinator executed the following on the combined source in the repository root:

- `make -C indigo_libs -j4 libindigo`: exit 0, native library/client static+dynamic universal rebuild.
- `make -C indigo_drivers/agent_alpaca -f ../../Makefile.drv`: exit 0, version 0x0300000F exceeds baseline 0x0300000E.
- `make -C indigo_server`: exit 0, linked native server with existing permitted simulator archives; no server process launched by this build.
- `make -C indigo_test -j4 test-server-security-sanitize`: exit 0 with scoped permission for the synthetic HTTP listener. ASan/UBSan instrument the four repaired core compilation units and the test harnesses; unchanged static archive dependencies are not fully instrumented. Protocol security 13/13, cache 7/7, HTTP 7/7 and injected-allocation cache variant 8/8 passed. Small compile-time limits of 64/128 bytes and 0.2/1-second deadlines exercise limits without large allocations or indefinite stalls.
- `make -C indigo_test -j4 test-server-security-sanitize SECURITY_ASSERT_FLAGS=-DNDEBUG SECURITY_BUILD=build/security-ndebug`: exit 0 with the same four passing suites, confirming release-mode rejection paths.
- `make -C indigo_test test-agent-alpaca-cache build/unit/test_protocol_security build/unit/test_transport_cache_security build/unit/test_protocol_xml build/unit/test_protocol_json`: exit 0; synthetic Alpaca image-array characterization 1/1 passed.
- From indigo_test/, Python subprocess ran `build/unit/test_protocol_security`, `test_transport_cache_security`, `test_protocol_xml`, `test_protocol_json`, `test_base64`, `test_bus_property` and `test_bus_helpers`: all seven exit 0. Existing XML/JSON counts 8/8 and 6/6; new protocol/cache counts 13/13 and 7/7.
- `python3 tools/run_driver_test.py agent_alpaca`: exit 0; final recorded non-interactive hardware-free driver run 1/1, version 3.0.0.15, mac arm64. README Testing and TEST_SUMMARY updated by the runner under root AGENTS.md's explicit test-record exception. An earlier invocation using `--target` was rejected before execution because that option is hardware-only; the Makefile registration was corrected and the ordinary runner then succeeded. No script workaround, test weakening or real hardware operation occurred.
- `make -C indigo_test test-server-management-security-sanitize`: exit 0, management 12/12.
- `INDIGO_TEST_CASE_FILTER=serial_open_reports_not_a_serial_port indigo_test/build/unit/test_uni_io` with the same unrestricted permissions: exit 1, identical baseline /dev/null message mismatch. Broader test-unit remains known failing on this host; this is not represented as a pass, waived requirement, or introduced regression. The affected acceptance and configured Factory checks are independent of that pre-existing I/O diagnostic expectation.
- `git diff --cached --check` and `git diff --check`: pass after removing one introduced Makefile terminal blank line.

The independent management subagent separately inspected and reran transport/parser/Alpaca regressions against the integrated source, with matching counts and no sanitizer diagnostics. It excluded its own management implementation from independent approval. Coordinator independently reviewed management validation/quoting and literal format handling and repeated its regression/full RPI source syntax checks. See security.md, quality-review.md and their retained delegated reports for independence and final review disposition.

| Acceptance | Result and evidence |
| --- | --- |
| AC-001 | Pass: invalid framing, per-upload and aggregate limits, injected allocation failure/budget restoration, empty body. |
| AC-002 | Pass: short/EOF failures preserve old cache; concurrent GET and bus publication remain responsive; progress/absolute timeouts; retirement rejects publication. |
| AC-003 | Pass: retain/release, all permissions, move/shrink, concurrent lookup/retirement; XML and Alpaca consumers migrated without public struct layout change. |
| AC-004 | Pass: management12 verifies canonical choices/single quoted argument/unknown rejection and literal percent diagnostics; full RPI source parses. |
| AC-005 | Pass: XML attribute/name/entity exact/overflow/malformed boundaries. |
| AC-006 | Pass: JSON token bounds, exact WebSocket reads and uint64 lengths, full-capacity message, controls/fragmentation and serialized control output. |
| AC-007 | Pass: inbound response-only XML verbs rejected before handler execution. |
| AC-008 | Pass: distinct inline item storage across large/small ordering, both protocol versions, strict padding/declared-size mismatch/truncated second item/empty update. |
| AC-009 | Pass: existing XML/JSON/BLOB/Alpaca byte-order contracts, no token policy/property/generator/vendor/prohibited-driver changes, owner Xcode delta preserved outside index. |
| Coupled XML message repair | Pass: percent-containing define/update/message/delete notifications remain literal; missing message retains NULL. |

## Configured gates and limits

Final configured check/test/security and archival/process-gate results are recorded below when executed. Temporary full logs remain under /private/tmp/indigo-security-*; durable summaries and exact developer commands are retained in [transport](transport-developer.md), [parsers](parsers-developer.md) and [management](management-developer.md). Baseline reproductions in those reports are developer evidence, not an independent exploit certification.

No Linux/Windows/32-bit runtime, physical hardware, full ConformU, TSan, exhaustive fuzzing, or full-library leak/sanitizer certification. New nonblocking FND-20261023121227-a609-fix-server-security-high-002 tracks the pre-existing XML cached RO payload leak separately; remaining lower-priority audit findings stay active. No residual risk is accepted and no release/deployment approval is implied.


Final pre-commit configured test gate: `./.factory/scripts/test` exited 0, 175/175 Factory tests in 133.992 seconds. Final check/security and committed-range process results are delivery gates; final committed evidence is verified from a clean detached worktree so the owner's unrelated Xcode working changes need not be moved or discarded. Product code was tested from the coordinator checkout with exact staged identities; Xcode registration itself was plist-validated in both intended and owner-combined forms.


Final pre-archive `./.factory/scripts/check` and `./.factory/scripts/security`: both exit 0 after canonical reconciliation; findings validator reports 105 active and 38 archived, including all 14 verified closures and the retained new medium follow-up. Decision index and project boundaries pass. Task execution gates are satisfied for the repaired scope; the unrelated pre-existing broader native unit failure remains recorded above.


Archival retained the completed task and emitted its durable completion record. Post-move findings validation passes (105 active,38 archived); maintenance derives2/5 uncovered tasks and no due full audit. Archiver changed exactly two live Markdown links (project snapshot and Alpaca REFACTOR) to the archived evidence. Coordinator reviewed those link-only changes; tested behavior is unchanged. `make -C indigo_test test-clean` passed after the final pre-commit test runs. Final immutable commit SHA and post-commit checks are reported externally without a bookkeeping-only follow-up commit.


## Delivery candidate reconciliation

Owner explicitly authorized merging to master, pushing and deleting the completed task branch in the follow-up conversation. The owner added commit c35c056edd5d6cf3ff7fa387199cd49f8fcf16e5 after the previously verified e044bf7d0 candidate. Only indigo.xcodeproj/project.pbxproj changed. A parsed JSON object comparison using plutil showed exactly five removed PBXFileReference objects for obsolete FND-000001 through FND-000005 entries and removal of those five IDs from the active findings PBXGroup children. All remaining objects, build settings/phases and task test registrations are semantically unchanged; most textual churn is object ordering. The working tree was clean at delivery start. No C/header/test source changed, so previous native/sanitizer/175-test evidence still binds those exact identities. Reviewed-content and SHA256 identities now include the owner's cleaned project file. This evidence refresh is a justified additional commit for a new owner-authored candidate delta, not a rewrite of prior history or a repeated passing-check-only commit.

Origin fetch confirmed master/origin/master remain 989ec8db122f65ea3813af9db13d510d18bee90e. Final candidate delivery checks are recorded externally after commit, with no source changes planned.
