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

## Environment and limitations

- Linux x86_64 (Ubuntu, GNU ld 2.42), simulator only. macOS, Windows and ARM were not run in this session.
- Physical cameras, guiders and mounts were not used; dithering and abort timing are software timing on the simulators.

## Final test summary

- Simulated tests: 53 run, 53 passed (39 + 8 + 6).
- Hardware tests: 0 run, 0 passed.
