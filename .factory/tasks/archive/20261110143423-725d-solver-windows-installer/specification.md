# Specification

Status: approved by the owner request of 2026-11-10

## Problem and outcome
A stand-alone "INDIGO Solver" installer next to the INDIGO installer.

## In scope
See brief.md.

## Out of scope
Index files (downloaded with `indigo_solver --download`), the MSVC runtime (not shipped by the INDIGO installer either).

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | `indigo_solver.iss` installs indigo_solver.exe with pthreadVC3.dll, zlib.dll and libraw.dll into `{app}\bin`, as its own application (own AppId, name, folder, output file) with the optional PATH task | static review |
| AC-002 | The INDIGO installer behaves as before; its PATH code is shared unchanged | diff of the moved code |
| AC-003 | `tools/build_windows_installer.sh` builds both installers | `bash -n`, review |
| AC-004 | New files are in the Xcode project | pbxproj diff, balance check |

## Failure behavior and edge cases
A missing DLL fails the installer build (no `skipifsourcedoesntexist` for the solver's required DLLs).

## Permissions, data, and security requirements
Same PATH handling as the INDIGO installer.

## Compatibility and nonfunctional requirements
x64 only, as the INDIGO installer.

## Assumptions and open questions
Not compiled with ISCC here.

## Owner decisions and requirement changes
2026-11-10: separate installer instead of components.
