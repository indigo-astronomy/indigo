# Task brief

- Task ID: 20261113005229-eff7-static-server-gphoto2-include
- Owner: Rumen G. Bogdanovski
- Created: 2026-11-13

## Problem and desired outcome
Owner request 2026-11-13: "fix 005 in master" — FND-20261112184305-e0a6-scheduler-agent-core-005 (recorded on branch `scheduler`): `indigo_server.c` includes `ccd_gphoto2/indigo_ccd_gphoto2.h` for static Linux builds, but the driver lives in `obsoleted_drivers/`, so a `STATIC_DRIVERS` build cannot compile.

## Users and use cases
Builders of a static `indigo_server`.

## Scope and non-goals
In scope: the static driver list in `indigo_server/indigo_server.c` on master. Non-goals: reviving the gphoto2 driver.

## Constraints and references
Fix on master through a task branch; the finding record lives on branch `scheduler` and is closed there after master is merged into it.

## Initial questions
None.
