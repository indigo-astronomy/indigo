# Test plan

AC-001/002: Compile the actual property tool translation unit separately with renamed main; link its parser functions with the harness and existing client library. Static valid remote endpoint behavior is reviewed by inspection; invalid endpoint handling exercises CLI main before connection. Check normal and boundary fields, counts, whitespace/quotes and malformed input without network access.

AC-003/004: Invoke compiled RAW crop in temporary directories against synthetic files for all four types. Compare exact crop bytes. Check malformed/truncated headers and pixels, invalid dimensions, signatures, ROI bounds/numeric overflow/trailing input and output errors. Use ASan/UBSan when supported. Baseline tests are expected to fail on the original source; isolate dangerous baseline inputs.
