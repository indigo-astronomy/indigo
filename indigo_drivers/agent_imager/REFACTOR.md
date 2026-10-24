# Imager Agent validation record

Status: hardware-free validation of the Imager Agent alone, of its cooperation with the production Guider and Mount agents, and of the coordination of several Imager Agent instances. Production defects `IMG-1` and `IMG-2` below are fixed and covered by regression tests. Hardware validation was not performed.

## Current-state audit

- The Imager Agent is a hand-written agent/filter driver (`indigo_agent_imager.c`, INDIGO driver API 3). It drives one camera with optional wheel, focuser, rotator and external shutter, and coordinates through `FILTER_RELATED_AGENT_LIST` with a Guider Agent (dithering), a Mount Agent (`ABORT_RELATED_PROCESS`, transit pause), a plate solver and other Imager Agent instances (breakpoint barrier).
- Additional instances are created through `ADDITIONAL_INSTANCES` and named `Imager Agent #2`, `#3`, ... A leader with `AGENT_IMAGER_RESUME_CONDITION` = `BARRIER` copies its breakpoints to the related instances, switches them to `TRIGGER`, starts and aborts them together with its own batch, and releases all of them once every member reported `AGENT_PAUSE_PROCESS` BUSY on a breakpoint (`AGENT_IMAGER_BARRIER_STATE`).
- Dithering: after a light frame the batch sets `AGENT_GUIDER_DITHER.TRIGGER` on the related Guider Agent and waits in phase `DITHERING` for the guider to report BUSY and then OK or ALERT (`do_dither()`).
- The agent is platform independent, has no vendor SDK and no serial transport.

## Tests

- `integration/test_agent_imager.c` (39 cases): the agent alone with the CCD simulator and stub peers for guider, mount, solver and external shutter.
- `integration/test_agent_imager_guider_mount.c` (8 cases, new): the production Imager, Guider and Mount agents with the CCD simulator (imager camera, guider camera and guide output) and the mount simulator on one in-process bus with strict bus locking. Covers the dithering handshake during a guided batch, dither cadence (`FRAMES_TO_SKIP_BEFORE_DITHER`, `DITHER_AFTER_LAST_FRAME`), aborting the imager or the guider while dithering, dither requests without guiding, and the Mount Agent stopping imager and guider processes on slew and park, including a withdrawn `ABORT_RELATED_PROCESS.IMAGER` permission.
- `integration/test_agent_imager_instances.c` (6 cases, new): three instances on three simulator cameras. Covers barrier lockstep with different exposure times (every frame starts on all cameras within 0.5 s and after the previous frame ended everywhere), abort propagation from the leader, a member aborting while the others wait at the barrier (`IMG-1`), leader-only dithering through the production Guider Agent with PRE_CAPTURE and with POST_CAPTURE + PRE_CAPTURE barriers without any member exposure overlapping a dither (`IMG-2`), and independent instances not interfering.
- Both new suites fork every case, give it a private HOME through `indigo_test_use_private_home()` and link the production driver archives. The cooperation suite also passed under AddressSanitizer with the agents and simulators compiled from source.

## Found defects

- `IMG-1` (reproduced, production): a barrier member whose batch was aborted or failed never reaches the barrier again, so the leader and the other members waited in `check_breakpoint()` forever. Root cause: the leader only resumes when every member's `AGENT_PAUSE_PROCESS` is BUSY. Fix: `snoop_barrier_state()` now aborts the whole group when a barrier member publishes `AGENT_START_PROCESS` in ALERT while the leader's batch runs (decision: abort the group rather than continue without the member). Regression: `member abort aborts barrier group`, which also restarts the group afterwards.
- `IMG-2` (reproduced, production): with a PRE_CAPTURE barrier the leader dithered right after its own, shorter exposure while members were still exposing (0.2 s overlap measured). Fix: before dithering, a barrier leader with a related Guider Agent waits until every member is paused on a breakpoint (`wait_for_barrier_members()`, abort and pause aware). Limitation: with a POST_CAPTURE-only barrier the dither is delayed until the members pause again. Regressions: `leader dithers with PRE_CAPTURE barrier`, `leader dithers with POST_CAPTURE and PRE_CAPTURE barrier`. A first version waited even without a related guider and blocked the barrier; the wait now happens inside `do_dither()` after the guider check.
- Test defect (fixed): `download listing payload delete` compared 33 payload bytes against a 32 byte prefix buffer and read past it.
- `DRIVER_VERSION` raised from `0x0300003B` to `0x0300003D`.
- `IMG-3` (reproduced, production, 2026-09-27): a follow-up of the `IMG-1` fix. The aborted exposure batch cleared `AGENT_ABORT_PROCESS` before it moved `AGENT_START_PROCESS` out of BUSY; a barrier member whose ALERT arrived in between made `snoop_barrier_state()` see the batch still running with no abort pending, so it set `AGENT_ABORT_PROCESS` BUSY again and aborted a batch that had already ended, and nothing cleared it. Seen as the intermittent `additional instances and barrier` failure (1 of 10 runs, also in the recorded macOS run). Fix: the batch sets its final `AGENT_START_PROCESS` state before it clears the abort. Regression: `additional instances and barrier` now delivers the member's ALERT inside the leader's `AGENT_ABORT_PROCESS` OK publication and checks that the abort stays OK (5 of 5 runs failed before the fix, 10 of 10 passed after). `DRIVER_VERSION` raised to `0x0300003E`.

