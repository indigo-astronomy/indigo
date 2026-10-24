# Task quality review
Verdict: pass
Reviewer and independence: quality reviewer pass, same agent as the developer (Claude Code, claude-opus-5-5). Not independent; a separate pass over the final diff.
Reviewed revision and working-tree state: `task/20261023231027-08cd-esatto-calibration-home` at `ba09a1367` plus the uncommitted diff.

## Scope and method
- Read the `.driver` diff and the regenerated `.c` diff. Only the version, the flag, the visibility, the two messages and a comment changed.
- Checked that the generated connect handler still defines X_CALIBRATE after `on_connect`, so hiding it there takes effect. `esatto_profile` confirms this.
- Compared the behaviour with the v4.0 document and the two hardware logs.
- Checked the formatting rules: tabs, braces, calls on one line.

## Quality findings
None. Notes:
- X_CALIBRATE is now shown explicitly on a SESTO SENSO. Before, a SESTO SENSO following an unsupported model on the same device would have kept it hidden; this only makes the visibility follow the model.
- The home-sensor reason uses the reported state only (value 0, target above 0, ESATTO, not aborted). It needs no new parsing of `NOPCMD`, which flickers within one move.
- The needs-calibration reason still takes precedence.
- The simulator threshold is marked as an assumption, and the hardware check agrees with it.

## Fix reinspection
The first test run failed because the driver library had not been rebuilt (wrong make invocation), and because the inward move was too long for `position_is`. Both were fixed. The final cases fail without the driver change and pass with it.

## Quality verdict
Pass.
