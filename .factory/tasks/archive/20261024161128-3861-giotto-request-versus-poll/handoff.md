# Task handoff

Keep detailed reports in their task files; this handoff owns the current assignment, concise transitions and delivery state, with links to verification.md, quality-review.md and security.md. Update project.md only with necessary current conclusions and evidence references; keep shared guides procedural. Preserve those references when archiving or pruning this task under the applicable retention routine.

## Current assignment
- Task: 20261024161128-3861-giotto-request-versus-poll
- Phase: complete. The commit awaits the requester's approval of the commit plan.
- Responsible role: coordinator
- Allowed write paths: `indigo_drivers/aux_giotto/`, `indigo_test/integration/test_aux_giotto_simulator.c`, `MIGRATION_STATUS.md`, `TEST_SUMMARY.md`, `indigo.xcodeproj/project.pbxproj`, the closed finding and the `.factory/` task and completion files.
- Expected output: one task commit after approval, with no push.
- Blockers: none.

## Host/model handoff
- Current host/tool and model ID: Claude Code (VS Code extension), claude-opus-5-5.
- Capabilities: file read/edit, shell, local Git (no push), no network, no delegation; no device used.
- Current writer and assigned paths: this agent; the paths above.
- Incoming host/writer: none.
- Snapshot revision and uncommitted work to preserve: `task/20261024161128-3861-giotto-request-versus-poll` at `d75fb905d`; uncommitted. The requester's unrelated untracked files are untouched.
- Loaded instructions: root AGENTS.md; CLAUDE.md; bootstrap; the findings, archive and process-check routines.
- Capability limitations: Git 2.43 runs through the session wrapper.
- Context pack: none.

## Model choice and budget
- Risk: low (poll guard, simulator-verified).
- Tier: strong (session model); model claude-opus-5-5.
- Same-problem attempts: one.
- Budget: none defined.

## Git delivery tracking
- Task branch: `task/20261024161128-3861-giotto-request-versus-poll`, from master `d75fb905d`.
- Tested candidate: the working tree; see [verification.md](verification.md).
- Integration / publication / cleanup: pending. The commit waits for plan approval; no push.

## Latest handoff
- From role / to role: developer → verifier → reviewers → coordinator (same agent, separate passes).
- Completed work: GIOTTO poll guard (3.0.0.2), simulator delay, regression test, documentation, finding closure.
- Changed files: see process.json reviewed content.
- Decisions: guard per property after the exchange, as in the focuser driver.
- Evidence: [verification.md](verification.md), [quality-review.md](quality-review.md), [security.md](security.md).
- Open findings and limitations: the same flaw in the ALTO (next task); no hardware rerun; the recurring repository audit was just run upstream.
- Next concrete step: commit after approval.

## Handoff history
- 2026-10-24, coordinator: task created at the requester's request.
- 2026-10-24, developer, verifier and reviewers: reproduced, fixed, recorded run 26/26; reports written.

## Final delivery
The task was archived after acceptance and reviews, before the final commit. Integration and publication are pending.

## Portable process evidence
process.json version 2 with scope, checks, risk reviews and reviewed content.

## Central findings handoff
- FND-20261024121920-3bd2-repository-audit-021 closed as fixed.
