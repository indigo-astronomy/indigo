# Verification

Verdict: pass
The executed checks passed; the checks under "Not run" remain open gaps, accepted by the owner at closure (see Closure).

Run in a throwaway detached worktree of master da50114b3 with the modified script (LF, as committed) copied in, and a planted `build/Release/x64/indigo_stale_leftover.dll`.

| Check | Result |
| --- | --- |
| `bash -n tools/build_windows_installer.sh` | pass |
| `bash tools/build_windows_installer.sh` | pass, exit 0 |
| MSBuild Clean, then Build of `indigo_windows.sln` (Release, x64) | pass, 0 errors, 1927 compiler warnings |
| Planted leftover after the run | removed |
| Vendor DLLs and installer inputs after deleting the output folder | pass, both ISCC compiles succeeded |
| Setups in `dist/` | indigo-windows-3.0-10-x64-setup.exe, indigo-solver-windows-3.0-10-x64-setup.exe |

## Not run
- Installing the setups from this run.
- ARM64 and Debug configurations.
- `.factory/scripts` process check, findings check and archive-task: Python is not available on this host.

## Closure
Closure on Linux (2026-11-11): nothing further can be run on this host; installing the setups and the ARM64/Debug configurations remain not run.
`./.factory/scripts/findings check`: pass (25 active, 292 archived). The task was closed and archived on the owner's request ("finish them and archive", 2026-11-11), with the not-run items above accepted as remaining gaps; its product commits were already on master.
