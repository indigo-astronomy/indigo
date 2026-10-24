# Focused tool input safety test evidence

Test author/verifier: delegated test agent; production implementation was written by coordinator. This is test-author evidence, not independent final-diff approval. Working directory: `/Users/polakovic/Development/indigo`; task branch `task/20261024113017-1e1c-fix-fnd-000036-000037`; base HEAD `25c92ed424f747af067a2009981915c7f65b186a`, with uncommitted production/test changes. Host: Darwin arm64; Apple clang 21.0.0; Python 3.9. No network or hardware used.

## Coverage

| Acceptance | Executable checks |
| --- | --- |
| AC-001 | Real separately compiled property-tool parser; ordinary names, spaces, quote removal/escaped quotes, fitting/oversized device/property/item/value fields, exactly MAX_ITEMS and MAX_ITEMS+1 get/set requests, malformed delimiters and empty fields. List-property leading/trailing spaces included. |
| AC-002 | Real renamed CLI main, `-r endpoint --help`, so validation precedes return without bus/network startup: host-only, ports 1/65535 and fitting 254-byte host accepted; 255-byte host, missing host/port, zero/negative/oversized/overflow/trailing port and newline host rejected. Default-port assignment requires source review because help exits before connection. |
| AC-003 | RAW1/RAW2/RAW3/RAW6, exact crop bytes and headers, optional metadata, full image and bottom-right single pixel; truncated headers/pixels, unsupported signature, zero/huge dimensions, ROI syntax/trailing input/numeric overflow, zero/negative/out-of-range dimensions and origin rejected with positive exit, never a signal. |
| AC-004 | Deterministic separate-TU fault wrappers for first/second allocation, header/payload read/write, input/output close, plus real output-open failures; normal close always executes in injected close failures. Sanitizer diagnostics fail negative cases too. |

## Commands and results

- `python3 indigo_test/unit/test_tool_input_safety.py`: PASS, 5 property cases plus 6 RAW cases, including bounded subcases. Final test source includes valid remote help paths. All fixtures and compiled objects live under a temporary directory and are removed automatically.
- `SANITIZERS=address,undefined python3 indigo_test/unit/test_tool_input_safety.py`: PASS, final source including positive remote help-path assertions, 11/11; rerun after confirming all four negative-case assertion sites require a positive exit status (`assertGreater(result.returncode, 0, result.stderr)`). Native and sanitizer final reruns both exited 0. No ASan/UBSan diagnostics. Compiler emitted existing `sprintf` deprecation warnings from unrelated property display code.
- `INDIGO_TEST_RESULTS=/tmp/indigo-tool-input-safety-results.tsv python3 indigo_test/unit/test_tool_input_safety.py`: PASS, 11/11 before final positive help-path assertions. Result file inspected: two plans (5 and 6), 11 pass records, zero failures. Final assertions retain the same case totals and recording implementation.
- `git diff --check`: PASS at test handoff; coordinator owns final candidate checks.

The runner respects `CC` (default `cc`) and optional `SANITIZERS`, compiles production files separately, links the parser object with existing `build/lib/libindigo_client.a`, and uses the standard C test runner. RAW fault wrappers are a separate translation unit linked with the real RAW tool object, whose allocation/stdio symbol names are redirected during that test compilation. There is no direct production `.c` inclusion or production API exposure. The prebuilt client archive itself is not sanitizer-instrumented by this runner. Final committed-range and integration checks remain coordinator responsibilities.

Makefile invocation from `indigo_test/`: `$(PYTHON) unit/test_tool_input_safety.py` (or `python3 unit/test_tool_input_safety.py`); require `../build/lib/libindigo_client.a`. No persistent native test build artifacts are produced.

## Safe original-source baseline

Extracted original `indigo_tools/indigo_raw_crop.c` with `git show 25c92ed424f747af067a2009981915c7f65b186a:indigo_tools/indigo_raw_crop.c` into a temporary directory; compiled with `cc -std=gnu11 -I/Users/polakovic/Development/indigo/indigo_libs <temp>/crop.c -o <temp>/crop`. Fixtures used `struct.pack("=4sII", signature, 4, 3) + bytes(range(12))`; each invocation had a 5-second timeout. These three bounded cases each incorrectly exited 0 (expected nonzero):

- Valid RAW1 fixture, ROI `0,0,1,1junk`, ordinary output path.
- Valid RAW1 fixture, ROI `0,0,1,1`, output under a nonexistent temporary subdirectory.
- Unsupported `NOPE` signature, complete 4x3 byte payload, ROI `0,0,1,1`, ordinary output path.

No overflowing original-parser or huge-allocation original-RAW cases were executed. Those paths are covered against the repaired candidate only.

## Handoff content identities

SHA-256 at test handoff (final Git identities owned by coordinator):

- `indigo_tools/indigo_prop_tool.c`: `be2ee86fb74541281ac9224568ab85d6046acb796fe67fbb84adc0e22f25a7f1`
- `indigo_tools/indigo_raw_crop.c`: `d9b0d3d4e01bd0e605ad13f9dbe1d2805abb5fc005b1baa556ee4226dc210f6b`
- `indigo_test/unit/test_prop_tool_parser.c`: `9f651893826580aad6fa2d01dc8b410c7787a088dec67a37428b031c1211cfcf`
- `indigo_test/unit/test_tool_input_safety.py`: `b5f37da8feb84086d95049f900d42036cb3fce128d2233aab2f5b6fdaba33647`
