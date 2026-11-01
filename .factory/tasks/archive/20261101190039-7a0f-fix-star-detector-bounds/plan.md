# Plan

1. Preserve preceding verification commit as task base; local master remains untouched.
2. Add common geometry/radius validation and clamp scan border to one; use checked size_t scratch sizing. Document contract in public header.
3. Add automated test target compiling the changed translation unit directly; run the acceptance matrix under ASan/UBSan and compare ordinary precise output against retained pre-optimization baseline.
4. Independently review final diff, run configured checks, reconcile findings, archive evidence and commit. Publication/integration pending authorization.
