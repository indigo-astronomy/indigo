# Audit specification
AC1: inventory maintained components and review design/source contracts, tests, documentation, build/configuration and agent workflow.
AC2: inspect findings against current source, avoid duplicates, and preserve all existing records.
AC3: record reproducible new defects with exact locations, impact, remediation, severity and accountable follow-up; route security concerns through security review.
AC4: run relevant safe local checks, recording actual results and platform/hardware limitations.
AC5: separate full coverage from sampling. Only a qualified full audit resets cadence; unresolved blocking findings or uncovered components prevent that declaration.
Exclusions: vendored dependencies and SDKs, generated output except source-linked checks, build products, obsolete sources, archives except targeted evidence, indigo_test/shell_compliance, and the five owner-protected drivers.

Complexity: high because driver lifecycle, asynchronous callbacks, parser boundaries and test/report contracts interact. Risk: medium for audit misclassification and unsupported completion claims; product edits are outside this audit pass. Assumptions: local source review and synthetic tests, no hardware/publication; no owner-defined budget.

## Owner-approved closure scope
Owner direct instruction on2026-11-01: change the rules as proposed and close this audit as partial, preserving findings and unchecked areas. This supersedes the previous requirement to finish all repository semantic coverage before closing this task. Acceptance is now accurate bounded inspection/reporting, preservation of canonical finding statuses/evidence, explicit unchecked-area follow-up and no full/incremental checkpoint claim or product-security risk acceptance. Previously failed product checks remain failed evidence; no application/recognition rerun is required for administrative closure.
