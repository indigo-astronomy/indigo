# Refactoring plan for INDIGO 3.0 Player One CCD driver

Date: 2026-09-08.

Status: implementation checkpoints 1–8 are complete for the available macOS/Mars-C II environment. The fake SDK suite contains 44 cases; final coverage and evidence are listed below. Unavailable hardware/platform acceptance is explicitly deferred, not marked passed.

## Goal and scope

Migrate `indigo_ccd_playerone.c` to `indigo_generator`, preserving camera acquisition, streaming, optional ST4 guider, cooling, SDK-dependent controls and device identity. Replace handwritten property/lifecycle/hot-plug boilerplate with generated code and move SDK work to handler queues. Include repository formatting, the required `X_` custom-property prefix, deterministic fake SDK coverage and real hardware regression testing.

Work in independently verifiable commits. Do not upgrade the bundled SDK, refactor other camera drivers or change the generator merely to preserve legacy scaffolding. If a required behavior cannot be expressed, isolate and test the smallest generator extension before switching this driver. Keep unrelated defects in `indigo_drivers/REVIEW.md`, automated coverage/deferred work in `indigo_test/CHANGES.md`, and migration progress here; this inventory does not advance an incremental review baseline.

## References

- `wheel_playerone/REFACTOR.md` and `indigo_wheel_playerone.driver`: completed SDK discovery migration, SDK identity-based `unplug_match`, bounded suffix handling and test process. Camera SDK contracts differ from the wheel SDK.
- `wheel_asi`, `focuser_asi`, `rotator_asi`: current generated SDK hot-plug and driver queue implementation, not their superseded migration stages.
- `ccd_sx/REFACTOR.md`: shared CCD/guider lifecycle and nonblocking exposure finalizers. Its direct USB identity model does not apply to Player One.
- `ccd_dsi/REFACTOR.md` and `.driver`: generated CCD structure and acquisition cleanup.
- `ccd_touptek/REFACTOR.md`: camera queue/lifetime and physical test scenarios; that refactor deliberately does not use the generator.
- `indigo_docs/DRIVER_GENERATOR_MIGRATION.md`, `DRIVER_DEVELOPMENT_BASICS.md`, `DEVELOPMENT.md`, `TIMERS_AND_QUEUES.md`, and actual output from `indigo_tools/indigo_generator.c`: lifecycle, master queue routing, cancellation and generated handler semantics.
- Root `AGENTS.md`, `.editorconfig`, `uncrustify.cfg`, `README.md`, `TESTING.md`, `indigo_docs/MAKEFILES.md` and this driver's `README.md`.
- `bin_externals/libplayeronecamera/include/PlayerOneCamera.h`, SDK changelog and bundled SDK manual: camera SDK contract. Resolve remaining timing/identity questions against these and hardware before implementation decisions.
- `indigo_docs/PROPERTIES.md`, section `ccd_playerone`: current public property/source mapping.
- `indigo_test/AGENTS.md`, `integration/test_wheel_playerone_sdk.c`, the ASI SDK replacement tests, `simulator_test_common.h`, and their `indigo_test/Makefile` rules.

Paths above are repository-relative except sibling driver names and this driver's SDK paths.

## Recorded baseline

Source baseline: `0f52fe3bc0b826767c91d3e7bd409fe8370abd69`; workspace was clean before this document was added. Source references below are lines in that commit's `indigo_ccd_playerone.c` and remain baseline references after reformatting.

- Handwritten C: 2,412 lines, no `.driver`. Entry/name `indigo_ccd_playerone`, label `Player One Camera`, version `0x03000011`; driver metadata advertises multi-device support.
- README supports SDK-recognized Player One/iOptron cameras, Linux Intel 64-bit/ARM v6+ and macOS. Historical tested devices are MARS-C II, Poseidon-C Pro and Sedna-M; this is not evidence of hardware availability or a test of the migration.
- SDK files include macOS, Linux x86/x64/ARM/ARM64 and Windows Win32/x64 libraries. The changelog's first version is 3.10.1; its date ordering is inconsistent. Record runtime `POAGetSDKVersion()`/`POAGetAPIVersion()` during baseline testing instead of inferring the actual loaded binary version from the changelog.
- No local Makefile is needed. `make -n -C indigo_drivers/ccd_playerone -f ../../Makefile.drv all` resolved the archive, dylib and standalone executable and the bundled macOS SDK. This was target discovery only, not a compilation.
- No dedicated `test_ccd_playerone_sdk` target exists. The wheel test is a structural reference, not camera coverage.

### Source and execution inventory