## Environment and limitations

- Linux x86_64 (Ubuntu, GNU ld 2.42), simulator only. macOS, Windows and ARM were not run in this session.
- Physical cameras, guiders and mounts were not used; dithering and abort timing are software timing on the simulators.

## Final test summary

- Simulated tests: 55 run, 55 passed (39 + 10 + 6) on macOS arm64 with `0x0300003E` through
  `tools/run_driver_test.py agent_imager --no-record`. Earlier: 53 run, 53 passed (39 + 8 + 6).
- Hardware tests: 0 run, 0 passed.

## Selected audit repairs (2026-10-24)

Before implementation: task `20261024155149-8381-fix-core-audit-findings` scopes this change to derived focus handling (audit 033 and earlier audit 032). Existing architecture, protocol calls, properties and test coverage described above remain applicable. The affected driver is handwritten; no generator migration, transport change or property change is planned. Hardware testing: no devices used; macOS arm64 software tests only. Baseline `make -C indigo_test test-agent-imager` passed all existing cases with driver version `0x03000040` before this driver change. Detailed commands and retained results belong to the task verification report.

Atomic plan:
1. Capture the existing integration baseline and audit equal minima / invalid contrast paths: complete; baseline passed and source confirms zero-variance rejection.
2. Repair focus reduction, reject unusable contrast and guard final focus motion: complete; exact zero-variance minima are retained, empty/nonfinite reductions fail and the final motion path checks derived finiteness. Driver version raised from `0x03000040` to `0x03000041`. Numerical regression fixture passes 54 assertions across two named cases under ASan/UBSan.
3. Add numerical regression coverage, run existing integration and record the final driver run: complete; `python3 tools/run_driver_test.py agent_imager --no-build` recorded 57/57 asserted cases on macOS arm64 with `0x03000041` (2 numerical + 39 imager + 10 cooperation + 6 instance cases). The script updated README Testing and TEST_SUMMARY.md. All prerequisites had been rebuilt.
4. Review the final diff, preserve properties/ABI and document limitations: complete; independent scoped review and final reinspection passed. No properties or ABI changed; no generator migration occurred. An unchanged instance-test cleanup ignores shutdown/stop return values and emitted one attached-device warning in the final run; separate nonblocking FND-20261024155149-8381-fix-core-audit-findings-001 tracks it. Successful case counts do not certify that teardown.

Found defects: identical finite per-star minima produce zero standard deviation and every minimum is discarded by the strict outlier comparison; no surviving values produce a nonfinite focus command. Fully masked contrast can be nonfinite and previously escaped the zero-only estimator check. The planned fix keeps common minima and rejects invalid derived values before movement; it does not add generic change-property input validation.

### Final test summary for selected audit repairs

- Simulated tests: 57 run, 57 asserted cases passed through the recorded driver script; cleanup qualification above remains open and is not counted as independently verified cleanup. Intermediate full run: 56/56 with the numerical assertions grouped in one case.
- Numerical fixture: 2 named cases / 54 assertions, ASan/UBSan passed. Shared image/math regression and defensive configuration/DNS fixtures also passed sanitizers; these are library tests, not additional simulated hardware cases.
- Hardware tests: 0 run, 0 passed. Linux/Windows execution and physical timing were not tested.
