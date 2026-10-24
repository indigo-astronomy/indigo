# RAW image boundary repair report

Task: `20261024183351-e8e1-fix-motion-image-alpaca`, FND-20261023101753-bc8d-complete-repository-audit-022.

Writer: delegated developer/test designer, isolated detached worktree `/private/tmp/indigo-raw-20261024`, parent owns integration. No commits made. No shared Makefile, Xcode, migration-status, README, TEST_SUMMARY or Alpaca main/version/REFACTOR changes made. No generator, public-property or hardware changes.

## Result

The shared `indigo_validate_raw_image(const void *, long)` copies the RAW header before examining it, verifies minimum header size and non-null input, accepts MONO8/16 and RGB24/48, requires positive dimensions and bounded signed-int pixel offsets, bounds the complete BLOB to INT_MAX, and checks that all pixel bytes are available using division bounds before multiplication. It allows trailing metadata. This is an additive public helper; existing API signatures and structure layout remain unchanged. Existing guider validation is unchanged.

Imager capture validates before reading dimensions or Bayer metadata; its handwritten version advances `0x03000041` to `0x03000042`. Alpaca image conversion validates while holding its acquired cached entry, then unlocks/releases and reports InvalidOperation on failure before sending a successful image response. ImageBytes Content-Length arithmetic uses unsigned long so metadata plus a near-INT_MAX source payload remains representable on 32-bit hosts. Generic platesolver preview validates before JPEG conversion and checks the JPEG helper's existing `3 * pixels * bpp` intermediate against SIZE_MAX; this prevents 32-bit intermediate wraparound while preserving safe source shapes.

## Tests and baseline

Before production changes, original source tests passed: RAW image 15/15, cached Alpaca 1/1, numerical/platesolver 2/2. Commands compiled tests with `-DINDIGO_MACOS -D_GNU_SOURCE -Iindigo_libs -I/Users/polakovic/Development/indigo/build/include`, linked against `/Users/polakovic/Development/indigo/build/lib/libindigo` and the parent Alpaca archive, with an explicit rpath to that library directory. The cached-image fixture initially added a zero-width header expectation; original code incorrectly returned a successful empty Value array. Its retained failure is `/private/tmp/raw-negative-baseline.log` (exit 1). No invalid-memory reproducer was executed against the original code.

After repair, source-current focused tests pass under AddressSanitizer, UndefinedBehaviorSanitizer and float-cast-overflow instrumentation:

- RAW image unit tests: 17/17. New coverage includes null, -1/zero/short header lengths, bad signature, unsupported format, zero dimensions, unsigned/signed product boundaries, overlarge complete BLOB, exact and one-byte-truncated payloads for all four formats, and trailing metadata.
- Cached Alpaca image tests: 3/3. All 1..11-byte headers, all four one-byte-truncated pixel types, zero/huge dimensions and unsupported signature are rejected across all four requested ImageBytes/GZip option combinations. Existing mono8 column order remains. RGB48 JSON planes and ImageBytes metadata, column order, payload and Content-Length remain correct.
- Extracted production Imager/platesolver fixture: 4/4. Existing focus and exposure cases remain; direct actual `capture_frame()` and `update_preview_image()` calls reject malformed inputs before Bayer/JPEG peers and accept four complete formats. The preview fixture models SIZE_MAX=UINT32_MAX to exercise the 32-bit intermediate bound with a header-only synthetic fixture and claimed complete length; no huge allocation occurs. Transport/time, Bayer and JPEG conversion are deterministic peers; shape validation and consumer branches are production source extracted at test build time.

All four changed production C sources compile against current checked-in headers and retained built dependencies. Final `git diff --check` passes. Exact successful command transcript and outputs follow below. The complete execution script is `/private/tmp/raw-repair-checks.sh`.

LeakSanitizer failed to initialize because `detect_leaks` is unsupported on this macOS runtime; `/private/tmp/raw-repair-leak-check-unavailable.log` retains this unavailable check. Successful sanitizer checks use `detect_leaks=0`; no leak-check pass is claimed. Existing framework dependencies are uninstrumented; RAW helpers and Alpaca CCD source are directly instrumented, and the extracted Imager/preview functions are directly instrumented. No Linux/Windows/physical-hardware runtime checks were performed. Native 32-bit execution remains unverified.

## Required coordinator follow-up

Transfer all nine changed files from the isolated worktree without overwriting the parent's separate Alpaca main/REFACTOR work. Rebuild library and the two agents before broad integration: configure normal Makefile.inc in the parent's existing checkout as already available; use `make -C indigo_libs libindigo`, and the established driver-specific Makefile.drv commands for agent_imager/agent_alpaca (existing generated build configuration controls target/platform options). Existing registered targets remain valid: `make -C indigo_test test-raw-image-sanitize`, `make -C indigo_test test-agent-alpaca-cache`, and `make -C indigo_test build/integration/test_agent_imager_numerics` then run the latter. No new Makefile/Xcode registration is needed because all sources/tests already exist.

Parent must run/record the final full Imager simulator scope using `tools/run_driver_test.py`, update version-derived README/TEST_SUMMARY via that tool, finish Alpaca version/REFACTOR and shared task evidence, and obtain the final independent review. The Imager REFACTOR update keeps that full run explicitly pending. Final archive/process/commit/publication are parent responsibilities.

## Successful check transcript (trailing spaces normalized)

