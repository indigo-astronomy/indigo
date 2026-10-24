# Task brief

- Task ID: 20261024213010-b356-primaluce-panel-cover-firmware-bound
- Owner: Rumen G. Bogdanovski (requester)
- Created: 2026-10-25

## Problem and desired outcome
Upstream fixed FND-20261024121920-3bd2-repository-audit-029 in `indigo_focuser_primaluce` (6a2a53928): the firmware
versions were appended to `INFO` with unbounded `strcat()`. `indigo_aux_giotto` and `indigo_aux_alto` copied that code.
The requester approved fixing them in a separate follow-up after the merge (2026-10-25).

## Users and use cases
Anyone connecting a panel or cover whose firmware reports long version strings (a faulty or hostile device).

## Scope and non-goals
In scope: the bounded append in both drivers, simulator profiles, regression tests. Non-goals: other INFO items.

## Constraints and references
Root, driver and test AGENTS.md; the focuser fix in 6a2a53928.

## Initial questions
None.
