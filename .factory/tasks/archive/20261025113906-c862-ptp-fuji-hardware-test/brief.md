# Task brief

- Task ID: 20261025113906-c862-ptp-fuji-hardware-test
- Owner: Peter Polakovic
- Created: 2026-10-25

## Problem and desired outcome
The owner asked (2026-10-25) for a test of `indigo_ccd_ptp` with the Fuji camera attached to the Raspberry Pi 5 (`ssh -A indigo@indigosky.local`), mode: non-interactive hardware run, without physical hot-plug. Defects found are fixed, the full scope rerun and committed, not pushed.

## Users and use cases
Users of Fujifilm cameras with `indigo_ccd_ptp`.

## Scope and non-goals
In scope: running the existing hardware suite `indigo_test/hardware/test_ccd_ptp_hw.c` against the Fuji body, suite adaptations needed for Fuji, driver fixes for defects it finds, fake USB regressions, the driver records. Non-goals: hot-plug, other camera bodies.

## Constraints and references
Root, indigo_drivers and indigo_test AGENTS.md; `indigo_test/DRIVER_TESTING_RULES.md`; `indigo_drivers/ccd_ptp/REFACTOR.md`. Base: master at `e49bfce27`.

## Initial questions
Mode and hot-plug answered by the owner (non-interactive hardware, no hot-plug).
