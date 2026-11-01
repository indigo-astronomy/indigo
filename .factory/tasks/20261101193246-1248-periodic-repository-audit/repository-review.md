# Periodic repository audit

- Task: 20261101193246-1248-periodic-repository-audit
- Date: 2026-11-01 (Europe/Bratislava).
- Reviewed revision: 42011325b8d3f9dcca2cc768594d42b385d4d481; initial working tree clean. Only audit metadata/evidence has been written.
- Verdict: partial — confirmed changes required; no full-audit checkpoint reset.
- Independence: separate read-only component agents; coordinator reconciles records and independently checks selected source/proofs. Exact host model/effort unknown; no provider/model switch or external review invocation.

## Scope and method

Repository-wide discovery used Git inventories with archive, dependency, build, obsolete, shell-compliance and five protected-driver exclusions. The 2,533 discovery paths include documentation/assets/build files and retained generated outputs: this is an inventory, not a claim of semantic review. The changed discovery inventory contains 532 paths since the preceding reviewed revision. Semantic inspection and tests are described below and in verification.md. No line-count or search result certifies coverage.

Core/server pass: TCP resource/upload/worker handling; token/session/SER/TIFF/DSLR helpers; timer ownership/scheduling through line 630; FITS/Base64 conversion; bus BLOB/resize/copy/detach paths; unified I/O framing/admission/URL/close/map/compression; CCD naming/preview/recording; stretch/RAW/star-refinement; selected client/discovery/platesolver/UI changes. Continuation added complete JSON tokenization/handlers/serialization and three adapter construction/destruction files; XML handlers/entity/Base64/cleanup paths, AO/AUX/guider/polaralign implementations, GPS/focuser/rotator/dome lifecycle/change paths, filter selection/cache, server management queues and selected astronomy/polynomial entrypoints. Remaining adapter reads and UI templates are incomplete.

Tools/tests pass: recording orchestrator and summary generation, solver comparison, test runner/Makefile selection and dependency contracts, loader and parallel-runner harnesses, tool-input and crop tests, solver/indexer parsing, selected CLI utilities, editor backend, example shared image/process/remote helpers. Continuation added complete editor driver_format/source_map/static app.js, all six C examples/shared headers, remaining conversion/repair/metadata utilities, simulator/hardware cache/recording/wait/hotplug helper architecture, MountSim/leak orchestration, and selected packaging/identity/FTDI/UT1/Wi-Fi scripts. Individual scenarios, generator/property-tool internals, large RPi scripts, fxload and minifier remain incomplete.

Driver pass: current solver FITS decoder and Trutek wheel state transitions independently inspected by coordinator. Additional driver reviewer inventories 453 source/input files (~211k lines, including simulators), excluding generated C with matching .driver sources. Current deep inspection covers Linux GPIO/ASIAIR, libcamera acquisition, macOS BT lifecycle/Atik2 excerpts, optional Andor/RPi lifecycle excerpts. This is explicitly not complete driver semantic coverage.

Coordinator pass: current Factory startup/cadence/task/Git/findings/process/archive rules, provenance and boundary validation; root and shared makefiles; Windows SDK makefile and client project, installer/service configuration; current documentation indexes, source-reference/link checks and relevant build/solver/property documentation. Missing link candidates in root HOME.md were not automatically treated as defects without establishing its consumer; removed AGENTS.override.md links duplicate existing finding 60d4-007. Simulator cache retained-pointer lifetime and server driver-catalog capacity are investigation questions without confirmed consumer/reproduction; no new finding is allocated. Windows standalone project has missing legacy SDK references, but its current integration/use remains an investigation question, not a registered defect.

## Confirmed findings

All new records below are open and nonblocking follow-up records owned by coordinator; nonblocking classification does not accept security risk or approve release. Security-specific conclusions are in security.md. Functional records distinguish runtime demonstrations from source-contract evidence.

### FND-20261101193246-1248-periodic-repository-audit-001 — Gaia indexer evaluates shell substitutions in directory arguments

Category: security; severity: medium; confidence: high for the stated evidence; blocking: false.

Location: `indigo_tools/indigo_solver_indexer.c:210–211`.

