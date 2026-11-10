# Specification

Status: approved by the owner decisions of 2026-11-10 (brief.md)

## Problem and outcome
Originally: `indigo_solver` links libindigo_client on every platform. Delivered after the owner decision of
2026-11-10: the Windows client library project is repaired and built by the solution; the solver build is unchanged
and moves to a separate task.

## In scope
- `indigo_tools/Makefile`: client library, tool-local `indigo_timer`, `indigo_align`, `indigo_json`, `indigo_dslr_raw`,
  static libraw and libjpeg.
- Windows: `indigo_windows/indigo_client/indigo_client.vcxproj` (+ filters) repaired and added to `indigo_windows.sln`;
  `indigo_tools/indigo_solver.vcxproj` linked to it; the installer ships `indigo_client.dll` next to the exe files.

## Out of scope
Other tools and projects; client library source changes.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 (withdrawn) | The Linux `indigo_solver` binary does not depend on libindigo, libusb or libudev and depends on libindigo_client | `ldd` |
| AC-002 (withdrawn) | The tool behaves as before: solving of INDIGO RAW, FITS, JPEG and camera raw files, index management | tool suite, real files |
| AC-003 | The Windows client library project builds the Linux client source set, exports its API, uses existing paths and is part of the solution | static review of the project files (no Windows build available) |
| AC-004 | The installer ships indigo_client.dll next to the exe files instead of the drivers folder (the tool project part withdrawn) | static review |

## Failure behavior and edge cases
Not applicable beyond build and link failures.

## Permissions, data, and security requirements
No change of behaviour, input handling or network access.

## Compatibility and nonfunctional requirements
macOS uses the existing static client library branch of the Makefile; unverified here.

## Assumptions and open questions
MSVC accepts the per-file export setting and the locally defined imported functions (C4273/LNK4217 warnings expected
for indigo_json.c and the callers of the tool-local modules); to be confirmed by a Windows build.

## Owner decisions and requirement changes
2026-11-10: build the extra modules into the tool; switch Windows too, repairing the client library project.
2026-11-10 (later): deliver the Windows client library repair alone; AC-001, AC-002 and the tool part of AC-004
withdrawn, the solver build becomes a separate self-contained task.
