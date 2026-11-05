# MCP load-driven lifecycle quality review

Verdict: pass
Date: 2026-11-05
Reviewer and independence: `guides_review` subagent, no production, manual or test authorship. Earlier draft/embedding reports are preserved. Same reviewer separately performed the security pass; this is not independent dual-review certification.
Candidate: uncommitted existing task branch, base `323a560ff37d036c34bd7984a5e38abed9036c6a`. Review is the owner-authorized autostart delta following the embedding candidate, not a new full repository audit.

Final SHA-256 identities: agent `2bf287e4b37b3efe9e10b86e6865cd42e04fd55c9edf1ae2f54e858d167db6f9`; generated help `8e6a232d1faf029f84526e04f2308f1cc7cf823765f673830d83fff610af89be`; integration test `c95f21cd49237b098de8b29ca48308262fc127b04e3e26804b4792f22db1d915`; PROPERTIES.md `2f54fc74c4473cfc35c543ecfa6e48ea5e41936f4d4296fb6d8fd9165e560979`.

## Scope and conclusions

Inspected initialization/error exits, device attach/template/base callback, admission state, shutdown/handler retirement, cache/client lifetime, repeated INIT/SHUTDOWN and reinitialization. Compared the hidden inherited-property convention with Alpaca and the base device/bus implementation. Reviewed revised lifecycle tests, public properties documentation, README, manuals and regenerated bundle. No unrelated production behavior, public custom properties, framework or forbidden-driver changes were introduced.

The explicit owner requirement replaces the prior disabled-by-default contract: successful load now starts service; unload stops it. Removing old CONNECTION-enable/disable test assertions is justified by that changed specification. The 34-case suite retains parser, bus/token, visibility, help, saturation, self-unload and drain coverage.

Admission becomes enabled under the existing mutex only after successful device and internal-client attachment/initial enumeration. This does not newly guarantee every cache allocation succeeded; the existing sticky unavailable-cache error remains authoritative. Earlier failures return before registering a handler or enabling admission. Successful shutdown disables admission before retiring the handler; resource retirement drains users before client/cache/device destruction. Repeated INIT remains idempotent, and reinitialization enables service again. Same-stack native-dispatch shutdown still returns BUSY before changing enabled state.

CONNECTION is hidden before client enumeration, remains undefined and is absent from MCP discovery/cache. The base changeable matcher requires a defined property, so a guessed native CONNECTION request cannot invoke a custom service toggle; that handler no longer exists. MCP get/set of the missing property fails and leaves serving unaffected. Version 3.0.0.2 and the common-property exception/manuals reflect the new contract. No remaining second-connect instruction was found in the current manual/README lifecycle text.

## Evidence and limitations

Baseline, requirement mapping, final native 34/34, sanitizer 34/34 and runner/build evidence belong in [verification-autostart.md](verification-autostart.md) and [verification.md](verification.md). The reviewer independently ran the final verifier-built native binary with authorized isolated loopback permission: `indigo_test/build/integration/test_agent_mcp > /private/tmp/mcp-autostart-independent-review.log 2>&1`; exit 0, all 34 cases passed. This is an independent execution, not an independent rebuild or sanitizer run. No physical hardware, external service, user server restart or real-client operation was performed.

Independent final checks: generator --check reported 18 current documents, git diff --check passed, and configured Factory security-boundary validation passed. Final hashes matched the verifier's frozen identities. Init-failure cleanup was source-inspected; allocation/client-attach fault injection was not added or claimed. Existing embedding/semantic findings and platform limitations remain as recorded in prior reports; no new finding in this delta.

Pass is limited to the changed load-driven lifecycle. Linux/Windows execution, GUI compatibility and physical workflow correctness remain unverified for this adjustment. This review neither accepts unrelated risk nor authorizes commit/push/integration or resets repository-maintenance cadence. Coordinator owns final cleanup and owner handoff.
