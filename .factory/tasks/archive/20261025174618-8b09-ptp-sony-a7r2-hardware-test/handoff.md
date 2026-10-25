# Task handoff

## Current assignment
- Task: 20261025174618-8b09-ptp-sony-a7r2-hardware-test
- Phase: complete.
- Responsible role: coordinator
- Allowed write paths: `indigo_drivers/ccd_ptp/`, `indigo_test/hardware/test_ccd_ptp_hw.c`, `indigo_test/integration/test_ccd_ptp_sdk.c`, `indigo_test/Makefile`, `MIGRATION_STATUS.md`, `TEST_SUMMARY.md`, `.factory/` task, finding and completion files.
- Expected output: final commit on the task branch; no push.
- Blockers: none.

## Host/model handoff
- Host/tool and model: Claude Code desktop app, claude-opus-5-5.
- Capabilities: file read/edit, shell, local Git, SSH to the owner's Raspberry Pi 5 (indigosky.local) with the Sony ILCE-7RM2, read-only review subagent.
- Writer: this agent. The Pi checkout `~/indigo` is used only to build and run the same tree (patch applied, hashes compared); Pi-only tracing is never committed.
- Base: master 70d34909b.

## Model choice and budget
- Risk: medium (camera control path, physical device). Tier: strong; claude-opus-5-5. Budget: none defined.

## Maintenance
The full repository audit is due. The owner explicitly requested this hardware test of the PTP driver, a continuation of the PTP hardware work under the recorded PTP deferral; no new deferral recorded.

## Git delivery tracking
- Task branch: `task/20261025174618-8b09-ptp-sony-a7r2-hardware-test` from master 70d34909b.
- Checkpoint commit: the owner asked to commit their Xcode project cleanup (removal of unneeded references, `indigo.xcodeproj/project.pbxproj`) together with the current changes before the task is finished. The task stays open; the final commit follows.

## Latest handoff
- Completed: stepping support in the hardware suite; Sony live view window; old-API stale shot accounting with expiry; ignored-press retry for every Sony body; write confirmation for every Sony body (old API 15 s / 5 s); leftover drain before the press; nine fake regressions; two independent review rounds; recorded hardware run 15/15 and fake run 177/177 on 3.0.0.60.
- Evidence: [verification.md](verification.md), [quality-review.md](quality-review.md), [security.md](security.md); driver REFACTOR.md section "Sony ILCE-7RM2 hardware run".
- Open findings: none.
- Also done on the owner's request: the research note written by another session into the live path of archived task 20261025134403-8427 was moved into that task's archive (commit f3803ac66), which removed the duplicate task location.
- Next step: owner decides on integration and push.

## Handoff history
- 2026-10-25: task created on the owner's request; runs, fixes, review; checkpoint commit on the owner's request; research note moved; root cause of the remaining failures traced, fixes, second review, one recorded run on the owner's battery request; complete.
