# Partial refactoring plan for ccd_touptek

Date: 2026-09-08.

Status: Steps 1–7 completed for the available macOS build environment and physical guiding cameras on 2026-09-08. Eleven branded variants build for arm64/x86_64; native regression, queue and AddressSanitizer checks pass. ToupTek and Altair hardware workflows pass, including physical Altair unplug/replug under streaming load and SDK unload/reload. Unavailable platform/hardware validation is explicitly deferred below. Rosetta runs require a separate user request and were not repeated in step 7.

## Goal and scope

Refactor the handwritten `indigo_ccd_touptek.c` to use INDIGO handler queues and a persistent driver-wide queue for SDK discovery, hot-plug and connection lifecycle, following the current `wheel_asi`, `focuser_asi` and `rotator_asi` implementation model.

Do not migrate to `indigo_generator`, create a `.driver` file, modify the generator or replace vendor SDKs. Preserve public device names, properties, configuration, camera/guider sharing, filter-wheel and focuser behavior. Include lifecycle fixes required to make the queue conversion safe; avoid unrelated feature changes.

The source is shared by multiple branded drivers through `#include` and SDK macros. Inventory all consumers, including Altair, Baccam, Bresser, OmegonPro, StarshootG, Rising, Mallin, Meade, Ogma and SVBony, before editing. A source change must remain compatible with their available SDK variants.

## Minimal-change constraint

Explicit user instruction: make only changes necessary for queues and safe hot-plug; preserve the driver's overall logic. This constraint governs all steps below.

- Move existing operation bodies into handlers while preserving their branching, SDK call order, validation, messages and property transitions.
- Preserve current SDK-id/OEM discovery, device naming, shared-handle ownership and `gp_bits` unless a specific queue/lifetime defect requires a narrowly scoped correction.
- Do not redesign exposure, streaming, frame processing, configuration or connection accounting as part of this task. Add guards or synchronization only where the execution-context change demonstrably requires them.
- Updated user requirement during step 3: eliminate all driver-owned timers and custom mutexes. SDK callbacks only enqueue notifications; all SDK operations and image processing execute on driver/device queues. Preserve the original settling intervals through delayed handlers.
- Do not centralize helpers, rename unrelated functions, remove working locks, coalesce events or reformat code merely for cleanup. Each behavioral change must have a concrete queue or hot-plug safety justification.
- Implement and validate in small steps. If an independent pre-existing bug is found, record it separately rather than expanding this refactor.

## References

- `indigo_docs/TIMERS_AND_QUEUES.md` and `indigo_docs/DRIVER_DEVELOPMENT_BASICS.md`: queue execution, cancellation, master-device routing and lifecycle.
- `indigo_docs/DEVELOPMENT.md`, `README.md`, `TESTING.md` and `indigo_docs/MAKEFILES.md`: bus semantics, build and validation.
- Current ASI `.driver` and generated `.c` files: implementation references only; use their final driver-queue approach, not an intermediate migration state.
- `indigo_drivers/REVIEW.md`: existing ToupTek findings, especially SDK callback and disconnect risks. Record new findings here without advancing the whole-folder review baseline.
- Bundled SDK headers: callback restrictions, Stop/Close behavior, enumeration and device identity. Do not assume ASI SDK contracts apply to ToupTek.
- `indigo_test/AGENTS.md` and the ASI SDK replacement integration tests: hardware-free validation conventions.

## Baseline structure and risks to address (before refactoring)

- `process_plug_event()` scans `EnumV2()` and the ToupTek OEM inventory, reconciles SDK ids, attaches CCD/guider, wheel or focuser devices, and removes missing devices. Every libusb arrival/removal currently schedules a fresh timer callback after 0.5 seconds.
- An enumeration mutex serializes discovery with selected Open/Close calls, but does not provide a stable execution thread or ownership of pending hot-plug work during shutdown.
- Four connection callbacks run on timer threads. Camera and guider share one SDK handle and private data; connection ownership currently uses `gp_bits`.
- Property callbacks contain synchronous SDK calls on the bus path. Polling, guide-pulse completion and exposure watchdog use timers.
- The SDK image callback pulls and processes frames and schedules completion/abort/video-stop callbacks. Some SDK operations can wait for the callback thread, so moving code or adding a mutex can introduce a deadlock.
- Disconnect, queued completions, SDK callback termination, buffer release and shared-handle close require an explicit ordering. Preserve independent guider operation when the CCD disconnects.

## Target execution model

| Work | Execution context |
| --- | --- |
| SDK enumeration, discovery probes, attach/detach, Open/Close and all CONNECTION handlers | One persistent queue per compiled driver instance, shared by all its physical devices |
| Property SDK operations, motion/temperature polling, pulse completion and exposure control | Physical device's master handler queue; guider uses the camera's queue |
| SDK event notification | SDK callback only enqueues an event value; frame retrieval, processing and completion run on the camera queue |
| Bus property callbacks | Validate/copy requests, publish appropriate state, enqueue work and return |

Driver and device queues are different threads: explicitly coordinate connection/disconnection with running device handlers. Serializing connection handlers alone does not serialize them with exposures, polling or SDK image delivery. The connection handlers drain the logical device queue before using the framework master-device lock, which also serializes a CCD connection with the still-connected guider on the shared handle. There are no private or enumeration pthread mutexes. After CCD Stop joins SDK callbacks, release the framework lock, drain final notifications, then reacquire it before freeing the buffer or closing the shared handle.

## Implementation steps

### 1. Capture baseline and contracts

- [x] Record HEAD and existing workspace changes; inventory shared-source consumers and available build targets.
- [x] Inventory all SDK calls, timers, property branches, private-data ownership and every attach/detach path.
- [x] Record existing property counts, visibility, permissions, ranges and persistence as the compatibility baseline.
- [x] Read relevant SDK callback restrictions and determine which operations join or stop SDK threads.
- [x] Document existing connection/removal state and camera-only, guider-only and combined ownership; add state only where required to reject work during teardown.

### Step 1 results

#### Baseline and build coverage

