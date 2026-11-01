# Audit verification

Verdict: pass

Reviewed source: 42011325b8d3f9dcca2cc768594d42b385d4d481, clean at start; audit evidence and finding records are the only changes. Working directory for commands below: /Users/polakovic/Development/indigo. Host: macOS; exact model/effort unobservable. Separate component agents inspected current source; coordinator authored this reconciliation and independently reproduced FITS/MinGW defects. No product implementation was performed.

## Acceptance

- AC1: accepted partial scope under the owner closure amendment. Actual reviewed portions remain in repository-review.md and remaining gaps in unchecked-areas.md; no full-audit assertion.
- AC2: pass for current records. Existing active summaries consulted; duplicates retained under their canonical IDs. Prior server T07 targeted history read solely to assess incomplete framing repair.
- AC3: pass for registered observations, subject to independent evidence review; 14 new records plus reopened T07. No inferred risk acceptance.
- AC4: pass for accurate bounded evidence/closure. Safe checks below executed; editor baseline remains failed and platform/hardware validation unavailable/unperformed. No successful product result is invented.
- AC5: pass under owner amendment. Close with completion kindpartial-audit/auditnull, retain finding evidence and unchecked areas; no full/incremental checkpoint, product risk acceptance or master merge/push.

## Executed baseline and retests

| Command | Result and limits |
| --- | --- |
| git status --short --branch; git rev-parse HEAD origin/master | Clean master at source SHA; refreshed origin/master equals local master. |
| git fetch origin | Initial sandbox FETCH_HEAD write denied; escalated rerun exit 0. |
| ./.factory/scripts/maintenance status | Full review due; 64 uncovered work completions, last full date 2026-10-24. |
| ./.factory/scripts/check | Exit 0; whitespace, finding integrity, ADR index, Factory project boundaries. |
| ./.factory/scripts/test | Exit 0; 190 Factory unittest cases in 121.708 s. |
| ./.factory/scripts/security | Exit 0; local Factory metadata boundaries only, not product security certification. |
| make -n -C indigo_test test-unit | Read-only target inspection: 45 planned lines, no protected driver/shell-compliance builds. |
| make -C indigo_test test-unit | First sandbox run stopped at loader local bind assertion. Escalated rerun exit 0: 416 PASS cases in 34 completed suites. Uses current tree and existing build prerequisites; not a clean rebuild attestation of every linked object. |
| python3 -B -m unittest discover -s indigo_tools/driver_editor -p 'test_*.py' | Sandbox: 37 cases, 2 failures/5 errors. Escalated: 37 cases, 2 failures/2 errors, all remaining failures match existing polaralign/id/discover records; no duplicates. |
| make -C indigo_test test-driver-test-recording test-tool-input-safety test-buildsystem | Exit 0; 25 recording, 6 crop, 7 build-system Python cases and property-tool C input-safety cases. |
| make -n -C indigo_windows/gcc rebuild | Exit 2, reproduced missing build target; dry run did not clean anything. |

Native logs were captured at /tmp/indigo-audit-test-unit.log and /tmp/indigo-audit-test-unit-escalated.log; editor logs at /tmp/indigo-audit-editor.log and /tmp/indigo-audit-editor-escalated.log; tools log at /tmp/indigo-audit-tools-tests.log. These are temporary supporting logs, not permanent evidence links; outcomes and relevant diagnostics are retained here. Expected service-discovery/timer error-injection diagnostics occurred inside passing cases.

## Defect demonstrations

FITS: extracted unchanged source from `#define FITS_RECORD` through `fits_header`, prefixed standard includes, added synthetic 5,760-byte header with BITPIX/NAXIS/NAXIS1/NAXIS2/END cards. Exact compiler argument vector: `clang -fsanitize=undefined,float-cast-overflow -fno-sanitize-recover=all proof.c -o proof`. Control `proof 8 8` exits 0 and accepts image. `proof 8 nan`, `proof nan 8`, `proof 1e100 8` each terminate with UBSan float-to-int overflow. Temporary harness path: /var/folders/t0/tyrwlgx15fb1z5cq_lv6ddgr0000gn/T/indigo-fits-audit-o544cux0/proof.c. This demonstrates malformed file input; it does not test framework property validation.

MinGW graph: copied the unchanged gcc/Makefile to an isolated temporary directory, provided a synthetic C input and compiler script. DLL compiler sleeps 0.5 s before creating its import library; executable compiler exits 17 if that library does not exist. Argument vector: `make -j2 all INDIGO_OBJECTS= MDNS_OBJECTS= CC=FAKE LINKER=FAKE INDIGO_BASE=FIXTURE`. Actual make exit 2, diagnostic `MISSING_IMPORT_LIBRARY build/libindigo_client.lib`; confirms dependency ordering, not MinGW code generation.

