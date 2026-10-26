# Plan

1. Establish isolated task branch from local master a35823d61; preserve original checkout and offline dependencies.
2. Separate implementation by disjoint files: numerical/image/LibRaw; clone/guider lifecycle; editor mapping; coordinator portable discovery and SHA-1.
3. Capture pre-fix failures with the new regressions where feasible. Each implementer reports exact commands and coverage limitations.
4. Coordinator integrates Makefile and Xcode references for persistent new non-Factory files, builds modified libraries, runs affected suites and configured Factory checks.
5. Independent final-diff quality review, then security triage; repair/retest any issues.
6. Reconcile nine findings, complete process evidence, archive task, commit verified candidate and run committed-range process checks. Leave master integration and publication pending owner authorization.

No dependencies or architecture changes. Preserve successful paths and existing private-data ownership. Network tests use controlled local resources only. Source-map generator invocation remains temporary and hardware-free.
