# Task brief

- Task ID: 20261102110138-ff60-separate-factory-commits
- Owner: Rumen G. Bogdanovski (requester)
- Created: 2026-11-02

## Problem and desired outcome
The Factory default puts implementation, tests, documentation and task evidence into one commit per task. The owner wants Factory changes and code changes in separate commits, with Factory commit messages starting with `factory:`. Outcome: a rule in root AGENTS.md (INDIGO repository rules) that states this and takes precedence over the one-commit default.

## Users and use cases
Agents and maintainers committing INDIGO work; reviewers reading history who want code changes without Factory bookkeeping.

## Scope and non-goals
In scope: one bullet in AGENTS.md "Repository Hygiene". Non-goals: changing `.factory/routines/git.md` or other template-owned Factory files (template-updates routine), rewriting existing history.

## Constraints and references
Root AGENTS.md precedence (explicit owner instructions), `.factory/routines/git.md` one-commit default, `.factory/routines/process-checks.md` committed-range check. Owner decision: agent instruction files such as AGENTS.md are Factory changes, so this task has no code change and is delivered as a single `factory:` commit.

## Initial questions
None; the owner specified the rule and the `factory` prefix.
