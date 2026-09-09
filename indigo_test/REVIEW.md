# indigo_test Review

## Status

| Field | Value |
| --- | --- |
| Last reviewed commit | `017ba602857378e4aed489c065c76eacae15924c` |
| Review state | Initial baseline retained; scoped test-harness findings recorded and closed. |

## Scope

Automated test harness, unit tests, integration tests, fixtures, and test documentation under `indigo_test/`.

## Current Findings

| ID | Severity | File | Summary | Status |
| --- | --- | --- | --- | --- |
| TEST-001 | Medium | `integration/simulator_test_common.h:simulator_client_update_property` | The update counter counted only CONNECTION, so no-update-after-disconnect assertions missed motion and polling updates. | Closed (fixed) |
| TEST-002 | Medium | `integration/simulator_test_common.h:enumerate_simulator_device` | The enumeration selector had TEXT_VECTOR type and excluded switch/number properties. | Closed (fixed) |

## Finding Summaries

### TEST-001 (Closed — fixed)

The update counter counted only CONNECTION, so no-update-after-disconnect assertions missed motion and polling updates. The counter now includes every update from the selected device and is updated atomically.

Validation: affected simulator/fake transport tests rerun successfully.

### TEST-002 (Closed — fixed)

The enumeration selector had TEXT_VECTOR type and excluded switch/number properties. Initial attach broadcasts hid this error until switching the cache to a sibling device. The selector now uses wildcard type 0.

Validation: AO/guider shared connection orders and sibling survival pass.

## Review Focus

- Determinism and hardware-free execution.
- Public API coverage rather than inclusion of production `.c` files.
- Bounded waits for asynchronous simulator behavior.
- Cleanup of generated `build/` artifacts through `make -C indigo_test test-clean`.
- Alignment with `indigo_test/AGENTS.md` and `indigo_test/DRIVER_TESTING_RULES.md`.
- Updates to `CHANGES.md` for meaningful test coverage changes.

## Reviewed Ranges

| From | To | Date | Notes |
| --- | --- | --- | --- |
| Repository start | `017ba602857378e4aed489c065c76eacae15924c` | 2026-08-01 | Initial review baseline only. |
| `84298256404b3aee1028d29ce152233ffd8afe2e` | working tree | 2026-09-09 | Focused shared simulator harness review; recorded and closed `TEST-001` and `TEST-002`. Verified all-property update counting and wildcard enumeration through affected simulator/fake transport tests, including AO/guider shared connection orders and sibling survival. This does not mark the entire test folder reviewed or advance its baseline. |