The -g directory is inserted into a double-quoted ls command passed to popen. Double quotes preserve command substitution. An exact-command synthetic marker printed before listing failed; this is distinct from metadata-extractor FND-000040. A directory copied from an untrusted source can execute with the invoking user privileges. Enumerate directories without a shell and check enumeration errors.

Owner/next action: coordinator assigns a focused repair and regression under the existing workflow. No product fix or risk acceptance is claimed.

### FND-20261101193246-1248-periodic-repository-audit-002 — Gaia catalog infinite right ascension never completes normalization

Category: functional; severity: medium; confidence: high for the stated evidence; blocking: false.

Location: `indigo_tools/indigo_solver_indexer.c:232–265`.

strtod accepts inf and no finite-value check precedes repeated subtraction of 2π. Exact extracted parser with CSV 10,20,10,0,0 completes; inf,20,10,0,0 exceeds a two-second subprocess timeout. Validate conversion, finite propagated coordinates, magnitude and proper motion before bounded normalization.

Owner/next action: coordinator assigns a focused repair and regression under the existing workflow. No product fix or risk acceptance is claimed.

### FND-20261101193246-1248-periodic-repository-audit-003 — Unmatched native test filter reports all tests passed with no execution

Category: quality; severity: low; confidence: high for the stated evidence; blocking: false.

Location: `indigo_test/test_runner.h:258–292`.

A temporary harness including the unchanged header registered one failing case. Unfiltered invocation exits 1; INDIGO_TEST_CASE_FILTER=misspelled-case executes none, prints all tests passed and exits 0. Direct binaries/Makefile targets can falsely appear green. run_driver_test.py already rejects filtered recording. Preserve cross-suite filtering with explicit skips and an aggregate no-tests failure.

Owner/next action: coordinator assigns a focused repair and regression under the existing workflow. No product fix or risk acceptance is claimed.

### FND-20261101193246-1248-periodic-repository-audit-004 — MinGW property tool link races the import library build

Category: functional; severity: medium; confidence: high for the stated evidence; blocking: false.

Location: `indigo_windows/gcc/Makefile:59,109–110`.

The executable depends only on its C source while linking libindigo_client.lib produced by the sibling DLL prerequisite of all. An isolated fixture used the unchanged Makefile and a synthetic delayed linker under make -j2: executable link started before the import library existed and make exited 2. Add an explicit dependency and correct directory ordering; test serial, parallel and direct executable targets.

Owner/next action: coordinator assigns a focused repair and regression under the existing workflow. No product fix or risk acceptance is claimed.

### FND-20261101193246-1248-periodic-repository-audit-005 — MinGW rebuild target depends on nonexistent build target

Category: functional; severity: low; confidence: high for the stated evidence; blocking: false.

Location: `indigo_windows/gcc/Makefile:151`.

make -n -C indigo_windows/gcc rebuild exits 2: No rule to make target build, needed by rebuild. A real invocation first performs clean then fails. Point rebuild to the actual all target with safe ordering. No MinGW compiler or Windows runtime was executed.

Owner/next action: coordinator assigns a focused repair and regression under the existing workflow. No product fix or risk acceptance is claimed.

### FND-20261101193246-1248-periodic-repository-audit-006 — Solver FITS dimensions and BITPIX convert nonfinite values to int

Category: functional; severity: medium; confidence: high for the stated evidence; blocking: false.

Location: `indigo_drivers/agent_solver/indigo_solver.c:262–271`.

fits_card_value accepts strtod nan; dimension comparisons with NaN all evaluate false. BITPIX is cast before validating supported values. A current-source extracted header parser compiled with undefined/float-cast-overflow sanitizers accepts a valid 8x8 control, but NAXIS1=nan, BITPIX=nan and BITPIX=1e100 each abort at the conversion. Require finite integer header fields and validate BITPIX before conversion; reject malformed FITS without changing framework property validation.

Owner/next action: coordinator assigns a focused repair and regression under the existing workflow. No product fix or risk acceptance is claimed.

### FND-20261101193246-1248-periodic-repository-audit-007 — Trutek wheel reports success at a slot different from its target

Category: functional; severity: medium; confidence: high for the stated evidence; blocking: false.

Location: `indigo_drivers/wheel_trutek/indigo_wheel_trutek.driver:84–95,112–118`.

