# Development plan

Status: final

## Approach and affected components
`findings sync-project` for the findings groups; a scratchpad script (not committed) for the archived task folders and
completion records, following the earlier repair script without `plutil`.

## Baseline
- Commands and environment: `./.factory/scripts/findings check`, Linux x86_64.
- Existing failures: "Xcode findings references are stale".
- Repository revision / working-tree state: master `ff98d482b`.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | `findings sync-project` | AC-001 | None | findings check | done |
| 2 | five task folders and completions | AC-002, AC-003 | None | grep checks | done |
| 3 | Windows project check | AC-004 | None | XML parse | done |
| 4 | this task's own references after archival | AC-002 | 1–3 | grep checks | see verification.md |

## Test strategy
Structural checks only; no build is affected.
