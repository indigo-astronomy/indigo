# Task handoff

## Current assignment
- Task: 20261025113906-c862-ptp-fuji-hardware-test
- Phase: complete.
- Responsible role: coordinator
- Allowed write paths: `indigo_drivers/ccd_ptp/`, `indigo_test/hardware/test_ccd_ptp_hw.c`, `indigo_test/integration/test_ccd_ptp_sdk.c`, `MIGRATION_STATUS.md`, `TEST_SUMMARY.md`, `.factory/` task and completion files.
- Expected output: one commit on the task branch, no push.
- Blockers: none.

## Host/model handoff
- Host/tool and model: Claude Code desktop app, claude-opus-5-5.
- Capabilities: file read/edit, shell, local Git (no push to origin), SSH with agent forwarding to the owner's Raspberry Pi 5 (indigosky.local) with the Fujifilm X-T1 attached, read-only review subagent.
- Writer: this agent. The Pi checkout `~/indigo` (own branch `task/20261025113906-c862-ptp-fuji-hardware-test` at `e49bfce27`) was used only to build and run the same tree; changed files copied and hash-checked.
- Base: master at `e49bfce27`.

## Model choice and budget
- Risk: medium (camera control path, physical device). Tier: strong; claude-opus-5-5. Budget: none defined.

## Maintenance
The full repository audit is due. The owner explicitly requested this hardware verification of the PTP driver, a continuation of the hardware verification recorded under the PTP deferral (as for task 20261025112041-9ddc-ptp-sony-hardware-test); no new deferral recorded. The audit remains due before the next unrelated task.

## Git delivery tracking
- Task branch: `task/20261025113906-c862-ptp-fuji-hardware-test` from master `e49bfce27`. Integration (fast-forward to master), push and cleanup need the owner.

## Latest handoff
- Completed: X-T1 hardware run; suite made vendor-neutral (compression labels, shutter labels, RAF); D-FJ1 Bulb hidden without declared USB BULB, D-FJ2 readback after a rejected write, D-FJ3 only still image interfaces attached; three fake regressions; version 3.0.0.58.
- Evidence: [verification.md](verification.md), [quality-review.md](quality-review.md), [security.md](security.md).
- Open findings: FND-20261025113906-c862-ptp-fuji-hardware-test-001 (Sony capture lost after reconnect).
- Next step: owner decides on integration and push.

## Handoff history
- 2026-10-25: task created for the owner's hardware request; runs, fixes, USB BULB diagnosis, review, final recorded runs; extra Sony run on owner request, finding opened.