| Baseline source | Responsibility / migration constraint |
| --- | --- |
| 103–137, 207–246, 425–440 | Shared private data, `count_open`, SDK Open/Init/Close, global lock and image buffer; CCD master and optional guider share them. |
| 248–423 | Exposure geometry/format/config setup and cooler helper; preserve SDK units and ordering. |
| 447–710 | Single exposure and streaming timer workers; countdown sleeps, readiness polling, blocking image download, image processing and abort/failure/video cleanup. |
| 712–803 | Five-second temperature polling and separate RA/DEC pulse completion timers. |
| 805–944 | CCD attach, SDK format/bin-derived modes, property allocations and capability defaults. |
| 945–1353 | Dynamic typed SDK configs, sensor modes and preset selection synchronization. |
| 1355–1463 | CCD connection initialization, property definition/deletion and timer cancellation. |
| 1465–1893 | CCD bus changes, some synchronous SDK calls, zero-delay workers, CONFIG persistence and frame/bin/mode synchronization. |
| 1895–2060 | CCD detach; guider attach/connect/change/detach, guide output writes on the bus path. |
| 2064–2412 | Manual arrays, SDK-id scans, naming, attach/detach, USB callbacks, init/shutdown. |

Legacy `MAX_DEVICES = 12` counts logical devices, so a CCD+guider consumes two slots. `connected_ids[]` and shutdown bookkeeping also treat camera ids as bounded array indices. USB registration filters VID `0xa0a0` and any PID; arrival/removal schedules unowned 0.5-second timers. Discovery probes Open/Close, attaches CCD first and optional guider when `isHasST4Port`, with names `model #suffix` and `model (guider) #suffix`, made unique using SDK id.

### Property compatibility inventory

All five custom properties are connected-only. Preserve groups, labels, permissions, item names, switch rules and dynamic values except the explicit property-name migration below.

| Current name | Proposed name | Contract to capture and preserve |
| --- | --- | --- |
| `PIXEL_FORMAT` | `X_PIXEL_FORMAT` | RW one-of-many, up to four supported formats: `RAW 8`, `RGB 24`, `RAW 16`, `MONO 8`; linked to CCD mode, binning and frame BPP; saved by CONFIG. |
| `POA_ADVANCED` | `X_ADVANCED` | RW number property built from remaining writable SDK configs, with SDK names/ranges and bool/int/float conversion; saved by CONFIG. Preserve bandwidth default 45 and reconnect rebuilding. |
| `POA_CUSTOM_SUFFIX` | `X_CUSTOM_SUFFIX` | RW text, item `SUFFIX`, flash-backed SDK custom id; updated name on replug, empty clears. Camera limit is 16 bytes, unlike wheel's 24. Legacy name parsing truncates to 15; make full-length handling explicit and bounded. |
| `POA_PRESETS` | `X_PRESETS` | RW at-most-one, four items `POA_HIGHEST_DR`, `POA_UNITY_GAIN`, `POA_LOWEST_RN`, `POA_GAIN_HCG`; dynamic labels/values from SDK gain/offset recommendations, synchronized with gain/offset/egain. |
| `POA_SENSOR_MODE` | `X_SENSOR_MODE` | RW one-of-many, dynamically resized from SDK sensor modes; hidden if unsupported or initialization fails; saved by CONFIG. |

As requested, custom property names use only the `X_` prefix; remove the legacy `POA_` property prefix rather than retaining it after `X_`. This is an intentional public compatibility change: update scripts/configuration guidance and `PROPERTIES.md` in the same commit, including the `.driver` source mapping when switched. Do not add undocumented aliases or silently claim old saved custom-property names still load.

Standard properties retain standard names. Capture a bus snapshot per fake capability profile before edits, covering constructors/items, all `count`, `hidden`, permission and numeric range mutations, and CONFIG persistence. In particular:

- CCD INFO count 8, SDK/API string, sensor metadata, serial, dimensions and pixel size; guider INFO count 5.
- SDK-dependent format/bin mode list and default selection, square binning, frame alignment and minimum binned frame dimension 64; RAW8/16, RGB24 and MONO8 dimensions/BPP/Bayer metadata.
- Streaming and streaming settings visible, image format count 7; exposure units support `POA_EXP` and legacy microsecond `POA_EXPOSURE` through the long-exposure compile path.
- Gain/offset/egain, temperature/cooler/power visibility, permissions, limits and values from SDK configs; temperature tolerance 0.5 degrees, display rounded to 0.1 degree, five-second polling.
- Guider RA/DEC independent pulses, direction mapping and BUSY-to-completion state/zeroing; optional guider attachment, standalone guider connection and either connection/disconnection order.

### SDK contracts and unresolved migration decisions

