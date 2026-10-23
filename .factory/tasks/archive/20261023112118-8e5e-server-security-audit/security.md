# Defensive server security audit

Verdict: changes required
Reviewer: coordinator reconciliation of three separately delegated source reviews.
Date: 2026-10-23. Source base: cb3c392e9f8833ddb6d29744a34cfae618b04fc8. Product source unchanged.
Independence: surface reviewers are separate contexts; coordinator owns reconciliation and records, without application implementation. Requested reviewer model gpt-6-astra/high per repository policy; actual inference settings are not independently exposed.

## Authorization and boundaries

The owner explicitly requested renewed defensive review of their own open-source project for defect removal. This supersedes the earlier exclusion for this focused task; historical audit scope is preserved. All work is local source inspection and bounded synthetic verification. No third-party target, deployment, real credentials, service disruption or hardware testing.

Assets include server availability, in-process memory, device data integrity, host management command arguments and browser rendering. Trust boundaries are TCP input, protocol decoding, HTTP BLOB cache publication/lifetime, management helper execution, and device/upstream data rendered by clients. Actual deployment firewall, helper installation privileges and hardware are unknown.

## Evidence and coverage

- [Transport review](transport-security.md): complete TCP/HTTP handler plus dependent read/allocation/cache/worker contracts.
- [Parser review](parsers-security.md): XML/JSON parser state, buffers, protocol direction and adapters.
- [Access/server/UI review](access-security.md): documented token model, management commands, maintained browser surfaces and configuration.
- Each corresponding coverage JSON records inspected identities and exclusions. [Prior evidence disposition](prior-evidence-disposition.md) rejects the zero-master-token proposal because it conflicts with explicit documentation. No retained proposal was applied.

The token/locking design explicitly is not a security boundary and requires a configured master token. Public control and read visibility are documented behavior, not new vulnerabilities. SHA-1 is used for the WebSocket protocol handshake; no password-security claim follows from that algorithm. Two existing transport defects remain assigned to their existing IDs (FND-000046 and FND-20261023065605-60d4-repository-audit-009).

## Findings and repair gates

Registered 18 new findings: 13 high, 4 medium and 1 low; 17 security findings and one local functional ACL defect. IDs FND-20261023112118-8e5e-server-security-audit-001 through -018. The central active records and [finding-map.json](finding-map.json) own canonical IDs and dispositions; the linked surface sections provide locations, prerequisites, impact, remediation and evidence strength. No claimed fix or retest exists. High security defects continue to block product security sign-off. The owner explicitly authorized closing and archiving this evidence-only audit, with repairs assigned to a subsequent task; see brief.md and handoff.md. This is an audit-delivery exception, not risk acceptance or release authorization. Findings remain active and the security verdict remains changes required.

## Checks and limits

See [verification](verification.md) for actual configured check results and bounded synthetic diagnostics. Factory's security command validates workflow boundaries only. Source inspection confirms the stated contracts; it does not certify exploitability, every platform/configuration, all vendor dependencies, browser execution, runtime races, hardware behavior or the absence of other defects. No broad server fuzzing, network flood or high-memory allocation was run.

Accepted risks: none. Next: repair confirmed issues in bounded groups, derive regressions from their contracts, and rerun independent security review. Unconfirmed leads remain explicitly separate from canonical findings.
