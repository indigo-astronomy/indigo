# First increment plan

Scope: additive HTTP request-handler API in indigo_server_tcp.h/.c, focused synthetic integration tests and Makefile registration, developer HTTP documentation. Legacy public APIs, GET/PUT callback ownership and prefix routing stay intact. No driver or property changes.

1. Synchronize existing task with current master; retain analysis and owner no-commit gate.
2. Define callback-scoped immutable request/header structs and registration context. Registration returns bool and rejects duplicate path ownership. New handlers match raw origin-form paths exactly, independently of query; no URL decoding. Legacy resources are fallback only.
3. Dispatch new handlers before legacy GET/PUT processing. Parse method/version, validate header syntax and lengths, reject Transfer-Encoding and duplicate Content-Length, buffer exact body bytes up to 1 MiB. POST/PUT require explicit Content-Length (zero valid); other methods may have length or no body. Reject Expect rather than deadlock on 100-continue. Apply read progress/total deadlines, at most 16 buffered requests, and suppress content logging.
4. The callback owns writing the complete response through the provided borrowed handle and must use Connection: close. It may not retain request/handle/context after return or remove handlers synchronously. The server always closes new-style connections; existing retirement keeps the context/code alive through buffered reads and callbacks. Callback CPU/runtime and response size are handler responsibilities, not a generic streaming service.
5. Verify old/new routing, framing and lifecycle with synthetic local sockets; rerun existing HTTP security/resource and WebSocket regressions. Review final uncommitted changes independently and record limits.

Write ownership: coordinator/developer owns library/header and task evidence; test designer owns only the new HTTP-handler integration test and its Makefile entries. Separate files, one writer each. English documentation. No dependencies, hardware, off-limits drivers, generator changes, commit/merge-to-master/publication/archive.

Risk: network parser and callback lifetime high consequence; medium bounded implementation complexity. Balanced inherited host retained, exact model/effort unobservable. Strong specialist review recommended if evidence exposes subtle auth/lifetime ambiguity. No paid provider changes.