- Header lines 131–154 expose unique `cameraID`, serial, `localPath`, capability flags, formats and bins. Lines 217–241 explicitly distinguish enumeration index from id and permit metadata queries without opening. Deduplicate by id comparison, never by an assumed id range.
- Neither enumeration order nor the mere presence of `localPath` proves a portable mapping to a libusb event. Reuse current `sdk.unplug_match` support to reconcile SDK presence where appropriate; failed enumeration is inconclusive. Verify two-camera arrival order, startup enumeration, unrelated wheel events under the same VID and delayed SDK visibility. Resolve readiness retry ownership without untracked timers or sleeping on the driver queue.
- Header lines 586–605 document blocking `POAGetImageData()` and timeout; current calls use 2,000 ms. A delayed handler does not make that SDK call nonblocking. Establish a bounded readout strategy and measure abort/guide latency, including the macOS path that deliberately disables `POA_SAFE_READOUT`.
- Both acquisition paths use `POAStartExposure(id, false)`. Preserve the documented Saturn-C single-frame workaround despite misleading single-frame log text. Keep `POA_ENABLE_LONG_EXPOSURES` and platform-specific safe-readout policy until separate evidence supports a change.
- Sensor mode changes require stopped acquisition. Suffix writes touch flash and can interrupt exposure (header lines 665 and 696–710). Busy guards must run before accepted state is overwritten; test late requests as well as normal idle changes.
- Generated optional guider attachment, exact naming with suffix position, dynamic property resize/definition order and shared failure cleanup must be proved with a temporary generated prototype before cutover. Do not assume the single-wheel template supplies these behaviors.
- Inspect the Open-success/Init-failure path (227–240), unchecked config enumeration/attribute outputs (1367–1378), repeated readiness results (494–504 and 629–639), and explicit Close after detach (2295). Turn migration-relevant cleanup/error cases into regressions; record independent findings in the folder review file before widening scope.

## Target execution and ownership

Use `driver playerone`, `playerone_open(indigo_device *device)` and `playerone_close(indigo_device *device)`. Generated `.driver` is the source of truth, with synchronized `.c`, `.h` and `_main.c`.

Use `sdk { hotplug = true; vid = POA_VENDOR_ID; ... }` with the existing any-PID policy, Player One discovery in `plug`, and SDK presence matching on unplug if required. The generator owns registration, retained USB references, device arrays, capacity checks, driver queue and teardown. Use the generator default capacity, never a `MAX_DEVICES` override; document and test the resulting physical/logical capacity rather than promising the legacy twelve logical slots.

One CCD master and a capability-gated guider share generated private data. The generator owns multi-device `PRIVATE_DATA->count`; remove `count_open` and `gp_bits` connection aliases without introducing duplicate accounting. Helpers perform first-open/last-close SDK and global-lock operations, with balanced rollback on Open, Init or subsequent connection initialization failure.

| Work | Target context |
| --- | --- |
| Discovery, attach/detach, Open/Init/Close and CONNECTION | Persistent generated driver queue |
| Acquisition control/readout, config SDK calls, temperature and guiding | Physical camera master handler queue; slave callbacks retain their guider device argument |
| Exposure wait/countdown, image readiness, stream continuation and guide completion | Bounded delayed handlers/finalizers; priority timing where needed for guide completion |
| Bus callbacks | Validate/copy accepted requests, publish appropriate state, enqueue and return |

Driver and device queues are different threads. Prove generated cancellation/draining and framework locking protect shared SDK calls before removing enumeration/private mutexes. CCD disconnect stops acquisition and finalizes video without disabling a connected guider. Guider disconnect stops both physical guide outputs while keeping a connected CCD alive. Last close occurs after no handler can use the handle/buffer. Removal detaches slave before master and frees shared state once. No completion may publish deleted properties or revive polling after disconnect.

Do not move an entire exposure or infinite streaming loop onto the master queue. Split acquisition into start, wait/readout and completion; yield between frames. Preserve one image publication per successful frame, streaming counts, abort cleanup, failure cleanup and exactly-once video finalization. Deadline/state fields should be only those needed by this protocol.

Use generator attributes for inherited property visibility/persistence/dispatch where possible. Use `connection_result`, with no return from connect/disconnect blocks. Empty `on_change { }` is only for copy-only properties; omitted handlers delegate appropriately. Inspect BUSY guards and generated copies. Ordinary handlers use generated state/update epilogues; handlers containing `_finalizer` own every initial, error, no-op and completion update.

## Atomic implementation sequence

Each numbered step is a separate reviewable commit (or a documented smaller split). Record commands, results and deviations below its checklist before proceeding. Every production-changing commit must compile and pass the narrow regression suite; do not commit an intermediate driver that only connects but has lost acquisition or guider behavior.

