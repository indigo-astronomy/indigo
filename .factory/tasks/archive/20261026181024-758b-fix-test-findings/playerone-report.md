# Player One test diagnosis and repair

Role: developer/diagnostician. Scope: FND-000029; tests and driver coverage notes only.
Test-source baseline `bab591d40`; task working tree on macOS arm64. Initial
checks linked the pre-existing `libindigo` build artifact, which root later found
to predate current source and rebuilt. Initial results are diagnostic only; the
final repetitions below link the rebuilt current library. No hardware,
physical timing measurement, production source, generated output, or driver
version change. Same agent implements and executes checks; root owns final review.

## Commands and results

Working directory `/Users/polakovic/Development/indigo` for all commands.

- `make -C indigo_test build/integration/test_ccd_playerone_sdk`: exit 0, baseline and repaired build. Build logs `/tmp/playerone-build.log` and `/tmp/playerone-fixed-build.log`.
- Baseline Python subprocess loop invokes `indigo_test/build/integration/test_ccd_playerone_sdk cooler_individual_failures` and `indigo_test/build/integration/test_ccd_playerone_sdk "Guiding and abort"` 10 times each: 20/20 passed. Per-run logs `/tmp/playerone-baseline-<n>-<filter>.log`.
- Baseline `indigo_test/build/integration/test_ccd_playerone_sdk`: exit 0, 52/52 passed; `/tmp/playerone-baseline-full.log`.
- First controlled-poll experiment required a fresh publication for an unchanged initial OK poll and failed. `indigo_bus.c:1203` suppresses unchanged updates; revised helper verifies actual callback completion through a queue barrier and exactly one SDK read, then checks the resulting state.
- Cooler-only repair targeted repeats: 20 runs of each historical filter, 40/40 passed. Full suite 52/52 passed; `/tmp/playerone-fixed-full.log`. These runs preceded the long-exposure gate addition and used the pre-existing library artifact; they are intermediate evidence, not the final candidate.
- Gated long-exposure first run: exit 0; `/tmp/playerone-gated-abort.log`. After root rebuilt `libindigo`, `touch indigo_test/integration/test_ccd_playerone_sdk.c && make -C indigo_test build/integration/test_ccd_playerone_sdk` exits 0. Final source/current-library subprocess repetitions pass 20/20 for each target (40/40 total), every process exit 0; logs `/tmp/playerone-final-<n>-<filter>.log`. Root owns the final recorded full-suite run.

## Diagnosis and change

The fault case used a recurring accelerated poll (10 ms) while injecting errors
and changing cooler/temperature requests. Its checks could race other callbacks
and bus publication suppression. This repair controls poll execution only inside
this case through the already-existing test scheduling interception. Queue
barriers establish that preceding property handlers and the real poll completed.
Each of four read faults and three write faults still must produce ALERT, and
recovery must produce OK. Recurring scheduler behavior remains covered by the
separate temperature/disconnect and slow-initialization tests.

The long-exposure historical failure did not reproduce in baseline standalone
or full-suite runs. The latest-state wait loop is unchanged since `e0a87ff703`
(2026-09-08), so no later revision establishes a causal fix for the old load-only
failure. Rather than closing from repetition evidence, the case now controls the
operation endpoints: a test-only SDK gate holds EAST guide-off while the test
asserts guider BUSY, camera exposing and no delivered image. A queue barrier
after releasing the gate establishes completion before guide OK/off is checked.
A second SDK gate holds abort at POAStopExposure; BUSY/exposing are required
before release, and an SDK stop count plus queue barrier precedes exposure
ALERT/abort OK/no-image checks. Fresh reacquisition requires both a delivered
image and exposure OK after queue drain. Every old behavior assertion is
preserved, with stronger explicit in-flight and completion assertions. Global
wait_state semantics and production code remain unchanged. Cleanup releases
both gates even after an assertion failure. This addresses the test scheduling
assumptions without claiming a reproduced historical driver defect or universal
load certification.

Case count unchanged: 52 simulator/fake SDK, 0 hardware added. Root must run the
final recorded test via `tools/run_driver_test.py ccd_playerone`, update the
Testing record/summary and reconcile finding status with this limitation.

## Final handoff

Source is stable; tests and coverage notes only. `git diff --check` for the two
assigned files exits 0. No case was removed, weakened or added: existing 52 cases
remain registered. Current-library final targeted verification runs 40 cases,
40 pass, physical hardware 0 run/0 pass. Root must attach its full recorded
52-case result before final finding closure. The test scheduling assumptions
are repaired through deterministic SDK gates and queue completion; the exact
historical load-only failure was not reproduced, so do not describe this as a
reproduced driver bug or claim hardware/load certification.
