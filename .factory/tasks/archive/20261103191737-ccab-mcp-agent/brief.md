# Final delivery brief

Deliver the reviewed opt-in INDIGO MCP agent, its minimal shared-handler POST support, focused tests and platform registration. Four tools provide device/property discovery, readable values and native text/number/switch write dispatch. BLOBs are read URLs; unsupported write types and unavailable client/platform/hardware checks are documented. User explicitly accepts the current verified candidate and authorizes one final local commit of all MCP changes, without push or integration. The separately authorized shared-jsmn relocation remains the earlier checkpoint82e884b80.

The task was developed incrementally in the original checkout after owner correction; retain the following historical brief/amendments and the specification's final delivery contract as provenance.

## Initial analytical assignment (historical)

# Task brief

- Task ID: 20261103191737-ccab-mcp-agent
- Owner: Peter Polakovic
- Created: 2026-11-03

## Problem and desired outcome
Expose INDIGO devices and properties to MCP clients through an INDIGO agent using the existing server HTTP listener. The owner requested incremental implementation and explicitly requested an analytical subagent as the first step. Current authorization is analysis and specification; do not implement application changes in this assignment.

## Users and use cases
MCP clients discover devices, inspect property schemas and values, and eventually change writable properties through existing bus APIs. Long-running device operations must distinguish request acceptance from completion.

## Scope and non-goals
Analyze agent architecture, shared HTTP transport integration, lifecycle/unload safety, MCP version/client compatibility, authentication and authorization, synchronization, BLOB handling, portability/build integration, and meaningful tests. Propose staged implementation with a minimal first increment. No source, generator, build-project, README, property-reference or registry changes; no installation, merge, push or deployment.

## Constraints and references
Read root and applicable scoped AGENTS.md, Factory analyst role, relevant developer documentation and existing agent/HTTP implementations. All reports are English. Do not use rg/ripgrep. Exclude task/findings archives, vendor trees, shell_compliance and off-limits drivers from routine discovery. Preserve generated-source ownership and public ABI. Do not assume current HTTP handler API supports POST or exposes headers. Use official MCP specifications and verify current vs earlier protocol contracts; supported version is a proposal until compatibility is assessed.

## Initial questions
Identify concrete compatibility targets, default access policy, minimal tool surface, and whether generic property changes should be a later increment. Separate confirmed owner requirements from proposed defaults and material unresolved decisions. The factory incremental audit is due: retain this prerequisite before implementation; no checkpoint or implicit deferral is authorized by this analysis assignment.

## Owner continuation — 2026-11-04
Owner explicitly requested implementing only the first increment: HTTP handler API with POST, headers, body and exact routing, and inspection before any commit. This supersedes the analysis-only restriction for this bounded transport increment. No MCP endpoint/agent is enabled. The request follows disclosure of the due audit and prioritizes this increment; record a scoped maintenance deferral through owner inspection, without resetting checkpoints or accepting risks.

## Owner design correction — 2026-11-04
Owner explicitly requested using the existing PUT-style handler for POST. Remove the uncommitted second registration API, buffered request structures/parser and separate new-style routing. Extend only the existing request dispatcher; preserve callback ABI, prefix registration, PUT semantics and handler-owned headers/body/response. Exact endpoint checks and framing/security policy belong to the eventual MCP handler. Owner inspection before commit and the original checkout remain mandatory.

## Next owner-authorized increment — 2026-11-04
Owner requested the next MCP-agent step, expecting remaining functionality inside the agent without further framework impact. Keep all existing uncommitted POST changes and the owner's header reset. Scope now includes a new agent lifecycle and minimal protocol endpoint, focused tests and necessary build/project registration. No tools exposing device data/control yet. Same original checkout/branch, no commit. Client target was asked asynchronously; use a pinned initial protocol and record interoperability limits rather than claim all-client support.

## Shared tokenizer relocation — 2026-11-04
Owner requested moving primalucelib/jsmn.h to a shared vendor location as part of the MCP dependency work. Use existing indigo_libs/externals convention: externals/jsmn/jsmn.h. Preserve header bytes/license/macros and update all maintained consumers, prerequisites and project references. Same task/branch/original checkout; no commit. Scope is include-path/build metadata only, no behavior or driver-version change.
