# Refactoring plan for INDIGO 3.0 ASI rotator driver

Goal: refactor the ZWO CAA rotator driver into a generator-friendly INDIGO 3.0 structure and migrate it to `indigo_generator`, following the completed `wheel_asi` and `focuser_asi` migrations. Preserve the public rotator interface, SDK identity, USB hot-plug and multi-device behavior while moving lifecycle and property boilerplate into generated code.

Status: Steps 1–7 implemented on 2026-09-07. Step 8 delivered the mandatory hardware-free regression suite: all 11 scenarios passed on native macOS arm64, with remaining coverage listed below. `.driver` is authoritative and reproduces the checked-in C/header/main; the driver and tests build for macOS x86_64/arm64. Step 9 build/documentation checks are complete; physical CAA validation remains pending because no CAA device is visible on the local host. The migration is not hardware-validated.

## Reference material

- `indigo_drivers/rotator_asi/indigo_rotator_asi.driver`: authoritative definition, version `0x03000004`; `.c`, `.h` and `_main.c` are generated. The handwritten baseline is recorded in Step 1.
- `indigo_drivers/wheel_asi/REFACTOR.md` and `indigo_drivers/focuser_asi/REFACTOR.md`: migration sequence and recorded results. Use their current `.driver` files as the implementation reference; do not repeat the wheel migration's intermediate direct-libusb identity workaround.
- `indigo_drivers/focuser_asi/indigo_focuser_asi.driver`: closest reference for CAA-style model/suffix parsing, motion finalizers, sync, abort, settings and global-lock ownership.
- `indigo_drivers/wheel_asi/indigo_wheel_asi.driver`: dynamic SDK product-id filtering and generated SDK discovery.
- `indigo_docs/DRIVER_GENERATOR_MIGRATION.md`, `indigo_docs/DRIVER_DEVELOPMENT_BASICS.md` and `indigo_docs/DEVELOPMENT.md`: extraction, generated lifecycle, bus properties and handler queues.
- `indigo_tools/indigo_generator.c`: authoritative generated SDK hot-plug, connection cleanup, property dispatch and finalizer semantics.
- `indigo_drivers/rotator_asi/bin_externals/libCAA/include/CAA_API.h`: SDK ids, angle units, motion status and error contracts. Read bundled documentation as needed; do not modify the vendor SDK.
- `README.md`, `TESTING.md`, `indigo_docs/MAKEFILES.md` and the driver `README.md`: build, supported platforms and physical validation.
- `indigo_docs/PROPERTIES.md`: documented `rotator_asi` property inventory.
- `indigo_test/integration/test_focuser_asi_sdk.c`, `test_wheel_asi_sdk.c` and their `indigo_test/Makefile` targets: existing hardware-free SDK replacement tests. These now exist even though the historical focuser migration results describe their earlier absence.

## Current public behavior to preserve

- Entry point/name: `indigo_rotator_asi`; label: `ZWO CAA Rotator`.
- One logical rotator per physical CAA; startup enumeration and USB hot-plug for vendor `0x03c3`, filtered through `CAAGetProductIDs()`.
- SDK device id assigned during discovery and retained across connection cycles. Names come from `CAA_INFO.Name`, with a parenthesized suffix rendered as `model #suffix`, followed by `indigo_make_name_unique()` using the SDK id.
- The handwritten driver has `MAX_DEVICES = 10`. Per user instruction, the migrated driver will use the generator default (currently 5), without a capacity override or generator changes.
- `INFO_PROPERTY->count = 6`, SDK model in the model item, SDK version in the firmware item with label `SDK version`.
- Standard properties and attach-time specialization:

  | Property | Current specialization / operation |
  | --- | --- |
  | `ROTATOR_POSITION` | Degrees, range 0–480, step 1; `CAAMoveTo()` for goto and `CAACurDegree()` for sync. |
  | `ROTATOR_RELATIVE_MOVE` | Visible, signed range −120–120, step 1; reads the current angle and requests an absolute SDK target. |
  | `ROTATOR_ON_POSITION_SET` | Visible; selects goto or sync through the base rotator behavior. |
  | `ROTATOR_DIRECTION` | Visible; `CAAGetReverse()` / `CAASetReverse()`. |
  | `ROTATOR_LIMITS` | Visible; initial minimum 0 and maximum 360, item ranges 0–480. Connection fixes the minimum to 0 and reads the maximum through `CAAGetMaxDegree()`. |
  | `ROTATOR_ABORT_MOTION` | `CAAStop()` followed by position/status reporting. |
  | `ROTATOR_BACKLASH` | Hidden. |

- Connected-only custom properties in `ROTATOR_ADVANCED_GROUP`:

  | Property | Schema and persistence |
  | --- | --- |
  | `CAA_BEEP_ON_MOVE` | Writable one-of-many switch; `ON` / `OFF`, defaults false / true; read/write through `CAAGetBeep()` / `CAASetBeep()`; saved by `CONFIG_SAVE`. |
  | `CAA_CUSTOM_SUFFIX` | Writable text with one `SUFFIX` item; maximum SDK payload 8 bytes; `CAASetID()`; naming change takes effect on replug. |

- Movement updates absolute and relative property states together, with approximately 0.5 s completion polling.
- Disconnect stops motion, closes the SDK handle and releases the INDIGO global lock.
- There is no implemented temperature property or temperature acquisition: the current temperature callback only reschedules itself.

