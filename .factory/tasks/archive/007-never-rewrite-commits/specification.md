# Specification

- AC-001: AGENTS.md forbids rewriting existing commits, local or pushed, and names the commands that do it: amend, rebase, squash or fixup rewrites, reset that drops commits, filter-branch, filter-repo and force pushes.
- AC-002: The rule says how to correct a mistake instead (a new commit on top) and that it overrides the pre-publication consolidation allowed by .factory/routines/git.md.
- AC-003: No other instruction file changes; the rule sits in the project-owned INDIGO part of AGENTS.md, not in the template-owned shared part.
- AC-004: The rule reaches origin before the task records, as the owner required, and the records are added later without rewriting that commit.
