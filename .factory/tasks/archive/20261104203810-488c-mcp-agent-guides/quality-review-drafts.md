# Draft documentation quality review

Verdict: pass
Reviewer and independence: fresh `guides_review` subagent; no manual, source, test or acceptance-specification edits. The reviewer requested corrections through the coordinator and reinspected them. This reviewer also performed the security pass; those are separate passes, not two independent reviewers.
Date: 2026-11-04
Reviewed revision and working-tree state: base `323a560ff37d036c34bd7984a5e38abed9036c6a`, task branch `task/20261104203810-488c-mcp-agent-guides`; untracked draft documents and task evidence, no commit or integration.
Reviewed documentation identity: 17 Markdown files under `indigo_docs/mcp/`, aggregate SHA256 `399ab76b3545e3263f406faa5b311d00d71f96e538f1b5336d41ef10ddf17c73` (sorted repository-relative path, NUL, file bytes, NUL for each file).

## Scope and method

Read the bootstrap, preparation, catalog, future acceptance scenarios, all eleven native-agent manuals and the optional Sequencer manual. Independently confirmed the eleven public native agent entrypoint files. Compared essential science workflows and exact control/frame semantics with relevant public agent sources, shared filter, shared solver, public names, scripting native bindings and Sequencer implementation. The source review was targeted contract verification, not a complete source audit. Loaded quality/security roles, documentation instructions, task brief/specification/plan, project policy and active finding summaries.

Reviewed agent-first loading through server DRIVERS, device preparation versus normal operations, selector readiness and directional relations, units, command/state/result distinctions, pause/abort/recovery, persistent effects, output freshness limits and current four-tool MCP boundaries. No framework, driver, build, Xcode or native property changes are included. Vendor internals, off-limits drivers, task/findings archives and shell-compliance tests were excluded. No simulator, hardware, GUI, backend or sequence execution was performed.

Check and acceptance evidence belongs in [verification.md](verification.md). The reviewer independently reran the final structural script successfully: all 17 documents, 411 local links/anchors and 12 JSON examples. A separate literal-method comparison found all 108 public Sequence prototype methods represented in the new guide; this establishes inventory coverage, not runtime correctness.

## Findings and correction reinspection

- The initial Mount manual incorrectly asserted J2000 unconditionally for targets/readbacks. Reinspection confirmed passthrough selected-mount coordinates and solver conversion to reported MOUNT_EPOCH. The new coordinate-epochs section now documents the actual frame and explicitly separates unresolved derived-calculation assumptions. Underlying source discrepancy remains [FND-20261104203810-488c-mcp-agent-guides-001](../../../findings/active/FND-20261104203810-488c-mcp-agent-guides-001.json), an open nonblocking functional follow-up owned in the registry; see verification evidence. This prose task does not fix or accept that risk.
- Missing optional Sequencer coverage was raised and addressed with a separate inventory/manual and explicit native-bootstrap prerequisite. Its 88,741-byte library exceeds the existing 64 KiB request-body limit; no unsupported single-call upload or filename-import recipe is offered.
- Final Sequencer reinspection corrected the nonexistent EXPOSURE_BATCH process name to EXPOSURE, joystick-agent wording to the actual joystick selector, numeric time to epoch seconds, and explicit mount/solver coordinate-frame requirements. Rechecked the final text against Sequencer.js, scripting time_to_delay and public wire names.
- The existing canonical Sequencer meridian-flip seconds-versus-hours discrepancy is disclosed rather than silently resolved. Retained as [FND-20261104203810-488c-mcp-agent-guides-002](../../../findings/active/FND-20261104203810-488c-mcp-agent-guides-002.json), open nonblocking quality follow-up; ownership/status remain canonical in the registry.
- Scripting/Imager sequence cross-links were reconciled: native capture batches are distinct from the optional library and from custom JavaScript.

No unresolved blocking defect was found in the revised draft package. Findings above are retained with concrete evidence, not treated as completed implementation fixes.

## Scoped verdict and limits

Pass means the package is suitable to show the owner as a source-grounded proposal. It does not mean complete production semantic help or model-operated observing has been validated. The catalog explicitly retains incomplete Alpaca per-member semantics, Bahtinov spike-angle interpretation, hardware-specific recipes, Sequencer bootstrap deployment and the two source-contract follow-ups. The manuals do not invent meanings for these gaps. No embedded-help retrieval, client integration or runtime acceptance is delivered here.

This review does not close the task, authorize a commit, accept residual risk, or reset repository-audit cadence. Next assignment: coordinator presents the drafts and explicit gaps for owner review; later embedding, semantic-gap completion and functional fixes require their own scoped work.
