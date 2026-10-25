# Task brief

- Task ID: 20261025074335-0b62-fix-rpio-gpio-findings
- Owner: Peter Polakovic
- Created: 2026-10-25

## Problem and desired outcome
The owner asked (2026-10-25) to fix the active findings that concern the Raspberry Pi, using the attached Raspberry Pi 3B+ and Raspberry Pi 5, and then to verify the aux_rpio change on both. The active RPi findings are FND-000014 to FND-000018 and FND-20261023065605-60d4-repository-audit-011 (aux_rpio and aux_asiair, shared sysfs layer) and FND-20261024202957-6bd9-fix-ccd-rpi-001 (ccd_rpi autofocus).

## Users and use cases
Users of the Raspberry Pi GPIO and ZWO ASIAIR power port drivers; other programs that share the GPIO header.

## Scope and non-goals
In scope: export ownership in the shared sysfs layer, pulse-end write failure in both drivers, regression tests, closure evidence for the findings already resolved in current source. Non-goal: ccd_rpi autofocus, which needs an autofocus camera that neither Pi has (see the handoff).

## Constraints and references
Root, indigo_drivers and indigo_test AGENTS.md; indigo_linux_drivers/aux_rpio/REFACTOR.md; indigo_linux_drivers/aux_asiair/REFACTOR.md.

## Initial questions
None.
