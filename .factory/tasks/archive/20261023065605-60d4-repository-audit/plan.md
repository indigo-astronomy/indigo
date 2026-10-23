# Audit plan

1. Inventory maintained files and scoped instructions; establish reviewed revision and current active findings.
2. Review build/workflow configuration, changed contracts since prior audit, framework transport/lifecycle, server/UI, all driver families/platforms and utilities/examples with source evidence. Use whole-inventory static analysis to locate candidates, then inspect relevant call paths before confirmation.
3. Audit native/Factory test wiring and available checks without broad builds that touch excluded drivers; run safe hardware-free targets and record limitations.
4. Reconcile finding evidence; write scope matrix, new findings, priorities, security triage and same-agent independence limits.
5. Separate final evidence-diff quality pass, required checks and maintenance qualification. Archive/commit only if actual completion gates pass. Otherwise retain live task with precise remaining work and do not claim full audit.

## Authorized blocker repair

1. Preserve audit clean-build failure as baseline for 001; change only archive flag forwarding. Validate a fresh macOS archive and an ar-compatible archive, including symbol presence and no leaked temporary outputs.
2. Add a serialized-reply key collision regression against old parser and record its expected failure. Existing 24 library cases and driver simulator coverage provide preservation baseline.
3. Require equal key lengths before comparison. Increment only affected focuser version 28 to 29 and regenerate unchanged generator outputs. No properties or generator implementation change.
4. Run library unit suite, actual driver simulator/motion suites and sanitizer scope where available. Noninteractive simulator mode; no hardware testing. Update required recording only and preserve migration Comment.
5. Independent subagents review core/platform/tooling coverage and final repair diff; coordinator records true closure scope. Never infer a full checkpoint or a waiver from the request to close.
