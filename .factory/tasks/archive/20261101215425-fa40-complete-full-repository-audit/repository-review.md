# Full maintained-repository source-contract audit

- Task: 20261101215425-fa40-complete-full-repository-audit
- Reviewed source: `f3bb131e4f4b3b9460c85cec1f57c405c2f721d3`; clean source at start, audit metadata only thereafter.
- Reviewed on: 2026-11-01, owner timezone Europe/Bratislava.
- Scope: full maintained first-party source, test, documentation, configuration and workflow contracts, using current inspection plus explicitly attributable still-applicable semantic evidence.
- Verdict: pass

This verdict means the declared full source-contract inspection is complete. **Product changes are required:** the24 new findings below remain open with accountable follow-up. This is not risk acceptance, a release approval, a claim that every application test passes, or hardware/Linux/Windows runtime certification. No product source was changed or unsafe input executed.

## Coverage and method
The maintained inventory accounts for2295 of9346 tracked source-base paths;7051 paths have explicit archive/dependency/generated/build/obsolete/protected/shell-compliance exclusions. Counts and hashes are bookkeeping/identity evidence, not semantic review by themselves. Component reports identify actual inspected lifecycle, ownership, parser, protocol, numeric, failure, test-assertion and document/build contracts. Current changed/new implementations were inspected; unchanged implementations reuse completed semantic reports from the previous qualified full audit only after identity and affected-consumer reconciliation. The prior accepted partial audit remains partial and unchanged.

| Area | Source-contract evidence |
| --- | --- |
| Imaging, AUX, optional/Linux drivers and SDK boundaries | [Driver A](components/drivers-a.md) and its per-path ledger; new ALTO/libcamera, full Atik2/RPi/Andor gaps, shared PrimaLuce and paired QSI/QHY/libcamera test contracts. |
| Motion, wheels, domes, Bluetooth and system Alpaca | [Driver B](components/drivers-b.md) and its per-path ledger; changed finalizers/targets/transport/SDK ownership plus identity-matched prior complete family review. |
| PTP/Pentax camera paths and tests | [PTP/Pentax](components/ptp-pentax.md): vendor protocol, event/capture/download/abort/USB/hotplug and five paired simulated/hardware test-source contracts; no device execution. |
| Native platesolver | [Native solver](components/native-platesolver.md): all source/header formats, index maps, matching/fitting/limits, agent download/cache/abort/registration and four current test contracts. |
| Tools, generator, property CLI, scripts, tests and examples | [Tools/tests](components/tools-tests.md):466 paths reconciled, current changed scenarios/assertions/harnesses and attributable retained grouped-contract review. |
| Image framework | [Core images](components/core-image.md): six primary files, affected consumers and143 executable definitions reconciled;34 new/changed bodies inspected and109 retained exact-body semantic reviews. |
| Remaining framework/math/agents/browser/Factory/docs/build/assets | [Coordinator closure](components/coordinator.md) and1132-path ledger, including general A/B support-document/build delegation, native three VS support files and eleven Factory test files. |
| Independent completion/security | [Coverage review](components/final-coverage-review.md) and [security review](components/security-review.md), distinct from component authors and final coordinator writing. |

The independent verifier reconciled the union against every maintained inventory path, with zero missing mandatory paths or source-identity mismatch. Six existing iOptron aliases have explicit link/target identity treatment; two additional generated-main rows add no false semantic credit. Earlier A PTP/Pentax delegation labels are superseded by completed PTP/Pentax evidence. General support/build/native VS metadata is explicitly coordinator-owned and complete.

Previous partial unchecked groups now have source coverage: remaining parser/adapter/base/timer/math by coordinator/core-image; browser templates and control lifecycles by coordinator/current deltas and qualified prior review; driver families by A/B/PTP/native; individual scenarios/harnesses and generator/property/RPi/tools by tools-tests and paired test-source reviews; documentation/build/configuration by coordinator. Investigation questions (cache pointer lifetime, server catalogue capacity, standalone SDK/HOME consumers and non-NULL formatting caller) remain separately reasoned unconfirmed questions, not uninspected files or invented findings.

