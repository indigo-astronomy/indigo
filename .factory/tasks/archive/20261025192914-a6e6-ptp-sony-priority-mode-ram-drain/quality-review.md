# Task quality review

Verdict: pass
Reviewer and independence: two fresh read-only general-purpose subagents (same model), plus the coordinator's own pass
Reviewed revision and working-tree state: uncommitted diffs on the task branch; the final candidate adds only the listed fixes and the tightened access rule after the second review

## Scope and method
Sony exposure state machine, event handler, connect handshake, generic private data, hardware suite (EXIF parser, new case) and fake model, with AGENTS.md style rules.

## Quality findings
Round 1: pending record kept for a non-shot (fixed: cleared at a timeout once the shot's expected end has passed and memory reads 0), EXIF big-endian over-read (fixed), countdown wait could pass without a shot (fixed), expiry test no longer testing expiry (rewritten), A7R4 fake path not exercised (added with polled objects), silent PriorityMode failure (logged). Round 2: deadline clear too early for long shots (fixed), lag window after a drain reopened by the press (fixed by a fresh read after 1 s instead of clearing at the press). Recorded only: old-API late image after exhausted retries; one refresh per exposure.
A fake case for an unannounced memory drop was written and removed because it passed with the old rule too (not discriminating).

## Fix reinspection
Coordinator reinspected each fix; negative controls in verification.md.

## Quality verdict
Pass; limitations in the driver REFACTOR.md.
