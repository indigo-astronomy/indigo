# Audit specification

AC-001: Inventory all tracked maintained components with explicit vendor/generated/legacy/archive/off-limits exclusions.
AC-002: Inspect design, reliability, tests, documentation, configuration and agent workflow with concrete component evidence; identify actual scope limits.
AC-003: Reconcile current findings without silently closing or accepting existing risks, and register only confirmed new findings with location, consequence, severity and follow-up.
AC-004: Execute appropriate available static and hardware-free checks; record failures and unperformed platform/hardware checks honestly.
AC-005: Produce repository-review.md plus separate verification, quality and security reports. Qualify maintenance checkpoint only if complete coverage and finding gates are satisfied; a partial audit cannot reset it.

Complexity high: large cross-component C/C++ lifecycle, protocol, build and workflow contracts; risk of incorrect assurance medium. Owner follow-up authorizes repairing audit findings 001/002 and closing this task after verification. Scope now includes the shared library Makefile/parser, focused regressions, affected focuser version/generated output and required evidence. Other findings remain follow-ups. Owner resolves any risk acceptance; no such acceptance assumed.

AC-006: Clean archive builds honor configured AR/ARFLAGS on macOS and ar-compatible invocations, preserving private temporary builds and atomic publication.
AC-007: Reply keys match full names at every JSON path segment; prefix collisions cannot satisfy a missing field or hide a later exact match. Existing well-formed responses retain values and command bytes.

## Owner-approved closure scope

On 2026-10-23 the owner explicitly approved closing the two verified repairs with the audit retained as partial, and deferring its remaining semantic review until the next audit task before unrelated development. For this task AC-002/AC-005 require truthful partial reporting and retained due status, not a fabricated full-audit qualification. No security risk is accepted, no checkpoint is reset, and no existing required repair check is waived.
