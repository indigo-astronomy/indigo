# Verification strategy
AC-001/002: original-source reproduction and valid controls, edge/malformed cases, failure recovery and lifecycle/ownership where relevant. Prefer actual public/parser/hardware-free driver path, no real device. Baseline and affected full suites before/after, retain trace for supported driver contracts.
AC-003: strict compile, sanitizer coverage of changed production code, relevant registered suite, independent review; Windows/Linux/hardware unperformed must remain explicit.
AC-004: findings integrity, Factory checks/test/security, final content identities and committed-range process gate; do not bypass unfinished-audit error or reset cadence.
No shell_compliance, protected drivers, vendor edits or external probes. Test subprocesses/resources isolated and cleaned.
