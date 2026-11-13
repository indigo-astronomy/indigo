# Development plan

Status: complete

## Approach and affected components
Remove the gphoto2 include and static entry from `indigo_server/indigo_server.c`.

## Baseline
- master at 0aba316b8; `gcc -fsyntax-only -DSTATIC_DRIVERS` fails: `ccd_gphoto2/indigo_ccd_gphoto2.h: No such file or directory`.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Remove the two `#ifdef INDIGO_LINUX` blocks | AC-001, AC-002 | None | syntax checks | done |

## Test strategy
Syntax checks of both configurations; no runtime behaviour changes for dynamic builds.

## Security risks and mitigations
None.

## Dependencies and architecture decisions
None.

## Migration, rollout, and rollback
Not applicable.

## Deviations and owner decisions
None.
