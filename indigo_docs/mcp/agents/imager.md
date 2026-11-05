# Imager Agent operating manual — draft

This is an offline-ready draft for owner review. It describes source-visible behavior, not a runtime-validated observing procedure. This manual is included in compiled MCP help. Names below are property/item identifiers, not labels to guess from. Discover the actual device name and property schema on the connected server before writing; keep any instance number or `@ host` suffix.

## Purpose and preparation

Load module `indigo_agent_imager` to obtain `Imager Agent` (additional instances have distinct names). It coordinates the imaging camera, filter wheel, focuser and optional external shutter, and cooperates with Guider, Mount and solver agents. It owns preview, exposure batches, streaming, autofocus and synchronized camera processes. A multi-step observing sequence uses the [Sequencer library](sequencer.md) through the [Scripting Agent](scripting.md), after explicit library provisioning; the current Imager process menu does not expose a sequence execution command.

Use the [server and device preparation guide](../server-and-devices.md): discover the Server device; if the agent is missing, select its existing `DRIVERS.indigo_agent_imager` switch and wait for server loading to finish and the agent to appear. Load each needed hardware driver through the same mechanism. Set serial port/model/network prerequisites on the physical device while disconnected. Then select the devices on the agent, wait for preparation to complete, configure the agent and start its process. Ordinary imaging is performed through this agent, rather than by sending individual physical-camera exposures.

| Selection on Imager Agent | Meaning and readiness |
| --- | --- |
| `FILTER_CCD_LIST` | Select one camera by its exact dynamic switch name. The shared filter prepares/connects the selected device and republishes its supported camera properties on the agent. Wait until the list leaves BUSY successfully and camera properties appear. |
| `FILTER_WHEEL_LIST` | Optional filter wheel. Selection exposes wheel properties and populates the agent filter menu. |
| `FILTER_FOCUSER_LIST` | Needed for autofocus, bracketing and agent manual focus control. |
| `FILTER_AUX_1_LIST` | Optional external shutter of AUX shutter interface, not a second arbitrary camera. The agent coordinates camera exposure and external shutter internally. |
| `FILTER_RELATED_AGENT_LIST` | Select cooperating agents by their listed names. First matching Guider is used for dithering; Mount supplies transit/abort coordination; solver-related agents cooperate on solving; related Imagers participate in barriers. A relation is not the hardware selection. |

Select/deselect only when processes are idle. The shared selector refuses changes during a process and reports preparation errors/timeouts. Its `NONE` item deselects a device; discover the actual item rather than assuming a hardware name. Selection can clear star coordinates, and a changed binning/frame can invalidate coordinates. Configure image type, binning, gain/offset, cooler, image format, upload mode and local file naming through the selected camera's **agent-side proxy properties** when available. Discover their schema; capability is camera-dependent. Configure wheel names/offsets through wheel proxies. Do not infer that a loaded module or a completed JSON-RPC call means hardware preparation succeeded.

Source: [Imager implementation](../../../indigo_drivers/agent_imager/indigo_agent_imager.c), [device-name declaration](../../../indigo_drivers/agent_imager/indigo_agent_imager.h), [shared selection/proxy implementation](../../../indigo_libs/indigo_filter.c), [canonical property reference](../../PROPERTIES.md).

## Function inventory

The IDs identify complete user function groups; the recipe anchors cover the corresponding controls and readbacks. A scalar setting has no independent “start” or “abort”; its update/configuration state is the acknowledgement. A process starts only through its command and must be observed to completion.

