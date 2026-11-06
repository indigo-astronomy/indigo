# Task brief

- Task ID: 20261106223707-63b0-fix-synscan-j2000-epoch
- Owner: Peter Polakovic
- Created: 2026-11-06

## Problem and desired outcome
A user running INDIGO 3.0-9 with a Sky-Watcher Wave 100i reported that `indigo_mount_synscan` shows
`MOUNT_EPOCH` = 2000 but slews to the J2000 coordinates as if they were JNow: a GoTo to Regulus
(J2000 10.1395 h / +11.967°) settled at RA 10.1415 h / DEC +11.969°, while the JNow place of Regulus is
10.1633 h / +11.835°, about 21′ away. The assessment of the report confirmed it from the source: the driver
computes the hour angle as the local sidereal time minus the requested RA with no precession
(`synscan_radec_to_steps()`), and computes the raw RA from the sidereal time minus the encoder hour angle the same way
(`synscan_read_mount_coordinates()`). Both are JNow computations, applied to coordinates the bus declares as J2000.

The owner decided (2026-11-06, chat): keep J2000 on the bus (`MOUNT_EPOCH` stays 2000, read-only) and precess inside
the driver, as the mount base driver does for its horizontal coordinates: J2000 to JNow before the hour angle is
formed for a GoTo, JNow to J2000 after the encoder hour angle is read, so `MOUNT_RAW_COORDINATES`,
`MOUNT_EQUATORIAL_COORDINATES` and the alignment points are all J2000.

## Users and use cases
Every user of a SynScan/EQMOD motor controller (Sky-Watcher, Orion, AZ-GTi, AstroEQ, Wave) slewing from a
J2000 catalogue, including plate-solve-and-sync sequencers; the error grows about 50″ per year since 2000.

## Scope and non-goals
In scope: the two coordinate conversions of the driver, the horizontal coordinates derived from them, the driver
version, the simulator integration tests (targets expressed in J2000, a new epoch case), the hardware test's
hour-angle case for consistency, `MIGRATION_STATUS.md` test count. Non-goals: the other mount drivers whose epoch
handling is pinned the same way (NexStar, iOptron, PMC8, NexStarAUX), the framework's `MOUNT_EPOCH` convention,
the SynScan protocol simulator, hardware validation (no SynScan mount is available in this session).

## Constraints and references
Root AGENTS.md driver and generated-driver rules; `indigo_drivers/AGENTS.md`; `indigo_test/AGENTS.md`;
`indigo_test/DRIVER_TESTING_RULES.md`; driver source `indigo_drivers/mount_synscan/indigo_mount_synscan.driver`;
suite `indigo_test/integration/test_mount_synscan_simulator.c`; hardware suite
`indigo_test/hardware/test_mount_synscan_hw.c`; precession helpers `indigo_libs/indigo/indigo_align.h`
(`indigo_j2k_to_jnow()`, `indigo_jnow_to_j2k()`); the user's report (quoted above); commit 7abd66639
("EPOCH handling unified", 2024-10-04) which removed the driver's former epoch 0 (JNow) declaration.

## Initial questions
None blocking.