1. **Freeze executable baseline and seed fake SDK tests.**
   - [x] Record current HEAD/dirty files, compiler/platform, runtime SDK/API version and a forced narrow rebuild of both C translation units.
   - [x] Add a separately compiled driver + fake camera SDK/libusb test target under `indigo_test`, using public bus APIs and entry points, real framework queues and CCD/guider bases. No production `.c` inclusion, vendor library, network server or real USB enumeration in the tests.
   - [x] Capture property snapshots and successful connect/acquisition/guide/config/disconnect workflows against the handwritten implementation; isolate test configuration in temporary storage. Record known failing cases separately rather than encoding defects as expected behavior.
   - [x] Record physical baseline on available hardware, or explicitly mark unavailable profiles pending.
   - Acceptance: baseline build/test logs and reproducible fixtures; `CHANGES.md` describes actual coverage.

2. **Reformat handwritten driver without changing behavior.**
   - [x] Format only this driver's handwritten C/header/main as applicable: UTF-8/LF, tabs, trimmed whitespace, K&R and mandatory control-flow braces, operator/comma spacing, spaced inline initializers, one blank line between functions and none inside, all calls/macros on one line.
   - [x] Review formatter output manually against root rules; do not alter expressions, SDK call order or logs as cleanup. Preserve license/history, extend copyright to 2026 and append the OpenAI Codex refactoring notice after the license header.
   - Acceptance: isolated formatting diff, `git diff --check`, rebuild and unchanged baseline tests. Do not later hand-format generated output.

3. **Apply the custom-property prefix migration.**
   - [x] Rename the five public names using the mapping above and update internal handles consistently; keep standard properties and custom item names unchanged.
   - [x] Update `PROPERTIES.md`, configuration/script migration notes in driver README and fake snapshots together. Verify save/reload under the new names and ensure old names are not still advertised.
   - Acceptance: only documented property-name differences in snapshots; acquisition/guider regressions pass.

4. **Make acquisition and peripheral workers queue-compatible.**
   - [x] Replace sleeping acquisition/streaming loops with bounded start/poll/readout/finalize handlers; keep vendor workarounds and image semantics. Move ordinary SDK property operations, temperature and guide pulses to the master queue.
   - [x] Implement cancellation and physical relay-off behavior, buffer lifetime and stale-work rejection; retain necessary synchronization until generated lifecycle ownership replaces it.
   - [x] Add timeout, abort-at-each-phase, streaming interruption and concurrent guide/temperature regressions alongside the changes.
   - Acceptance: no wait loop monopolizes the queue; measured bounded abort and guide latency, CCD/guider sharing and image payload assertions pass. Any readout timing change is documented and checked on available hardware before cutover.

5. **Prove generator representation, then switch complete lifecycle atomically.**
   - [x] Prototype in a temporary directory: SDK hot-plug, optional guider, exact names, dynamic properties, failure rollback, capacity and removal matching. If a generator capability is missing, land its minimal separately tested extension first; regenerate representative existing drivers to detect unintended changes.
   - [x] Write the complete `.driver` directly from the now-tested implementation using generator-owned blocks. If reverse extraction is chosen, `-c` must target the new `.driver`, never the existing C source.
   - [x] Transfer discovery/open/close and CCD/guider lifecycle to generated ownership; delete duplicate arrays, counts, timers, manual property ownership and mutexes only after equivalent lifetime guarantees are verified.
   - [x] Generate all three outputs and add `.driver` to the Xcode group and relevant project listings; preserve existing project changes. Update the property source mapping.
   - Acceptance: complete driver builds; baseline, failure rollback, multi-camera and queue tests pass; every behavioral difference from pre-cutover source has an explanation. Second generation produces byte-identical output.

6. **Complete adversarial fake SDK regression coverage.**
   - [x] Implement remaining rows of the matrix below; assert state/event order, SDK calls and payloads, not merely return codes. Run normal and supported AddressSanitizer/UndefinedBehaviorSanitizer builds, plus queue cancellation/sharing tests.
   - [x] Exercise both safe-readout and macOS acquisition policies with isolated test objects; never redefine platform behavior globally just to make a test pass.
   - Acceptance: deterministic bounded tests, no SDK calls after close, duplicate close, leaked session/buffer/reference or updates after detach. Clean test outputs using `make -C indigo_test test-clean`; update `CHANGES.md`.

7. **Validate builds and the migration diff.**
   - [x] Build archive/shared library/standalone with the bundled SDK using `make -C indigo_drivers/ccd_playerone -f ../../Makefile.drv all`, ensuring changed sources actually recompile.
   - [x] Check available macOS architectures and Linux x64/ARM builds. Windows project/library presence alone is not supported-platform validation; record Windows separately if a usable build exists. Mark unavailable platforms deferred.
   - [x] Recheck generation idempotence, project syntax, property inventory, formatting of handwritten blocks, absence of capacity overrides and the final behavioral diff. Keep vendor binaries/build products out of commits.
   - Acceptance: recorded build evidence for each claimed platform and all narrow tests pass; broader tests only where framework/generator changes warrant them.

