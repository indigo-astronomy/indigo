# Task handoff

## Current assignment
- Task: 20261025134403-8427-fix-sony-capture-deadline
- Phase: complete.
- Responsible role: coordinator
- Allowed write paths: `indigo_drivers/ccd_ptp/`, `indigo_test/integration/test_ccd_ptp_sdk.c`, `indigo_test/Makefile`, `MIGRATION_STATUS.md`, `TEST_SUMMARY.md`, `.factory/` task, finding and completion files.
- Expected output: one commit on the task branch, no push.
- Blockers: none.

## Host/model handoff
- Host/tool and model: Claude Code desktop app, claude-opus-5-5.
- Capabilities: file read/edit, shell, local Git (no push), SSH with agent forwarding to the owner's Raspberry Pi 5 (indigosky.local) with the Sony ILCE-7RM4 attached, read-only review subagent, web search.
- Writer: this agent. The Pi checkout `~/indigo` (master 4387df2c0) was used only to build and run the same tree; candidate files applied as a patch and hash-checked. Temporary Pi-only diagnostic tracing and the reconnect loop were never committed and were removed.
- Base: master 4387df2c0.

## Model choice and budget
- Risk: medium (camera control path, physical device). Tier: strong; claude-opus-5-5. Budget: none defined.

## Maintenance
The full repository audit is due. The owner explicitly asked to fix this PTP finding from the previous PTP hardware task, a continuation of the PTP hardware work under the recorded PTP deferral (as tasks 20261025112041-9ddc and 20261025113906-c862); no new deferral recorded. The audit remains due before the next unrelated task.

## Git delivery tracking
- Task branch: `task/20261025134403-8427-fix-sony-capture-deadline` from master 4387df2c0. Integration (fast-forward to master), push and cleanup need the owner.

## Latest handoff
- Completed: diagnosis on hardware (two failure modes), shot retry with LENR-aware window, Sony session reopen delay, three fake regressions, independent review fixes, version 3.0.0.59, recorded runs.
- Evidence: [verification.md](verification.md), [quality-review.md](quality-review.md), [security.md](security.md); driver `REFACTOR.md` section "Sony ILCE-7RM4 reconnect capture fix".
- Open findings: none from this task; FND-20261025113906-c862-ptp-fuji-hardware-test-001 closed as fixed.
- Next step: owner decides on integration and push.

## Handoff history
- 2026-10-25: task created on the owner's request; reproduction and diagnosis on the Pi, fixes, review, final recorded runs.
