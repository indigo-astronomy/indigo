# Final MCP deliverable quality review

Verdict: pass

Date: 2026-11-04. Independent reviewer: `mcp_review` subagent, same host/provider, no implementation/test authorship. The same reviewer performed a separate security pass. Reviewed the final uncommitted candidate in the original checkout `/Users/polakovic/Development/indigo`, branch `task/20261103191737-ccab-mcp-agent`, over HEAD `82e884b80e3713d49051020b5d6a4a50fa80202d`. Owner now authorizes committing all MCP changes locally, with no push. This verdict is ready-for-local-commit review; it does not claim that a commit, main integration, publication or task archival has occurred.

## Final scope and evidence

Inspected the actual tracked diff and new agent/test/project/README deliverables, plus retained task/client evidence. The final feature is a default-disabled MCP 2025-11-25 agent on the existing HTTP listener, exact /mcp POST handling, discovery/read tools and scalar text/number/switch set_property. Visible WO schemas are discoverable without values. Writes cover owner-authorized visible RW/WO targets including server/agents and report dispatch only. The only framework delta remains POST through the existing PUT-style callback; the owner's header/comment reset is preserved. Necessary native/static server, Xcode, Windows and test registration/status rows are included. No off-limits-driver, generator, unrelated UI or additional framework API edit is present. The byte-identical jsmn relocation is already committed in the base and is not counted as a new final-diff change.

Detailed semantic reviews are retained in [read-tools-review.md](read-tools-review.md), [write-tools-review.md](write-tools-review.md), [client-review.md](client-review.md) and historical sections below. Final source audit found FND-20261103191737-ccab-mcp-agent-005, medium functional portability: numeric_value called strndup without an MSVC compatibility implementation in the v143/C11 target. The coordinator replaced only that bounded copy with malloc(length+1), memcpy and a NUL terminator. Reinspected allocation/failure/lifetime behavior: validated JSON-token length is bounded by input, exact bytes are preserved and no platform-specific code/core change is needed. This was source-audit evidence, not an observed Windows compilation failure; Windows compilation/runtime remain unverified. Microsoft documents the supported [strdup aliases](https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/strdup-wcsdup?view=msvc-170), while repository compatibility supplies strdup but no strndup wrapper.

Inspected fresh final [native](commit-logs/native.log.gz), [ASan/UBSan](commit-logs/sanitize.log.gz) and [recorded](commit-logs/recorded.log.gz) results:30/30 each, after the portability repair; recorded README time20:38. Coordinator also records strict compile/universal module build. Existing HTTP29/WebSocket5 evidence remains applicable to unchanged POST/core identities. Real-client evidence is Codex CLI8/8 and Claude Desktop Code9/9, including reversible text write/readback/restoration against actual INDIGO software simulators. Those client cycles ran on the immediately preceding source; they were not repeated for the equivalent numeric-token allocation copy. No claim of a new GUI execution on the final hash is made. See [verification.md](verification.md) for executor-attributed commands/results and limits.

Fresh reviewer checks: git diff --check, plutil lint, Windows XML source/header/project-reference path and GUID inspection, configured validate_project.py --security all passed. No runtime suite was rerun by this reviewer for final closure. No unresolved blocking finding identified. Canonical task findings001–004 were inspected closed/fixed in their archived records. FND005 is fixed/reverified by this final source pass and30-case retests; coordinator owns its final canonical closure/archive. No risk acceptance is substituted for a fix.

BLOB writes, SSE/session streams and broader protocol features are unsupported; client numeric/switch writes, BLOB downloads, Codex GUI, Claude Desktop Chat, Linux/Windows execution and physical hardware remain unverified as documented. Automated coverage remains30/0; manual client 8/9 call totals are separate. This is a final task-diff review, not a whole-repository audit or cadence reset. Coordinator retains process/archival/commit responsibilities.

## Final product content identities

Git blob IDs, independently read from the candidate:

