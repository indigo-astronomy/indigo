# First increment tests

AC-001: unchanged legacy GET prefix/query behavior, PUT raw headers/body ownership, WebSocket and BLOB/HTTP regression suites.
AC-002: exact path beats legacy prefix, query split, raw method including GET/POST/PUT/DELETE/OPTIONS, case-insensitive header lookup with duplicates preserved, context identity, binary body and fragmented delivery, duplicate registration fails.
AC-003: no callback on malformed/duplicate/overflow length, transfer coding, invalid header name/value, invalid request line, missing POST length, unsupported Expect, oversized body, premature EOF, stalled body, saturation; no pipeline replay after a new request. Existing header framing limits preserved.
AC-004 (transport portion): removal waits for an in-flight callback/body read; no new admission after withdrawal; clean reuse/re-registration. Callbacks must be bounded by contract; general agent/unload tests deferred to agent increment.

Use synthetic loopback sockets and ephemeral port, no hardware or credentials. Accelerated compile-time body deadlines in tests and explicit completion barriers for lifecycle. Existing suite baseline recorded before edits where feasible. No claim of full MCP/Windows/hardware verification.
