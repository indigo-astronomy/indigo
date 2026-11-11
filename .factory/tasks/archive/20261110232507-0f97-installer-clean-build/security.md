# Security review

Verdict: pass
No security findings (retrospective, 2026-11-11, from the original quality review).
`rm -rf` is limited to `build/<configuration>/<platform>` with defaulted variables and is skipped with `SKIP_BUILD=1`. Not independent.
