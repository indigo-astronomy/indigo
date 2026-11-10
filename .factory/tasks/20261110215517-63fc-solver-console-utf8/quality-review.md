# Quality review

Reviewed: uncommitted diff of `indigo_tools/indigo_solver.c`. Not independent: same agent as the implementation.

- Windows code is confined to a command line tool, not a driver, and guarded by `INDIGO_WINDOWS`.
- `GetConsoleOutputCP()` returns 0 without a console, so redirected or detached runs change nothing.
- The console code page is restored at exit, as it outlives the process.

Verdict: no blocking findings.