8. **Run real hardware acceptance and close the plan.**
   - [x] Run applicable matrix workflows on the available Mars-C II; record unavailable camera capabilities explicitly. Run the matrix below on each available camera with the real vendor SDK, including actual cable unplug/replug and unload/reload. A fake disappearance or logical disconnect does not establish physical hot-plug coverage.
   - [x] Record date, commit, OS/architecture, SDK/API, model/firmware, capability profile, test procedure and result in `TESTING.md`; summarize here and update README tested status accurately.
   - Acceptance: software verification and hardware results are separate. Unavailable cooler/ST4/model/platform scenarios remain explicitly pending; do not mark full hardware validation complete from a single camera or fake tests.

## Fake SDK acceptance matrix

Use independent named cases for CCD and guider, plus a fixture that observes both devices for shared-lifetime scenarios. Follow `indigo_test/AGENTS.md`; test work stays under `indigo_test`, with production changes in their own steps. Test hooks should follow existing SDK replacement linker/compile patterns and preserve actual framework behavior.

| Area | Required cases and observations |
| --- | --- |
| Discovery | Initial enumeration, duplicate arrivals, ignored unrelated PID/device with same VID, delayed SDK readiness, query failure/recovery, reordered SDK ids versus USB events, ids outside 0–11, two identical model names, generator capacity overflow/recovery. |
| Optional devices/ownership | Mono/color, with/without ST4, CCD-only, guider-only, both connect/disconnect orders, repeated cycles; one Open/Init per shared session and one final Close/global unlock. |
| Failure rollback | Open/Init/config/mode/preset failure, partial attach failure where injectable, global-lock failure, reconnect after each; no invalid output use or stranded discovery reservation. |
| Properties/config | All five renamed properties; dynamic config types/ranges/counts/visibility, unsupported sensor modes, grow/shrink across reconnect, new-name persistence, suffix 0/15/16/17 bytes and write failure, malformed names, full-length nonterminated SDK strings. |
| Image contract | Supported RAW8/16, MONO8, RGB24; full frame/ROI/bin geometry, payload size/pattern, Bayer metadata, exposure units, short/long exposures, readiness retries, state/readout/start/stop failure, deadline expiry. |
| Streaming/abort | Finite and indefinite stream, exact successful frame count, stop/abort before start, during wait/readout and between frames; no duplicate BLOB or finalization, cleanup permits immediate next exposure. |
| Controls under load | Busy rejection before copies for format/sensor/suffix where required; gain/offset/preset partial failure and egain synchronization; temperature read/set error, cooler values and periodic scheduling. |
| Guider timing | All four directions, simultaneous RA/DEC, pulse replacement and zero request, SDK write failure, disconnect during a pulse; physical output OFF and public zero/OK or ALERT as appropriate. Guiding remains responsive during acquisition. |
| Removal/queue lifetime | CCD disconnect while guider stays active and converse; physical-event simulation during each acquisition phase and pending pulse/config/poll, reconnect/replug with changed ids, multiple removals, shutdown with queued discovery and init/shutdown cycles. |

Use fake SDK call logs and deterministic barriers where concurrency matters, bounded public-property waits and an external process deadline for deadlock cases. Fast-forward long deadlines only through isolated test hooks while retaining production queue ordering. Fake SDK tests establish driver behavior, not vendor timing or physical relay/image correctness.

## Real hardware acceptance matrix

Candidate coverage from README: MARS-C II, Poseidon-C Pro and Sedna-M. Add Saturn-C specifically if available to verify the existing exposure workaround, and an iOptron-branded camera for OEM discovery if available. Do not assume these devices are attached.

| Workflow | Evidence required |
| --- | --- |
| Identity/lifecycle | Correct CCD/optional guider names and serials at startup and replug; repeated connections in both orders, guider-only session, two cameras with different connection/arrival order, no incorrect detach. |
| Acquisition | Real usable images in every supported format; full frame, ROI, supported bins, short and long exposures; inspect dimensions, bit depth, Bayer pattern, pixel content and successful subsequent exposures. |
| Streaming | Finite exact frame count and sustained indefinite stream; abort/stop/restart, report duration/frame count and relevant latency rather than just “passed”. |
| Guiding | Verify SDK command results, property completion and cancellation; overlap axes and image acquisition, disconnect during pulse. |
| Cooling/controls | On capable hardware: target change, temperature/power readback, cooler off/on, gain/offset/presets, advanced controls and sensor modes, configuration reload; restore original settings. |
| Suffix | Record original suffix, write/test full camera-supported length, replug/name check, clear/replug and restore original suffix; avoid repeated flash writes in endurance loops. |
| Physical interruption | Cable unplug during idle, exposure, stream/readout and guiding; replug and reacquire. Use two cameras to prove the surviving device continues operating. Record any SDK timeout or required recovery. |
| Teardown/reload | Disconnect both logical devices, unload/reload driver/SDK in a supported host, reacquire; no lingering process, queue or device allocation symptoms. |

