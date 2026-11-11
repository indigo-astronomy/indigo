# Specification

Status: retrospective. Written on 2026-11-11 at closure from brief.md, verification.md and product commit 34c8ab3f2;
the original Windows session recorded the requirements only in brief.md.

## Behaviour
On the Windows console `indigo_solver` switches the console output code page to UTF-8 so the degree sign displays correctly, and restores the previous code page at exit. Files and pipes are unchanged.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | Degree sign displays correctly on a code page 850 console | verification.md |
| AC-002 | Console code page is restored after exit | verification.md |
| AC-003 | Shared tool behaviour unchanged (native test suite) | verification.md |

Results and the criteria that were not run are in [verification.md](verification.md).
