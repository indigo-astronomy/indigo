# Task brief

- Task ID: 20261025134403-8427-fix-sony-capture-deadline
- Owner: Peter Polakovic
- Created: 2026-10-25

## Problem and desired outcome
Fix finding FND-20261025113906-c862-ptp-fuji-hardware-test-001: on the Sony ILCE-7RM4 an exposure after a reconnect intermittently never completes and CCD_EXPOSURE stays BUSY. Outcome: reconnect and capture work reliably on the camera, each hardware observation is reproduced in the fake USB backend, and the recorded hardware run passes.

## Users and use cases
Imaging clients that disconnect and reconnect a Sony A7R IV and then expose.

## Scope and non-goals
In scope: `indigo_drivers/ccd_ptp/` (Sony capture path, session reopen), `indigo_test/integration/test_ccd_ptp_sdk.c`, the PTP fake build flags in `indigo_test/Makefile`, records in the driver `REFACTOR.md`, `README.md` test record, `TEST_SUMMARY.md`, `MIGRATION_STATUS.md` counts. Non-goals: other vendors, hot-plug, Windows.

## Constraints and references
Owner request in conversation 2026-10-25 ("oprav FND-...-001", camera on the RPi5 at indigosky.local); non-interactive hardware mode (fix defects without asking). Previous evidence: `.factory/tasks/archive/20261025113906-c862-ptp-fuji-hardware-test/findings.md`.

## Initial questions
None.