Both movement acknowledgement and finalizer treat any nonzero slot as OK. Query decoding accepts an ASCII slot without range validation. For requested slot 3, a well-framed stopped status at slot 2 publishes OK with value 2 and target 3. This can let imaging proceed through the wrong filter. Require an in-range actual slot equal to the target for OK and report stopped-elsewhere as ALERT; test wrong-slot acknowledgement and completion. Source-contract confirmation only, no hardware run.

Owner/next action: coordinator assigns a focused repair and regression under the existing workflow. No product fix or risk acceptance is claimed.

### FND-20261101193246-1248-periodic-repository-audit-008 — SER timestamp arithmetic narrows to 32-bit long on Windows

Category: functional; severity: medium; confidence: high for the stated evidence; blocking: false.

Location: `indigo_libs/indigo_ser.c:100–103`.

Timestamp arithmetic uses long, which is 32 bits on Windows LLP64: the cast first narrows epoch seconds, then multiplication by 10000000L can overflow signed long. The timezone multiplication also uses 32-bit long; the uint64_t serializer cannot repair prior overflow. Synthetic Unix seconds 1793563200 require 639291636000000000 ticks, beyond 2147483647. Use explicit 64-bit epoch/tick and signed timezone arithmetic and assert serialized timestamps. Windows execution was unavailable.

Owner/next action: coordinator assigns a focused repair and regression under the existing workflow. No product fix or risk acceptance is claimed.

### FND-20261101193246-1248-periodic-repository-audit-009 — Bayer interpolation reads outside images narrower than two pixels

Category: functional; severity: medium; confidence: high for the stated evidence; blocking: false.

Location: `indigo_libs/indigo_stretch.cpp:44–51; indigo_libs/indigo_ccd_driver.c:1079–1123`.

Public Bayer helpers have no documented minimum dimension or guard. For RGGB 1x1, interpolation reads raw[1] and raw[2]. A padded demonstration through an existing unrebuilt dylib changes output from [10,0,0] to [10,77,77] and [10,200,200] solely by changing bytes after the one logical pixel. Current source confirms those reads; library provenance is not attested. Define narrow-image handling in ordinary/stretched paths and test 1x1,1xN,Nx1. Driver-specific remote reachability is unestablished; solver preview uses no Bayer pattern.

Owner/next action: coordinator assigns a focused repair and regression under the existing workflow. No product fix or risk acceptance is claimed.

### FND-20261101193246-1248-periodic-repository-audit-010 — JSON number parser silently misreads valid positive exponents

Category: functional; severity: medium; confidence: high for the stated evidence; blocking: false.

Location: `indigo_libs/indigo_json.c:524–535 NUMBER_VALUE_STATE`.

The numeric token state accepts e/E and minus but omits plus. A current-source parser linked with a capture-only indigo_change_property substitute parsed 100 as 100 and 1e-2 as 0.01, but valid JSON 1e+2 as 1. This silently changes requested values. Accept the complete JSON numeric grammar and reject malformed/truncated tokens; add both exponent-sign regressions.

Owner/next action: coordinator assigns a focused repair and regression under the existing workflow. No product fix or risk acceptance is claimed.

### FND-20261101193246-1248-periodic-repository-audit-011 — WeMacro Bluetooth unexpected disconnect prevents reconnection

Category: functional; severity: medium; confidence: high for the stated evidence; blocking: false.

Location: `indigo_mac_drivers/focuser_wemacro_bt/indigo_focuser_wemacro_bt.m:191–202,371–375`.

Unexpected didDisconnectPeripheral marks the public connection disconnected without resetting device_count. After a successful connection count remains 1; the next connect increments it to 2 and skips the only delegate connect call guarded by count++ == 0. Reset/remove the single-device refcount coherently and test connected→unexpected-loss→reconnect callbacks. Source-contract evidence; no Bluetooth hardware executed.

Owner/next action: coordinator assigns a focused repair and regression under the existing workflow. No product fix or risk acceptance is claimed.

### FND-20261101193246-1248-periodic-repository-audit-012 — ASI rotator reports OK when motion stops short of requested angle

Category: functional; severity: medium; confidence: high for stated source contract; blocking: false.

Location: `indigo_drivers/rotator_asi/indigo_rotator_asi.driver:265–267,294–305`.