Record incomplete scenarios and required hardware explicitly. Keep temporary images/logs outside tracked source and remove avoidable test processes/artifacts after recording results.

## Execution results — steps 1–2 (2026-09-08)

- Pre-existing task edits: this plan (subsequently staged by the user) and its Xcode reference. Preserve staging and unrelated edits; implementation steps are recorded as verified working-tree checkpoints rather than automatically committing the user's index.
- Forced baseline build compiled C/main for macOS arm64 and x86_64 and produced archive/dylib/executable. Existing linker warning: SDK deployment target 10.15 versus driver 10.10. Runtime SDK 3.10.1, API 20260430.
- Added separately compiled fake SDK/libusb integration test and opt-in hardware executable, both in Xcode. Three baseline cases pass: CCD properties/RAW image/CONFIG save, guider-first shared session/pulse/CCD disconnect, and finite three-frame streaming. Configuration is isolated; image processing and framework remain real. The fake global lock models an already-unlocked failure rather than allowing its count to become negative; legacy CCD detach performs a redundant unlock.
- Mars-C II is attached (USB VID 0xa0a0). SDK enumeration requires execution outside the desktop sandbox; unrestricted enumeration sees one camera. Physical baseline produced twelve RAW images at 1936 x 1100, passed the preceding exposure/abort/stream operations, then aborted with `pthread_mutex_destroy(mutex) == 0` in libusb `threads_posix.h:59` during the remaining lifecycle workflow. This is a baseline failure, not a passed reconnect/reload test; investigate with phase logging after lifecycle conversion. ST4 command/property completion does not measure physical relay outputs.
- Formatted handwritten C with mandatory braces, single-line calls, whitespace and function separation; preserved expressions/SDK order and extended license years/agent notice. No formatter executable was available. Narrow build and all three fake cases pass after formatting. Further handwritten blocks must follow the same rules; generated output is exempt.
- Additional capability profiles, configuration reload, adversarial SDK errors and image content details remain in steps 4/6. The initial harness is a seed, not completion of the full test matrix.

Final user metadata clarification: author is Peter Polakovic and copyright is CloudMakers. Preserve the historical note about the original ASI implementation by Rumen G. Bogdanovski.

## Execution results — generator cutover and acceptance (2026-09-08)

- Public custom names now use only `X_`; item names remain compatible. README and PROPERTIES describe configuration migration. Author is Peter Polakovic; copyright is CloudMakers. The `.driver` source and both test sources are referenced in Xcode.
- Acquisition uses delayed handlers with bounded SDK reads, including retry of the real macOS SDK's transient OPERATION_FAILED only while its camera state remains EXPOSING. The Saturn single-exposure workaround is retained. Abort, disconnect and completion cancel pending work and release image resources through one acquisition finish path.
- SDK discovery/open/close and shared CCD/optional guider ownership now use generator scaffolding and its default logical-device capacity. Small generator additions express optional attachment, exact SDK names and guards before property copies; SDK multi-device CONNECTION changes use the driver queue. An earlier comparison before the shared-open lifecycle changes matched representative outputs; this is not a claim that later lifecycle changes leave generated output unchanged. Generator architecture tests pass.
- Guider completion callbacks follow the required `guider_ra_finalizer` / `guider_dec_finalizer` naming. Their initiating blocks explicitly publish state because `_finalizer` suppresses the generator's default prologue/epilogue. Root AGENTS.md and migration documentation now require this convention. Tests observe BUSY on both axes and cover replacement, completion and disconnect.
- Thirteen fixture-isolated fake SDK cases pass, including rollback, shared ownership, optional guider/capacity, SDK-id removal, attach failure recovery, config roundtrip, busy guards, streaming/readout/abort and guiding. Normal macOS and explicit POA_SAFE_READOUT builds pass. ASan/UBSan arm64 instrumentation of the test, production driver and separately compiled base framework also passes; prebuilt static dependencies are not instrumented.
- Added monotonic ON-to-OFF timestamps at entry to fake POASetConfig. Measured 72 pulses (20/100/250 ms, all four directions, three repeats, idle and during exposure), excluding one warm-up per phase. After finalizer renaming: idle mean +1.928 ms / max +5.060 ms; exposure mean +2.684 ms / max +5.054 ms. Per-sample errors and distribution statistics are reported without asserting host-specific timing limits. This does not measure electrical ST4 output.
- macOS arm64/x86_64 archive, dylib and standalone builds pass; the pre-existing SDK 10.15 versus target 10.10 linker warning remains. Repeated generation is byte-identical and Xcode project syntax passes. Linux/Windows execution and builds are unavailable in this environment.
- Mars-C II with SDK 3.10.1/API 20260430 passes exposure, abort/reacquire, finite and indefinite streaming, guider-first sharing, CCD disconnect with guider retained, reconnect and driver shutdown/reinitialization (the SDK stays loaded). The former baseline libusb teardown assertion did not recur. A separate physical USB unplug during streaming, user-assisted replug, fresh exposure and guide pulse, then driver shutdown/reinitialization also passes. Details are recorded in TESTING.md.
- Open at that earlier checkpoint (superseded by the final coverage below): exhaustive injected SDK failures and image-pattern/ROI/bin coverage; delayed SDK-discovery readiness; hot-unplug at every phase; other camera models, cooling, multi-camera physical survival and persistent suffix/replug validation. These matrix rows are not implied complete by the passing narrow acceptance suite.

