# Task brief

- Task ID: 20261027205926-c759-makefile-driver-lists
- Owner: Peter Polakovic
- Created: 2026-10-27

## Problem and desired outcome
The owner asked (chat, 2026-10-27) to change the driver lists in the root Makefile:
- move `ccd_pentax` from `EXCLUDED_DRIVERS` to `UNTESTED_DRIVERS` and remove it from `DEVELOPED_DRIVERS`;
- add `agent_solver` to `UNTESTED_DRIVERS` (`ccd_libcamera` is already there);
- move `ccd_qhy2` from `UNSTABLE_DRIVERS` to `STABLE_DRIVERS`;
- remove `system_alpaca` from `DEVELOPED_DRIVERS`, leaving the list empty;
- move every `UNTESTED_DRIVERS` driver whose README `## Testing` section records a hardware test to `STABLE_DRIVERS`.

Rule applied: a passing record against the real device counts as hardware testing. Host simulators, fake SDKs, Arduino (ESP32-S3) protocol simulators and a serial loopback do not. Result: `agent_alpaca` (ConformU against FlipFlat, Pocket Powerbox, Ultimate Powerbox 3, LX200 mount), `ccd_libcamera` (ov5647) and `ccd_pentax` (PENTAX K-m) move to `STABLE_DRIVERS`. Not moved: `polaralign_mlastro` (only hardware record is a failed run) and `aux_rts` (FTDI serial loopback only); flagged to the owner.

## Users and use cases
People building and installing INDIGO: `ccd_pentax` is now built, and `ccd_pentax` and `system_alpaca` are now installed.

## Scope and non-goals
Root Makefile only. Non-goals: driver source changes, README or CHANGELOG edits, packaging.

## Constraints and references
`Makefile.drvs` uses `EXCLUDED_DRIVERS` (build) and `DEVELOPED_DRIVERS` (install); the other lists are only written to the ignored, generated `Makefile.inc`.

## Initial questions
None. The owner approved postponing the overdue full repository audit for this task (chat, 2026-10-27).