Dependencies/vendor SDK/Duktape/DDHidLib/libuvc/browser bundles/rJSmin/fxload internals, generated outputs with authoritative input, archives outside selected reuse questions, obsolete/build products, shell-compliance and five owner-protected drivers are excluded. Pixel/coefficient/manual truth, all possible interleavings and physical safety/timing are not proved by proportional source inspection.

## Findings and accountable follow-up
Canonical records own IDs, status and evidence. All new records are open, coordinator-owned and nonblocking for accurate inspection completion; this classification is not product release approval or security-risk acceptance. Assign focused repair tasks with contract-derived regressions; generator changes still require concrete owner approval. Known records are deduplicated, not silently closed or accepted.

## Confirmed findings

### FND-20261101215425-fa40-complete-full-repository-audit-001 — Property CLI reads file uploads from EOF

Category: functional; severity: medium; confidence: high for stated source/probe evidence; owner: coordinator; status: open; blocking: false for accurate inspection completion, not release approval.

Location/evidence: indigo_tools/indigo_prop_tool.c:93-107; components/tools-tests.md item1.

The file helper seeks to EOF for size but never rewinds before read; nonempty script/BLOB uploads return-2 and the failure path leaves the handle open. Actual indigo_uni_seek uses lseek/_lseek. Check size and seek errors, rewind, close on every outcome, terminate text, and add nonempty upload/failing-read cleanup regressions.

### FND-20261101215425-fa40-complete-full-repository-audit-002 — Generator tokenizer hangs on an unsupported DSL byte

Category: functional; severity: medium; confidence: high for stated source/probe evidence; owner: coordinator; status: open; blocking: false for accurate inspection completion, not release approval.

Location/evidence: indigo_tools/indigo_generator.c:340-411,426; components/tools-tests.md item2.

Tokenizer alternatives do not advance or reject an unsupported byte. Exact unchanged-generator temporary build accepts a valid definition but times out on an @ token; prefix match also accepts driverXYZ. Reject unsupported tokens with position and require complete keyword match. A concrete generator implementation change still requires separate owner approval; no generator edit was made.

### FND-20261101215425-fa40-complete-full-repository-audit-003 — Wi-Fi client password guards execute text and continue destructive changes

Category: functional; severity: medium; confidence: high for stated source/probe evidence; owner: coordinator; status: open; blocking: false for accurate inspection completion, not release approval.

Location/evidence: tools/rpi_ctrl.sh:449-450; tools/rpi_ctrl_v2.sh:450-451; components/tools-tests.md item3.

Invalid password guards invoke a quoted message as a command instead of __ALERT. Exact isolated-function stubs show password x writes configuration/disables or deletes the old connection and returns OK despite command-not-found. Reject invalid length before mutations and test lower/upper bounds plus valid controls. No network/service/reboot was executed.

### FND-20261101215425-fa40-complete-full-repository-audit-004 — Sky v2 updater installs the wrong package for advertised 3.x versions

Category: functional; severity: medium; confidence: high for stated source/probe evidence; owner: coordinator; status: open; blocking: false for accurate inspection completion, not release approval.

Location/evidence: tools/rpi_ctrl_v2.sh:533-546; Makefile:45-55,283; components/tools-tests.md item4.

The helper advertises indigo3 versions but install-version always requests indigo=VERSION, so a version available only for the intended3.x package cannot install. Preserve and allowlist package identity with the selected version; add synthetic apt argument regressions for both channels. No apt command ran.

### FND-20261101215425-fa40-complete-full-repository-audit-005 — Documented macOS bootstrap executes unauthenticated HTTP sources

Category: security; severity: medium; confidence: high for stated source/probe evidence; owner: coordinator; status: open; blocking: false for accurate inspection completion, not release approval.

Location/evidence: tools/cltools.sh:23,35,47; README.md cltools reference; components/tools-tests.md item5 and components/security-review.md.

The documented bootstrap downloads GNU sources over plain HTTP without pinned authentication/integrity, executes configure/make and invokes sudo make install. A network intermediary can replace executable source. Use authenticated initial transport and verified artifact authenticity/integrity in a private workspace with fail-fast checks. Independently confirmed from source; no download or external probe, no risk acceptance.

