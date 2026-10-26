# Specification

AC-001: All six C examples build with current headers. Make all uses service_discovery; clean is idempotent and removes all outputs including the legacy misspelling.
AC-002: Failed dynamic loading returns failure without removing an uninitialized entry.
AC-003: Remote examples accept host/service/device configuration, check setup, and fail within a bounded operation timeout. Cleanup uses only an acquired server.
AC-004: Image examples handle open, short write and close failures, report failure, release URL BLOB ownership and never log false success.
AC-005: Executable examples handle pipe/fork/exec errors, close unused descriptors and detach/release/stop/reap in normal shutdown without exit/kill in callbacks.
AC-006: Preserve simulator image capture and mount slew demonstrations. No property changes.

Complexity high: asynchronous completion and adapter/process lifetimes. Risk medium: file and process cleanup. No new dependencies.
