# Test and regression plan
Status: complete

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001 | `indigo_test/unit/test_primalucelib.c` `a_duplicate_reply_is_discarded`, `a_late_reply_is_skipped`, `sestosenso3_log_line_before_loglevel`, `only_stale_replies_fail_the_request` | scripted PTY peer | the right reply answers each request; only stale replies fail | before and after the fix |
| T-002 | AC-002 | simulator `sestosenso3_uncalibrated` | `--profile sestosenso3-uncalibrated` | ALERT "not calibrated", no GOTO | after |
| T-003 | AC-003, AC-004 | simulator `sestosenso3_calibration` | `--profile sestosenso3` | sequence without CAL_DIR; second START refused; END publishes the delayed new maximum | after; mutation check |
| T-004 | AC-001, AC-003, AC-004, AC-005 | simulator `sestosenso3_calibration_restart` | `--profile sestosenso3` | restart message; request during the step refused; one Init; abort sends StopMotor and stop_calib; moves refused afterwards | after |
| T-005 | AC-004 | simulator `calibration_end_without_run` | normal | ALERT, no CAL_FOCUSER | after |
| T-006 | AC-006 | simulator `move_not_started` | `ignore` fault | ALERT with reason | after |
| T-007 | AC-002, AC-003 | `sestosenso3_profile` (LOGLEVEL line, DIMLEDS ack) | `--profile sestosenso3` | connect and LED change OK | after |
| T-008 | AC-007 | `python3 tools/run_driver_test.py focuser_primaluce` | full suite | all pass | final |
| T-009 | AC-001 to AC-004 | supervised hardware session | requester's SESTO SENSO 3 | moves and calibrations work | during the task |

## Baseline evidence
The task 010 record is 55/55 on Linux x64. The hardware defects were reproduced from server logs
(verification.md).

## Bug reproduction
- T-001: the first two cases fail against the previous library.
- T-003: fails with the previous range poll (publishes 100000).
- The hardware defects are reproduced in the logged sessions.

## Negative cases and permission boundaries
Refused calibration requests, refused moves, and stale-only replies.

## Isolation and cleanup
Each simulator case runs its own simulator process and temporary journal. No server is left
running after the session.

## Deferred tests and coverage limitations
- SESTO SENSO 3 abort during calibration, and the HIZ case, are simulator-only.
- macOS and Windows are not run.
- Hot-plug is not part of this task.
