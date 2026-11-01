# Repair plan
1. Preserve unfinished audit evidence as a documented prerequisite checkpoint, then isolate repair from configured master in its own worktree. Complete.
2. Developer: establish a synthetic characterization/regression against actual Gaia parser. Record expected pre-fix failure.
3. Developer: use portable indigo_uni_scandir, explicit filter/order, checked path construction and ownership cleanup. Keep numeric-parser finding002 outside scope.
4. Verifier: run targeted regressions and relevant tool checks; record actual build/link provenance and platform limitations.
5. Independent reviewer: inspect final diff against literal-path contract, leaks/failure behavior and test strength. Fix/retest if necessary.
6. Coordinator: close001 through findings lifecycle, archive completed repair records, capture content identities, commit once and run committed-range process checks. Unfinished audit remains live; integration/publication/cleanup pending authorization.

Results: step2 reproduced final six-Gaia-method baseline with six expected assertion/subcase failures; step3 implemented scanner/filter/sort/checked paths/cleanup; step4 normal12+parser5, ASan/UBSan, compile and Factory190 passed; step5 independent quality/security passes after Windows-capacity correction and retest. Step6 finding closure, archival and committed-range validation next; no main integration/publication.
