# Driver B source-contract review

Verdict: source-contract coverage complete for the 69 assigned components; changes required through the nonblocking follow-ups below. This is an audit of maintained implementation contracts, not certification of every supported physical device, vendor SDK, platform, or protocol variant. No production changes, shared builds, external endpoint requests or hardware operations were performed by this reviewer.

## Scope, method and independence

Reviewed base `f019b004769072275a3d98a87c8e9740a42c19a4`, with production read-only and writes limited to this report, `driver-b-coverage.json` and `driver-b-evidence/`. Explicitly loaded repository-reviewer, repository-review routine, root project/task context, scoped driver AGENTS and relevant driver development/PrimaLuce protocol material. Root is the canonical findings/production writer. This reviewer did not implement any repairs being reviewed.

The per-component ledger records lifecycle/resource ownership, transport/SDK reply contracts, operations/finalizers/abort/disconnect, and public property behavior. Review is proportionate contract tracing, not a claim to have read every line or exhaustively tested every branch. The generated-driver lifecycle is shared; definitions and authored helpers are authoritative. `contract-anchors.json` records navigation anchors, not independent verification. Source searching and previously successful regeneration locate code and identify ownership; they are not the evidence for semantic completion. The ledger records the actual distinct contract reasoning for every assigned component, including simulator devices and the 15 authored system_alpaca modules.

Owner-prohibited driver directories were not inspected. Vendor SDK/parser implementation supplied as a dependency is not certified. Driver-local simulator/test implementation quality is coordinated with the root test/infrastructure review; their existence and prior result records do not prove physical behavior. Runtime, native-platform and hardware absence are explicit verification limits, rather than unexamined maintained production components.

## Identity-aware prior evidence

`driver-b-evidence/source-manifest.json` identifies 93 authoritative C/.driver/header inputs including the Lunatico shared authored implementation. Of these, 90 have unchanged Git blob identities from the prior audit outcome `0cf556f8f`; the three changed inputs are PrimaLuce `.driver`, library C and library header. Exact comparisons are in `prior-source-identities.json`.

Reused the specifically selected prior audit's `generation-results.json`, `final-driver-generation.json` and `independent-review.md` for generation/build/test facts only. The prior audit explicitly lacked semantic coverage, so those results were not relabeled as source review. This pass supplies the missing contract review. Prior macro/framework/SDK runtime results remain bounded by that report's revision and environment.

Current PrimaLuce changes since the prior audit were inspected directly: SESTO SENSO 3 debug-text prefix stripping before JSON, STEP/DEG alternative ARCO acknowledgements, calibrated position range validation, and final angle tolerance. The current delta preserves exact path-key matching introduced by the prior repair. The library response-buffer string lifetime is respected in the changed blocks. Delta code does not change public property identities; version31 is higher than prior29. Hardware narratives in the current REFACTOR/PROTOCOL are historical author evidence, not hardware tests executed by this reviewer. The existing credential-escaping finding FND-000052 remains applicable.

## Confirmed follow-ups for coordinator registration

All locations below are repository-relative at the reviewed base. These are source-confirmed contract defects, with synthetic expression checks explicitly distinguished from production runtime reproducers. Coordinator owns final severity/gate/owner registration. Proposed follow-up owner: coordinator assigning a dedicated driver repair task with applicable characterization and simulator regression. Nonblocking here means the audit can report the defect; it does not accept physical-motion or data-integrity risk for release.

### B-01 — Device-reported filter counts exceed allocated property arrays

Severity: medium; confidence: high; proposed audit gate: nonblocking functional follow-up, with security reviewer notified of the malformed-input consequence.

`wheel_trutek/indigo_wheel_trutek.driver:49`, `wheel_xagyl/indigo_wheel_xagyl.driver:58`, and `wheel_qhy/indigo_wheel_qhy.driver:106` assign response-derived `WHEEL_SLOT_NAME_PROPERTY->count` and `WHEEL_SLOT_OFFSET_PROPERTY->count` without checking the allocated capacities. Base wheel attach in `indigo_libs/indigo_wheel_driver.c:53` allocates16 slots. Trutek byte0xFF decodes207, Xagyl `FilterSlots 100` decodes100 and QHY `MXP` reply `Z` decodes35. Those become authoritative property counts; consumers iterate count rather than magically resizing the allocation. Corrupt/device-invalid responses can therefore expose invalid property metadata and out-of-bounds item access. `driver-b-evidence/wheel-count-contract.json` retains literal-expression examples; no malformed frame was sent to a real driver/device.

