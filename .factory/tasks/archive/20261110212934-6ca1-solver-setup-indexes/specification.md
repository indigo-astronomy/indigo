# Specification

Status: retrospective. Written on 2026-11-11 at closure from brief.md, verification.md and product commit a4f968b30;
the original Windows session recorded the requirements only in brief.md.

## Behaviour
The INDIGO Solver setup offers a page to choose solver indexes, downloads the selected ones with `indigo_solver` after installation, and states that indexes can be installed later with the tool. `indigo.iss` and `path.iss` stay unchanged.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | Setup compiles with Inno Setup 6 | verification.md |
| AC-002 | Only `indigo_solver.iss` changes | verification.md |
| AC-003 | Index choices match the solver index table | verification.md |
| AC-004 | Selected indexes are downloaded; failure is reported (interactive) | verification.md |

Results and the criteria that were not run are in [verification.md](verification.md).
