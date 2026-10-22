# Task brief

- Task ID: 010-primalucelib
- Owner: Rumen G. Bogdanovski (requester); project owner Peter Polakovic
- Created: 2026-10-22

## Problem and desired outcome
More PrimaLuce Lab drivers are planned. The JSON protocol used by every current PrimaLuce
controller is implemented only inside `indigo_drivers/focuser_primaluce`: the transport
(USB serial and HTTP `ajax.php`), framing, JSON parsing (jsmn), reply-path lookup, error
detection and about 100 ad-hoc request strings and path arrays. A new driver would have to
copy it.

Desired outcome: a reusable protocol library ("SDK") in `indigo_drivers/shared/primalucelib/`.
It has comprehensive, self-describing function names and named constants for the protocol
keys and values, and covers the JSON command sets of the focusers (SESTO SENSO 2/3, ESATTO),
the ARCO rotator, the GIOTTO flat panel and the ALTO cover. `focuser_primaluce` is refactored
to use it with unchanged observable behavior and unchanged wire traffic, and the tests are
adjusted to match.

## Users and use cases
- Driver developers writing new PrimaLuce drivers (for example a GIOTTO or ALTO driver).
- Maintainers of `focuser_primaluce`, who get a smaller driver.
- End users: no visible change is intended.

## Scope and non-goals
In scope:
- New library `indigo_drivers/shared/primalucelib/` (header + implementation), portable C,
  using `indigo_uni_io` for transport.
- Refactoring `focuser_primaluce` (`.driver` input and regenerated outputs) to use the library.
- New hardware-free unit tests for the library (request formatting, reply parsing, error
  forms, transport framing over a pseudo-terminal) and characterization of the driver before
  and after the change (existing simulator suites + ordered protocol trace comparison).
- Registration of the new files in the Xcode and Visual Studio projects and the test Makefile.
- Driver `REFACTOR.md` record and `PROPERTIES.md` check.

Non-goals:
- No new driver (GIOTTO/ALTO API is provided and unit-tested, not wired to a driver).
- No Sesto Senso v1 ASCII protocol (owner decision 2026-10-22).
- No behavior change, no property change, no generator change.
- No hardware run (owner chose a non-interactive simulator run).

## Constraints and references
- Root `AGENTS.md`, `indigo_drivers/AGENTS.md` (characterization first, preserve call
  order and manner, reference trace), `indigo_test/AGENTS.md`.
- Precedent for cross-driver shared code: `dome_dragonfly` and `rotator_lunatico` include
  `../<driver>/shared/*.{h,c}` from their `.driver` blocks.
- Protocol knowledge: the current driver (hardware-verified on a SESTO SENSO 2) and the INDI
  `primalucacommandset` analysis in the session scratchpad (not committed).
- Off-limits drivers are untouched. README changes only by the test-run recording script.

## Initial questions
Resolved with the requester on 2026-10-22:
- Directory name: `primalucelib` (the typed `plimlucelib` was a typo).
- SDK scope: JSON protocol, all devices (focuser, ARCO, GIOTTO, ALTO).
- Test mode: non-interactive simulator run; commit locally, do not push.
