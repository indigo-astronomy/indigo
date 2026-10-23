# Plan

1. Revalidate findings against current server help, make rules, example sources and local link targets (AC-001–005).
2. Edit only the five affected developer guides; reconcile newer generator prerequisite behavior (AC-001–005).
3. Compare documentation with source, check links/whitespace and run configured Factory check/test/security groups. No new prose-only unit tests or driver builds (AC-001–006).
4. Perform separate same-agent verification, documentation, quality and low-risk security passes; close findings with verification references.
5. Archive completed evidence, record staged identities and commit one reviewed candidate; run committed-range gate. Integration/publication remain pending without authority.

Baseline: master and origin/master equal 2ad52b557492a1126d7f963c497bedfa2eba4183 after fetch; initially clean. All five historical problems reproduced by source/document inspection. No executable behavior changes warrant native builds or hardware testing.
