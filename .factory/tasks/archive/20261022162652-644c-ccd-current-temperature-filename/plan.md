# Development plan
1. Capture baseline using the actual static filename function with synthetic properties; existing T macros must pass and new t macros must fail.
2. Extend the shared temperature branch to select current or target temperature; preserve uppercase conditions and precision.
3. Run the focused harness, compile the production object, and run configured Factory checks. Update the existing properties reference.
4. Perform separate same-agent verification, final-diff quality and security passes, reconcile maintenance/findings, archive evidence and create one outcome commit. Do not integrate or publish without authorization.
Allowed paths: indigo_libs/indigo_ccd_driver.c, indigo_docs/PROPERTIES.md, this task, .factory/state.json and .factory/maintenance.json. No dependencies or ADR changes needed. Rollback is a revert of the task commit.
Baseline: clean master/origin-master at 80d073bcbb1ef02f0dc85f5c3bb60417d21b1ca7; local-only target synchronization (no remote fetch authorization). Known unrelated FND-000047 affects broad unit tests, so use focused coverage and production compilation.