### FND-20261101215425-fa40-complete-full-repository-audit-006 — Falcon motion publishes ALERT while still moving and occupies the handler queue

Category: functional; severity: medium; confidence: high for stated source/probe evidence; owner: coordinator; status: open; blocking: false for accurate inspection completion, not release approval.

Location/evidence: indigo_drivers/rotator_falcon/indigo_rotator_falcon.driver:158-188; components/drivers-b.md B01.

After an accepted MD move and settle delay falcon_move sets ALERT, then FD progress updates publish it while FR can still report moving. The synchronous polling loop holds the queue for up to120s, delaying queued disconnect; asynchronous abort flag remains serviced. Keep BUSY until bounded finalizer completion and preserve abort/disconnect ownership; add healthy-progress/timeout/disconnect cases.

### FND-20261101215425-fa40-complete-full-repository-audit-007 — Trutek accepts replies with an invalid protocol checksum

Category: functional; severity: medium; confidence: high for stated source/probe evidence; owner: coordinator; status: open; blocking: false for accurate inspection completion, not release approval.

Location/evidence: indigo_drivers/wheel_trutek/indigo_wheel_trutek.driver:40-74,117-136; components/drivers-b.md B02.

Identity, position and movement replies validate size/opcode/payload but never the fourth-byte additive checksum, unlike the repository simulator protocol. A corrupted frame with a plausible target slot can be accepted and report success. Validate checksum before consuming all reply types; add malformed-checksum regressions. Source-contract confirmation, no hardware claim.

### FND-20261101215425-fa40-complete-full-repository-audit-008 — WeMacro Bluetooth valid zero batch count underflows the encoded count

Category: functional; severity: medium; confidence: high for stated source/probe evidence; owner: coordinator; status: open; blocking: false for accurate inspection completion, not release approval.

Location/evidence: indigo_mac_drivers/focuser_wemacro_bt/indigo_focuser_wemacro_bt.m:337,438; components/drivers-b.md B03.

COUNT advertises a minimum0 but the accepted batch encodes (uint32_t)count-1, turning0 into UINT32_MAX. This contradicts the bounded step-count contract and portable sibling minimum1. Set a correct minimum or define safe zero semantics before transmission and test zero/minimum/maximum encoding. Device execution was not measured.

### FND-20261101215425-fa40-complete-full-repository-audit-009 — Astromechanics accepts malformed measurement replies as valid sky brightness

Category: functional; severity: medium; confidence: high for stated source/probe evidence; owner: coordinator; status: open; blocking: false for accurate inspection completion, not release approval.

Location/evidence: indigo_drivers/aux_astromechanics/indigo_aux_astromechanics.driver:71-75; components/drivers-a.md item1.

Any positive section read is converted by indigo_atod without complete numeric parse validation; garbage newline becomes0 and OK, and numeric-prefix junk becomes a plausible reading. Validate complete finite device payloads and preserve previous data/ALERT on corruption; add fake serial malformed-reply checks. This concerns transport input, not framework property input.

### FND-20261101215425-fa40-complete-full-repository-audit-010 — FlipFlat accepts a foreign operation acknowledgement as success

Category: functional; severity: medium; confidence: high for stated source/probe evidence; owner: coordinator; status: open; blocking: false for accurate inspection completion, not release approval.

Location/evidence: indigo_drivers/aux_flipflat/indigo_aux_flipflat.driver:41-53; components/drivers-a.md item2.

The command helper accepts any positive reply beginning with *, so a delayed status or malformed acknowledgement can make light/intensity requests report OK without confirming the requested operation. Repository simulator defines distinct operation acknowledgements. Validate echoed opcode/shape/payload and test stale/wrong-operation replies; no hardware execution.

### FND-20261101215425-fa40-complete-full-repository-audit-011 — PrimaLuce AP mode ignores a negative command acknowledgement

Category: functional; severity: medium; confidence: high for stated source/probe evidence; owner: coordinator; status: open; blocking: false for accurate inspection completion, not release approval.

Location/evidence: indigo_drivers/shared/primalucelib/primalucelib.c:302-305,652-667,712-714; components/drivers-a.md item3.

