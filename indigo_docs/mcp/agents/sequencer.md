# Sequencer library

**DRAFT — source-inspected operating manual. Examples have not been run by this documentation task. This manual and the canonical reference are embedded as documentation; executable library source is not installed or evaluated by MCP.**

Sequencer is a JavaScript library for the [Scripting Agent](scripting.md), not another driver or device. Its `Sequence` builder coordinates Imager, Mount, Guider and solver agents, waits for their published states and exposes sequence progress on `Scripting Agent`. Use agent processes for science operations; the library's underlying device-property writes implement its helpers rather than prescribe a separate direct-camera workflow.

## Bootstrap and operating workflow

1. Discover/load `indigo_agent_scripting` and the required science agents through `Server.DRIVERS`; load hardware drivers, complete connection/serial setup and select devices as described in [server/device preparation](../server-and-devices.md).
2. **Install/evaluate the complete Sequencer library in the intended Scripting Agent JavaScript context through a native INDIGO client or prepared agent configuration.** `boot.js` installs the base scripting API, not `Sequence`. The current library is 88,741 bytes; MCP's current request-body limit is 64 KiB before JSON escaping. One `AGENT_SCRIPTING_RUN_SCRIPT.SCRIPT` MCP request cannot bootstrap it. There is no JavaScript file-import binding: passing a filename does not load it. Do not split the library at arbitrary offsets or assume this guide installs it.
3. Confirm the library was evaluated in that same agent context, for example with the short inline script `indigo_send_message('Sequence: ' + typeof Sequence);`. It should report `function`. An interpreter reload loses runtime definitions unless configuration/on-load scripts reinstall them. Embedding the reference does not install the library; native client/configuration bootstrap remains required.
4. Discover exact agent/device names and supported items. Select devices and related agents needed for [imaging](imager.md), [guiding](guider.md), mount operation and solving before starting their processes. A Sequencer selector does not automatically establish all related-agent relationships.
5. Submit a small sequence program as JavaScript text through `AGENT_SCRIPTING_RUN_SCRIPT.SCRIPT`, or execute a stored script. Construct steps, then call `start()` once. Script evaluation completing is separate from sequence completion.
6. Monitor the sequence properties below and the owning agents. Perform explicit end-of-run shutdown steps appropriate to the equipment; completion does not automatically park, stop guiding, warm the camera or close the dome.

The following synthetic example assumes a connected camera already selected in the default Imager Agent and a pre-evaluated library. It starts real agent acquisition if executed:

```javascript
var sequence = new Sequence('Three short frames');
sequence.abort_on_failure();
sequence.capture_batch(3, 2);
sequence.send_message('Capture steps completed');
sequence.start('Imager Agent', 'Mount Agent', 'Guider Agent');
```

Builders do not form a fluent chaining API. Ordinary helper calls append execution steps; they do not immediately move equipment. `repeat()` expands its block during construction, and the two loop-display reset toggles change construction behavior immediately. Construct a fresh `Sequence(name)` for a new plan; omitted name is an empty string. Do not start another sequence while one is active.

## Complete public function inventory

This inventory covers all 108 public `Sequence.prototype` methods in the inspected library. Parenthesized parameters denote JavaScript arguments, not INDIGO item names. Numbers are numeric values, switches are booleans internally, and names are strings matching discovered names/items. Optional arguments are identified in the semantics column. All stateful operations also require the corresponding devices/properties to exist and be usable; unavailable optional hardware is not silently emulated.