Baseline commit: `30e667c8cd0b57c0aa0feb285ae79a651ce3dd2c`. Driver version: `0x03000029`. All source line references below refer to this commit's `indigo_ccd_touptek.c`, unless another file is named. The committed source is the exact baseline for SDK-dependent values and inherited property defaults; the tables below record driver specializations, not a replacement definition of the base classes.

Workspace at entry: `indigo.xcodeproj/project.pbxproj` already modified, and this `REFACTOR.md` untracked. Preserve the existing Xcode edit. This step changes documentation only.

| Driver directory | Selection macro / SDK prefix | Bundled macOS SDK |
| --- | --- | --- |
| `ccd_touptek` | default TOUPTEK / Toupcam | libtoupcam |
| `ccd_altair` | ALTAIR / Altaircam | libaltaircam |
| `ccd_baccam` | BACCAM / Baccam | libbaccam |
| `ccd_bresser` | BRESSER / Bressercam | libbressercam |
| `ccd_omegonpro` | OMEGONPRO / Omegonprocam | libomegonprocam |
| `ccd_ssg` | STARSHOOTG / Starshootg | libstarshootg |
| `ccd_rising` | RISING / Nncam | libnncam |
| `ccd_mallin` | MALLIN / Mallincam | libmallincam |
| `ccd_meade` | MEADE / Toupcam | libmeadecam (directory `libmedecam`) |
| `ccd_ogma` | OGMA / Ogmacam | libogmacam |
| `ccd_svb2` | SVBONY / Svbonycam | libsvbonycam |

All ten wrapper sources include the ToupTek C file. All eleven directories contain headers and macOS dylibs; `lipo -archs` confirms x86_64 and arm64 in every listed dylib. Each uses `../../Makefile.drv` without a local Makefile or Makefile.inc. The `all` target provides the driver archive, dylib and executable. The local generated configuration is Darwin/arm64 with universal x86_64+arm64 compiler/linker flags. Do not copy its absolute paths into committed build files.

Verified target discovery with `make -n -C indigo_drivers/ccd_touptek -f ../../Makefile.drv all`; this was a dry run only, and existing outputs were up to date. Later compilation must ensure wrapper objects rebuild when the included ToupTek source changes. Availability of universal SDK slices does not establish successful compilation, Linux/Windows coverage or hardware behavior.

#### Execution and timer inventory

| Existing context | Functions / responsibilities | Baseline references |
| --- | --- | --- |
| Bus change callback | CCD mode/bin/frame, exposure/stream/abort, cooler/temperature/gain/offset, advanced/fan/heater/conversion gain/LED/bin mode, CONFIG | 1170–1557 |
| Bus change callback | Guider CONNECTION, DEC and RA pulses | 1679–1735 |
| Bus change callback | Wheel CONNECTION, slot, calibration, model and CONFIG | 1930–2004 |
| Bus change callback | Focuser CONNECTION, reverse, absolute position, limits, backlash, relative steps, abort, compensation, buzzer, mode and CONFIG | 2360–2641 |
| Immediate timer, or synchronous call from detach | `ccd_connect_callback`, `guider_connect_callback`, `wheel_connect_callback`, `focuser_connect_callback` | 936, 1602, 1855, 2244 |
| SDK thread | `pull_callback`; optional `pull_callback_dummy` under disabled `USB3_EXPOSURE_CLUDGE` | 474–573 |
| Immediate, unreferenced device timers | `finish_exposure_async`, `abort_cleanup_async`, `stop_video_mode_async` | 417–451, scheduled from image/abort paths |
| Device timer | Exposure watchdog: exposure +25 seconds, or 1.5× exposure above 50 seconds; canceled synchronously by SDK callback, abort and CCD disconnect | 453, 496, 1298–1299, 1333 |
| Device timer | CCD temperature: starts at 5 seconds and repeats every 5 seconds | 575–620, 968 |
| Device timers | RA/DEC completion after requested pulse duration; replacement cancels the previous axis timer; guider disconnect cancels both synchronously | 1638–1727 |
| Device timer | Wheel move: 0.5-second polling; calibration starts after 0.5 seconds then sleeps/polls at 1-second intervals; both use `wheel_timer` | 1766–1814, 1965–1978 |
| Device timers | Focuser motion: 0.5 seconds; temperature starts at 0.1 seconds and repeats every 2 seconds, with compensation able to schedule motion | 2019–2175, 2330–2331 |
| Unowned timer | Every USB arrival/removal schedules `process_plug_event` after 0.5 seconds, with no retained timer reference or USB pointer | 2887–2889 |

Connection requests copy values, publish BUSY and schedule the connection timer. Detach sets the disconnected switch and invokes the same callback synchronously when `IS_CONNECTED`. Polling cancellation is therefore part of each class's disconnect contract. Wheel initialization and calibration contain blocking loops: do not transfer an indefinite wait onto the shared driver queue without considering removal/shutdown progress (review reference below).

#### Property compatibility baseline