| Path | Git blob |
| --- | --- |
| indigo_drivers/agent_mcp/indigo_agent_mcp.c | 13693451ba2243c6f5493b17062a34b7112b9672 |
| indigo_drivers/agent_mcp/indigo_agent_mcp.h | ed63ea5980d9af3837d0319207225ddffce92afc |
| indigo_drivers/agent_mcp/indigo_agent_mcp.vcxproj | 62106ce7b1b619a74131df0f704b6d37f8624d9e |
| indigo_drivers/agent_mcp/README.md | 3500e9b42c0a9173f7141ce050b140ca91fff337 |
| indigo_test/integration/test_agent_mcp.c | 75da8e53e9102ff7f1f91ae2c2a4ff1c3f9e2a50 |
| indigo_test/integration/test_transport_http_security.c | 1cec14c3ca8a58d4b2d3f4b91450f20621696fd4 |
| indigo_libs/indigo_server_tcp.c | cdcb139b602745970418b0cfdf2596ebc5578201 |
| indigo.xcodeproj/project.pbxproj | 931b6ad10c4474eb6b74d164d9603e63155cb8c2 |
| indigo_server/indigo_server.c | 3c39d310d96ac99cdcdbe9458e510ac73daa4979 |
| indigo_server/indigo_server.vcxproj | 245b13540a9e56d42234cd051b98f6d58c3f7d4a |
| indigo_windows.sln | 1a0862901a8525a9e8dc76a032d91d3094931a45 |
| indigo_test/Makefile | f71e313776423266d85624805e2e7ee000bf842b |
| MIGRATION_STATUS.md | 3de666fecb29b634629fe9e257b20ccbf6ad6d6d |
| TEST_SUMMARY.md | 6640eb662f21c7f10194a57291109b309e1639b5 |

## Historical scoped reviews

The following retained sections describe earlier increments and their then-current limits. Current scope/verdict above supersedes their provisional no-tools/no-commit statements; historical execution and identities are preserved.

---

# MCP skeleton increment quality review

Date: 2026-11-04
Reviewer: independent `mcp_review` subagent, same host/provider; no production, test, acceptance or integration edits.
Checkout: `/Users/polakovic/Development/indigo`, existing task/branch `20261103191737-ccab-mcp-agent`, uncommitted changes over `67a7e06fd91254322307860b0ff4e26fda216ebb`.
Historical verdict: **pass for the MCP protocol skeleton and retained POST dispatch increment**. No unresolved blocking quality finding. This is not acceptance of the full MCP feature, a repository audit, authorization to commit, or a client/platform certification.

## Reviewed content

Read actual tracked diff and untracked agent/test/project/README files, surrounding shared HTTP dispatch and retirement, agent lifecycle, private JSON tokenizer contract, platform registration and focused tests. Inspected latest task requirements/plan and root/scoped instructions. Public framework header and existing developer documentation remain unchanged; the owner’s comment reset is preserved. New Xcode membership is necessary for the new agent/test/README, and no Factory metadata was introduced.

Working-file Git blob identities below are content evidence, not staging or committed-candidate claims:

| Path | Reviewed identity |
| --- | --- |
| `indigo_drivers/agent_mcp/indigo_agent_mcp.c` | `e79b420944fb818fd22baf43f1e84b1bc9c4079d` |
| `indigo_drivers/agent_mcp/indigo_agent_mcp.h` | `ed63ea5980d9af3837d0319207225ddffce92afc` |
| `indigo_drivers/agent_mcp/indigo_agent_mcp.vcxproj` | `62106ce7b1b619a74131df0f704b6d37f8624d9e` |
| `indigo_drivers/agent_mcp/README.md` | `148b60cd4759747666eb31692aaa30e0e78618b9` |
| `indigo_test/integration/test_agent_mcp.c` | `566906950a4c0c3520e5b3065743e85582c50f50` |
| `indigo_test/Makefile` | `f71e313776423266d85624805e2e7ee000bf842b` |
| `indigo.xcodeproj/project.pbxproj` | `931b6ad10c4474eb6b74d164d9603e63155cb8c2` |
| `indigo_server/indigo_server.c` | `3c39d310d96ac99cdcdbe9458e510ac73daa4979` |
| `indigo_server/indigo_server.vcxproj` | `245b13540a9e56d42234cd051b98f6d58c3f7d4a` |
| `indigo_windows.sln` | `1a0862901a8525a9e8dc76a032d91d3094931a45` |
| `MIGRATION_STATUS.md` | `ede64d235e794bd5a58b399127632534388e0adc` |
| `TEST_SUMMARY.md` | `ed53e11378cd824bb2bdf183d61f1f4bb78ed436` |
| `indigo_libs/indigo_server_tcp.c` | `cdcb139b602745970418b0cfdf2596ebc5578201` |
| `indigo_test/integration/test_transport_http_security.c` | `1cec14c3ca8a58d4b2d3f4b91450f20621696fd4` |

