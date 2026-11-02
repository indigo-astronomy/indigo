# Security review

Verdict: pass

Scope181b4f6df server/token sources + actualtests against9557ca85e; developer securitypass and independentrootsource review, runtime developer-owned. RemoteHTTPframing is bounded before handlerdispatch, including dripprogress; idlefirstprotocol gate unchanged. Requestheaderbytes/lines/linepayload bounded, NUL and incompleteframing rejected. Monotonic total+progress clocks perrequest and smallqueuedprefix avoid indefiniteworker retention. Bytes are not consumed by preflight, preserving customPUTparsers; arbitrarycustombody ownership unchanged, BLOBbody existingbounds remain. EstablishedXML/JSON/WebSocket do not inheritHTTPdeadlines; this is not a blanket connectionlifetime cap. Chosen4KiBcap/defaulttimeout behavior and unavailableplatform/customsocket limits explicit.

LocalACLconfig whitespace can no longer trigger buffer[-1]; tokenlookup/normalrecords/refusedmalformed records preserved, no authenticationpermission changes or acceptedrisk. Actual ASan/UBSanbaseline reproducer and fixednormal/sanitized tests in verification.md. No dependencies, credentials, remoteprobe, publicABI/layout, generator or protecteddriver change. Clean configuredsecurity is only structural, not proof of absenceofvulnerabilities. The two requested findings are verifiedfixed within statedsource/runtime limits; canonicalstatus remains root-owned.