| Class / property | Existing specialization to preserve |
| --- | --- |
| CCD INFO | Count 8; model name/pixel geometry from SDK model; maximum preview width/height from all preview resolutions. Serial/HW/FW read at connection. |
| CCD MODE, BIN, FRAME | RW mode list, initially count 0 then populated for bin 1–8: supported RAW08/10/12/14/16 plus RGB08 for color, supported MON08/10/12/14/16 for mono. First mode selected. Binning RW, both axes 1–8 with square-bin enforcement. Frame maxima follow sensor dimensions; FRAME RO without ROI_HARDWARE. Bit-depth limits initially 8 and raised according to supported flags. Preserve the exact format-selection code (709–832). |
| CCD temperature/cooler | Temperature visible with GETTEMPERATURE; RW plus visible cooler and cooler power with TEC_ONOFF, otherwise temperature RO. Cooler defaults OFF. |
| CCD streaming/image formats/gain/offset | Streaming and gain visible; image-format count 7. Exposure/streaming range read from SDK microseconds and divided by 1e6; gain bounds read from SDK. Offset shown on successful black-level support probe, min BLACKLEVEL_MIN, max 248 (conversion logic stays unchanged). |
| X_CCD_ADVANCED | RW number, 9 items for color, count 1 (SPEED only) for mono. SPEED 0..model.maxspeed; other items use SDK MIN/MAX/DEF macros and step 1. ToupTek values: CONTRAST −255..255 default 0; HUE −180..180 default 0; SATURATION 0..255 default 128; BRIGHTNESS −255..255 default 0; GAMMA 20..180 default 100; R_GAIN/G_GAIN/B_GAIN −127..127 default 0. Preserve branded SDK macro evaluation. |
| X_CCD_FAN / HEATER | Allocated only with FAN / HEAT flags; each RW number, one item FAN_SPEED / POWER, initial 0..0 step 1 default 0; maxima queried at connection. |
| X_CCD_CONVERSION_GAIN | RW one-of-many: LCG default, HCG; HDR third item only with CGHDR. Allocated with CG or CGHDR; count 2 or 3. |
| X_CCD_BIN_MODE | RW one-of-many, count 3: SATURATE default, EXPAND, AVERAGE. |
| X_CCD_LED | RW one-of-many, count 2: ON default, OFF; initially hidden, visibility determined by successful TAILLIGHT query. |
| CCD custom lifecycle/configuration | Custom properties are enumerated only while connected and deleted on disconnect. Explicit CONFIG saves: ADVANCED, CONVERSION_GAIN, BIN_MODE, LED. FAN and HEATER are not explicitly saved here. Remaining handling passes to the CCD base class. |
| Guider | INFO count 8, model from SDK and serial/HW/FW at connection; standard RA/DEC properties retained. SDK directions N=0, S=1, E=2, W=3. Completion zeros both axis items and publishes OK. |
| Wheel | INFO count 7. X_CALIBRATE: connected-only RW one-of-many, count 1, START=false. X_WHEEL_MODEL: RW one-of-many, 5_POSITIONS/7_POSITIONS/8_POSITIONS, default 7; explicitly saved by CONFIG. Slot maximum, slot-name count, slot-offset count and private `count` track model selection. SDK slots are zero-based; public slots one-based. |
| Focuser limits/position/steps | Limits visible, minimum position fixed at 0, maximum-position range 0..65000 step 100. POSITION and STEPS 0..65000 step 1. Connection reads hardware maximum into the limits value/target. |
| Focuser other standard properties | SPEED hidden; BACKLASH visible 0..10000 step 1; ON_POSITION_SET, TEMPERATURE, REVERSE_MOTION, COMPENSATION and MODE visible. COMPENSATION count 2, coefficient −10000..10000. Other item defaults/thresholds remain inherited. |
| Focuser mode | MANUAL defines motion controls and POSITION RW; automatic mode deletes manual motion controls and redefines POSITION RO (2603–2627). Preserve this explicit define/delete behavior, including existing SPEED handling. |
| X_AAF_BEEP | Connected-only RW one-of-many, count 2, ON=false/OFF=true before SDK read. No active custom CONFIG save; the commented EAF save is not implemented behavior. Base focuser CONFIG handling remains active. |

No property schema change is authorized by this inventory. Driver property constructors, item constructors, count/hidden/permission assignments, numeric range assignments and explicit saves were inspected. Inherited definitions remain authoritative in the base driver sources at the baseline commit; hardware-dependent values cannot be reduced to a single static property snapshot.

#### Ownership and connection state

- `DRIVER_PRIVATE_DATA` (262–300) owns SDK descriptor/handle, presence flag, CCD/guider pointers, buffer and exposure format/state, mutex, timer references and class-specific custom properties. Wheel/focuser allocate separate private data, with their own logical device stored in `camera`; that field does not always mean a CCD.
- A physical CCD is its own master; its optional ST4 guider shares private data and points to the CCD as master. `devices[SDK_DEF(MAX)]` stores physical master devices, not guider slots. There is no independent connection reference count: each logical device's `gp_bits` is used as a 0/1 connection marker.
- First logical connect acquires the global lock and opens `@<SDK id>` when handle is NULL. A sibling reuses an existing handle. Failed Open releases the acquired global lock, publishes ALERT/disconnected and resets that device's `gp_bits` to 0.
- With both CCD and guider connected: CCD disconnect calls Stop and cancels its timers/releases buffer; it retains the handle while guider `gp_bits` is 1. Guider disconnect cancels its own timers and retains the handle while CCD `gp_bits` is 1. The last logical disconnect closes when its sibling exists and has `gp_bits == 0`.
- Exact existing exception: CCD close condition requires `PRIVATE_DATA->guider != NULL`; a CCD without guider skips this block. Wheel/focuser close conditions check their own device through `PRIVATE_DATA->camera` before clearing their own `gp_bits`, so ordinary successful disconnect skips Close there too. These are baseline findings, not a reason to silently rewrite connection accounting; see DRV-059 in the folder review file.
- Existing lock order in connection: master-device lock, enumeration mutex around Open, and enumeration mutex then private mutex around Close. Discovery holds enumeration mutex but releases it before detach, avoiding recursive acquisition by disconnect. SDK callbacks do not take the private mutex.
- `present` is a scan marker: all existing devices are marked false, then matched SDK ids become true. It is not a synchronized disconnecting/removing guard. There is no session generation or explicit removal flag. Do not add general state machinery in step 1; introduce only the minimum guard required by the eventual queue teardown protocol.
- Discovery (2717–2884) allocates/attaches CCD, optional guider, wheel or focuser based on model flags. macOS probes camera serial with temporary Open/Close for naming; other platforms use unique display names. ToupTek additionally maps OEM VID/PIDs in `OEMCamEnum` (2694). Preserve id-based reconciliation rather than inventing a libusb-to-SDK identity mapping.
- Both missing-device removal and `remove_all_devices` detach/free guider first, then master, private data and master allocation. Class detach releases custom properties and delegates to the appropriate base detach. SHUTDOWN currently checks masters, deregisters hot-plug and calls removal synchronously. Pending unowned discovery timers are not explicitly canceled.

#### SDK callback contract and minimal migration decisions

