# Verification

Status: focused acceptance passes; final synchronized candidate verification pending.

Reviewer: same Codex session as implementer, separate verifier pass; no independent review claim. Working tree based on master 3b48146ca, task branch task/20261023181250-5c67-fix-raw-fits-bounds. Source still uncommitted; exact final source identities will be captured in process.json after synchronization/review.

## Baseline

Built original indigo_fits.c from initial master with `clang -g -fsanitize=address,undefined -Iindigo_libs -c indigo_libs/indigo_fits.c`, archived the real object and linked the new public-API test with the original library only for its then-required allocation symbols. Executing short_headers_are_rejected aborted with exit 134; ASan reported a heap-buffer-overflow read from the one-byte allocation. See baseline.log. Symbolizer startup failed in sandbox, but instrumented bounds evidence is present. This is the expected pre-fix finding, not an unrelated baseline failure.

## Focused acceptance and reproducibility

Working directory for every command below: this task worktree repository root. `sh .factory/tasks/20261023181250-5c67-fix-raw-fits-bounds/verify-fits.sh` exited 0; exact compiler/build/run commands retained in verify-fits.sh, results in fits-verification.log. Apple clang 21.0.0, arm64 macOS host. The script builds a static library containing the actual production converter object and links public API tests to that archive; it never includes production .c source in the test and uses no mocked converter behavior. It builds plain and ASan/UBSan variants with -O2, -Wall/-Wextra/-Werror (test runner unused helper warnings explicitly suppressed for tests only). It also exercises the registered Makefile target against that module archive and runs required test-clean. Full unrelated native driver/unit suites were not run.

- AC-001 pass: NULL required arguments, negative input, every short header length, unsupported type, zero/large/overflowing dimensions, full mono/RGB payload truncation.
- AC-002 pass: 25–100 additional cards, including exactly 36 and 37 total cards; correct END and data alignment, multi-record embedded and mixed metadata, exact 80-byte card.
- AC-003 pass: full RGB48 payload offset, extension without final NUL, optional NUL, malformed fields/quotes/termination, oversized keyword/value; checked RGB bytes in all supported formats.
- AC-004 pass: exact mono8/mono16/RGB24/RGB48 output, unaligned input, zero data padding, previous output preserved on validation and allocation failure. Task-local allocation-failure.c interposes only the converter object's realloc at compile time and verifies ENOMEM at all four header allocations plus final output realloc, followed by success when no allocation is failed. No production hook is introduced.

11 focused cases pass in all three runs (plain, sanitizer, Makefile target). Source-only universal arm64/x86_64 build additionally compiled without warnings and executed on arm64. An initial Makefile link attempt used an arm64-only archive with universal test flags; corrected to universal archive. A task-script first attempt treated existing unused test_runner static helpers as errors; scoped -Wno-unused-function for test compilation resolves that pre-existing warning without changing assertions. A fixture initially left the keyword sentinel before the intended header-boundary data; fixed fixture initialization, retaining expected card counts.

## Factory gates before synchronization

`./.factory/scripts/check` exit 0: diff whitespace, findings integrity, ADR index, project boundaries; check.log.
`./.factory/scripts/test` exit 0: 175 Factory tests, 111.321 seconds; factory-test.log.
`./.factory/scripts/security` exit 0: local structural security boundaries; security-check.log. This is not a semantic security certification.

## Remaining delivery gate

Await coordinator-supplied verified post-006 master SHA. Reconcile maintenance and Xcode project, preserve predecessor tests/source changes, repeat affected acceptance and reviews, archive task and verify committed-range process gate. This report does not yet approve an integrated candidate.