set_access_point returns transport/verb success even for nested AP_SET_STATUS Error text. GIOTTO/ALTO/focuser consumers memoize the requested switch and publish OK while readback only changes separate status. Validate named acknowledgement and retain rejection error before memoizing state; use existing simulator refusal fixtures. Related credential/reboot acknowledgement handling is follow-up within this family.

### FND-20261101215425-fa40-complete-full-repository-audit-012 — Atik2 failed replacement guide pulse removes the original relay stop

Category: functional; severity: medium; confidence: high for stated source/probe evidence; owner: coordinator; status: open; blocking: false for accurate inspection completion, not release approval.

Location/evidence: indigo_mac_drivers/ccd_atik2/indigo_ccd_atik2.driver:420-478; components/drivers-a.md item4; components/security-review.md.

Supported accept_while_busy replacement cancels the old axis finalizer before the SDK relay write. If replacement write fails, the old relay mask remains but no original stop is rescheduled, so the previous pulse may persist until another command/disconnect. Preserve/rearm its deadline or confirm safe stop, with a failed-replacement fake SDK regression. ANYTIME generated branch was independently checked; no physical relay claim.

### FND-20261101215425-fa40-complete-full-repository-audit-013 — RPi ROI control membership compares iterators from different temporary maps

Category: functional; severity: medium; confidence: high for stated source/probe evidence; owner: coordinator; status: open; blocking: false for accurate inspection completion, not release approval.

Location/evidence: indigo_optional_drivers/ccd_rpi/indigo_ccd_rpi.cpp:516,1499; components/drivers-a.md item5.

GetControls returns its ControlInfoMap by value, but ROI code compares find from one temporary with end from another. That is not a valid same-container membership test and can select unsupported ScalerCrop instead of the full-frame fallback. Store one local map and compare its own iterators; test a camera profile lacking the control. No platform runtime executed.

### FND-20261101215425-fa40-complete-full-repository-audit-014 — ALTO simulator mutates state while returning a command refusal

Category: quality; severity: low; confidence: high for stated source/probe evidence; owner: coordinator; status: open; blocking: false for accurate inspection completion, not release approval.

Location/evidence: indigo_drivers/aux_alto/alto_simulator/alto_simulator.c:497-519,596; components/drivers-a.md item7.

set_preset applies NAME/M1POS before checking the configured refusal, then reports Error; DIMLEDS similarly changes state before rejecting. This undermines failure/rollback regression fidelity. Apply mutations only when accepted and assert rejected writes retain prior state. Source proof only, no simulator execution.

### FND-20261101215425-fa40-complete-full-repository-audit-015 — Andor enumeration exceeds the eight-device storage capacity

Category: functional; severity: medium; confidence: high for stated source/probe evidence; owner: coordinator; status: open; blocking: false for accurate inspection completion, not release approval.

Location/evidence: indigo_optional_drivers/ccd_andor/indigo_ccd_andor.c:1434-1435,1480-1574; components/drivers-a.md item6; components/security-review.md.

The SDK count controls init/shutdown loops indexing devices[8] without a capacity bound; nine enumerated cameras can access outside the array. Cap before allocation and every array loop, ignoring/rejecting excess devices, then test with a fake SDK count9. Distinct from existing Andor mutex findings000019/000020. Local SDK/device prerequisite; no remote exploit or hardware execution claimed.

### FND-20261101215425-fa40-complete-full-repository-audit-016 — Pentax hotplug leaks allocations and a USB reference when registry is full

Category: functional; severity: low; confidence: high for stated source/probe evidence; owner: coordinator; status: open; blocking: false for accurate inspection completion, not release approval.

Location/evidence: indigo_drivers/ccd_pentax/indigo_ccd_pentax.c:1684-1704; components/ptp-pentax.md PP01.

The plug callback allocates device/private data and acquires a libusb reference before searching the ten-slot registry, then loses their ownership if no slot exists. Reserve a slot before allocation or clean up all acquired resources on the full-registry path; fake repeated over-capacity hotplug should retain allocations/reference counts. Existing bulb timing and PTP decoder records stay separate.

