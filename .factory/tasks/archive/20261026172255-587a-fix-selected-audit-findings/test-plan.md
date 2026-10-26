# Test plan

AC-001: synthetic master with one/two slaves, clone identities, enumeration, shared ownership, connected removal rejection and recreate cleanup.
AC-002: repeated 4x4 Bayer tiles with four channel levels for 8/16 bits and saturation edges.
AC-003: repeated synthetic guider attach/detach allocation/release accounting.
AC-004: changing-date results versus fresh recomputation (cache-aware oracle).
AC-005: injected libraw_init failure, return/error and no NULL close.
AC-006: reference epoch and independently computed aberration in degrees.
AC-007: controlled bind failure, repeated attempts and descriptor closure; inspect Windows cleanup branch.
AC-008: explicit CCD logical devices and inherited CCD_EXPOSURE generated ranges; editor suite.
AC-009: existing WebSocket known-answer handshake and sanitize suite with halt_on_error.

Run relevant existing unit suite plus Factory check/test/security. No physical hardware or broad driver suite. Record unavailable platforms honestly. New regressions must fail against original source when practical.
