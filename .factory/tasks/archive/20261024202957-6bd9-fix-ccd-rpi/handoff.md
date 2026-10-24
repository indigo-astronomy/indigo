# Task handoff

Keep detailed reports in their task files; this handoff owns the current assignment, concise transitions and delivery state, with links to verification.md, quality-review.md and security.md. Update project.md only with necessary current conclusions and evidence references; keep shared guides procedural. Preserve those references when archiving or pruning this task under the applicable retention routine.

## Current assignment
- Task: 20261024202957-6bd9-fix-ccd-rpi
- Phase: delivery
- Responsible role: coordinator acting as implementer (single agent, separate review passes)
- Allowed write paths: indigo_optional_drivers/ccd_rpi/, this task directory, central findings records for this driver
- Expected output: fixed driver, hardware evidence, quality review, commit only after owner approval
- Blockers: none; the `RPi AF` rename to an `X_` name remains an owner decision outside this task

## Host/model handoff
- Current host/tool and model ID: Claude Code desktop app, claude-opus-5-5
- Capabilities: file read/edit, shell, Git, local Raspberry Pi camera, subagent delegation; no sudo
- Current writer and assigned paths: this session, paths above
- Snapshot revision and uncommitted work to preserve: base 923bca9a214d4b5d9ba2982e2964e982f26c007a, uncommitted driver changes on task/20261024202957-6bd9-fix-ccd-rpi
- Loaded instructions: AGENTS.md, CLAUDE.md, .factory/bootstrap.md, .factory/project.md; indigo_drivers/AGENTS.md and indigo_test/AGENTS.md not applicable (optional driver, no automated tests)

## Model choice and budget
- Ordinary-to-difficult driver repair with concurrency changes; current host model retained; no budget supplied

## Git delivery tracking
- Task branch / main target: task/20261024202957-6bd9-fix-ccd-rpi / master
- Check evidence: [verification.md](verification.md)
- Integration / publication / cleanup: owner authorized commit, merge and push on 2026-10-24; merge of origin/master d25dcabf2, fast-forward of master, push and branch deletion pending until confirmed in Git

## Latest handoff
- Completed work: libcamera 0.7 build and request-control fixes; registry findings FND-000021..027 and audit-030; additional defects (full sensor size, ROI rectangle, RGB888 byte order, connect failure cleanup, countdown race, abort/disconnect races, persistent advanced settings, focuser relative moves and abort, CCD_FRAME bits-per-pixel range)
- Changed files: indigo_optional_drivers/ccd_rpi/Makefile, indigo_optional_drivers/ccd_rpi/indigo_ccd_rpi.cpp; task evidence rpi-hw-checks.sh, fitsinfo.py
- Decisions: no generator or handler-queue migration; property set unchanged; DRIVER_VERSION 0x002 -> 0x003
- Next: commit, merge origin/master, fast-forward master, push, delete task branch
- Open follow-up: FND-20261024202957-6bd9-fix-ccd-rpi-001 (autofocus)
