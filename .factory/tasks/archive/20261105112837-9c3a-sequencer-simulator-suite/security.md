# Security review

Verdict: pass

Scope: new test implementation only. This is not repaired production authorization or release approval. Independent coverage_review context inspected source, trust boundaries and retained probe evidence without operating the server. Root implemented and executed the tests; runtime verification is not independent. Date:2026-11-05. Base:77bad42c0d871cef5e275e585db23c454df74f9d plus task content identities in process.json.

The launcher starts an isolated native server with a new HOME/cache/output path, existing local indexes only, no downloads, and temporary device plus nonzero-master ACLs. Client writes are authenticated, and negative probes reject unauthorized Server and Scripting Agent writes. The ACL is mode0600, removed after owned-process shutdown, and never included in metadata, logs or repository evidence. The initial unauthenticated-listener issue is fixed and retained as FND-20261105112837-9c3a-sequencer-simulator-suite-002.

The test-only relay trusts the script-owned RO request property on the owned server, deduplicates request IDs, permits only the three native property-write operations plus a fixed native geometry conversion, and adds the test token without exposing it in published request values. Geometry uses the locally built INDIGO library with explicit ctypes ABI and JSON scalar reply. No operation completion or image is simulated by the relay. Argument-array subprocess calls and JSON encoding avoid shell interpolation. Output paths must be new, actual acquisition uses agents and simulators, and only owned processes are stopped. Live preparation modules were restored to their original unloaded state after observing idle/unused selectors; original MCP and selections were preserved.

Open production findings, deliberately not repaired per explicit owner reply "Tests and documented findings only" (translated owner reply):

- FND-20261105112837-9c3a-sequencer-simulator-suite-003 (high): access_token0 matches unset master0 and bypasses nonzero device ACLs. Independently confirmed source and local probe; XML/JSON native regression reproduces it.
- FND-20261105112837-9c3a-sequencer-simulator-suite-004 (medium functional): native Scripting Agent text/number/switch change bindings omit local tokens. Native regression and direct --native-bindings probe reproduce rejected writes.
- FND-20261105112837-9c3a-sequencer-simulator-suite-005 (medium functional): protected Configuration Agent restore replays unauthenticated saved vectors and fails. The live script's positive restore case remains Failed.

The nonzero test master mitigates only the owned test listener. Neither the relay nor this scoped verdict fixes or accepts existing production risk. Coordinator follow-up is retained in all three records; production integration/release remains outside this delivery. No human risk acceptance is recorded.

Configured security checks passed; they inspect local Factory boundaries and do not prove product authorization correctness. Exact probes, failing regressions, final recorded run and limits are in verification.md. Physical hardware, Windows/Linux execution, additional solver backends and guiding resume across a natural flip are unverified. No third-party system was probed and no new package/provider was introduced.