## Standard expansion follow-up (2026-09-08)

The authoritative camera test scenarios are in `indigo_test/DRIVER_TESTING_RULES.md`, linked from both AGENTS files. At the earlier checkpoint the expanded fake suite contained 32 cases with simulator image fixtures, 120 guider timing samples, capability/error profiles and controlled acquisition races; see `indigo_test/CHANGES.md` for coverage and explicit gaps. The old thirteen-case and 72-sample results above record the earlier acceptance stage.

`playerone_open()` now rolls back failed initialization itself; the generator increments its shared count only after success. `playerone_close()` releases a successfully opened handle without an `opened` flag. The cooled fake profile exposed a request/poll race; `CCD_TEMPERATURE` uses `preserve_values` and reads the requested target rather than the measured value. The user approved the three DSL extensions and lifecycle fixes; the later parser fix follows their instruction to parse `handler` before `handle`, with no change to prefix matching. The generator suite now exercises 85 attribute/value/block cases.

Final rerun after these changes: all 32 fake cases pass in normal, safe-readout and ASan/UBSan builds; generator parsing/architecture tests pass; the driver builds for macOS arm64/x86_64. The expanded Mars-C II test passes, including four pixel formats, ROI/bin checks, configuration roundtrip, completed long exposure with guiding and a ten-second stream. Physical interruption at other phases and the remaining standard gaps above are still open.

## Final checklist audit and closure (2026-09-08)

The numbered steps above are verified working-tree checkpoints; the user's staged changes are preserved and no commits were created. The three-case handwritten baseline remains the historical evidence from steps 1–2; later capability snapshots describe the generated implementation, not a retroactive run against the old driver.

### Software acceptance

| Matrix row | Implemented evidence |
| --- | --- |
| Discovery | `optional_guider_capacity_and_identity`, `sdk_discovery_identity_and_strings`, `discovery_filter_and_recovery`, `duplicate_events_and_pending_shutdown`, `discovery_retry_lifetime`: nontrivial/reordered SDK ids, identical models, wrong VID/descriptor errors and inconclusive enumeration, capacity, burst events and delayed SDK visibility without a second USB arrival. Six half-second retries are bounded, coalesced per USB device, canceled on removal and released on shutdown. An unrelated same-VID event with no SDK camera cannot attach a camera. |
| Ownership | `lifecycle_metadata_and_repetition`, `guider_and_sharing`, `partial_slave_attach_and_replug`: both connection/disconnection orders, individual logical interfaces, optional guider, mono/color profiles, partial attachment and reconnect. Whole-call SDK guards detect concurrent access and calls after close; teardown asserts balanced USB references and locks. |
| Rollback | `connection_rollback`, `registration_and_global_lock_failures`, `initialization_read_failures`, `optional_mode_query_failures`: required config/attribute and preset query failures roll back; optional mode failures hide the property. Failed reads leave outputs untouched, so tests cannot pass by consuming fake values after an error. |
| Properties/config | `published_property_contract`, `configuration_and_suffix`, `suffix_boundaries_and_failure`, `sensor_mode_capability_rebuild`, `bounded_attribute_and_mode_strings`: all five X_ properties, persistence, 0/15/16/17-byte suffixes, mode rebuilding and bounded camera/model/serial/config/mode strings. |
| Images | `image_formats_roi_and_bins`, `bin_boundaries_and_raw16_only`, `geometry_errors_and_recovery`, `frame_types_and_exposure_units`: deterministic generated RAW/RGB noise, all four SDK formats, RAW16-only mono capability, ROI and bin dimensions, alignment/bounds, setup/readback failures and recovery. RAW pixels, geometry and Bayer metadata are checked at the driver handoff; framework encoders are outside driver coverage. |
| Acquisition/streaming | `abort_setup_wait_and_removal`, `pending_frame_abort_orders`, `readout_deadline_and_recovery`, `stream_errors_and_active_removal`, `finite_stream_frame_count`: before-handler/setup/wait/readout aborts, both final-frame/abort orders, exact finite counts, indefinite streaming, watchdog and reacquisition. |
| Controls/polling | `controls_and_error_recovery`, `partial_controls_keep_readback`, `cooler_individual_failures`, `temperature_polling_and_disconnect`, `slow_initialization_and_polling`: individual read/write errors, accepted-value preservation after partial writes, egain synchronization, target-versus-measurement race, optional cooled profile and initialization spanning accelerated polling intervals. |
| Guiding | `simultaneous_axes_and_zero`, `pulse_replacement_and_disconnect`, `pulse_duration_at_sdk_entry`: four directions, independent axes, zero/replacement, failure/disconnect cleanup, imaging coexistence and 120 monotonic ON/OFF timing samples. |
| Queue/removal | `queued_discovery_shutdown`, `abort_setup_wait_and_removal`, `discovery_retry_lifetime`: queued discovery, rejected shutdown during initialization, retry cancellation, active readout/removal, fresh SDK ids after replug and no SDK access after close. |

