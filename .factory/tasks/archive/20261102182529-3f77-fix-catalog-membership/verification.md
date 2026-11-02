# Verification

Verdict: pass

## Scope and environment

Working directory `/private/tmp/indigo-catalog-membership`, source base b6d3ee192. Developer executed tests; root separately inspected the3-file source/test diff and reported PASS. macOS arm64 runtime, universal x86_64/arm64 production build. Prebuilt common libraries copied from identical base; solver/core-test sources instrumented for sanitizer run, common libraries not instrumented. No hardware, image recognition, indexes/downloads, external services or Linux/Windows runtime. This is a bounded native catalog-helper repair, not a full agent driver-run certification. New cases are in an existing registered unit file; driver-wrapper integration case count162 is unchanged.

## Commands and baseline

- `make -C indigo_drivers/agent_platesolver -f ../../Makefile.drv all`: baseline and fixed universal native build exit0. Initial isolated setup lacked build/drivers, so the archive target failed before baseline; created that local output directory and reran successfully. Ignored Makefile.inc paths are remapped to this worktree and common library/include prerequisites copied, preserving primary artifacts.
- `make -C indigo_test build/unit/test_agent_platesolver_core`: exit0. Initial test draft used const pointer elements against a mutable-pointer quad API and produced a qualifier warning; corrected test type before the retained final baseline/retest.
- Baseline `INDIGO_TEST_CASE_FILTER=index_file_roundtrip indigo_test/build/unit/test_agent_platesolver_core`: exit0,1/1 preserved control.
- Baseline `INDIGO_TEST_CASE_FILTER=catalog_rejects_invalid_star_references indigo_test/build/unit/test_agent_platesolver_core`: exit1, expected failure. The old code accepted invalid interior-byte references and a layout-dependent foreign stack reference; no data behind invalid references is dereferenced by the test. Retained catalog-baseline-invalid.txt.

## Found defect and fix

FND-20261101200402-fcb2-fix-solver-fits-header-001: subtracting a quad star pointer from the stars array before checking membership invokes undefined C pointer subtraction. This makes invalid reference rejection dependent on layout/optimization and can serialize incorrect indexes. The repair compares uintptr_t addresses first, checks lower bound, element-size alignment and quotient range, then derives the index without pointer subtraction. Unsigned offset arithmetic is reached only after lower-bound success, so no negative/wrapped offset is accepted. Existing count/storage preconditions and allocation policy are unchanged; no claim to validate arbitrary caller storage size or pointer provenance. Partial catalogs retain existing free-and-clear failure behavior.

New catalog_rejects_invalid_star_references covers stack/heap storage, independent foreign objects, NULL, before/end neighbors, valid one-past pointer and byte-misaligned/before references; each invalid case must fail and leave cleared catalog fields. New catalog_preserves_valid_star_indices checks all positions in reverse order; existing index serialization/mapping controls retain repeated references and stored star data. No test expectation was weakened. Agent DRIVER_VERSION0x0300001D→0x0300001E.

## Fixed execution

- For each filter `catalog`, `index_file_roundtrip`, `mapped_index`, ran `INDIGO_TEST_CASE_FILTER=<filter> indigo_test/build/unit/test_agent_platesolver_core`: exit0,2+1+2=5/5 focused cases pass at productionO3. Diagnostic errors in damaged-index cases are expected rejection controls.
- `clang -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer -std=gnu11 -DINDIGO_MACOS -Duint=unsigned -I. -Iindigo_libs -Ibuild/include -Iindigo_drivers/agent_platesolver indigo_test/unit/test_agent_platesolver_core.c indigo_drivers/agent_platesolver/indigo_solver.c indigo_drivers/agent_platesolver/indigo_solver_utils.c indigo_drivers/agent_platesolver/indigo_solver_healpix.c -Lbuild/lib -Wl,-rpath,/private/tmp/indigo-catalog-membership/build/lib -lindigo -lindigocat -ljpeg -lm -lpthread -o /tmp/catalog-sanitize-core`: exit0. Same three filters against that binary: exit0,5/5; no sanitizer diagnostic. No leak-sanitizer claim.
- The same direct compile command with `-O0` replacing `-O1 -fsanitize=address,undefined -fno-omit-frame-pointer`, output `/tmp/catalog-o0-core`: exit0. `catalog` and `index_file_roundtrip` filters against that binary: exit0,3/3.
- `./.factory/scripts/check` and `./.factory/scripts/security`: exit0; scoped Factory validation only. Root runs combined required Factory regression scope, no per-agent197-case repeat.

AC-1,AC-2,AC-3 pass with the limitations above. Console output retained as tracked.txt, trailing whitespace normalized only. Source review root passed before source commit; runtime execution is developer-owned.

## Final test summary

Baseline preserved control1/1; new expected-failure reproducer0/1. Fixed optimized focused5/5, ASan/UBSan5/5, unoptimized controls3/3. Hardware0 run,0 passed. No real-image solve or full agent test run/README record claimed. Immutable process gate runs after source/Factory commits and is recorded externally. Main integration/publication remain coordinator-owned.

`make -C indigo_test test-clean`: exit0, task test build artifacts removed after retaining evidence. Source commit2df9f0585 contains the reviewed code. Final report review/Factory archival belongs to the root handoff.
