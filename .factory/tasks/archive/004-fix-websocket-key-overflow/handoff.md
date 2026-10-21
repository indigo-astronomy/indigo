# Task handoff

Detailed results live in [verification.md](verification.md), [quality-review.md](quality-review.md) and [security.md](security.md).

## Current assignment
- Task: 004-fix-websocket-key-overflow
- Phase: complete; execution and reviews done, integration pending
- Responsible role: coordinator
- Allowed write paths: indigo_libs/indigo_server_tcp.c, indigo_test/integration/test_server_websocket.c, indigo_test/Makefile, indigo.xcodeproj/project.pbxproj, .factory/findings/, .factory/state.json, .factory/maintenance.json, this task's records, and the task 003 records that held a false unit-test claim
- Expected output: FND-000045 fixed with a regression test and reverified
- Blockers: none

## Host/model handoff
- Host and model: Claude Code desktop, claude-opus-5-5 as exposed by the host; effort and cost not exposed.
- Capabilities used: file read/edit, shell, local compiler and sanitizers, local Git. No network, delegation, hardware or paid API.
- Writer: this session only. Snapshot: f1a28a893 plus the uncommitted task diff, committed as this task's single commit.
- Loaded instructions: root AGENTS.md, indigo_libs/AGENTS.md, indigo_test/AGENTS.md, the Git, findings, archive and process-check routines.

## Model choice and budget
- Risk: medium, a bounded memory-safety fix in a network-facing parser. The host model was kept. No owner budget is defined.
- Unsuccessful attempts: the first test draft carried a wrong RFC accept constant and a close check that misread the reply body. Both were corrected and the final test was rerun against the base source.

## Git delivery tracking
- Task branch: task/004-fix-websocket-key-overflow, created from task/003-initial-repository-audit at f1a28a893. Target: master; remote origin.
- Reason for the stacked base: master does not contain the unmerged Factory adoption, so integration order is 001, 002, 003, then 004.
- Check evidence: verification.md, on the working tree that became the commit.
- Integration, publication and cleanup: pending, not authorized in this task.

## Latest handoff
- From coordinator to owner, 21 Oct 2026.
- Completed work: over-long Sec-WebSocket-Key values are refused with 400 before any copy; new server WebSocket integration test and an ASan/UBSan variant compiled from the real server source.
- Changed files: see the allowed write paths above, plus findings archive FND-000045.json and repro-websocket-key-fixed.c in this task.
- Decisions: refuse rather than truncate an over-long key, because a truncated key would produce a wrong accept value; the limit is the existing buffer, so no conforming client is affected.
- Corrections: the audit task 003 recorded the unit subset as passing, but its log shows one failure that also stops the RAW image suite. Its verification.md, repository-review.md and process.json were corrected and FND-000047 registered.
- Open findings: FND-000046 (SHA-1 signed shifts, low) and FND-000047 (platform-fragile uni_io test, medium), both nonblocking.
- Next concrete step: owner resumes task 003 for its coverage-gate decision, and decides whether FND-000046 and FND-000047 get repair tasks.

## Handoff history
- 21 Oct 2026: task created by the coordinator after the owner's disposition of FND-000045; task 003 explicitly paused for it.
- 21 Oct 2026: specification, failing tests, fix, verification and reviews completed by the same agent in sequence.

## Final delivery
Execution and reviews are complete and the task is archived in the same commit. Integration into master, publication and branch cleanup remain pending and need owner authorization, after tasks 001, 002 and 003.

## Portable process evidence
process.json records scope, passing checks with their reports, and version 2 content identities captured from the staged diff. Run `./.factory/scripts/process-check --task 004-fix-websocket-key-overflow` against the committed candidate.

## Central findings handoff
- FND-000045 closed as fixed in archive/FND-000045.json with references to 003 security.md and this verification.md.
- FND-000046 and FND-000047 registered open and nonblocking; next_id is 48.
- No reports are pruned, so no pinning was needed. `./.factory/scripts/findings check` passes.
