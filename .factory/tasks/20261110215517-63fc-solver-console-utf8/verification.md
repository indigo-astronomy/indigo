# Verification

Working directory: repository root, task branch based on d617e4f4b, uncommitted `indigo_tools/indigo_solver.c`.

| Check | Result |
| --- | --- |
| `msbuild indigo_windows.sln /t:indigo_solver /p:Configuration=Release /p:Platform=x64` | pass, exit 0 |
| Before the fix: `chcp 850`, `indigo_solver --list` in a hidden console, read back with `ReadConsoleOutputCharacterW` | defect reproduced, `0.2┬░` |
| After the fix: same procedure | pass, `0.2°` |
| `chcp` in the same console after the tool exits | pass, still 850 |

## Not run
- Native test suite, including `indigo_test/integration/test_indigo_solver_tool.c`: no make toolchain on this host. The change is inside `#if defined(INDIGO_WINDOWS)`.
- Restoration of the code page when a solve is killed by Ctrl+C: the `atexit` handler doesn't run, the console then stays on UTF-8.
- `.factory/scripts` process check, findings check and archive-task: Python is not available on this host.