### FND-20261101215425-fa40-complete-full-repository-audit-017 — ALTO hardware restoration can fail without failing the test run

Category: quality; severity: medium; confidence: high for stated source/probe evidence; owner: coordinator; status: open; blocking: false for accurate inspection completion, not release approval.

Location/evidence: indigo_test/hardware/test_aux_alto_hw.c restore_initial_state; components/tools-tests.md item6.

Restoration ignores move_to/hw_set failures, while those helpers return bool without incrementing the global failure count. Exact extracted restore function with all four calls false still exits0/failures0, leaving changed state while reporting success. Aggregate restoration outcomes and propagate failure while retaining cleanup; no hardware was operated.

### FND-20261101215425-fa40-complete-full-repository-audit-018 — Custom Imager and Platesolver filters report success with zero executed cases

Category: quality; severity: low; confidence: high for stated source/probe evidence; owner: coordinator; status: open; blocking: false for accurate inspection completion, not release approval.

Location/evidence: indigo_test/integration/test_agent_imager_cleanup.py main; indigo_test/integration/test_agent_platesolver.c:1154-1197; components/tools-tests.md item7.

An unmatched Imager environment filter records plan0/exits0, even with a nonexistent child binary; native Platesolver unmatched argv filter likewise selects0 and returns0. The prior common native runner repair did not cover these bespoke runners. Return explicit skip and aggregate no-execution failure consistently, retaining genuine selected-case behavior; add direct no-match regressions.

### FND-20261101215425-fa40-complete-full-repository-audit-019 — ALTO simulator numeric helper accepts NaN as a passing comparison

Category: quality; severity: medium; confidence: high for stated source/probe evidence; owner: coordinator; status: open; blocking: false for accurate inspection completion, not release approval.

Location/evidence: indigo_test/integration/test_aux_alto_simulator.c:245; components/tools-tests.md item8.

The new local number_is helper uses fabs(actual-expected)>tolerance, so a cached NaN passes. Exact unchanged-function synthetic proof returns true; environment/property/preset assertions use it. Reject nonfinite unexpected values/tolerance and use the shared corrected assertion contract. New custom helper scope is distinct from closed common ASSERT_NEAR findingbc8d-029.

### FND-20261101215425-fa40-complete-full-repository-audit-020 — QSI fake trace writer claims success after output errors

Category: quality; severity: low; confidence: high for stated source/probe evidence; owner: coordinator; status: open; blocking: false for accurate inspection completion, not release approval.

Location/evidence: indigo_test/integration/qsi_fake_sdk.cpp:340-362; components/drivers-a.md QSI trace candidate.

qsi_fake_trace_write returns file!=NULL after unchecked fprintf/fclose although its header promises false on I/O failure. A disk-full/write/close failure after successful open can be reported as a captured normalized migration trace. Check ferror and close status, with an injected write/close-error regression; no actual full-device or trace-output probe performed.

### FND-20261101215425-fa40-complete-full-repository-audit-021 — Native RAW loader narrows an unchecked payload length before validating it

Category: security; severity: medium; confidence: high for stated static/source arithmetic evidence; owner: coordinator; status: open; blocking: false for inspection completion, not release approval.

Location/evidence: indigo_drivers/agent_platesolver/indigo_solver.c:588-619,171; components/native-platesolver.md NP01; components/security-review.md.

The RAW truncation guard multiplies uint32 dimensions in size_t and casts the unchecked payload to long. On64bit RGB48 width=height0x80000000 yields modular payload2^63, which narrows negative and lets a header-only input pass; the later mono allocation can terminate the service. Direct CLI and IMAGE BLOB reachability are source-confirmed, and preview rejection does not gate the backend. Validate signed-representable geometry and nonoverflowing input/output extents before conversion/allocation, preferably reusing the common RAW validator and solver limits. Safe arithmetic-only proof; no unsafe allocation, image solve or arbitrary-code-execution claim.

### FND-20261101215425-fa40-complete-full-repository-audit-022 — Solver radius documentation promises a stricter boundary than implemented