## Target shape

- `indigo_rotator_asi.driver` with `driver asi`, label `ZWO CAA Rotator`, one `rotator { name = "%s"; ... }` block and generator-compatible `asi_open(indigo_device *device)` / `asi_close(indigo_device *device)` helpers.
- `sdk { hotplug = true; vid = ASI_VENDOR_ID; plug { ... } }`, with dynamic CAA product filtering inside `plug`. Do not copy the EAF fixed product id.
- Generator ownership of callback registration, driver-wide lifecycle queue, device storage, attach/detach, CONNECTION, custom property allocation/definition/deletion/release, CONFIG persistence and shutdown cleanup.
- ASI code owns SDK probing, naming, angle conversion/validation, motion/settings operations and SDK resource acquisition/release.
- Minimal private data: SDK id, model/suffix cache, required SDK information, current/target/maximum angle and explicit motion state where needed. Remove unused fields after checking every use.
- Per-device handlers perform SDK property work; a `rotator_move_finalizer()` polls motion. Remove the empty temperature loop and obsolete `gp_bits` connection alias.
- `.driver` becomes the source of truth; regenerate and retain `.c`, `.h` and `_main.c` together.

## Step-by-step plan

1. Establish a baseline and inventory
   - Record the source commit, dirty workspace state and baseline build output using `make -C indigo_drivers/rotator_asi -f ../../Makefile.drv`.
   - Inventory all `indigo_init_*_property()` / `indigo_init_*_item()` calls and every `count`, `hidden`, permission, range, default and configuration mutation. Compare with the base rotator implementation and `PROPERTIES.md`.
   - Record connect/probe/disconnect and failed-acquisition ownership: SDK handle, global lock, id reservation, timer, properties and device memory.
   - Record the existing capacity and use the generator default for the migration, as requested by the user; no capacity customization is required.

   Result (2026-09-07):
   - Baseline commit: `01183bd94fe423830b982fb0d029d554f1f102e6`. `git status --short` was empty before work and after the baseline build; no pre-existing tracked/untracked changes were present.
   - Environment: Darwin arm64, Apple clang 21.0.0 (`clang-2100.1.1.101`), existing repository build configuration targeting both x86_64 and arm64. The bundled SDK is `bin_externals/libCAA/lib/macOS/libCAA.a`, with headers under `bin_externals/libCAA/include`.
   - `make -C indigo_drivers/rotator_asi -f ../../Makefile.drv` succeeded (exit 0), but existing products were up to date. To verify actual compilation/linking without cleaning the SDK or touching sources, also ran `make -C indigo_drivers/rotator_asi -f ../../Makefile.drv -W indigo_rotator_asi.c -W indigo_rotator_asi_main.c`; it rebuilt both objects, the archive, dynamic library and standalone executable successfully (exit 0).
   - Compiler emitted no diagnostics. Both link steps reported existing SDK deployment-target mismatches: x86_64 CAA objects built for macOS 10.12 versus link target 10.10; arm64 CAA objects built for macOS 15.0 versus effective link target 11.0. `file` confirmed x86_64/arm64 universal archive, dylib and executable. No executable/server was started, and Linux/hardware operation was not tested. Existing build products were refreshed, not added to the source change.

   Property inventory (baseline source references):

   | Source | Confirmed inventory |
   | --- | --- |
   | `indigo_rotator_asi.c:129` (`rotator_attach`) | Exactly two custom property allocations and three item initializations, matching the custom-property table above. Both properties start OK/RW in `ROTATOR_ADVANCED_GROUP`; beep labels are `Beep on move`, `On`, `Off`; suffix labels are `Device name custom suffix`, `Suffix`, initialized from the discovery cache. |
   | `indigo_rotator_asi.c:135` | The only driver `count` mutation is INFO to 6. Model and SDK version are populated and firmware label becomes `SDK version`. No driver permission mutations exist. |
   | `indigo_rotator_asi.c:142` | All five visibility writes: limits visible, backlash hidden, relative move visible, on-position-set visible, direction visible. Attach-time ranges/defaults match the table above; minimum/maximum limit steps remain inherited at 1. |
   | `indigo_rotator_asi.c:219` | Connection fixes minimum limit value/target/min/max to 0; maximum value/target comes from `CAAGetMaxDegree()`. Position value is read into `target_position`; `current_position` and the public position target are not initialized from that read. Reverse and beep switches are read from the SDK with complementary normal/off values. |
   | `indigo_rotator_asi.c:79`, `:295` | Polling and motion/sync/abort update position value from the current-position cache. Requests copy property values/targets; goto/relative motion update private target. Limits changes read back only the maximum value, leaving requested target/cache handling for Step 5. Suffix writes currently update the cache before SDK success. |
   | `indigo_rotator_asi.c:120`, `:253`, `:274`, `:501` | Custom properties are enumerated/defined only when connected, deleted on disconnect and released on detach. |
   | `indigo_rotator_asi.c:490`; `indigo_libs/indigo_rotator_driver.c:274` | CONFIG_SAVE saves beep then falls through to the base rotator, which attempts to save steps/revolution, backlash and limits. The common save helper (`indigo_libs/indigo_driver.c:333`) skips hidden/read-only properties, so hidden steps/backlash are not saved. Suffix is stored through the SDK, not explicit driver CONFIG persistence. Calibration load/save is inactive while position offset is hidden. |
   | `indigo_libs/indigo_rotator_driver.c:98` | Base position/relative defaults are 0; goto defaults true, sync false; normal direction defaults true, reversed false; abort is a one-item at-most-one switch defaulting false. The driver overrides the base position range −180–360, relative range −180–180 and limit ranges/default minimum as listed above. |
   | `indigo_libs/indigo_rotator_driver.c:98`, `:154`, `:161` | Unmodified inherited properties remain hidden: steps/revolution (1–3600, step/default 1/3600), raw position (RO, −90–360, step/default 0/0), position offset (RW, −90–360, step/default 0/0). Hidden backlash remains 0–999, step/default 0/0. Base connected-property definition/deletion and on-position-set copy-only behavior remain applicable. |
   | `indigo_docs/PROPERTIES.md:1361` | Both custom property names and all listed driver-specific inherited properties agree with the source. No property was added/removed or behavior changed, so no reference edit is required in Step 1. |

   Resource/lifecycle baseline (ownership to preserve or explicitly correct in later steps):

   | Path / source | Acquisition and release behavior |
   | --- | --- |
   | Discovery, `indigo_rotator_asi.c:541`, `:623` | Enumeration mutex covers free-slot lookup, SDK enumeration, probe and attach. `find_plugged_device_id()` marks `connected_ids[id]` before opening; this is an attached/discovered-id reservation, not connection state. Probe uses no INDIGO global lock; successful open is closed after property success or non-moving failure. Open/probe failures leave the reservation set. Moving errors retain the handle/mutex during an unbounded 1 s retry loop. SDK ids index arrays without bounds checks. |
   | Attach, `indigo_rotator_asi.c:129`, `:667` | Hot-plug allocates device/private data; base attach owns the rotator context/properties, custom attach initializes `usb_mutex` and allocates beep/suffix. The attach return value is ignored before storing `devices[slot]`; partial custom allocation failures have no explicit local unwind. |
   | Connect, `indigo_rotator_asi.c:186` | Enumeration mutex protects id lookup; device mutex protects global-lock acquisition and SDK open/initial reads. Global-lock failure releases both mutexes and publishes alert/disconnected. Failed SDK open releases the global lock and later enumeration mutex. Successful open retains handle/global lock until disconnect; failed initialization reads are logged but do not reject connection. Missing id lookup does not explicitly switch to alert/disconnected. |
   | Timers, `indigo_rotator_asi.c:79`, `:111`, `:255`, `:306` | Connection worker has an unretained zero-delay timer. Retained motion timer starts at 0.5 s and reschedules while moving; retained no-op temperature timer starts at 0.1 s and repeats every 2 s. Movement starts another motion timer; abort cancels it without synchronous waiting. Hot-plug/unplug dispatch uses unretained 0.5 s timers with a NULL device. |
   | Disconnect/detach, `indigo_rotator_asi.c:270`, `:501` | Disconnect synchronously cancels retained motion/temperature timers, deletes custom properties, locks enumeration/device mutexes, calls stop/close, releases the global lock and mutexes, then clears legacy connection state. Stop result is overwritten by close result. Detach disconnects when needed, releases custom properties and delegates base property/context cleanup; no `pthread_mutex_destroy()` exists for the device mutex. |
   | Unplug/shutdown, `indigo_rotator_asi.c:576`, `:693`, `:744`, `:789` | Unplug enumeration clears the absent id reservation, removes the array slot and drops enumeration mutex before detach/free. Shutdown requires disconnected devices, deregisters libusb callback, detaches/frees stored devices and clears all reservations. Unretained plug/unplug timers have no explicit cancellation here. |

   Capacity decision:
   - `write_c_define_section()` (`indigo_tools/indigo_generator.c:1330`) emits `MAX_DEVICES = 5`; generated device storage and lifecycle loops use this value.
   - Per subsequent user instruction, keep this generator default. Do not add `#undef MAX_DEVICES`, a replacement definition or a generator extension. The change from the handwritten limit of 10 is intentional.
   - Step 8 capacity coverage will follow the generator default, including rejection beyond capacity and reuse of a released slot. Preserving ten-device capacity is not a migration requirement.

