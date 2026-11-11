# Development plan

Status: complete (delivery pending owner approval)

## Approach and affected components
The SH20 profile is fixed to V1.3 in the `.driver` source, and the outputs are regenerated through `Makefile.drv`. The host simulator `touptek_dispatch` is made to answer as V1.3 describes. The SH20 cases are updated so that they fail against the old driver and pass against the fixed one. `PROTOCOL.md` (SH20 section and sources) and `indigo_docs/PROPERTIES.md` are updated.

Affected paths:
- `indigo_drivers/mount_lx200/indigo_mount_lx200.driver` and the regenerated `indigo_mount_lx200.c`; the `.h` and `_main.c` are unchanged.
- `indigo_drivers/mount_lx200/mount_lx200_simulator/mount_lx200_simulator.c`
- `indigo_test/integration/test_mount_lx200_simulator.c`
- `indigo_drivers/mount_lx200/PROTOCOL.md`
- `indigo_docs/PROPERTIES.md`

## Baseline
- **Commands and environment:**
  - `make -C indigo_drivers/mount_lx200 -f ../../Makefile.drv all`
  - `make -C indigo_test build/integration/test_mount_lx200_simulator`
  - `./build/integration/test_mount_lx200_simulator touptek` in `indigo_test`
  - Environment: Ubuntu 24.04 x86_64, gcc 13.3.0.
- **Existing failures:** none. 13/13 SH20 cases pass (verification.md).
- **Repository revision / working-tree state:** master `da414d9fd`, clean apart from the owner's untracked files.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Baseline build and the SH20 cases on the original driver | — | None | 13/13 pass | done: 13/13 pass |
| 2 | Simulator follows V1.3 (simulator source) | AC-001–AC-008 | 1 | compiles | done |
| 3 | SH20 cases expect V1.3 (test source) | AC-001–AC-008 | 2 | against the old driver source (HEAD `.c` compiled into the test) the new cases fail for the defects | done: 6 cases fail at the expected checks (D1, D2, D3/D4, D5, D7, D8), see verification.md |
| 4 | Driver fixes D1–D9 and version 84 (`.driver`, regenerated `.c`) | AC-001–AC-008, AC-011 | 3 | builds; SH20 selection passes | done: 13/13 pass |
| 5 | PROTOCOL.md and PROPERTIES.md | AC-009 | 4 | review | done |
| 6 | Full lx200 simulator suite (other profiles) | AC-010 | 4 | all pass | see verification.md |
| 7 | Independent quality review, findings check, archive, process record | all | 6 | quality-review.md, process check | see handoff.md |

## Test strategy
The hardware-free simulator suite `test_mount_lx200_simulator` is used:
- The reproducers come first: the new expectations are run against the original driver, compiled from HEAD into the test binary.
- Then the fixed driver is run with the SH20 selection and the full suite.
- No hardware run is planned or claimed.

## Security risks and mitigations
None new. Replies are compared by their first character. The unpark confirmation loop is bounded to 10 reads.

## Dependencies and architecture decisions
No new dependency or ADR.

## Migration, rollout, and rollback
- **Client-visible changes:**
  - `X_TOUPTEK_MERIDIAN` items are renamed: `AUTO_FLIP_AT_LIMIT` becomes `AUTO_FLIP` and `TRACK_PASSED_MERIDIAN` becomes `RESUME_TRACKING`.
  - `X_TOUPTEK_MERIDIAN_LIMIT` is removed.
  - `MOUNT_HOME` loses its `SEARCH` item.
  - The SH20 profile is unreleased.
- **Rollback:** revert the product commit.

## Deviations and owner decisions
- **Abort handler.** The `MOUNT_HOME_PROPERTY->count == 3` branch was the SH20 home-search path only. It became `count >= 2`, so an interrupted home of any two-item profile publishes `AWAY` immediately instead of an empty selection until the next poll. This is consistent with the poll code; the full suite checks it.
- **Search logic.** The home-search request path (`home_search_allowed`) was removed with `:MFH#`. It became dead code once the only profile that used it no longer offers the search.
- **README.** `indigo_drivers/mount_lx200/README.md` still mentions the "Search" item. Changing it needs owner approval, and it was proposed to the owner.
