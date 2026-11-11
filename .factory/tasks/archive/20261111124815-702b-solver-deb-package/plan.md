# Plan

1. Add `INDIGO_SOLVER_PACKAGE = $(INDIGO_PACKAGE)-solver` and a Linux-only `package-solver` target to the root Makefile.
2. Add `indigo3-solver` to the main package `Provides` (versioned), `Replaces` and `Conflicts`.
3. Remove solver package outputs in `clean-all`.
4. Build and copy the solver package in `tools/build_debs.sh` (4th argument) and pass it from the `deb-*-pkg` targets.
5. Verify per test-plan.md.
