# Development plan

Status: complete

## Approach and affected components
New `indigo_solver.iss` modelled on `indigo.iss`; `[Code]` moved verbatim into `path.iss` and `#include`d by both;
build script runs ISCC on both; Xcode group `indigo_windows/installer` with the three scripts.

## Baseline
- Commands and environment: Linux aarch64, no Inno Setup.
- Existing failures: not applicable.
- Repository revision / working-tree state: master e72a734cb, untracked owner folders only.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | `indigo_windows/installer/path.iss`, `indigo.iss` | AC-002 | None | moved code diff empty | done |
| 2 | `indigo_windows/installer/indigo_solver.iss` | AC-001 | 1 | review | done |
| 3 | `tools/build_windows_installer.sh` | AC-003 | 2 | `bash -n` | done |
| 4 | `indigo.xcodeproj/project.pbxproj` | AC-004 | 2 | balance check | done |

## Test strategy
Static checks only; ISCC build and installation on Windows by the owner.

## Security risks and mitigations
None new.

## Dependencies and architecture decisions
New AppId CB57093D-4D51-4A37-A7E3-25DDAEF0411E, so both applications install and uninstall independently.

## Migration, rollout, and rollback
Revert the commit.

## Deviations and owner decisions
None.
