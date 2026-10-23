# Task quality review
Verdict: pass
Reviewer and independence: quality reviewer pass, same agent as the developer (Claude Code, claude-opus-5-5). Not independent; a separate pass over the final diff.
Reviewed revision and working-tree state: `task/20261023091538-dfa4-primaluce-calibration-position-check` at `8989126b5` plus the uncommitted diff.

## Scope and method
- Read the diff: `.driver` and regenerated `.c`, simulator, test, docs.
- Checked the generator semantics: the finalizer publishes its own states, as before.
- Checked the formatting rules.

## Quality findings
No quality finding remains in the reviewed diff. Notes:
- The position read uses `primalucelib_focuser_get_position()`, which also requires the key,
  instead of a request followed by an unchecked number.
- The check is generic (outside the range), not a test for the magic 1000000.
- The new case uses a one-shot simulator fault and checks behaviour through the published state,
  the message and the requests.

## Fix reinspection
Not applicable; the first version passed.

## Quality verdict
Pass. Limitation: no hardware reproduction.
