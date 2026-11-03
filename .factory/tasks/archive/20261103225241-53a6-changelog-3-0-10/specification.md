# Specification

AC-001 A `# [3.0-10] - <date>` block is inserted above the 3.0-9 block with `## Overall:`, `## New drivers:` and `## Driver changes:`, in the existing file format (tab-indented items, one blank line between components, two before the older block); older blocks unchanged.

AC-002 Scope: all non-merge commits `e5787a7e2..HEAD` by all authors; the block describes the net user-visible change; tests, test infrastructure, simulators for tests, test records, development and Factory notes, version bumps, regenerated output without behaviour change, formatting and build housekeeping are left out.

AC-003 New drivers are the driver directories absent at `e5787a7e2` (aux_alto, aux_giotto, aux_simulator, ccd_libcamera) and, by owner decision (2026-11-04), agent_platesolver, which replaces the 3.0-9 agent_solver skeleton whose solve always failed.

AC-004 Line style: lower case, no trailing period, fixes start with "fix", additions end with "added", issue numbers appended, one bullet per driver, changes made the same way in many drivers are one Overall line naming the drivers; every line traceable to commits (traceability in `commit-inventory.txt` and verification.md).
