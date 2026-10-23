# Repository review report

Task: 20261023065605-60d4-repository-audit (2026-10-23)
Reviewer: current Codex agent, with three owner-authorized read-only subagents; independence and scope are recorded in independent-review.md.
Revision: e0139d57ceb8bb6f4421efa8849c8541690e8b2c; initially clean. Initial audit preceded the two scoped PrimaLuce repairs below.
Verdict: partial

## Coverage and assurance

Repository-wide inventory and static assessment cover maintained component classes, with semantic inspection and executable checks detailed in the final coverage ledger. Exhaustive semantic inspection of every maintained driver/agent, every platform call path and all remaining analyzer candidates is not complete. This is a substantial partial audit, not a qualified completed full-repository checkpoint. Do not reset maintenance or treat successful regeneration as hardware/behavioral acceptance.

Explicit exclusions: vendor directories, generated driver outputs except source-driven reproducibility checks, build products, indigo_tests, obsoleted_drivers, task/finding archives and five owner-prohibited driver directories. Existing archived reports were not bulk-loaded. Generated/bundled assets and IDE user-state files are inventory-only. No physical hardware, installed services or user configuration changed.

## Confirmed findings

Twelve confirmed findings were allocated: nine initial findings below and three independent-review follow-ups (010 GP allocation failure, 011 GPIO OFF failure, 012 obsolete findings prose). The first two initial findings were blocking and are now fixed and archived after verification. Other defects have concrete repair follow-ups owned by the coordinator; classification is not risk acceptance or release certification. Confidence is high for each: direct compiler/runtime reproduction or exact current-source/path evidence. The two scoped repairs are recorded in verification.md; remaining findings are follow-ups.

## Existing findings

All 42 existing active records are preserved. FND-000047 reproduced after current framework rebuild and removal of sandbox networking restrictions. FND-000019/020 remain visible in current Andor init/shutdown paths; FND-000021/023/024 remain represented by RPi wait/copy/mmap paths. FND-000036 field-width construction remains in the property tool. FND-000046 and FND-000052 retain their existing evidence and unresolved follow-up. Other legacy observations were not silently considered reverified or closed.

## AI slop and candidate discipline

No claim about authorship. Concrete integration drift is present in the editor catalog/class/mapping and documentation instruction links. The generation corpus itself matches the unchanged generator: 117/117 definitions generate successfully with identical outputs. Clang warnings are candidates, not automatically findings: catalog-table bounds warning is constrained by the public dispatch date interval, FFT allocation sizeof warnings allocate the same total bytes, HTTP working-copy cleanup is present in the failure path, and GP allocation-failure was independently confirmed as follow-up 010; other warnings retain their stated limits. No safeguard or regression test was removed for style.

## Cadence and next action

No qualified full-audit checkpoint is emitted. The recurring full audit remains due. The owner authorized closing the verified repairs as a work task with audit=null, retaining this partial report and deferring remaining audit work until the next audit task before unrelated development/release. See handoff.md and verification.md.

## Canonical finding details

### FND-20261023065605-60d4-repository-audit-001: PrimaLuce shared archive recipe fails with the configured macOS libtool

Severity: high; blocking: true; confidence: high. Owner: coordinator. Disposition: fixed and archived; see verification.md.

Location/evidence: indigo_drivers/shared/primalucelib/Makefile:42; Makefile.inc:18,23; primalucelib-build.log.

The recipe hard-codes ar-style rcs after $(AR), but macOS config sets AR=/usr/bin/libtool and ARFLAGS=-static -o. A clean archive build fails with libtool usage and exit 2, preventing a clean focuser_primaluce build. Reproduced with BUILD_LIB redirected to a new /tmp directory. Next: coordinator assigns a portable archive-recipe repair using configured ARFLAGS, with clean macOS and GNU-ar verification; preserve atomic publication.

### FND-20261023065605-60d4-repository-audit-002: PrimaLuce reply lookup accepts shorter keys as the requested field

Severity: medium; blocking: true; confidence: high. Owner: coordinator. Disposition: fixed and archived; see verification.md.

Location/evidence: indigo_drivers/shared/primalucelib/primalucelib.c:355-373; primaluce-prefix-repro.c; primaluce-prefix-repro.log.

primalucelib_find_token compares only the received key length with strncmp. ABS_POS therefore satisfies a request for ABS_POS_STEP; the reproducer reports present=1 and returns 123 for an absent field. Units/position-field selection can be wrong despite successful missing-field guards. Next: coordinator assigns exact-length key matching with serial/HTTP parsed-reply regression cases, existing library/driver suites and affected version bookkeeping.

### FND-20261023065605-60d4-repository-audit-003: Travis commands reference missing test and Windows solution paths

Severity: medium; blocking: false; confidence: high. Owner: coordinator. Disposition: open.

Location/evidence: .travis.yml:54,56; static-inventory-checks.json.

