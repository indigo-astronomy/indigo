# Test plan

Actual Imager Python runner uses a nonexistent child binary with an unmatched filter. Compile exact C/C++ helper and main functions with deterministic transport/cache/stdio stubs; exercise failures and positive controls without hardware, bus startup or image solving. Generator is built standalone and invoked with bounded2s subprocess timeouts. Compare baseline and candidate generated output for all maintained permitted driver definitions. No hardware execution authorized or planned.

Regression files: indigo_test/unit/test_custom_harness_integrity.py and test_generator_tokens.py. Parent registers both in the existing test-harness-verdicts target and Xcode unit group. Existing aggregate77/no-execution behavior is retained.
