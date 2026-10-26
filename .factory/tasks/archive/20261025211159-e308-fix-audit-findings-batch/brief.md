# Task brief

- Task ID: 20261025211159-e308-fix-audit-findings-batch
- Owner: Peter Polakovic
- Created: 2026-10-25

## Problem and desired outcome
The owner asked to repair 31 open findings from the repository audits (owner request: "oprav tento zoznam chyb"). Each finding is repaired in source, covered by a regression where a hardware-free harness exists, and closed in the central findings registry with evidence.

Findings in scope:
- 3bd2-repository-audit: 015, 016, 018, 020, 023, 032
- bc8d-complete-repository-audit: 003, 004, 007, 008, 009, 010, 011, 013, 016, 017, 018, 023, 024, 025, 028, 029, 035, 036, 037, 038, 040, 044, 045
- 60d4-repository-audit: 009
- FND-000048

## Users and use cases
INDIGO users and integrators affected by the listed driver, library, server and web UI defects.

## Scope and non-goals
Scope: the source locations named by each finding record, their tests and required documentation/version updates. Non-goals: other open findings, off-limits drivers (ccd_apogee, ccd_sbig, mount_asi, mount_mxhd, system_ascol), generator changes, hardware runs, push or publication.

## Constraints and references
Root AGENTS.md, indigo_drivers/AGENTS.md, indigo_test/AGENTS.md, indigo_libs/AGENTS.md; finding records in .factory/findings/active/. The full repository audit is due (maintenance status); this task is audit follow-up repair of findings, recorded as such, and does not reset the audit checkpoint. No push authorized.

## Initial questions
None blocking. Findings requiring hardware-only verification are recorded as source/simulator-verified with the hardware gap stated.