Remediation: validate reply framing/checksum where applicable and the count against both `allocated_count` fields before storing or publishing; reject connection/readback on invalid counts. Add malformed-count simulator regressions proving no property publication exceeds allocation. Do not merely clamp device facts or weaken framework/client validation.

### B-02 — Motion completion lacks an overall timeout in several wheels/mounts

Severity: medium; confidence: high; proposed audit gate: nonblocking functional follow-up.

`wheel_quantum/indigo_wheel_quantum.driver:36-40` repeatedly reads valid `P` positions until the requested slot; a controller continuing to report another slot never lets this synchronous handler return. Individual serial read timeout does not bound a stream of valid replies, so disconnect is queued behind it. Related asynchronous endless-BUSY shapes occur in `wheel_trutek:77-80` (slot0), `wheel_xagyl:99-102` (wrong slot), `wheel_asi:155-156` and calibration175-176 (`pos == -1`), `wheel_playerone:149-155` (the30-poll bound applies only to initial target0, not ordinary motion), `mount_pmc8:620-622` (moving park rates), and `mount_temma:297-301` (GOTO status s1; park already has a deadline).

Remediation: use monotonic operation deadlines and bounded finalizers, retain last measured position and requested target, publish ALERT on expiry, and perform protocol-supported stop/recovery. Characterize stationary/wrong-target/endless-moving replies plus queued abort/disconnect. Bounded transport transactions alone are not whole-operation timeouts.

### B-03 — Lakeside valid stationary progress replies evade the stall bound

Severity: medium; confidence: high; proposed audit gate: nonblocking functional follow-up.

`focuser_lakeside/indigo_focuser_lakeside.driver:177-225`: failed reads and unexpected lines increment `stalled`, but a valid repeated `P<n>` enters218-225, resets only if position changed, and otherwise reschedules without incrementing. An endless repeated position without DONE never reaches40, contradicting the comment's claimed stall bound. The handler yields, but the client remains BUSY and further motion stays excluded indefinitely.

Remediation: count unchanged valid position frames or impose a separate overall deadline, send existing CH stop on expiry, preserve target and publish ALERT. Regression: repeated stationary P frames without DONE; genuine progressing P followed by DONE must still succeed.

### B-04 — SynScan AUTOHOME blocks its own queued abort/disconnect

Severity: high; confidence: high; proposed audit gate: nonblocking functional follow-up, per coordinator's explicit direction to distinguish audit evidence completion from driver release/hardware readiness. No safety risk acceptance is claimed.

`mount_synscan/indigo_mount_synscan.driver:2132` calls `synscan_perform_autohome()` synchronously from AUTOHOME on_change. The helper calls30-second stopped-axis loops577-595,120-second index loops644-665 and711-764, and a three-second sleep698. The loops inspect `abort_motion`, but this source sets the flag only in the queued abort handler1961 and disconnect hook1672. There is no bus-thread on_change_request setting it. These handlers share the serialized queue and cannot run while auto-home waits. Urgent priority cannot preempt the currently executing callback. A requested abort/disconnect can consequently wait through multi-stage physical homing.

Remediation: staged handler/finalizer home state machine with bounded single-transaction checks and named cancellation/stop paths. Preserve documented device command order; add timed simulator assertions that abort/disconnect execute during each stage, verify actual stop-command edges and failure/timeout state, then perform authorized hardware validation in the repair task. Merely setting a volatile flag is not a substitute for queue ownership/cancellation and does not resolve the synchronous waits.

### B-05 — Stopped-short moves are reported successful

Severity: medium; confidence: high; proposed audit gate: nonblocking functional follow-up.

