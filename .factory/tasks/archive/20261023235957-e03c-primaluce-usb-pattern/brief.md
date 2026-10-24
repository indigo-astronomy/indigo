# Task brief

- Task ID: 20261023235957-e03c-primaluce-usb-pattern
- Owner: Rumen G. Bogdanovski (requester)
- Created: 2026-10-24

## Problem and desired outcome
The requester asked to add the USB VID/PID of the PrimaLuce controllers to the driver's serial port preference, as the
ZWO mount drivers (`mount_asi`, `mount_lx200`) do.

## Users and use cases
Users with several USB serial devices, whose PrimaLuce port should be preselected.

## Scope and non-goals
In scope: the driver's serial match pattern, a regression test, the documentation.

Non-goals: VID/PID on the Windows port list (framework code); the README.

## Constraints and references
- Root, driver and test AGENTS.md.
- `indigo_drivers/mount_asi` and `mount_lx200` (read only).
- `indigo_libs/indigo_usbserial_utils.c`.

## Initial questions
Resolved: the controllers report the generic Silicon Labs identity, so the VID/PID is combined with the existing product
pattern.
