# Findings repaired during this unfinished task

These findings describe interim candidates and the inherited USB cleanup defect, not unresolved final source blockers. Physical acceptance remains independently blocked.

- FND-20261026212631-884a-complete-pentax-001: Queued connection and teardown can deadlock or access released properties. Public connection reproducer stalled; per-device queue owns master locking and is now drained before property teardown. Fake public connect/disconnect and ASan pass.
- FND-20261026212631-884a-complete-pentax-002: Native image property can retain a freed capture buffer. Framework retains native BLOB data; last_image now owns it through replacement/detach. Re-enumeration and post-abort BLOB access pass under ASan.
- FND-20261026212631-884a-complete-pentax-003: Readout finalizers can starve abort requests. Normal-priority readout and urgent abort replace the priority inversion. A gated multi-block download abort and immediate recovery pass.
- FND-20261026212631-884a-complete-pentax-004: Camera acknowledgements can falsely accept unapplied exposure settings. Control and temporary exposure settings now require full-state readback; malformed and refused settings fail before trigger. Refused-program and zero-readback regressions pass.
- FND-20261026212631-884a-complete-pentax-005: Inactive upload error states can falsely fail a later exposure. Capture completion checks only enabled upload outputs. CLIENT-to-LOCAL and LOCAL-to-CLIENT recovery regressions pass.
- FND-20261026212631-884a-complete-pentax-006: Completed or aborted capture can disable later countdowns. Successful starts now resume the shared countdown. Public countdown progress after completion and abort/restart passes.
- FND-20261026212631-884a-complete-pentax-007: USB auto-detach can leave the system driver detached after failed claim. A privileged failed claim left no Mass Storage child. Public kernel reattach restored it; explicit detach/claim/release/reattach now has transactional ownership and three rollback regressions pass.
