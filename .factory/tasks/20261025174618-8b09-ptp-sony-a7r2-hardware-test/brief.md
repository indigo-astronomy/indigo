# Task brief

- Task ID: 20261025174618-8b09-ptp-sony-a7r2-hardware-test
- Owner: Peter Polakovic
- Created: 2026-10-25

## Problem and desired outcome
Owner request (2026-10-25): test the PTP driver with the Sony A7R II now attached to the Raspberry Pi 5. Outcome: the hardware suite passes on the ILCE-7RM2, every defect found is fixed and reproduced hardware-free, the run is recorded.

## Users and use cases
Imaging clients using older Sony bodies (old Sony PTP API: stepping ISO/shutter/aperture, event-driven ObjectAdded).

## Scope and non-goals
In scope: `indigo_drivers/ccd_ptp/` Sony path, `indigo_test/hardware/test_ccd_ptp_hw.c`, `indigo_test/integration/test_ccd_ptp_sdk.c`, `indigo_test/Makefile` fake flags, records. Non-goals: other vendors, hot-plug, Windows.

## Constraints and references
Non-interactive hardware mode (standing owner instruction to fix defects without asking), no physical hot-plug, conserve camera battery (no repeated full suite runs without need).

## Initial questions
None.
