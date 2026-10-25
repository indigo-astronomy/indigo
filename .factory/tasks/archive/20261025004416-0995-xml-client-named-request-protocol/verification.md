# Verification report

Own acceptance results, exact executed checks, baseline comparisons and retest evidence here. Link quality-review.md for diff quality and security.md for security conclusions; do not copy those reviews or the handoff timeline.

Verdict: pass
Reviewer: verifier role, same agent as the developer (Claude Code, claude-opus-5-5)
Date: 2026-10-25
Revision and working-tree state: `task/20261025004416-0995-xml-client-named-request-protocol` at master `5aa5660f3`, uncommitted task changes.
Review independence: not independent. Mitigation: the regression case was run on the unchanged library first and failed.

## Acceptance evidence
| Criterion | Result: pass / fail / blocked / N/A | Evidence and rationale |
| --- | --- | --- |
| AC-001 | pass | V1 (fails on the unchanged library), V2 |
| AC-002 | pass | V2 |
| AC-003 | pass | V5 |
| AC-004 | pass | V3, V4 |

## Executed checks
Linux x86_64 (Ubuntu, gcc 13.3.0), `libindigo` rebuilt with `make -C indigo_libs libindigo`.

| Command | Working directory / environment | Exit code | Result and evidence |
| --- | --- | --- | --- |
| V1 `./build/unit/test_protocol_xml` with the unchanged library | indigo_test | 1 | new case FAIL: the named request carried no switch (followed by a cascade in the next case, whose cleanup was skipped) |
| V2 the same with the fix | indigo_test | 0 | 9/9 PASS |
| V3 `make test-unit` | indigo_test | 0 | 348 PASS in 26 suites, no failure |
| V4 `./build/integration/test_detach_abort` | indigo_test | 0 | all tests passed |
| V5 `indigo_server -vvv -p 7655 indigo_aux_giotto`, `indigo_prop_tool get_state/list_state` after OFF and AP | root | 0 | IDLE after OFF, OK after AP (before the fix: OK after OFF although the server sent `state='Idle'`); the server answered `switchProtocol` |
| V6 `./.factory/scripts/security`, `git -c core.whitespace=cr-at-eol diff --cached --check`, `findings check`, process check | root, Git wrapper | 0 | pass |

## Baseline comparison and regressions
XML protocol suite 8/8 before, 9/9 now; no other change.

## Findings
None registered.

## Manual checks and limitations
Not checked against a real INDI server (none here); `switch` is already sent to such servers on the request for
everything. Only Linux.

## Verification verdict rationale
The reproduced wrong state is gone; legacy behaviour for INDI servers is unchanged.

## Register reconciliation
No finding.
