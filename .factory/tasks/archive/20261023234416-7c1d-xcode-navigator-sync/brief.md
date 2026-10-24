# Task brief

- Task ID: 20261023234416-7c1d-xcode-navigator-sync
- Owner: Rumen G. Bogdanovski (requester)
- Created: 2026-10-24

## Problem and desired outcome
The requester asked to fix the Windows and Xcode project issues flagged during the PrimaLuce work. `findings check`
failed with "Xcode findings references are stale". The five PrimaLuce tasks from 2026-10-23 and 2026-10-24 had not
registered their archived task folders, completion records or archived finding in `indigo.xcodeproj`, as the
repository rule for new persistent files requires. The outcome is a complete Xcode navigator and confirmed Windows
projects.

## Users and use cases
Maintainers who browse the repository in Xcode, and the Factory findings gate.

## Scope and non-goals
In scope:
- the Xcode references of the archived finding, the five task folders and their completion records;
- this task's own records;
- a check of the PrimaLuce Windows projects.

Non-goals:
- Windows and Xcode builds, which cannot run on this Linux host.

## Constraints and references
- Root AGENTS.md (Repository Hygiene).
- `.factory/routines/findings.md`.
- `.factory/tasks/archive/20261023195506-cdf8-fix-findings-xcode/repair-project.py` (needs macOS `plutil`).

## Initial questions
Resolved: the requester asked for the fix (2026-10-24).
