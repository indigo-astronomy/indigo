# Security review

Verdict: pass
Reviewer: coordinator (Claude Code, claude-opus-5-5), with the independent diff review covering input handling and memory safety
Date: 2026-11-12
Revision and working-tree state: branch `scheduler` at cbf8e03bf with this task's uncommitted changes
Review independence: partly independent (diff review by a fresh subagent).

## Scope, assets, and trust boundaries
Task text and commands come from INDIGO clients over the bus and are untrusted. The task body is executed by the Scripting Agent with the same trust as `AGENT_SCRIPTING_RUN_SCRIPT`.

## Risk coverage
| Area | Applicable? | Review evidence / reason not applicable |
| --- | --- | --- |
| Authentication and authorization | Yes | No new access path: the agent uses the existing bus ACLs; anyone who may write its properties could already run scripts through the Scripting Agent. |
| Input, injection, and output handling | Yes | Header parser bounded (16 KiB text, 255-character header lines, 40-byte names, control characters rejected, decimal-only numbers); the task name and text enter the wrapper only as JSON string literals (`json_string` escapes quotes, backslashes, control characters, U+2028/2029); `%s` arguments never used as formats; test `quoted_name_reaches_the_body`. |
| Data, secrets, cryptography, and logs | No | No secrets handled. |
| Filesystem and outbound network access | Yes | Only the agent's own INDIGO configuration file through `indigo_save_property()`; no network access. |
| Dependencies and configuration | No | No new dependencies. |
| Data model and migrations | Yes | The process policy flags `MIGRATION_STATUS.md` by name. The change adds one documentation row for `agent_scheduler` (status columns only, Comment column empty); no data schema, stored format or migration is involved. The agent's own configuration format is new and documented in SCHEDULER.md "Persistence". |
| Abuse and failure behavior | Yes | Memory safety checked with ASan/UBSan; use-after-free between client and handler threads fixed; bounded timeouts for start and stop; 128-task limit. |

## Automated checks
ASan/UBSan integration and unit runs, see verification.md.

## Findings
None security-specific. Related functional finding FND-20261112184305-e0a6-scheduler-agent-core-002 (library reload).

## Accepted risks
None recorded.

## Verdict and limitations
Pass. Running arbitrary JavaScript from uploaded tasks is by design, at the Scripting Agent's trust level, and is documented in SCHEDULER.md.

## Register reconciliation
No security findings created.
