# Task handoff

Keep detailed reports in their task files; this handoff owns the current assignment, concise transitions and delivery state, with links to verification.md, quality-review.md and security.md. Update project.md only with necessary current conclusions and evidence references; keep shared guides procedural. Preserve those references when archiving or pruning this task under the applicable retention routine.

## Current assignment
- Task: 20261023231027-08cd-esatto-calibration-home
- Phase: complete. The commit awaits the requester's approval of the commit plan.
- Responsible role: coordinator
- Allowed write paths: `indigo_drivers/focuser_primaluce/`, `indigo_drivers/shared/primalucelib/PROTOCOL.md`, `indigo_test/integration/test_focuser_primaluce_simulator.c`, `indigo_docs/PROPERTIES.md`, `MIGRATION_STATUS.md`, `TEST_SUMMARY.md`, and the `.factory/` task and completion files.
- Expected output: one task commit after approval, with no push.
- Blockers: none.

## Host/model handoff
- Current host/tool and model ID: Claude Code (VS Code extension), claude-opus-5-5.
- Capabilities: file read/edit, shell, build, tests, local Git (no push), no network, no delegation. The requester's ESATTO2 with ARCO was used.
- Current writer and assigned paths: this agent; the paths above.
- Incoming host/writer: none.
- Snapshot revision and uncommitted work to preserve: `task/20261023231027-08cd-esatto-calibration-home` at `ba09a1367`; the task is uncommitted. The requester's unrelated untracked files are untouched.
- Loaded instructions: root, driver and test AGENTS.md; CLAUDE.md; bootstrap; the archive and process-check routines.
- Capability limitations: Git 2.43 runs through the session wrapper. Only Linux is available.
- Context pack: none.

## Model choice and budget
- Risk: medium (focuser and rotator motion on hardware).
- Tier: strong; model claude-opus-5-5.
- Same-problem attempts: one; the first filtered run used a stale driver library.
- Budget: none defined.

## Git delivery tracking
- Task branch: `task/20261023231027-08cd-esatto-calibration-home`, from master `ba09a1367`. origin/master is behind master; the push is the requester's decision.
- Tested candidate: the working tree; see [verification.md](verification.md).
- Integration / publication / cleanup: pending. The commit waits for plan approval; no push.

## Latest handoff
- From role / to role: developer → verifier → reviewers → coordinator (same agent, separate passes).
- Completed work: ESATTO calibration hidden, ESATTO needs-calibration message, home-sensor reason (driver 39), simulator, tests, documentation, test record, hardware check including the ARCO calibration.
- Changed files: see `git status`.
- Decisions: the relative `MOVE` is documented but not used. The home reset is detected from the reported state, not from `NOPCMD`.
- Evidence: [verification.md](verification.md), [quality-review.md](quality-review.md), [security.md](security.md).
- Open findings and limitations: the README text proposal; the recurring repository audit is due.
- Next concrete step: commit after approval.

## Handoff history
- 2026-10-24, coordinator: task created after the requester approved the proposal.
- 2026-10-24, developer: driver, simulator, tests and documentation; the three cases fail without the driver change.
- 2026-10-24, hardware: supervised focuser moves, abort, ARCO checks and ARCO calibration on the unit.
- 2026-10-24, verifier and reviewers: recorded run 71/71; reports written.

## Final delivery
The task was archived after acceptance and reviews, before the final commit. Integration and publication are pending.

## Portable process evidence
process.json version 2 with scope, checks, risk reviews and reviewed content.

## Central findings handoff
- No finding.
