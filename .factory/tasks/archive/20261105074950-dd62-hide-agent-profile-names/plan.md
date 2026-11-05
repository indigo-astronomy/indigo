# Atomic plan

1. Baseline and regression — complete (oldunit1/1+MCP30/30; newregression2/4 beforefix): build/run existing profile and MCP suites, extend profile contract tests, reproduce agent visibility failure on unchanged production.
2. Common fix — complete (centralbitmask default,11versions/propertyreference): one agent-interface-based default on PROFILE_NAME at base attachment; bump all eleven agent versions and relevant version expectation; update property reference.
3. Verification — complete (all11builds,4/4unit+30/30MCP native/sanitized, independentreview): scoped core/all-agent builds, profile and MCP regressions/sanitizers as applicable, unchanged-device persistence, all-agent source inventory and independent quality/security review; no hardware.
4. Commit — pending: archive complete task evidence, stage only task files, capture reviewed Git identities and run process gates. Single requested commit; no push/main integration.
5. Restore unfinished MCP work — pending: return to original task checkout, fast-forward its task branch to this commit if needed, apply saved WIP and resolve only overlap by preserving both changes. Revalidate affected MCP unit/native tests after restoration; no second commit.

Host/model: current inherited host/model, exact model/effort/cost unavailable; no provider switch, extra API charge or budget ceiling. Medium reasoning scope with low reversible implementation risk. Parent owns code/versions/property reference/coordination; scoped test writer owns tests/verification; independent reviewer owns review reports. Same-feature owner-requested correction before unrelated work; maintenance cadence unchanged and due incremental audit not claimed performed. No generator/build packaging change expected.
