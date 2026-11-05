# MCP embedded-manual verification

Assigned test writer: integration test and this report only, original checkout and existing task branch. No product implementation, real hardware/client operations, commits or integration performed by this writer.

## Baseline

Working directory: `/Users/polakovic/Development/indigo`. Existing working tree includes reviewed draft manuals; production embedding had not started. Existing MCP integration suite has 30 cases.

- `make -C indigo_test build/integration/test_agent_mcp`: exit 0; log `/private/tmp/mcp-embedding-baseline-build.log`.
- `indigo_test/build/integration/test_agent_mcp`: exit 0, 30/30 passed using synthetic local sockets and the existing private-HOME fixture; log `/private/tmp/mcp-embedding-baseline.log`.

The socket run required the same authorized local-listener sandbox escalation as earlier MCP verification. No external service or physical device was contacted.

## Planned coverage

| Acceptance | Scenario | Observable result |
| --- | --- | --- |
| Offline discovery/bootstrap | Initialize instructions and capabilities; tools/list | Agent-first instructions identify help access and resource/help tool discovery |
| Embedded resource inventory | resources/list and every resources/read | Exact stable URI whitelist, Markdown MIME type and decoded content equal maintained source |
| Help tool | Default index, exact relative topic and URI aliases | Text matches the corresponding resource; valid JSON escaping/Unicode survives round trip |
| Input boundary | Unknown/traversal resource URI; malformed resource/help arguments; unknown topic | Unknown resources produce -32002, malformed arguments -32602, unknown help topic is an explicit tool error |
| Runtime independence | Initialize/retrieve from isolated empty current directory | No current-directory documentation dependency; cwd restored before reporting assertions |
| Regression | Existing transport/read/write suite plus sanitizer | All original cases retained, catalog response capacity increased only if needed |

Interface details and final commands/results will be recorded after coordinator confirmation and implementation availability. Initial new cases may fail before implementation; these are expected new-feature failures, distinct from the passing baseline.

## New-feature baseline

After adding four integration cases, before production embedding was present:

- `make -C indigo_test build/integration/test_agent_mcp`: exit 0; log `/private/tmp/mcp-embedding-tests-build.log`.
- `indigo_test/build/integration/test_agent_mcp`: exit 1, 30/34 passed; exactly the four new embedded-help cases failed. Existing 30 cases remained passing. Log `/private/tmp/mcp-embedding-new-tests-baseline.log`.

Confirmed interface: 18 resources (17 maintained guides plus virtual `reference/sequencer.md`); exact `indigo://help/<path>` resource URI; `get_help` accepts exact `.md` relative topic or full URI and defaults to `README.md`; successful help is a single raw Markdown text content block without structuredContent. Unknown resource URI is -32002; unknown help topic is a tool error. The generator rewrites maintained Markdown links to embedded URIs, so resource/help decoded text equality plus exact inventory, Markdown markers and Unicode checks replace an invalid raw-source byte comparison.

The four cases cover bootstrap/capability/tool discovery; every listed resource and both tool topic forms/default; boundary/error behavior; and agent shutdown/reinitialization plus all-resource retrieval in an empty temporary working directory. The empty cwd is restored and removed before assertions are reported. Existing initialization/catalog buffers were enlarged for added semantic content; existing assertions are preserved. No Makefile changes by the test writer.

## Final contract refinements

Coordinator/reviewer clarified that a known whitelisted base resource/topic may include a `#fragment`; retrieval returns the entire Markdown document and resources/read reports the canonical base URI. Tests verify known-base resource and relative-tool fragments, including an unknown fragment, while unknown-base-with-fragment remains rejected. This supports intra-manual links without accepting arbitrary paths, queries or percent-encoded aliases.

Standard resource request `_meta` is accepted only as an object; its arbitrary contents are ignored without promising progress notifications. Tests cover valid metadata on list/read and reject nonobject/duplicate metadata. Field errors use -32602. Existing global envelope behavior is preserved: array params produce HTTP 400/-32600, and resource methods without IDs are rejected. List without params succeeds; no cursor is advertised.

Test-fixture corrections after the first implemented run (31/34) were justified by observable contracts: expected driver version increased from 3.0.0.0 to 3.0.0.1; a degree-glyph marker was obsolete after manual normalization and replaced with retained literal Unicode `J2000→JNow`; the generic RPC parser accepts HTTP 400 envelope errors while the existing tool helper still requires HTTP 200. Existing behavioral assertions were retained; no product defect was asserted from those fixture failures.

## Final verification

Working directory `/Users/polakovic/Development/indigo`, HEAD `323a560ff37d036c34bd7984a5e38abed9036c6a` with the current uncommitted documentation/embedding implementation, driver version 3.0.0.1. Actual target is macOS arm64 for sanitizer execution; native target is built universal x86_64/arm64 and executed on this arm64 host.

- `make -C indigo_test build/integration/test_agent_mcp build/integration/test_agent_mcp_sanitize`: exit 0; `/private/tmp/mcp-embedding-final-build.log`.
- `indigo_test/build/integration/test_agent_mcp`: exit 0, 34/34; `/private/tmp/mcp-embedding-final-native.log`.
- `indigo_test/build/integration/test_agent_mcp_sanitize`: exit 0, 34/34, no ASan/UBSan diagnostics; `/private/tmp/mcp-embedding-final-sanitize.log`.
- `git diff --check -- indigo_test/integration/test_agent_mcp.c`: exit 0.

All 18 resources were retrieved and compared byte-for-byte with both exact relative-topic and full-URI get_help outputs. Default get_help matches README.md. The same complete retrieval matrix passes after agent shutdown/reinitialization in an isolated empty working directory, demonstrating no cwd-relative documentation dependency. JSON strings were parsed and decoded, Markdown/text content shapes and Content-Length checked, Unicode content and escaped topic/URI names verified. Source inspection by the coordinator/reviewer remains responsible for proving absence of absolute-path runtime file lookup and correctness of generated source/link transformations; the integration comparison does not independently reconstruct the embedding generator.

Final content identities (SHA-256):

- Test source: `522edaf33b7416c90065d56068597ea66731824aebfd6583b92fc00ce7af6db7`.
- Agent source: `ec38bbca496e8a9d23b3b5de448b829b2c7e1a299272c4991c6609e3199de780`.
- Embedded header: `e6eaf4a63b226c705980c0beb5d8f3dbd67ad5635a3c74f0b2c4353c18029caf`.

Changed by this test assignment only: `indigo_test/integration/test_agent_mcp.c` and this report. Four new cases retain all original 30 transport/read/write cases. No Makefile, product source, user configuration, runtime server/client, hardware or off-limits driver writes. No full-server build, physical hardware or real-client acceptance is claimed for this increment. Parent owns the final recorded driver runner, README/summary and independent review. Binaries retained for that coordinated verification instead of broad test-clean during concurrent work. Test file released to coordinator/reviewer after these runs.

After the coordinator explicitly declared documents/header and fragment/metadata handling frozen, the same build, native and sanitizer commands above were repeated: all exit 0, native 34/34 and sanitizer 34/34, no sanitizer diagnostics. Final agent/header/test SHA-256 identities remained exactly those recorded above. The final log paths were replaced with this frozen-candidate run.
