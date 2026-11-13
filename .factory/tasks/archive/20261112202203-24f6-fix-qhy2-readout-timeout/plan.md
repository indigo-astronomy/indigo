# Development plan

Status: complete

## Approach and affected components
- `indigo_drivers/ccd_qhy2/indigo_ccd_qhy2.driver`: compute the single-frame timeout from the exposure and bump the version. Regenerate `indigo_ccd_qhy2.cpp` through `Makefile.drv`.
- `indigo_test/integration/test_ccd_qhy_sdk.cpp`:
  - the fake SDK records the timeout and an optional readout time;
  - a frame that exceeds the timeout returns success with a buffer of zeros;
  - a new `slow readout timeout` case (QHY2 build only).

## Baseline
- Commands and environment: macOS 27.0 arm64, Apple clang; `make -C indigo_test build/integration/test_ccd_qhy2_sdk build/integration/test_ccd_qhy_sdk`.
- Existing failures: none known in the qhy fake-SDK suites.
- Repository revision / working-tree state: master `e0280e650`, clean.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Fake-SDK frame timeout model and regression case (`indigo_test/integration/test_ccd_qhy_sdk.cpp`) | AC-002, AC-003 | None | Case fails against the unfixed driver | done |
| 2 | Timeout from exposure + 10 s, version 47 (`indigo_drivers/ccd_qhy2/indigo_ccd_qhy2.driver`, regenerated `.cpp`) | AC-001, AC-005 | 1 | Driver builds, case passes | done |
| 3 | Full qhy2 and legacy qhy fake-SDK suites | AC-004 | 2 | Both pass | done |

## Test strategy
Fake-SDK integration only. The narrow driver targets are used, without the whole suite. No hardware is available. The reporter is asked to test a build.

## Security risks and mitigations
None. The timeout stays bounded, so a hung camera cannot block the queue indefinitely.

## Dependencies and architecture decisions
No new dependency, no ADR.

## Migration, rollout, and rollback
Not applicable; revert the commit to roll back.

## Deviations and owner decisions
None.
