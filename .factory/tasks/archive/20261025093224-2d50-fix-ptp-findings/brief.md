# Task brief

- Task ID: 20261025093224-2d50-fix-ptp-findings
- Owner: Peter Polakovic
- Created: 2026-10-25

## Problem and desired outcome
The owner asked (2026-10-25) to fix all active findings that concern PTP: FND-20261023101753-bc8d-complete-repository-audit-019 (device-info decoders without payload bounds), FND-20261024121920-3bd2-repository-audit-026 (descriptor and vendor-extension counts beyond destination capacities), -027 (object downloads publish the metadata size instead of the downloaded size) and -028 (Canon text property encoding without a terminator).

## Users and use cases
Users of `indigo_ccd_ptp` with Canon, Nikon, Sony, Fuji and Olympus cameras over USB or ICA.

## Scope and non-goals
In scope: `indigo_drivers/ccd_ptp/` decoders and download paths, fake USB regression tests in `indigo_test/integration/test_ccd_ptp_sdk.c`, the driver's REFACTOR.md, README test record, TEST_SUMMARY.md, MIGRATION_STATUS.md counts, the four finding records. Non-goals: a general rewrite of every PTP decoder into a bounded cursor; physical camera testing.

## Constraints and references
Root, indigo_drivers and indigo_test AGENTS.md; `indigo_drivers/ccd_ptp/REFACTOR.md`; audit reports of tasks 20261023101753-bc8d-complete-repository-audit and 20261024121920-3bd2-repository-audit.

## Initial questions
None. Test mode: non-interactive simulator (fake USB), no hardware.
