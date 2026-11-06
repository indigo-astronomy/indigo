# Task brief

- Task ID: 20261105210224-60c2-plan-mount-touptek
- Owner: Rumen G. Bogdanovski (requester); repository owner Peter Polakovic
- Created: 2026-11-05

## Problem and desired outcome
INDIGO has no driver for the ToupTek ATLAS SH20 strain-wave mount. INDI has one, contributed by ToupTek
(INDI PR #2442, `indi_touptek_sh20`). This task extracts the SH20 serial protocol from that implementation into
a reviewable reference, then specifies and plans SH20 support. The owner first named a separate driver, `indigo_mount_touptek`; on 2026-11-06 this changed to a new dialect of `indigo_mount_lx200`.

This task is planning only. Its deliverables are [protocol.md](protocol.md), [specification.md](specification.md)
and [plan.md](plan.md). Implementation is a follow-up assignment that starts after the owner answers the open
decisions in the specification.

## Users and use cases
SH20 owners who use INDIGO clients (Ain Imager, AstroDSLR, INDIGO-based sequencers, the server UI) and want to:
GOTO and sync, use manual motion, control tracking, pulse guide, park and home, set the site and time, and
configure the SH20 options (EQ/Alt-Az mode, buzzer, meridian behavior).

## Scope and non-goals
In scope: the protocol reference, the driver specification (devices, properties, behavior, failure handling), and
the implementation, simulator, test and registration plan.

Non-goals for this task: driver code, simulator code, tests, README/PROPERTIES/MIGRATION_STATUS edits, and hardware access. Other ToupTek mounts are out of scope until their protocol is known.
Changes to `mount_asi` (off limits) and to the generator are also out of scope.

## Constraints and references
- Root `AGENTS.md`, `indigo_drivers/AGENTS.md`, `indigo_test/AGENTS.md`, `indigo_test/DRIVER_TESTING_RULES.md`.
- Protocol sources are pinned in [protocol.md](protocol.md#sources). INDI code is LGPL, so only protocol facts are used.
- `indigo_drivers/mount_lx200` is the target driver. Its ZWO AM dialect, simulator profiles and `PROTOCOL.md` are the references.

## Initial questions
See [specification.md, Open decisions](specification.md#open-decisions) and [protocol.md, Open questions](protocol.md#open-questions).