2. Reshape the handwritten driver into extractable sections
   - Separate includes/defines, private data, shared SDK helpers, rotator helpers, property handlers/finalizers and temporary manual hot-plug support.
   - Keep structural moves behavior-preserving; make necessary behavior corrections explicit in subsequent steps.
   - Preserve the license and version history, extend the copyright through 2026 and append the required OpenAI Codex refactoring notice after the license header.
   - Follow repository formatting for handwritten C and `.driver` blocks, without unrelated churn.

   Result (2026-09-07):
   - Organized `indigo_rotator_asi.c` into driver definitions, includes, ASI definitions, private data, temporary manual hot-plug state, SDK identity helpers, rotator polling callbacks, lifecycle/property handlers, temporary manual hot-plug support and the driver entry point.
   - Moved `find_index_by_device_id()` and `split_device_name()` before the rotator callbacks and removed the now-unnecessary forward declaration. Grouped the existing hot-plug arrays, constants, callback handle and enumeration mutex together; their values and ownership are unchanged.
   - Preserved the license/version history and driver version, extended copyright to 2024–2026 and added the OpenAI Codex refactoring notice. Applied required handwritten formatting: one blank line between functions, no blank lines inside bodies, braces around existing single-statement conditionals, single-line initializer macro call, and consistent operator/comma/initializer spacing.
   - Compared all 18 function token sequences against the pre-step source, excluding comments, whitespace and braces; operations and literals matched. Inspected the added braces in the diff to confirm unchanged control flow. Custom property initialization and all count/visibility mutation lines are unchanged, so `PROPERTIES.md` needs no update.
   - `make -C indigo_drivers/rotator_asi -f ../../Makefile.drv` compiled the driver and linked the archive, dylib and executable successfully for x86_64/arm64. Only the same bundled CAA SDK deployment-target warnings recorded in Step 1 remain. `git diff --check` passed.
   - This step does not change SDK behavior, timers, locking, motion handling or the existing handwritten capacity. The future generated driver will use the generator default as requested. Open/close extraction and behavior corrections remain in Steps 3–5; mandatory hardware-free regression tests remain in Step 8.