Several authored finalizers treat idle as arrival despite having measured position and retaining a different requested target: `focuser_mypro2/indigo_focuser_mypro2.driver:333-338`, `focuser_dsd:215-217`, `focuser_dmfc:277-278`, `focuser_fc3:223-224`, `focuser_asi:207-208`, `focuser_usbv3:254-255`, and shared Lunatico rotator `focuser_lunatico/shared/lunatico_shared.c:889`. For example, mypro2 target1000, measured500 and moving0 enters the unconditional OK branch; the caller cannot distinguish target arrival from premature device stop. Shared Lunatico affects the rotator interface exposed by both focuser_lunatico and rotator_lunatico.

Alpaca absolute focuser is excluded from this finding: its canonical class contract in `system_alpaca/indigo_system_alpaca_private.h` explicitly defines completion by IsMoving rather than comparing positions. Its deliberate settling window and idle completion are consistent with that accepted contract. A proposed target-equality finding there is rejected; changing the contract requires a separate explicit product decision.

Remediation: distinguish commanded vs external/aborted motion, compare measured position with commanded expected position using protocol-appropriate tolerance/backlash semantics, publish ALERT on stopped-short and retain requested target. Add one early-stop regression per affected driver rather than assuming all protocols are identical. Existing exact-target handling in Astroasis, FLI, EFA, FocusDreamPro, Askar and SteelDrive2 shows a local behavior-preserving pattern.

### B-06 — Falcon direction setter accepts failed/mismatched acknowledgements

Severity: medium; confidence: high; proposed audit gate: nonblocking functional follow-up.

`rotator_falcon/indigo_rotator_falcon.driver:222` tests `!falcon_command(...) && !strncmp(response, "FN:", 3)`. A transport-successful reply `ERR` makes the first operand false and never sets ALERT. A failed command with a different/empty reply also makes the prefix operand false. The generated prologue therefore publishes the requested reversal OK even when its acknowledgement is invalid or absent. `driver-b-evidence/falcon-direction-truth-table.json` records the exact Boolean truth table, not a hardware test.

Remediation: require transport success AND valid acknowledgement (failure OR invalid prefix), preserve/read back prior direction on failure, and regress success/refusal/timeout/stale reply combinations. Keep changes in the .driver and regenerate without changing generator semantics.

## Existing finding reconciliation and rejected overclaims

- FND-000006: current CGUSB-ST4 still emits `:Mg%c%4d#` using letters. The historical alternative encoding is not established by repository-local evidence. Leave blocked for manufacturer/upstream protocol confirmation and targeted hardware/independent emulator comparison; do not label either encoding a verified correction.
- FND-000028: current `indigo_test/Makefile:154` includes `test_rotator_lunatico_simulator` in INTEGRATION_TESTS. The historical registration defect is absent by source inspection. This review did not run its11-case executable; coordinator should reconcile retained passing evidence/current identity or run it before closing the record, and must not silently claim a new runtime pass.
- FND-000052: raw WiFi SSID/password interpolation remains in shared request templates/setters and driver call paths. Keep the existing record rather than duplicate it; JSON escaping needs its own repair/regression.
- GPUSB's unreliable SDK status bypass is deliberate, documented compatibility behavior, not unused fallback scaffolding to remove without hardware characterization.
- Framework input numeric validation belongs to the bus; duplicated range checks in some drivers are not automatically functional findings. Device/SDK reply bounds remain a distinct trust boundary.
- PrimaLuce debug-prefix tolerance has a specific recorded SESTO3 device reason. It is not evidence of fabricated protocol behavior. Current exact-token matching remains intact.
- Alpaca HTTP Content-Length uses decimal-only saturation and duplicate-value validation; non-replayable PUT retry is limited to connect failure, unlike replayable GET resets. JSON parser has explicit depth/node limits and malformed Unicode rejection. No new security clearance follows from these inspected guards.

## Remaining runtime limits and next assignment

All69 assigned production component contracts have ledger entries. Hardware, Linux/Windows/SDK executions, time-accuracy measurements and hostile live-network interoperability were not run here. Those are scoped verification/release limits, not claims that protocol behavior is universally correct. New source defects above require dedicated repair tasks with characterization; the root security reviewer must retain the malformed-reply consequence of B-01 in security disposition. Root reconciles canonical IDs and coordinates test/source evidence before qualifying the whole repository audit.
