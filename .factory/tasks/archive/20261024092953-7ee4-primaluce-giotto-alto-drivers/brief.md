# Task brief

- Task ID: 20261024092953-7ee4-primaluce-giotto-alto-drivers
- Owner: Rumen G. Bogdanovski (requester)
- Created: 2026-10-24

## Problem and desired outcome
The requester asked for INDIGO drivers for the PrimaLuce Lab GIOTTO flat panel and ALTO-1 cover, built on the shared
`primalucelib` and following the conventions of the existing flat panel and dust cap drivers. The GIOTTO and the ALTO are
separate USB devices and the generator shares one connection between the devices of a driver, so the requester chose two
drivers (2026-10-24). The requester then asked to finish the GIOTTO first. This task delivers `indigo_aux_giotto`; the
ALTO-1 driver is a follow-up task.

## Users and use cases
GIOTTO owners taking flat frames, locally or through clients such as Ain Imager.

## Scope and non-goals
In scope:
- `primalucelib` panel calls and unit tests;
- the GIOTTO simulator, driver, integration and opt-in hardware suites;
- build and project registration (Makefiles, server, Windows, Xcode);
- documentation (`README.md`, `REFACTOR.md`, `PROTOCOL.md`, `PROPERTIES.md`, `MIGRATION_STATUS.md`).

Non-goals:
- the ALTO-1 driver (follow-up task);
- generator changes.

## Constraints and references
- Root, driver and test AGENTS.md; `DRIVER_TESTING_RULES.md`.
- PrimaLuce Lab "GIOTTO Communication Protocol" v2.1 (`~/Downloads`, not in the repository).
- INDI `giotto.cpp`, `primalucacommandset.cpp`.
- The requester's GIOTTO (firmware 1.03.09), with full access granted on 2026-10-24.

## Initial questions
Resolved: two drivers; both devices available; the recurring repository audit deferred by the requester for this task;
light off at disconnect requested; README approved (all 2026-10-24).
