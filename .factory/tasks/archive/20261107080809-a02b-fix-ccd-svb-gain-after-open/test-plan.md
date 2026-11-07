# Test and regression plan

Status: accepted

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 gain before the first exposure | AC-001, AC-002 | integration, new `gain_is_accepted_before_the_first_exposure` in `test_ccd_svb_sdk.c` | fresh fixture (fake SDK refuses gain until the exposure is written), connect, `CCD_GAIN` = 20 without any exposure | `CCD_GAIN` OK, value 20, `config[SVB_GAIN]` = 20, `exposure_written` set by the connect | before the fix (fails: ALERT at 0), after (passes) |
| T-002 refusal reproduction | AC-003 | the same case against the master driver | as T-001 | FAIL at the OK wait: `CCD_GAIN` ALERT at 0 | step 2 |
| T-003 preserved behaviour | AC-005 | the 48 existing cases | unchanged | all pass | baseline and final |
| T-004 version | AC-006 | recorded test line | `python3 tools/run_driver_test.py ccd_svb` | line records 3.0.0.35 | final |
| T-005 hardware gain before the first exposure | AC-007 | hardware, `hardware_workflows()` step `gain_before_first_exposure()` | after the first connect, before any exposure | compiles; on hardware: `CCD_GAIN` OK at the changed value, restored | final (build only) |
| T-006 logging | AC-004 | code inspection | `svb_set_number()` | error line with both SDK results on failure, debug line on success | review |

## Baseline evidence
Recorded in verification.md after step 1.

## Bug reproduction
Step 2 builds the updated suite (fake SDK refusal) against the unchanged driver: T-001 fails with `CCD_GAIN` in
ALERT at the previous value, reproducing the user's report hardware-free. The restore of a saved gain at connect
goes through the same handler and is covered by the same refusal.

## Negative cases and permission boundaries
Not applicable: no new input or permission boundary.

## Isolation and cleanup
The suite uses its private configuration folder and image directory; `make -C indigo_test test-clean` after the
run.

## Deferred tests and coverage limitations
No hardware run (no SVBONY camera available); the hardware step is compiled only. Owner: Peter Polakovic;
follow-up: the reporting user's confirmation with the fixed driver on the SV705C and SV905C2.
