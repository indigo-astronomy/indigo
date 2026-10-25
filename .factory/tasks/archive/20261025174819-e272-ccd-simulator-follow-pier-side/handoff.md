# Task handoff

Keep detailed reports in their task files; this handoff owns the current assignment, concise transitions and delivery state, with links to verification.md, quality-review.md and security.md. Update project.md only with necessary current conclusions and evidence references; keep shared guides procedural. Preserve those references when archiving or pruning this task under the applicable retention routine.

## Current assignment
- Task: 20261025174819-e272-ccd-simulator-follow-pier-side
- Phase: complete. The commit awaits the requester's approval of the commit plan.
- Responsible role: coordinator
- Allowed write paths: `indigo_test/integration/test_ccd_simulator.c`, `indigo_drivers/ccd_simulator/README.md`, `indigo_drivers/ccd_simulator/REFACTOR.md`, `TEST_SUMMARY.md`, `indigo.xcodeproj/project.pbxproj` and the `.factory/` task and completion files.
- Expected output: one commit after approval, with no push.
- Blockers: none.

## Host/model handoff
- Current host/tool and model ID: Claude Code (VS Code extension), claude-opus-5-5.
- Capabilities: file read/edit, shell, local Git (no push), no network, no delegation; no device used.
- Current writer and assigned paths: this agent; the paths above.
- Incoming host/writer: none.
- Snapshot revision and uncommitted work to preserve: `task/20261025174819-e272-ccd-simulator-follow-pier-side` at `b2e62cc29`; uncommitted. The requester's unrelated untracked files are untouched.
- Loaded instructions: root AGENTS.md; CLAUDE.md; bootstrap; indigo_test AGENTS.md; the findings, archive and process-check routines.
- Capability limitations: Git 2.43 runs through the session wrapper.
- Context pack: none.

## Model choice and budget
- Risk: low (opt-in test script).
- Tier: strong (session model); model claude-opus-5-5.
- Same-problem attempts: one.
- Budget: none defined.

## Git delivery tracking
- Task branch: `task/20261025174819-e272-ccd-simulator-follow-pier-side`, from master `b2e62cc29`.
- Tested candidate: the working tree; see [verification.md](verification.md).
- Integration / publication / cleanup: pending. The commit waits for plan approval; no push.

## Latest handoff
- From role / to role: developer → verifier → reviewers → coordinator (same agent, separate passes).
- Completed work: mount-following case checks the side of the pier the mount took; 26/26.
- Changed files: see process.json reviewed content.
- Decisions: the case was time dependent; the simulator was right.
- Evidence: [verification.md](verification.md), [quality-review.md](quality-review.md), [security.md](security.md).
- Open findings and limitations: none.
- Next concrete step: commit after approval.

## Handoff history
- 2026-10-25, coordinator: task created at the requester's request (look at the CCD simulator failure).
- 2026-10-25, developer, verifier and reviewers: cause found (side of the pier depends on the time of day); test fixed; 26/26.

## Final delivery
The task was archived after acceptance and reviews, before the final commit. Integration and publication are pending.

## Portable process evidence
process.json version 2 with scope, checks, risk reviews and reviewed content.

## Central findings handoff
- Closed: FND-20261025152706-abbf-fix-server-leaks-002.