Before scope reduction, all 45 cases were validated in normal, explicit safe-readout and ASan/UBSan builds (full earlier groups plus narrow reruns of the final added/strengthened cases). Sanitizers cover the test, driver and test-local framework objects; prebuilt libraries are not instrumented. No universal guider precision threshold is asserted.

The user approved bounded discovery retries in the generator. `sdk.discovery_retries` is opt-in; omitted attributes preserve existing behavior. The parser/architecture suite passes 86 attribute/value/block fixtures and its platform/fallback tests. Temporary regeneration of six reference drivers was inspected: wheel_sx is identical; wheel_playerone, wheel_asi, focuser_asi, rotator_asi and ccd_dsi differ only by the previously approved failed-registration/queue rollback code. No retry machinery appears in these non-opted-in drivers.

Driver cleanup also removes the unreachable suffix-handler epilogue and seven redundant OK assignments, plus redundant handled-property updates. Standard handlers use generated prologues/epilogues; finalizers retain explicit state ownership. Gain/offset and advanced controls preserve measured/accepted values until successful SDK writes/readback. Initialization and cooler helpers return failures before reading invalid SDK outputs. Unsupported binning is rejected after the base layer's documented range clamp, and a RAW16-only profile selects an actual supported default.

macOS arm64/x86_64 archive, dylib and standalone builds pass. Generation is idempotent; Xcode syntax and handwritten formatting checks pass. Clang `-Werror=unreachable-code`, `-Wall -Wextra` (excluding unused callback parameters), and the static analyzer report no driver findings. The fake-driver build retains the unreachable-code error check when using Clang.

### Physical acceptance and explicit deferrals

Mars-C II passes four SDK pixel formats, full frame and 256×256 ROI with bins 1/2, short/long exposures, streaming, guider commands, configuration roundtrip, and actual USB interruption while idle, streaming, exposing and guiding. Every replug is followed by a new image and guide command. The exact 16-byte suffix is written and verified in the replugged name, then cleared and the original empty suffix restored. The initial 17-byte test-fixture typo was correctly rejected before SDK writing; the fixture now has a compile-time length assertion. The dynamic-loader test performs actual `dlclose`/`dlopen` of the driver and obtains a fresh exposure; the ordinary static harness is explicitly labeled INIT/SHUTDOWN only. See `TESTING.md` for commands and evidence.

The user confirmed that only Mars-C II is available. Cooled/other models, OEM/Saturn hardware and multi-camera physical survival need those cameras. These are deferred acceptance requirements, not missing implementation steps or claimed passes. Optical color calibration/scene correctness is likewise not established by valid RAW images. Linux x64/ARM and Windows toolchains/runners are unavailable; the installed Docker client has no running daemon. No Linux/Windows build or runtime success is claimed. Future models/platforms must run the same tests under `indigo_test/DRIVER_TESTING_RULES.md`.

Scope correction: driver tests exclude framework encoders, video-container validation and upload-destination matrices. The reduced 44-case suite retains SDK pixel formats and RAW handoff assertions. The modified frame-type/exposure-unit, finite-stream-count and RAW/ROI/bin cases passed after this reduction. Hardware tests were not repeated.

The current fake SDK suite has 45 cases. Image inputs use generated noise, with exact Bayer-pattern mapping and continuous SDK exposure-mode assertions. All 45 normal fake SDK cases pass after this extension; hardware tests were not repeated.
