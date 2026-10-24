# Audit specification

AC-001: Inventory all maintained files by component using Git, classify source-of-truth/generated/vendor/excluded paths, explicitly load scoped rules and record exclusions.
AC-002: Review design, reliability, source contracts, tests, documentation, build/configuration and workflow across every maintained component class, with file-level coverage evidence and concrete semantic checks; report remaining coverage gaps and do not call sampled/partial work full.
AC-003: Validate and reconcile active findings, deduplicate new evidence, register each confirmed new finding with path/line, impact, severity, confidence, follow-up and accountable owner; never mark a report as a fix or risk acceptance.
AC-004: Run appropriate local checks, distinguish environment/baseline failures and unverified platforms/hardware, inspect recent change integration and supporting contract tests.
AC-005: Independently review final audit evidence and update qualified completion/cadence only after actual full coverage is demonstrated. Keep a partial result active if mandatory coverage remains.

Complexity: high (large heterogeneous C/driver/test/workflow corpus and cross-component contracts). Risk: high for security review accuracy, low/reversible for audit artifacts. Strong reasoning recommended; preserve current host settings, actual model/effort unknown, no additional provider/spend authorization. This audit is a scoped source/contract and repository-health assessment, not a hardware/release or whole-system security certification.
