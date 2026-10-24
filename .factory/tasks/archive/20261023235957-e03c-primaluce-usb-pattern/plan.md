# Development plan

Status: final

## Approach and affected components
Add `vid`/`pid` to the existing generator `pattern` block, regenerate, and test the registered pattern with the library matcher.

## Baseline
- Commands and environment: `python3 tools/run_driver_test.py focuser_primaluce`, Linux x86_64.
- Existing failures: none (71/71 at 3.0.0.39).
- Repository revision / working-tree state: master `c7513fd72`.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | `.driver` pattern, regenerated `.c` | AC-001 | None | build | done |
| 2 | `usb_match_pattern` | AC-001 | 1 | fails without step 1 | done |
| 3 | Documentation | – | 1 | review | done |
| 4 | Recorded run | AC-002 | 1–3 | 72/72 | see verification.md |

## Test strategy
A simulator-suite case that reads the attached device's registered pattern.
