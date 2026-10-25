# Verification report

Verdict: pass

Verifier: implementing Codex agent in a separate verification pass; source/doc quality review was independently performed by a parent-arranged in-host reviewer (quality-review.md). Date: 2026-10-25. Candidate: unchanged reviewed source working tree based on fetched origin/master de3cd258e4898a4bdaca77cd9cf7f3b27bd1901a, task branch task/20261025182421-7a16-exclude-factory-from-xcode. Working directory for all checks: /private/tmp/indigo-factory-xcode-exclusion-20261025. Python 3.9.6, Git 2.54.0 (Apple Git-157).

## Acceptance evidence

- AC-1 passes: exclusion invariant in AGENTS.md/current project guidance and ADR-003; current routine/script instructions retire synchronization. Existing owner rules retained.
- AC-2 passes: 15 isolation/transaction regressions prove record operations without Xcode access and preserve identity/collision/approval/duplicate/write/unlink safeguards. Obsolete helper and mapping removed, existing source pin a94e122fbe358d877863cbc1b38ae8284edc91c4 retained.
- AC-3 passes: plist parses; exact parsed-object comparison against base removes only 1320 Factory navigator objects and their incoming child reference, preserving all other 6756 objects and build semantics. No .factory literal remains. All 1622 snapshotted prior evidence/finding/completion/vendor files remain byte-identical. See pbx-check.json and preservation-result.json.
- AC-4 passes for local execution: configured checks/tests/security passed, independent source/doc review passed. Archival and immutable committed-range gate are final coordinator delivery operations; final SHA/gate result is reported externally. No integration or push is authorized.

## Executed checks

| Command or check | Exit | Evidence |
| --- | --- | --- |
| ./.factory/scripts/check (baseline and final) | 0 | baseline-check.log, check.log; diff, findings 86 active/119 archived, decision index, Factory boundaries |
| python3 -B -m unittest discover -s indigo_test/factory -p test_findings_project.py -v | 0 | findings-isolation.log; 15 tests passed in 0.802s; independent reviewer rerun 0.720s |
| ./.factory/scripts/test | 0 | test.log; 190 tests passed in 106.857s |
| ./.factory/scripts/security | 0 | security.log; local security boundaries valid |
| plutil -lint indigo.xcodeproj/project.pbxproj | 0 | syntax valid |
| Python standard-library SHA256 inventory and plutil JSON exact graph comparison against base | 0 | pbx-check.json, preservation-result.json; rerun before closure passed |
| ./.factory/scripts/maintenance status | 0 | maintenance-before-closure.json; 83 completions/31 uncovered, audit due |

The earlier configured-test session lost its process handle and had no terminal summary; it was not counted as passing. The complete suite was rerun to an observed exit 0. No test expectation was weakened. Baseline configured check passed; no new regression or task-scope finding was found.

## Limits and maintenance

No native, vendor, forbidden-driver, or hardware execution was needed for metadata-only scope; these behaviors are not certified. The existing original hardware checkout and ambiguous live/archive task were not repaired or mutated by this agent. A final read-only observation showed external concurrent progress there (same hardware branch, clean HEAD f3803ac6668ee38f4e89baa8f7691f5246a94d58), so no claim is made that other actors left it unchanged.

Full-repository audit remains due. Owner explicitly approved only this change's deferral on 2026-10-25: “Áno, odložiť audit iba pre túto zmenu; potom zostáva splatný”. No checkpoint/history reset. Existing 86 active and 119 archived findings are retained; no new canonical finding or closure was required. Separate security review: security.md. Local ready-commit delivery only; merge and push remain pending authorization.
