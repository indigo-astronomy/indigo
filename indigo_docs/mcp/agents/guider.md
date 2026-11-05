# Guider Agent operating manual — draft

This offline-ready draft is for owner review. It describes functions present in inspected source; it is not a hardware-validated tuning preset. This manual is included in compiled MCP help. Read actual property/item definitions and use exact device names, including instance/remote suffixes. A completed tool request is not proof of calibration, guiding or physical correction.

## Purpose and agent-first setup

Module `indigo_agent_guider` exposes `Guider Agent`. It acquires guide frames, measures displacement, calibrates the relationship between image motion and guide pulses, applies RA/Dec corrections, manages dither reference changes and reports guiding diagnostics. Detection and correction are separate choices: detection measures drift; correction decides how to react to it.

Follow [server/device preparation](../server-and-devices.md): if the agent is absent, discover the Server device and set its existing `DRIVERS.indigo_agent_guider` item true; wait for loading to finish and the agent to appear. Load the camera and mount/guide-output drivers through Server.DRIVERS as well. Configure serial port, communication/model and device-specific prerequisites directly on disconnected hardware devices only as setup. Then select those devices on the agent, wait for preparation, configure the guider and start an agent process. Do not replace an agent workflow with direct physical-camera exposures or manually timed guide pulses.

| Property on Guider Agent | Operation and prerequisite |
| --- | --- |
| `FILTER_CCD_LIST` | Choose the guide **camera** by its actual dynamic switch name. Wait for preparation/connection to leave BUSY and for agent camera proxies to appear. |
| `FILTER_GUIDER_LIST` | Choose the device supplying RA/Dec guide output. It may be the mount's guide interface or a camera guide port; it must actually drive the intended mount. This is distinct from selecting the camera. |
| `FILTER_RELATED_AGENT_LIST` | Related Imager/Mount agents enable coordination. Mount metadata/abort coordination and Imager dither requests are relationships, not alternate guide-output selections. |

A camera is sufficient for preview/star finding; calibration/guiding require both selections. Filter selection prepares/connects devices, and refuses changes while a process runs. The `NONE` item deselects a device. Read accepted selector state/proxied connection rather than treating module loading as connection. Configure guide camera gain, binning, image geometry and optics/pixel scale through supported agent-side camera/optics proxies while idle. Changing camera/binning can clear regions/star coordinates; recalibrate when optical geometry, camera angle or guide-speed settings invalidate the calibration.

Use the related [Mount Agent](mount.md) as the normal source of telescope orientation. Agent-side `AGENT_GUIDER_MOUNT_COORDINATES` is metadata, not a slew command. Physical mount tracking and safe pointing must be ready before calibration.

Source: [Guider implementation](../../../indigo_drivers/agent_guider/indigo_agent_guider.c), [name/entry point](../../../indigo_drivers/agent_guider/indigo_agent_guider.h), [shared filter selection/proxy behavior](../../../indigo_libs/indigo_filter.c), [canonical properties](../../PROPERTIES.md).

## Complete user-function inventory

