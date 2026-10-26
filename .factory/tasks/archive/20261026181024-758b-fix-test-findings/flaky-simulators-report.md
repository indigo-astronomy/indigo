# Simulator test diagnosis and repair

Role: developer/diagnostician. Reviewed base bab591d40 and working-tree test changes on task/20261026181024-758b-fix-test-findings. Assigned files only; no production driver or generator changes. Host: macOS arm64, clang universal arm64/x86_64 builds. Physical hardware is not used. Case counts are unchanged.

## Optec descriptor accounting (e308 batch 001)

The original `OPTEC_TEST_FILTER=descriptor_cleanup build/integration/test_focuser_optec_simulator` fails at the final descriptor comparison. A temporary diagnostic measured 3 descriptors before bring-up, 5 after attachment, 6 while connected and 5 after shutdown. `lsof` identified the retained pair as a pipe, with no retained serial descriptor. Source tracing links this to process-global libusb initialization by serial-port hotplug registration (`indigo_enumerate_serial_ports` / `register_serial_ports_hotplug` / `indigo_start_usb_event_handler`). It deliberately outlives individual driver shutdown.

The repaired case explicitly initializes the shared USB event handler before taking its baseline, then requires exact descriptor balance after each of three complete INIT/connect/disconnect/SHUTDOWN cycles. This retains detection of both attach and connection leaks and adds repeated lifecycle coverage. Three focused runs passed (nine checked lifecycles) against the initial library. No production version bump applies.

## iOptron RA wrap (e308 batch 003)

The simulator starts at current LST minus two hours. The old movement helper subtracts RA linearly, so east movement across 24h becomes a large negative delta. The old west checks also compare to `ra + 0.2`, permitting success before real west movement. Angular movement/stability checks now use `remainder(change, 24)` for RA only, preserving the signed distance thresholds. West checks compare against the actual starting readback. Every manual-motion dialect now synchronizes near 23.99h before the scenario, deterministically exercising the midnight boundary.

A temporary baseline build restored only the old linear movement helper while retaining the near-wrap setup. `/tmp/test_ioptron_linear_baseline ioptron_manual_motion_8407` failed with exit 1: `RA did not change by 0.05 from 23.99 (now 3.36556)`. The corrected build passed the same filtered variant. Initial unmodified six-variant run passed 6/6, illustrating the old scenario's dependence on sidereal time. The rebuilt-library final six-variant run passed 6/6. Broad final recorded runs are coordinated by the root agent.

## Mount simulator abort synchronization (e308 batch 002)

Six initial filtered original-case runs passed; the historical intermittency was not independently reproduced in those runs. Source review found that the test accepts stale property states, the cache copies state before items, and BUSY request acceptance does not prove that the queued slew handler has started. The repaired case waits for fresh completed SYNC/GOTO/abort publications and actual coordinate movement before abort. It retains exact stopped-coordinate equality, requested-target checks after alignment changes, successful fresh GOTO, and idle-abort tracking/position checks.

One intermediate attempt required a subsequent fresh ALERT publication to prove stable position. It failed because unchanged updates are suppressed; that requirement was removed and the existing 1.2-second stable-position observation retained. These intermediate failures are not final candidate results. The original full suite passed 28/28 against the initial library. With the rebuilt current library, the repaired filtered case passed three consecutive runs; the interval/equality and target assertions all remain unchanged.

## Commands and evidence status

All build commands start in the repository root; test commands start in `indigo_test/`.

- `make -C indigo_test build/integration/test_focuser_optec_simulator build/integration/test_mount_simulator build/integration/test_mount_ioptron_simulator`: exit 0.
- `OPTEC_TEST_FILTER=descriptor_cleanup build/integration/test_focuser_optec_simulator`: original exit 1; repaired focused runs 3/3 exit 0, each covering three lifecycles.
- `INDIGO_TEST_CASE_FILTER=mount_abort_allows_fresh_goto build/integration/test_mount_simulator`: original six repetitions exit 0; rebuilt-library final three repetitions exit 0.
- `build/integration/test_mount_ioptron_simulator ioptron_manual_motion`: original 6/6 pass; forced-wrap old-helper build exit 1, corrected 8407 1/1 pass; rebuilt-library final six-variant run 6/6 pass, exit 0.

The coordinator identified an outdated pre-existing libindigo build. Earlier runs used that old binary as diagnostic evidence. Final focused checks below use the rebuilt library; final broad acceptance uses recorded runs through `tools/run_driver_test.py`. No README/summary/finding lifecycle writes are performed by this agent.

## Rebuilt-library focused validation

After the coordinator rebuilt `libindigo`, the final Optec descriptor case passed three consecutive runs (nine lifecycle checks), and the final mount abort case passed three consecutive runs. See `flaky-final-focused.log`. The original full mount baseline passed 28/28 (`mount-original-full-baseline.log`). The rebuilt-library iOptron near-wrap final six-variant run passed 6/6 with exit 0 (`ioptron-wrap-final.log`). Final recorded full suites belong to the coordinator.
