# Specification

Status: approved by owner request ("fix all ccd_rpi defects")

## Problem and outcome
`indigo_ccd_rpi` must build against libcamera 0.7.2, capture images from CSI cameras and be free of the defects recorded for it in the findings registry and found by review. See [brief.md](brief.md).

## In scope
- AC-001: the driver builds with libcamera 0.7.2 (C++20).
- AC-002: exposures complete; full sensor resolution, binning modes and subframes produce images of the requested geometry.
- AC-003: abort, disconnect and frame loss never leave CCD_EXPOSURE BUSY or hang a callback; a stopped or failed exposure publishes ALERT and no image.
- AC-004: findings FND-000021 to FND-000027 and audit-030 are fixed.
- AC-005: X_CCD_ADVANCED settings apply to every exposure; RGB24 and BGR24 give correct colour order.
- AC-006: connection failures release every acquired resource; repeated connect cycles do not leak properties.

## Out of scope
Autofocus redesign (recorded as FND-20261024202957-6bd9-fix-ccd-rpi-001), property renames, generator or handler-queue migration, README changes.
