# Task handoff

Keep detailed reports in their task files; this handoff owns the current assignment, concise transitions and delivery state, with links to verification.md, quality-review.md and security.md.

## Current assignment
- Task: 20261112202203-24f6-fix-qhy2-readout-timeout
- Phase: complete
- Responsible role: coordinator
- Allowed write paths: `indigo_drivers/ccd_qhy2/indigo_ccd_qhy2.driver`, `indigo_drivers/ccd_qhy2/indigo_ccd_qhy2.cpp`, `indigo_test/integration/test_ccd_qhy_sdk.cpp`, this task's records
- Expected output: fixed driver, regression test, two commits on the task branch
- Blockers: none

## Host/model handoff
- Current host/tool and model ID: Claude Code (desktop app, Code tab), claude-opus-5-5
- Capabilities: file read/edit, shell/tests, Git, network (GitHub issue read through `gh`), subagent delegation
- Current writer and assigned paths: this coordinator session, paths above
- Incoming host/writer and release of outgoing writer: none
- Snapshot revision and uncommitted work to preserve: base master `e0280e650`
- Loaded shared/scoped instructions and capability limitations: root AGENTS.md, CLAUDE.md, .factory/bootstrap.md, indigo_drivers/AGENTS.md, indigo_test/AGENTS.md (relevant sections), project.md, process-checks and concurrent-tasks routines. No hardware.
- Context pack/source excerpts provided and omitted scope: not used

## Model choice and budget
- Assignment risk, selected tier, actual model/settings, and rationale: low risk, one-argument driver fix with a fake-SDK test; current host model retained (claude-opus-5-5) per project.md
- Same-problem unsuccessful attempts and escalation/de-escalation checkpoint: none
- Owner-defined budget/currency and existing spending authorization: none supplied

## Git delivery tracking
- Task branch / main target / remote: `task/20261112202203-24f6-fix-qhy2-readout-timeout` / `master` / origin (no push)
- Base revision and latest merged target revision: `e0280e650`
- Tested candidate revision: the product commit on the task branch; recorded externally after commit
- Check evidence on synchronized candidate: [verification.md](verification.md)
- Integration / publication / cleanup: pending; fast-forward to master, push and branch deletion need the owner's go-ahead
- Existing authorization and protected-branch requirements: no push without the owner

## Latest handoff
- From role / to role: coordinator (implementer) → owner
- Completed work: timeout fix, version 47, fake-SDK frame timeout model and regression case, independent review and its fixes
- Changed files: `indigo_drivers/ccd_qhy2/indigo_ccd_qhy2.driver`, `indigo_drivers/ccd_qhy2/indigo_ccd_qhy2.cpp`, `indigo_test/integration/test_ccd_qhy_sdk.cpp`
- Decisions and deviations: the timeout is exposure + 10 s, the same as the acquisition deadline, and is not unlimited (0), so a hung camera cannot block the device queue
- Evidence references: [verification.md](verification.md), [quality-review.md](quality-review.md), [security.md](security.md)
- Open findings and limitations: no hardware confirmation; the reporter of indigo#783 needs to test a build. The repository maintenance status reports an incremental review due; that is a separate audit task for the owner to schedule.
- Next concrete step: owner decides on integration to master, push and a reply on indigo#783

## Handoff history
- 2026-11-12: coordinator created the task after the owner's "ano, oprav to"; implemented, reviewed in a fresh context, fixed Q3–Q5, verified; complete.

## Final delivery
Archived before the final commits; integration, publication and cleanup pending the owner.

## Portable process evidence
process.json with scope, checks and reviewed content; committed-range process check run over both commits.

## Central findings handoff
- Relevant canonical FND IDs and registry reference: none; the defect is fixed in this task
- Reconciliation decisions or blockers, if any: none
- Durable report pins and findings check/prune-gate evidence: `./.factory/scripts/findings check` before commit
