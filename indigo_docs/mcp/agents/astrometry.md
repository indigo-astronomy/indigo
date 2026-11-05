# Astrometry Agent operating manual

Draft for review. Source-inspected, offline-ready operating guidance; included in compiled MCP help; not runtime-tested by this documentation task.

Module `indigo_agent_astrometry` creates `Astrometry Agent`. It is an INDIGO agent around an **external Astrometry.net toolchain**, with source-agent capture, mount coordination and polar alignment provided by the shared solver layer. It is distinct from `indigo_agent_platesolver` (`Platesolver Agent`), whose star/quad solver runs inside the process using native `.isix` indexes. Both module initializers reject loading while the other is loaded. Pick one backend for the server and switch only after work has stopped and the old backend has been safely unloaded.

The complete shared capture/solve/sync/center/polar/image/settings semantics are in the local [solver operations manual](platesolver.md). They apply to Astrometry Agent with the device name substituted, except the native index-management and native hint-interpretation details. This page supplies the external-backend setup and function-specific route through those recipes; no network documentation lookup is required.

## Function inventory

| ID | User function | Recipe |
| --- | --- | --- |
| A01 | Load and validate external backend prerequisites | [Prepare the backend](#prepare-the-backend) |
| A02 | Install/remove Tycho-2 or 2MASS index families | [Manage indexes](#manage-indexes) |
| A03 | Select installed indexes and tune Astrometry hints | [Select indexes and hints](#select-indexes-and-hints) |
| A04 | Configure source agent, exposure and solver filter | [Select agents and capture settings](#select-agents-and-capture-settings) |
| A05 | Capture and solve once | [Solve only](#solve-only) |
| A06 | Automatically solve incoming related-agent frames | [Incoming frames](#incoming-frames) |
| A07 | Solve and synchronize mount | [Sync](#sync) |
| A08 | Solve, sync and center current target | [Center](#center) |
| A09 | Precise GOTO to a new target | [Precise GOTO](#precise-goto) |
| A10 | Measure three-point polar error | [Polar alignment](#polar-alignment) |
| A11 | Recalculate after manual polar correction | [Polar alignment](#polar-alignment) |
| A12 | Read WCS/FOV/polar error | [Read results](#read-results) |
| A13 | Solve uploaded image through native BLOB input | [Image input and preview](#image-input-and-preview) |
| A14 | Mirror image, enable preview and configure JPEG conversion | [Image input and preview](#image-input-and-preview) |
| A15 | Abort subprocess/download/capture/mount work and recover | [Abort and recovery](#abort-and-recovery) |
| A16 | Legacy modes, reset defaults and shared instances | [Defaults and compatibility](#defaults-and-compatibility) |

## Prepare the backend

Use [server and device setup](../server-and-devices.md) and [common instructions](../instructions.md). Discover `Server.DRIVERS` and the offered `indigo_agent_astrometry` item. The server environment must already provide `solve-field` and `image2xy` on its executable search path; on non-Windows systems initialization also checks `curl`. Missing dependencies produce an unresolved-dependency load failure rather than a usable Astrometry Agent. Repeatedly setting DRIVERS true cannot install a missing binary. Prepare the executable environment on the server host, then load the module and confirm the actual agent device appears.

`image2xy` extracts star positions from a prepared image. `solve-field` performs the external field solve; its normal runtime dependencies, including the Astrometry engine it uses, must also be installed correctly even though the module's initial checks name only those two frontends. The module's command execution and output parsing are not interchangeable with an arbitrary executable bearing the same name. This manual does not claim every third-party Astrometry.net package/version/platform has been verified.

This setup is separate from camera/mount hardware and from MCP client installation. No generic MCP tool installs packages or alters the server's PATH. Provision the approved server-side toolchain and indexes before disconnected operation. The normal solve does not need internet once binaries, index files and image data are locally available. Remote image URLs still require their source to be reachable.

The backend stores files in `$INDIGO_CACHE_BASE/astrometry`, or `~/.indigo/astrometry` when unset. It writes an `astrometry.cfg` describing selected indexes, temporary prepared images/star tables and other solve artifacts there. Ensure the server account can read indexes and write/clean its work files. This path differs from the native solver's `solver` directory and `.isix` format; copying files between these directories does not convert them.

## Manage indexes

The backend exposes two any-of-many management properties:

| Property | Item names | Content |
| --- | --- | --- |
| `AGENT_ASTROMETRY_INDEX_41XX` | `4119` down to `4107` | Tycho-2 index groups |
| `AGENT_ASTROMETRY_INDEX_42XX` | `4219` down to `4200` | 2MASS index groups |

Read the actual item labels for catalog-pattern angular-size intervals and file sizes, paying attention to degrees versus arcminutes. These labels describe index pattern sizes rather than camera pixel scale. Choose groups fitting the source field; do not select every item merely to make an empty list go away.

Setting a management item true requests installation of the group's files; clearing it deletes its associated local FITS files. Some groups are split across multiple files. The cache contains `index-<identifier>.fits`; installation and membership are checked by the backend, not inferred from a single successful property dispatch. Wait for the corresponding management property to leave `Busy`, check messages and confirm actual true items. Download/storage/deletion are setup side effects and may use substantial network/disk resources. An unavailable index download is not a successful offline installation.

After installation, re-enumerate/read `AGENT_PLATESOLVER_USE_INDEX`: installed groups appear there. Select intended groups for actual solving. Management state and use selection are different. Installing a group does not by itself prove that all subsequent solves will use it; after reload verify selections again. Attempts to change management while WCS is `Busy` are rejected. To interrupt a download, use the agent abort; wait for completion and inspect actual installed state before retrying. Partial files are cleaned by backend lifecycle/transfer handling; never manually rename an incomplete file to pretend it is installed.

## Select indexes and hints

`AGENT_PLATESOLVER_USE_INDEX` is an any-of-many set with dynamic installed index identifiers such as `4119` or `4208`. Select at least one suitable available item true. The solver creates its external configuration from selected installed data; a missing usable index fails before a useful solve can occur. Native names such as `INDEX_02DEG_TYC2` and property `X_AGENT_SOLVER_INDEX` belong to the other backend and must not be sent to Astrometry Agent.

Shared `AGENT_PLATESOLVER_HINTS` has these exact items:

| Item | Units / external-backend effect |
| --- | --- |
| `RADIUS`, `RA`, `DEC` | Radius degrees, RA hours, DEC degrees. Positive radius enables positional hints; backend converts RA to degrees for external arguments. Radius 0 removes the restriction. |
| `EPOCH` | 0 requests JNow result convention; use 2000 for J2000. Other requested values are adjusted to 2000. Match positional/target coordinate conventions; the shared coordination layer handles mount epoch conversion. |
| `SCALE` | Degrees/pixel. Positive value supplies a scale interval of approximately ±15%, converted to arcseconds/pixel. Negative requests available camera-derived scale; 0 leaves it unconstrained. |
| `PARITY` | −1, 0, +1; nonzero constrains external parity. Wrong parity can reject the field. |
| `DOWNSAMPLE` | Integer factor, 1–16, applied to star extraction. |
| `DEPTH` | Positive value passes a star-depth limit; 0 leaves external progression unspecified by the agent. |
| `CPULIMIT` | Positive seconds passed to solve-field; 0 omits this limit. It is not a whole-workflow timeout including capture/motion. |

Change hints while idle and read back settings. Example: 2 arcseconds/pixel means SCALE≈0.0005555556, not SCALE=2. A related Mount Agent can update pointing hints. If the solve fails, first inspect stars/image quality, selected index scale, positional radius, parity, pixel scale and tool execution messages. Remove an incorrect restriction deliberately and start a fresh solve; a failed solve never justifies a mount sync.

The common input schema does not mean the two backends implement an identical search strategy: native quad stages, Gaia/Tycho indexes, retained memory-mapped index and native CPU deadline are documented on [Platesolver](platesolver.md#hints); Astrometry search behavior comes from the invoked tools and FITS indexes here.

## Select agents and capture settings

Load Imager Agent or Guider Agent and the actual camera driver through `Server.DRIVERS`. Configure driver ports/model as setup, select camera on the source agent and wait for connection/image readiness. For sync/center/GOTO/polar work, load Mount Agent and mount driver, configure transport/model while disconnected, select mount on Mount Agent and establish site/time. See [Imager](imager.md), [Guider](guider.md), [Mount](mount.md).

On Astrometry Agent select source-agent and optional Mount Agent full-name items in `FILTER_RELATED_AGENT_LIST`. This is directional; confirm this list regardless of reverse relations. The first related Imager is preferred over any related Guider. Choose one intentional source and one Mount Agent to make routing clear. A raw camera selected elsewhere is not a substitute for a related source agent. Read the related Mount Agent's forwarded `MOUNT_EPOCH.EPOCH` before motion/sync: shared solver conversion uses its reported frame, with 2000 only as a startup fallback until reported. Mount Agent forwards these converted targets unchanged; its derived geometry/display has a separate [non-J2000 implementation limitation](mount.md#coordinate-epochs), not a guarantee that all input is J2000.

Set `AGENT_PLATESOLVER_EXPOSURE.EXPOSURE` in seconds for solver-initiated captures. `AGENT_PLATESOLVER_FILTER.CURRENT` keeps current filter; dynamic alternative items mirror the related Imager's `AGENT_WHEEL_FILTER`. Choosing a filter makes solver captures use it and then restore the previous filter while solving. Watch source wheel/capture state and verify filter restoration after interruption. `AGENT_PLATESOLVER_MOUNT_SETTLE_TIME.SETTLE_TIME` is seconds after coordinated movement, default 3. Camera selection, wheel mapping and binning/ROI remain source-agent setup.

The obsolete `AGENT_PLATESOLVER_PA_SETTINGS.EXPOSURE` aliases the modern exposure setting; editing PA settings can copy its current exposure target into the modern property. Configure PA settings first and set modern exposure last. Exposure setting changes do not themselves start an exposure.

## Solve only

With backend, indexes and source ready, set neutral automatic policy `AGENT_PLATESOLVER_SYNC.DISABLED=true`, configure capture/hints, then start `AGENT_START_PROCESS.SOLVE=true` on Astrometry Agent. It requests capture through related Imager (otherwise Guider), receives/possibly downloads the image, extracts stars with image2xy, runs solve-field, parses the result and publishes WCS/FOV. No normal workflow step writes physical `CCD_EXPOSURE` or `MOUNT_EQUATORIAL_COORDINATES` directly.

Watch `AGENT_START_PROCESS`, `AGENT_PLATESOLVER_WCS` and source capture. Confirm a result associated with this request and plausible field scale/coordinates. `Alert` can originate from image transfer, preprocessing, external command failure, no match or later coordination. See [shared capture-and-solve recipe](platesolver.md#capture-and-solve) for MCP property-call examples, replacing the solver device name. Abort and recover below; do not report a stale WCS as the failed frame's solution.

## Incoming frames

Set `AGENT_PLATESOLVER_SOLVE_IMAGES.ENABLED=true` to allow automatic solves of images from selected related sources; select `DISABLED` to stop that behavior for future frames. This does not start an acquisition sequence itself. Start imaging/guiding through its source agent. Each arriving frame may consume CPU or invoke post-solve mount actions according to `AGENT_PLATESOLVER_SYNC`; leave that mode DISABLED when only measurements are intended.

Observe new WCS updates and source state. Disable future automatic inputs and abort outstanding work to stop solving; stop the originating acquisition through its own agent if required. The [shared incoming-image recipe](platesolver.md#incoming-images) explains the distinction from explicit solver-start captures.

## Sync

Select a properly configured Mount Agent with sync capability. Start `AGENT_START_PROCESS.SYNC=true`. This captures/solves, converts the result to the mount epoch and invokes Mount Agent's sync process. It alters the mount pointing model. Check a solve-only result first; watch both solver and Mount Agent process/state. A successful external solve does not prove the following mount sync was accepted/completed.

Abort via Astrometry Agent and verify related movement/capture states. If sync has already happened, a later failure does not undo it. Resolve alignment/epoch/related-agent errors before starting again. [Shared solve-and-sync recipe](platesolver.md#solve-and-sync) applies without replacing it with direct mount writes.

## Center

With source and Mount Agent selected, make `AGENT_PLATESOLVER_HINTS.RA/DEC` reflect the intended center in the configured hints epoch, then start `AGENT_START_PROCESS.CENTER=true`. This captures/solves, syncs the mount at the measured field center and slews back to the saved intended center, waiting SETTLE_TIME. It is a defined solve/sync/recenter sequence, not an accuracy-guaranteed iterative loop. Watch WCS syncing/centering stages and both processes; take a fresh solve-only frame for independent final centering confirmation when needed.

Abort through Astrometry Agent; verify actual pointing/motion and repair errors before retry. The [shared center recipe](platesolver.md#center) covers result and recovery details, including hints versus target distinction.

## Precise GOTO

Write `AGENT_PLATESOLVER_GOTO_SETTINGS.RA` hours and `.DEC` degrees in `AGENT_PLATESOLVER_HINTS.EPOCH`, then start `AGENT_START_PROCESS.PRECISE_GOTO=true`. It first slews through Mount Agent, then captures/solves/syncs/recenters. Side effects begin before solving; dome/rotator/unpark behavior follows the configured Mount Agent. Monitor both agent processes and WCS GOTO→solve→sync→center stages. [Shared precise-GOTO recipe](platesolver.md#precise-goto) includes prerequisites and settling.

Abort through solver and verify mount/dome/rotator quiescence. Do not auto-retry a failed movement with an old target or use physical mount-coordinate writes as fallback when an agent is unavailable.

## Polar alignment

For measurement, select source plus Mount Agent, establish correct site/time and a safe initial pointing above about 40° altitude, preferably just past the meridian. Set `AGENT_PLATESOLVER_PA_SETTINGS.HA_MOVE` in degrees (commonly 15–20), `COMPENSATE_REFRACTION` numeric 0/1, then modern exposure. Start `AGENT_START_PROCESS.CALCULATE_PA_ERROR=true`. The shared layer takes three solved exposures separated by hour-angle moves; Astrometry.net supplies each field solution. Watch solver/Mount processes and `AGENT_PLATESOLVER_PA_STATE`. Do not touch polar knobs during reference collection.

After successful measurement use `ALT_CORRECTION_UP` (1 up/0 down), `AZ_CORRECTION_CW` (1 clockwise/0 counterclockwise), signed `ALT_POLAR_ERROR`/`AZ_POLAR_ERROR` and total `POLAR_ERROR` in **degrees** to adjust the mount mechanically using its equipment procedure. This is not motorized polar correction. Then start `AGENT_START_PROCESS.RECALCULATE_PA_ERROR=true` for a new exposure/residual calculation against the retained reference; repeat manually as appropriate. A valid retained reference is required. After abort/reset/lost reference or unrelated movement repeat the full measurement.

See [measurement](platesolver.md#polar-measurement), [adjustment](platesolver.md#polar-adjustment) and [PA result fields](platesolver.md#results) for the complete shared state/units/recovery contract. An accuracy warning, degenerate references or failed solve must be resolved rather than treating partial numbers as trustworthy corrections.

## Read results

`AGENT_PLATESOLVER_WCS` items: `STATE`, `RA` (hours), `DEC` (degrees), `EPOCH` (0 JNow/2000 J2000), `ANGLE` (degrees east of north), `WIDTH`, `HEIGHT` (degrees), `SCALE` (degrees/pixel), `PARITY` (±1) and `INDEX` (Astrometry index identifier). ANGLE/PARITY follow Astrometry conventions: +1 is mirrored sky (east right with north up), −1 sky as seen. Interpret results only with the current property's success state and association to the intended frame.

Numeric WCS STATE is 0 idle, 1 waiting for image, 2 solving, 3 syncing, 4 centering, 5 initial GOTO. These are stages, separate from the property's `Ok`/`Busy`/`Alert`. FOV is propagated to related Mount Agent `AGENT_MOUNT_FOV` after success. `AGENT_PLATESOLVER_PA_STATE` contains reference/adjustment stage, target/current RA hours and DEC degrees, declination drift and polar errors in degrees, numeric correction directions and warning. Its full item list and stage mapping are in [shared results](platesolver.md#results).

Current MCP reads are cache snapshots with no operation ID or event history. A poll showing `Ok` can be the old snapshot; it alone cannot establish a fresh solve or successful physical motion. Follow [observation rules](../instructions.md), correlate changed result/target/source states where possible, and report when freshness/completion cannot be established.

## Image input and preview

WO BLOB `AGENT_PLATESOLVER_IMAGE.IMAGE` accepts existing images through native INDIGO BLOB transport; the shared layer can download URL-backed input, solve it and copy its solution to GOTO settings. Review the automatic sync mode first because upload is not inherently isolated from mount post-processing. Current MCP `set_property` rejects BLOB writes; a URL/file/base64 string is not a supported substitute. Use source-agent capture or a native BLOB-capable client. [Shared upload recipe](platesolver.md#image-upload) applies.

Read `AGENT_PLATESOLVER_IMAGE_OUTPUT.IMAGE` for the solved-image mirror. `CCD_PREVIEW` uses `DISABLED`, `ENABLED`, `ENABLED_WITH_HISTOGRAM`; `CCD_PREVIEW_IMAGE.IMAGE` holds generated display image metadata. `CCD_JPEG_SETTINGS` items `QUALITY`, `TARGET_BACKGROUND`, `CLIPPING_POINT`, `REFERENCE_CHANNEL`, and `CCD_JPEG_STRETCH_PRESETS` `SLIGHT`, `MODERATE`, `NORMAL`, `HARD` tune conversion/display, not optical acquisition or WCS coordinates. See [preview units and recovery](platesolver.md#preview). MCP BLOB reads provide URLs, not pixels embedded in JSON. Preview enabling does not capture an image by itself.

## Abort and recovery

Send `AGENT_ABORT_PROCESS.ABORT=true`. Shared abort invalidates outstanding work/polar references, attempts related source-capture and Mount Agent motion abort, and calls backend abort. Astrometry abort requests termination of its subprocess group/job and interrupts index transfers, rather than leaving solve-field children running intentionally. Check process/WCS, index-management, source capture and mount-motion states; the abort acknowledgment is not a measured guarantee that everything stopped.

If a binary cannot launch, inspect the server's executable environment and runtime dependencies. If no solve is found, inspect image stars, field scale, indexes and hints. If transfer/storage failed, repair permissions/capacity/connectivity. If sync/center failed, inspect Mount Agent and epoch/site assumptions, preserving that an earlier sync may already have changed alignment. Resume only with deliberate new inputs after quiescence; no hidden automatic retry or rollback is promised.

Do not unload a busy backend as the ordinary abort method. After an interrupted index download re-read actual installed/use-index state and retry the setup action only as intended. Do not delete cache files or kill unrelated server processes to recover a single failed solve.

## Defaults and compatibility

`AGENT_PLATESOLVER_SYNC` remains an obsolete but active policy for automatic images: `DISABLED`, `SYNC`, `CENTER`, `CALCULATE_PA_ERROR`, `RECALCULATE_PA_ERROR`. Explicit start processes choose their mode temporarily and restore the saved policy when finished. Keep automatic policy intentional. `AGENT_PLATESOLVER_ABORT.ABORT` is the legacy alias of modern abort.

`AGENT_START_PROCESS.RESET=true` restores shared solve-images/hints/sync/exposure/PA/settling defaults and CURRENT filter, then saves configuration. It neither reinstalls binaries nor deletes installed index files nor resets a physical mount's alignment. Check defaults before acquiring again. Shared relation/instance/configuration support is covered in [server and devices](../server-and-devices.md); use full discovered instance names and do not assume CONNECTION is a visible agent control.

## Source references and draft limits

Checked against [Astrometry backend](../../../indigo_drivers/agent_astrometry/indigo_agent_astrometry.c), [shared solver](../../../indigo_libs/indigo_platesolver.c), [filter](../../../indigo_libs/indigo_filter.c), [wire names](../../../indigo_libs/indigo/indigo_names.h) and [properties reference](../../PROPERTIES.md). Source links are review provenance; the operational descriptions and local cross-linked shared recipes are the offline manual.

Unresolved equipment/environment specifics: approved external package/version installation procedure, available server-host storage/network and camera/mount-specific setup must be established for that deployment. No universal solver precision, third-party package compatibility, safe mechanical travel or runtime test result is claimed here. Reading a guide does not authorize index downloads, package installation or hardware motion.
