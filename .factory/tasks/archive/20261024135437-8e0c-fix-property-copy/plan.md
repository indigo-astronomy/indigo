# Plan

1. Establish native property baseline and add contract regressions without causing deliberate memory corruption on the old implementation.
2. Guard self-assignment, clear old destination dependencies, resize, and preserve actual capacity across the metadata copy. Retain text deep copying.
3. Rebuild only the core library, run native unit regressions and targeted memory checks where available. Run Factory checks.
4. Independent final diff quality/security review; reconcile finding, archive evidence, commit and run committed-range process checks. Keep main integration and publication pending.

## Expanded work assignment

Root retains 001 and all task evidence. Worker analyze_http_resource owns002 in /tmp/indigo-high-http; analyze_core_high owns003/010/011 in /tmp/indigo-high-core; review_property_copy owns042 in /tmp/indigo-high-ui. Each edits disjoint isolated worktrees at base ce3a972818fb2875a7a901678c5c68e406f4c703, produces uncommitted implementation/tests and returns its diff. No worker edits Makefile or canonical registries. Root integrates selected changes, adds central test rules, runs combined checks and assigns independent cross-review. This replaces the initial plan of separate tasks with one task because the core repairs share source files.
