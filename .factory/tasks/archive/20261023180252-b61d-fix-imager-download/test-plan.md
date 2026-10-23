# Test plan

AC-001: Synthetic XHR HTTP 404/500/206/0, empty/null Blob, incomplete progress, error/abort/timeout, DOM download failure and agent Alert. Assert no delete/continuation, visible error and retry.
AC-002: Verify click/delete/timer ordering and request-local filename despite later mutation; advance synthetic list and execute continuation.
AC-003: Repeat button/property/event notifications and verify at most one action. Execute exact inline page JavaScript in a VM with synthetic DOM/XHR/property bus, without network/hardware.
