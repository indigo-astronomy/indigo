# Task handoff

## Current assignment
- Task: 20261025112041-9ddc-ptp-sony-hardware-test
- Phase: complete.
- Responsible role: coordinator
- Allowed write paths: `indigo_drivers/ccd_ptp/`, `indigo_test/hardware/test_ccd_ptp_hw.c`, `indigo_test/Makefile`, `indigo_test/integration/test_ccd_ptp_sdk.c`, `MIGRATION_STATUS.md`, `TEST_SUMMARY.md`, `indigo.xcodeproj/project.pbxproj`, `.factory/` task and completion files.
- Expected output: one commit on the task branch, no push.
- Blockers: none.

## Host/model handoff
- Host/tool and model: Claude Code desktop app, claude-opus-5-5.
- Capabilities: file read/edit, shell, local Git (no push to origin), SSH with agent forwarding to the owner's Raspberry Pi 5 (indigosky.local) with the Sony ILCE-7RM4 attached, read-only review subagent.
- Writer: this agent. The Pi checkout `~/indigo` was used only to build and run the same tree (branch pushed to the Pi's own repository, changed files copied and hash-checked).
- Base: `task/20261025093224-2d50-fix-ptp-findings` at `a89afe87d` (unintegrated; this task depends on it and branches from it).

## Model choice and budget
- Risk: medium (camera control path, physical device). Tier: strong; claude-opus-5-5. Budget: none defined.

## Maintenance
The full repository audit is due. The owner explicitly requested this hardware verification of the PTP repair after being told; it is recorded as part of the PTP deferral's scope (verification of that repair), not as a new deferral.

## Git delivery tracking
- Task branch: `task/20261025112041-9ddc-ptp-sony-hardware-test`, on top of the PTP findings branch. Integration (both branches fast-forward to master), push and cleanup need the owner.

## Latest handoff
- Completed: hardware suite `test_ccd_ptp_hw` (15 cases), D-HW1 INFO identity, D-HW2 DeviceBusy after connect, D-HW3 dropped SetControlDeviceA writes verified by a finalizer (approach approved by the owner after the libgphoto2 comparison); three fake regressions; version 3.0.0.57.
- Evidence: [verification.md](verification.md), [quality-review.md](quality-review.md), [security.md](security.md).
- Open findings: none new.
- Next step: owner decides on integration and push.

## Handoff history
- 2026-10-25: task created for the owner's hardware request; runs, fixes, libgphoto2 comparison, owner approval of driver verification, review fixes, final recorded runs.