```text
+ cd /private/tmp/indigo-raw-20261024
+ cc -DINDIGO_MACOS -D_GNU_SOURCE -Iindigo_libs -I/Users/polakovic/Development/indigo/build/include -c indigo_drivers/agent_imager/indigo_agent_imager.c -o /private/tmp/raw-imager.o
+ cc -DINDIGO_MACOS -D_GNU_SOURCE -Iindigo_libs -I/Users/polakovic/Development/indigo/build/include -c indigo_libs/indigo_platesolver.c -o /private/tmp/raw-platesolver.o
+ cc -DINDIGO_MACOS -D_GNU_SOURCE -Iindigo_libs -I/Users/polakovic/Development/indigo/build/include -O1 -g -fsanitize=address,undefined,float-cast-overflow -fno-omit-frame-pointer -c indigo_libs/indigo_raw_utils.c -o /private/tmp/raw-utils-sanitize.o
+ cc -DINDIGO_MACOS -D_GNU_SOURCE -Iindigo_libs -I/Users/polakovic/Development/indigo/build/include -O1 -g -fsanitize=address,undefined,float-cast-overflow -fno-omit-frame-pointer -c indigo_drivers/agent_alpaca/indigo_alpaca_ccd.c -o /private/tmp/raw-alpaca-ccd-sanitize.o
+ cc -DINDIGO_MACOS -D_GNU_SOURCE -Iindigo_libs -I/Users/polakovic/Development/indigo/build/include -O1 -g -fsanitize=address,undefined,float-cast-overflow -fno-omit-frame-pointer indigo_test/unit/test_raw_image.c /private/tmp/raw-utils-sanitize.o -L/Users/polakovic/Development/indigo/build/lib -Wl,-rpath,/Users/polakovic/Development/indigo/build/lib -lindigo -lm -lpthread -o /private/tmp/raw-utils-sanitize
+ UBSAN_OPTIONS=halt_on_error=1
+ ASAN_OPTIONS=detect_leaks=0
+ /private/tmp/raw-utils-sanitize
Running raw image unit tests
  PASS raw_validation_accepts_complete_formats_and_metadata
  PASS raw_validation_rejects_incomplete_and_unrepresentable_shapes
  PASS selection_rejects_incomplete_rois
  PASS selection_rejects_invalid_geometry
  PASS selection_rejects_nonfinite_or_unrepresentable_centers
  PASS selection_rejects_missing_inputs
  PASS selection_accepts_complete_edge_rois
  PASS psf_high_contrast_mono16_border_uses_finite_variance
  PASS psf_fractional_border_variance_is_preserved
  PASS selection_interior_star_preserves_measurements
  PASS raw_header_type_values_are_stable
  PASS mono_raw_with_bayer_extension_is_detected
  PASS raw_without_bayer_extension_is_not_bayered
  PASS color_raw_is_not_treated_as_bayered
  PASS bayer_channel_equalization_balances_simple_mono8_frame
  PASS saturation_mask_marks_saturated_feature
  PASS contrast_for_constant_frame_is_zero_and_not_saturated
raw image unit tests: all tests passed
+ cc -DINDIGO_MACOS -D_GNU_SOURCE -Iindigo_libs -I/Users/polakovic/Development/indigo/build/include -O1 -g -fsanitize=address,undefined,float-cast-overflow -fno-omit-frame-pointer indigo_test/integration/test_agent_alpaca_cache.c /private/tmp/raw-alpaca-ccd-sanitize.o /private/tmp/raw-utils-sanitize.o /Users/polakovic/Development/indigo/build/drivers/indigo_agent_alpaca.a -L/Users/polakovic/Development/indigo/build/lib -Wl,-rpath,/Users/polakovic/Development/indigo/build/lib -lindigo -lz -lm -lpthread -o /private/tmp/raw-alpaca-sanitize
+ UBSAN_OPTIONS=halt_on_error=1
+ ASAN_OPTIONS=detect_leaks=0
+ /private/tmp/raw-alpaca-sanitize
20:41:52.311431 Application: 0 <- // Wrapped 3
20:41:52.311931 Application: 0 <- HTTP/1.1 200 OK
20:41:52.311931 Application: 0 <- Content-Type: application/json
20:41:52.311931 Application: 0 <-
20:41:52.312148 Application: 0 <- { "Type": 2, "Rank": 2, "Value": [
20:41:52.312318 Application: 0 <- [
20:41:52.312477 Application: 0 <- 1
20:41:52.312640 Application: 0 <- , 3
20:41:52.312799 Application: 0 <- ]
20:41:52.312969 Application: 0 <- , [
20:41:52.313129 Application: 0 <- 2
20:41:52.313310 Application: 0 <- , 4
20:41:52.313468 Application: 0 <- ]
20:41:52.313639 Application: 0 <- ], "ErrorNumber": 0, "ErrorMessage": "", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.313822 Application: 0 <- // Closed
20:41:52.314221 Application: 1 <- // Wrapped 3
20:41:52.314418 Application: 1 <- HTTP/1.1 200 OK
20:41:52.314418 Application: 1 <- Content-Type: application/json
20:41:52.314418 Application: 1 <- Content-Length: 112
20:41:52.314418 Application: 1 <-
20:41:52.314418 Application: 1 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.314623 Application: 1 <- // Closed
20:41:52.314910 Application: 2 <- // Wrapped 3
20:41:52.315111 Application: 2 <- HTTP/1.1 200 OK
20:41:52.315111 Application: 2 <- Content-Type: application/json
20:41:52.315111 Application: 2 <- Content-Length: 112
20:41:52.315111 Application: 2 <-
20:41:52.315111 Application: 2 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.315399 Application: 2 <- // Closed
20:41:52.315737 Application: 3 <- // Wrapped 3
20:41:52.315931 Application: 3 <- HTTP/1.1 200 OK
20:41:52.315931 Application: 3 <- Content-Type: application/json
20:41:52.315931 Application: 3 <- Content-Length: 112
20:41:52.315931 Application: 3 <-
20:41:52.315931 Application: 3 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.316139 Application: 3 <- // Closed
20:41:52.316446 Application: 4 <- // Wrapped 3
20:41:52.316636 Application: 4 <- HTTP/1.1 200 OK
20:41:52.316636 Application: 4 <- Content-Type: application/json
20:41:52.316636 Application: 4 <- Content-Length: 112
20:41:52.316636 Application: 4 <-
20:41:52.316636 Application: 4 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.316807 Application: 4 <- // Closed
20:41:52.317071 Application: 5 <- // Wrapped 3
20:41:52.317259 Application: 5 <- HTTP/1.1 200 OK
20:41:52.317259 Application: 5 <- Content-Type: application/json
20:41:52.317259 Application: 5 <- Content-Length: 112
20:41:52.317259 Application: 5 <-
20:41:52.317259 Application: 5 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.317448 Application: 5 <- // Closed
20:41:52.317728 Application: 6 <- // Wrapped 3
20:41:52.317930 Application: 6 <- HTTP/1.1 200 OK
20:41:52.317930 Application: 6 <- Content-Type: application/json
20:41:52.317930 Application: 6 <- Content-Length: 112
20:41:52.317930 Application: 6 <-
20:41:52.317930 Application: 6 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.318115 Application: 6 <- // Closed
20:41:52.318374 Application: 7 <- // Wrapped 3
20:41:52.318545 Application: 7 <- HTTP/1.1 200 OK
20:41:52.318545 Application: 7 <- Content-Type: application/json
20:41:52.318545 Application: 7 <- Content-Length: 112
20:41:52.318545 Application: 7 <-
20:41:52.318545 Application: 7 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.318717 Application: 7 <- // Closed
20:41:52.318967 Application: 8 <- // Wrapped 3
20:41:52.319155 Application: 8 <- HTTP/1.1 200 OK
20:41:52.319155 Application: 8 <- Content-Type: application/json
20:41:52.319155 Application: 8 <- Content-Length: 112
20:41:52.319155 Application: 8 <-
20:41:52.319155 Application: 8 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.319323 Application: 8 <- // Closed
20:41:52.319580 Application: 9 <- // Wrapped 3
20:41:52.319751 Application: 9 <- HTTP/1.1 200 OK
20:41:52.319751 Application: 9 <- Content-Type: application/json
20:41:52.319751 Application: 9 <- Content-Length: 112
20:41:52.319751 Application: 9 <-
20:41:52.319751 Application: 9 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.319922 Application: 9 <- // Closed
20:41:52.320181 Application: 10 <- // Wrapped 3
20:41:52.320367 Application: 10 <- HTTP/1.1 200 OK
20:41:52.320367 Application: 10 <- Content-Type: application/json
20:41:52.320367 Application: 10 <- Content-Length: 112
20:41:52.320367 Application: 10 <-
20:41:52.320367 Application: 10 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.320540 Application: 10 <- // Closed
20:41:52.320802 Application: 11 <- // Wrapped 3
20:41:52.320971 Application: 11 <- HTTP/1.1 200 OK
20:41:52.320971 Application: 11 <- Content-Type: application/json
20:41:52.320971 Application: 11 <- Content-Length: 112
20:41:52.320971 Application: 11 <-
20:41:52.320971 Application: 11 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.321142 Application: 11 <- // Closed
20:41:52.321398 Application: 12 <- // Wrapped 3
20:41:52.321569 Application: 12 <- HTTP/1.1 200 OK
20:41:52.321569 Application: 12 <- Content-Type: application/json
20:41:52.321569 Application: 12 <- Content-Length: 112
20:41:52.321569 Application: 12 <-
20:41:52.321569 Application: 12 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.321737 Application: 12 <- // Closed
20:41:52.321992 Application: 13 <- // Wrapped 3
20:41:52.322167 Application: 13 <- HTTP/1.1 200 OK
20:41:52.322167 Application: 13 <- Content-Type: application/json
20:41:52.322167 Application: 13 <- Content-Length: 112
20:41:52.322167 Application: 13 <-
20:41:52.322167 Application: 13 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.322337 Application: 13 <- // Closed
20:41:52.322594 Application: 14 <- // Wrapped 3
20:41:52.322765 Application: 14 <- HTTP/1.1 200 OK
20:41:52.322765 Application: 14 <- Content-Type: application/json
20:41:52.322765 Application: 14 <- Content-Length: 112
20:41:52.322765 Application: 14 <-
20:41:52.322765 Application: 14 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.322941 Application: 14 <- // Closed
20:41:52.323196 Application: 15 <- // Wrapped 3
20:41:52.323373 Application: 15 <- HTTP/1.1 200 OK
20:41:52.323373 Application: 15 <- Content-Type: application/json
20:41:52.323373 Application: 15 <- Content-Length: 112
20:41:52.323373 Application: 15 <-
20:41:52.323373 Application: 15 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.323549 Application: 15 <- // Closed
20:41:52.323812 Application: 16 <- // Wrapped 3
20:41:52.323989 Application: 16 <- HTTP/1.1 200 OK
20:41:52.323989 Application: 16 <- Content-Type: application/json
20:41:52.323989 Application: 16 <- Content-Length: 112
20:41:52.323989 Application: 16 <-
20:41:52.323989 Application: 16 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.324167 Application: 16 <- // Closed
20:41:52.324458 Application: 17 <- // Wrapped 3
20:41:52.324645 Application: 17 <- HTTP/1.1 200 OK
20:41:52.324645 Application: 17 <- Content-Type: application/json
20:41:52.324645 Application: 17 <- Content-Length: 112
20:41:52.324645 Application: 17 <-
20:41:52.324645 Application: 17 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.324841 Application: 17 <- // Closed
20:41:52.325104 Application: 18 <- // Wrapped 3
20:41:52.325300 Application: 18 <- HTTP/1.1 200 OK
20:41:52.325300 Application: 18 <- Content-Type: application/json
20:41:52.325300 Application: 18 <- Content-Length: 112
20:41:52.325300 Application: 18 <-
20:41:52.325300 Application: 18 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.325498 Application: 18 <- // Closed
20:41:52.325753 Application: 19 <- // Wrapped 3
20:41:52.325965 Application: 19 <- HTTP/1.1 200 OK
20:41:52.325965 Application: 19 <- Content-Type: application/json
20:41:52.325965 Application: 19 <- Content-Length: 112
20:41:52.325965 Application: 19 <-
20:41:52.325965 Application: 19 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.326147 Application: 19 <- // Closed
20:41:52.326417 Application: 20 <- // Wrapped 3
20:41:52.326606 Application: 20 <- HTTP/1.1 200 OK
20:41:52.326606 Application: 20 <- Content-Type: application/json
20:41:52.326606 Application: 20 <- Content-Length: 112
20:41:52.326606 Application: 20 <-
20:41:52.326606 Application: 20 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.326783 Application: 20 <- // Closed
20:41:52.327043 Application: 21 <- // Wrapped 3
20:41:52.327217 Application: 21 <- HTTP/1.1 200 OK
20:41:52.327217 Application: 21 <- Content-Type: application/json
20:41:52.327217 Application: 21 <- Content-Length: 112
20:41:52.327217 Application: 21 <-
20:41:52.327217 Application: 21 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.327384 Application: 21 <- // Closed
20:41:52.327634 Application: 22 <- // Wrapped 3
20:41:52.327806 Application: 22 <- HTTP/1.1 200 OK
20:41:52.327806 Application: 22 <- Content-Type: application/json
20:41:52.327806 Application: 22 <- Content-Length: 112
20:41:52.327806 Application: 22 <-
20:41:52.327806 Application: 22 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.327981 Application: 22 <- // Closed
20:41:52.328240 Application: 23 <- // Wrapped 3
20:41:52.328411 Application: 23 <- HTTP/1.1 200 OK
20:41:52.328411 Application: 23 <- Content-Type: application/json
20:41:52.328411 Application: 23 <- Content-Length: 112
20:41:52.328411 Application: 23 <-
20:41:52.328411 Application: 23 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.328584 Application: 23 <- // Closed
20:41:52.328834 Application: 24 <- // Wrapped 3
20:41:52.329006 Application: 24 <- HTTP/1.1 200 OK
20:41:52.329006 Application: 24 <- Content-Type: application/json
20:41:52.329006 Application: 24 <- Content-Length: 112
20:41:52.329006 Application: 24 <-
20:41:52.329006 Application: 24 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.329171 Application: 24 <- // Closed
20:41:52.329417 Application: 25 <- // Wrapped 3
20:41:52.329591 Application: 25 <- HTTP/1.1 200 OK
20:41:52.329591 Application: 25 <- Content-Type: application/json
20:41:52.329591 Application: 25 <- Content-Length: 112
20:41:52.329591 Application: 25 <-
20:41:52.329591 Application: 25 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.329764 Application: 25 <- // Closed
20:41:52.330018 Application: 26 <- // Wrapped 3
20:41:52.330188 Application: 26 <- HTTP/1.1 200 OK
20:41:52.330188 Application: 26 <- Content-Type: application/json
20:41:52.330188 Application: 26 <- Content-Length: 112
20:41:52.330188 Application: 26 <-
20:41:52.330188 Application: 26 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.330393 Application: 26 <- // Closed
20:41:52.330659 Application: 27 <- // Wrapped 3
20:41:52.330825 Application: 27 <- HTTP/1.1 200 OK
20:41:52.330825 Application: 27 <- Content-Type: application/json
20:41:52.330825 Application: 27 <- Content-Length: 112
20:41:52.330825 Application: 27 <-
20:41:52.330825 Application: 27 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.331007 Application: 27 <- // Closed
20:41:52.331272 Application: 28 <- // Wrapped 3
20:41:52.331456 Application: 28 <- HTTP/1.1 200 OK
20:41:52.331456 Application: 28 <- Content-Type: application/json
20:41:52.331456 Application: 28 <- Content-Length: 112
20:41:52.331456 Application: 28 <-
20:41:52.331456 Application: 28 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.331651 Application: 28 <- // Closed
20:41:52.331985 Application: 29 <- // Wrapped 3
20:41:52.332197 Application: 29 <- HTTP/1.1 200 OK
20:41:52.332197 Application: 29 <- Content-Type: application/json
20:41:52.332197 Application: 29 <- Content-Length: 112
20:41:52.332197 Application: 29 <-
20:41:52.332197 Application: 29 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.332404 Application: 29 <- // Closed
20:41:52.332696 Application: 30 <- // Wrapped 3
20:41:52.332888 Application: 30 <- HTTP/1.1 200 OK
20:41:52.332888 Application: 30 <- Content-Type: application/json
20:41:52.332888 Application: 30 <- Content-Length: 112
20:41:52.332888 Application: 30 <-
20:41:52.332888 Application: 30 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.333068 Application: 30 <- // Closed
20:41:52.333330 Application: 31 <- // Wrapped 3
20:41:52.333521 Application: 31 <- HTTP/1.1 200 OK
20:41:52.333521 Application: 31 <- Content-Type: application/json
20:41:52.333521 Application: 31 <- Content-Length: 112
20:41:52.333521 Application: 31 <-
20:41:52.333521 Application: 31 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.333715 Application: 31 <- // Closed
20:41:52.333979 Application: 32 <- // Wrapped 3
20:41:52.334178 Application: 32 <- HTTP/1.1 200 OK
20:41:52.334178 Application: 32 <- Content-Type: application/json
20:41:52.334178 Application: 32 <- Content-Length: 112
20:41:52.334178 Application: 32 <-
20:41:52.334178 Application: 32 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.334356 Application: 32 <- // Closed
20:41:52.334626 Application: 33 <- // Wrapped 3
20:41:52.334821 Application: 33 <- HTTP/1.1 200 OK
20:41:52.334821 Application: 33 <- Content-Type: application/json
20:41:52.334821 Application: 33 <- Content-Length: 112
20:41:52.334821 Application: 33 <-
20:41:52.334821 Application: 33 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.335006 Application: 33 <- // Closed
20:41:52.335258 Application: 34 <- // Wrapped 3
20:41:52.335442 Application: 34 <- HTTP/1.1 200 OK
20:41:52.335442 Application: 34 <- Content-Type: application/json
20:41:52.335442 Application: 34 <- Content-Length: 112
20:41:52.335442 Application: 34 <-
20:41:52.335442 Application: 34 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.335618 Application: 34 <- // Closed
20:41:52.335864 Application: 35 <- // Wrapped 3
20:41:52.336062 Application: 35 <- HTTP/1.1 200 OK
20:41:52.336062 Application: 35 <- Content-Type: application/json
20:41:52.336062 Application: 35 <- Content-Length: 112
20:41:52.336062 Application: 35 <-
20:41:52.336062 Application: 35 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.336229 Application: 35 <- // Closed
20:41:52.336479 Application: 36 <- // Wrapped 3
20:41:52.336654 Application: 36 <- HTTP/1.1 200 OK
20:41:52.336654 Application: 36 <- Content-Type: application/json
20:41:52.336654 Application: 36 <- Content-Length: 112
20:41:52.336654 Application: 36 <-
20:41:52.336654 Application: 36 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.336822 Application: 36 <- // Closed
20:41:52.337085 Application: 37 <- // Wrapped 3
20:41:52.337262 Application: 37 <- HTTP/1.1 200 OK
20:41:52.337262 Application: 37 <- Content-Type: application/json
20:41:52.337262 Application: 37 <- Content-Length: 112
20:41:52.337262 Application: 37 <-
20:41:52.337262 Application: 37 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.337434 Application: 37 <- // Closed
20:41:52.337696 Application: 38 <- // Wrapped 3
20:41:52.337887 Application: 38 <- HTTP/1.1 200 OK
20:41:52.337887 Application: 38 <- Content-Type: application/json
20:41:52.337887 Application: 38 <- Content-Length: 112
20:41:52.337887 Application: 38 <-
20:41:52.337887 Application: 38 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.338072 Application: 38 <- // Closed
20:41:52.338328 Application: 39 <- // Wrapped 3
20:41:52.338512 Application: 39 <- HTTP/1.1 200 OK
20:41:52.338512 Application: 39 <- Content-Type: application/json
20:41:52.338512 Application: 39 <- Content-Length: 112
20:41:52.338512 Application: 39 <-
20:41:52.338512 Application: 39 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.338682 Application: 39 <- // Closed
20:41:52.338927 Application: 40 <- // Wrapped 3
20:41:52.339119 Application: 40 <- HTTP/1.1 200 OK
20:41:52.339119 Application: 40 <- Content-Type: application/json
20:41:52.339119 Application: 40 <- Content-Length: 112
20:41:52.339119 Application: 40 <-
20:41:52.339119 Application: 40 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.339295 Application: 40 <- // Closed
20:41:52.339565 Application: 41 <- // Wrapped 3
20:41:52.339740 Application: 41 <- HTTP/1.1 200 OK
20:41:52.339740 Application: 41 <- Content-Type: application/json
20:41:52.339740 Application: 41 <- Content-Length: 112
20:41:52.339740 Application: 41 <-
20:41:52.339740 Application: 41 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.339911 Application: 41 <- // Closed
20:41:52.340164 Application: 42 <- // Wrapped 3
20:41:52.340355 Application: 42 <- HTTP/1.1 200 OK
20:41:52.340355 Application: 42 <- Content-Type: application/json
20:41:52.340355 Application: 42 <- Content-Length: 112
20:41:52.340355 Application: 42 <-
20:41:52.340355 Application: 42 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.340535 Application: 42 <- // Closed
20:41:52.340800 Application: 43 <- // Wrapped 3
20:41:52.340974 Application: 43 <- HTTP/1.1 200 OK
20:41:52.340974 Application: 43 <- Content-Type: application/json
20:41:52.340974 Application: 43 <- Content-Length: 112
20:41:52.340974 Application: 43 <-
20:41:52.340974 Application: 43 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.341156 Application: 43 <- // Closed
20:41:52.341412 Application: 44 <- // Wrapped 3
20:41:52.341596 Application: 44 <- HTTP/1.1 200 OK
20:41:52.341596 Application: 44 <- Content-Type: application/json
20:41:52.341596 Application: 44 <- Content-Length: 112
20:41:52.341596 Application: 44 <-
20:41:52.341596 Application: 44 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.341800 Application: 44 <- // Closed
20:41:52.342069 Application: 45 <- // Wrapped 3
20:41:52.342247 Application: 45 <- HTTP/1.1 200 OK
20:41:52.342247 Application: 45 <- Content-Type: application/json
20:41:52.342247 Application: 45 <- Content-Length: 112
20:41:52.342247 Application: 45 <-
20:41:52.342247 Application: 45 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.342415 Application: 45 <- // Closed
20:41:52.342681 Application: 46 <- // Wrapped 3
20:41:52.342868 Application: 46 <- HTTP/1.1 200 OK
20:41:52.342868 Application: 46 <- Content-Type: application/json
20:41:52.342868 Application: 46 <- Content-Length: 112
20:41:52.342868 Application: 46 <-
20:41:52.342868 Application: 46 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.343041 Application: 46 <- // Closed
20:41:52.343307 Application: 47 <- // Wrapped 3
20:41:52.343481 Application: 47 <- HTTP/1.1 200 OK
20:41:52.343481 Application: 47 <- Content-Type: application/json
20:41:52.343481 Application: 47 <- Content-Length: 112
20:41:52.343481 Application: 47 <-
20:41:52.343481 Application: 47 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.343664 Application: 47 <- // Closed
20:41:52.343911 Application: 48 <- // Wrapped 3
20:41:52.344086 Application: 48 <- HTTP/1.1 200 OK
20:41:52.344086 Application: 48 <- Content-Type: application/json
20:41:52.344086 Application: 48 <- Content-Length: 112
20:41:52.344086 Application: 48 <-
20:41:52.344086 Application: 48 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.344257 Application: 48 <- // Closed
20:41:52.344503 Application: 49 <- // Wrapped 3
20:41:52.344678 Application: 49 <- HTTP/1.1 200 OK
20:41:52.344678 Application: 49 <- Content-Type: application/json
20:41:52.344678 Application: 49 <- Content-Length: 112
20:41:52.344678 Application: 49 <-
20:41:52.344678 Application: 49 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.344843 Application: 49 <- // Closed
20:41:52.345087 Application: 50 <- // Wrapped 3
20:41:52.345260 Application: 50 <- HTTP/1.1 200 OK
20:41:52.345260 Application: 50 <- Content-Type: application/json
20:41:52.345260 Application: 50 <- Content-Length: 112
20:41:52.345260 Application: 50 <-
20:41:52.345260 Application: 50 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.345429 Application: 50 <- // Closed
20:41:52.345672 Application: 51 <- // Wrapped 3
20:41:52.345840 Application: 51 <- HTTP/1.1 200 OK
20:41:52.345840 Application: 51 <- Content-Type: application/json
20:41:52.345840 Application: 51 <- Content-Length: 112
20:41:52.345840 Application: 51 <-
20:41:52.345840 Application: 51 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.346006 Application: 51 <- // Closed
20:41:52.346251 Application: 52 <- // Wrapped 3
20:41:52.346436 Application: 52 <- HTTP/1.1 200 OK
20:41:52.346436 Application: 52 <- Content-Type: application/json
20:41:52.346436 Application: 52 <- Content-Length: 112
20:41:52.346436 Application: 52 <-
20:41:52.346436 Application: 52 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.346606 Application: 52 <- // Closed
20:41:52.346863 Application: 53 <- // Wrapped 3
20:41:52.347039 Application: 53 <- HTTP/1.1 200 OK
20:41:52.347039 Application: 53 <- Content-Type: application/json
20:41:52.347039 Application: 53 <- Content-Length: 112
20:41:52.347039 Application: 53 <-
20:41:52.347039 Application: 53 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.347238 Application: 53 <- // Closed
20:41:52.347486 Application: 54 <- // Wrapped 3
20:41:52.347669 Application: 54 <- HTTP/1.1 200 OK
20:41:52.347669 Application: 54 <- Content-Type: application/json
20:41:52.347669 Application: 54 <- Content-Length: 112
20:41:52.347669 Application: 54 <-
20:41:52.347669 Application: 54 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.347841 Application: 54 <- // Closed
20:41:52.348096 Application: 55 <- // Wrapped 3
20:41:52.348291 Application: 55 <- HTTP/1.1 200 OK
20:41:52.348291 Application: 55 <- Content-Type: application/json
20:41:52.348291 Application: 55 <- Content-Length: 112
20:41:52.348291 Application: 55 <-
20:41:52.348291 Application: 55 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.348472 Application: 55 <- // Closed
20:41:52.348733 Application: 56 <- // Wrapped 3
20:41:52.348907 Application: 56 <- HTTP/1.1 200 OK
20:41:52.348907 Application: 56 <- Content-Type: application/json
20:41:52.348907 Application: 56 <- Content-Length: 112
20:41:52.348907 Application: 56 <-
20:41:52.348907 Application: 56 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.349085 Application: 56 <- // Closed
20:41:52.349354 Application: 57 <- // Wrapped 3
20:41:52.349536 Application: 57 <- HTTP/1.1 200 OK
20:41:52.349536 Application: 57 <- Content-Type: application/json
20:41:52.349536 Application: 57 <- Content-Length: 112
20:41:52.349536 Application: 57 <-
20:41:52.349536 Application: 57 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.349717 Application: 57 <- // Closed
20:41:52.349964 Application: 58 <- // Wrapped 3
20:41:52.350158 Application: 58 <- HTTP/1.1 200 OK
20:41:52.350158 Application: 58 <- Content-Type: application/json
20:41:52.350158 Application: 58 <- Content-Length: 112
20:41:52.350158 Application: 58 <-
20:41:52.350158 Application: 58 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.350331 Application: 58 <- // Closed
20:41:52.350584 Application: 59 <- // Wrapped 3
20:41:52.350764 Application: 59 <- HTTP/1.1 200 OK
20:41:52.350764 Application: 59 <- Content-Type: application/json
20:41:52.350764 Application: 59 <- Content-Length: 112
20:41:52.350764 Application: 59 <-
20:41:52.350764 Application: 59 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.350943 Application: 59 <- // Closed
20:41:52.351184 Application: 60 <- // Wrapped 3
20:41:52.351377 Application: 60 <- HTTP/1.1 200 OK
20:41:52.351377 Application: 60 <- Content-Type: application/json
20:41:52.351377 Application: 60 <- Content-Length: 112
20:41:52.351377 Application: 60 <-
20:41:52.351377 Application: 60 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.351553 Application: 60 <- // Closed
20:41:52.351874 Application: 61 <- // Wrapped 3
20:41:52.352151 Application: 61 <- HTTP/1.1 200 OK
20:41:52.352151 Application: 61 <- Content-Type: application/json
20:41:52.352151 Application: 61 <- Content-Length: 112
20:41:52.352151 Application: 61 <-
20:41:52.352151 Application: 61 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.352398 Application: 61 <- // Closed
20:41:52.352703 Application: 62 <- // Wrapped 3
20:41:52.352902 Application: 62 <- HTTP/1.1 200 OK
20:41:52.352902 Application: 62 <- Content-Type: application/json
20:41:52.352902 Application: 62 <- Content-Length: 112
20:41:52.352902 Application: 62 <-
20:41:52.352902 Application: 62 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.353070 Application: 62 <- // Closed
20:41:52.353338 Application: 63 <- // Wrapped 3
20:41:52.353507 Application: 63 <- HTTP/1.1 200 OK
20:41:52.353507 Application: 63 <- Content-Type: application/json
20:41:52.353507 Application: 63 <- Content-Length: 112
20:41:52.353507 Application: 63 <-
20:41:52.353507 Application: 63 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.353680 Application: 63 <- // Closed
20:41:52.353931 Application: 64 <- // Wrapped 3
20:41:52.354118 Application: 64 <- HTTP/1.1 200 OK
20:41:52.354118 Application: 64 <- Content-Type: application/json
20:41:52.354118 Application: 64 <- Content-Length: 112
20:41:52.354118 Application: 64 <-
20:41:52.354118 Application: 64 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.354291 Application: 64 <- // Closed
20:41:52.354541 Application: 65 <- // Wrapped 3
20:41:52.354725 Application: 65 <- HTTP/1.1 200 OK
20:41:52.354725 Application: 65 <- Content-Type: application/json
20:41:52.354725 Application: 65 <- Content-Length: 112
20:41:52.354725 Application: 65 <-
20:41:52.354725 Application: 65 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.354908 Application: 65 <- // Closed
20:41:52.355165 Application: 66 <- // Wrapped 3
20:41:52.355355 Application: 66 <- HTTP/1.1 200 OK
20:41:52.355355 Application: 66 <- Content-Type: application/json
20:41:52.355355 Application: 66 <- Content-Length: 112
20:41:52.355355 Application: 66 <-
20:41:52.355355 Application: 66 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.355525 Application: 66 <- // Closed
20:41:52.355780 Application: 67 <- // Wrapped 3
20:41:52.355962 Application: 67 <- HTTP/1.1 200 OK
20:41:52.355962 Application: 67 <- Content-Type: application/json
20:41:52.355962 Application: 67 <- Content-Length: 112
20:41:52.355962 Application: 67 <-
20:41:52.355962 Application: 67 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.356131 Application: 67 <- // Closed
20:41:52.356402 Application: 68 <- // Wrapped 3
20:41:52.356584 Application: 68 <- HTTP/1.1 200 OK
20:41:52.356584 Application: 68 <- Content-Type: application/json
20:41:52.356584 Application: 68 <- Content-Length: 112
20:41:52.356584 Application: 68 <-
20:41:52.356584 Application: 68 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.356750 Application: 68 <- // Closed
20:41:52.356995 Application: 69 <- // Wrapped 3
20:41:52.357169 Application: 69 <- HTTP/1.1 200 OK
20:41:52.357169 Application: 69 <- Content-Type: application/json
20:41:52.357169 Application: 69 <- Content-Length: 112
20:41:52.357169 Application: 69 <-
20:41:52.357169 Application: 69 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.357341 Application: 69 <- // Closed
20:41:52.357603 Application: 70 <- // Wrapped 3
20:41:52.357778 Application: 70 <- HTTP/1.1 200 OK
20:41:52.357778 Application: 70 <- Content-Type: application/json
20:41:52.357778 Application: 70 <- Content-Length: 112
20:41:52.357778 Application: 70 <-
20:41:52.357778 Application: 70 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.357954 Application: 70 <- // Closed
20:41:52.358219 Application: 71 <- // Wrapped 3
20:41:52.358382 Application: 71 <- HTTP/1.1 200 OK
20:41:52.358382 Application: 71 <- Content-Type: application/json
20:41:52.358382 Application: 71 <- Content-Length: 112
20:41:52.358382 Application: 71 <-
20:41:52.358382 Application: 71 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.358546 Application: 71 <- // Closed
20:41:52.358791 Application: 72 <- // Wrapped 3
20:41:52.359019 Application: 72 <- HTTP/1.1 200 OK
20:41:52.359019 Application: 72 <- Content-Type: application/json
20:41:52.359019 Application: 72 <- Content-Length: 112
20:41:52.359019 Application: 72 <-
20:41:52.359019 Application: 72 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.359192 Application: 72 <- // Closed
20:41:52.359445 Application: 73 <- // Wrapped 3
20:41:52.359630 Application: 73 <- HTTP/1.1 200 OK
20:41:52.359630 Application: 73 <- Content-Type: application/json
20:41:52.359630 Application: 73 <- Content-Length: 112
20:41:52.359630 Application: 73 <-
20:41:52.359630 Application: 73 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.359800 Application: 73 <- // Closed
20:41:52.360050 Application: 74 <- // Wrapped 3
20:41:52.360225 Application: 74 <- HTTP/1.1 200 OK
20:41:52.360225 Application: 74 <- Content-Type: application/json
20:41:52.360225 Application: 74 <- Content-Length: 112
20:41:52.360225 Application: 74 <-
20:41:52.360225 Application: 74 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.360406 Application: 74 <- // Closed
20:41:52.360657 Application: 75 <- // Wrapped 3
20:41:52.360822 Application: 75 <- HTTP/1.1 200 OK
20:41:52.360822 Application: 75 <- Content-Type: application/json
20:41:52.360822 Application: 75 <- Content-Length: 112
20:41:52.360822 Application: 75 <-
20:41:52.360822 Application: 75 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.360987 Application: 75 <- // Closed
20:41:52.361229 Application: 76 <- // Wrapped 3
20:41:52.361400 Application: 76 <- HTTP/1.1 200 OK
20:41:52.361400 Application: 76 <- Content-Type: application/json
20:41:52.361400 Application: 76 <- Content-Length: 112
20:41:52.361400 Application: 76 <-
20:41:52.361400 Application: 76 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.361568 Application: 76 <- // Closed
20:41:52.361812 Application: 77 <- // Wrapped 3
20:41:52.361983 Application: 77 <- HTTP/1.1 200 OK
20:41:52.361983 Application: 77 <- Content-Type: application/json
20:41:52.361983 Application: 77 <- Content-Length: 112
20:41:52.361983 Application: 77 <-
20:41:52.361983 Application: 77 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.362154 Application: 77 <- // Closed
20:41:52.362397 Application: 78 <- // Wrapped 3
20:41:52.362573 Application: 78 <- HTTP/1.1 200 OK
20:41:52.362573 Application: 78 <- Content-Type: application/json
20:41:52.362573 Application: 78 <- Content-Length: 112
20:41:52.362573 Application: 78 <-
20:41:52.362573 Application: 78 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.362746 Application: 78 <- // Closed
20:41:52.362989 Application: 79 <- // Wrapped 3
20:41:52.363166 Application: 79 <- HTTP/1.1 200 OK
20:41:52.363166 Application: 79 <- Content-Type: application/json
20:41:52.363166 Application: 79 <- Content-Length: 112
20:41:52.363166 Application: 79 <-
20:41:52.363166 Application: 79 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.363472 Application: 79 <- // Closed
20:41:52.363838 Application: 80 <- // Wrapped 3
20:41:52.364069 Application: 80 <- HTTP/1.1 200 OK
20:41:52.364069 Application: 80 <- Content-Type: application/json
20:41:52.364069 Application: 80 <- Content-Length: 112
20:41:52.364069 Application: 80 <-
20:41:52.364069 Application: 80 <- { "ErrorNumber": 1035, "ErrorMessage": "Invalid operation", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.364253 Application: 80 <- // Closed
20:41:52.364643 Application: 81 <- // Wrapped 3
20:41:52.364859 Application: 81 <- HTTP/1.1 200 OK
20:41:52.364859 Application: 81 <- Content-Type: application/json
20:41:52.364859 Application: 81 <-
20:41:52.365028 Application: 81 <- { "Type": 2, "Rank": 3, "Value": [
20:41:52.365187 Application: 81 <- [
20:41:52.365370 Application: 81 <- [1, 2, 3]
20:41:52.365532 Application: 81 <- , [7, 8, 9]
20:41:52.365729 Application: 81 <- ]
20:41:52.365893 Application: 81 <- , [
20:41:52.366057 Application: 81 <- [4, 5, 6]
20:41:52.366222 Application: 81 <- , [10, 11, 65535]
20:41:52.366383 Application: 81 <- ]
20:41:52.366542 Application: 81 <- ], "ErrorNumber": 0, "ErrorMessage": "", "ClientTransactionID": 7, "ServerTransactionID": 9 }
20:41:52.366719 Application: 81 <- // Closed
20:41:52.367021 Application: 82 <- // Wrapped 3
20:41:52.367226 Application: 82 <- HTTP/1.1 200 OK
20:41:52.367226 Application: 82 <- Content-Type: application/imagebytes
20:41:52.367226 Application: 82 <- Content-Length: 68
20:41:52.367226 Application: 82 <-
20:41:52.367396 Application: 82 <- 
20:41:52.367558 Application: 82 <- 
20:41:52.367739 Application: 82 <- // Closed
Running agent_alpaca cache tests
  PASS imagearray_preserves_column_order
  PASS imagearray_rejects_invalid_raw_frames
  PASS imagearray_preserves_rgb48_planes_and_imagebytes_order
agent_alpaca cache tests: all tests passed
+ CFLAGS='-O1 -g -fsanitize=address,undefined,float-cast-overflow -fno-omit-frame-pointer'
+ LDFLAGS=-fsanitize=address,undefined,float-cast-overflow
+ python3 indigo_test/integration/test_agent_imager_numerics.py /private/tmp/raw-numerics-sanitize
+ UBSAN_OPTIONS=halt_on_error=1
+ ASAN_OPTIONS=detect_leaks=0
+ /private/tmp/raw-numerics-sanitize
Running Imager numerics and platesolver acceptance
  PASS focus minima regressions
  PASS exposure acceptance regressions
  PASS image consumers reject malformed frames
  PASS image consumers accept complete RAW formats
Imager numerics and platesolver acceptance: all tests passed
+ git diff --check

```
