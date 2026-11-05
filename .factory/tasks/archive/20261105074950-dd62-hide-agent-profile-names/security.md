# Agent profile-name security review

Verdict: pass
Date: 2026-11-05
Reviewer: independent `guides_review` subagent, no implementation/test changes. Same reviewer performed separate quality/security passes; not two independent reviewers. Candidate scope/content identity is recorded in quality-review.md.

## Scope and boundaries

Reviewed the new common PROFILE_NAME visibility default, the authoritative AGENT interface bit across all eleven attachment routes, hidden-property definition/write behavior, ordinary device persistence and the complete bounded diff. The unfinished MCP embedding/autostart work is stashed and outside this commit. No user configuration, external system or hardware was accessed; tests use private synthetic configuration.

The change is a property-visibility correction, not a new authentication mechanism. Agent PROFILE_NAME stays hidden and undefined; the existing base changeable matcher rejects undefined properties, preventing guessed writes from reaching rename/common-file persistence through this handler. No extra writable service control or request-derived file path is introduced. The non-agent predicate is false, preserving normal profile labels, selection and persistence instead of treating all profiles as private.

The allocation, native bus authorization, connection/process/lifecycle paths, serialization, secret handling, logs, dependencies and cryptography are unchanged. Agent-source diffs are version metadata only, plus matching MCP initialize version. There is no security claim that hiding a generic property authenticates a caller, revokes unrelated agent controls or protects arbitrary script-defined properties.

## Evidence and findings

The verifier reproduced pure/mixed agent definition/rename/persistence before repair and preserved non-agent controls, then recorded final unit 4/4 and MCP 30/30 with scoped sanitizer passes. See verification.md for exact evidence and native-linked-library sanitizer limitations. Reviewer independently ran the four profile cases: exit 0, including no definition/update/persistence from guessed agent writes and unchanged physical profile behavior. Source inspection confirms all current agent routes use the common constructor and no current module escapes with an override.

`python3 -B .factory/scripts/validate_project.py --security` passed; this is a local Factory-boundary check, not application-security certification. Whitespace/diff checks passed. No new security finding in this scope. Root functional finding FND-20261105074950-dd62-hide-agent-profile-names-001 has a verified repair; coordinator owns canonical closure. No residual risk waiver or owner self-approval was inferred.

## Limitations and delivery

Pass is bounded to this correction. No full-framework sanitizer build, Linux/Windows execution, hardware, hostile custom script, full bus-security audit or restored WIP validation was performed. Existing generic authorization and native persistence rules remain authoritative. Coordinator owns archival, separate commit and later uncommitted MCP restoration/retest; no publication or main integration permission is implied by this review.
