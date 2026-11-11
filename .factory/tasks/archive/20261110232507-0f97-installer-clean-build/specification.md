# Specification

Status: retrospective. Written on 2026-11-11 at closure from brief.md, verification.md and product commit af4de8547;
the original Windows session recorded the requirements only in brief.md.

## Behaviour
`tools/build_windows_installer.sh` always cleans the solution and removes `build/<configuration>/<platform>` before building, except with `SKIP_BUILD=1`.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | Script passes `bash -n` | verification.md |
| AC-002 | A planted stale file in the output folder is removed | verification.md |
| AC-003 | Both setups are produced after a clean build | verification.md |

Results and the criteria that were not run are in [verification.md](verification.md).