3. Introduce SDK open/close helpers and generated discovery design
   - `asi_open()` acquires the INDIGO global lock and opens the already-associated CAA id; all failed acquisition paths release only resources actually acquired.
   - `asi_close()` stops motion, closes the handle, logs failures and releases the global lock exactly once.
   - Populate and validate the CAA product list in `on_init`; reject failed/empty enumeration rather than registering an ineffective driver.
   - In `sdk.plug`, validate `CAAGetID()` results against `0 <= id < CAA_ID_MAX`, skip ids already attached in generated `devices[]`, probe with `CAAOpen()` / `CAAGetProperty()` / `CAAClose()`, and cache the id/model/suffix before attach.
   - Parse names with bounded, terminated copies; preserve parenthesized CAA suffix handling rather than copying the wheel SDK's `#` parser.
   - Follow the current `wheel_asi` / `focuser_asi` plug/unplug mechanism exactly, as requested by the user: skip attached SDK ids, probe each candidate once and skip failures; remove `connected_ids[]` without introducing replacement reservations or a custom retry mechanism.
   - Use the same libusb-reference ownership and event-device removal as those references. Validate simultaneous arrivals, SDK enumeration reordering and removal of either device in Step 8 and hardware checks; do not claim physical identity validation from source inspection alone.
   - Remove the unbounded `CAA_ERROR_MOVING` sleep loop. A failed probe is skipped, including moving errors, as in the reference drivers. Let the generated SDK framework own the final hot-plug queue and attach/detach scaffolding.

   Result (2026-09-07):
   - Added `asi_open(indigo_device *device)` and `asi_close(indigo_device *device)` and routed the existing connection callback through them. The caller retains enumeration locking and the helpers retain per-device locking until Step 4's queue conversion. Open checks the cached id, acquires the global lock and releases it on SDK-open failure. Close logs stop/close failures independently and releases the global lock. Missing devices now produce alert/disconnected instead of leaving a successful connection indication.
   - Validated the product count using the SDK-documented `CAAGetProductIDs(NULL)` query before filling the fixed product array; rejected empty/oversized lists. Driver initialization records success only after queue creation and libusb callback registration succeed; failed registration releases the queue.
   - Replaced SDK reservation and removed-id searches with the current EAF/EFW mechanism: validate SDK ids, scan attached `devices[]`, open/probe/close one candidate at a time, then retain its id/model/suffix and event `libusb_device *`. No retry counters, delayed probe retries, `connected_ids[]` or blocking sleep remain.
   - Copied the reference USB callback and unplug handler behavior, including queue dispatch with retained libusb references and removal by the stored USB device. Both function bodies match the current `focuser_asi` generated C verbatim. Product filtering remains dynamic like `wheel_asi`, rather than copying the EAF product id. Manual hot-plug scaffolding remains temporary until `.driver` extraction/regeneration in Steps 6–7.
   - Name parsing now bounds and terminates model/suffix copies, validates parenthesis order and trims model/suffix-leading spaces like the EAF reference. Published naming remains `model #suffix`, with an eight-byte suffix and SDK-id uniqueness.
   - Inspected the source diff and confirmed motion, temperature and property-change function bodies are unchanged. Property allocation/count/visibility contracts are unchanged; `PROPERTIES.md` needs no update. `make -C indigo_drivers/rotator_asi -f ../../Makefile.drv` built successfully for x86_64/arm64, with no compiler diagnostics and only the baseline bundled-SDK deployment-target linker warnings. `git diff --check` passed.
   - Runtime failure injection, simultaneous arrivals, enumeration reordering and physical-device identity have not yet been tested. They remain mandatory Step 8 / Step 9 validation. The implementation follows the requested reference mapping and does not introduce a separate CAA identity algorithm or generator changes.