The bundled `libtoupcam/include/toupcam.h:427–430` explicitly forbids Close/Stop inside the event callback (deadlock), and forbids `put_Option` with TRIGGER, BITDEPTH, PIXEL_FORMAT, BINNING or ROTATE there (`E_WRONG_THREAD`). The data callback restriction is repeated at 583–584. Equivalent restrictions were verified in all ten branded headers; Meade uses lines 410–411 and 564, the others use 427–428 and 584.

`setup_exposure` calls Stop and restarts pull mode when the format changes, and also changes trigger/binning/bit-depth options (624–700). `stop_video_mode_async` changes OPTION_TRIGGER and the driver comment at 423–426 says it joins the SDK callback thread and must not hold bus/private mutexes. The SDK headers establish forbidden contexts, but do not document an exhaustive internal join graph or complete callback-quiescence guarantees for every failure return. Treat the join statement as an existing driver contract, not newly verified vendor implementation.

Consequences for subsequent steps: retain SDK frame pulling/processing and its current sequencing; move only deferred control work. No SDK callback may synchronously wait for a queue executing Stop/Close/trigger-mode changes. Do not hold locks needed for image publication while waiting for callbacks. The exposure watchdog may remain a timer if converting it would change cancellation behavior. Existing DRV-026 and DRV-030 fixes remain intact. No new state fields or synchronization changes were made in step 1.

Validation performed: source inventory, all branded SDK callback restrictions, all macOS SDK architecture slices, build-target dry run and documentation whitespace check. Runtime behavior, queue safety and physical device behavior remain to be validated in later steps.

#### SDK call-site index

Lexical inventory of call expressions (log strings/macros/comments excluded). `pull_callback_dummy` and selected setup/connect calls remain conditional on `USB3_EXPOSURE_CLUDGE`; it is disabled at baseline. Numbers identify baseline source lines.

| Function | SDK operations and call-site lines |
| --- | --- |
| `get_blacklevel` | `get_Option` (331, 336) |
| `handle_offset` | `put_Option` (373) |
| `get_bayer_pattern` | `get_RawFormat` (397) |
| `stop_video_mode_async` | `put_Option` (440) |
| `exposure_watchdog_callback` | `put_Option` (463) |
| `pull_callback_dummy` | `put_Option` (481) |
| `pull_callback` | `PullImageV2` (509); `put_Option` (559) |
| `ccd_temperature_callback` | `get_Temperature` (581); `get_Option` (604, 608) |
| `setup_exposure` | `Stop` (631); `put_Option` (634, 636, 641, 643, 648, 650, 655, 695); `StartPullModeWithCallback` (660); `put_Roi` (686) |
| `ccd_connect_callback` | `Open` (947); `get_Option` (961, 964, 983, 985, 992, 1008, 1051, 1057, 1059, 1065, 1083); `put_Option` (970, 1093); `get_SerialNumber` (972); `get_HwVersion` (974); `get_FwVersion` (976); `get_ExpTimeRange` (1015); `put_AutoExpoEnable` (1019); `get_ExpoAGainRange` (1021); `get_ExpoAGain` (1023); `get_Speed` (1043); `get_FanMaxSpeed` (1049); `StartPullModeWithCallback` (1104, 1114); `Trigger` (1106); `Stop` (1109, 1124); `Close` (1156) |
| `ccd_change_property` | `put_ExpoTime` (1293, 1320); `Trigger` (1296, 1348); `put_Option` (1324, 1356, 1374, 1470, 1484, 1506, 1520); `put_Temperature` (1369); `put_ExpoAGain` (1393); `put_Contrast` (1413); `put_Hue` (1420); `put_Saturation` (1427); `put_Brightness` (1434); `put_Gamma` (1441); `put_WhiteBalanceGain` (1449); `put_Speed` (1457) |
| `guider_connect_callback` | `Open` (1612); `put_Option` (1622); `get_SerialNumber` (1624); `get_HwVersion` (1626); `get_FwVersion` (1628); `Close` (1645) |
| `guider_change_property` | `ST4PlusGuide` (1700, 1703, 1719, 1722) |
| `set_wheel_positions` | `put_Option` (1755); `get_Option` (1758) |
| `wheel_timer_callback` | `get_Option` (1768) |
| `calibrate_callback` | `put_Option` (1786); `get_Option` (1793) |
| `wheel_connect_callback` | `Open` (1866); `get_HwVersion` (1876); `get_FwVersion` (1878); `put_Option` (1891); `get_Option` (1896); `Close` (1916) |
| `wheel_change_property` | `put_Option` (1956); `get_Option` (1960) |
| `focuser_timer_callback` | `AAF` (2024, 2033) |
| `compensate_focus` | `AAF` (2098, 2113) |
| `temperature_timer_callback` | `AAF` (2135, 2152) |
| `focuser_connect_callback` | `Open` (2255); `get_HwVersion` (2265); `get_FwVersion` (2267); `AAF` (2272, 2279, 2288, 2297, 2306, 2314); `Close` (2346) |
| `focuser_change_property` | `AAF` (2381, 2419, 2431, 2438, 2463, 2484, 2515, 2538, 2559, 2566, 2592) |
| `process_plug_event` | `EnumV2` (2726); `Open` (2757); `get_SerialNumber` (2760); `Close` (2761) |
| `ENTRY_POINT` | `Version` (2927) |

The TOUPTEK-only `OEMCamEnum` also calls `Toupcam_get_Model` directly at line 2704 and uses libusb enumeration; the commented model-dump example in ENTRY_POINT is not active code. `get_blacklevel`, `handle_offset`, `get_bayer_pattern`, `setup_exposure`, `set_wheel_positions` and `compensate_focus` inherit their caller's execution context.

### 2. Introduce the persistent driver lifecycle queue

- [x] Create the driver queue before hot-plug registration/startup enumeration; handle queue creation and registration failure with complete rollback.
- [x] Queue all four CONNECTION handlers and discovery-time Open/Close calls on this queue.
- [x] Keep the first SDK enumeration and subsequent enumerations on the same live worker throughout the loaded driver lifecycle.
- [x] Preserve shared-handle acquisition/release and global-lock ownership; verify that queue migration still closes exactly once when the last logical user disconnects. Check camera-without-guider and failed-open paths explicitly; correct only lifecycle defects necessary for this change.
- [x] Define coordination with the master queue before Stop/Close: prevent new device work, finish or cancel existing work, and avoid waiting while holding a lock needed by the worker or SDK callback.

