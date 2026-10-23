# Audit specification

AC-001: Reconcile the retained partial-audit inventory/checks/findings with the current source revision, documenting changed evidence and all maintained component assignments.
AC-002: Complete source/contract review across all maintained components with per-component coverage evidence; searches, regeneration and sampled driver checks alone do not qualify. Keep runtime/platform limits explicit rather than claiming hardware certification.
AC-003: Resolve every previously listed candidate into confirmed finding, existing finding, disproved candidate or concrete evidence-backed remaining limitation; reconcile existing findings without silent closure.
AC-004: Run configured checks and appropriate locally available hardware-free checks; reuse unchanged retained results with their exact revision and scope. Distinguish pre-existing failures, unavailable tools and fresh passes.
AC-005: Register new confirmed findings with severity, owner, evidence and concrete follow-up. Fix/reverify blockers within ordinary authorized scope; no agent risk acceptance. Broader nonblocking repairs remain separate.
AC-006: Independent final diff quality review, scoped security report, verification and process evidence; declare a qualified maintenance checkpoint only after genuine full-coverage and finding gates.

Scope: maintained source, tests, build/platform configuration, documentation, AI instructions; vendor/build/generated/legacy/archive and five owner-prohibited driver directories excluded as documented. No physical hardware, dependency installation, external service probing, generator modification, unrelated refactor, merge or push authorized by this audit request.

AC-007: Reject overlong XML names/entities/attributes safely; enforce device-token authorization when master token is unset; reject malformed/ambiguous BLOB Content-Length before allocation/mutation and preserve prior payload on incomplete upload or allocation failure. Keep valid public requests and ABI unchanged. These are bounded blocking fixes within this audit.

## Owner-directed scope change

The latest owner instruction explicitly excludes server security audits and requests continuing the next step. AC-007 implementation is suspended and its unfinished proposals retained outside production sources. Complete the functional/contract audit and other maintained areas; report the server-security exclusion explicitly. This instruction is not acceptance of any known risk and does not authorize a security certification.