4. Move lifecycle and properties onto handler queues
   - Replace zero-delay connection dispatch and direct SDK work in bus callbacks with queued handlers; replace delayed motion timers with `indigo_execute_handler_in()`.
   - Use `IS_CONNECTED` for established connection state and the CONNECTION switch for requested state. Remove `device->is_connected` / `gp_bits` use.
   - Map successful connection reads of maximum angle, position, reverse and beep to `rotator.on_connect`; initialize current position, target and published values consistently before accepting motion.
   - Use generator-provided `connection_result`. Classify essential connection reads versus optional setting reads; failed essential initialization must leave CONNECTION alert/disconnected with no leaked open handle or lock. Inspect generated failure cleanup to avoid missing or double `asi_close()` calls.
   - Do not return from `on_connect` or `on_disconnect`; let the generator complete lifecycle and property cleanup.
   - Represent custom properties declaratively; use `persistent = true` for beep and preserve connected-only visibility. Use inherited visibility attributes where supported; keep dynamic limits in attach/connect code.
   - Let base behavior handle `ROTATOR_ON_POSITION_SET` unless an explicit copy-only handler is needed; use empty `on_change { }` for copy-only handling.
   - Remove mutexes only after checking serialization between the driver lifecycle queue, device handlers and in-flight disconnect/unplug work. Cancellation and lifecycle serialization must prevent SDK access after close. Follow `wheel_asi`: keep the disconnected guard in delayed finalizers, not in every property handler.

   Result (2026-09-07):
   - Extracted seven SDK property handlers (direction, absolute/relative motion, limits, abort, beep and suffix). The bus change callback now checks connection/busy state, copies requests and dispatches with `INDIGO_COPY_VALUES_PROCESS_CHANGE()`; it performs no CAA SDK calls. Absolute and relative requests share a pre-copy busy guard. Detailed motion/error-state corrections remain Step 5.
   - CONNECTION runs on the same driver queue/mutex as hot-plug. The connection handler additionally locks `DEVICE_CONTEXT->device_mutex`, the mutex automatically held by device handlers, so disconnect waits for any active device SDK operation before cancelling pending handlers and closing the SDK. Enumeration locking remains in the handwritten discovery/connection code until generation.
   - Removed the redundant per-device `usb_mutex`, including the unmatched limits-handler unlock. Removed `gp_bits` / `device->is_connected`; established-state guards use `IS_CONNECTED`. A separate `sdk_open` flag records SDK/global-lock ownership only, making close idempotent and allowing detach to clean up even when CONNECTION has already changed to busy/disconnected.
   - Connection initialization uses `connection_result` and treats maximum angle, position, reverse and beep reads as required, matching the EAF initialization policy. Invalid/non-finite angle reads fail initialization. On failure after open, `asi_close()` releases acquired resources before CONNECTION becomes alert/disconnected. On success, private current/target and public position value/target are initialized together before properties are exposed.
   - Per user instruction, ordinary property handlers have no repeated `if (!IS_CONNECTED) { return; }` prologue, following `wheel_asi`. The delayed finalizer retains its connection guard.
   - Converted motion polling to guarded `rotator_move_finalizer()` scheduling with `indigo_execute_handler_in()`; abort cancels its pending handler. Disconnect cancels pending device handlers; detach also removes pending CONNECTION work for that device from the driver queue before releasing properties. Removed the empty temperature callback and its timer early because they performed no SDK work and exposed no property.
   - Preserved connected-only custom properties, CONFIG_SAVE persistence of beep, inherited visibility and base `ROTATOR_ON_POSITION_SET` behavior. Their declarative `persistent`/visibility attributes will be written during Step 6 extraction; no partial `.driver` is introduced before that step. The current connection body is prepared for transfer into generated lifecycle blocks without early returns.
   - `make -C indigo_drivers/rotator_asi -f ../../Makefile.drv` compiled/linked successfully for x86_64/arm64; only the baseline CAA SDK deployment-target warnings remain. Source checks confirmed seven queued SDK branches, no SDK calls in `rotator_change_property`, no legacy timer API/connection alias/USB mutex, and unchanged property initialization/count/visibility lines. `git diff --check` passed.
   - This validation is compilation and source inspection, not runtime coverage. Failure injection, queue/disconnect races and bus-level regression assertions remain required in Step 8. No public property was added or removed, so `PROPERTIES.md` is unchanged.

