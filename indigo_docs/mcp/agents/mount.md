# Mount Agent operating manual

Draft for review. Source-inspected operating instructions; this manual is included in compiled MCP help; these recipes have not been executed as part of this documentation task.

Module: `indigo_agent_mount`. Default device: `Mount Agent`. Discover the complete instance name, including any suffix, before addressing it. The agent coordinates a selected mount, dome, rotator, GPS and joystick and exchanges pointing information with related imaging, guiding and solver agents. It is the normal mount-operation endpoint; hardware properties exposed through its filter are subordinate controls.

## Function inventory

| ID | User function | Recipe |
| --- | --- | --- |
| M01 | Load, select, connect and inspect capabilities | [Prepare](#prepare) |
| M02 | Set site, select its source, propagate host time | [Site and time](#site-and-time) |
| M03 | Set a target and slew | [Slew](#slew) |
| M04 | Synchronize the mount pointing model | [Sync](#sync) |
| M05 | Park and unpark the mount | [Park and unpark](#park-and-unpark) |
| M06 | Find mount home | [Home](#home) |
| M07 | Turn tracking on or off | [Tracking](#tracking) |
| M08 | Park/unpark dome and open/close shutter | [Dome operations](#dome-operations) |
| M09 | Slave dome to mount | [Dome slaving](#dome-slaving) |
| M10 | Control rotator and maintain field orientation | [Rotator and derotation](#rotator-and-derotation) |
| M11 | Enforce hour-angle and local-time limits | [Limits](#limits) |
| M12 | Read pointing, transit, airmass and FOV | [Readouts](#readouts) |
| M13 | Route joystick motion/rates/park/tracking/home/abort | [Joystick](#joystick) |
| M14 | Coordinate Imager, Guider and solver agents | [Related agents](#related-agents) |
| M15 | Offer an LX200 network endpoint | [LX200 server](#lx200-server) |
| M16 | Abort, recover, reset defaults and manage instances | [Abort and recovery](#abort-and-recovery) |
| M17 | Manual mount jog/rate and dome azimuth through the agent | [Manual controls](#manual-controls) |

## Prepare

Follow [server and device setup](../server-and-devices.md). Discover `Server.DRIVERS`; load the offered item for `indigo_agent_mount` if missing and wait for the agent device to appear. Load the actual mount driver in the same way. Configure its serial/network port and model-specific requirements while disconnected. Load optional dome, rotator, GPS or joystick drivers only when needed.

On Mount Agent select the discovered device-name item in `FILTER_MOUNT_LIST`; similarly use `FILTER_DOME_LIST`, `FILTER_ROTATOR_LIST`, `FILTER_GPS_LIST` and `FILTER_JOYSTICK_LIST`. Items are full device names, not fixed `MOUNT`/`DOME` item IDs. Selecting a device invokes the filter's connection handling; wait for the selector to leave `Busy`, retain the intended selection, and publish its forwarded properties. `NONE` deselects a device and can disconnect the previous selection. Do not change selection during an active operation.

Inspect `AGENT_MOUNT_FEATURE` items `SLEW`, `SYNC`, `PARK`, `HOME`, `TRACK`, and `AGENT_DOME_FEATURE` items `SLEW`, `SYNC`, `PARK`, `OPEN`. These are read-only capability switches derived from selected devices' properties. A process switch exists even when the selected device cannot perform it; unsupported starts finish in `Alert`. Do not interpret an empty capability set as a request to bypass the agent.

For all process recipes, write one true item in `AGENT_START_PROCESS`. Watch that property's state and the corresponding read-only `AGENT_MOUNT_STATE` or `AGENT_DOME_STATE` light. `Busy` means outstanding work; `Ok` on the process is the agent's completion report, and `Alert` requires inspection before retry. An MCP `dispatched:true` reply is only submission, not that completion report. Query the property again after dispatch. The process switches clear on completion; their false value alone does not distinguish failure from success. Current MCP reads have no event history or operation identifier: an unchanged `Ok` snapshot alone cannot establish that a new operation completed. Follow [observation rules](../instructions.md) and explicitly report when freshness or completion cannot be established.

## Site and time

`AGENT_SITE_DATA_SOURCE` is a one-of-many choice: `HOST` uses the agent's coordinates, `MOUNT` uses selected mount coordinates, `DOME` uses selected dome coordinates, `GPS` uses selected GPS coordinates. HOST here means the agent's configured site, not automatic computer geolocation.

For HOST write agent `GEOGRAPHIC_COORDINATES` items `LATITUDE` (degrees north, negative south), `LONGITUDE` (degrees east; accepted range includes negative western longitude), `ELEVATION` (metres). Example synthetic inputs are latitude 48, longitude 17, elevation 200. Set the source explicitly and read the agent's coordinates back. With mount source the site propagates to the dome; with dome source to mount; GPS or HOST propagates to both. If a selected source is unavailable, implementation falls back to configured agent targets. The exact pair latitude=0, longitude=0 is treated as unset and is not written over device sites; a warning is emitted. Confirm a plausible site before relying on altitude, dome geometry, limits or polar alignment.

`AGENT_SET_HOST_TIME` has independent `MOUNT` and `DOME` switches, initially true. Setting them and site propagation can send host-time commands to selected devices. Read back the forwarded device time properties where supported; `Ok` on this agent setting only confirms processing of the setting, not an independent clock measurement. Disable unwanted propagation explicitly before selecting devices. Site/time updates also affect related Imager FITS headers and derived coordinate readouts. There is no long-running site-setting process to abort; restore previous values/source to recover.

## Coordinate epochs

Read the selected mount's forwarded `MOUNT_EPOCH.EPOCH` before preparing coordinates. It declares the frame used by its `MOUNT_EQUATORIAL_COORDINATES`: commonly 2000 for J2000 or 0 for JNow; use the selected driver's actual contract for other offered epochs. Mount Agent forwards its `AGENT_MOUNT_EQUATORIAL_COORDINATES` targets unchanged to the selected mount and mirrors measured coordinates unchanged. The agent target property has no separate EPOCH item and performs no automatic conversion for a client-provided RA/DEC. A client must supply the selected mount's declared frame; if that frame is unknown, resolve it before motion or sync. Do not infer it solely from RA/DEC labels or from the absence of an epoch property.

For an image-derived destination, prefer solver SYNC/CENTER/PRECISE_GOTO: the shared solver tracks the related Mount Agent's forwarded `MOUNT_EPOCH`, converts from its hints/WCS frame, then submits the converted target to Mount Agent. Its startup fallback is 2000 until an epoch is reported, so verify the relation and reported epoch before relying on conversion.

**Unresolved implementation inconsistency:** Mount Agent's derived display currently applies a J2000→JNow conversion to mirrored mount coordinates unconditionally, and parts of its coordinated geometry and LX200 bridge also assume J2000 input. This does not establish that every selected mount reports J2000; the shared solver explicitly supports the reported mount epoch. For a non-J2000 selected mount, do not treat the derived JNow/altitude/azimuth, transit/airmass/limit, dome/derotation or LX200-frame calculations as validated by this guide. Resolve that implementation/driver combination before using those dependent functions. If the driver supports configuring J2000 as setup, verify that setting and readback rather than silently changing frames during a workflow. No source correction or non-J2000 runtime validation is supplied by this documentation task.

## Slew

Prerequisites: selected connected mount, `AGENT_MOUNT_FEATURE.SLEW=true`, valid site/time and an idle process. Identify a safe target expressed in the selected mount's declared `MOUNT_EPOCH.EPOCH` frame; see [coordinate epochs](#coordinate-epochs). Write `AGENT_MOUNT_EQUATORIAL_COORDINATES` with `RA` in decimal hours (0–24) and `DEC` in decimal degrees (−90 to +90); then write `AGENT_START_PROCESS.SLEW=true` on Mount Agent. For example RA=5.5, DEC=20 is a numeric illustration, not a safe target for an arbitrary site/time.

The target property's number **targets** retain the requested destination while its **values** follow measured mount coordinates. Merely setting this property stages a target; the SLEW process starts motion. Watch `AGENT_START_PROCESS`, `AGENT_MOUNT_STATE.SLEW`, and measured coordinate values until completion. The agent can unpark before coordinate operations and coordinates any enabled dome slaving and field derotation. It warns if a controlled dome shutter is closed; this warning is not an interlock preventing every slew. Operator clearance remains necessary.

Slew may abort related imaging/guiding when enabled in `ABORT_RELATED_PROCESS`, and resets related Guider star selection. `AGENT_ABORT_PROCESS.ABORT=true` requests an abort/stop through the agent; verify measured movement stops before recovery. On `Alert`, inspect selected mount/dome/rotator states and messages, correct the cause, re-read position and stage a new safe target. Do not replay an old slew automatically.

## Sync

Use sync only when the telescope's true pointing is known, normally from a solver. With capability `SYNC`, stage the known RA/DEC in the selected mount's declared epoch in `AGENT_MOUNT_EQUATORIAL_COORDINATES` and start `AGENT_START_PROCESS.SYNC=true`. This changes the mount's pointing interpretation rather than requesting a normal move to a new object. Observe `AGENT_START_PROCESS` and coordinate readback. Enabled field derotation can also sync the selected rotator. Use the solver's solve-and-sync process when the known position comes from an image; it performs epoch conversion and this agent operation in sequence.

Abort/recovery follows [abort and recovery](#abort-and-recovery). A wrong sync can corrupt subsequent pointing: do not retry with guessed coordinates; restore a trustworthy alignment by the mount's documented procedure or a verified solve.

## Park and unpark

Start `AGENT_START_PROCESS.PARK=true` or `.UNPARK=true` with mount `PARK` capability. No coordinate input is required: the selected driver's stored park position and semantics apply. Watch process state and `AGENT_MOUNT_STATE.PARK`: `Ok` denotes parked, `Idle` unparked, `Busy` transition, `Alert` failure. Process `Ok` is expected for a successful unpark even though the park light becomes `Idle`.

When dome control is enabled, these workflows can coordinate dome park/unpark; inspect `AGENT_DOME_STATE.PARK` too. Park is not the same as close-shutter: use the explicit dome shutter operation as required. Abort through the agent and verify actual states; never assume parking, shutdown or physical protection from an MCP submission reply. Recover by resolving the selected driver's alert, checking actual park position/status and issuing the appropriate explicit process.

## Home

With `AGENT_MOUNT_FEATURE.HOME=true`, start `AGENT_START_PROCESS.HOME=true`. Home is the selected mount's reference-finding operation, distinct from parking. Observe the process and `AGENT_MOUNT_STATE.HOME`. It can move hardware and interrupt related work through the agent's monitoring. Abort via the agent; after interruption verify the device's reference status before using pointing operations. A driver-specific home-position definition belongs to setup and its own manual, not a fabricated universal agent property.

## Tracking

Start `AGENT_START_PROCESS.TRACK_ON=true` or `.TRACK_OFF=true` when `TRACK` capability is present. Observe the process and `AGENT_MOUNT_STATE.TRACK` (`Ok` on, `Idle` off). This does not choose a universal sidereal/lunar/solar tracking rate; supported rates are forwarded selected-device settings and must be discovered. Tracking off changes the sky drift of the current field and prevents continuous dome slaving/derotation prerequisites from being met. Stop acquisition/guiding intentionally before changing it. Abort and inspect on failure, then issue the desired state explicitly.

## Dome operations

Select a dome and inspect `AGENT_DOME_FEATURE.PARK` and `.OPEN`. Start one of `AGENT_START_PROCESS.DOME_PARK`, `.DOME_UNPARK`, `.DOME_OPEN`, `.DOME_CLOSE`. Each is a boolean trigger; no numeric input. Observe process state and `AGENT_DOME_STATE.PARK` or `.OPEN`. For OPEN, `Ok` means open and `Idle` closed; for PARK, `Ok` parked and `Idle` unparked. Process `Ok` means that particular requested transition completed.

These operations move dome mechanics and can interrupt related acquisition/guiding under configured permissions. DOME_PARK does not universally imply DOME_CLOSE. Abort via the agent; if interruption leaves an intermediate shutter or park state, inspect physical/device status and recover through an explicit agent operation. Driver-specific shutter limits and dome geometry are setup requirements.

## Dome slaving

Configure actual dome geometry and its coordinate support through selected-device setup first. Select both mount and dome, set a valid site, then enable `AGENT_PROCESS_FEATURES.ENABLE_DOME_SLAVING`. On agent slews the dome can move to the calculated target azimuth; when idle it follows mount updates while both are unparked and the mount tracks. Observe `AGENT_MOUNT_STATE.DOME_SLAVING`, `AGENT_DOME_STATE.SLEW` and forwarded dome coordinates. No separate slaving start command exists.

Disable `ENABLE_DOME_SLAVING` to stop future automatic updates; use agent abort to stop current movement and verify it. `MAKE_DOME_SLAVING_PERSISTENT` controls whether the enabled setting is saved across reload; otherwise configuration saves clear the persisted enabled flag while retaining the live setting. This is not a movement-completion switch. Failed geometry/site/connection must be repaired before re-enabling.

## Rotator and derotation

Select through `FILTER_ROTATOR_LIST`. Forwarded `ROTATOR_POSITION` and `ROTATOR_ON_POSITION_SET` describe the selected rotator's capabilities and units (position in degrees); configure required direction/limits/calibration in setup. There is no independent `AGENT_START_PROCESS.ROTATE` command. Normal coordinated rotation is driven by `AGENT_PROCESS_FEATURES.ENABLE_FIELD_DEROTATION` and agent SLEW/SYNC.

Enabling derotation records the current rotator position relative to the calculated parallactic angle. While mount is selected, unparked and tracking, the agent updates the rotator to preserve that field orientation; coordinated slews also use it. Watch `AGENT_MOUNT_STATE.FIELD_DEROTATION`, forwarded rotator position/state, and display `PARALLACTIC_ANGLE`/`DEROTATION_RATE`. `MAKE_FIELD_DEROTATION_PERSISTENT` controls persistence of the enable setting. The coordinate calculation depends on correct site/pointing and is not guaranteed mechanical cable-wrap protection.

Disable the enable switch to stop future corrections, abort current movement and verify rotator state. Correct the site, rotator reference/direction or mechanical condition before enabling again. Manual positioning/calibration through forwarded device controls is setup/service work; inspect and use the selected rotator's published items rather than inventing an agent process.

## Limits

Set `AGENT_LIMITS.HA_TRACKING` (hour angle in hours, 0–24) and `LOCAL_TIME` (server local time in hours, 0–24), then enable `AGENT_PROCESS_FEATURES.ENABLE_HA_LIMIT` and/or `.ENABLE_TIME_LIMIT`. Number **targets** are thresholds; **values** are updated with current hour angle and local time, so a readback value is not necessarily the configured threshold. Time comparisons distinguish the morning and afternoon halves of the day; these are not a general date-aware scheduler.

When a threshold is reached while the selected mount is unparked, the agent requests parking and may abort related imaging/guiding according to configuration. Observe park/process/device states; do not assume a complete autonomous observatory shutdown. `AGENT_LIMITS.COORDINATES_PROPAGATE_THRESHOLD` is a degree threshold used to detect changes in latitude/longitude from mount/dome/GPS before propagating the selected site source, initially five arcseconds (5/3600 degree); it is not the hour-angle limit or a solver search radius. Disable the corresponding limit switches when intentionally removing this behavior. After a limit alert check the saved targets and actual park status before restarting tracking or acquisition.

## Readouts

`AGENT_MOUNT_EQUATORIAL_COORDINATES` values mirror measured selected-mount RA hours/DEC degrees without epoch conversion; interpret them using `MOUNT_EPOCH.EPOCH`, not an unconditional J2000 assumption. `AGENT_MOUNT_DISPLAY_COORDINATES_PROPERTY` is the exact wire name, including `_PROPERTY`. Its items are `RA_JNOW`, `DEC_JNOW`, `ALT`, `AZ`, `AIRMASS`, `HA`, `RISE`, `TRANSIT`, `SET`, `TIME_TO_TRANSIT`, `FLIP_REQUIRED`, `PARALLACTIC_ANGLE`, `DEROTATION_RATE`. RA, HA and time readouts use hours; angular positions use degrees; derotation rate uses arcseconds/second; airmass is dimensionless and flip-required is numeric 0/1. They depend on reported coordinates and site/time and are subject to the [non-J2000 calculation limitation](#coordinate-epochs). An idle display can mean tracking is off, not a disconnected endpoint.

`AGENT_MOUNT_FOV.ANGLE`, `.WIDTH`, `.HEIGHT` are degrees describing the image field, writable for display/coordination and normally supplied by a related solver's WCS result. They do not measure optical focal length themselves. Read them after a successful solve rather than assuming old values describe a new camera/binning configuration. The agent also publishes state lights and capability switches described above; they are observations, not writable actions.

## Joystick

Choose an actual joystick in `FILTER_JOYSTICK_LIST` and enable `AGENT_PROCESS_FEATURES.ENABLE_JOYSTICK_CONTROL`. The agent listens to forwarded `JOYSTICK_MOUNT_PARK`, `JOYSTICK_MOUNT_TRACKING`, `JOYSTICK_MOUNT_HOME`, `JOYSTICK_MOUNT_SLEW_RATE`, `JOYSTICK_MOUNT_MOTION_RA`, `JOYSTICK_MOUNT_MOTION_DEC`, `JOYSTICK_MOUNT_ABORT_MOTION` updates. Park/tracking/home requests become corresponding agent processes. Rate choices are `GUIDE`, `CENTERING`, `FIND`, `MAX`; RA axis directions are `WEST`/`EAST`, DEC `NORTH`/`SOUTH`. Releasing an axis must publish false directions so continuous motion stops; missing release updates must not be assumed harmless.

Observe selected mount motion/rate and agent lights. Disable joystick control to stop future routing, explicitly abort ongoing motion and verify stopped state. Joystick mapping, dead zones and physical buttons come from the chosen joystick driver, not fixed button numbers in this manual. Avoid simultaneous joystick and automated control.

## Manual controls

For intentional manual jogging, use the properties forwarded **on Mount Agent**, after selecting/connecting the mount and stopping conflicting automatic work. Discover the published `MOUNT_SLEW_RATE` choices (`GUIDE`, `CENTERING`, `FIND`, `MAX` where supported). Send `MOUNT_MOTION_RA` with `WEST` or `EAST` true, or `MOUNT_MOTION_DEC` with `NORTH` or `SOUTH` true. These are continuous-motion switches, not distances in degrees. Release the corresponding axis by sending its direction switches false; a submission or disconnected client is not a guaranteed stop. Use forwarded `MOUNT_ABORT_MOTION.ABORT_MOTION` when offered, or agent abort for coordinated stop, and verify actual motion states. Movement monitoring can interrupt related work according to the configured abort permissions. Restore idle controls before starting an agent SLEW process.

A dome offering manual azimuth control exposes forwarded `DOME_ON_COORDINATES_SET` (choose discovered GOTO or SYNC mode) and `DOME_HORIZONTAL_COORDINATES.AZ` in degrees on Mount Agent. GOTO moves the dome; SYNC changes its azimuth reference and must only use a known reference position. This is manual setup/service control, distinct from normal coordinated dome slaving. Disable conflicting slaving, confirm capability/schema and clearance, issue the intended operation, then observe forwarded dome state and `AGENT_DOME_STATE.SLEW`. Abort through the agent and inspect actual state on failure. Restore a correct reference and slaving policy before resuming coordinated observations.

## Related agents

Select full discovered names in `FILTER_RELATED_AGENT_LIST`. `ABORT_RELATED_PROCESS.IMAGER` and `.GUIDER` independently permit this Mount Agent to abort those related processes; both default false. Movement monitoring can use these on slew/park/home/dome changes, including changes detected from selected devices. Slews also reset the related Guider star selection, so restart guiding with fresh selection/calibration as appropriate.

Related Imager receives pointing/airmass/site FITS headers; related Guider receives mount coordinates and side of pier. A solver related to this Mount Agent can observe its pointing and derive updated positional hints through the shared solver layer. The site-coordinate propagation threshold is not a solver-hint change threshold. For solver-controlled sync, center or polar alignment, also select this Mount Agent in the **solver's** related-agent list. Do not assume selecting the solver here establishes the reverse relationship unless the discovered symmetric-relations setting confirms it. Use [Imager](imager.md), [Guider](guider.md), [Platesolver](platesolver.md) or [Astrometry](astrometry.md) recipes to control those processes. Multiple candidates are resolved by implementation's first related agent of a required type; select one intentional controller of each role.

## LX200 server

This optional endpoint lets an LX200 client control Mount Agent. Configure `AGENT_LX200_CONFIGURATION.PORT` (TCP port, default 4030) and `EPOCH` (0=JNow, 2000=J2000; other values are adjusted to 0), then set `AGENT_LX200_SERVER.STARTED=true`. Observe that property leaving `Busy`; `Alert` can indicate bind/start failure. The listener translates supported coordinate, slew/sync and motion requests to the agent/selected mount. It is a separate network listener, not an MCP transport or authentication setting. Its current J2000 bridge assumption is subject to the [coordinate-epoch limitation](#coordinate-epochs); do not infer correct non-J2000 conversion merely from its configurable client EPOCH.

Stop with `AGENT_LX200_SERVER.STOPPED=true` and wait for state confirmation. Stop before changing the effective listening port. If it fails, inspect port conflicts and lifecycle messages; do not expose a new port as a guessed recovery. Reset defaults does not reset LX200 configuration while its server handle is active.

## Abort and recovery

Send `AGENT_ABORT_PROCESS.ABORT=true` to Mount Agent. It requests abort of selected mount, dome and rotator motion. Watch process and underlying motion states: abort acceptance alone does not prove a moving mechanism is stationary, and home/park abort reporting can be limited by selected hardware. Wait for quiescence; re-read positions, park/home/tracking states and any alerts. Recover the selected device's actual condition before issuing a new agent process.

`AGENT_START_PROCESS.RESET=true` resets site-source, host-time, process-feature and limit settings, and resets LX200 configuration only when the server is inactive. It saves configuration; it does not home hardware, rebuild a pointing model or reset the selected device's firmware. Recheck site/time/limits/relations before resuming. Additional instances use shared `ADDITIONAL_INSTANCES` support and full-name discovery; common configuration/instance semantics are described in [server and devices](../server-and-devices.md). The agent's own CONNECTION is hidden: connect through selectors, not a fabricated agent CONNECTED workflow.

## Source references and draft limits

Checked against [Mount Agent source](../../../indigo_drivers/agent_mount/indigo_agent_mount.c), [filter implementation](../../../indigo_libs/indigo_filter.c), [wire names](../../../indigo_libs/indigo/indigo_names.h) and the canonical [properties reference](../../PROPERTIES.md). Capability-specific forwarded properties remain the selected driver's contract. Hardware clearance, dome geometry, tracking-rate availability and joystick mappings cannot be inferred from generic labels; discover their schema and consult the relevant driver instructions. This manual does not claim runtime execution or a universal physical safety interlock.
