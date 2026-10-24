# Specification
Status: final

## Problem and outcome
Token lookups in primalucelib never index the token array beyond the tokens the parser filled.

## In scope
`indigo_drivers/shared/primalucelib/primalucelib.[ch]`, `indigo_test/unit/test_primalucelib.c`, PROTOCOL.md; version bumps
of the three dependent drivers.

## Out of scope
Token and response capacities; driver behavior.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | With a reply of exactly `PRIMALUCE_MAX_TOKENS` tokens, a lookup of an absent member after the last subtree and a lookup below a non-object value return false without reading past the array, and a present member is still found. | `lookups_stay_within_a_full_token_array` under ASan/UBSan (overflow before the fix) |
| AC-002 | Replies below the capacity are looked up as before. | the rest of the unit suite and the simulator suites of the three drivers |

## Failure behavior and edge cases
A failed parse leaves the token count 0, so every lookup on it fails.

## Permissions, data, and security requirements
Controller replies are untrusted input; the bound applies whatever their content.

## Compatibility and nonfunctional requirements
`indigo_focuser_primaluce` 40 to 41, `indigo_aux_giotto` 2 to 3, `indigo_aux_alto` 2 to 3 (they link the library statically).

## Assumptions and open questions
None.

## Owner decisions and requirement changes
2026-10-24: fix it, keeping the count in the per-connection state.
