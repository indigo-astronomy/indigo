# Development plan

Status: done

## Approach and affected components
- `indigo_drivers/ccd_svb/indigo_ccd_svb.driver`: exposure write in the CCD `on_connect` after a successful
  `initialize_camera()`; logging in `svb_set_number()`; version 35. Regenerate `indigo_ccd_svb.c`, `.h` and
  `_main.c` with `make -C indigo_drivers/ccd_svb -f ../../Makefile.drv all`.
- `indigo_test/integration/test_ccd_svb_sdk.c`: fake SDK state `exposure_written` and opt-out
  `accepts_gain_before_exposure` in `mock_camera`; refusal in `SVBSetControlValue()`; reset in `SVBOpenCamera()`;
  new case `gain_is_accepted_before_the_first_exposure` registered in `tests[]`.
- `indigo_test/hardware/test_ccd_svb_hw.c`: `gain_before_first_exposure()` step right after the first connect of
  `hardware_workflows()`.
- `MIGRATION_STATUS.md`: `ccd_svb` row automated-test count 48 -> 49.

## Baseline
- Commands and environment: macOS arm64 (Darwin 27.0.0), Apple clang 21; INDIGO library and drivers already
  built under `build/`; `make -C indigo_test build/integration/test_ccd_svb_sdk` and the binary from
  `indigo_test/`.
- Existing failures: see verification.md (baseline section).
- Repository revision / working-tree state: task branch from master 0d1e4672e.

## Hardware-test decision
No hardware run: no SVBONY camera is available in this session. The hardware suite gains the pre-exposure gain
step and is compiled only; the reporting user is asked to confirm on the SV705C/SV905C2.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | baseline: build and run the unchanged suite | AC-005 | None | result recorded in verification.md | done: 48/48 |
| 2 | fake SDK refusal and the new case; run against the unchanged driver | AC-003 | 1 | the new case fails with the master driver, the others pass | done: the new case, `Matrix control_failures` and `Configuration roundtrip` fail with the master driver (ALERT gain; CONFIG restore ALERT), 46/49 pass; the SV305PRO refused the gain before the first exposure too |
| 3 | driver: exposure write at connect, logging, version 35; regenerate | AC-001, AC-002, AC-004, AC-006 | 2 | regenerated outputs differ only as explained | done: version define, `svb_set_number()` and the `on_connect` block |
| 4 | hardware suite step; `MIGRATION_STATUS.md`; final recorded run `python3 tools/run_driver_test.py ccd_svb`; `make -C indigo_test build/hardware/test_ccd_svb_hw` | AC-005, AC-007 | 3 | all cases pass, README and TEST_SUMMARY.md updated, hardware suite compiles | done: 49/49 and SV305PRO 1/1 recorded |
| 5 | independent quality review; security review; verification | all | 4 | reports with `Verdict: pass` | |

## Test strategy
Fake-SDK integration tests in process. The fake SDK models the SDK's refusal of a gain write before the first
exposure write, so the new case fails against the pre-fix driver exactly as the user's cameras did and passes
with the fix; the existing 48 cases guard the rest of the driver.

## Security risks and mitigations
None: one constant SDK write and log lines.

## Dependencies and architecture decisions
No new dependency. The workaround is placed in the generator-owned `on_connect` block (not in `svb_open()`), after
the control enumeration, so the `CCD_EXPOSURE` property keeps showing the SDK's own value at connect as in 3.0-7.

## Migration, rollout, and rollback
None needed; revert the commit to roll back.

## Deviations and owner decisions
- The owner asked (2026-11-07, chat) to test with the attached SV305PRO: the hardware suite is run (non-interactive, no hot-plug case) before and after the fix. Against the unchanged driver the SV305PRO refused the gain before the first exposure as well, so the behaviour is not limited to the user's two models.
- The reproduction showed that `Configuration roundtrip` (saved settings restored at connect) fails with the quirk modelled: that is the user's second symptom (the agent refusing the camera on a configuration restore in ALERT), now covered by the existing case.
