# Agent function catalog — draft

Use the [model instructions](instructions.md) and [server preparation](server-and-devices.md) first. This catalog indexes semantic function groups, not property labels. Open the linked manual before executing a function; check its prerequisites, exact controls, completion evidence and failure handling against the live schema.

## Coverage and limits

All eleven in-tree native agent modules have a manual below. The optional Sequencer library is documented separately under Scripting; it is not another loadable native agent. Function inventories group related settings with their operation rather than claiming one function per property.

This is a source-grounded draft, not an operationally certified reference. Explicit remaining limits:

- Alpaca bridge setup and supported member inventory are covered; complete per-member ASCOM semantics, encoding and error recipes remain incomplete.
- Imager Bahtinov spike-angle diagnostics lack a complete interpretation recipe; the model must not infer one.
- Mount targets/readbacks follow the selected mount epoch. Derived Mount Agent calculations contain unresolved J2000 assumptions; non-J2000 dependent calculations are not validated.
- Sequencer must already have been evaluated in the scripting interpreter. Its full source exceeds the current MCP request limit; uploading/loading it through MCP is not implemented.
- Hardware-specific setup beyond the documented generic preparation requires its own explicit driver recipe. Missing documentation is a reason to report the gap, not guess from labels.

Full manuals and this index are compiled into the MCP agent. Read this catalog with `get_help({"topic":"agent-catalog.md"})` or resource `indigo://help/agent-catalog.md`, then follow embedded manual links. Source links outside the bundle provide optional review provenance; they are not runtime dependencies.

## Imager Agent

[Read the complete manual](agents/imager.md). IDs are local to this manual.

