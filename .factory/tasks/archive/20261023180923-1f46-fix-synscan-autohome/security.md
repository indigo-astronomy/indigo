# Security review

Verdict: pass

## Scope and independence

Reviewed version 21 SynScan `.driver` and regenerated C, protocol simulator and tests, Makefile test substitutions/selection, Xcode metadata, status/test records and task evidence on included master 114b80ff5157c6c4923e09e232a37f6dcacae0dc. Same agent implemented and performed this separate pass; no independent specialist/provider review or physical-device certification is claimed. Existing owner-selected host/model retained; strong tier recommended for operational concurrency risk, exact model/settings unavailable.

## Boundaries and findings

Device replies remain untrusted and use the existing exact-payload/acknowledgement validation and bounded portable I/O. The new state machine does not add parsing, numeric request validation, dependencies, authentication, configuration paths or remote services. Progress callbacks perform bounded transactions, with monotonic motion/index deadlines and named cancellation; terminal cleanup attempts both axes independently. Stop refusal reports ALERT for abort and a disconnect error message; software cannot guarantee physical stopping when the controller/transport refuses a command.

FND-20261023180923-1f46-fix-synscan-autohome-001 exposed an actual test authority boundary: legacy broadcast discovery could initialize a physical mount before checking responder locality. The test-only network wrapper now supplies 127.0.0.1 to the real discovery API and asserts locality. Production discovery remains available to real users. Normal and instrumented confined discovery cases pass, including dropped replies; central resolution is fixed, not accepted risk. The suite's socket cases are explicitly opt-in while remaining part of the requested recorded driver run.

Private state stays in the serialized master queue; cancellation clears active home stage and scheduled starts/continuations. Homing claims both axes before a previous guide finalizer can restore motion. Tracking/manual/encoder writers that can interfere with homing defer their queued handlers and preserve targets. Disconnect uses generated cancellation and shared-handle ownership, with no extra close after failed open. No provider upload, physical hardware run, push, deployment, secret or dependency addition occurred.

The data-model risk signal from MIGRATION_STATUS.md is metadata only: recalculated simulator/hardware case availability and simulator retest status, with Comment preserved. No database/schema or public-property migration occurs. Xcode parent repairs retain object identities and public test content.

## Evidence and limits

Configured local security checks pass; strict/reproducibility/property checks and ASan/UBSan AUTOHOME/discovery evidence are in verification.md. Device-protocol failure, malformed/missing reply, timeout and emergency-stop refusal cases pass. Full final acceptance belongs to verification.md. Hardware response, physical sensor clearance/deceleration, real motor stop latency and Linux/Windows execution remain unverified under the explicit simulator-only scope. No unresolved security finding was identified in this diff; this verdict is limited to inspected code and performed software checks.