### Step 2 results

- Added one persistent `driver_queue` per compiled driver variant before USB callback registration. SDK discovery/probing and CCD/guider/wheel/focuser connection handlers use that queue. SDK version reporting remains synchronous; it does not enumerate/open hardware.
- Replaced unowned hot-plug timers with delayed discovery tasks on the driver queue, preserving the existing 0.5-second settling delay and SDK/OEM reconciliation. The callback carries no USB pointer, so no new libusb reference ownership is introduced.
- Connection functions now use the `_connection_handler` naming convention. They cancel pending device handlers before acquiring the master lock. Disconnect cancels the relevant existing timers before Close; CCD also cancels completion timers after Stop, before releasing its image buffer. Ordinary property callbacks and image acquisition logic are unchanged and remain for subsequent steps.
- Wheel connection uses `wheel_connection_handler` plus `wheel_connection_finalizer`; calibration uses `wheel_calibrate_handler` plus `wheel_calibrate_finalizer`. A handler sends the existing SDK command once; a finalizer makes one status read and schedules its next invocation through `indigo_execute_handler_in()` when unfinished. No sleeping/waiting loop or private polling thread was introduced. Finalizers run on the device queue and disconnect cancels them before taking the master lock/closing the handle. Calibration was included because waiting for its old timer loop during disconnect could otherwise stall the lifecycle queue.
- Fixed only the close guards identified in DRV-059: a CCD without guider and standalone wheel/focuser now release their handle/global lock on disconnect. Camera+guider still use their existing per-logical-device `gp_bits`. Removal clears the guider pointer after freeing the guider so the CCD does not read freed sibling state.
- A small submission gate prevents new lifecycle requests during shutdown/removal. Removal drops pending connection tasks for both logical devices. Shutdown uses standard `indigo_queue_remove`/`indigo_queue_delete`, with no private condition-variable shutdown protocol. It rejects connected/connecting CCD, guider, wheel or focuser, and resumes discovery on rejection. Queue creation/registration failure restores a retryable driver state. Normal shutdown removes already-disconnected devices after draining discovery/lifecycle work; their handles have already been closed by connection handlers.
- Queue submission capacity is unlimited, matching the old timer submission behavior: the queue API has no enqueue failure result, so connection requests must not silently disappear at the default pending-task limit. Coalescing scans remains outside this minimal step.
- Driver version advanced to `0x0300002a`; license year and Codex refactoring notice updated. No property/item additions, removals or range changes, hence no PROPERTIES.md schema update.

Validation: all eleven branded driver archive/dylib/executable builds succeeded with universal macOS arm64+x86_64 flags. The linker reports the existing SDK deployment-target mismatch (SDK macOS 11.0 versus project 10.10). `test_ccd_touptek_sdk` compiles the production source separately against replacement SDK/USB hooks and is registered in the integration test target. It covers both CCD/guider connection orders, CCD-only close, wheel/focuser close, stable enumeration/Open/Close worker identity, a moving wheel not blocking another device, rejected shutdown with unfinished wheel connection, calibration cancellation, unplug during wheel initialization, failed Open, failed queue creation, failed registration and repeated load/unload. The final test passed natively on arm64 and as x86_64 under Rosetta (not on a physical Intel Mac). The expected queue-creation error message is injected by the failure-path test. The fixture does not emulate image callbacks or vendor SDK internals; exposure/callback races and physical hardware remain unvalidated here.

User standardization constraint: subsequent steps must keep the same handler + finalizer model and normal INDIGO queue APIs. Do not introduce blocking handler loops, private polling workers or a custom queue synchronization protocol.

### 3. Move property operations to device handlers

- [x] Split CCD property branches into bus dispatch and queued handlers: exposure, streaming, abort, cooler/temperature, gain/offset and vendor controls; also serialize mode/bin/frame state used by exposure setup.
- [x] Convert guider pulses, wheel slot/calibration/model controls and focuser movement/settings/abort to handlers on the appropriate master queue.
- [x] Preserve BUSY/OK/ALERT transitions, validation, property messages and base-class dispatch.
- [x] Guard queued operations against disconnected/removing devices. Do not allow a delayed request to operate on a closed handle or a later connection session.
- [x] Decide per property whether repeated requests coalesce or retain individual values; avoid handlers accidentally executing overwritten request values.

### 4. Convert polling and exposure completion safely

- [x] Replace temperature, wheel/focuser motion and guide-pulse timers with delayed handlers and targeted cancellation. Ensure only one polling chain is active.
- [x] Move deferred exposure completion, abort cleanup and video-stop work to handlers with connection/session guards.
- [x] Preserve watchdog semantics and verify cancellation against image completion. The watchdog is now a delayed device handler with the same timeout formula. Do not make the SDK callback synchronously wait for a queue that may be waiting for that callback.
- [x] Move frame retrieval and processing to the camera queue. The bundled SDK header supports both event callbacks and window-message pull mode and does not require PullImageV2 on its internal callback thread. The SDK callback only reads an atomic generation and queues an integer notification; buffer, abort and video state belong to the camera queue.
- [x] Ensure OPTION_TRIGGER changes, Stop and Close run outside the bus callback and without locks needed by SDK callbacks. Preserve finite/infinite streaming and abort cleanup.
- [x] Remove obsolete timer fields and helper names only after all call sites are converted. Document any deliberately retained timer and its ownership.

#### Step 3–4 implementation and validation

