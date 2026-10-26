# Development plan

The 31 findings are split into six independent groups by touched files. Each group runs in its own worktree and branch (`task/20261025211159-e308-fix-audit-findings-batch-gN`, worktree `../igw-N`), with one implementer subagent as the single writer of that worktree. The coordinator merges the group branches into the task branch, resolves shared-file conflicts (indigo_test/Makefile, indigo.xcodeproj, driver versions touched by two groups), reruns the affected checks on the merged candidate and closes the findings.

| Group | Area | Findings |
| --- | --- | --- |
| G1 | core library, test runner | bc8d-003, bc8d-011, bc8d-013, bc8d-016, bc8d-029, bc8d-035, bc8d-036, 3bd2-015 |
| G2 | image processing and recording | bc8d-004, bc8d-007, bc8d-008, bc8d-009, bc8d-010 |
| G3 | server, HTTP, browser UI | bc8d-037, bc8d-038, bc8d-040, 60d4-009 |
| G4 | single-driver defects | 3bd2-016, 3bd2-018, 3bd2-020, 3bd2-032, bc8d-025, bc8d-028, bc8d-044, FND-000048 |
| G5 | motion completion deadlines | bc8d-024 |
| G6 | wheel counts, aux finalizers, firmware simulators | bc8d-017, bc8d-018, bc8d-023, bc8d-045, 3bd2-023 |

Each fix gets a hardware-free regression where a harness exists; Arduino firmware simulators are verified by inspection. Driver versions are bumped. No README, generator, push or hardware run.
