# Verification

Verdict: pass

## Candidate and independence

Working directory: /Users/polakovic/Development/indigo. Base and included local master: ce3a972818fb2875a7a901678c5c68e406f4c703. All results below concern the combined uncommitted task candidate unless explicitly marked baseline/worker. Root implemented001, integrated002/003/010/011/042 from isolated workers, and ran combined verification. Runtime verification is not independent of implementation; independent source reviews are recorded in quality-review.md and security.md. macOS 27.0.1 arm64, Apple clang, universal x86_64/arm64 normal builds, Node 26.7.0; Linux/Windows/hardware execution unavailable.

## Baseline and corrected drafts

Original `make -C indigo_test build/unit/test_bus_property && indigo_test/build/unit/test_bus_property`: exit 0,11/11. Added regressions on old implementation failed destination capacity assertions (expected2/got8, expected8/got1, expected3/got1); run terminated before completion. This is expected defect evidence, not a passing check, and no further old-code fault runs were made. The original property baseline did not cover reuse. A draft unconditional clear was rejected in independent review because WO BLOB copies borrow their payload; corrected to text-only clear and added a positive borrowed-payload regression.

Initial `make -C indigo_test test-unit` failed 9 uni-I/O cases when sandbox blocked local sockets and temporary-device access. The same suite rerun with permitted local access passed; no tests were weakened. Initial `leaks --atExit -- indigo_test/build/unit/test_bus_property` could not obtain the task port in the sandbox (exit 255), then passed with permitted local inspection.

## Final combined commands and results

| Command (from working directory above) | Result |
| --- | --- |
| `make -C indigo_libs libindigo` | exit 0, core and client libraries rebuilt, no off-limits driver build |
| `make -C indigo_server indigo_server.o` | exit 0, final server source including raw service text compiles |
| `make -C indigo_test test-unit build/integration/test_ccd_file_names build/integration/test_bus_lifecycle` | exit 0, complete unit target passed; new core 4/4 and property 17/17 included |
| `indigo_test/build/integration/test_ccd_file_names` | exit 0,1/1 public base-CCD fixture with many contract assertions |
| `indigo_test/build/integration/test_bus_lifecycle` | exit 0,2/2 |
| sanitizer compilation/link commands in sanitizer-commands.log, followed by its three binaries | exit 0 each: property 17/17, core 4/4, CCD 1/1; no ASan/UBSan diagnostics |
| `leaks --atExit -- indigo_test/build/unit/test_bus_property` | exit 0,0 leaks/0 bytes on corrected001 draft; other final core changes do not alter its copy lifecycle |
| `make -C indigo_test build/security/test_transport_http_security` | exit 0, ASan/UBSan-instrumented HTTP core objects |
| `indigo_test/build/security/test_transport_http_security` | exit 0,15/15, isolated local loopback fixture with permitted network access; see http-tests.log |
| `python3 tools/run_driver_test.py agent_alpaca` | exit 0,1/1, non-interactive hardware-free run; recorded 3.0.0.16 in README Testing and TEST_SUMMARY |
| `make -C indigo_test test-mount-manual-motion test-imager-download` | exit 0,22/22 motion plus 18/18 existing download cases |
| `./.factory/scripts/check` | exit 0 before final bookkeeping; repeated for final candidate |
| `./.factory/scripts/test` | exit 0, 188 tests passed in 105.262s |
| `./.factory/scripts/security` | exit 0, local security boundaries valid |

The sanitizer build instruments all seven changed core compilation units and each new/affected test, linked to the rebuilt native dependency library. Dependencies outside those units are uninstrumented. HTTP target separately instruments bus/XML/JSON/HTTP/unified-I/O under its configured small synthetic budgets. Existing compiler deprecation warnings are unrelated to these changes.

## Acceptance mapping

AC-001: six new property cases cover small/larger real destination capacity, repeated copying, zero source count, inactive long text slots, long-to-long/short replacement, type replacement, self-copy and borrowed WO BLOB payload. AC-002: static/file metadata copied, active GET/PUT callbacks drained, duplicate removers and remove-all/single removal serialized, unrelated registry additions/lookups progress and new registrations survive remove-all. Final native Alpaca build/cache record passes; shutdown ordering independently source-reviewed, not a full driver shutdown-concurrency certification.

AC-003/010: core four-case fixture covers service port suffix/adapter bounds, unique-name rejection without mutation, literal percent-bearing wrappers/snoop values and filter proxy names/groups. CCD fixture verifies baud/directory suffix rejection and percent-bearing FITS data. Server service initialization and default-directory raw use are source-reviewed and compiled. AC-011: public process_image fixture covers ordinary/XXX/XXXX sequence behavior, S/I and repeated I, object/filter literal %, 58-character FITS values, %J, numeric/frame/temperature/resolution/binning tokens, unknown/trailing %, expanded overflow, and complete511-character success versus512-character rejection. Tests release bus/device fixtures on failed assertions.

AC-042: VM tests verify all four actual HTML event bindings, outside release, cancellation/capture loss, blur/hidden/pagehide, unrelated pointers, duplicate releases/presses, refused starts/capture failures and explicit abort preservation. No real browser pointer-routing or physical mount movement was performed.

## Delivery gates and limits

Only local synthetic resources were used. No hardware/ConformU/whole-server certification, Linux/Windows runtime, remote synchronization/publication or master integration is claimed. Full broad driver/integration suite was not run because it includes unrelated/off-limits drivers; relevant core, CCD, bus, HTTP and Alpaca scopes were run. Post-commit configured checks and process-check are required and reported externally with final SHA. `make -C indigo_test test-clean` passed and removed generated test artifacts before final delivery. `git merge --ff-only master` returned Already up to date, confirming the included local target before final review.
