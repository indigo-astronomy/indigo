# Development plan

Status: complete

## Approach and affected components
Makefile link change with tool-local framework objects; Windows client project rewritten after `indigo.vcxproj`
(INDIGO_WINDOWS_DLL exports, `indigo_libs/bin_externals` and `externals/bonjour-sdk` paths, shared output folder,
Linux client source list); solution entry; tool project references; installer runtime DLL list.

## Baseline
- Commands and environment: Linux aarch64, gcc; `ldd build/bin/indigo_solver` listed libindigo.so, libudev, libusb.
- Existing failures: none in the tool suite (32/32 after the previous task).
- Repository revision / working-tree state: master 7c245acd59ec19544e53c698f2f0fd083331d9d8, untracked owner folders only.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | `indigo_tools/Makefile` | AC-001, AC-002 | None | ldd, suites, real files | reverted (owner decision) |
| 2 | `indigo_windows/indigo_client/indigo_client.vcxproj(.filters)`, `indigo_windows.sln` | AC-003 | None | XML/solution checks, review | done |
| 3 | `indigo_tools/indigo_solver.vcxproj` reverted (owner decision); `indigo_windows/installer/indigo.iss` kept | AC-004 | 2 | review | done |

## Test strategy
Existing tool and agent suites, `ldd`, real-file solves, static checks of the Windows project files.

## Security risks and mitigations
None: link configuration only.

## Dependencies and architecture decisions
The tool keeps a ProjectReference to `indigo.vcxproj` without linking it, for build order: that project generates the
libjpeg configuration header the tool's bundled libjpeg sources need and copies the runtime DLLs into the build folder.

## Migration, rollout, and rollback
Revert the commit. Windows packages gain indigo_client.dll in bin.

## Deviations and owner decisions
The Windows client project was found broken (not in the solution, empty .def exports, missing `indigo_windows/externals`
paths, no md5/fits/raw_utils); repaired with owner approval. Afterwards the owner decided to deliver this repair alone and to build the solver from
INDIGO sources in a separate task; the Makefile and tool project changes were reverted to HEAD.
