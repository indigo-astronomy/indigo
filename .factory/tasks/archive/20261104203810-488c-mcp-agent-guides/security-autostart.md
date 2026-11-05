# MCP load-driven lifecycle security review

Verdict: pass
Date: 2026-11-05
Reviewer: `guides_review`, no implementation/test/manual edits; separate security pass by the same reviewer as quality review.
Candidate identities and scope are recorded in [quality-review-autostart.md](quality-review-autostart.md). This report supplements and preserves the prior embedding security review.

## Authority and boundaries

The owner explicitly requested that loading MCP activates service immediately and that unloading stops it, removing the redundant inherited CONNECTION toggle. The activation change is intentional authorized behavior, not an accidental authentication bypass. Loading this module is now the service-activation action. Existing listener reachability, native authorization scope and token behavior remain unchanged and documented; this pass adds no authentication claim or new approval gate.

Reviewed the external HTTP admission boundary against initialization/teardown, native hidden-property behavior, active callbacks and the bus/cache lifetime. Service admission becomes true only after successful device/client setup and under the same mutex used by request admission and shutdown. Error exits do not publish the endpoint. Shutdown turns admission off before removing/draining the route and destroying state. Same-stack self-unload returns BUSY without partially disabling the running service. Eight-request saturation still returns 503; removing the public disabled state does not remove the capacity bound.

No new listener, request parser, script execution, path access, network fetch, secret exposure, dependency, cryptographic mechanism or native bus authority was introduced. Hidden CONNECTION is not exposed through MCP and cannot remain as a secret service switch: the custom handler was removed, the inherited base callback requires defined/changeable properties, and hidden CONNECTION is not defined. Runtime configuration/cache accesses retain their existing ownership. Earlier static help whitelist/escaping protections are unchanged.

## Evidence, findings and limits

Independent final native integration execution passed all 34 cases using isolated loopback fixtures; log `/private/tmp/mcp-autostart-independent-review.log`. Reviewed verifier evidence for final 34/34 native and 34/34 ASan/UBSan plus the recorded runner; see [verification-autostart.md](verification-autostart.md). Independently reran generated-help freshness, whitespace and `python3 -B .factory/scripts/validate_project.py --security`: all passed. The Factory check validates local repository boundaries, not complete application security.

No new security finding in this delta. Existing open semantic/source-contract findings remain outside this lifecycle change; no owner risk acceptance was inferred. Initialization failures were source-inspected, without fault-injection coverage. Linux/Windows execution, external-client compatibility and full hostile-load/fault testing were not performed. No live user server or hardware was modified.

Pass applies to the requested load/unload control model with preserved admission/drain/native-token boundaries. It does not certify native authentication strength, a universal absence of vulnerabilities or safe physical equipment behavior. No commit, integration or publication permission follows from this review.