Category: quality; severity: low; confidence: high for stated static/source arithmetic evidence; owner: coordinator; status: open; blocking: false for inspection completion, not release approval.

Location/evidence: indigo_drivers/agent_platesolver/README.md:82; indigo_agent_platesolver.c:611-637,629-661; components/native-platesolver.md NP02.

The README says a field outside the radius is not solved, but native solving uses radius plus half-field extent only to select catalogue stars and never checks fitted-center angular distance against the requested radius. A solve center may lie outside the stated boundary. Clarify the intended hint contract or enforce the documented fitted-center bound and test a near-boundary field. Source mismatch only; no recognition rerun.

### FND-20261101215425-fa40-complete-full-repository-audit-023 — Public native solver header reverses the returned parity convention

Category: quality; severity: low; confidence: high for stated static/source arithmetic evidence; owner: coordinator; status: open; blocking: false for inspection completion, not release approval.

Location/evidence: indigo_drivers/agent_platesolver/indigo_solver.h:132-135; indigo_solver.c:1586-1593; indigo_solver_utils.c:95-98; components/native-platesolver.md NP04.

The public compute_center comment calls parity+1 normal/east-left and-1 mirrored, while implementation, helper convention, WCS assignment and independent CD-matrix assertion use+1mirror/east-right and-1sky/east-left. No wrapper flips the sign. Correct the header to match the actual public result and retained README convention; no algorithm or image-solving change required.

### FND-20261101215425-fa40-complete-full-repository-audit-024 — Native star extraction reads an uninitialized count after detector failure

Category: functional; severity: medium; confidence: high for stated static/source arithmetic evidence; owner: coordinator; status: open; blocking: false for inspection completion, not release approval.

Location/evidence: indigo_drivers/agent_platesolver/indigo_solver.c:588-619,793,819-830; indigo_libs/indigo_raw_utils.c:2649,3101; components/native-platesolver.md NP05; components/core-image.md CI01; components/security-review.md.

A valid1x1 RAW with downsample2 becomes a zero-sized normalized image; filtered_image returnsNULL and the common detector rejects geometry without writing stars_found. The wrapper leaves that local uninitialized, ignores the FAILED result and uses it as a loop bound, potentially exceeding capacity-sized arrays. Initialize count, honor detector failure, validate returned count and reject zero post-bin geometry; add a tiny valid frame/downsample2 error-path regression. Two independent source traces confirmed; no unsafe image execution or exploit claim.

## Existing record reinspection
Existing quality finding `FND-20261023065605-60d4-repository-audit-012` is archived as fixed after independent verification that already-merged commit `6e2b8aa12ab3692f2295f174bba044f98ad2c273` corrected origin-scoped ID and per-record storage prose. This task made no procedural code/document fix for it. All other prior open findings, including server HTTP framing/ACL, PTP decoder, Pentax timing, native catalogue pointer and recycled registration, retain their canonical dispositions. Register totals after this audit are59 active and221 archived.

## AI slop inspection
No finding infers authorship or demands cosmetic cleanup. False acknowledgements, cancelled stop ownership, malformed-input handling, unsupported success and misleading test assertions have concrete contracts/impact. Protective checks, tests, source architecture and accepted behavior were preserved. Temporary summary regeneration found only row-order drift with identical recorded outcomes; no separate cosmetic defect was allocated.

## Verification and limits
Configured Factory tests passed197/197 in98.665s; [verification](verification.md) records exact checks/probes/reuse. Targeted local synthetic generator/function/arithmetic checks demonstrate only their stated defects, not broad product success. Existing editor failures remain associated with canonical findings. No image recognition, device operation, broad driver build, external probe or dependency installation was repeated. Platform/SDK/hardware and actual C++ DMA/sensor execution remain unperformed limits, not passing tests.

## Cadence and completion
This is a new qualified full audit of the actual frozen source, not a relabelled partial task. Only after final artifact review, metadata checks and source synchronization will completion use kind `audit`, reviewed source/date and the128 completion IDs visible at that source. It advances full cadence and actual inspected work coverage; no mutable counter or one-time incremental baseline is rewritten. Main merge/publication and cleanup remain separate delivery actions requiring applicable authorization.