5. Implement rotator motion and explicit error handling
   - Map `ROTATOR_POSITION`, `ROTATOR_RELATIVE_MOVE`, `ROTATOR_ABORT_MOTION`, `ROTATOR_LIMITS`, `ROTATOR_DIRECTION`, beep and suffix to their respective `on_change` blocks.
   - Preserve angle units and distinguish current value from requested target. Inspect generated target-copy behavior, using `preserve_values` where appropriate.
   - Rely on framework validation for incoming property values; check only effective device limits for motion targets (including the computed relative target); establish a documented reject/clamp policy for relative targets outside the limits. Do not add angle wrapping or mechanical-angle APIs as an incidental change.
   - Use explicit motion state to guard overlapping absolute/relative requests; generated handlers may reset a property's state before custom code runs, so do not rely solely on the old BUSY check.
   - After successful `CAAMoveTo()`, publish both movement properties busy and schedule one finalizer. A failed move start must alert both immediately without scheduling a success completion.
   - The finalizer initializes status variables, checks `CAAIsMoving()` and `CAAGetDegree()` independently, preserves errors and updates only confirmed position values. Account for both motor and hand-controller status according to the CAA SDK contract; do not complete solely on exact float equality.
   - Sync publishes success only after `CAACurDegree()` and position readback succeed. Abort resets the switch, checks stop/readback and confirms completion; failed stop must not falsely mark the movement successful or discard necessary tracking.
   - Correct the limits handler: pass the requested maximum to `CAASetMaxDegree()`, read back the confirmed maximum and maintain value/target/cache consistency. Remove its unmatched mutex unlock. Keep the fixed minimum at zero; the SDK has no minimum-angle setter.
   - Update cached suffix only after `CAASetID()` succeeds; restore the previous value on failure. Preserve empty suffix and the 8-byte boundary. Use correct floating-point logging formats and normal debug logging on successful SDK calls.
   - Explicit updates are required on early returns and when `_finalizer` suppresses the generator's epilogue. Verify every success/failure branch publishes a terminal or deliberately busy state.
   - Remove the no-op temperature callback and its timer without adding a new public property.

   Result (2026-09-07):
   - Added shared position-read, motion-state publication, readiness, target-validation and move-start helpers. Incoming property values are validated by the framework. Per user instruction, handlers do not duplicate finiteness/item-range checks. Motion targets, including computed relative targets, are checked against confirmed device limits without wrapping or clamping.
   - Motion readiness reads SDK motor/hand-controller state and uses private `moving` / `abort_pending` state rather than relying on a generated property's initial state. Rejected requests during motion preserve the active target and keep completion polling scheduled. Ordinary property handlers have no connection-guard prologue; only the delayed finalizer retains the guard, following the requested `wheel_asi` pattern.
   - Failed move starts alert both motion properties and schedule no completion. Successful starts publish current value separately from target, mark both properties busy and schedule one finalizer. The finalizer checks status and position independently, publishes only confirmed position values, and does not overwrite a read error with OK. Completion depends on both SDK movement flags becoming false, not float equality with the target.
   - Sync checks `CAACurDegree()` plus readback and reports success only when both succeed. Abort resets its switch and checks stop/status/readback; failed stop remains alert while ongoing motion continues to be tracked. Hand-controller motion remains busy and produces an abort alert with a release-controller message, consistent with the SDK contract that `CAAStop()` cannot stop it.
   - Limits rely on framework validation of the requested values and send the requested maximum to `CAASetMaxDegree()`, validate readback and synchronize maximum value/target/cache from the last confirmed value. The minimum remains zero. SDK failures stay alert even if another call succeeds. The unmatched mutex unlock and empty temperature callback were already removed in Step 4.
   - Suffix validation/write failures restore the confirmed cached text. Successful writes update the bounded cache only afterward; empty and exactly eight-byte suffixes are supported. Floating-point SDK logs use floating-point formats and successful suffix writes use debug logging.
   - Built the driver archive, dylib and executable for x86_64/arm64 successfully; only baseline SDK deployment-target linker warnings remain. A temporary standalone harness extracted the affected helpers and exercised them with CAA SDK stubs under AddressSanitizer/UndefinedBehaviorSanitizer: invalid targets, relative limit rejection, failed start/status/position reads, busy-at-target behavior, sync failure/success, failed stop, hand-controller abort, maximum write/readback failures and suffix rollback/boundaries all passed. Temporary harness files were removed.
   - These checks validate helper logic, not the public bus, real queues or USB lifecycle. The required SDK-replacement integration suite remains Step 8. Updated `PROPERTIES.md` to describe the intentional motion/limits/suffix behavior changes; public property names and item inventory are unchanged. `git diff --check` passed.

6. Annotate and extract the initial `.driver`
   - Add `//+ include`, `define`, `data`, `code`, `on_init`, `rotator.code`, lifecycle and property `on_change` annotations as required by the extraction guide.
   - Leave temporary manual callbacks, device arrays, mutex scaffolding and shutdown removal outside the final extracted code blocks.
   - From `indigo_drivers/rotator_asi`, run `indigo_generator -c indigo_rotator_asi.driver`. The argument is the new `.driver` output path, never the existing `.c` file.
   - Inspect and complete metadata, SDK blocks, inherited/custom properties, persistence and helper names. Check void-handler returns and collisions with generated helper/property names.
   - Generate into a temporary directory first to inspect the shape. Include `<stdbool.h>` before `<CAA_API.h>` if required by SDK header ordering.

   Result (2026-09-07):
   - Added extraction annotations to the handwritten C without changing its executable code, then ran `indigo_generator -c indigo_rotator_asi.driver` and completed the extracted definition. Preserved the license, Codex notice, version, label and author.
   - Used `sdk { hotplug = true; vid = ASI_VENDOR_ID; plug { ... } }` with the wheel-style dynamic product filter and SDK enumeration/probe pattern. Unplug uses the generator implementation, as in wheel/focuser; no custom retry, arrays, mutexes or capacity override are carried into the definition. The temporary generated output uses the default `MAX_DEVICES = 5`.
   - Declared inherited visibility, connected-only custom properties and beep persistence. Position, relative movement and limits use `preserve_values = true`; inspected the generated target-copy branches. Kept `ROTATOR_ON_POSITION_SET` handling in the base driver. Generator-owned USB/property pointers, allocation, definition, deletion and release are omitted from custom data/code.
   - Moved SDK initialization into `on_connect` using `connection_result`, preserving explicit close on failed initialization. Ordinary setting handlers rely on generated OK/update handling; suffix success sends its message before the generated update. Motion handlers publish their own states, and their `_finalizer` references suppress the normal epilogue. Relative movement explicitly cancels the pending finalizer before the shared move-start helper. No duplicate framework input checks or per-handler connection guards were added.
   - Generated C/header/main only in a temporary directory and built the archive, dylib and executable for x86_64/arm64 with the normal driver compiler/linker flags. Compilation and linking passed, with only the existing vendor SDK deployment-target warnings. Inspected generated property names, handler epilogues, target copies and SDK hot-plug shape; `git diff --check` passed. Temporary generated files and build products were removed.
   - Repository regeneration remains Step 7. The generated lifecycle replaces the handwritten connection/device mutex and detach cancellation scaffolding with the same framework path used by wheel/focuser; disconnect/unplug concurrency and resource balance still require the Step 8 bus-level tests. This step provides build/source validation, not runtime or hardware coverage.

