# Embedded MCP help quality review

Verdict: pass
Date: 2026-11-04
Reviewer and independence: fresh `guides_review` subagent; no implementation, documentation or test edits. Requested fixes through the coordinator and reinspected the final candidate. The same reviewer performed a separate security pass; this is not independent dual-review certification.
Base revision: `323a560ff37d036c34bd7984a5e38abed9036c6a`, task branch `task/20261104203810-488c-mcp-agent-guides`, uncommitted working candidate. Earlier draft-only review is preserved in quality-review-drafts.md and does not substitute for this implementation review.

## Reviewed candidate and scope

Final SHA-256 identities:

- Agent C: `ec38bbca496e8a9d23b3b5de448b829b2c7e1a299272c4991c6609e3199de780`.
- Help generator: `a6a03796ec4e4e16f518f98f3b86b4f73576d117e628de2ce71a5a5bfa999872`.
- Generated header: `e6eaf4a63b226c705980c0beb5d8f3dbd67ad5635a3c74f0b2c4353c18029caf`.
- Integration test: `522edaf33b7416c90065d56068597ea66731824aebfd6583b92fc00ce7af6db7`.

Inspected the complete production diff and relevant surrounding parser/serializer/lifecycle code; generator and header representation; local driver/test Makefile dependencies; Xcode and Windows registrations; all new integration cases and preserved assertions; README/manual transition to actual help delivery; generated test-summary/status changes. No unrelated native source, public bus properties, core/framework or off-limits driver changes were found. Factory files are not added to Xcode. Vendor sources, broad repository audits and shell-compliance suites were excluded.

Compared resource capabilities, result shapes and initialization instructions with official MCP 2025-11-25 [resources](https://modelcontextprotocol.io/specification/2025-11-25/server/resources), [tools](https://modelcontextprotocol.io/specification/2025-11-25/server/tools), [lifecycle](https://modelcontextprotocol.io/specification/2025-11-25/basic/lifecycle) and [schema](https://modelcontextprotocol.io/specification/2025-11-25/schema). Empty resources capability correctly avoids subscription/list-change promises. Read-only help is separate from the four existing bus tools. Existing transport, bus/token policy and lifetime synchronization are preserved rather than reimplemented.

## Findings, fixes and reinspection

[FND-20261104203810-488c-mcp-agent-guides-003](../../../findings/archive/FND-20261104203810-488c-mcp-agent-guides-003.json): initial lookup rejected the section fragments emitted in embedded manual links. Confirmed concrete impact was failure when following a catalog recipe link. The final code decodes the string, removes its fragment, then compares the base against the fixed whitelist; it returns the full manual and canonical resource URI. Unknown bases, traversal, queries and percent-encoded aliases remain rejected. Reinspected production and regression assertions; the independent final integration run passes them.

[FND-20261104203810-488c-mcp-agent-guides-004](../../../findings/archive/FND-20261104203810-488c-mcp-agent-guides-004.json): initial resource parameter validation rejected valid optional protocol _meta objects. The final resource-only validator permits object metadata, rejects duplicate/nonobject metadata and unknown parameters, and skips metadata contents without promising progress notifications. Tool arguments remain strict; the bus tools are unchanged. Reinspected against the primary schema and final regression coverage; the independent run passes.

Both findings are retained as fixed/reverified in the canonical archive; neither is an owner risk acceptance. Existing open findings 001 (Mount epoch-derived calculations) and 002 (canonical Sequencer transit-offset units) remain out of scope and visibly qualified in help. The embedded canonical reference receives an explicit conflict/bootstrap preface without changing upstream source. No unresolved blocking finding in the embedding diff.

## Independent verification and evidence

Acceptance, baseline, native build, sanitizer and recorded-runner evidence belongs in [verification-embedding.md](verification-embedding.md) and [verification.md](verification.md). Reviewed the frozen-candidate 34/34 native and ASan/UBSan evidence, including the original 30 cases and new offline retrieval/boundary cases. Environment-only restricted loopback failure is retained separately from successful authorized execution.

The reviewer independently executed the already-built final integration binary using isolated loopback permission: `indigo_test/build/integration/test_agent_mcp > /private/tmp/mcp-embedding-independent-review.log 2>&1`, exit 0, all 34 cases passed. This was an independent execution of the verifier-built artifact, not a second independent rebuild. No hardware or external service was used.

Also independently ran generator --check (18 current documents), git diff --check, configured security validation, Xcode plutil lint and Windows XML parsing: all passed. A strict C99 syntax check of the final generated header passed with Apple clang 21.0.0, -Wall -Wextra -Werror -pedantic. Decoded all 18 numeric-byte arrays: valid UTF-8, exactly one terminal NUL each; all 206 embedded help links resolve to one of the 18 whitelisted base topics after removing fragments. The Make dry-run orders generation before agent compilation; test targets depend on the same header. Checked-in byte arrays avoid compiler source-encoding and long string-literal assumptions.

## Verdict and limits

The additive offline-help implementation satisfies the reviewed embedding scope. It introduces no request-derived path lookup or runtime documentation/network dependency. Generation is deterministic and checked; Make regenerates when declared sources change. Xcode/Windows consume the checked-in bundle, so maintainers must run --check/regeneration when editing manuals; those platforms do not acquire a Python runtime requirement.

No Linux or Windows execution/build was available to this reviewer. Xcode project syntax and native Make/header compilation do not certify every platform. No new Codex/Claude GUI compatibility run, model-level task acceptance, physical operation, allocation-fault injection or full source-security audit is claimed. Manual semantic gaps remain explicit, not certified by embedding. This pass does not authorize commit/push/integration, close unrelated findings or reset maintenance cadence. Coordinator owns cleanup and the owner handoff.