- Readiness checks are centralized in each bus dispatcher; the repeated `property_handler_ready` and `property_change_allowed` wrappers were removed. Queue draining owns task lifetime. `_handler` and `_finalizer` suffixes are reserved for queue tasks; directly called helpers use ordinary names. No tests or builds will be rerun without the user’s instruction.
- Every custom `change_property` branch now dispatches a named handler through the standard INDIGO macros. Configuration is also queued; its base-class call receives a local property copy because the base switch-copy operation cannot use the same vector as both source and destination. Unknown properties retain the ordinary typed-base fallback.
- Preserved base fall-through for CCD exposure/streaming/abort/gain, successful CCD frame changes, wheel model and CONFIG; all former explicit-return branches remain self-contained. In particular, gain still reports its SDK error and then invokes the original base path (which publishes OK); offset returns its own result. This existing difference is asserted by the test.
- `handle_offset` is merged into `ccd_offset_handler`. Guider pulses, wheel/focuser motion, calibration and temperature use delayed handlers/finalizers and targeted cancellation. No `indigo_set_timer`, `indigo_reschedule_timer`, timer references, `indigo_usleep`, or `pthread_mutex_*` remain in the driver. The disabled USB3 workaround was removed; none of the consumers enabled it.
- SDK format, binning and ROI settling retain the original 0.1-second intervals via separate setup handlers. Abort cancels the unfinished setup chain and forces format setup on the next acquisition. Mode/bin/frame and bin-mode changes are rejected while acquisition owns the format; exposure and streaming cannot overwrite each other's pending setup.
- Standard BUSY admission retains the first pending request for command/settings properties, including focuser motion; requests received while BUSY are ignored before copying. Guide axes use ANYTIME to retain the baseline replacement/cancellation behavior (DRV-061 follow-up). Temperature uses the standard ANYTIME macro because BUSY also means physical settling; newer setpoints intentionally replace the desired value. CCD bin dispatch copies targets so the handler can still compare the old horizontal/vertical values. Related mode/bin/frame reservations are checked together. Focuser STEPS retains its own BUSY predicate, including after switching from automatic compensation to manual control. Cancelled BUSY properties are reset to ALERT for the next connection session.
- SDK notifications contain the event number and pull-mode generation without allocated payloads. Stop invalidates notifications from the old pull mode; streaming stop invalidates its last notifications and finalizes an aborted stream on the queue. Disconnect drains before Stop and again after the SDK callback has terminated. These handoffs prevent stale events from accessing freed buffers or completing a later acquisition.
- `test_ccd_touptek_sdk` now has six scenarios: lifecycle, CCD controls/base dispatch, acquisition/watchdog/streaming, guider, wheel and focuser. The fake SDK creates a separate callback thread and joins it during simulated Stop/trigger transitions. Tests cover a successful image, timeout with the original delay formula, finite/infinite streaming, abort, stale Stop notifications, shared CCD/guider worker identity, rejected duplicate values and queued-command cancellation followed by reconnect. The watchdog-only test hook shortens its wait after recording the requested production delay.
- An earlier revision compiled and linked all eleven branded variants for arm64 and x86_64; the latest cleanup remains unvalidated after the user stopped testing. Hardware-free native/Rosetta validation does not establish vendor SDK throughput, real USB timing, image fidelity or physical Intel Mac behavior. Those remain hardware checks in steps 6–7.

### 5. Serialize hot-plug and make teardown complete

- [x] Replace unowned hot-plug timers with driver-queue work. Preserve the existing SDK-id reconciliation and OEM enumeration where needed; do not assume an ASI-style one-to-one mapping from a libusb pointer to an SDK id.
- [x] Preserve the existing discovery settling delay and scan behavior. Add coalescing only if required for correct bounded queue operation, without losing removal or final topology state.
- [x] If queued events retain `libusb_device *`, balance references on every enqueue, rejection, cancellation, attach failure and detach path. If events only request a rescan, do not retain unused USB pointers.
- [x] Handle duplicate arrivals, missing slots, partial camera/guider attach failures and rapid unplug/replug without leaks or duplicate devices.
- [x] Mark devices as removing, prevent new work and cancel/drain their queued operations before releasing properties or private data. Detach guider before its camera/master.
- [x] Quiesce SDK image callbacks before freeing the image buffer; ensure no deferred completion accesses detached devices.
- [x] Shutdown: stop accepting hot-plug work, deregister callbacks, resolve queued discovery/connection work, remove already-disconnected devices after lifecycle work has stopped, then delete the driver queue (the standard ASI shutdown pattern; no SDK calls in this final detach). Define the failure/rollback path when connected devices prevent shutdown.
- [x] Validate unload/reload in the same process with the fake SDK; real vendor SDK thread association remains a hardware check.

#### Step 5 implementation and deferred validation

- Hot-plug uses the three generated-driver-style functions: `hotplug_callback` dispatches ARRIVED to `process_plug_event_handler` and LEFT to `process_unplug_event_handler`, both on the driver queue with the existing 0.5-second delay. Plug enumerates SDK/OEM records and attaches missing ids; unplug enumerates and removes missing ids. Plug first invokes unplug reconciliation to reclaim stale slots. Shutdown and attach rollback invoke the same unplug handler with an explicit device, without enumeration. There is no `hotplug_handler`, separate plug/unplug helper or `remove_all_devices` wrapper. USB pointers are not retained because identity still comes from the SDK.
- Kept the existing 0.5-second queued rescan, SDK-id matching and OEM enumeration. Events retain no USB pointers; repeated scans do not attach another device with an existing id. The queue remains unlimited, so no new coalescing mechanism is needed. Shutdown deregisters the USB callback before cancelling/draining discovery work; the scan itself needs no driver-state guard.
- Reconcile presence and detach missing ids before attaching arrivals, making vacated slots available in the same scan. Check for a free slot before probing or allocating a new device. Skip SDK records without a model.
- Check both bus registration and attach callback results. On failure, mark private data as removing, cancel pending lifecycle work, detach and free the partial device. If guider attach fails, roll back the CCD too, so a later scan can retry the complete pair. Normal detach drains device handlers; guider is always detached before its master.
- Failed OEM USB-list/descriptor reads no longer use uninitialized discovery data. A short or missing serial from a discovery probe no longer causes a read before the serial buffer; successful serial-based names retain the same suffix.
- The file-local atomic `last_action` also controls work admission; there is no separate `driver_queue_accepting` flag. Set INIT before registering callbacks, restore SHUTDOWN on registration failure, and restore INIT when shutdown is rejected.
- Retained the existing CCD Stop/generation invalidation/drain ordering before buffer release. Reference counting uses `PRIVATE_DATA->count`; shutdown rejects active references and pending connection transitions. Rejected shutdown restores INIT, re-registers the USB callback and schedules a fresh discovery scan. Failure to restore the callback is logged and returns INDIGO_FAILED; the connected driver remains loaded.
- Like the ASI reference drivers, successful shutdown performs final detach of already-disconnected devices on the entry-point thread after draining driver work, then deletes the driver queue. All SDK enumeration, Open, Stop and Close work remains on the lifecycle/device queues. No custom waiting protocol, timer, mutex or helper wrapper was introduced.
- Initially inspected only while testing was paused. After the user authorized testing again, step 6 covered unload/reload, callback registration failures, rejected shutdown, duplicate arrivals, partial attach rollback and active removal with the fake SDK. Real SDK/hardware validation remains pending.

