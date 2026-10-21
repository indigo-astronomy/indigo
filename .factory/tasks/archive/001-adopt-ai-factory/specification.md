# Adoption specification

Status: verified adoption; initial full audit explicitly deferred by owner to before the next development task or release.

## Acceptance criteria
| ID | Observable requirement | Evidence |
| --- | --- | --- |
| AC-001 | One clean, detached external template checkout and immutable full SHA used for every source | template-source.json; source verification |
| AC-002 | Existing instructions and application layout preserved; Factory rules precede INDIGO rules; override renamed | instruction diff and scoped context export |
| AC-003 | Every unresolved legacy issue retains an FND alias/evidence/owner; exact REVIEW.md files removed | legacy-findings.md; findings check |
| AC-004 | New task/state, real commands, project map and narrowly installed support | new-task execution, project.md, wrapper tests |
| AC-005 | Lock uses approved URL/full pin and reviewed source-to-target mapping | template-map.json; init and plan |
| AC-006 | Persistent registry preserves all entries and target fleet entry is current at pin | sanitized registration/fleet evidence |
| AC-007 | Every new persistent path is represented in Xcode with valid groups; stale references removed | Xcode path reconciliation and plutil |
| AC-008 | Baseline/post checks, security/quality review and initial full repository audit are honest | verification.md; quality-review.md; security.md; repository-review.md |
| AC-009 | No unauthorized application/README/vendor/off-limits edits or external delivery | final scoped diff and registration privacy checks |

Malformed/conflicting registration data, wrong origins, source pin drift, concurrent inventory changes or existing unknown provenance must block affected writes. No baseline may be invented. Routine reversible instruction/path integration is owner-authorized. The explicit owner reply authorizes migrating findings/deleting only indigo_tests/REVIEW.md and otherwise ignoring that subtree.

Complexity: high for provenance consistency, atomic external registration and preserving coupled instruction/finding/state ownership. Risk: medium; reversible repository changes but loss of findings or private registry data would be material. Existing source evidence resolves routine decisions; no new application semantics are specified.