| ID | User function | Recipe |
| --- | --- | --- |
| IMG-01 | Load, choose camera/wheel/focuser/shutter and related agents | [Preparation](#purpose-and-preparation) |
| IMG-02 | Configure capture/storage/format/optics through selected-device proxies | [Preparation](#purpose-and-preparation) |
| IMG-03 | Capture one plain frame with a separate duration control | [Single capture](#single-capture) |
| IMG-04 | Preview one processed frame | [Preview](#preview) |
| IMG-05 | Run continuous processed preview | [Preview](#preview) |
| IMG-06 | Acquire finite or indefinite exposure batch | [Exposure batches](#exposure-batches) |
| IMG-07 | Run camera streaming batch | [Streaming](#streaming) |
| IMG-08 | Run signed focus-bracketing/macro batch | [Bracketing](#bracketing) |
| IMG-09 | Choose focus estimator and run U-curve autofocus | [Autofocus](#autofocus) |
| IMG-10 | Run iterative HFD/peak, RMS-contrast or Bahtinov autofocus | [Autofocus](#autofocus) |
| IMG-11 | Set autofocus backlash/stack/retry/failure policy | [Focus settings](#focus-settings) |
| IMG-12 | Find stars, choose a listed star, set multistar/regions/subframe | [Star selection](#star-selection) |
| IMG-13 | Clear star selection | [Star selection](#star-selection) |
| IMG-14 | Select a filter and optionally apply focus offsets | [Filter and manual focus](#filter-and-manual-focus) |
| IMG-15 | Jog focuser inward/outward and stop | [Filter and manual focus](#filter-and-manual-focus) |
| IMG-16 | Enable batch dithering, choose cadence and last-frame policy | [Dithering and transit](#dithering-and-transit) |
| IMG-17 | Automatically pause a batch around transit | [Dithering and transit](#dithering-and-transit) |
| IMG-18 | Pause with abort, pause with wait and resume | [Pause and abort](#pause-and-abort) |
| IMG-19 | Abort an active process and recover | [Pause and abort](#pause-and-abort) |
| IMG-20 | Configure breakpoints, leader/follower barriers and group control | [Synchronization](#synchronization) |
| IMG-21 | Refresh stored-file list, download a listed image | [Stored images](#stored-images) |
| IMG-22 | Delete a listed stored image; inspect disk capacity | [Stored images](#stored-images) |
| IMG-23 | Read process/image/focus/selection diagnostics | [Monitoring](#monitoring) |
| IMG-24 | Reset agent defaults, save/restore configuration, create instances | [Reset and instances](#reset-and-instances) |

## Common execution contract

`AGENT_START_PROCESS` is an at-most-one switch vector: set exactly the desired operation item to boolean `true`. Wait for BUSY and then a terminal state/readback; operations clear their switch when finished. An empty selection starts nothing. Concurrent starts are ignored while this process, star finding or plain capture is BUSY. Check the command property, associated statistics, device results and messages together. After an error, wait for motion/exposure cleanup before retrying. Settings that merely return OK are not proof of focus, a saved file or an exposure.

Normal workflows use `AGENT_ABORT_PROCESS.ABORT=true` to stop. Specific exceptions and terminal-state caveats are described below. The agent issues low-level camera/shutter/focuser commands itself; these are implementation actions, not additional user steps.

## Single capture

**IMG-03.** With a camera selected and no active main process (or a deliberately paused process), set number `AGENT_IMAGER_CAPTURE.CAPTURE` to the desired exposure in seconds. This starts capture immediately; it is not the same setting as the batch exposure. The property becomes BUSY and its value reports remaining exposure while its target retains the requested duration. The agent temporarily requests client upload and RAW image format, captures/downloads a frame, and restores those mode selections. Wait for `AGENT_IMAGER_CAPTURE` OK or ALERT and an image result on the agent camera proxy.

This path does not run the preview focus-estimation pipeline. While capturing, stop through the **agent-side** camera abort-exposure proxy if needed: that request sets the dedicated capture-aborted flag. `AGENT_ABORT_PROCESS` also invokes camera abort, but the plain capture has its own completion state. An abort/failure produces ALERT on `AGENT_IMAGER_CAPTURE`; re-read it and camera state before another capture. Never substitute a direct physical-camera exposure when this agent operation fails.

Source: `capture_plain_frame`, `capture`, capture handler and agent-side abort interception in the Imager implementation.

## Preview

**IMG-04/05.** Set `AGENT_IMAGER_BATCH.EXPOSURE` (seconds), select an estimator/selection if focus diagnostics are desired, then start `AGENT_START_PROCESS.PREVIEW_1=true` for one processed frame or `.PREVIEW=true` for continuous acquisition. These operations require a camera, not a focuser. They acquire RAW client images internally and calculate the selected image/focus diagnostics, temporarily preserving/restoring camera upload and image-format selections. Continuous preview ends on agent abort or acquisition/processing failure.

Monitor agent image output and `AGENT_IMAGER_STATS`, including HFD/FWHM/peak or RMS/Bahtinov measurements as appropriate. Single/continuous preview clear their start switch when done. **Source caveat:** these preview wrappers set `AGENT_START_PROCESS` and statistics back to OK even after reporting “Preview failed.” Do not classify a preview as successful from that final OK alone; require the expected fresh image/statistics and inspect messages. On failure, abort if still running, verify selected camera output/RAW support, adjust exposure/selection and retry at the agent.

## Agent-level payload example

After the discovered camera selector is ready and output policy configured, these are illustrative `set_property` argument objects for a discovered device named exactly `Imager Agent`. Substitute the actual name; the numeric choices describe an example, not a validated exposure preset:

```json
{"device":"Imager Agent","property":"AGENT_IMAGER_BATCH","items":{"COUNT":10,"EXPOSURE":60,"DELAY":0,"FRAMES_TO_SKIP_BEFORE_DITHER":-1}}
```

Read the accepted batch settings, confirm `MACRO_MODE` is false, then start:

```json
{"device":"Imager Agent","property":"AGENT_START_PROCESS","items":{"EXPOSURE":true}}
```

Observe the agent's main process/statistics and image/file results. To stop this finite batch:

```json
{"device":"Imager Agent","property":"AGENT_ABORT_PROCESS","items":{"ABORT":true}}
```

Wait for cleanup and final states; a stopped finite batch can end ALERT. These are existing agent property operations, not a proposed new MCP astronomy-command API.

## Exposure batches

**IMG-06.** Prepare file/upload/image-type settings on camera proxies, disable `AGENT_PROCESS_FEATURES.MACRO_MODE`, and configure `AGENT_IMAGER_BATCH`:

| Numeric item | Meaning |
| --- | --- |
| `COUNT` | Number of frames; `-1` repeats until aborted; `0` requests no frames. |
| `EXPOSURE` | Seconds per frame. |
| `DELAY` | Seconds of delay after each completed exposure in the batch loop, including its final loop iteration. |
| `FRAMES_TO_SKIP_BEFORE_DITHER` | `-1` disables dithering; `0` requests after every eligible frame; `N` skips N frames before the next dither (cadence N+1). |
| `PAUSE_AFTER_TRANSIT` | Transit pause offset in **hours**, used only with the corresponding feature and valid related Mount transit information. |

Start `AGENT_START_PROCESS.EXPOSURE=true`. Monitor `AGENT_START_PROCESS`, `AGENT_IMAGER_STATS.FRAME/FRAMES/EXPOSURE/DELAY/PHASE` and camera image/file results. A batch may retry a failed exposure up to three attempts, so command delivery alone does not predict frame count. Ordinary finite success ends OK, clears EXPOSURE and sets phase IDLE. Finite abort/failure ends ALERT; aborting `COUNT=-1` is treated as stopping a successful indefinite batch. Use the process abort recipe rather than racing the selected camera.

Camera upload policy determines whether results arrive as client images, local files or both. An exposure-completion state alone does not prove a particular file exists: inspect file output/listing. Optional shutter control, FITS metadata, mount abort permission and dither coordination are performed internally.

## Streaming

**IMG-07.** Require a camera with the proxied `CCD_STREAMING` capability. Set batch `COUNT` and `EXPOSURE`, then `AGENT_START_PROCESS.STREAMING=true`. The agent issues one streaming request carrying count/exposure; this is not a software loop of normal exposures. Monitor start-process state, streaming proxy state and `AGENT_IMAGER_STATS.FRAME/FRAMES`. The current implementation copies the camera's remaining-stream-count readback into its FRAME statistic, so do not assume it is always completed-frame count.

Abort through the agent; finite abort/failure ends ALERT, while stopping an indefinite `COUNT=-1` stream is reported OK. Ordinary batch delay/dither/breakpoint behavior is not implemented by `streaming_batch`; do not promise it for streaming. Select an exposure batch when that orchestration is required. Confirm generated video/image files using camera output and the file workflow.

## Bracketing

**IMG-08.** Require camera and focuser. Set `AGENT_PROCESS_FEATURES.MACRO_MODE=true`, batch COUNT/EXPOSURE and `AGENT_IMAGER_FOCUS.BRACKETING_STEP` in focuser steps. Positive step moves outward between frames; negative moves inward. Start the same `AGENT_START_PROCESS.EXPOSURE` command. The agent captures at successive focus positions and attempts to return by its accumulated offset at completion, abort or motion failure.

Observe process state, FRAME and the proxied focuser position/motion; allow the return move to finish before using the position again. Abort through the agent. This is **focus-position bracketing**, not automatic exposure-duration bracketing. The bracketing loop does not implement the ordinary batch's full dither/delay/breakpoint orchestration. Do not reuse an indefinite batch here without a deliberate stopping plan. After an interrupted/failed return, read the actual focuser position; a request to restore position is not proof of physical restoration.

## Autofocus

**IMG-09/10.** Require a camera and motor focuser; choose an appropriate star/field and batch exposure first. Pick one boolean item on `AGENT_IMAGER_FOCUS_ESTIMATOR`, configure the settings below, then start `AGENT_START_PROCESS.FOCUSING=true`. Changing estimator is refused during an active process except continuous preview; plan estimator/selection while idle.

| Estimator item | Measurement and actual operation |
| --- | --- |
| `U_CURVE` | Uses selected-star HFD at sampled focuser positions and polynomial fitting to estimate the best focus position. Multiple star estimates can be combined. Needs measurable stars, suitable sample spacing and both sides of the minimum; it does not solve arbitrary severe defocus automatically. |
| `HFD_PEAK` | Iterative star HFD/peak optimization, reducing movement from initial to final step. Uses one star for this estimator even if a larger selection count is configured. |
| `RMS_CONTRAST` | Iterative image RMS-contrast optimization, useful for a structured field; star selection is not the metric. Saturation can mislead the metric, and saturated areas may be masked/reset internally. |
| `BAHTINOV` | Iterative diffraction-spike symmetry measurement; requires a suitable bright target and a physical Bahtinov mask. The implementation warns when frame dimensions exceed 500 pixels; use an appropriate smaller camera ROI. Other estimators must not be assumed valid with that mask in place. |

During focusing monitor `AGENT_START_PROCESS`, `AGENT_IMAGER_STATS` (FOCUS_POSITION, FOCUS_OFFSET, BEST_FOCUS_DEVIATION and estimator-specific values), and actual focuser position. U-curve/iterative execution may move in both directions and capture many images. It temporarily selects manual focuser mode, client upload and RAW image format, then restores the saved mode selections; automatic star subframes are also restored on the cleanup path. It disables a related solver's automatic solving and adjusts related Mount abort coordination. Do not run another client against the same focuser/camera concurrently.

Success sets the process OK and clears FOCUSING. Failure or abort sets it ALERT and clears the abort state during cleanup. Before retrying, inspect the error, exposure, star saturation/ROI, calibration of focuser travel/backlash and actual final position. A saved temperature-compensation/mode setting being restored does not validate hardware focus.

## Focus settings

**IMG-11.** `AGENT_IMAGER_FOCUS` contains scalar numbers. They tune the next autofocus/bracketing operation and do not start it:

| Item | Unit and effect |
| --- | --- |
| `ITERATIVE_INITIAL`, `ITERATIVE_FINAL` | Focuser steps: coarse starting increment and fine ending increment for iterative methods. |
| `U_CURVE_SAMPLES`, `U_CURVE_STEP` | Integer sample count (6–24 in this source) and focuser steps between fit samples. |
| `BAHTINOV_SIGMA` | Dimensionless diffraction-spike detection threshold. |
| `BRACKETING_STEP` | Signed steps between frames in macro/bracketing mode. |
| `BACKLASH`, `BACKLASH_OVERSHOOT_FACTOR` | Focuser steps and factor. Factor 1 uses ordinary compensation; greater than 1 uses overshoot/return compensation. Updating BACKLASH also updates the selected focuser's backlash proxy when present. |
| `STACK` | Frames evaluated per focus measurement (1–5). |
| `REPEAT`, `DELAY` | Additional failure-retry count and initial retry delay in seconds; delay doubles between retries. REPEAT is not a schedule to refocus repeatedly after success. |
| `INITIAL`, `FINAL` | Obsolete compatibility controls. If newer corresponding controls were not changed in the request, legacy changes can update iterative/U-curve initial step or iterative final step. Prefer the explicit modern items. |

`AGENT_IMAGER_FOCUS_FAILURE.STOP/RESTORE` chooses failure recovery; RESTORE attempts the starting position on supported autofocus failure paths. RMS contrast explicitly disables that restore option for its run. It is not a guaranteed rollback after hardware/transport failure. These settings and estimator changes are saved by the agent. Use the [detailed autofocus tuning reference](../../IMAGING_AF_TUNING.md) for background; the operational recipe above is self-contained.


### Exact focus items and source defaults

These are source initialization limits/defaults, not an equipment-specific tuning recommendation. Live schema/readback remains authoritative for a connected version. Semantics and prerequisites are in the recipes above.

| Exact numeric item | Meaning / unit | Min…max | Step | Default |
| --- | --- | --- | --- | --- |
| `INITIAL` | Initial / U-Curve step (obsolete) | 1…65535 | 1 | 20 |
| `FINAL` | Final step (obsolete) | 1…65535 | 1 | 5 |
| `ITERATIVE_INITIAL` | Iterative initial step | 1…65535 | 1 | 20 |
| `ITERATIVE_FINAL` | Iterative final step | 1…65535 | 1 | 5 |
| `U_CURVE_SAMPLES` | U-Curve fitting samples | 6…24 | 1 | 10 |
| `U_CURVE_STEP` | U-Curve sample step | 1…65535 | 1 | 20 |
| `BAHTINOV_SIGMA` | Bahtinov sigma | 0…3 | 0 | 0.15 |
| `BRACKETING_STEP` | Bracketing step | -0xFFFF…65535 | 1 | 1 |
| `BACKLASH` | Backlash | 0…65535 | 1 | 0 |
| `BACKLASH_OVERSHOOT_FACTOR` | Backlash overshoot factor (1 disabled) | 1…3 | 0.5 | 1 |
| `STACK` | Stacking | 1…5 | 1 | 3 |
| `REPEAT` | Repeat count | 0…10 | 1 | 0 |
| `DELAY` | Initial repeat delay (s) | 0…3600 | 1 | 0 |

## Star selection

**IMG-12/13.** `AGENT_IMAGER_STARS.REFRESH=true` captures a new frame and rebuilds a dynamic switch list (up to 50 detections). Wait for this property's BUSY→OK/ALERT transition, then read its actual items. Numeric-string items such as `"0"` select a detected star; writing one true copies that star's coordinates into primary X/Y and clears the momentary selection. Do not treat an old list index as a persistent star identity.

`AGENT_IMAGER_SELECTION` is a numeric vector:

- `RADIUS`: measurement/search radius in image pixels.
- `SUBFRAME`: automatic-subframe factor relative to radius; 0 disables automatic subframing. Geometry is grid-aligned/clipped by camera support, not an arbitrary exact user rectangle. Automatic focus subframing applies with one selected star, not multistar focusing.
- `COUNT`: maximum stars requested; changing it rebuilds selection/statistic vectors and resets coordinate slots. Discover resulting `X/Y`, `X_2/Y_2`, … items rather than using a stale count.
- `X`, `Y`, `X_2`, `Y_2`, …: selected positions in current image pixels. Zero means no usable selected coordinate in the automatic-selection checks.
- `INCLUDE_LEFT`, `INCLUDE_TOP`, `INCLUDE_WIDTH`, `INCLUDE_HEIGHT`, `EXCLUDE_LEFT`, `EXCLUDE_TOP`, `EXCLUDE_WIDTH`, `EXCLUDE_HEIGHT`: image-pixel rectangles constraining star detection. Dimensions/regions are normalized against the current frame; re-read the accepted values.

Start `AGENT_START_PROCESS.CLEAR_SELECTION=true` to clear coordinates/dynamic stars, without an exposure. Automatic autofocus selection can capture/find/select when no valid coordinates remain. Manual selection changes are refused during non-preview processes; camera or binning changes can clear it. Abort star refresh through `AGENT_ABORT_PROCESS`; wait for the star-list state, then select/retry. If no stars are detected, change exposure/ROI while idle and refresh; do not invent a star index.

## Filter and manual focus

**IMG-14.** After wheel selection, read `AGENT_WHEEL_FILTER`; its actual items are named `1`, `2`, … and labeled using available wheel names. Set one true to select that slot. The property becomes BUSY and follows wheel completion. With `AGENT_PROCESS_FEATURES.APPLY_FILTER_OFFSETS=true` and a selected focuser, it applies the difference between the selected wheel filter offsets before changing the slot. Temperature-compensation mode suppresses offset movement with a warning. A failed offset move restores the old filter selection and reports ALERT. Inspect focuser/wheel states before retrying.

**IMG-15.** `AGENT_FOCUSER_CONTROL.FOCUS_IN=true` or `.FOCUS_OUT=true` commands a large relative move (65535 steps) in the selected direction; this is a hold/jog control, not a one-step adjustment. Set both false to issue focuser abort and end this control. Monitor the agent control and proxied position/motion. Stop this jog explicitly when the requested coarse move is sufficient, before autofocus or another process. For a precise service move, use the exposed focuser proxy deliberately; do not mix a jog and autofocus.

## Dithering and transit

**IMG-16.** Select a running, calibrated related Guider Agent; configure its strategy/settling using the [Guider guide](guider.md#dither-and-manual-offsets). Set `AGENT_PROCESS_FEATURES.ENABLE_DITHERING=true`, batch `FRAMES_TO_SKIP_BEFORE_DITHER`, and optionally `DITHER_AFTER_LAST_FRAME=true`. Only eligible light-frame ordinary exposure batches request dithers. Statistics expose FRAMES_TO_DITHERING, PHASE and DITHERING. The agent waits for a dither start/settling result; **current behavior can report a missing guider or settle failure and continue the batch**. A finished batch is not proof that every dither succeeded. Inspect guider state/messages and image results. Do not trigger independent manual dithers during an imaging exposure.

**IMG-17.** Select the related Mount Agent and obtain valid transit data. Enable `AGENT_PROCESS_FEATURES.PAUSE_AFTER_TRANSIT` and set batch `PAUSE_AFTER_TRANSIT` in hours. Before an exposure that crosses the configured boundary, the agent sets pause item `PAUSE_AFTER_TRANSIT`, pauses once for that batch and clears star selection. Mount coordination can then perform its work and release the pause. This item is **agent-set**: manually requesting it is cleared rather than scheduling a future pause. Monitor Mount process, imager pause and camera state. If coordination cannot complete, abort the agent process and inspect Mount state; do not resume blindly across a flip.

## Pause and abort

**IMG-18/19.** During an active main process, write one of `AGENT_PAUSE_PROCESS.PAUSE=true` (pause and abort ongoing exposure/motion) or `.PAUSE_WAIT=true` (pause without immediate abort; allow the current operation to reach its wait point). The property becomes BUSY while paused. To resume a BUSY pause, write a pause request with its item false; the handler clears pause flags and returns the pause property to OK. The active start-process property remains BUSY across a normal pause. Requesting pause while idle produces ALERT.

A PAUSE-aborted ordinary exposure is retried after resume instead of counted as a completed exposure. A streaming operation has different pause behavior and may end; it is not a resumable exposure batch. For any workflow, observe both pause and actual operation states before assuming it is safe to move equipment.

Write `AGENT_ABORT_PROCESS.ABORT=true` to terminate. It releases a paused wait, internally aborts camera/external shutter/focuser as applicable, and in barrier-leader mode propagates abort to related Imagers. Wait until the active process/star/capture state is terminal and ABORT is cleared. Finite batch/autofocus failure/abort usually yields ALERT; indefinite batch abort and preview stop can yield OK as described above. Failure recovery means inspecting actual hardware states/position, correcting configuration while idle and explicitly starting again, not retrying the start request automatically after a network timeout.

## Synchronization

**IMG-20.** `AGENT_IMAGER_BREAKPOINT` is an any-of-many boolean vector: `PRE_BATCH`, `PRE_CAPTURE`, `POST_CAPTURE`, `PRE_DELAY`, `POST_DELAY`, `POST_BATCH`. Each pauses at that actual point in an ordinary batch. `AGENT_IMAGER_RESUME_CONDITION.TRIGGER=true` means explicit/manual release through the pause control.

For a multi-camera group, configure one leader with selected follower Imagers in `FILTER_RELATED_AGENT_LIST`, and `AGENT_IMAGER_RESUME_CONDITION.BARRIER=true`. Set all cameras' own batch/exposure/storage settings before starting the leader. The leader propagates its breakpoint selection to followers, forces followers to TRIGGER, starts their exposure batches and releases the group only when all barrier members are paused. `AGENT_IMAGER_BARRIER_STATE` exposes dynamic per-member lights. Stop from the leader to propagate abort. Configure one leader and avoid circular related-Imager relationships.

Use matching boundary breakpoints that all members actually reach; do not promise hardware-simultaneous shutter opening. The implementation also waits for barrier members before dithering so no follower should still expose during that leader-controlled move. Source inspection shows normal batch code invokes PRE/POST_DELAY breakpoints inside its delay section; do not assume followers skip those boundaries without observing the configured process. If a member is missing, errors or stops early, inspect every member's start/pause/barrier state and abort the group instead of releasing incomplete data as success.

## Stored images

**IMG-21.** Camera local directory/upload/naming must already be set through proxies. The imager tracks the local directory and builds a list of recognized nonempty image/video files; this is a directory on the **server host**, not the MCP client's filesystem. `AGENT_IMAGER_DOWNLOAD_FILES.REFRESH=true` rescans it. Dynamic file items are exact names with size labels. Choose a listed file switch true, or set `AGENT_IMAGER_DOWNLOAD_FILE.FILE` to that exact listed basename. Wait for both download-file and `AGENT_IMAGER_DOWNLOAD_IMAGE` states; read BLOB item `IMAGE` as a URL through MCP. Requesting an unlisted/unreadable/empty file produces ALERT. This reads a stored file; it does not capture a new image.

**IMG-22.** `AGENT_IMAGER_DELETE_FILE.FILE=<listed basename>` deletes that server-side file and refreshes the listing. This is destructive; use only the intended listed name and verify the resulting property/list state. There is no trash/undo operation in this handler. Do not offer arbitrary-path deletion or assume that downloading authorizes deleting.

`AGENT_IMAGER_DISK_USAGE.TOTAL/USED/FREE` are read-only filesystem-capacity diagnostics in the implementation's MB scale (bytes divided by 1024²). Refresh is automatic on the agent's timer (about 30 seconds); there is no separate disk-refresh command. Values concern the configured image folder's volume. With no configured folder it reports zeros/IDLE; a filesystem query failure reports zeros/ALERT. Usage above 85% is BUSY and above 95% is ALERT: BUSY here is a capacity warning, not an active disk operation. Storage readiness requires checking the actual directory/file outputs too.

## Monitoring

**IMG-23.** Read `AGENT_IMAGER_STATS`; its fields are results, not commands. The exact remaining-exposure item is `EXPOSURE` (seconds). Other fields include `DELAY` (seconds), `FRAME/FRAMES`, `BATCH_INDEX/BATCH/BATCHES`, and `PHASE` (enum). In the shared enum: 0 idle, 1 setting filter, 2 focusing, 3 capturing, 4 dithering, 5 waiting, 6 rotating, 7 slewing, 8 calibrating; not every process exercises every phase. Use the active command state together with phase instead of treating phase alone as completion.

Image/focus results include `DRIFT_X/DRIFT_Y` (image pixels), `FOCUS_OFFSET/FOCUS_POSITION` (focuser steps), `RMS_CONTRAST`, `BEST_FOCUS_DEVIATION` (%), `FRAMES_TO_DITHERING`, `BAHTINOV_ERROR`, `MAX_STARS_TO_USE`, `PEAK`, `FWHM` and `HFD`, `HFD_2`, … for the actual dynamic star count. HFD/FWHM are image-pixel measures, not arcseconds. `DITHERING` is the related-guider settling diagnostic. `AGENT_IMAGER_SPIKES` exposes read-only `RHO_1/THETA_1` through `RHO_3/THETA_3` for Bahtinov diagnostics; these do not start focus and are not mount angles. Exact theta interpretation beyond the image-processing diagnostic is not required to operate autofocus and remains a draft interpretation gap.

## Reset and instances

**IMG-24.** Idle `AGENT_START_PROCESS.RESET=true` resets focus failure/estimator, process features, batch/focus settings and star selection defaults and saves configuration. It is not a factory reset of the camera/focuser and does not delete stored images. Read the accepted defaults afterward. The agent saves relevant configuration on ordinary setting changes; inherited `CONFIG` controls and system-wide profiles are described in [server preparation](../server-and-devices.md), not new imaging processes.

`ADDITIONAL_INSTANCES.COUNT` creates/removes additional agent instances through the shared filter lifecycle. Do this while idle and rediscover exact names; each instance needs independent selectors/configuration. It does not create extra physical hardware. Stop processes before removal. This is the supported preparation for separate imaging trains, not a reason to operate the same camera concurrently.

## Draft verification and source map

The inventory was checked against property creation, `agent_change_property`, capture/preview/batch/stream/bracketing/autofocus, download and snoop handlers in [the implementation](../../../indigo_drivers/agent_imager/indigo_agent_imager.c). Related behavior was checked against [shared filter source](../../../indigo_libs/indigo_filter.c), [property-name definitions](../../../indigo_libs/indigo/indigo_names.h), and existing local [autofocus](../../IMAGING_AF_TUNING.md) / [multicamera](../../INDIGO_IMAGER_MULTICAMERA_SYNCHRONIZATION.md) references. The recipes above do not require those files or network access at runtime; links support maintainer review and optional further reading.

No hardware, simulator, client, process or physical motion was executed for this draft. Device-specific property capabilities, platform storage behavior, synchronized-start timing and hardware focus quality remain unverified. Current source includes legacy sequence fields without an exposed sequence property/handler; this guide deliberately routes sequence orchestration to Scripting rather than inventing an Imager sequence API. A source/state/message disagreement should be escalated as a defect or documentation gap, not normalized into a successful observing recipe.