### 6. Hardware-free regression coverage

- [x] Add an SDK replacement integration harness, compiling the production driver as a separate translation unit and exercising public bus APIs. Follow existing ASI tests; do not include production `.c` directly in the test.
- [x] Simulate CCD-only, CCD+guider, filter wheel, focuser and multiple physical devices.
- [x] Assert that enumeration/probing/Open/Close use one stable driver worker, and that device operations share the correct master queue.
- [x] Test camera-first/guider-first connect and disconnect, repeated connect, failed Open and balanced Close/global-lock ownership.
- [x] Exercise exposure success/timeout/abort, finite and indefinite streaming, final-frame/abort overlap and disconnect during an SDK callback. Have the fake SDK model callback-joining behavior to expose deadlocks.
- [x] Test pulse replacement while BUSY, cancellation and subsequent requests, polling cancellation, unplug during work, duplicate/rapid hot-plug, queue shutdown, registration failure and unload/reload with pending events.
- [x] Use bounded waits and check cleanup. Document fake-SDK limitations in `indigo_test/CHANGES.md`; passing these tests is not proof of vendor SDK or physical USB behavior.

#### Step 6 results (2026-09-08)

The harness is consolidated into `indigo_test/integration/test_ccd_touptek_sdk.c`. The production driver, framework dispatcher and simulator image fixture remain separately compiled translation units. The build uses SDK/USB replacements and real bus/queue code. Configuration files and image/video output use a temporary directory without changing HOME.

The initial 17 scenarios passed on native macOS arm64 and x86_64 under Rosetta, both with exit status 0. Coverage includes all four logical devices, published property inventories and passive writable property round-trips; dedicated workflows exercise CCD controls, exposure/abort/watchdog/recovery, finite/infinite streaming and all seven image formats, guiding, wheel calibration and focuser motion/temperature compensation. RAW payloads are checked against samples from `ccd_simulator/indigo_ccd_simulator_data.c`; FITS headers and finalized local SER/AVI signatures are checked. CONFIG SAVE/LOAD and CLIENT/LOCAL/BOTH/NONE uploads also passed.

Lifecycle tests cover shared CCD/guider ownership, queue affinity, duplicate BUSY rejection, cancelled work and reconnect, failed registration/Open/attach, hot-plug duplicates, active removal and unload/reload. Three additional hardware-free scenarios now force final-frame/abort overlap in both queue orders, disconnect inside the real SDK callback, and rapid hot-plug plus shutdown with pending events. The corresponding step 6 checkboxes are complete; this covers the specified interleavings without claiming every possible vendor SDK schedule. Guide requests received while BUSY replace the active pulse, restoring baseline behavior in the DRV-061 follow-up.

The universal test binary compiled without warnings. After merging the four harness files into one, it rebuilt successfully and the focused configuration persistence/upload scenario passed again. `git diff --check` passed; test build artifacts were removed with `make -C indigo_test test-clean`. Detailed coverage and fake-SDK limitations are recorded in `indigo_test/CHANGES.md`.

#### Additional step 6 race coverage

- A one-frame stream is tested in both orders: abort queued before its final notification, and abort queued while the final frame is being pulled. Assertions check exact frame/BLOB counts, standard base CCD states and successful acquisition afterward. A cancelled finite stream is ALERT; an abort arriving after completed streaming is itself ALERT.
- The real SDK callback is held inside its enqueue call after reading the event generation. Disconnect reaches SDK Stop and must join that still-running callback. After release, no stale frame is pulled or published; the guider stays connected and a new CCD connection successfully exposes.
- Eight unplug/replug cycles submit 1,024 alternating USB arrival/removal notifications. A further 64 USB events are explicitly held pending while shutdown deregisters the callback and drains its queue. Assertions verify no additional enumeration, balanced Open/Close/global locks, complete detach and a working unload/reload/connect/disconnect/shutdown cycle.

Test-only gates have bounded 10-second waits and delegate to the real queue APIs. The production driver logic is unchanged. The complete harness now contains 20 scenarios. Full native macOS arm64 and x86_64/Rosetta runs both passed all 20 with exit status 0, including all three new race cases, without gate timeouts. The universal build emitted no compiler warnings and `git diff --check` passed. Build artifacts were removed with `make -C indigo_test test-clean`. In this repeat run, mean guiding completion errors were +3.443 ms on arm64 and +3.899 ms on Rosetta, illustrating why the earlier lower Rosetta mean is not an architectural advantage.

#### Guiding pulse timing (initial 17-scenario run)

Each architecture run measured 20 pulses: EAST, WEST, NORTH and SOUTH at 20, 50, 100, 250 and 500 ms. `CLOCK_MONOTONIC` timestamps are taken when the fake SDK accepts the ST4 command and when the matching guide property returns to OK with zero axis values. Guiding delays are not accelerated. The output reports each duration/error and aggregate statistics.

| Execution | Samples | Mean signed error | Mean absolute error | p95 absolute error | Maximum absolute error |
| --- | ---: | ---: | ---: | ---: | ---: |
| Native macOS arm64 | 20 | +3.703 ms | 3.703 ms | 5.048 ms | 5.066 ms |
| x86_64/Rosetta | 20 | +2.947 ms | 2.947 ms | 5.048 ms | 5.063 ms |