A stopped, non-aborted BUSY move is unconditionally completed OK after reading current angle, without comparing it to retained target_position. Successful CAAMoveTo(90), then CAAIsMoving=false and CAAGetDegree=30 reports OK at 30 with target 90. Require arrival within SDK-precision tolerance or ALERT; preserve manual-control and abort semantics. Current fake-SDK recovery test at integration/test_rotator_asi_sdk.c:444–446 refuses an overlapping request to 60 as intended, but then accepts completion at 10 for the previously accepted retained target 50. Independently review that completion expectation; preserve correct overlap rejection. Source-contract evidence only.

Owner/next action: coordinator assigns a focused repair and regression. No product fix or risk acceptance is claimed.

### FND-20261101193246-1248-periodic-repository-audit-013 — Dome hardware arrival assertion accepts missing or NaN azimuth

Category: quality; severity: medium; confidence: high for stated source contract; blocking: false.

Location: `indigo_test/hardware/dome_hw_common.h:30–38,90–96`.

Missing numeric data leaves current=NAN. The comparison distance > tolerance is false for NaN and dome_hw_at returns true. A temporary unchanged-header harness with empty cache and call dome_hw_at(0,90,0.1) exits 0 despite printing missing number item DOME_HORIZONTAL_COORDINATES.AZ. Actual callers include test_dome_baader_hw.c:127, test_dome_nexdome_hw.c:136 and test_dome_nexdome3_hw.c:142. Require successful item retrieval and finite inputs before accepting arrival. No hardware was operated.

Owner/next action: coordinator assigns a focused repair and regression. No product fix or risk acceptance is claimed.

### FND-20261101193246-1248-periodic-repository-audit-014 — Server leak check ignores failures in the exercised operations

Category: quality; severity: medium; confidence: high for stated source contract; blocking: false.

Location: `indigo_test/leak/run_server_leak_check.py:86–89,108–125,195–218`.

Server.tool ignores subprocess exit status; exercise ignores connection, operation and disconnect outcomes and skips absent properties. Startup accepts any nonempty device set after retry exhaustion. A clean Valgrind report can therefore exit 0 without exercising the intended paths. Require expected devices and successful state transitions, positive cycles and guaranteed cleanup on failure. Confirmed from complete source flow; no Valgrind execution or observed clean false-positive report is claimed.

Owner/next action: coordinator assigns a focused repair and regression. No product fix or risk acceptance is claimed.

## Previously recorded findings

Reopen FND-20261023112118-8e5e-server-security-audit-007 for the missing HTTP framing/progress portion of its prescribed repair. The admission cap and initial five-second detection fixes remain present. A client supplying G reaches a line reader with unlimited interbyte timeout and can retain one of 128 worker slots indefinitely; request/header loops also lack an overall deadline. Current locations: indigo_server_tcp.c:291,311,327; indigo_uni_io.h:306; indigo_uni_io.c:2056–2057,2123. An in-memory socketpair demonstration against an existing unrebuilt library supports the source contract: blocked after 400 ms with a nominal 100 ms initial timeout until the remainder of a line arrived. No listener/flood or third-party testing occurred. Add bounded complete request/header framing tests. The earlier closure is not treated as complete coverage of this remaining contract.

Existing blank-ACL-line underflow (server audit-010), stale initialized registry flag (solver-exclusive-init-001), editor polaralign/id/discover findings, and other current active records remain unchanged. No duplicate IDs or unsupported closures.

## AI slop inspection

No new finding was based on suspected authorship or generic style. False success in the test runner is a concrete verification defect, not an authorship claim. No useful safeguards/tests were removed, and no speculative cleanup quota was applied.

## Limits, cadence and handoff

The current pass is partial. Remaining semantic gaps include complete parser/adapter/base-driver/math/browser coverage, most driver families, remaining tests/hardware harnesses/tools/scripts and documentation-to-source contracts. Tests on this macOS host do not establish Linux/Windows/hardware correctness, and existing library execution alone does not attest a rebuild of the reviewed commit. Component inventories and earlier reports cannot substitute for missing current-source inspection.

New security observations remain open without owner risk acceptance; existing editor failures and incomplete full coverage prevent a passing completion claim. Do not archive this task, emit a qualified full-audit completion or reset cadence. Continue the same task/branch; no duplicate task is needed for role changes. See verification.md for actual checks and handoff.md for current assignment. Canonical finding records preserve ownership and evidence; no reports are pruned.
