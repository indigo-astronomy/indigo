# Task handoff

## Current assignment
- Task: 20261025174618-8b09-ptp-sony-a7r2-hardware-test
- Phase: implementation (checkpoint committed).
- Responsible role: coordinator
- Allowed write paths: `indigo_drivers/ccd_ptp/`, `indigo_test/hardware/test_ccd_ptp_hw.c`, `indigo_test/integration/test_ccd_ptp_sdk.c`, `indigo_test/Makefile`, `MIGRATION_STATUS.md`, `TEST_SUMMARY.md`, `.factory/` task, finding and completion files.
- Expected output: driver and suite fixes, recorded clean hardware run on the ILCE-7RM2; no push.
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
- Completed: stepping support in the hardware suite; Sony live view window; old-API stale shot accounting with expiry; four fake regressions; independent review fixes; version 3.0.0.60; recorded fake run 172/172; recorded hardware run 15/11 Failed.
- Open: lost ObjectAdded leaves the driver one object behind (see the driver REFACTOR.md section "Sony ILCE-7RM2 hardware run").
- Untouched: `.factory/tasks/20261025134403-8427-fix-sony-capture-deadline/sony-open-source-research.md` was written by another session into the archived task's live path; it makes `findings check` report an ambiguous task location and is left for the owner.
- Next step: detect a missed object on old-API bodies (ObjectInMemory still above 0x8000 after a download), model it in the fake, recorded hardware rerun.

## Handoff history
- 2026-10-25: task created on the owner's request; runs, fixes, review; checkpoint commit on the owner's request.
