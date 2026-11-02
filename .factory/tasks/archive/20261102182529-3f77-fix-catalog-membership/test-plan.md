# Test plan

AC-1: independent stack/heap stars, before/end references, byte-offset misalignment, null. Valid-sized source arrays avoid unrelated geometry/allocation behavior.
AC-2: each reference position in a four-element array, repeated references and serialized roundtrip controls.
AC-3: existing index_file_roundtrip, mapped index tests, new catalog membership case; ASan/UBSan and optimized clang build. No broad solve, hardware, real imagery or indexes. Existing tests are registered; unit additions do not change driver-wrapper integration availability count.