Gaia: exact parse_gaia_catalog extracted into a small temporary C harness with two-star storage and repository public headers. Normal CSV `10,20,10,0,0` completes; `inf,20,10,0,0` exceeds a two-second subprocess timeout. Separately, the exact constructed ls command with synthetic directory `/tmp/$(printf INDIGO_AUDIT_COMMAND_SUBSTITUTION >&2)` prints the harmless marker before listing failure. No hostile command, third-party service, large catalog or index creation was used. Agent harness was automatically removed; code path and observed outcomes are retained in repository-review.md.

Native-filter and dome-helper minimal harnesses are retained beside this report. JSON harness is also retained; its exit 0 convention means the expected defect reproduced while controls passed, not that parser correctness passed. JSON compilation uses current indigo_json.c and a capture-only dispatch replacement with existing library support. Native-filter unfiltered exit 1 versus unmatched-filter exit 0 demonstrates false success. Dome helper returns 0 for an empty cache while logging missing azimuth. No hardware initialization is called.

Core supporting demonstrations against the existing dylib: a socketpair read after a single G remained blocked past a nominal first-byte timeout until the rest of the line arrived; a padded 1x1 Bayer image changed RGB output solely with trailing padding. The existing library was not rebuilt/revision-attested, so these supplement source contracts rather than replacing current-source inspection. SER/Trutek/WeMacro/ASI/leak findings are source-contract observations; Windows/real hardware/Valgrind were not executed.

## Found defects and remediation state

See repository-review.md and canonical records 001–014 plus T07. The discovery pass made no product edits. Subsequent separate repair tasks closed selected11findings; canonical records retain exact repair evidence. No assertion or test was weakened by this closure. The cache-lifetime stress example, server catalog capacity question and filter-format forwarding lead remain investigation questions; no confirmed production-path claim or finding was manufactured.

## Final test summary

Factory: 190 passed. Native unit: 416 passed across 34 suites. Editor: 37 executed, final 2 failures and 2 errors matching existing findings. Additional tools regression groups passed as recorded above. Driver simulator acceptance runs: 0; physical hardware tests: 0; Linux builds: 0; Windows builds: 0. These categories are not combined into a misleading single pass count.

## Retained-harness rerun and integrity

Coordinator reran the retained minimal harnesses after report drafting:

- `clang -O1 -I. .factory/tasks/20261101193246-1248-periodic-repository-audit/native-filter-repro.c -o /tmp/indigo-audit-native-filter`: compile exit 0. Unfiltered `/tmp/indigo-audit-native-filter`: exit 1, deliberately failing case. `INDIGO_TEST_CASE_FILTER=misspelled-case /tmp/indigo-audit-native-filter`: exit 0 and all tests passed, no case executed.
- `clang -O1 -DINDIGO_MACOS -I. -Iindigo_libs .factory/tasks/20261101193246-1248-periodic-repository-audit/dome-helper-repro.c -Lbuild/lib -lindigo -Wl,-rpath,/Users/polakovic/Development/indigo/build/lib -o /tmp/indigo-audit-dome-helper && /tmp/indigo-audit-dome-helper`: exit 0 while logging missing number item DOME_HORIZONTAL_COORDINATES.AZ.
- `clang -DINDIGO_MACOS -Dindigo_change_property=audit_capture_change -Iindigo_libs indigo_libs/indigo_json.c .factory/tasks/20261101193246-1248-periodic-repository-audit/json-positive-exponent-repro.c -Lbuild/lib -lindigo -Wl,-rpath,/Users/polakovic/Development/indigo/build/lib -o /tmp/indigo-audit-json && /tmp/indigo-audit-json`: exit 0 under expected-defect convention; values 100→100, 1e-2→0.01, 1e+2→1, exactly one captured change each.
- `./.factory/scripts/findings check`: exit 0, 46 active and 209 archived after 14 additions and one reopen.
- `./.factory/scripts/check` and `./.factory/scripts/security`: repeated after canonical registration, both exit 0.
- `./.factory/scripts/maintenance status`: still 64 uncovered work tasks and full_review_due true; no completion emitted.

The probes are evidence of defects, not passing correctness tests. Their binaries and logs remain outside the repository. No source file or Xcode project was changed.

## Final partial-closure acceptance
Owner direct instruction on2026-11-01: change the rules as proposed and close this audit as partial, preserving findings and unchecked areas. This passing verdict concerns accurate evidence retention and administrative closure only; repository-review.md remains Verdict: partial. Original failed editor results and security findings remain explicit, not waived or converted to successes. Selected11origin findings are closed/fixed;004/005/011 andT07stayopen. No application/image-recognition rerun for closure. Final configured metadata, finding/maintenance and committed-range results will be retained with closure evidence; no original measurement is rerun or rewritten.

Final closure metadata checks from /Users/polakovic/Development/indigo after policy integration: .factory/scripts/check and .factory/scripts/security exit0; archive-task emittedpartial-audit/auditnull; findingscheck afterarchive exit0,36active/220archived. No product tests rerun. Independent closure review PASS. Final committed process/maintenance results retained externally without self-referential report commits.
