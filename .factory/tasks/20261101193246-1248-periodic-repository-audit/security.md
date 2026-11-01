# Audit security review

Verdict: changes required

Scope/revision: 42011325b8d3f9dcca2cc768594d42b385d4d481, audit metadata-only diff. Coordinator loads security-reviewer role and reviews component evidence; substantive product-security sign-off is not claimed. Same host/provider throughout, exact model/effort unknown, strong qualified review recommended for remediation. Separate read-only core/tools contexts supply evidence; coordinator is report writer.

Trust boundaries inspected include CLI catalog directory→shell, external image/CSV decoding, network request framing and local test/report integrity. FND-...-001 demonstrates directory command substitution; existing server-security-audit-007 is reopened for missing HTTP request/header bounds while preserving the implemented initial deadline and admission cap. Neither issue is fixed or owner-accepted. Malformed FITS and narrow Bayer memory handling are functional records requiring security-oriented remediation review; remote exploitability is unestablished.

Only synthetic local inputs and in-memory socketpairs were used; no network listener/flood, third-party probing, credentials, hardware or external provider was involved. Factory `./.factory/scripts/security` exits 0 but checks local metadata boundaries only; it cannot waive product concerns. No dependencies, authentication configuration, deployment, generator or product files were changed.

A potential filter format-string forwarding site was rejected as a confirmed issue: ordinary current bus update callbacks receive NULL, and a supported non-NULL direct caller was not established. A server catalog capacity scenario and simulator-cache pointer lifetime need concrete current-consumer validation before registration. Existing blank ACL-line underflow remains under its existing canonical record.

Disposition: retain open records and a live task; no risk acceptance, complete security verdict, archive or qualified full-audit checkpoint. Next assignment is a focused remediation/security review plus completion of remaining audit scope, preserving existing authorization boundaries.