## Design and test assessment

The implementation keeps MCP framing, validation and protocol behavior in one new hand-written agent. There is no new framework API or generic HTTP/parser subsystem. The small existing-handler POST change is retained unchanged (`cdcb139b...`), and its six tests remain unchanged (`1cec14c3...`) from the preceding independent `http_review` pass. This review rechecked the surrounding dispatch/cleanup and confirms the callback contract and built-in POST restrictions remain intact; earlier broader-design reports under superseded-http-api/ do not validate the current design.

The agent has a meaningful bounded skeleton: standard CONNECTION controls a default-disabled endpoint; no unnecessary property cache/client or device tools are added. Its private existing MIT tokenizer is followed by explicit grammar, UTF-8/surrogate and full-document validation, so tokenization success is not treated as JSON validity. Fixed limits cover body bytes, token count, nesting, ID size and admission; unknown fields do not introduce dynamic execution or bus access. Static mutex state is limited to singleton enable/admission lifecycle.

Tests cover enabled/default-disabled/idempotent/failed initialization, protocol/ID behavior, empty tools, invalid envelopes/escaped duplicate keys, malformed grammar/UTF-8, framing/media/origin/length errors, body/token/depth bounds, fragmented input, close/no-pipeline-replay, and admission/draining on unload. The unload test proves admission through 503 saturation and route withdrawal through 404 before asserting pending removal; it is not a sleep-only race assertion. Existing PUT/GET and WebSocket tests preserve the transport contract. Both new test targets list their directly included private tokenizer/header dependencies.

Platform integration is narrow: agent source membership in both INDIGO Xcode targets, navigator-only integration test, static-server registration and a standard Windows project/solution/server reference. The README and migration/test counts match the actual 14-case software-only run, retain a blank migration Comment column and distinguish a Windows project from executed Windows validation. No driver migration, hardware claim, new dependency installation or generator change is present.

## Review feedback and reinspection

FND-20261103191737-ccab-mcp-agent-001: source inspection found that an explicitly empty/whitespace-only MCP-Protocol-Version value was treated as absence during initialize/ping. The developer added a present-empty guard and a standalone whitespace-only header fixture; the final source/test identities above include the fix. Reinspection and the reviewer’s independent final ASan/UBSan run passed all 14 cases. The coordinator owns the canonical closed-finding reconciliation and exact found-defect/retest record in verification.md. No risk acceptance is needed or claimed.

## Evidence and limitations

[verification.md](verification.md) owns exact build/run evidence. Parent/test-writer evidence includes universal driver/archive build, recorded native 14/14 and final sanitizer 14/14, with existing HTTP 29/29 and WebSocket 5/5 evidence attributed to their executors. The reviewer independently ran `git diff --check`, `python3 -B .factory/scripts/validate_project.py --security` and `plutil -lint indigo.xcodeproj/project.pbxproj`; all passed. The reviewer also independently ran the final `indigo_test/build/integration/test_agent_mcp_sanitize` binary after verifying it was newer than the final source/test: 14/14, exit 0, no sanitizer diagnostic. Separate 4,535-record deterministic JSON grammar review with ASan/UBSan had zero mismatches against the Python JSON oracle under the declared Unicode policy; details are in security.md and are not additional registered driver cases.

Real Codex/Claude GUI sessions, the absent local Claude stdio bridge, Linux/Windows execution, whole-framework/application builds and hardware remain unverified. The first implementation deliberately has no authentication and no device access; access/confidentiality policy must be implemented before enabling data/control tools. Owner inspection, committed-range process evidence, eventual task closure/archive and integration/publication remain coordinator-owned and pending.