These measure software completion latency, including queue scheduling and property delivery, not physical ST4 signal duration. Rosetta's lower sample mean does not demonstrate faster or more accurate execution: the runs were sequential, contained only 20 samples each and experienced different scheduler conditions. The p95 and maximum were practically identical. Establishing an architectural difference would require many more samples and repeated alternating runs under comparable load. Rosetta is not physical Intel Mac validation.

### 7. Build, documentation and hardware validation

- [x] Build ToupTek and available branded consumers; validate macOS arm64 and x86_64 compilation and linking. Record unavailable platform/SDK combinations explicitly.
- [x] Run the focused regression suite and relevant existing queue tests. Use sanitizer runs where supported to investigate remaining callback/lifetime risks.
- [x] Preserve the license header, extend its year to 2026 and add the required Codex refactoring notice. Update driver version according to repository convention.
- [x] Compare property inventory with the baseline; update `indigo_docs/PROPERTIES.md` if a property or item is added/removed. This refactor intends no public schema change.
- [x] Update this plan with completed stages and actual validation results; keep risks in `REVIEW.md` and coverage/deferred tests in `indigo_test/CHANGES.md`.
- [x] On available physical guiding cameras, validate discovery/connect cycles, exposure/streaming/abort, guider sharing and USB removal under load. Record unavailable wheel/focuser, simultaneous multi-camera and physical Intel validation separately.

#### Step 7 physical guiding camera result (2026-09-08)

Added the explicitly opt-in `indigo_test/hardware/test_ccd_touptek_hw.c`. Run `make -C indigo_test test-ccd-touptek-hw`; normal `make test` neither builds nor runs it. The executable also requires `--run` before touching USB. It links the real driver and vendor SDK, with no test replacements.

Native arm64 execution passed on Touptek GPCMOS01200KMB #04E69B: three exposures, exposure abort, five-frame streaming, indefinite streaming/abort, four 100 ms guide directions, guider operation with CCD disconnected, both shared-handle connection orders and a successful exposure after full disconnect/reconnect. All 12 delivered RAW frames had valid 1280 × 960 payloads (1,228,812 bytes). Cleanup disconnected both devices and driver shutdown succeeded. Running outside the sandbox was required for SDK enumeration and explicitly approved. Rosetta was not run.

The optional target and its exclusion from the default suite were verified. Build/link succeeded with the existing macOS deployment-target warning from the bundled SDK (11.0 versus repository flags 10.10). The build and available physical-camera checks are now complete; unavailable platforms and hardware are listed separately below. Detailed invocation and limits are in `indigo_test/CHANGES.md`.

#### Step 7 final validation

- Forced fresh compilation of ToupTek, Altair, Baccam, Bresser, OmegonPro, StarshootG, Rising, Mallin, Meade, Ogma and SVBony shared-source consumers with their bundled SDKs. Every archive, dylib and executable built successfully; `lipo -archs` confirmed arm64 and x86_64 dylib slices. No Rosetta execution was performed. The SDKs produce existing linker deployment-target warnings (SDK minimum macOS 11.0 versus repository target 10.10); these builds do not establish runtime support on macOS 10.10.
- The native SDK regression suite passed all 20 scenarios. Native `test_timer` passed all 86 timer/handler/queue cases. AddressSanitizer instrumentation of the test harness, production driver, image fixture and framework dispatcher passed the three deterministic race scenarios without a sanitizer report. Prebuilt framework archives and closed-source SDK internals are outside that instrumentation; this is not a ThreadSanitizer or full-stack instrumentation claim.
- All 45 property/item initialization calls match baseline `30e667c8cd0b57c0aa0feb285ae79a651ce3dd2c` after resolving extracted name defines and whitespace. All 27 property count/visibility assignments preserve their public effect; the wheel assignment no longer also writes the private slot count because `PRIVATE_DATA->count` now holds connection references. No property/item was added or removed, so `indigo_docs/PROPERTIES.md` needs no schema update.
- License/copyright includes 2026, the Codex queue-refactoring notice is present, and driver version is already `0x0300002b` (baseline `0x03000029`). No further version bump or driver behavior change was needed for validation.
- Altair ALTAIRGP224C #E61F50 passed the same physical acquisition/guider workflows using the real Altair SDK, with RGB RAW frames at 1280 × 960 (3,686,412 bytes). The user physically unplugged it during active streaming; both logical devices were removed. After replug, a fresh exposure and guide pulse succeeded. A further native run successfully shut down/reinitialized the driver in the same process and acquired a frame on its new queue. Both runs exited 0.
- Hardware testing remains opt-in: `make -C indigo_test test-ccd-touptek-hw HW_DRIVER=altair` selects Altair; append `HW_HOTPLUG=1` only when an operator can unplug/replug at the printed prompts. Default `HW_DRIVER=touptek` selects ToupTek. `make -n test` confirms that normal tests neither build nor run the hardware executable. Detailed coverage is recorded in `indigo_test/CHANGES.md`; no review baseline was advanced.

Deferred external validation: Linux/Windows toolchains, physical Intel macOS, real wheel/focuser hardware, simultaneous multiple physical cameras and prolonged vendor-SDK stress are unavailable/unverified in this session. They are not represented as passed. The two guiding cameras were tested sequentially; only Altair underwent the manual cable-removal test. `git diff --check` passed and test/ASan build artifacts were removed with `make -C indigo_test test-clean`.

## Completion criteria

The handwritten source uses a stable driver-wide SDK lifecycle queue and master-device handlers for normal work; bus callbacks no longer perform blocking SDK control operations. Hot-plug and shutdown cannot leave pending work targeting freed devices. Camera/guider ownership and SDK callback termination are covered by focused tests, available branded builds pass, and public behavior is preserved. SDK callback handoff and queue teardown are documented; no driver timers remain. Every production change is necessary for execution-context migration or its lifecycle safety, with existing overall logic preserved. Hardware-dependent claims remain explicitly pending until tested. No generator migration is part of this change.