7. Regenerate and integrate
   - Run `indigo_generator indigo_rotator_asi.driver`; from this point edit only `.driver` for driver behavior.
   - Build the narrow driver target and inspect all generated changes, including `.h` and `_main.c`.
   - Remove the replaced handwritten hot-plug framework, id/slot lookup scaffolding, timer handles, redundant property pointers and manual property lifetime code from the generator input.
   - Include the `.driver` in Xcode/project metadata where repository conventions require it; inspect automatic project edits for unrelated changes and local paths.
   - Regenerate again to confirm reproducibility; retain no temporary files or build products in the change.

   Result (2026-09-07):
   - Regenerated the repository C/header/main from `indigo_rotator_asi.driver`. All subsequent driver changes belong in that definition. The original license and Codex refactoring notice remain in the definition; generated files use the generator's standard license/header template.
   - Replaced the temporary handwritten lifecycle, property ownership, dispatch and hot-plug scaffolding with generated code. The generated unplug handler and libusb callback match `wheel_asi` byte-for-byte. SDK discovery keeps the same single-probe enumeration pattern; capacity uses the generator default of five, without generator changes.
   - Inspected the complete generated diff: custom property names/items/defaults and beep persistence are preserved; explicit inherited visibility matches the base defaults. Numeric dispatch now copies targets, standard setting handlers use generated state/update handling, and motion handlers retain their own completion updates. The public entry point/version remain unchanged; the public header now exposes only the driver API, as in wheel/focuser.
   - Generated lifecycle semantics replace the temporary manual locking/detach cleanup and init/shutdown scaffolding, including standard connection messages, the generator's `SET_DRIVER_INFO` flag and initialization action/error handling. Runtime failure injection and lifecycle concurrency validation remain Step 8; no handwritten replacement of the wheel/focuser framework was added.
   - The Xcode group already references `.driver` with a relative path; no further project or Makefile edits were needed. Updated `PROPERTIES.md` source ownership, with no property inventory changes.
   - `make -C indigo_drivers/rotator_asi -f ../../Makefile.drv` successfully compiled/linked the archive, dylib and executable for x86_64/arm64. Only the existing vendor SDK deployment-target linker warnings remain. A second generator run produced byte-identical C/header/main, and `git diff --check` passed. No build products are included in the change.
   - No rotator SDK integration target exists yet. The mandatory production-object/SDK-replacement regression suite is the next step, not replaced by this build verification.

8. Create and run mandatory hardware-free regression tests
   - Creating these regression tests is part of the refactoring deliverable, not optional follow-up work. Use the same SDK replacement mechanism as `test_wheel_asi_sdk.c` and `test_focuser_asi_sdk.c`; lack of physical CAA hardware must not defer their implementation or execution.
   - Follow `indigo_test/AGENTS.md`; implement test work within `indigo_test`, separately from production edits.
   - Add `integration/test_rotator_asi_sdk.c` and a narrow Makefile target modeled on the current EAF/EFW tests: compile the production driver separately and replace CAA/libusb/lock hooks in the test build. Exercise public bus APIs and the public driver entry point, without real USB hardware or vendor binary calls.
   - Compile `indigo_rotator_asi.c` into a dedicated test object using the bundled CAA header and test-local preprocessor replacements for USB discovery, device attach/detach and global-lock hooks. Supply stateful CAA SDK function implementations in the test executable and link against the real INDIGO library, without linking the vendor CAA binary. Do not include production `.c` directly or add test-only branches to the production driver.
   - Simulate USB arrival/removal through the captured hot-plug callback, control SDK enumeration and motion state, inject SDK failures and count resource acquisitions/releases as in the EAF/EFW harnesses. Use `test_runner.h` and `simulator_test_common.h` for assertions, property observation and bounded asynchronous waits.
   - Register `test_rotator_asi_sdk` in `INTEGRATION_TESTS` so it runs under the standard `test-integration` and `test` targets, as well as through its narrow build/run command.
   - Cover metadata, init/enumeration, custom schemas/visibility/persistence, repeated connect/disconnect, failed open and initialization cleanup, wrong products/invalid ids, duplicate arrivals, capacity, probe/attach failure and multi-device removal/replug.
   - Cover absolute/relative movement, overlapping commands, sync, stop and hand-controller status, failed start/status/position reads, limits write/readback failures, reverse, beep, suffix boundaries and failed suffix rollback.
   - Cover disconnect/unplug while polling or probing, pending work at shutdown and balanced SDK open/close/global-lock ownership. Use bounded waits and clean teardown.
   - Record implemented coverage and deferred cases in `indigo_test/CHANGES.md`; run the narrow target, then `make -C indigo_test test-clean` as required. An SDK mock validates driver logic, not vendor SDK timing or USB identity on hardware.

   Result (2026-09-07):
   - Added `indigo_test/integration/test_rotator_asi_sdk.c` and its narrow build target; registered the executable in `INTEGRATION_TESTS` for the standard `test-integration` and `test` targets. Used the same production-object/SDK-replacement mechanism as wheel/focuser, with the real INDIGO library and no vendor CAA binary or hardware access. No production driver or generator changes were required.
   - Implemented 11 independent lifecycle scenarios covering metadata/schema/visibility, repeated connection, failed open and all required initialization reads, fractional absolute/relative movement, overlapping requests, effective limits, move/status/position failures, sync/readback, abort and hand-controller behavior, limits/settings failures, beep persistence selection, suffix boundaries/rollback/replug, discovery failures, duplicate arrival, capacity/slot reuse and multi-device removal.
   - Covered disconnect/unplug with pending motion polling and unplug queued behind a bounded SDK probe. Every scenario checks balanced SDK opens/closes, global locks and USB references; cleanup also runs after test assertions fail. Beep persistence uses test-local save/base-dispatch hooks to observe CONFIG_SAVE selection without writing user configuration; disk serialization is not tested.
   - `make -C indigo_test build/integration/test_rotator_asi_sdk` compiled the test and separate production object for x86_64/arm64. `indigo_test/build/integration/test_rotator_asi_sdk` passed all 11 scenarios on native macOS arm64, including suffix naming after replug and continued BUSY at the target angle while the SDK reports motor activity. Expected error logs came from injected SDK failures.
   - Recorded detailed coverage in `indigo_test/CHANGES.md`. `git diff --check` passed and `make -C indigo_test test-clean` removed test build artifacts.
   - Remaining automated coverage: forced overlap of an executing device handler with disconnect, shutdown with queued/in-flight hot-plug work, callback registration/queue allocation failures, invalid startup product counts and commands racing the initial delayed SDK read. Vendor timing and physical USB-to-SDK identity require hardware. Linux/x86_64 execution and the full test suite were not run; these limits are also recorded in `CHANGES.md`.