| ID | User function | Recipe |
| --- | --- | --- |
| IMG-01 | Load, choose camera/wheel/focuser/shutter and related agents | [Preparation](agents/imager.md#purpose-and-preparation) |
| IMG-02 | Configure capture/storage/format/optics through selected-device proxies | [Preparation](agents/imager.md#purpose-and-preparation) |
| IMG-03 | Capture one plain frame with a separate duration control | [Single capture](agents/imager.md#single-capture) |
| IMG-04 | Preview one processed frame | [Preview](agents/imager.md#preview) |
| IMG-05 | Run continuous processed preview | [Preview](agents/imager.md#preview) |
| IMG-06 | Acquire finite or indefinite exposure batch | [Exposure batches](agents/imager.md#exposure-batches) |
| IMG-07 | Run camera streaming batch | [Streaming](agents/imager.md#streaming) |
| IMG-08 | Run signed focus-bracketing/macro batch | [Bracketing](agents/imager.md#bracketing) |
| IMG-09 | Choose focus estimator and run U-curve autofocus | [Autofocus](agents/imager.md#autofocus) |
| IMG-10 | Run iterative HFD/peak, RMS-contrast or Bahtinov autofocus | [Autofocus](agents/imager.md#autofocus) |
| IMG-11 | Set autofocus backlash/stack/retry/failure policy | [Focus settings](agents/imager.md#focus-settings) |
| IMG-12 | Find stars, choose a listed star, set multistar/regions/subframe | [Star selection](agents/imager.md#star-selection) |
| IMG-13 | Clear star selection | [Star selection](agents/imager.md#star-selection) |
| IMG-14 | Select a filter and optionally apply focus offsets | [Filter and manual focus](agents/imager.md#filter-and-manual-focus) |
| IMG-15 | Jog focuser inward/outward and stop | [Filter and manual focus](agents/imager.md#filter-and-manual-focus) |
| IMG-16 | Enable batch dithering, choose cadence and last-frame policy | [Dithering and transit](agents/imager.md#dithering-and-transit) |
| IMG-17 | Automatically pause a batch around transit | [Dithering and transit](agents/imager.md#dithering-and-transit) |
| IMG-18 | Pause with abort, pause with wait and resume | [Pause and abort](agents/imager.md#pause-and-abort) |
| IMG-19 | Abort an active process and recover | [Pause and abort](agents/imager.md#pause-and-abort) |
| IMG-20 | Configure breakpoints, leader/follower barriers and group control | [Synchronization](agents/imager.md#synchronization) |
| IMG-21 | Refresh stored-file list, download a listed image | [Stored images](agents/imager.md#stored-images) |
| IMG-22 | Delete a listed stored image; inspect disk capacity | [Stored images](agents/imager.md#stored-images) |
| IMG-23 | Read process/image/focus/selection diagnostics | [Monitoring](agents/imager.md#monitoring) |
| IMG-24 | Reset agent defaults, save/restore configuration, create instances | [Reset and instances](agents/imager.md#reset-and-instances) |

## Guider Agent

[Read the complete manual](agents/guider.md). IDs are local to this manual.

| ID | Function group | Recipe |
| --- | --- | --- |
| GDR-01 | Load/select camera, guide output, related agents; configure proxies | [Setup](agents/guider.md#purpose-and-agent-first-setup) |
| GDR-02 | Single-frame preview | [Preview](agents/guider.md#preview) |
| GDR-03 | Continuous preview | [Preview](agents/guider.md#preview) |
| GDR-04 | Refresh stars and select a dynamic listed star | [Selection and regions](agents/guider.md#selection-and-regions) |
| GDR-05 | Configure multistar, search radius, regions, edge clipping, subframe | [Selection and regions](agents/guider.md#selection-and-regions) |
| GDR-06 | Clear selection | [Selection and regions](agents/guider.md#selection-and-regions) |
| GDR-07 | Choose Selection/Weighted Selection/DONUTS/Centroid detection | [Drift detection](agents/guider.md#drift-detection) |
| GDR-08 | Calibrate guide response | [Calibration](agents/guider.md#calibration) |
| GDR-09 | Calibrate then start guiding | [Calibration](agents/guider.md#calibration) |
| GDR-10 | Guide using valid calibration | [Guiding](agents/guider.md#guiding) |
| GDR-11 | Select RA/Dec correction controllers and tune response | [Correction controllers](agents/guider.md#correction-controllers) |
| GDR-12 | Set Dec direction/backlash and flip policy | [Declination and pointing](agents/guider.md#declination-and-pointing) |
| GDR-13 | Supply/update pointing and calibration metadata | [Declination and pointing](agents/guider.md#declination-and-pointing) |
| GDR-14 | Choose dither strategy, trigger/wait/reset sequence | [Dither and manual offsets](agents/guider.md#dither-and-manual-offsets) |
| GDR-15 | Set manual guide-reference X/Y offsets | [Dither and manual offsets](agents/guider.md#dither-and-manual-offsets) |
| GDR-16 | Tune predictive PEC / multi-kernel GP and inspect learning | [Predictive models](agents/guider.md#predictive-models) |
| GDR-17 | Reset each learned predictive model | [Predictive models](agents/guider.md#predictive-models) |
| GDR-18 | Set lost-star/failure recovery policy | [Failure policy](agents/guider.md#failure-policy) |
| GDR-19 | Enable/configure/read guiding/calibration logs | [Logging](agents/guider.md#logging) |
| GDR-20 | Read drift, correction, RMSE, response and model statistics | [Statistics](agents/guider.md#statistics) |
| GDR-21 | Abort and recover a running operation | [Abort and recovery](agents/guider.md#abort-and-recovery) |
| GDR-22 | Reset defaults, save/restore settings, manage instances | [Reset and instances](agents/guider.md#reset-and-instances) |

## Mount Agent

[Read the complete manual](agents/mount.md). IDs are local to this manual.

| ID | User function | Recipe |
| --- | --- | --- |
| M01 | Load, select, connect and inspect capabilities | [Prepare](agents/mount.md#prepare) |
| M02 | Set site, select its source, propagate host time | [Site and time](agents/mount.md#site-and-time) |
| M03 | Set a target and slew | [Slew](agents/mount.md#slew) |
| M04 | Synchronize the mount pointing model | [Sync](agents/mount.md#sync) |
| M05 | Park and unpark the mount | [Park and unpark](agents/mount.md#park-and-unpark) |
| M06 | Find mount home | [Home](agents/mount.md#home) |
| M07 | Turn tracking on or off | [Tracking](agents/mount.md#tracking) |
| M08 | Park/unpark dome and open/close shutter | [Dome operations](agents/mount.md#dome-operations) |
| M09 | Slave dome to mount | [Dome slaving](agents/mount.md#dome-slaving) |
| M10 | Control rotator and maintain field orientation | [Rotator and derotation](agents/mount.md#rotator-and-derotation) |
| M11 | Enforce hour-angle and local-time limits | [Limits](agents/mount.md#limits) |
| M12 | Read pointing, transit, airmass and FOV | [Readouts](agents/mount.md#readouts) |
| M13 | Route joystick motion/rates/park/tracking/home/abort | [Joystick](agents/mount.md#joystick) |
| M14 | Coordinate Imager, Guider and solver agents | [Related agents](agents/mount.md#related-agents) |
| M15 | Offer an LX200 network endpoint | [LX200 server](agents/mount.md#lx200-server) |
| M16 | Abort, recover, reset defaults and manage instances | [Abort and recovery](agents/mount.md#abort-and-recovery) |
| M17 | Manual mount jog/rate and dome azimuth through the agent | [Manual controls](agents/mount.md#manual-controls) |

## Platesolver Agent and shared solver operations

[Read the complete manual](agents/platesolver.md). IDs are local to this manual.

| ID | User function | Recipe |
| --- | --- | --- |
| P01 | Load solver, prepare source and select related agents | [Prepare](agents/platesolver.md#prepare) |
| P02 | Install/remove/select native quad indexes | [Indexes](agents/platesolver.md#indexes) |
| P03 | Configure exposure and temporary solver filter | [Capture settings](agents/platesolver.md#capture-settings) |
| P04 | Restrict/accelerate solving with hints | [Hints](agents/platesolver.md#hints) |
| P05 | Capture and solve once without mount correction | [Capture and solve](agents/platesolver.md#capture-and-solve) |
| P06 | Solve images arriving from related agents | [Incoming images](agents/platesolver.md#incoming-images) |
| P07 | Solve and synchronize the mount | [Solve and sync](agents/platesolver.md#solve-and-sync) |
| P08 | Solve, sync and return to the intended center | [Center](agents/platesolver.md#center) |
| P09 | Slew to a specified target then solve/sync/center | [Precise GOTO](agents/platesolver.md#precise-goto) |
| P10 | Measure polar-alignment error with three fields | [Polar measurement](agents/platesolver.md#polar-measurement) |
| P11 | Recalculate error after mechanical adjustment | [Polar adjustment](agents/platesolver.md#polar-adjustment) |
| P12 | Read WCS, FOV and polar state | [Results](agents/platesolver.md#results) |
| P13 | Upload an existing image using native BLOB transport | [Image upload](agents/platesolver.md#image-upload) |
| P14 | Obtain original/preview images and tune JPEG display | [Preview](agents/platesolver.md#preview) |
| P15 | Abort solver, capture, mount movement or index work | [Abort and recovery](agents/platesolver.md#abort-and-recovery) |
| P16 | Legacy sync/abort compatibility, defaults and instances | [Compatibility and defaults](agents/platesolver.md#compatibility-and-defaults) |

## Astrometry Agent

[Read the complete manual](agents/astrometry.md). IDs are local to this manual.

| ID | User function | Recipe |
| --- | --- | --- |
| A01 | Load and validate external backend prerequisites | [Prepare the backend](agents/astrometry.md#prepare-the-backend) |
| A02 | Install/remove Tycho-2 or 2MASS index families | [Manage indexes](agents/astrometry.md#manage-indexes) |
| A03 | Select installed indexes and tune Astrometry hints | [Select indexes and hints](agents/astrometry.md#select-indexes-and-hints) |
| A04 | Configure source agent, exposure and solver filter | [Select agents and capture settings](agents/astrometry.md#select-agents-and-capture-settings) |
| A05 | Capture and solve once | [Solve only](agents/astrometry.md#solve-only) |
| A06 | Automatically solve incoming related-agent frames | [Incoming frames](agents/astrometry.md#incoming-frames) |
| A07 | Solve and synchronize mount | [Sync](agents/astrometry.md#sync) |
| A08 | Solve, sync and center current target | [Center](agents/astrometry.md#center) |
| A09 | Precise GOTO to a new target | [Precise GOTO](agents/astrometry.md#precise-goto) |
| A10 | Measure three-point polar error | [Polar alignment](agents/astrometry.md#polar-alignment) |
| A11 | Recalculate after manual polar correction | [Polar alignment](agents/astrometry.md#polar-alignment) |
| A12 | Read WCS/FOV/polar error | [Read results](agents/astrometry.md#read-results) |
| A13 | Solve uploaded image through native BLOB input | [Image input and preview](agents/astrometry.md#image-input-and-preview) |
| A14 | Mirror image, enable preview and configure JPEG conversion | [Image input and preview](agents/astrometry.md#image-input-and-preview) |
| A15 | Abort subprocess/download/capture/mount work and recover | [Abort and recovery](agents/astrometry.md#abort-and-recovery) |
| A16 | Legacy modes, reset defaults and shared instances | [Defaults and compatibility](agents/astrometry.md#defaults-and-compatibility) |

## Configuration Agent

[Read the complete manual](agents/configuration.md). IDs are local to this manual.

| Function ID | Exact interface | Meaning / result |
| --- | --- | --- |
| `configuration.inspect` | `AGENT_CONFIG_DRIVERS`, `AGENT_CONFIG_PROFILES`, dynamic `AGENT_CONFIG <agent-name>` | Read observed drivers, selected profile identifiers, filter selections and related-agent links. These are RO snapshots, not editable setup forms. |
| `configuration.setup-autosave` | `AGENT_CONFIG_SETUP.AUTOSAVE_DEVICE_CONFIGS` | Boolean; default false. On a service save, request `CONFIG.SAVE` on devices selected by tracked agent filters. |
| `configuration.setup-unload` | `AGENT_CONFIG_SETUP.UNLOAD_UNUSED_DRIVERS` | Boolean; default false. During restore, turn off unused driver switches instead of retaining additional currently loaded drivers. |
| `configuration.device-profiles` | Device `PROFILE.PROFILE_n`, `PROFILE_NAME.NAME_n`, `CONFIG.SAVE` | Setup-only device operations choose/name/save the profile referenced by the service snapshot; inspect the actual driver's definitions. |
| `configuration.save` | `AGENT_CONFIG_SAVE.NAME` | Text name; save the current arrangement to a named service configuration. |
| `configuration.list` | Items of `AGENT_CONFIG_LOAD` | Dynamically scanned names available for load; names are the actual switch item IDs. |
| `configuration.load` | `AGENT_CONFIG_LOAD.<exact-name> = true` | Start asynchronous restore; observe this property and `AGENT_CONFIG_LAST_CONFIG`. |
| `configuration.remove` | `AGENT_CONFIG_REMOVE.NAME` | Delete the named saved service configuration; this is the actual property name, not `AGENT_CONFIG_DELETE`. |
| `configuration.last` | `AGENT_CONFIG_LAST_CONFIG.NAME` and state | Last saved/loaded name and whether its observed arrangement still matches; `Idle` after relevant live changes does not erase the saved file. |

## Auxiliary Agent

[Read the complete manual](agents/auxiliary.md). IDs are local to this manual.

| Function ID | Exact property/items | Semantics |
| --- | --- | --- |
| `auxiliary.select-1` | `FILTER_AUX_1_LIST.<exact-device-name>` / `NONE` | Select or release slot1. |
| `auxiliary.select-2` | `FILTER_AUX_2_LIST.<exact-device-name>` / `NONE` | Select or release slot2. |
| `auxiliary.select-3` | `FILTER_AUX_3_LIST.<exact-device-name>` / `NONE` | Select or release slot3. |
| `auxiliary.select-4` | `FILTER_AUX_4_LIST.<exact-device-name>` / `NONE` | Select or release slot4. |
| `auxiliary.control-selected` | Dynamically published `AUX_1_…` through `AUX_4_…` properties | Read/change the selected device through the filter's translated properties; actual items/types/units come from that device. |
| `auxiliary.relate-imager` | `FILTER_RELATED_AGENT_LIST.<exact-Imager-Agent-name>` | Any-of-many related links; candidate validation accepts Imager Agent names. This is a relation, not another AUX device slot. |
| `auxiliary.relation-symmetry` | `FILTER_FORCE_SYMMETRIC_RELATIONS.ENABLED` / `DISABLED` | Default enabled; control reciprocal related-agent selection through the shared filter. |
| `auxiliary.instances` | `ADDITIONAL_INSTANCES.COUNT` when published on the base instance | Create extra filter-agent instances with separate selectors; discover actual instance names and limits. |

## Scripting Agent

[Read the complete manual](agents/scripting.md). IDs are local to this manual.

| Function ID | Exact property/item | Semantics |
| --- | --- | --- |
| `scripting.run-inline` | `AGENT_SCRIPTING_RUN_SCRIPT.SCRIPT` | Execute submitted JavaScript source immediately; not a filename. |
| `scripting.add` | `AGENT_SCRIPTING_ADD_SCRIPT.NAME`, `.SCRIPT` | Add a stored script with a nonempty unique display name; source is raw text. |
| `scripting.inspect-edit` | Dynamic `AGENT_SCRIPTING_SCRIPT_n.NAME`, `.SCRIPT` | Read/edit/rename a stored script; IDs are published slot IDs, not the script's display name. |
| `scripting.execute` | `AGENT_SCRIPTING_EXECUTE_SCRIPT.<published-script-property-ID>` | Execute one stored script; the switch item's label is its human name. |
| `scripting.delete` | `AGENT_SCRIPTING_DELETE_SCRIPT.NAME` | Delete by exact stored human name; not by slot ID. |
| `scripting.on-load` | `AGENT_SCRIPTING_ON_LOAD_SCRIPT.<script-ID>` | Any-of-many scripts selected for execution after configuration restoration; also has `AGENT_SCRIPTING_ADD_SCRIPT` (“New script”) as the selection default for newly added scripts. |
| `scripting.on-unload` | `AGENT_SCRIPTING_ON_UNLOAD_SCRIPT.<script-ID>` | Selected scripts run during detach; same new-script default item. |
| `scripting.events` | `indigo_devices`, `indigo_event_handlers`, callback API below | Maintain observed state and react without blocking the engine. |
| `scripting.timers` | `indigo_set_timer`, `indigo_cancel_timer` and time helpers below | One-shot scheduling and cooperative cancellation; no independent calendar/sequence job manager. |
| `scripting.publish` | Native define/redefine/update/delete functions below | Create and maintain script-owned bus properties. |
| `scripting.blobs` | BLOB observation, enable/populate/save functions below | Explicit server-side BLOB fetch/save, distinct from MCP Agent's URL-only BLOB reads. |
| `scripting.stop-recovery` | Script-managed timers/handlers and owning agent abort controls | No generic abort-script property or forced JavaScript interrupt is implemented. |

## Alpaca Agent — ASCOM device bridge

[Read the complete manual](agents/alpaca.md). IDs are local to this manual.

| Function ID | Exact interface | Meaning |
| --- | --- | --- |
| `alpaca.load-enable` | Server `DRIVERS.indigo_agent_alpaca` when offered | Loading attaches the bridge, starts UDP discovery and registers HTTP handlers immediately. Agent CONNECTION is hidden; loading starts its service without a separate HTTP enable switch. |
| `alpaca.discovery-port` | `AGENT_ALPACA_DISCOVERY.PORT` | UDP discovery port, default32227, integer0–65535; changing it restarts discovery. This is not the HTTP server port. |
| `alpaca.mapping` | `AGENT_ALPACA_DEVICES.<decimal-number>` | Persistent text mapping from numeric slot to exact INDIGO device name; actual available items/labels come from the property. |
| `alpaca.camera-bayer` | `AGENT_ALPACA_CAMERA_BAYERPAT.<published-camera-number>` | Per-camera text override, empty or RGGB/GRBG/GBRG/BGGR; invalid patterns alert. Items' labels identify the actual cameras. |
| `alpaca.management` | GET `/management/apiversions`, `/management/v1/description`, `/management/v1/configureddevices` | Discover API revision, server information and published device name/type/number/UniqueID. |
| `alpaca.setup` | `/setup…` | Redirect to INDIGO server `/mng.html`; there is not a separate per-device Alpaca configuration editor here. |
| `alpaca.connect-device` | PUT `/api/v1/<type>/<number>/connected`, form `Connected=true/false` | Native device connection through the bridge; observe actual result/connected state and underlying device messages. |
| `alpaca.camera` | `/api/v1/camera/<number>/…` | Camera metadata/settings/exposure/image transfer mapping described below. |
| `alpaca.telescope` | `/api/v1/telescope/<number>/…` | Mount pointing/tracking/park/sync and paired guider pulse functions. |
| `alpaca.filterwheel` | `/api/v1/filterwheel/<number>/…` | Filter names/offsets and zero-based position mapping. |
| `alpaca.focuser` | `/api/v1/focuser/<number>/…` | Position/move/halt/temperature compensation mapping. |
| `alpaca.rotator` | `/api/v1/rotator/<number>/…` | Position/relative/absolute/mechanical move/sync/reverse/halt. |
| `alpaca.dome` | `/api/v1/dome/<number>/…` | Available azimuth/altitude/shutter/park/home/slaving operations. |
| `alpaca.covercalibrator` | `/api/v1/covercalibrator/<number>/…` | Light brightness/on/off and cover controls when corresponding native properties exist. |
| `alpaca.switch` | `/api/v1/switch/<number>/…` | Mapped power/USB/heater/GPIO/special scalar switch channels. |
| `alpaca.disable-recover` | Server unload of this module, after clients stop | Remove HTTP routes and discovery; not a universal hardware stop/abort. |

## MCP Agent

[Read the complete manual](agents/mcp.md). IDs are local to this manual.

| Function ID | Interface | Meaning |
| --- | --- | --- |
| `mcp.lifecycle` | Server `DRIVERS.indigo_agent_mcp` when actually listed | Loading starts the MCP service immediately; unloading stops it. Verify `MCP Agent` exists and the endpoint responds. There is no agent CONNECTION toggle. |
| `mcp.list-devices` | `list_devices({})` | Devices with visible properties, including WO schemas. Names preserve service suffixes and Unicode. |
| `mcp.list-properties` | `list_properties({"device":"<exact name>"})` | Property/item schema, type, permission, state, labels; number limits/step/format and switch rule. Lists omit values/targets. |
| `mcp.get-property` | `get_property({"device":"<exact name>","property":"<exact property>"})` | Latest observed visible RO/RW values/state; excludes WO. This is cache observation, not a hardware poll. |
| `mcp.set-property` | `set_property({"device":"…","property":"…","items":{"ITEM":scalar},"token":"optional hex"})` | Partial typed writes to visible RW/WO text/number/switch properties. Unsupported light/BLOB/RO/hidden/missing targets fail explicitly. |
| `mcp.status-recovery` | Server driver lifecycle plus tool result/property state | Recover unavailable cache/service, observe real process progress, and avoid unsafe retries. |

## Test Agent — developer scaffold

[Read the complete manual](agents/test.md). IDs are local to this manual.

| Function ID | Start | Actual handler behavior / observation |
| --- | --- | --- |
| `test.test-1` | `TEST.1 = true` | Log start; sleep approximately1second in its handler; clear item1; publish `TEST=Ok`; log finish. |
| `test.test-2` | `TEST.2 = true` | Same one-second demonstration for item2. |
| `test.test-3` | `TEST.3 = true` | Same one-second demonstration for item3. |
| `test.test-4` | `TEST.4 = true` | Queue handler1 twice, handler3 with a10second delay, and handler2 with explicit priority100; clear item4 and publish `Ok`. Other queued callbacks can run after this publication. |

## Optional Sequencer library

[Sequencer functions and bootstrap prerequisites](agents/sequencer.md). This library extends the Scripting Agent after explicit provisioning; it is not automatically available when that agent is loaded.

| Function group | Recipe |
| --- | --- |
| `sequence.build` | [Construction, loops and messages](agents/sequencer.md#construction-loops-and-messages) |
| `sequence.policy` | [Failure and conditional flow](agents/sequencer.md#failure-and-conditional-flow) |
| `sequence.wait` | [Time and sky conditions](agents/sequencer.md#time-and-sky-conditions) |
| `sequence.select` | [Loading and selecting agents/devices](agents/sequencer.md#loading-and-selecting-agentsdevices) |
| `sequence.camera` | [Camera and file settings](agents/sequencer.md#camera-and-file-settings) |
| `sequence.capture` | [Preview, batches and streams](agents/sequencer.md#preview-batches-and-streams) |
| `sequence.focus` | [Focusing](agents/sequencer.md#focusing) |
| `sequence.mount` | [Mount, dome and rotator](agents/sequencer.md#mount-dome-and-rotator) |
| `sequence.guide` | [Guiding and dithering](agents/sequencer.md#guiding-and-dithering) |
| `sequence.solve-flip` | [Solving and meridian flip](agents/sequencer.md#solving-and-meridian-flip) |
| `sequence.control` | [Start, observe, pause and abort](agents/sequencer.md#start-observe-pause-and-abort) |
