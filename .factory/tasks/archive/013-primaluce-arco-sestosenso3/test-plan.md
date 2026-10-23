# Test and regression plan
Status: complete

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001, AC-002 | simulator `sestosenso3_rotator_move` | `--profile sestosenso3` | moves to 45 and 10 end OK | before (fails) and after |
| T-002 | AC-003, AC-004 | simulator `sestosenso3_rotator_sync` | `--profile sestosenso3` | sync OK without a move; moves in synced angles; refused sync keeps the angle | before (fails) and after |
| T-003 | AC-004 | `test_primalucelib` `debug_text_before_a_reply_is_skipped` | scripted peer | sync acknowledged behind debug text | before (fails) and after |
| T-004 | AC-005 | `python3 tools/run_driver_test.py focuser_primaluce` | full suite | all pass | final |
| T-005 | AC-001 to AC-005 | supervised hardware | requester's SESTO SENSO 3 with ARCO | moves OK, abort ALERT, syncs OK | after |

## Baseline evidence
The task 012 record is 61/61. Hardware logs show the three defects.

## Bug reproduction
- T-001 fails without the DEG acknowledgement and with the exact comparison (settles at 44.9999, ALERT).
- T-002 and T-003 fail without the DEG sync acknowledgement and without the prefix skip.

## Negative cases and permission boundaries
A refused sync (`reject` fault) keeps the angle; `rotator_move_failure`; `rotator_abort`.

## Isolation and cleanup
One simulator per case. The server is stopped after the hardware checks.

## Deferred tests and coverage limitations
- ARCO calibration on the SESTO SENSO 3 is not exercised.
- Only Linux was run.
