# Task brief

- Task ID: 20261024164718-0e69-primalucelib-token-bounds
- Owner: Rumen G. Bogdanovski (requester)
- Created: 2026-10-24

## Problem and desired outcome
FND-20261024121920-3bd2-repository-audit-017: `primalucelib_find_token()` walks the jsmn token array until it meets an
unused token. A reply that fills all `PRIMALUCE_MAX_TOKENS` leaves no unused token, so a lookup of an absent field after the
last subtree reads past the array. The requester asked to fix it (2026-10-24), and asked whether the library has per-device
state for it: it has, `primalucelib_link` holds the response buffer and the tokens of each connection, and now the count too.

## Users and use cases
Every driver built on primalucelib: `indigo_focuser_primaluce`, `indigo_aux_giotto`, `indigo_aux_alto`.

## Scope and non-goals
In scope: the parse step and the token lookup in primalucelib, a unit regression case, PROTOCOL.md, the version of each
dependent driver. Non-goals: changing the token or response capacities.

## Constraints and references
Root, driver and test AGENTS.md; the audit finding and `indigo_drivers/shared/primalucelib/PROTOCOL.md`.

## Initial questions
None.