Final metadata reinspection: after behavioral verification, the developer added only the repository-style version-history comments to the two new agent C/header files. The table above contains their updated identities. Executable code is unchanged; the prior behavioral evidence remains applicable and no extra test run was needed. `git diff --check` passed again. The coordinator confirmed the fixed version-header finding was reconciled and archived.

## Shared tokenizer relocation follow-up — 2026-11-04

Owner authorized moving the existing tokenizer into the shared vendor tree. **Scoped pass; no new quality finding.** This follow-up inspects only relocation and direct include/build/project dependencies; it reuses the preceding MCP semantic review and does not reopen unrelated framework, image, HTTP or hardware work. The principal content table above is refreshed for the current include-path/project/README/test-record identities. Additional relocation identities follow:

| Path | Reviewed identity |
| --- | --- |
| `indigo_libs/externals/jsmn/jsmn.h` | `b3e2338c1f029df5bf75e62d9b7f73aa34ccc32a` |
| `indigo_drivers/shared/primalucelib/primalucelib.c` | `01a9f2dd85ac554b7c368bfd881377b544259069` |
| `indigo_drivers/shared/primalucelib/primalucelib.h` | `c4904e53ae0c4746e35ea778cf0348213a4de19e` |
| `indigo_drivers/shared/primalucelib/Makefile` | `652e85d27ecffb7538df2a0f9aefa5feb335ff3d` |
| `indigo_drivers/shared/primalucelib/primalucelib.vcxproj` | `ce83d540c3c9e925fab525d6ca0bc902e6bc0d07` |
| `indigo_drivers/aux_giotto/giotto_simulator/giotto_simulator.c` | `8fa33293a9f630470430e90b389a0c94c0fd3466` |
| `indigo_drivers/aux_alto/alto_simulator/alto_simulator.c` | `cc2c9f2e938547b295894efdad84f683d5007c39` |

Independent byte comparison against `git show HEAD:indigo_drivers/shared/primalucelib/jsmn.h` proved that `indigo_libs/externals/jsmn/jsmn.h` is unchanged, including the MIT license and configuration logic (11,188 bytes; SHA-256 `a7811ac6efe4ba60ea6a635bb7d02c8ccb0b88ce5e8a92e06e77cef305385990`). The old path is absent and there is one maintained physical header, without forwarding duplicate or symlink. The deletion/untracked-add presentation is expected before staging; it is a byte-preserving move, not a parser rewrite.

Inspected all five maintained include consumers: PrimaLuce C/header, MCP agent, Giotto host simulator and Alto host simulator. Only include spelling changed. PrimaLuce's STATIC/STRICT/PARENT_LINKS/HEADER choices and the simulators'/MCP's translation-unit-local implementation remain unchanged, preserving token layout and linkage. Existing INDIGO library include roots already satisfy the new angle-bracket path; no new dependency or compiler search-path workaround is needed.

Direct Makefile prerequisites now refer to the new header in the shared PrimaLuce build, unit dependencies, both MCP targets and both simulator targets. Windows ClInclude and existing AdditionalIncludeDirectories resolve the new physical path. The legacy Windows project retains its surrounding CRLF content; only the edited header-reference line uses LF, without a whole-file rewrite. Xcode retains one stable tokenizer file reference, its build/header-phase identity and membership, while moving its navigator parent into `indigo_libs/externals/jsmn`. No stale maintained old include, duplicate reference or Factory entry was found. The new agent README points at the correct retained license. Include-path-only movement does not change driver behavior and needs no functional version bump.

Independently executed `git diff --check`, `plutil -lint indigo.xcodeproj/project.pbxproj`, `python3 -B .factory/scripts/validate_project.py --security`, byte-identity/maintained-reference checks and Xcode/Windows path-resolution assertions: all passed. Parent execution evidence inspected: `jsmn-primalucelib-test.log` contains 38 passing cases; `jsmn-mcp-sanitize-test.log` has 14/14 with no sanitizer diagnostic; `jsmn-mcp-recorded-test.log` records native MCP14/14; `jsmn-build.log` shows affected unit/MCP and Giotto/Alto simulator builds. Exact commands/environment remain in verification.md. No broader runtime suite was rerun by this reviewer. Windows/Linux execution, GUI/client checks, and the whole-feature/owner pre-commit gates remain unchanged and unverified/pending as previously stated.