| Function group | Recipe |
| --- | --- |
| `sequence.build` | [Construction, loops and messages](#construction-loops-and-messages) |
| `sequence.policy` | [Failure and conditional flow](#failure-and-conditional-flow) |
| `sequence.wait` | [Time and sky conditions](#time-and-sky-conditions) |
| `sequence.select` | [Loading and selecting agents/devices](#loading-and-selecting-agentsdevices) |
| `sequence.camera` | [Camera and file settings](#camera-and-file-settings) |
| `sequence.capture` | [Preview, batches and streams](#preview-batches-and-streams) |
| `sequence.focus` | [Focusing](#focusing) |
| `sequence.mount` | [Mount, dome and rotator](#mount-dome-and-rotator) |
| `sequence.guide` | [Guiding and dithering](#guiding-and-dithering) |
| `sequence.solve-flip` | [Solving and meridian flip](#solving-and-meridian-flip) |
| `sequence.control` | [Start, observe, pause and abort](#start-observe-pause-and-abort) |

### Construction, loops and messages

| Exact method | Semantics and input |
| --- | --- |
| `repeat(count, block)` | Expand the builder callback `block` a finite `count` times; supports nested loops and publishes loop iteration display. Use an integer count. This is not an infinite-loop command or a runtime retry. |
| `enable_reset_loop_content_state()` / `disable_reset_loop_content_state()` | Enable/disable clearing the displayed states of steps inside each loop iteration. Enabled initially; affects how subsequent loops are built, not hardware reset. |
| `enable_verbose()` / `disable_verbose()` | Queue verbose execution logging on/off. |
| `evaluate(code)` | Evaluate JavaScript text when this step executes. Its earlier side effects survive an exception; it is not a sandbox or rollback transaction. |
| `send_message(message)` | Send the supplied string as an INDIGO message at this step. |

```javascript
var sequence = new Sequence('Two filter rounds');
sequence.repeat(2, function () {
  sequence.select_filter('L'); // Replace with an actual published filter name.
  sequence.capture_batch(2, 5);
});
sequence.start();
```

### Failure and conditional flow

| Exact method | Semantics and result |
| --- | --- |
| `abort_on_failure()` | Default policy: a failed step ends sequence execution with `Alert`. This is not a guaranteed all-equipment emergency stop. |
| `continue_on_failure()` | Mark a failed step `Alert` and continue. Final sequence `Ok` can therefore coexist with failed steps; inspect step states. |
| `recover_on_failure()` | After a failure skip forward to the next recovery point; without one, fail the sequence. Does not retry or reverse previous side effects. |
| `recovery_point()` | Mark a forward recovery destination for the above policy. Put explicit repair/cleanup after the marker. |
| `resume_point()` | Mark a forward destination for a triggered conditional break. Different from failure recovery; without a destination a break finishes the remaining plan rather than executing skipped steps. |

Policies take effect when their queued step runs. Recovery paths must consider which preceding microsteps already succeeded. For example, an interrupted helper may have changed batch settings before failing; inspect actual properties before a restart. Failure policy alone does not park or close a dome. Explicit operator abort has different cancellation behavior described below.

### Time and sky conditions

| Exact method | Semantics and units/prerequisites |
| --- | --- |
| `wait(delay)` | Nonblocking scheduled delay, seconds. |
| `wait_until(time)` | Wait until a UTC string accepted by the scripting time helpers, or a numeric epoch timestamp in seconds passed to `indigo_time_to_delay`. A passed target advances immediately; prefer a validated UTC string for clarity. |
| `break_at(time)` | When this step is reached, test the supplied UTC string or numeric epoch seconds; if reached, skip forward to `resume_point()`. Does not arm a general interrupt during another long-running step. |
| `break_at_ha(limit)` | Test hour-angle limit at this step; numeric hours or a sexagesimal string decoded by `indigo_stod`. Requires mount coordinates/site context. |
| `wait_until_solar_altitude_below(limit)` | Poll until solar altitude falls below the limit in degrees; requires valid observing location. |
| `wait_until_target_altitude_above(limit, ra, dec)` | Poll until target altitude exceeds degrees `limit`; RA is hours, DEC degrees. Requires location/time. |
| `break_if_solar_altitude_above(limit)` | At this step, skip to resume point if solar altitude exceeds degrees `limit`. |
| `break_if_target_altitude_below(limit, ra, dec)` | At this step, skip to resume point if target altitude is below degrees `limit`; RA hours, DEC degrees. |

Site coordinates come from the selected Mount Agent's available GPS/geographic properties. Validate location and time first. Sky-condition breaks are evaluated where placed; repeat them between batches if required by the plan. A warning for waits/breaks more than12 hours ahead does not prohibit that schedule. None of these conditions is a substitute for autonomous equipment protection.

### Loading and selecting agents/devices

| Exact method | Target and semantics |
| --- | --- |
| `load_config(name)` | Select named `AGENT_CONFIG_LOAD` item on the default `Configuration Agent`. Loading configuration can reconfigure selected equipment. |
| `load_driver(name)` / `unload_driver(name)` | Set the named `DRIVERS` item true/false on literal `Server`. Use the discovered module item ID. There is no server-name argument or implicit routing to an arbitrary remote server. Unloading active participants can break the sequence. |
| `select_imager_agent(agent)` / `select_mount_agent(agent)` / `select_guider_agent(agent)` | Queue a new exact device name for subsequent role-specific helpers; include a remote ` @ host` suffix when published. |
| `select_solver_agent(agent)` | Queue an explicit solver device name. Requires a loaded compatible solver; start also selects a default as described below. |
| `select_imager_camera(camera)` | Imager `FILTER_CCD_LIST`. |
| `select_filter_wheel(wheel)` | Imager `FILTER_WHEEL_LIST`. |
| `select_focuser(focuser)` | Imager `FILTER_FOCUSER_LIST`. |
| `select_rotator(rotator)` | Mount `FILTER_ROTATOR_LIST`. |
| `select_mount(mount)` | Mount `FILTER_MOUNT_LIST`. |
| `select_dome(dome)` | Mount `FILTER_DOME_LIST`. |
| `select_gps(gps)` | Mount `FILTER_GPS_LIST`. |
| `select_guider_camera(camera)` | Guider `FILTER_CCD_LIST`. |
| `select_guider(guider)` | Guider output `FILTER_GUIDER_LIST`; separate from its guide camera. |

Use discovered exact selector item names. Selection changes the owning agent's proxy surface; allow it to settle and recheck required properties. `start()` initially chooses a loaded solver, preferring the selected imager's host, then local, then another host. Later `select_imager_agent()` alone does not reselect a still-loaded solver: explicitly select the intended solver. If a chosen solver disappears, the library can search for another loaded compatible one when used. Solver camera/related-agent configuration must still be correct.

### Camera and file settings

These queued settings act through the selected Imager Agent's camera proxy. They are scalar configuration steps, not capture processes. Supported modes/items and numeric ranges depend on the actual camera.

| Exact method | Target, input and side effects |
| --- | --- |
| `select_frame_type(type)` | `CCD_FRAME_TYPE`, discovered switch item (light/dark/flat/bias support is camera-dependent). |
| `select_image_format(format)` | `CCD_IMAGE_FORMAT`, discovered format switch. |
| `set_frame(left, top, width, height)` / `reset_frame()` | `CCD_FRAME.LEFT/TOP/WIDTH/HEIGHT`, sensor coordinates/dimensions in pixels. Reset requests an oversized full-frame extent for framework/device clipping; inspect resulting ROI. |
| `select_camera_mode(mode)` | `CCD_MODE`, discovered mode switch. |
| `set_gain(value)` / `set_offset(value)` / `set_gamma(value)` | `CCD_GAIN.GAIN`, `CCD_OFFSET.OFFSET`, `CCD_GAMMA.GAMMA`; device-defined numeric scales/ranges. |
| `select_program(program)` / `select_aperture(aperture)` / `select_shutter(shutter)` / `select_iso(iso)` | Corresponding `DSLR_PROGRAM`, `DSLR_APERTURE`, `DSLR_SHUTTER`, `DSLR_ISO` switch items; require those optional DSLR properties. Use published keys/labels, not assumed universal values. |
| `enable_cooler(temperature)` / `disable_cooler()` | `CCD_COOLER.ON/OFF`; enable also sets `CCD_TEMPERATURE.TEMPERATURE`, degrees Celsius. Check camera temperature/state before critical acquisition; reaching a setpoint depends on camera behavior. |
| `set_fits_header(keyword, value)` / `remove_fits_header(keyword)` | `CCD_SET_FITS_HEADER.KEYWORD/VALUE` or removal property; changes subsequent image metadata. Strings, subject to camera support. |
| `select_filter(filter)` | Imager `AGENT_WHEEL_FILTER`, published filter name or slot represented as a string. Requires a selected wheel. |
| `enable_filter_offsets()` / `disable_filter_offsets()` | Imager `AGENT_PROCESS_FEATURES.APPLY_FILTER_OFFSETS`; enabling requires configured offsets/focuser. Can cause physical focus changes during filter selection. |
| `set_directory(directory)` | Camera `CCD_LOCAL_MODE.DIR`: a path on the INDIGO server host, not the MCP client's filesystem. |
| `set_file_template(template)` | `CCD_LOCAL_MODE.PREFIX`, camera-supported filename template. |
| `set_object_name(name)` | `CCD_LOCAL_MODE.OBJECT`, object naming field. |

### Preview, batches and streams

| Exact method | Start, wait and result |
| --- | --- |
| `start_preview(exposure)` | Set Imager batch count0 and exposure seconds, start `AGENT_START_PROCESS.PREVIEW`; advance when the process becomes `Busy`, leaving preview running. |
| `stop_preview()` | Abort the Imager process if running and wait for process completion; immediately advance if idle. It addresses the Imager main process, so do not substitute it for a narrowly scoped unrelated stop. |
| `capture_batch(count, exposure)` / `capture_batch(name_template, count, exposure)` | Optional filename prefix, numeric frame count and exposure seconds. Set `AGENT_IMAGER_BATCH.COUNT/EXPOSURE`, camera upload mode `BOTH`, then run Imager `AGENT_START_PROCESS.EXPOSURE` to completion. |
| `capture_stream(count, exposure)` / `capture_stream(name_template, count, exposure)` | Same overloads/settings, run Imager `STREAMING`; requires streaming-capable selected camera and agent support. |

Use finite positive counts for a finite plan. Batches/streams leave modified upload/naming/batch settings in place; they do not restore all previous settings. File writes occur on the server according to its camera local-mode settings. Sequence exposure totals count planned exposure time, not readout, focusing, solving, cooling, guiding or dithering overhead. Review Imager process/statistics and produced frames, rather than treating exposure progress alone as proof of complete data delivery.

### Focusing

| Exact method | Semantics and prerequisites |
| --- | --- |
| `set_manual_focuser_mode()` / `set_automatic_focuser_mode()` | Proxy `FOCUSER_MODE.MANUAL/AUTOMATIC`; camera/focuser-dependent operating mode, not an autofocus start. |
| `focus(exposure)` | Save Imager batch settings, set a single exposure of the supplied seconds, run Imager `FOCUSING`, then restore batch settings on the normal path. Configure stars/ROI, algorithm, travel/backlash and focuser first in the Imager. |
| `focus_ignore_failure(exposure)` | Same helper, but tolerate its focus-process failure. Does not change the general failure policy for all subsequent operations. |
| `clear_focuser_selection()` | Imager `AGENT_START_PROCESS.CLEAR_SELECTION`, clears autofocus star selection. |
| `set_focuser_position(position)` | Select optional `FOCUSER_ON_POSITION_SET.GOTO`, then absolute `FOCUSER_POSITION.POSITION`, device steps; requires an absolute-position-capable focuser. |

Watch Imager focus statistics, process state and focuser motion. On interruption/failure check batch settings and actual position; do not assume the helper reached its restoration step or that a failed focus recovered the starting position.

### Mount, dome and rotator

| Exact method | Semantics and units |
| --- | --- |
| `slew(ra, dec)` | Set Mount `AGENT_MOUNT_EQUATORIAL_COORDINATES.RA/DEC` (hours/degrees in the selected mount's declared epoch; see [Mount coordinate epochs](mount.md#coordinate-epochs)), then `AGENT_START_PROCESS.SLEW`; wait for completion. |
| `park()` / `unpark()` / `home()` | Mount `AGENT_START_PROCESS.PARK/UNPARK/HOME`. Availability and safe motion prerequisites are equipment-dependent. |
| `enable_tracking()` / `disable_tracking()` | Mount proxy `MOUNT_TRACKING.ON/OFF`. |
| `dome_slew(az)` | Optional dome GOTO mode, then `DOME_HORIZONTAL_COORDINATES.AZ` in degrees; wait for completion. |
| `dome_park()` / `dome_unpark()` / `dome_open()` / `dome_close()` | Mount `AGENT_START_PROCESS.DOME_PARK/DOME_UNPARK/DOME_OPEN/DOME_CLOSE`; shutter/motion side effects require a selected capable dome. |
| `enable_ha_limit()` / `disable_ha_limit()` | Toggle Mount hour-angle-limit feature; configure its actual limit beforehand. |
| `enable_time_limit()` / `disable_time_limit()` | Toggle Mount time-limit feature; configure its limit beforehand. |
| `enable_dome_slaving()` / `disable_dome_slaving()` | Toggle Mount dome slaving; requires selected/configured dome and valid mount/site geometry. |
| `make_dome_slaving_persistent()` / `make_dome_slaving_not_persistent()` | Set/clear the Mount feature controlling persistence of dome slaving. |
| `enable_field_derotation()` / `disable_field_derotation()` | Toggle Mount field derotation; requires a selected rotator and correct geometry/site data. |
| `make_field_derotation_persistent()` / `make_field_derotation_not_persistent()` | Set/clear the corresponding persistence feature. |
| `enable_joystick_control()` / `disable_joystick_control()` | Toggle Mount joystick-control feature; requires a joystick device selected through Mount Agent `FILTER_JOYSTICK_LIST`. |
| `wait_for_gps()` | Wait if Mount proxy `GPS_GEOGRAPHIC_COORDINATES` is `Busy`. Missing property fails; a non-Busy state advances, so this is not proof of an accurate fix or of `Ok`. Inspect GPS result. |
| `set_rotator_angle(value)` | Optional `ROTATOR_ON_POSITION_SET.GOTO`, then Mount proxy `ROTATOR_POSITION.POSITION`, degrees; wait for the operation. |

These helpers use the Mount Agent's sequencing/interlocks and proxies. Review its limits and dome coordination first. Completion reports the observed property/process state, not an independent physical safety check.

### Guiding and dithering

| Exact method | Semantics and units |
| --- | --- |
| `calibrate_guiding(exposure, step)` | Optionally set exposure seconds and `AGENT_GUIDER_SETTINGS.STEP0` calibration-pulse seconds; run Guider `AGENT_START_PROCESS.CALIBRATION` to completion. Both arguments may be omitted to retain settings. Requires selected guide camera/output and configured mount/guide orientation. |
| `start_guiding(exposure)` | Optional exposure seconds, then Guider `GUIDING`; advance when the process becomes `Busy`, leaving guiding running. Check calibration/stars and quality statistics. |
| `start_guiding_exposure(exposure)` | Deprecated older helper with required exposure; use `start_guiding(exposure)` for new plans. |
| `stop_guiding()` | Abort the Guider main process if Busy and await completion; advance directly if idle. |
| `clear_guider_selection()` | Guider `AGENT_START_PROCESS.CLEAR_SELECTION`, clears guide-star selection. |
| `enable_dithering(amount, time_limit, skip_frames)` | Set Guider `DITHERING_MAX_AMOUNT` (pixels), `DITHERING_SETTLE_TIME_LIMIT` (seconds), Imager batch `FRAMES_TO_SKIP_BEFORE_DITHER` (frame count), then Imager `ENABLE_DITHERING` feature. Requires guiding and related-agent wiring; does not start guiding. |
| `disable_dithering()` | Disable Imager's dithering feature; does not stop guiding or restore previous Guider settings. |

Dithering is performed by the agent capture workflow between frames. Monitor Guider settling and Imager progress together. This is not a helper for issuing manual guide pulses.

### Solving and meridian flip

| Exact method | Semantics and prerequisites |
| --- | --- |
| `sync_center(exposure)` | Set selected solver `AGENT_PLATESOLVER_EXPOSURE.EXPOSURE` seconds, start solver `AGENT_START_PROCESS.CENTER`; requires its imager/mount relationships, usable solver and suitable image/search settings. |
| `set_solver_filter(filter)` | Solver `AGENT_PLATESOLVER_FILTER` selection by filter/slot; `0` maps to `CURRENT`. A subsequent solver capture uses its filter policy; this alone is not an ordinary Imager filter change. |
| `precise_goto(exposure, ra, dec)` | Set solver exposure seconds and `AGENT_PLATESOLVER_GOTO_SETTINGS.RA/DEC` hours/degrees in the solver `AGENT_PLATESOLVER_HINTS.EPOCH` frame, start solver `AGENT_START_PROCESS.PRECISE_GOTO`; wait for completion. |
| `enable_meridian_flip(use_solver, time)` | Boolean recenter/solve choice plus transit-pause offset; configure flipper and enable Imager `PAUSE_AFTER_TRANSIT`. **Draft gap: nonzero `time` units conflict between the canonical library manual (seconds) and implementation (passes unchanged to Imager's hours item). Do not use a guessed nonzero offset.** |
| `disable_meridian_flip()` | Disable Imager transit-pause feature; does not reverse a completed flip or other previous settings. |

Flip handling coordinates the Imager transit pause, Mount flip requirement, optional solving/recentering and resumption of guiding/settling. It temporarily changes the HA-limit state and restores it on its recovery path. Enabling flip can persist after the sequence ends; observe `FLIPPER_STATE` as well as sequence/agent states. After a failed flip, guiding is not automatically restarted; inspect mount side/coordinates, camera pause and guide state before explicitly resuming. A failure can stop participating processes even after the enabling sequence has finished. The units discrepancy must be resolved before advertising a nonzero-offset recipe.

### Start, observe, pause and abort

`start(imager_agent, mount_agent, guider_agent)` begins the built plan. Optional names default to `Imager Agent`, `Mount Agent`, `Guider Agent`. It also chooses the initial solver as above. Exact role names, including host suffixes, matter. The library installs its own event handlers and properties on `Scripting Agent`; these are not native base Scripting Agent controls before library evaluation.

| Exact property/items on Scripting Agent | Interpretation/control |
| --- | --- |
| `SEQUENCE_NAME.NAME` | Read-only sequence display name. |
| `SEQUENCE_STATE.STEP` | Current zero-based user step. Property state reports overall `Busy`, `Ok` or `Alert`. |
| `SEQUENCE_STATE.PROGRESS`, `.PROGRESS_TOTAL` | Completed/planned internal microsteps; one builder can generate several. |
| `SEQUENCE_STATE.EXPOSURE`, `.EXPOSURE_TOTAL` | Exposure progress/planned total in seconds; excludes overhead and other operation durations. |
| `SEQUENCE_STEP_STATE.<published-step-item>` | Per-step light state; discover dynamic numeric-string keys. Check for `Alert` even when continue-on-failure produces overall `Ok`. |
| `LOOP_n.STEP`, `.COUNT` | Dynamic loop display: step and current zero-based iteration. Discover existing loop properties. |
| `AGENT_PAUSE_PROCESS.PAUSE_WAIT` | Boolean pause/resume request. Pauses sequence advancement; during a capture batch forwards an Imager pause. It is not a universal instant stop of mount/dome/focus operations. |
| `AGENT_ABORT_PROCESS.ABORT` | True requests explicit sequence cancellation and participating process abort handling. Check each actual agent afterward. |
| `SEQUENCE_RESET.RESET` | Reset displayed state/progress/loop information while idle; does not abort hardware, restore equipment settings or reinstall the library. |
| `FLIPPER_STATE.WAITING_FOR_TRANSIT`, `.WAITING_FOR_SLEW`, `.WAITING_FOR_SYNC_AND_CENTER`, `.WAITING_FOR_GUIDING`, `.WAITING_FOR_SETTLE_DOWN`, `.RESUME_GUIDING`, `.USE_SOLVER` | Read-only flip phase/options; observe property state and messages for failure. |

Explicit abort cancels pending sequence waits/timer generation and requests aborts on participating agents, including the solver. **Already-running Guider `GUIDING` is intentionally left running by this path.** To stop it, explicitly request the Guider abort/stop operation and verify completion. Abort does not imply parking, shutter closure or camera warming. A pause/abort with no active sequence can report `Alert`.

After failure/abort: read sequence step states/messages; inspect owning agent processes and physical positions; stop any unwanted continuing activity through its agent; repair the failed prerequisite; reset display only after idle if useful; build a fresh plan for the remaining work. There is no automatic rollback, checkpoint restart, or public `Sequence.abort()` method. Do not call private singleton internals as though they were supported builder functions.

## Offline reference and verification limits

This page supplies the public function inventory and operating contract. The complete canonical Sequencer reference and example collection are also embedded at `indigo://help/reference/sequencer.md`, retrievable with `get_help({"topic":"reference/sequencer.md"})`. Neither GitHub access nor installed documentation files are required. This reference contains a known seconds-versus-hours discrepancy for the meridian-flip offset; the warning above remains authoritative until the discrepancy is resolved. Embedding documentation does not embed/evaluate Sequencer.js or remove the native bootstrap prerequisite.

Maintainer evidence: [Sequencer implementation](../../../indigo_drivers/agent_scripting/library/Sequencer.js), [canonical reference](../../../indigo_drivers/agent_scripting/library/Sequencer.md), [base boot API](../../../indigo_drivers/agent_scripting/boot.js), [MCP request limit](../../../indigo_drivers/agent_mcp/indigo_agent_mcp.c), [property reference](../../PROPERTIES.md). These source links support review; the embedded user-facing manual is usable without source files. This draft was checked against source, not executed against a scripting runtime, simulators or hardware.
