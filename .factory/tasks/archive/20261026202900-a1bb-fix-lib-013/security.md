# Security review

Verdict: pass
Reviewer: Codex in-host subagent /root/agent_attach, assigned security-reviewer
Date: 2026-10-26
Reviewed base: 3f6f3ad271e7527a3cb5562fa086a717070f68de
Reviewed state: uncommitted working-tree candidate on task/20261026202900-a1bb-fix-lib-013, after the independent final quality review passed. Subsequent task archival, process finalization and commits belong to the coordinator.
Independence: separate agent from the production implementer, test author, runtime verifier and final quality reviewer; same host/provider. This reviewer previously inspected agent/system attach paths read-only and wrote no production code or tests.

## Scope, assets and trust boundaries

Reviewed the actual bus implementation/header diff, lifecycle regression diff, specification, plan, handoff, final verification and quality reports, configured check definitions, retained lifecycle/additional-instance results, native/simulator result metadata, configured check/security logs, simulator log summaries and FND-000013 reconciliation. Assets relevant to this change are process availability, initialized device state and existing bus authorization behavior. This review is limited to the LIB-013 repair and its evidence; it is not the overdue repository audit.

The production change extends the existing recursive registration mutex through the optional device attach callback, then releases this function's registration hold before automatic CONFIG.LOAD. A caller's independent outer recursive hold can remain; the final public comment accurately describes that boundary. Device capacity handling, callback result storage and the later token assignment are unchanged. No new external input or dependency is introduced.

## Risk coverage

| Area | Assessment and evidence |
| --- | --- |
| Authentication and authorization | No new authorization path. The initial access_token reset is now inside the registration hold; automatic configuration and subsequent indigo_get_device_token assignment retain their existing ordering. No new claim that all initialization/configuration authorization races are repaired. |
| Input, injection and output handling | No parser, command construction, protocol, output-encoding or logging change. The new lifecycle tests use synthetic in-process objects rather than external input. |
| Data, secrets and cryptography | No new sensitive data, credential, cryptographic primitive, storage or log payload. |
| Filesystem and outbound access | No added production file or network operation. Existing attach callbacks can perform local work while the bus is held; prior source inspection included agent configuration/bootstrap paths. |
| Dependencies and configuration | No added dependencies or API/ABI data-layout change. Maintenance metadata records the owner's scoped audit deferral without resetting its checkpoint. |
| Availability, failure and waits | The lock prevents concurrent bus access to a device before attach returns. The mutex remains recursive, preserving same-thread property publication. Registration unlock occurs before CONFIG, avoiding a new cross-thread CONFIG wait dependency. Callback failure/null handling and lock release paths remain unchanged. Existing slow attach callbacks now hold the bus for their duration; no concrete new cross-thread wait cycle was found in the preceding scoped driver inspection. The header documents that attach must not wait for another thread to use the bus. |

The known RPI server attach can execute synchronous shell queries. The inspected server startup attaches server_device before starting its listener (indigo_server.c:1996 and following); this diff neither introduces shell input nor makes a new listener available during that startup. RPI timing and arbitrary or opaque SDK callback behavior were not executed or certified. A future callback violating the documented wait contract could hang the whole bus; the scoped source review does not prove universal deadlock freedom.

## Checks and retained evidence

The coordinator executed the builds and runtime/configured checks documented in [verification.md](verification.md). This reviewer inspected their evidence rather than independently rerunning them:

- The new lifecycle regression failed on premature enumeration against the original library, then all five cases passed after the fix (baseline-lifecycle.log and lifecycle.log). Separate cases cover recursive property publication and another thread entering the bus during automatic CONFIG.
- Additional-instance lifecycle passed 2/2; native-unit execution passed with 392 PASS records. Complete recorded Lunatico simulator runs passed 64/64 rotator and 65/65 focuser, with exit 0 in native-results.json/simulator-results.json and matching log summaries.
- Factory tests passed 190 cases. Configured check and security logs report success; the security command is python3 .factory/scripts/validate_project.py --security and checks Factory local boundaries, not whole-source security.
- The coordinator reports macOS 27.0.1 arm64, Apple clang 21.0.0 and Python 3.9.6 in verification.md. This reviewer performed read-only source/evidence inspection and ran git diff --check from /Users/polakovic/Development/indigo, exit 0 with no output. No independent builds, runtime tests, scanners, hardware actions or SDK execution were performed.

The two-second worker handshake has a scheduling-coverage limit. The retained failing baseline establishes that the relevant pre-fix interleaving occurred in that run; it does not establish exhaustive concurrency coverage. The test executable watchdog bounds deadlock regressions.

## Findings and reconciliation

No actionable introduced security finding was identified in the reviewed scope. FND-000013 remains a functional finding: its archived closed record preserves historical provenance and links the repair task and verified results. No unrelated finding was closed by this review.

## Accepted risks

No security-risk waiver or owner risk acceptance is asserted. The owner-selected lock-through-attach design and audit deferral are scoped task authorizations, not whole-system risk acceptance.

## Verdict and limitations

Pass for this task's current source, contract, tests and retained evidence after the independent final quality pass. No unresolved blocking security issue was identified in that scope. No full-driver-fleet, Linux/Windows, physical-hardware, opaque vendor SDK, RPI shell timing, whole-server or whole-repository certification is implied. Final committed-range process validation remains the coordinator's delivery step. This reviewer changes only this report and releases its writer after saving it.
