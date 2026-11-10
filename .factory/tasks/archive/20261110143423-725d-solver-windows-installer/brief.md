# Task brief

- Task ID: 20261110143423-725d-solver-windows-installer
- Owner: Rumen G. Bogdanovski
- Created: 2026-11-10

## Problem and desired outcome
Owner request (2026-11-10): a separate Windows installer for `indigo_solver` (instead of optional components in the
INDIGO installer), now that `indigo_solver.exe` needs no INDIGO DLL.

## Users and use cases
Windows users who want only the command line plate solver.

## Scope and non-goals
- In scope: `indigo_windows/installer/indigo_solver.iss`, the shared PATH code in `indigo_windows/installer/path.iss`
  included by both scripts, `tools/build_windows_installer.sh` building both, Xcode project entries.
- Non-goals: changing what the INDIGO installer installs (it still contains indigo_solver.exe), bundling indexes.

## Constraints and references
No Inno Setup or Windows on this host.

## Initial questions
None.
