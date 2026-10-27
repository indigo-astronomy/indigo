# Task brief

- Task ID: 20261027210510-7dc4-aux-rts-stable
- Owner: Peter Polakovic
- Created: 2026-10-27

## Problem and desired outcome
The owner decided (chat, 2026-10-27) to treat `aux_rts` as tested: its README records an FTDI serial loopback run, which the previous task [20261027205926-c759-makefile-driver-lists](../../20261027205926-c759-makefile-driver-lists/brief.md) did not count as hardware. Move `aux_rts` from `UNTESTED_DRIVERS` to `STABLE_DRIVERS` in the root Makefile.

## Users and use cases
People building INDIGO; the list is only written to the generated Makefile.inc.

## Scope and non-goals
Root Makefile only. `polaralign_mlastro` stays untested.

## Constraints and references
The owner's postponement of the overdue full repository audit, given for the previous task in this session, covers this follow-up.

## Initial questions
None.