| ID | Function group | Recipe |
| --- | --- | --- |
| GDR-01 | Load/select camera, guide output, related agents; configure proxies | [Setup](#purpose-and-agent-first-setup) |
| GDR-02 | Single-frame preview | [Preview](#preview) |
| GDR-03 | Continuous preview | [Preview](#preview) |
| GDR-04 | Refresh stars and select a dynamic listed star | [Selection and regions](#selection-and-regions) |
| GDR-05 | Configure multistar, search radius, regions, edge clipping, subframe | [Selection and regions](#selection-and-regions) |
| GDR-06 | Clear selection | [Selection and regions](#selection-and-regions) |
| GDR-07 | Choose Selection/Weighted Selection/DONUTS/Centroid detection | [Drift detection](#drift-detection) |
| GDR-08 | Calibrate guide response | [Calibration](#calibration) |
| GDR-09 | Calibrate then start guiding | [Calibration](#calibration) |
| GDR-10 | Guide using valid calibration | [Guiding](#guiding) |
| GDR-11 | Select RA/Dec correction controllers and tune response | [Correction controllers](#correction-controllers) |
| GDR-12 | Set Dec direction/backlash and flip policy | [Declination and pointing](#declination-and-pointing) |
| GDR-13 | Supply/update pointing and calibration metadata | [Declination and pointing](#declination-and-pointing) |
| GDR-14 | Choose dither strategy, trigger/wait/reset sequence | [Dither and manual offsets](#dither-and-manual-offsets) |
| GDR-15 | Set manual guide-reference X/Y offsets | [Dither and manual offsets](#dither-and-manual-offsets) |
| GDR-16 | Tune predictive PEC / multi-kernel GP and inspect learning | [Predictive models](#predictive-models) |
| GDR-17 | Reset each learned predictive model | [Predictive models](#predictive-models) |
| GDR-18 | Set lost-star/failure recovery policy | [Failure policy](#failure-policy) |
| GDR-19 | Enable/configure/read guiding/calibration logs | [Logging](#logging) |
| GDR-20 | Read drift, correction, RMSE, response and model statistics | [Statistics](#statistics) |
| GDR-21 | Abort and recover a running operation | [Abort and recovery](#abort-and-recovery) |
| GDR-22 | Reset defaults, save/restore settings, manage instances | [Reset and instances](#reset-and-instances) |

This inventory covers all Guider-specific writable branches and shared preparation functions. Scalar settings configure a process; they do not start a process. Read-only statistics are observations, not a separate command. There is no Guider `AGENT_PAUSE_PROCESS` command in this implementation: “waiting for stars” is an internal recovery state, not an Imager-style user pause.

## Starting and observing processes

`AGENT_START_PROCESS` is at-most-one: send exactly one operation item true. Preview/star refresh and other process starts are guarded against overlap. Wait for BUSY and terminal process state/cleared item, and inspect messages/outputs. For indefinite guiding/preview, continued BUSY can be normal; it is not a hung-operation diagnosis by itself. Configure mode choices before starting, because detection/RA/Dec correction mode changes are refused while a process runs.

The calibration/guiding phase enum in `AGENT_GUIDER_STATS.PHASE` is: -2 ignore, -1 previewing, 0 guiding, 1 initializing, 2 clearing Dec backlash, 3 clearing RA backlash, 4 moving north, 5 moving south, 6 moving west, 7 moving east, 8 failed, 9 done. It is **not** the property OK/BUSY/ALERT enum. Poll command state, phase and fresh frame/statistic progress together.

## Preview

**GDR-02/03.** Set `AGENT_GUIDER_SETTINGS.EXPOSURE` and optional `.DELAY` in seconds; a selected guide camera is required. Start `AGENT_START_PROCESS.PREVIEW_1=true` for one frame or `.PREVIEW=true` for an ongoing preview. The agent temporarily chooses RAW image format and client upload and restores those saved selections when it exits. Preview does not require a guide-output device and does not perform calibration or guide corrections.

Read the agent camera image result and `AGENT_GUIDER_STATS.FRAME/PHASE`. Single preview sets phase DONE and command OK after a captured frame; capture failure sets FAILED/ALERT. Its abort is treated as a normal stop. Continuous preview exits OK/DONE on agent abort, FAILED/ALERT on capture failure. Both reset dither offsets and statistics at start. Stop through the Guider abort command; do not compete with it by starting camera exposures directly. If frames fail, check selected camera/proxy state, RAW capability and exposure/geometry, then retry while idle.

## Selection and regions

**GDR-04/05/06.** While idle, set `AGENT_GUIDER_STARS.REFRESH=true`. It captures a new frame, detects stars and rebuilds a dynamic list of up to 50 star switches; wait for BUSY→OK/ALERT. Item names are numeric strings (`"0"`, `"1"`, …), labels show detected coordinates. Select one actual listed switch true to copy it to primary selection X/Y; that momentary switch is cleared. Refresh after geometry/target changes; list indices are not persistent celestial identities.

`AGENT_GUIDER_SELECTION` contains these numbers:

| Item(s) | Effect and units |
| --- | --- |
| `RADIUS` | Star measurement/search radius in current image pixels. It also limits safe corrections. |
| `COUNT` | Maximum selected stars, integer. Changing it rebuilds the X/Y slots and resets coordinates. |
| `X/Y`, `X_2/Y_2`, … | Coordinates in current guide image pixels. Discover available slots after setting COUNT. Zero/unset coordinates let automatic selection find suitable stars. |
| `SUBFRAME` | Automatic-subframe factor relative to RADIUS; 0 disables. Camera grid/alignment/capability controls actual accepted ROI. |
| `EDGE_CLIPPING` | Ignored image border width in pixels for relevant image-digest processing. |
| `INCLUDE_LEFT`, `INCLUDE_TOP`, `INCLUDE_WIDTH`, `INCLUDE_HEIGHT` | Pixel rectangle within which stars may be found; validated/normalized to current geometry. |
| `EXCLUDE_LEFT`, `EXCLUDE_TOP`, `EXCLUDE_WIDTH`, `EXCLUDE_HEIGHT` | Pixel rectangle excluded from star detection. |

Start `AGENT_START_PROCESS.CLEAR_SELECTION=true` to clear selected coordinates and dynamic star list. Calibration/guiding can capture and select stars automatically when selection is empty; preview/refresh help verify the field first. Stop star refresh through `AGENT_ABORT_PROCESS`, then inspect its property and rediscover the list.

During a process the handler refuses changes to COUNT or EDGE_CLIPPING. Other selection fields have different source behavior, so configure all geometry while idle rather than using live edits as a reacquisition protocol. Camera/bin changes can clear regions and selection. For non-DONUTS guiding, the agent can choose an automatic subframe and attempts to restore the original frame/selection at normal guide exit. Confirm accepted geometry after interruption; do not promise physical-camera ROI restoration solely from a completed command.

## Drift detection

**GDR-07.** Choose one item on `AGENT_GUIDER_DETECTION_MODE` while idle:

| Item | Measurement and suitable prerequisites |
| --- | --- |
| `SELECTION` | Local centroids of selected stars; multi-star motion is combined with rejection of outliers. Suitable starting point for ordinary stars. RADIUS/COUNT/regions matter. |
| `WEIGHTED_SELECTION` | Same selected-star measurements with SNR weighting, useful when multiple stars have unequal signal quality. With one star, weighting offers no extra star diversity. |
| `DONUTS` | Correlation/digest of the star-field image pattern; useful for defocused stars without relying on one compact star centroid. EDGE_CLIPPING matters. Enable `AGENT_PROCESS_FEATURES.USE_INCLUDE_FOR_DONUTS=true` to use the include rectangle; otherwise it uses the frame. It is not a fix for a changing/empty field. |
| `CENTROID` | Whole-image light centroid, intended for suitable extended bright targets rather than selecting an ordinary guide star. |

The mode property returning OK means selection accepted, not drift measured. Verify a usable image/selection and then calibrate/guide. Changing the mode requires stopping the active process first. Detection failure is handled according to the failure-policy recipe. More background is available in the local [detection reference](../../INDIGO_GUIDER_DETECTION_MODES.md); the essential mode distinctions are included here for offline use.

## Agent-level payload example

After selecting a prepared camera and guide output on the discovered device named exactly `Guider Agent`, this illustrative `set_property` argument configures one-second guide exposures and automatic calibration-step selection. Use actual readback/equipment needs rather than assuming this is a validated preset:

```json
{"device":"Guider Agent","property":"AGENT_GUIDER_SETTINGS","items":{"EXPOSURE":1,"DELAY":0,"STEP0":0}}
```

Read the accepted settings and verify pointing/field readiness, then start the combined agent operation:

```json
{"device":"Guider Agent","property":"AGENT_START_PROCESS","items":{"CALIBRATION_AND_GUIDING":true}}
```

Follow calibration phases into GUIDING; it is not complete when the first write returns. To stop:

```json
{"device":"Guider Agent","property":"AGENT_ABORT_PROCESS","items":{"ABORT":true}}
```

Wait for terminal process/abort state and actual device cleanup. These payloads invoke existing agent operations; they do not request direct camera exposures or manual pulse durations.

## Calibration

**GDR-08/09.** Select both camera and correct guide output, prepare a usable target/field and valid current pointing metadata. Calibration sends physical guide corrections, so allow its full movement and use the agent abort if the wrong output or unsafe motion is observed. Start `AGENT_START_PROCESS.CALIBRATION=true` to measure only, or `.CALIBRATION_AND_GUIDING=true` to measure and proceed into continuous guiding on success. Configure these numeric items on `AGENT_GUIDER_SETTINGS` first:

| Item | Meaning |
| --- | --- |
| `EXPOSURE`, `DELAY` | Guide frame exposure and inter-frame delay, seconds. |
| `STEP0` | Calibration pulse seconds; 0 selects automatic initial estimation, followed by adaptive measurement/adjustment. This source uses pulse limits 0.05–2 seconds; a zero auto request is not a zero-length calibration. |
| `MAX_BL_STEPS`, `MIN_BL_DRIFT` | Backlash-clearing iteration budget and required image drift in pixels. |
| `MAX_CALIBRATION_STEPS`, `MIN_CALIBRATION_DRIFT` | Calibration-leg iteration budget and required image drift in pixels. |

The calibration state machine initializes measurements, clears backlash and measures north/south/west/east response as appropriate to the Dec mode. It adapts a too-short pulse based on observed motion rather than merely increasing iteration counts. A wrong guide port or no measured movement can still fail. It overwrites calibration outputs `ANGLE` (degrees), `BACKLASH` (Dec pixels), `SPEED_RA` (pixels/second at equator), `SPEED_DEC` (pixels/second) and calibration `SIDE_OF_PIER` on `AGENT_GUIDER_SETTINGS`. These values are a geometry/response model, not independent move commands.

Watch `AGENT_GUIDER_STATS.PHASE/FRAME` and messages through the clearing/motion phases; wait for the process terminal state. Calibration success reports DONE/OK and saves the resulting model. Combined calibration-and-guiding then keeps/returns the main process to BUSY and sets GUIDING; do not interpret that BUSY as failed calibration. Calibration failure/abort is not a valid new calibration; read terminal state/messages and do not start GUIDING using accidental zero/old values.

Near the celestial pole the RA estimate is less reliable (the code warns within 5 degrees). If calibration fails, stop, verify guide-output wiring, star visibility and exposure/ROI, actual guide rate and pointing, then recalibrate. Failure-policy switches may instead reset selection and retry internally; observe their accepted state before assuming one attempt will terminate.

## Guiding

**GDR-10.** With valid calibration and camera/output selections, start `AGENT_START_PROCESS.GUIDING=true`, or use the combined calibration command above. This reuses stored calibration; it does not validate that optical geometry is unchanged. The implementation rejects zero `SPEED_RA` or current declination more than 89 degrees from the equator in absolute value. It clears session statistics and offsets, takes a reference, then captures/measures/corrects in a loop.

Observe GUIDING phase, increasing FRAME, drift/correction/RMSE and camera/output states. RA/Dec correction durations are computed internally from the chosen controllers, calibration, current declination and configured pulse constraints. Camera exposure plus readout/transfer/correction/delay determine cadence; EXPOSURE alone is not loop period. Do not send simultaneous manual guide pulses as a fallback or correction supplement.

Normal guiding runs until `AGENT_ABORT_PROCESS.ABORT=true` or an unrecovered error. Aborting reports DONE/OK with a “Guiding aborted” message; failure reports FAILED/ALERT. It stops logging, records predictive model stop time, restores saved camera modes and, where used, subframe geometry. Start again only after confirming cleanup and actual camera/mount state.

## Correction controllers

**GDR-11.** Choose exactly one controller in each idle switch vector. `AGENT_GUIDER_CORRECTION_MODE_RA` offers `PI_CONTROLLER`, `HYSTERESIS`, `LINEAR_TREND`, `PPEC`, `MKGP`; `AGENT_GUIDER_CORRECTION_MODE_DEC` offers `PI_CONTROLLER`, `HYSTERESIS`, `LINEAR_TREND`, `RESIST_SWITCH`. These are alternatives per axis, not simultaneously enabled stages. Detection mode remains independent.

| Controller | Actual response and `AGENT_GUIDER_SETTINGS` inputs |
| --- | --- |
| `PI_CONTROLLER` | Proportional response plus integral/history term. `AGGRESSIVITY_RA/DEC` are percent gains, `I_GAIN_RA/DEC` are integral gains; `STACK` is integer averaging/history length in frames. I=0 makes the response proportional only. |
| `HYSTERESIS` | Mixes current residual with prior residual before correction. `HYSTERESIS_AGGRESSIVENESS_RA/DEC` (%) controls response; `HYSTERESIS_HYSTERESIS_RA/DEC` (%) controls history weighting. |
| `LINEAR_TREND` | Builds a residual-history trend and uses it in correction; `LINEAR_TREND_AGGRESSIVENESS_RA/DEC` (%) tunes response. Histories reset at session start. |
| `RESIST_SWITCH` (Dec only) | Resists changing correction direction from weak/noisy contrary drift; `RESIST_SWITCH_AGGRESSIVENESS_DEC` (%) and `RESIST_SWITCH_FAST_THRESHOLD_DEC` (pixels; 0 disables fast switching) control it. |
| `PPEC`, `MKGP` (RA only) | Reactive plus learned periodic prediction; separate controls/learning/reset described below. |

Shared settings are `MIN_ERROR` (pixel deadband), `MIN_PULSE` and `MAX_PULSE` (seconds). The loop converts pixel corrections using calibrated rates and applies correction constraints, rather than treating gain as a requested pulse duration. Configure safe pulse/time settings for the equipment; do not make universal numeric tuning promises. Modes cannot be changed during any active process. Many scalar settings can be accepted live, but a short OK acknowledgement is not evidence that settling has improved; compare fresh short-term and cumulative measurements over a meaningful interval.


### Exact settings items and source defaults

These are source initialization limits/defaults, not an equipment-specific tuning recommendation. Live schema/readback remains authoritative for a connected version. Semantics and prerequisites are in the recipes above.

| Exact numeric item | Meaning / unit | Min…max | Step | Default |
| --- | --- | --- | --- | --- |
| `EXPOSURE` | Exposure time (s) | 0…120 | 0.1 | 1 |
| `DELAY` | Delay time (s) | 0…120 | 1 | 0 |
| `STEP0` | Calibration step (s, 0 = auto) | 0…2 | 0.05 | 0 |
| `MAX_BL_STEPS` | Max clear backlash steps | 0…50 | 1 | 10 |
| `MIN_BL_DRIFT` | Min clear backlash drift (px) | 0…25 | 1 | 3 |
| `MAX_CALIBRATION_STEPS` | Max calibration steps | 0…50 | 1 | 20 |
| `MIN_CALIBRATION_DRIFT` | Min calibration drift (px) | 0…100 | 1 | 5 |
| `ANGLE` | Angle (°) | -180…180 | 1 | 0 |
| `SIDE_OF_PIER` | Side of Pier (-1=E, 1=W, 0=undef) | -1…1 | 1 | 0 |
| `BACKLASH` | Dec backlash (px) | 0…100 | 0 | 0 |
| `SPEED_RA` | RA speed at equator (px/s) | -500…500 | 0.1 | 0 |
| `SPEED_DEC` | Dec speed (px/s) | -500…500 | 0.1 | 0 |
| `MIN_ERROR` | Min error (px) | 0…5 | 0.1 | 0 |
| `MIN_PULSE` | Min pulse (s) | 0…1 | 0.001 | 0.01 |
| `MAX_PULSE` | Max pulse (s) | 0…5 | 0.1 | 1 |
| `AGGRESSIVITY_RA` | RA Proportional aggressiveness (%) | 0…150 | 5 | 80 |
| `AGGRESSIVITY_DEC` | Dec Proportional aggressiveness (%) | 0…150 | 5 | 80 |
| `I_GAIN_RA` | RA Integral gain | 0…10 | 0.05 | 0.5 |
| `I_GAIN_DEC` | Dec Integral gain | 0…10 | 0.05 | 0.5 |
| `STACK` | Integral stack size (frames) | 1…20 | 1 | 1 |
| `HYSTERESIS_AGGRESSIVENESS_RA` | RA Hysteresis aggressiveness (%) | 0…150 | 5 | 70 |
| `HYSTERESIS_AGGRESSIVENESS_DEC` | Dec Hysteresis aggressiveness (%) | 0…150 | 5 | 70 |
| `HYSTERESIS_HYSTERESIS_RA` | RA Hysteresis (%) | 0…100 | 1 | 10 |
| `HYSTERESIS_HYSTERESIS_DEC` | Dec Hysteresis (%) | 0…100 | 1 | 10 |
| `LINEAR_TREND_AGGRESSIVENESS_RA` | RA Linear Trend aggressiveness (%) | 0…150 | 5 | 80.0 |
| `LINEAR_TREND_AGGRESSIVENESS_DEC` | Dec Linear Trend aggressiveness (%) | 0…150 | 5 | 80.0 |
| `RESIST_SWITCH_AGGRESSIVENESS_DEC` | Dec Resist Switch aggressiveness (%) | 0…150 | 5 | 100 |
| `RESIST_SWITCH_FAST_THRESHOLD_DEC` | Dec Resist Switch fast-switch threshold (px, 0=disabled) | 0…20 | 0.1 | 0.6 |
| `DITHERING_MAX_AMOUNT` | Dithering max amount (px) | 0…15 | 1 | 1 |
| `DITHERING_SETTLE_TIME_LIMIT` | Dithering Settle time limit (s) | 0…300 | 1 | 60 |
| `DITHERING_LIMIT` | Dithering min settling limit (frames) | 1…50 | 1 | 5 |
| `PPEC_REACTIVE_GAIN_RA` | RA PPEC reactive gain (%) | 0…100 | 5 | 60 |
| `PPEC_PREDICTION_GAIN_RA` | RA PPEC predictive gain (%) | 0…100 | 5 | 50 |
| `PPEC_PERIOD_RA` | RA PPEC period (s, 0=auto) | 0…2000 | 10 | 0 |
| `PPEC_PERIOD_FIXED` | RA PPEC fixed period (0=auto-adjust, 1=fixed) | 0…1 | 1 | 0 |
| `PPEC_RETAIN_MODEL_RA` | RA PPEC retain model (% of period) | 0…80 | 5 | 40 |
| `MKGP_REACTIVE_GAIN_RA` | RA MKGP reactive gain (%) | 0…100 | 5 | 60 |
| `MKGP_PREDICTION_GAIN_RA` | RA MKGP predictive gain (%) | 0…100 | 5 | 50 |
| `MKGP_PERIOD_RA` | RA MKGP period (s, 0=auto) | 0…2000 | 10 | 0 |
| `MKGP_PERIOD_FIXED` | RA MKGP fixed period (0=auto-adjust, 1=fixed) | 0…1 | 1 | 0 |
| `MKGP_PERIOD2_RA` | RA MKGP 2nd stage period (s, 0=auto) | 0…2000 | 10 | 0 |
| `MKGP_PERIOD2_FIXED` | RA MKGP 2nd stage fixed period (0=auto-adjust, 1=fixed) | 0…1 | 1 | 0 |
| `MKGP_RETAIN_MODEL_RA` | RA MKGP retain model (% of period) | 0…80 | 5 | 40 |

## Declination and pointing

**GDR-12/13.** `AGENT_GUIDER_DEC_MODE` items `BOTH`, `NORTH`, `SOUTH`, `NONE` select allowed Dec correction directions. NONE means RA-only guiding; it does not disable all guiding. Switching to/from BOTH while the GUIDING operation is active is refused; stop before that transition. RA-only/one-sided Dec modes also project dithering onto RA so Dec is not deliberately moved by a dither.

`AGENT_GUIDER_APPLY_DEC_BACKLASH.ENABLED/DISABLED` controls application of the calibrated Dec-backlash correction. `AGENT_GUIDER_FLIP_REVERSES_DEC.ENABLED/DISABLED` selects whether Dec calibration direction reverses across a meridian flip. These settings do not execute a flip. Coordinate/slew/flip orchestration belongs to the related Mount Agent; stop/restart/recalibrate as that workflow requires.

`AGENT_GUIDER_MOUNT_COORDINATES` accepts numeric `RA` in hours (0–24), `DEC` in degrees (-90–90), `SIDE_OF_PIER` (-1 east, +1 west, 0 unknown). It describes **current** pointing and feeds declination/rotation/model handling; normally let the Mount relationship provide consistent values. Do not use it to command a mount slew.

`AGENT_GUIDER_SETTINGS.ANGLE/BACKLASH/SPEED_RA/SPEED_DEC/SIDE_OF_PIER` describe the **calibration** response. They are writable for restoration/service, but editing them is not calibration; preserve their units/signs and source provenance. Current and calibration side-of-pier are distinct. Unknown pointing/SOP may prevent predictive-model reuse and makes flip interpretation unreliable. When the geometry is uncertain, stop and recalibrate through the agent instead of guessing sign reversals.

## Dither and manual offsets

**GDR-14/15.** The normal imaging workflow lets the related [Imager Agent](imager.md#dithering-and-transit) request a dither **between exposures**. Configure `AGENT_GUIDER_DITHERING_STRATEGY` with one item `RANDOMIZED_SPIRAL` (default), `RANDOM` or `SPIRAL`. They generate successive reference offsets; changing strategy reseeds/resets its sequence counter. Set these numbers on `AGENT_GUIDER_SETTINGS`:

| Item | Unit and role |
| --- | --- |
| `DITHERING_MAX_AMOUNT` | Image pixels; controls generated dither amplitude. It is not a mount-coordinate displacement in arcseconds. |
| `DITHERING_SETTLE_TIME_LIMIT` | Seconds allowed to settle. |
| `DITHERING_LIMIT` | Number of residual frames used for the settling decision, not a fixed pixel acceptance threshold or seconds. |

While PHASE is GUIDING, trigger `AGENT_GUIDER_DITHER.TRIGGER=true`. The dither property becomes BUSY; the agent generates offsets, changes its reference and monitors residual settling. Wait for this property to return OK or ALERT and observe `AGENT_GUIDER_STATS.DITHERING`. A trigger while not guiding yields ALERT. A timeout/abort can yield ALERT even if the mount has moved: do not immediately start the next imaging exposure on command acknowledgement alone.

`AGENT_GUIDER_DITHER.RESET=true` resets/reseeds the **sequence counter** while this property is not BUSY; it does not calibrate, recenter the guide star or reset a predictive model. Changing strategy has a similar counter side effect. With non-BOTH Dec mode the agent projects the requested displacement onto RA, retaining magnitude; reported X/Y values may differ from requested targets.

For a deliberate manual reference shift, set numbers `AGENT_GUIDER_DITHERING_OFFSETS.X/Y` in guide-image pixels. A changed target during guiding resets settling/RMSE bookkeeping and changes the reference; this is a reference position, not one timed manual pulse. Its property can return OK as soon as the offset is stored, before the mount settles. Use the DITHER property trigger for the normal tracked dither operation; there is no separate “abort manual offset” command. Setting X/Y back to 0 requests a reference return and may itself move the mount. Global process abort stops guiding; it is not guaranteed to return to the old sky position.

Predictive models are informed when a normal dither is triggered; the loop falls back to reactive corrections while settling. Direct offset edits are a distinct lower-level agent control and should not be substituted into a science sequence without coordinating exposure timing and checking observed drift.

## Predictive models

**GDR-16/17.** Choose `PPEC` or `MKGP` on the RA correction-mode vector while idle. Both use a learned periodic model plus reactive response; MKGP adds a second periodic stage. They do not modify mount firmware PEC. They require valid RA calibration, usable residuals and enough actual guiding time to learn. The model of the inactive controller is separate.

All following controls are numeric items on `AGENT_GUIDER_SETTINGS`:

| Items | Meaning |
| --- | --- |
| `PPEC_REACTIVE_GAIN_RA`, `PPEC_PREDICTION_GAIN_RA` | Percent reactive/predictive weights. |
| `PPEC_PERIOD_RA` | Seconds; 0 permits automatic period estimation, positive value seeds the period. |
| `PPEC_PERIOD_FIXED` | 0 permits adjustment, 1 holds a chosen period fixed. A positive seed alone does not imply fixed. |
| `PPEC_RETAIN_MODEL_RA` | Maximum stop interval as percent of model period for considering restart reuse (0–80 in source). |
| `MKGP_REACTIVE_GAIN_RA`, `MKGP_PREDICTION_GAIN_RA` | Percent reactive/predictive weights for MKGP. |
| `MKGP_PERIOD_RA`, `MKGP_PERIOD_FIXED` | Primary period seconds and 0/1 fixed flag. |
| `MKGP_PERIOD2_RA`, `MKGP_PERIOD2_FIXED` | Second-stage period seconds (0 auto) and 0/1 fixed flag. |
| `MKGP_RETAIN_MODEL_RA` | Restart retention threshold as percent of period, analogous to PPEC. |

Observe `AGENT_GUIDER_STATS.PPEC_LEARNING/PPEC_PERIOD` or `MKGP_LEARNING/MKGP_PERIOD/MKGP_PERIOD2/MKGP_STRENGTH2`; learning/strength are percentages and periods seconds. Session restart reuse checks stop time and known unchanged pier side/current RA. Unknown SOP=0 causes a reset path; a retention setting alone cannot guarantee reuse after a flip or long stop.

Set `AGENT_GUIDER_RESET_PPEC.RESET=true` or `AGENT_GUIDER_RESET_MKGP.RESET=true` to discard that controller's learned model. While its controller is actively driven, the handler records a request and the guiding loop performs the reset; otherwise it can reset immediately. The momentary reset item clears and its property can already be OK while a message says reset **will** occur. Wait for subsequent model statistics/message before concluding learning restarted. Resetting a model does not stop guiding, recalibrate or reset dither sequence. On poor predictions, inspect calibration/cadence/pointing and residual quality; do not compensate with direct pulses.

## Failure policy

**GDR-18.** `AGENT_PROCESS_FEATURES` is any-of-many but normalizes mutually exclusive policy alternatives. Read accepted switches after changing it:

| Item | Meaning |
| --- | --- |
| `FAIL_ON_CALIBRATION_ERROR` | Stop on the applicable guide-star/calibration acquisition failure. |
| `RESET_ON_CALIBRATION_ERROR` | Reset selection/reacquire on applicable failures instead of the fail alternative. |
| `FAIL_ON_GUIDING_ERROR` | End guiding on applicable lost-star failure. |
| `CONTINUE_ON_GUIDING_ERROR` | Preserve the current selection/reference and wait for stars to reappear; frames without a trustworthy measurement do not justify fresh corrections. |
| `RESET_ON_GUIDING_ERROR` | Reset/reacquire selection/reference on applicable lost-star failure. |
| `RESET_ON_GUIDING_ERROR_WAIT_ALL_STARS` | With reset policy, wait for the initially required usable-star set before resuming. |
| `ENABLE_LOGGING` | Enable calibration/guiding file logging; see below. |
| `USE_INCLUDE_FOR_DONUTS` | Use include-region digest for DONUTS; see detection. |

These policies concern the implemented acquisition/lost-star paths; they do not turn communication/motion faults into guaranteed recovery. A process may remain BUSY while waiting for clouds/stars. Inspect phase, fresh FRAME, SNR, messages and actual device states; abort if the required field cannot return. Do not repeatedly submit GUIDING while it is already BUSY. After fail-policy termination, correct the cause while idle and start explicitly.

## Logging

**GDR-19.** While all processes are idle, set text `AGENT_GUIDER_LOG.DIR` to an existing writable **server-host** directory, nonempty, and `.TEMPLATE` to the desired strftime filename pattern (for example `GUIDING_%Y%m%d_%H%M%S.csv`). Enable `AGENT_PROCESS_FEATURES.ENABLE_LOGGING=true` before calibration/guiding. Log settings changes during a process yield ALERT and are refused. The handler normalizes a trailing directory separator; it does not provision an arbitrary missing directory.

Starting calibration/guiding opens a file and writes headers/records; stopping closes it. Source also registers `/guiding` as an HTTP file resource for the current log. This route is a server-host resource, not an MCP protocol method, and multiple Guider instances can share that route name; do not assume a per-instance archive endpoint. Confirm which instance/current log it denotes. File creation failure is reported by an agent message; logging enablement is not proof a file was written. Choose collision-free templates and verify the actual resulting server file/content. There is no Guider log-delete command in this interface.

## Statistics

**GDR-20.** `AGENT_GUIDER_STATS` is read-only. A single field is not a completion condition; use process state and fresh observations. Items and units:

| Items | Interpretation |
| --- | --- |
| `PHASE`, `FRAME` | State-machine phase and session frame number. |
| `REFERENCE_X/Y`, `DRIFT_X/Y` | Reference point and image displacement in pixels. |
| `DRIFT_RA/DRIFT_DEC` | Rotated axis residuals in pixels. |
| `DRIFT_RA_S/DRIFT_DEC_S` | Axis residuals in arcseconds using configured optical pixel scale; `_S` here does not mean seconds. |
| `CORR_RA/CORR_DEC` | Signed correction duration in seconds. |
| `RMSE_RA/RMSE_DEC`, `RMSE_RA_S/RMSE_DEC_S` | Session-cumulative residual RMS in pixels / arcseconds. |
| `RMSE_RA_ST/RMSE_DEC_ST`, `RMSE_RA_S_ST/RMSE_DEC_S_ST` | Sliding-window short-term residual RMS in pixels / arcseconds. |
| `CORR_RESPONSE_RA/CORR_RESPONSE_DEC` | Dimensionless lag-1 residual correlation (-1…1); zero until enough measurements are available. Positive/negative patterns help diagnose under/over-response, not target coordinates. |
| `SNR`, `DELAY`, `DITHERING` | Frame-digest signal measure, remaining delay seconds, current dither/settling offset diagnostic in pixels. |
| `PPEC_LEARNING/PPEC_PERIOD` | Model learning %, period seconds. |
| `MKGP_LEARNING/MKGP_PERIOD/MKGP_PERIOD2/MKGP_STRENGTH2` | Learning %, primary/second period seconds, second-stage strength %. |

A zero arcsecond residual is not a verified perfect mount if optical scale is missing. Poor residuals can come from measurement, seeing or correction behavior; choose detection based on the image, correction based on the mount response. Optional local background: [correction modes](../../INDIGO_GUIDER_CORRECTION_MODES.md), [correction-response tuning](../../GUIDING_CORRECTION_RESPONSE_TUNING.md), [PI tuning](../../GUIDING_PI_CONTROLLER_TUNING.md), [dithering](../../INDIGO_GUIDER_DITHERING.md). These references are supplemental; operating semantics are included above for offline use.

## Abort and recovery

**GDR-21.** Write `AGENT_ABORT_PROCESS.ABORT=true` during a process/star refresh. It marks abort and internally aborts the selected camera exposure; the worker observes the flag and exits its calibration/guiding/preview path. It is not a physical emergency-stop/zero-pulse guarantee: an in-flight guide transaction still has its transport/device completion behavior. Wait for main/star state to leave BUSY and ABORT to clear, and inspect mount/camera state.

Preview/guiding abort normally ends OK/DONE; calibration abort is unsuccessful calibration. A related Mount Agent may coordinate abort during telescope motion; let its workflow finish before restarting. Camera selection loss or transport failure can end or invalidate a process. Recover by verifying loaded drivers, selectors, connected/output state, image field and pointing; refresh/reselect stars or recalibrate as needed, then restart the agent. Never treat retrying a timed-out start request as exactly-once operation.

## Reset and instances

**GDR-22.** Idle `AGENT_START_PROCESS.RESET=true` resets correction modes, detection, Dec/backlash options, strategy, features, settings, flip policy and star/region selection to source defaults and saves them. It does not command mount firmware factory reset or replace the explicit learned-model reset controls; verify model statistics separately. It does not delete log files. Read back values after reset.

Relevant settings are automatically saved by the agent. Inherited `CONFIG` operations and system configuration profiles are described in [server preparation](../server-and-devices.md), rather than being separate calibration commands. `ADDITIONAL_INSTANCES.COUNT` creates/removes extra Guider instances; do it while idle and rediscover their names/independent selections. It does not create additional physical guide ports. Stop active work before instance removal, and do not attach multiple active guiding loops to the same physical output.

## Draft verification and remaining limits

The recipes/inventory were checked against every Guider-specific branch in `agent_change_property`, property creation, capture/selection, calibration, guiding, dither, predictive reset and logging code in [the implementation](../../../indigo_drivers/agent_guider/indigo_agent_guider.c). Shared contracts and exact identifiers were checked against [filter source](../../../indigo_libs/indigo_filter.c) and [name definitions](../../../indigo_libs/indigo/indigo_names.h). This is source inspection only: no client/simulator/hardware process, physical pulse, optical measurement, file creation or timing test was performed for these draft pages.

Hardware-specific minimum useful pulses, guiding rate, backlash, focus, optical scale, exposure and gain remain equipment/sky decisions. The source-derived recipes intentionally provide no universal tuned preset. Predictive model learning quality, per-platform file/HTTP behavior, multi-instance log-route ownership and physical cleanup after a transport failure remain runtime verification limits, not claimed successful tests. If phase, property state, fresh images and messages disagree, report the discrepancy rather than declaring guiding healthy from one green property.