9. Verify on hardware and close the migration
   - On a physical CAA, check startup discovery, hot-plug, reconnect, goto, relative motion, sync, abort, hand-controller interaction, angle limits, reverse, beep and suffix persistence after replug.
   - With multiple CAA units, verify correct names and physical-device control regardless of connection order, simultaneous arrival and removal of either unit. Check delayed SDK readiness without blocking other devices.
   - Rebuild with the bundled SDK; separate pre-existing linker warnings from new failures. Validate supported Linux/macOS builds where available and record untested platforms explicitly.
   - Update `PROPERTIES.md` in the same implementation change if properties/items are added or removed, or documented visibility/behavior changes; update source notes to reflect `.driver` ownership. Update the driver README only for changed operational behavior.
   - Run `git diff --check` and inspect the complete scoped diff. Keep review findings in the repository's `REVIEW.md` workflow and test plans/results in `indigo_test/CHANGES.md`; this file tracks migration steps/results.

   Result (2026-09-07, partial — hardware validation pending):
   - Rebuilt the production archive, dylib and executable with the bundled CAA SDK using `make -C indigo_drivers/rotator_asi -f ../../Makefile.drv`. macOS x86_64/arm64 compilation and linking passed. Only the existing vendor deployment-target linker warnings remain (SDK objects targeting newer macOS versions); no new compiler errors or warnings appeared.
   - Re-ran the generator and compared C/header/main byte-for-byte with the checked-in files: all matched. `git diff --check` passed. No generated build products are included in the change.
   - Confirmed `PROPERTIES.md` already records the current custom property inventory, motion/limits/suffix behavior and `.driver` source ownership. Added user-facing motion-limit, hand-controller abort and suffix behavior to the driver README; no public property or driver behavior changed in this step.
   - Inspected the local USB registry with `ioreg -p IOUSB -l -w 0`; it exposed no peripheral vendor/product entries and no visible CAA device. Consequently startup discovery, physical replug/reconnect, goto/relative/sync/abort, hand-controller interaction, settings and suffix persistence were not tested on hardware. Multi-unit identity/order/removal and delayed SDK readiness remain unverified.
   - The Step 8 hardware-free suite previously passed all 11 scenarios on native macOS arm64. This step does not replace the outstanding automated concurrency/failure coverage listed there with a hardware-validation claim. Linux builds/execution and x86_64 execution remain untested.
   - Step 9 remains open for physical CAA validation; the migration is not marked fully complete. One CAA is required for the single-device checks and at least two for physical identity and simultaneous hot-plug checks.

## Completion criteria

- `.driver` is authoritative and reproduces the checked-in C/header/main; the driver builds successfully.
- Generated SDK lifecycle and property ownership replace the handwritten framework, with no SDK work in bus callbacks or unbounded waits on queues.
- Public names, property schemas, persistence and multi-device identity/capacity are preserved or explicitly documented as intentional changes.
- Motion and failure paths publish accurate states; connection, retries and unplug/shutdown leave no stale SDK handles, locks or queued device access.
- `indigo_test/integration/test_rotator_asi_sdk.c`, its Makefile integration and coverage notes are delivered with the refactoring, and the hardware-free regression suite passes using the same SDK replacement mechanism as `wheel_asi` and `focuser_asi`. Physical CAA checks and any unresolved SDK/generator assumptions are recorded explicitly; compilation alone is not hardware validation.