## Owner-directed relocation-only checkpoint index review — 2026-11-04

**Scoped pass for the staged relocation checkpoint only.** Inspected `git diff --cached`, its name/status/no-renames inventory and `git diff --cached --check` directly. Nine diff entries are staged: one 100% header rename, six existing consumer/build files, the tokenizer-only Xcode group move and three existing test-Makefile dependency replacements. The ten-path no-renames inventory exactly matches that allowlist. No MCP agent/test/registration, POST transport, root status/count, task evidence or finding-registry content is staged. The partial Xcode/Makefile blobs contain no agent_mcp addition and deliberately differ from the broader reviewed working files.

| Indexed path | Indexed blob identity |
| --- | --- |
| `indigo.xcodeproj/project.pbxproj` | `6715f4206a98c43166cbda9d0298ae69b28e7141` |
| `indigo_drivers/aux_alto/alto_simulator/alto_simulator.c` | `cc2c9f2e938547b295894efdad84f683d5007c39` |
| `indigo_drivers/aux_giotto/giotto_simulator/giotto_simulator.c` | `8fa33293a9f630470430e90b389a0c94c0fd3466` |
| `indigo_drivers/shared/primalucelib/Makefile` | `652e85d27ecffb7538df2a0f9aefa5feb335ff3d` |
| `indigo_drivers/shared/primalucelib/primalucelib.c` | `01a9f2dd85ac554b7c368bfd881377b544259069` |
| `indigo_drivers/shared/primalucelib/primalucelib.h` | `c4904e53ae0c4746e35ea778cf0348213a4de19e` |
| `indigo_drivers/shared/primalucelib/primalucelib.vcxproj` | `ce83d540c3c9e925fab525d6ca0bc902e6bc0d07` |
| `indigo_libs/externals/jsmn/jsmn.h` | `b3e2338c1f029df5bf75e62d9b7f73aa34ccc32a` |
| `indigo_test/Makefile` | `8fe463c2e4dd3a442c5e8c73b429eab2cdb3a82b` |

Independent HEAD-old/index-new comparison again proves the vendor header byte-identical (SHA-256 a7811ac6efe4ba60ea6a635bb7d02c8ccb0b88ce5e8a92e06e77cef305385990). Include macros/layout/linkage and direct prerequisite/path checks from the bounded relocation review remain applicable. The index-only Xcode diff preserves stable file/build/header identity and moves only its parent group; the indexed Makefile changes only the three existing PrimaLuce/Giotto/Alto prerequisites. No scope leak or blocking quality issue was found. Parent owns staged-candidate build/test evidence; no full MCP or unrelated suite was rerun by this reviewer.

This is the owner-authorized independently useful checkpoint exception within the same unfinished MCP task, not task completion/archive or full-feature delivery. The owner authorized a commit of this indexed subset only and explicitly prohibited push. Review did not stage, commit, publish or modify implementation. Broader working-tree review evidence and pending owner inspection remain separate.

## Read-tools increment review
Independent final read-tools quality review is retained in [read-tools-review.md](read-tools-review.md). Scope: agent/cache/tools, focused tests and final documentation/status rows. FND002 item schema corrected and independently reverified, canonical record archived. Earlier POST/skeleton/relocation reviews remain scoped to their retained identities; no full task or audit completion claimed.

## Write-tools increment review
Independent final scalar-write quality pass: [write-tools-review.md](write-tools-review.md). FND003 finite numeric conversion corrected, independently reproduced/retested and canonical archived. WO discovery expectations intentionally amended by the write specification; value-denial remains covered. This scoped pass does not close the whole task or authorize commit/publication.

## Installed-client verification review
Final independent evidence/documentation pass: [client-review.md](client-review.md). CodexCLI8/8 and ClaudeDesktoplocalCodeGUI9/9 manualMCPchecks, including reversible textwrite/readback/restore on actualCCD/mountsimulators. Productsource/test unchanged. FND004 concerns only reusabletemporaryfixture isolation guard, fixed/reinspected/canonicalarchived; as-executed sources retained. Review does not claim independentGUIexecution or broaderclient/hardware support.
