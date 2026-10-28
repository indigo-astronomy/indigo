# Security review

Verdict: pass
Reviewer: Claude Fable 5.1 (security-reviewer role, same agent as the developer)
Date: 2026-10-28
Revision and working-tree state: task branch `task/20261027214032-3577-solver-index-0-5deg` from master `1e6a6e01f`, uncommitted working tree reviewed before the single task commit; reviewed content identities in process.json
Review independence: none; one agent in one session performed every role. Recorded as a limitation.

## Scope, assets, and trust boundaries
The task adds a catalogue reader to a developer tool (`indigo_solver_indexer -g <directory>`), a scale and a
catalogue name to the solver, item naming to the agent, and a new downloadable index asset. The agent downloads
index files from the fixed release URL as before (unchanged `download_index_file()` and header validation); the
new asset is one more file name. Assets: the owner's machine running the indexer on local CSV files, the user's
`indigo_server` loading a 988 MB index, the Gaia data licence.

## Risk coverage
| Area | Applicable? | Review evidence / reason not applicable |
| --- | --- | --- |
| Authentication and authorization | No | No new property or access path |
| Input, injection, and output handling | Yes | `parse_gaia_catalog()` builds the file list with `popen("ls \"<directory>\"/*.csv")`: the directory is a command line argument of a developer tool run by the owner, not data from the bus or a user; a directory name containing a double quote or shell metacharacters would reach the shell, which is acceptable for a local tool but is noted as a limitation in the README-level usage (the same tool already trusts its working directory for Tycho-2). CSV fields are parsed with `strtod` with bounds on the star count (`MAX_GAIA_STARS`), malformed lines are skipped. The agent's item name parser `index_item_fov()` accepts only names whose scale matches the known list. |
| Data, secrets, cryptography, and logs | No new | Gaia DR3 data is public under CC BY-SA 3.0 IGO; the README names the source and licence |
| Filesystem and outbound network access | No change in the agent | The extract download was done with a session-local script against the ESA TAP service, outside the repository; the agent itself downloads only from the fixed INDIGO release URL as before |
| Dependencies and configuration | No change | |
| Abuse and failure behavior | Yes | A missing or truncated index file fails the agent's header validation as before; a 907 MB index is loaded into memory on first use, like the 534 MB 1° index (documented size in the item label) |

## Automated checks
`./.factory/scripts/security` (`validate_project.py --security`): see verification.md.

## Diff risk signal
The baseline risk policy flags `MIGRATION_STATUS.md` as a data-model path by its name; the change is one table cell
(automated test count 149 to 150), documentation only, no risk.

## Findings
None blocking. Observation: the `popen`/`ls` file listing in the indexer is a local developer-tool convenience;
a `glob()` based listing would avoid the shell. Not a security boundary (owner-run tool with owner-chosen
arguments); recorded in quality-review.md as a non-blocking follow-up.

## Accepted risks
None recorded.

## Verdict and limitations
Pass for the reviewed diff; same-agent review; the asset upload to the GitHub release was refused by the host's
permission policy in this session and is the owner's step.

## Register reconciliation
No canonical FND record created; the active register holds no Solver Agent finding.
