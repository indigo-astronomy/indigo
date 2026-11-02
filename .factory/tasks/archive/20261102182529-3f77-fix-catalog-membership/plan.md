# Plan

1. Complete: original universal native build, preserved roundtrip1/1 and dedicated expected-failing invalid-reference case.
2. Complete: integer address/alignment/range guard; no foreign pointer subtraction, existing valid serialization preserved; version29→30.
3. Complete: optimized5/5, ASan/UBSan5/5, O0 controls3/3 and independent root source/test review pass. No new source after review.
4. Complete: reviewed source committed separately, evidence prepared for archival/Factory commit. Immutable committed-range gate follows that commit and its result is reported externally. Root owns main integration/central finding closure.
