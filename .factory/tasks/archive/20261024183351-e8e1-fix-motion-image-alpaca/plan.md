# Plan

1. Inspect findings, lifecycle and scoped instructions; capture relevant original-driver/test baseline.
2. In isolated read/write checkouts, implement motion regressions and protocol-specific completion fixes, and bounded shared RAW validation with consumer regressions. Parent writer owns Alpaca capacity fix and its synthetic discovery regression.
3. Import reviewed scoped changes, regenerate/build affected drivers, run recorded simulator/fake SDK regressions and RAW sanitizer checks. Update existing driver coverage documentation and metadata without changing manual migration comments.
4. Conduct independent final-diff quality/security review, reconcile findings, run configured Factory checks, archive completed evidence and commit the tested candidate. Main integration/publication remain pending owner authorization.

No dependencies or generator implementation changes. Reassess after two unsuccessful attempts on the same problem. Separate worktrees isolate concurrent source changes; parent alone writes shared registries/project files.
