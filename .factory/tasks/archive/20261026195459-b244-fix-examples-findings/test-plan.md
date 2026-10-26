# Test plan

AC-001: build all six sources; clean twice. Baseline client.c: nine errors; mount example: missing connect argument.
AC-002: failed simulator-library load must return nonzero without crash.
AC-003: closed loopback port, invalid options, missing device and local simulator success, with deadlines.
AC-004: synthetic BLOB tests for success/open/write/close failure and ownership release.
AC-005: missing executable, EOF child and normal simulator completion with exact child reaping.
AC-006: FITS output and mount simulator completion. macOS only; Linux/Windows/hardware not certified.
