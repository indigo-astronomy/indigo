# Verification

Verdict: pass
The executed checks passed; the checks under "Not run" remain open gaps, accepted by the owner at closure (see Closure).

Working directory: repository root, task branch, uncommitted `indigo_windows/installer/indigo_solver.iss`.

| Check | Result |
| --- | --- |
| `ISCC.exe /Q /O<scratch> indigo_windows\installer\indigo_solver.iss` (Inno Setup 6) | pass, exit 0, setup executable produced |
| Only `indigo_solver.iss` modified (`git status --short`) | pass |
| Index table compared with `indigo_solver_index_fovs`, `index_fields`, `indigo_solver_index_sizes` | pass, by reading |

## Not run
- Interactive setup: page layout, download of a selected index, reinstall, offline failure warning.
- `.factory/scripts` process check, findings check and archive-task: Python is not available on this host.

## Owner test of the first build
The first test setup packaged a stale `build\Release\x64\indigo_solver.exe` (2026-11-09, before the self-contained change), which imports from `indigo.dll`. It failed in the owner's run with "entry point indigo_validate_raw_image not found", against an older installed `indigo.dll` on PATH. The wizard page and the launch of the tool worked.

Rebuilt with `msbuild indigo_windows.sln /t:indigo_solver /p:Configuration=Release /p:Platform=x64` (exit 0). `dumpbin /dependents` shows no `indigo.dll`; `indigo_solver --list` with PATH limited to `C:\Windows\System32` exits 0. Repackaged with `INDIGO_VERSION=3.0 INDIGO_BUILD=10`, ISCC exit 0. Owner retest pending.

Observation: the executable imports `VCRUNTIME140.dll`, which the solver setup neither ships nor installs.

## Closure
Closure on Linux (2026-11-11): the Windows setup itself cannot be exercised on this host; the interactive setup run and the owner retest of the rebuilt setup remain not run. The missing `VCRUNTIME140.dll` dependency noted above stays an open observation for a follow-up task.
`./.factory/scripts/findings check`: pass (25 active, 292 archived). The task was closed and archived on the owner's request ("finish them and archive", 2026-11-11), with the not-run items above accepted as remaining gaps; its product commits were already on master.
