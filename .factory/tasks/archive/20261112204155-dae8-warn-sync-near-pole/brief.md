# Task brief

- Task ID: 20261112204155-dae8-warn-sync-near-pole
- Owner: Peter Polakovic
- Created: 2026-11-12

## Problem and desired outcome
A Sky-Watcher Wave 100i user synced the mount in its home position to RA 6:00 / Dec 90 before every test. With the framework alignment model (SINGLE_POINT / NEAREST_POINT), that sync adds an alignment point whose RA offset is (12h - LST): meaningless at the pole, where every right ascension points the same way, but applied to every later GoTo, so the mount slewed hours away from its targets. The owner asked for a warning through `indigo_send_message()` in `indigo_mount_driver` when a sync is made close to the pole.

## Users and use cases
Clients and agents that sync a mount through MOUNT_ON_COORDINATES_SET.SYNC with the framework alignment model; users who sync on the pole while the mount is in its home position.

## Scope and non-goals
In scope: the SYNC branch of `MOUNT_EQUATORIAL_COORDINATES` in `indigo_libs/indigo_mount_driver.c` and a unit test. Not in scope: refusing the sync, changing the alignment model, CONTROLLER-mode syncs handled by drivers, the SynScan driver.

## Constraints and references
Root AGENTS.md formatting rules; indigo_libs/AGENTS.md; indigo_test/AGENTS.md. Investigation of the SynScan driver and the 3.0-7 code is in this session's conversation; the conclusions are in specification.md.

## Initial questions
None blocking. Threshold chosen by the implementer: 85 degrees (see specification.md).
