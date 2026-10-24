# Task brief

- Task ID: 20261024202957-6bd9-fix-ccd-rpi
- Owner: Peter Polakovic
- Created: 2026-10-24

## Problem and desired outcome
The owner asked to build `indigo_optional_drivers/ccd_rpi`, check access to the Raspberry Pi camera, and then fix all defects of the driver. The driver did not build against libcamera 0.7.2 (C++20 `std::span`, removed `libcamera::Span`), exposures failed because libcamera 0.7 rejects replacing `Request::controls()`, and the central registry holds open findings FND-000021 to FND-000027 and FND-20261024121920-3bd2-repository-audit-030 for this driver. A source review found further defects (ROI rectangle, colour order, full-resolution size, connection failure cleanup, abort races, focuser semantics, CCD_FRAME handling).

## Users and use cases
INDIGO users capturing images with CSI cameras on Raspberry Pi through `indigo_ccd_rpi`.

## Scope and non-goals
- In scope: `indigo_optional_drivers/ccd_rpi/` source and Makefile, the listed findings, defects found by review, version bump.
- Non-goals: generator migration, handler-queue migration, new properties, renaming the non-`X_` `RPi AF` property (public rename needs owner decision), README changes, raw CSI-2 packed format support.

## Constraints and references
- Hardware available: Raspberry Pi 3B+ with OV5647 (Camera Module v1, colour, no autofocus); libcamera 0.7.2+rpt.
- Autofocus/lens and monochrome paths cannot be exercised on this hardware.
- No commit without owner approval (owner said "Not yet").

## Initial questions
- Rename `RPi AF` to an `X_` name? Left for the owner.
