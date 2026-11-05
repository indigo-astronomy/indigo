# Task brief

- Task ID: 20261105225444-90d8-factory-git-243-compat
- Owner: Rumen G. Bogdanovski (requester)
- Created: 2026-11-05

## Problem and desired outcome
On this host (Linux, git 2.43.0), all Factory helpers that call Git fail with "Git operation failed" or "task ID
history could not be checked". The cause is the global option `--no-lazy-fetch`, which needs git 2.44 or later.
`new-task`, `findings check`, `maintenance status`, `process-check`, `archive-task` and the template and metadata
updaters are affected. The owner asked for a permanent fix so that the helpers work with git 2.43 and stay that way.

## Users and use cases
Agents and maintainers running the Factory workflow on hosts with git older than 2.44 (Ubuntu 24.04 ships 2.43).

## Scope and non-goals
In scope: `.factory/scripts/` Git invocations, their tests in `indigo_test/factory/`, and the routine text that
states the Git requirement. Non-goals: product code, changing the safety contract (no lazy fetch from a helper),
and upstream AI Factory template changes (owner follow-up).

## Constraints and references
`.factory/routines/template-updates.md` (Git requirement), `AGENTS.md` (evidence, never weaken tests),
`.factory/template-lock.json` (the scripts are template-mapped, so this is a recorded local divergence).

## Initial questions
None.
