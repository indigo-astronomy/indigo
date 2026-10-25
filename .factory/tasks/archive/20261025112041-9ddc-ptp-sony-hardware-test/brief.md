# Task brief

- Task ID: 20261025112041-9ddc-ptp-sony-hardware-test
- Owner: Peter Polakovic
- Created: 2026-10-25

## Problem and desired outcome
The owner asked (2026-10-25) for a complete test of `indigo_ccd_ptp` with the Sony A7R4 attached to the Raspberry Pi 5 (`ssh -A indigo@indigosky.local`), mode: non-interactive hardware run, without physical hot-plug. Defects found are fixed, the full scope rerun and committed, not pushed.

## Users and use cases
Users of Sony mirrorless cameras with `indigo_ccd_ptp`.

## Scope and non-goals
In scope: a new hardware suite `indigo_test/hardware/test_ccd_ptp_hw.c`, driver fixes for defects it finds, fake USB regressions, the driver records. Non-goals: hot-plug, other camera bodies.

## Constraints and references
Root, indigo_drivers and indigo_test AGENTS.md; `indigo_test/DRIVER_TESTING_RULES.md`; `indigo_drivers/ccd_ptp/REFACTOR.md`. Builds on the unintegrated task 20261025093224-2d50-fix-ptp-findings; its branch is the base.

## Initial questions
Mode and hot-plug answered by the owner (non-interactive hardware, no hot-plug). The owner approved verifying dropped writes in the driver after the libgphoto2 comparison.
