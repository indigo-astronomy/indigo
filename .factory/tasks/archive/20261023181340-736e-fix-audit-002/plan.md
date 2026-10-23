# Completion plan

1. Preserve the repair checkpoint and merge current master on the existing isolated task branch. Resolve additive maintenance/Xcode conflicts, retaining both sides and validating the PBX graph. Complete.
2. Build the task worktree framework and necessary unchanged unit support archive. Run repaired URL tests normally/under sanitizers, all registered native unit suites, inherited browser tests and configured checks. Complete; known nonblocking FND-000047 is explicitly retained, see verification.md.
3. Perform separate same-agent verification, documentation, quality and security passes. Close only 002, preserve all other findings, archive evidence and register all persistent files in Xcode. Bind reviewed staged source content.
4. Create the final merge/completion commit retaining checkpoint ancestry. Run exact committed-candidate required gates. Fast-forward local master only while its included target is unchanged; clean the completed worktree/branch after integration. No push.
