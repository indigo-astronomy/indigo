# Platesolver Agent and shared solver operations

Draft for review. These source-inspected instructions are included in compiled MCP help. Documentation retrieval does not install solver prerequisites or validate the operating recipes.

Module `indigo_agent_platesolver` creates `Platesolver Agent`. It detects stars and matches quads **inside the INDIGO process** using native `.isix` indexes. It does not invoke Astrometry.net's `solve-field`. The separate [Astrometry Agent](astrometry.md) uses the same agent-operation interface with an external backend and different indexes. These two modules reject simultaneous loading in one server; choose one backend. Do not confuse Platesolver Agent with an obsolete “INDIGO Solver Agent” module/device name.

The process recipes below are also the shared operating contract for Astrometry Agent. When using that backend, substitute the discovered Astrometry Agent instance for the solver device and apply its [backend prerequisites](astrometry.md#prepare-the-backend). Every link is to a local manual; external websites are not required to understand operation.

## Function inventory

| ID | User function | Recipe |
| --- | --- | --- |
| P01 | Load solver, prepare source and select related agents | [Prepare](#prepare) |
| P02 | Install/remove/select native quad indexes | [Indexes](#indexes) |
| P03 | Configure exposure and temporary solver filter | [Capture settings](#capture-settings) |
| P04 | Restrict/accelerate solving with hints | [Hints](#hints) |
| P05 | Capture and solve once without mount correction | [Capture and solve](#capture-and-solve) |
| P06 | Solve images arriving from related agents | [Incoming images](#incoming-images) |
| P07 | Solve and synchronize the mount | [Solve and sync](#solve-and-sync) |
| P08 | Solve, sync and return to the intended center | [Center](#center) |
| P09 | Slew to a specified target then solve/sync/center | [Precise GOTO](#precise-goto) |
| P10 | Measure polar-alignment error with three fields | [Polar measurement](#polar-measurement) |
| P11 | Recalculate error after mechanical adjustment | [Polar adjustment](#polar-adjustment) |
| P12 | Read WCS, FOV and polar state | [Results](#results) |
| P13 | Upload an existing image using native BLOB transport | [Image upload](#image-upload) |
| P14 | Obtain original/preview images and tune JPEG display | [Preview](#preview) |
| P15 | Abort solver, capture, mount movement or index work | [Abort and recovery](#abort-and-recovery) |
| P16 | Legacy sync/abort compatibility, defaults and instances | [Compatibility and defaults](#compatibility-and-defaults) |

## Prepare

Follow [server and devices](../server-and-devices.md) and [common instructions](../instructions.md). Discover `Server.DRIVERS`, request the offered `indigo_agent_platesolver` item if absent and wait for `Platesolver Agent` to appear. If Astrometry is already loaded, first stop its work and unload it safely before loading the native backend. Choose the full discovered instance name, including remote/instance suffixes.

Load Imager Agent (`indigo_agent_imager`) or Guider Agent (`indigo_agent_guider`) through `Server.DRIVERS`, then the required camera driver. Configure camera transport/model while disconnected, select its actual name in that imaging agent's `FILTER_CCD_LIST`, and wait for selection/connection completion and usable image properties. Configure optical/image geometry and exposure prerequisites there. For mount-changing operations load Mount Agent (`indigo_agent_mount`) and the actual mount driver, configure its port/model, then select it on Mount Agent and establish correct site/time/pointing. Use the [Mount Agent manual](mount.md), not direct equatorial writes as a normal solver workflow.

On the solver set the chosen source-agent and optional Mount Agent items in `FILTER_RELATED_AGENT_LIST` true. These are dynamic full device-name items. The source must be configured and idle (or an Imager state explicitly permitting auxiliary capture); the solver does not select a camera directly. It chooses the first related Imager Agent when present, otherwise the first related Guider Agent. Multiple selected imaging agents are not an automatic load-balancing arrangement. Choose one intended source to avoid ambiguity. Mount operations choose the first related Mount Agent. Read its forwarded `MOUNT_EPOCH.EPOCH`: the shared solver converts hints/GOTO/WCS coordinates into that reported frame before staging Mount Agent targets. The shared layer initially assumes 2000 until a related epoch is reported; that fallback is not proof of the selected driver's frame. Confirm the actual epoch and see [Mount Agent coordinate limitations](mount.md#coordinate-epochs) before using non-J2000 derived display, dome/rotator geometry or LX200 coordination. Relations are directional; confirm the solver's list explicitly, even when the other agent lists the solver. Mount-to-solver relation provides useful pointing hints; it is not a substitute for solver-to-Mount selection.

Install suitable indexes and select at least one in `AGENT_PLATESOLVER_USE_INDEX`. The agent's own CONNECTION is hidden. An absent agent, empty relation list, unconfigured source, busy capture, missing indexes or unsupported mount process must be repaired before starting, not bypassed by addressing the physical camera/mount directly.

For every started operation, watch `AGENT_START_PROCESS`, `AGENT_PLATESOLVER_WCS` and, for polar alignment, `AGENT_PLATESOLVER_PA_STATE`. `Busy` indicates work, `Alert` failure/abort, and `Ok` after the new request is the relevant completion evidence. Read the latest solution rather than treating an old cached WCS as a new result. MCP snapshots have no operation ID or event history; an unchanged `Ok` poll alone does not prove that a new solve completed. Correlate target/source/result changes where possible and report unresolved freshness. MCP dispatch success only submits the property change; it does not confirm operation completion. [Common instructions](../instructions.md) describe current observation and transport limits.

## Indexes

Native install/remove control is `X_AGENT_SOLVER_INDEX`. True means install/retain an index; false requests file deletion. Installation can download large files; non-Windows builds use the server's `curl` executable, while Windows uses native HTTP(S) transfer. It is an explicit setup action with disk/network side effects, not something to retry automatically during acquisition. Wait for the management property to leave `Busy` and confirm actual selected/installed items; an `Alert` requires its message and filesystem/network problem to be addressed. Use [abort and recovery](#abort-and-recovery) to interrupt rather than unloading a busy agent.

| Management/use-index item | Native file | Catalog / quad scale | Approximate field range when all indexes are available |
| --- | --- | --- | --- |
| `INDEX_00_2DEG_GAIA` | `index-00.2deg-gaia.isix` | Gaia DR3 / 0.2° | Up to 0.45° |
| `INDEX_00_5DEG_GAIA` | `index-00.5deg-gaia.isix` | Gaia DR3 / 0.5° | 0.45–1° |
| `INDEX_01DEG_TYC2` | `index-01deg-tyc2.isix` | Tycho-2 / 1° | 1–2° |
| `INDEX_02DEG_TYC2` | `index-02deg-tyc2.isix` | Tycho-2 / 2° | 2–4.5° |
| `INDEX_05DEG_TYC2` | `index-05deg-tyc2.isix` | Tycho-2 / 5° | 4.5–10° |
| `INDEX_10DEG_TYC2` | `index-10deg-tyc2.isix` | Tycho-2 / 10° | 10–20° |
| `INDEX_20DEG_TYC2` | `index-20deg-tyc2.isix` | Tycho-2 / 20° | Over 20° |

Quad scale is the index's star-pattern scale, not an exact camera-field requirement or promise of solving every listed field. Choose a scale near 0.7 times the geometric mean of field width/height; approximately 0.4–1 times field size is useful. The documented practical image FOV is about 0.1–15°, limited by indexes/star density. Very small Gaia fields may have insufficient catalog stars.

Files reside in `$INDIGO_CACHE_BASE/solver` or `~/.indigo/solver` when the variable is unset. The current format is version 2; version 1 files are refused. Installed entries are offered in `AGENT_PLATESOLVER_USE_INDEX`; installation alone does not guarantee a selected use item. Explicitly select suitable discovered items true, preserving any intentionally selected alternatives. The solver orders selected indexes using scale/field estimates and keeps the last index loaded between solves. These native files are not interchangeable with Astrometry FITS indexes.

Offline operation requires installing or provisioning compatible files **before** disconnecting the server from the network. There is no need to download manuals or indexes per solve once the required local data exists. Index building is an administrator/setup operation documented in the backend's existing README; do not fabricate files or treat an installation switch as proof that a failed download succeeded.

## Capture settings

Set `AGENT_PLATESOLVER_EXPOSURE.EXPOSURE` in seconds (published 0–60, default 1) for exposures initiated by solver start processes. Prefer this property over the obsolete polar-settings EXPOSURE alias.

`AGENT_PLATESOLVER_FILTER` offers `CURRENT` plus dynamic items mirrored from the related Imager's `AGENT_WHEEL_FILTER`. CURRENT leaves the current filter unchanged. Choosing another offered item lets the solver select that filter before its capture and restore the previous filter while solving. Configure wheel selection and filter names in Imager first. Do not invent numeric filter IDs; discover this list after establishing the relation. Watch source capture/wheel states and solver process state. A filter-selection failure fails the process. If aborted or interrupted, verify the actual filter and source state before resuming an imaging sequence; restore intentionally if needed.

The solver requests an Imager auxiliary capture via `AGENT_IMAGER_CAPTURE`, or Guider settings plus its one-frame preview process when no Imager is selected. The normal workflow therefore remains source-agent capture. It does not require a direct `CCD_EXPOSURE` write by an MCP client. Solving automatic incoming images does not itself impose these exposure settings on every unrelated source exposure.

## Hints

Write numeric items of `AGENT_PLATESOLVER_HINTS`, then confirm targets/readback before starting. Changing hints does not start a solve.

| Item | Units and meaning | Operational choice |
| --- | --- | --- |
| `RADIUS` | Degrees; 0 disables positional restriction | Positive values restrict native catalog selection around RA/DEC, expanded by field extent. It is not a strict fitted-center rejection radius. |
| `RA`, `DEC` | Hours and degrees in hints EPOCH | Use a known approximate pointing; a related Mount Agent can update these. Bad narrow hints can exclude the correct field. |
| `EPOCH` | 0=JNow, 2000=J2000 result/hint convention | Use 0 or 2000 deliberately and match target coordinates; other requested epochs are adjusted to 2000. Never silently exchange JNow/J2000. |
| `SCALE` | Degrees per original pixel | Positive explicit scale; negative requests camera-derived scale; 0 removes explicit scale. Native backend also uses available source scale to order indexes. Divide arcseconds/pixel by 3600 before writing. |
| `PARITY` | −1, 0, +1 | 0 unknown, ±1 requires that parity. A wrong sign rejects a valid solution. |
| `DOWNSAMPLE` | Integer image binning factor, 1–16 | Native Bayer input is binned at least 2×2; odd Bayer factors are raised to even. Larger bins can lose faint/small stars. |
| `DEPTH` | Star count progression | Native 0 uses built-in 40,100,all progression; positive value tries that brightest-star subset before all detected stars. |
| `CPULIMIT` | Seconds, 0 unlimited; default 180 | Native solve deadline includes index loading; this does not bound exposure or every mount operation. |

RA is hours here even though catalog calculations often use degrees internally. For example 2 arcseconds/pixel is SCALE=0.0005555556 degree/pixel. Confirm current camera binning/ROI when deriving field size; guessed units can make an otherwise solvable field fail. If no solution, broaden radius/remove an incorrect parity/scale restriction, choose an appropriate installed index and verify star detection/exposure, then start a fresh solve. Do not repeatedly sync from failed or stale WCS.

## Capture and solve

Prerequisites: prepared related source, selected indexes, idle solver. Set exposure/hints/filter, choose `AGENT_PLATESOLVER_SYNC.DISABLED=true` for a neutral automatic-image policy, then start `AGENT_START_PROCESS.SOLVE=true`. The explicit SOLVE process temporarily selects solve-only behavior, captures through the related source agent, solves its received frame and publishes WCS/FOV. No mount sync or center is requested by this mode.

Watch process and WCS states; confirm a fresh successful solution with plausible RA/DEC, nonzero field dimensions/scale and appropriate parity. [Results](#results) explains units. This is the initial verification step before allowing solve-driven mount changes. Abort as described below; after failure check source availability, image quality, hints/indexes and source process state before retry.

Synthetic MCP property-call illustration, after actual names have been discovered and source/index preparation completed:

```json
{"device":"Platesolver Agent","property":"AGENT_PLATESOLVER_EXPOSURE","items":{"EXPOSURE":2}}
```

```json
{"device":"Platesolver Agent","property":"AGENT_START_PROCESS","items":{"SOLVE":true}}
```

These are arguments to `set_property`, not standalone JSON-RPC envelopes. Read `AGENT_START_PROCESS` and `AGENT_PLATESOLVER_WCS` with `get_property` afterward. They do not load a missing module or camera automatically.

## Incoming images

`AGENT_PLATESOLVER_SOLVE_IMAGES.ENABLED` allows images from selected related Imager/Guider agents to initiate solves; `.DISABLED` stops this automatic incoming-image behavior. This is independent of choosing an explicit start-process command. Select a source deliberately and coordinate with its acquisition workflow: automatic solve on each arriving frame can consume CPU and, depending on legacy sync mode, move/sync a mount. Choose `AGENT_PLATESOLVER_SYNC.DISABLED` when only measurements are intended.

Observe new WCS updates and source acquisition state rather than expecting every incoming image to correspond to an independently started AGENT_START_PROCESS. To stop solving use DISABLED for future incoming images and ABORT for outstanding work. Stop or reconfigure the originating Imager/Guider through its own agent workflow if it should stop acquiring too.

## Solve and sync

In addition to solving prerequisites, select a configured Mount Agent with sync capability and trustworthy site/alignment context. Start `AGENT_START_PROCESS.SYNC=true` on the solver. It captures and solves, converts the solved position to the mount epoch, stages it on Mount Agent and starts its SYNC process. Observe solver WCS/process **and** Mount Agent process/state. A successful image solve alone does not mean the sync step succeeded.

This changes the mount pointing model. Verify an initial solve-only result before using it and avoid changes during an unrelated observing process. A solver abort also attempts to abort its related capture and Mount Agent move. On failure inspect both agents; a completed sync is not automatically undone by a subsequent error.

## Center

Start `AGENT_START_PROCESS.CENTER=true` after selecting source and Mount Agent. Before capture, hints RA/DEC must identify the intended pointing in the hints epoch. The workflow saves that center, solves the actual field, syncs the mount to the solved position and slews back to the intended center through Mount Agent. Mount hints often provide this intended current pointing. It then waits the configured `AGENT_PLATESOLVER_MOUNT_SETTLE_TIME.SETTLE_TIME` seconds (0–60, default 3) after the commanded move.

This is a solve/sync/recenter sequence, not a documented arbitrary-accuracy iterative loop. Observe SYNCING/CENTERING WCS stages and final process state, then take another solve-only exposure if independent centering verification is needed. Abort via solver; check mount motion and pointing afterward. Incorrect target/hint epochs or a failed sync are causes to resolve before a fresh attempt.

## Precise GOTO

Write `AGENT_PLATESOLVER_GOTO_SETTINGS.RA` (hours) and `.DEC` (degrees) in `AGENT_PLATESOLVER_HINTS.EPOCH`; configure exposure/hints/settling, then start `AGENT_START_PROCESS.PRECISE_GOTO=true`. It first slews through related Mount Agent to the requested coordinates, then captures, solves, syncs and centers. Unlike CENTER this has an explicit new destination. It can unpark through Mount Agent and use its dome/rotator coordination.

Observe both agent processes, WCS STATE=5 during initial GOTO and subsequent solve/sync/center stages. Confirm final position with a fresh solve if needed. This command has motion side effects before an image is solved. Abort through solver, verify Mount Agent and any controlled dome/rotator have stopped, and repair alerts before staging another target. Do not use direct mount-coordinate writes as a substitute when the selected Mount Agent is absent or reports unsupported operations.

## Polar measurement

Select related source and Mount Agent, configure correct site/time, and ensure safe mount movement across three fields. Point through Mount Agent to a suitable field above roughly 40° altitude, preferably just past the meridian to avoid an unintended flip during the sequence. This recommendation is not an automatic collision/meridian check.

Set `AGENT_PLATESOLVER_PA_SETTINGS.HA_MOVE` in **degrees** (published −50 to +50, typically 15–20), and `COMPENSATE_REFRACTION` to numeric 0 or 1. Set exposure through `AGENT_PLATESOLVER_EXPOSURE.EXPOSURE`, along with suitable indexes/hints/filter. The obsolete PA_SETTINGS.EXPOSURE remains an alias; changing the PA settings property can propagate its EXPOSURE target into the modern exposure property, so set the modern exposure last when using partial settings updates.

Start `AGENT_START_PROCESS.CALCULATE_PA_ERROR=true`. The agent takes and solves three exposures separated by commanded hour-angle moves (second and third reference positions), using Mount Agent and settling between movements. Wait for solver process completion and `AGENT_PLATESOLVER_PA_STATE` to provide the measured errors and adjustment reference. It does not motorize the mechanical polar adjustment knobs. `Alert` or `ACCURACY_WARNING` requires attention; degenerate reference geometry can fail the measurement.

Abort via solver to stop capture/move/solve and discard the polar reference. Check actual mount state before recovering; repeat the full measurement after a failed/aborted reference sequence rather than treating partial references as valid.

## Polar adjustment

After a successful measurement, read `ALT_POLAR_ERROR`, `AZ_POLAR_ERROR`, `POLAR_ERROR` (degrees), `ALT_CORRECTION_UP` (1 up, 0 down), `AZ_CORRECTION_CW` (1 clockwise, 0 counterclockwise) and messages. Use these directions with the mount's actual mechanical altitude/azimuth adjustment procedure. One degree equals 60 arcminutes; do not interpret raw degrees as arcminutes. Source image/preview and current/target coordinates can help follow progress.

After a manual adjustment start `AGENT_START_PROCESS.RECALCULATE_PA_ERROR=true`. It captures another frame and recalculates residual error against the retained reference rather than taking three new reference positions. Repeat manual adjustment/recalculation until acceptable. Prerequisite is a valid retained polar reference; after abort, reset, unrelated movement or loss of that context, run CALCULATE_PA_ERROR again. Observe PA state/error and solver completion after each request, not an old error value. The recalculation is not a normal target slew and must not be replaced by direct mount equatorial commands.

## Results

`AGENT_PLATESOLVER_WCS` contains numeric `STATE`, `RA`, `DEC`, `EPOCH`, `ANGLE`, `WIDTH`, `HEIGHT`, `SCALE`, `PARITY`, `INDEX`. RA uses hours; DEC/ANGLE/WIDTH/HEIGHT use degrees; SCALE degrees/pixel. EPOCH 0 is JNow and 2000 J2000. ANGLE is east of north in that epoch; parity +1 is a mirror of the sky (east right with north up), −1 is the sky as seen. Native INDEX reports the used index scale; Astrometry reports its backend index identifier. Do not use that field as a common cross-backend file path.

WCS numeric STATE: 0 idle, 1 waiting for image, 2 solving, 3 syncing, 4 centering, 5 initial GOTO. This item is a stage; the property's `Ok`/`Busy`/`Alert` state is separate and must also be checked. A stage returning to idle does not prove success on its own. FOV angle/width/height are propagated to the related Mount Agent's `AGENT_MOUNT_FOV` after a successful solution.

`AGENT_PLATESOLVER_PA_STATE` numeric STATE: 0 idle, 1 start, 2/3/4 references one/two/three, 5 recalculate, 6 adjustment in progress. Other items: `DEC_DRIFT_2`, `DEC_DRIFT_3` in degrees; `TARGET_RA`, `CURRENT_RA` in hours; `TARGET_DEC`, `CURRENT_DEC` in degrees; the polar errors/directions above and `ACCURACY_WARNING` 0/1. Adjustment-in-progress can accompany a successfully measured error awaiting manual correction; it does not mean the mount is continuously slewing.

## Image upload

The shared solver exposes WO BLOB `AGENT_PLATESOLVER_IMAGE.IMAGE` for an existing image supplied by a native INDIGO client. This starts solving that uploaded image, clears camera-derived pixel scale for the upload, and can copy its solved position into GOTO settings. It is a separate input route from capture through related agents. Check the legacy sync mode before upload because post-solve mode can have mount side effects.

Current MCP `set_property` supports text/number/switch writes and explicitly rejects BLOB writes. Do not pass a file path, URL string or base64 text as if it were a supported MCP upload. Use a native BLOB-capable client or the agent capture workflow; documentation retrieval does not add upload support. Native URL-backed input can require downloading the image before solving; source connectivity/permissions apply. Observe WCS and abort through the agent just as for a captured image.

## Preview

`AGENT_PLATESOLVER_IMAGE_OUTPUT.IMAGE` mirrors the image being solved. `CCD_PREVIEW` items `DISABLED`, `ENABLED`, `ENABLED_WITH_HISTOGRAM` control generated preview. Read `CCD_PREVIEW_IMAGE.IMAGE` when available. MCP returns BLOB URL metadata, not an embedded pixel payload; retrieving an image is a distinct client operation. Preview does not start an exposure or replace a WCS result.

`CCD_JPEG_SETTINGS` configures `QUALITY` (10–100), `TARGET_BACKGROUND` (0–1), `CLIPPING_POINT` (−3 to 0) and `REFERENCE_CHANNEL` (0 auto white balance, 1 R, 2 G, 3 B). `CCD_JPEG_STRETCH_PRESETS` choices `SLIGHT`, `MODERATE`, `NORMAL`, `HARD` set background/clipping presets. These affect display conversion, not solving coordinates or optical focus. Read back settings and the next generated preview; no separate abort is needed for a setting change. Restore previous/preset values if display becomes unhelpful.

## Abort and recovery

Send `AGENT_ABORT_PROCESS.ABORT=true` on the solver. It requests backend abort, cancels outstanding image work, attempts related source exposure and Mount Agent motion abort, and resets polar references. With native index work it requests download/matching cancellation; check the management property to see the resulting installed state. An abort property returning `Ok` is acknowledgment of the request, not proof all related hardware has stopped. Inspect WCS/process, source capture and Mount Agent/device motion states until quiescent.

For a failed solve, first identify whether capture, image transfer, star detection, indexes, hint restriction, CPU limit, sync or movement failed. Correct that cause, confirm idle source/mount and start a new deliberate process. After failure do not promote last successful cached WCS to the failed frame, repeat motion blindly, or unload a busy agent. If an index download was interrupted, inspect installed/use-index lists and retry only the required setup action. Camera exposure/quality, field scale and catalog density are independent causes; a longer CPU limit cannot fix a blank image or missing index.

## Compatibility and defaults

`AGENT_PLATESOLVER_SYNC` is an obsolete but active automatic-image policy with `DISABLED`, `SYNC`, `CENTER`, `CALCULATE_PA_ERROR`, `RECALCULATE_PA_ERROR`. New explicit start processes temporarily set the required mode and restore the saved one at completion. Choose the explicit process for a single operation; keep automatic-image policy deliberately configured to avoid surprising mount changes. `AGENT_PLATESOLVER_ABORT.ABORT` is the older alias for modern abort behavior.

`AGENT_START_PROCESS.RESET=true` restores solve-images, hints, legacy sync mode, exposure, PA settings and mount settling defaults and selects filter CURRENT, then saves configuration. It is not a delete-all-indexes command or a mount alignment reset. Recheck automatic-image policy, exposure and hints before resuming. Shared additional-instance/related-agent behavior is described in [server and devices](../server-and-devices.md); use discovered full names. Backend switching requires idle/drained work and explicit module selection; it is not an instance name change.

## Source references and draft limits

Checked against [native agent](../../../indigo_drivers/agent_platesolver/indigo_agent_platesolver.c), [native solver](../../../indigo_drivers/agent_platesolver/indigo_solver.c), [shared solver operations](../../../indigo_libs/indigo_platesolver.c), [shared definitions](../../../indigo_libs/indigo/indigo_platesolver.h), [filter](../../../indigo_libs/indigo_filter.c), [wire names](../../../indigo_libs/indigo/indigo_names.h), and [properties reference](../../PROPERTIES.md). These links provide review provenance; operating semantics are included above for offline use.

Draft limits: image/camera-specific exposure suitability, mechanical correction direction viewed from a particular mount, physical clearances and driver-specific reference/calibration procedures require the selected equipment's documented setup. No generic precision/convergence guarantee or runtime validation is claimed. Backend index installation is a separately authorized setup operation and may need network access; reading this manual does not authorize it.