CI invokes indigo_test/test_suite.sh, which is absent, and changes into indigo_windows before invoking a solution that now lives at the repository root. Those commands fail even when compilation otherwise succeeds. Next: coordinator assigns CI command repair to current portable test and solution entry points; validate path resolution without installing dependencies or probing hosted CI.

### FND-20261023065605-60d4-repository-audit-004: Driver editor omits the supported polaralign device class

Severity: medium; blocking: false; confidence: high. Owner: coordinator. Disposition: open.

Location/evidence: indigo_tools/driver_editor/driver_format.py:26; editor.py:63-66; editor-fresh-generator-tests.log.

DEVICES excludes polaralign although the generator and two checked-in definitions support it. Both polaralign definitions fail preview with no supported device. The fresh unchanged generator directly regenerates both successfully. Next: coordinator assigns editor class/catalog integration and corpus regression coverage; no generator change is needed.

### FND-20261023065605-60d4-repository-audit-005: Editor attribute catalog omits the generator-supported device id

Severity: low; blocking: false; confidence: high. Owner: coordinator. Disposition: open.

Location/evidence: indigo_tools/template.driver device attribute catalog; indigo_tools/driver_editor/test_editor.py:209-216; editor-fresh-generator-tests.log.

The template-derived device catalog does not offer id, accepted by parse_device_block and needed for named logical devices. Source-to-catalog contract test fails without any generator binary dependency. Next: coordinator assigns a template/editor catalog correction and reruns the existing attribute contract test; preserve existing DSL input.

### FND-20261023065605-60d4-repository-audit-006: Editor source mapping loses CCD properties on explicitly identified logical devices

Severity: low; blocking: false; confidence: high. Owner: coordinator. Disposition: open.

Location/evidence: indigo_tools/driver_editor/source_map.py:110-150; indigo_drivers/ccd_simulator/indigo_ccd_simulator.driver:898-906; editor-fresh-generator-tests.log.

Mapping uses device kind for generated-function prefixes and misses explicit logical-device identifiers used by the generated CCD simulator. Corpus test finds an empty generated range for inherited CCD_EXPOSURE. Next: coordinator assigns device-identity-aware mapping and tests that select all CCD logical devices; do not alter generator output merely for highlighting.

### FND-20261023065605-60d4-repository-audit-007: Driver development guides link a removed scoped instruction filename

Severity: low; blocking: false; confidence: high. Owner: coordinator. Disposition: open.

Location/evidence: indigo_docs/DRIVER_GENERATOR_BASICS.md:21,423; indigo_docs/DRIVER_TESTING_BASICS.md:18,86,367; static-inventory-checks.json.

Five developer-facing relative links still name indigo_drivers/AGENTS.override.md, which does not exist after consolidation to AGENTS.md. Readers cannot reach the instructions the guides designate as authoritative. Next: coordinator assigns a scoped link/label update and verifies all affected local destinations.

### FND-20261023065605-60d4-repository-audit-008: Checked-in client examples do not compile against current public headers

Severity: medium; blocking: false; confidence: high. Owner: coordinator. Disposition: open.

Location/evidence: indigo_examples/remote_server_client_mount.c:177; indigo_examples/client.c:49-85,130; analyzer-results.json.

The mount example, included by the examples all target, calls indigo_connect_server with four arguments although the header requires five. The older client.c also lacks standard-name declarations and passes integer descriptors to an adapter requiring indigo_uni_handle pointers. Exact compiler diagnostics are retained; this is distinct from FND-000010 unbounded connection handling. Next: coordinator assigns example API migration and isolated compile checks; preserve separate runtime-failure findings.

### FND-20261023065605-60d4-repository-audit-009: HTTP BLOB gzip uses an input-sized buffer and can serve an incomplete gzip stream

Severity: medium; blocking: false; confidence: high. Owner: coordinator. Disposition: open.

Location/evidence: indigo_libs/indigo_server_tcp.c:212-224; indigo_libs/indigo_uni_io.c:2675-2702; blob-compression-repro.c; blob-compression-repro.log.

The HTTP caller allocates only the uncompressed byte count for gzip output. The compressor accepts a non-final deflate result and reports partial output. For bytes 0..31 in a 32-byte buffer it returns 32 bytes; a zlib decoder returns -5 rather than Z_STREAM_END and recovers only 15/32 bytes. The HTTP caller would label and send the partial stream with 200 OK. Next: coordinator assigns bounded gzip-capacity/error-contract repair and HTTP incompressible-payload regression coverage; audit did not perform a network download.

## Component coverage ledger

