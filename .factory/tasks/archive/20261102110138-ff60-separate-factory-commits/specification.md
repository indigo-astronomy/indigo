# Specification

AC-001 AGENTS.md states that Factory changes (all files under `.factory/` and the agent instruction files AGENTS.md, CLAUDE.md, GEMINI.md, .cursor/rules/ai-factory.mdc, .github/copilot-instructions.md) and code changes (everything else) are committed separately. Owner decision 2026-11-02: AGENTS.md counts as Factory.
AC-002 The code commit of a task comes first; its Factory record follows in a separate commit.
AC-003 Every Factory commit message starts with `factory:` and an example is given.
AC-004 The committed-range process check runs over the range covering both commits, so no gate is weakened.
AC-005 The rule states its precedence over the one-commit-per-task default in AGENTS.md and `.factory/routines/git.md`.
AC-006 Factory checks pass and the committed-range process check passes for this task's two commits.
