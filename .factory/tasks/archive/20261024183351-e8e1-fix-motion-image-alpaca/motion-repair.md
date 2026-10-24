# Motion completion repair handoff

Task: 20261024183351-e8e1-fix-motion-image-alpaca. Finding: FND-20261023101753-bc8d-complete-repository-audit-027. Single writer: motion subagent, isolated checkout `/private/tmp/indigo-motion-20261024` at original `463eb538daf440fa0f459b53a5f02fa949e244ed`. No commits. Source/docs write ownership released to coordinator.

## Implementation

- mypro2, DMFC, FC3 and ASI compare idle measured integer position with the commanded endpoint, exempting externally tracked motion and preserving established abort ALERT behavior.
- DSD stores the issued STRG target after software backlash and zero clipping, so successful overshoots remain accepted while public targets keep the original request.
- USBv3 stores the physical endpoint from cached origin plus issued signed steps, clipped to controller limits. This preserves reversed absolute moves: current600/request300 emits O00300 and arrives900.
- Shared Lunatico rotator stores the quantized raw step endpoint plus commanded/external ownership; idle off-target commanded motion ends ALERT. Both driver exports get an executable early-stop regression. Fractional angles that map to the same integer endpoint remain successful. Aborts and connection polls reset ownership.
- Eight exported versions incremented and checked-in outputs regenerated with the unchanged generator: mypro2 17→18, DSD23→24, DMFC23→24, FC312→13, ASI40→41, USBv315→16, focuser_lunatico19→20, rotator_lunatico18→19.
- Added real one-shot shortened travel to serial simulators, respecting valid acknowledgments and USBv3 autonomous completion markers; ASI directly supplies a measured intermediate idle result. No generic framework validation or generator semantics changed.

## Tests

Original full tests compiled before production changes. Seven initial early-stop reproducers all failed originally and passed after repair; an eighth explicit Lunatico focuser-export regression was compiled separately against retained original generated/shared source and also failed originally. Focused final run has 21 planned/21 passed checks, including eight early-stop cases, DSD software backlash, USBv3 reverse absolute arrival, Lunatico fractional target and external/hand-controller compatibility cases. Three Lunatico cases rerun after removing obsolete internal floating target storage also pass.

| Driver | Original full baseline passed | Final selected planned/passed |
| --- | --- | --- |
| focuser_mypro2 | 35 | 2/2 |
| focuser_dsd | 58 | 2/2 |
| focuser_dmfc | 38 | 3/3 |
| focuser_fc3 | 39 | 3/3 |
| focuser_usbv3 | 40 | 6/6 |
| focuser_asi | 23 | 2/2 |
| focuser_lunatico | 56 | 1/1 |
| rotator_lunatico | 27 | 2/2 |

All eight original suites finished successfully (316/316). DSD ASan/UBSan early-stop and backlash checks also passed (2/2); the parent recorded these late results after writer release. No hardware or other OS checks performed. No final recorded run or independent approval claimed. Parent must perform final `tools/run_driver_test.py` runs and update README/TEST_SUMMARY. Baseline unrecorded developer runs intentionally left README unchanged.

## Exact evidence paths and reproduction

- `/private/tmp/motion-baseline-<driver>.log`: original full suites, original binaries compiled before edits. Eight suites launched in parallel with `./build/integration/test_<driver>_<kind>` from `indigo_test/`, where kind is sdk for ASI and simulator otherwise.
- `/private/tmp/motion-expected-failure-<driver>.log`: dedicated expected baseline failures; filters `MYPRO2_TEST_FILTER=early_stop`, `DSD_TEST_FILTER=early_stop`, `DMFC_TEST_FILTER=early_stop`, `FC3_TEST_FILTER=early_stop`, `USBV3_TEST_FILTER=early_stop`, `INDIGO_TEST_CASE_FILTER=early` for ASI and `AUX_TEST_FILTER=early_stop` for rotator. Second export uses `AUX_TEST_FILTER=rotator_early_stop ./build/integration/test_focuser_lunatico_original`.
- `/private/tmp/run_motion_checks.py`: exact final developer-check dispatch with every filter and executable; `/private/tmp/motion-check-*.log` and `.tsv` contain outputs, exit codes, actual planned/passed records. The TSV run totals are 21/21.
- Build logs: `/private/tmp/motion-baseline-build.log`, `/private/tmp/motion-baseline-tests-build.log` (initial ASI missing libusb header subsequently resolved by copying existing parent build headers), `/private/tmp/motion-asi-baseline-build.log`, `/private/tmp/motion-fixed-build.log`, `/private/tmp/motion-fixed-tests-build.log`, `/private/tmp/motion-lunatico-final-build.log`.
- `/private/tmp/motion-second-export-baseline-build.log` and `/private/tmp/motion-original-focuser-lunatico/` retain native arm64 original source-object baseline.
- `/private/tmp/motion-second-export-final.log`, `/private/tmp/motion-rotator-final.log`, `/private/tmp/motion-fractional-final.log`: final Lunatico checks after internal structure cleanup.
- `/private/tmp/motion-dsd-sanitize-*`: sanitizer build and two selected check logs, both passed (2/2).

`git diff --check` passes. No shared Makefile, Xcode, MIGRATION_STATUS, properties reference, README or TEST_SUMMARY changes required for source registration: all test coverage extends existing registered files/targets. Parent should update testing summaries via the mandated recorded runner. No public property changes.

## Important build and test limits

The driver Makefile does not directly notice changes to the included Lunatico shared implementation; regenerate or touch the two generated driver C files before rebuilding archives. This handoff increments both generated C driver versions, so import naturally rebuilds them. Final local checks explicitly regenerated/rebuilt both after shared struct cleanup.

Simulator/fake-SDK validation cannot certify real motor mechanics, firmware tolerances, vendor SDK or platform behavior. Integer endpoint comparisons use controller/SDK step coordinates; no arbitrary mechanical tolerance is assumed. Retained original DSD backlash test explicitly allows measured3100/requested3000 and measured3900/requested4000; the repair preserves those public targets.