| Component | Executed inspection | Remaining depth / limit |
| --- | --- | --- |
| Framework/protocol/I-O | Clang analyzer over all in-inventory C translation units in framework/catalog/tools/examples; source review of bus publication, config saving, JSON/XML parsing, raw utilities, serial exclusivity, HTTP BLOB handling and changed code since prior checkpoint. Current framework built; all native unit binaries run. | Independent GP/config/JSON/XML/FFT triage is in independent-review.md; remaining RAW/PSF and BLOB PUT length candidates require reachability/trust-contract review. No Windows execution. |
| Portable drivers and agents | All definitions inventoried by class; all 117 production definitions regenerated in isolated trees and all produced outputs match. Whole code inventory scanned for property/schema handling, waits, shell calls, capacity overrides and placeholders. Inspected current PrimaLuce transport/parser/finalizers and representative shared-device/agent paths. | Automated checks cover the whole definition corpus; behavioral/semantic driver and agent inspection is still selective. No full integration or hardware acceptance claim. |
| Linux/macOS/optional drivers | Platform sources inventoried; Windows XML references checked; Andor init/shutdown and RPi exposure/data boundary findings reinspected, macOS Atik transactional connection/lifecycle context read. | No physical SDK/device run, Linux or Windows host. Independent GPIO/Andor/RPi reconciliation is retained in independent-review.md; proposed historical closures still need focused execution evidence. |
| Server/browser UI | Current main/help guards and BLOB HTTP path read; server code covered through framework transport analysis; all six unminified JavaScript candidates pass syntax checks. | UI was not exercised interactively; no full end-to-end WebSocket/HTTP security audit. |
| Tools/examples/editor | All in-scope C files analyzed; 35 editor tests with fresh unchanged generator and loopback fixtures; missing header/API and editor model drift confirmed. | Five analyzer units unavailable under initial common include flags; two are genuine example API failures, other compile-context/optional CFITSIO gaps need classification. |
| Tests | All 17 native unit suites run independently after current framework build (261/262 cases), 175 Factory tests and seven build-system tests. Test registration, isolation and fail-fast behavior inspected. | Full integration corpus, sanitizer suite and hardware modes not run; existing flaky FND-000029 not reproduced here. |
| Windows/system integration/build | All inventoried vcxproj/filter/props XML parsed; Xcode project lint passes; source references checked; solution/CI paths and systemd execution identities read; clean PrimaLuce archive failure reproduced. | Independent check found zero missing references in 127 allowed active solution projects; stale standalone SDK paths are legacy candidates. Windows toolchain, install/service start and hosted CI were not exercised. |
| Documentation/workflow | Local Markdown target scan and all Python AST parse candidates; current root/scoped policy, provenance, task/maintenance/findings helpers and model/approval boundaries reviewed. | HOME.md appears written with wiki-relative paths and needs intended-renderer confirmation before treating all link candidates as defects. Six legacy Python 2 device scripts are syntax candidates, not automatically Python 3 defects. Full procedural consistency pass is unfinished. |

Initial candidate inventory: 2,102 tracked regular files after directory exclusions. It still includes inventory-only `.vcxproj.user` files, bundled Duktape/minified browser libraries and large generated RGB/catalogue data. Their presence is not a claim of semantic review or permission to alter them. The complete candidate list, code-signal candidates, syntax results, generation results and analyzer diagnostics are retained alongside this report. No single pattern match is treated as a verdict. The broad static pass and targeted semantic checks do not satisfy the repository rule that a sampled audit cannot reset the full-review checkpoint.

## False positives and unresolved questions

- The static date-table bounds warning alone is insufficient: its private helper is reached through a narrower public date interval. No finding allocated without a valid out-of-range public reproducer.
- FFT `calloc` element-type warnings have equivalent allocated byte totals; no buffer defect inferred from that warning.
- The HTTP working-copy leak warning has explicit failure cleanup; no leak finding allocated without a reproducer.
- GP solver allocation-failure is confirmed as finding 010; other numerical warnings remain candidates.
- Existing example stream/null and repair-tool issues already have canonical findings and were not duplicated.
- Credential logging/transport confidentiality and existing parser/utility memory-boundary findings need qualified security follow-up before any broader security-clearance statement; no risk acceptance inferred from existing open statuses.

## Reconciliation result

Canonical register retains all 42 pre-existing active records. Twelve findings were allocated in this task; 001/002 are fixed and archived, ten remain nonblocking follow-ups. Final counts are 52 active and 20 archived. A work completion record retains this partial report without resetting the audit checkpoint. See [verification.md](verification.md) and [quality-review.md](quality-review.md).

## Independent supplemental review

Three owner-authorized subagents completed scoped read-only review; their findings, source dispositions, independence and limits are in [independent-review.md](independent-review.md). Active Windows solution references are intact; stale standalone SDK paths and root HOME links are classified as legacy candidates. Historical GPIO and rotator-registration findings require updated closure evidence rather than copied legacy assumptions. No full semantic-audit claim is added.

## Owner-approved completion boundary

The owner explicitly approved closing the verified two repairs with this audit retained as partial, with the remaining full audit deferred to the next audit task before unrelated development/release. The audit verdict remains partial; task completion will cover accepted repair/record scope only, with kind=work and audit=null. No checkpoint reset or security risk acceptance.
