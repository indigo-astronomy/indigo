# Alpaca Agent — ASCOM device bridge

**DRAFT — source-inspected, not newly runtime-validated; included in compiled MCP help. Bridge setup and interface families are documented below; the per-member semantic gap at the end remains explicit.**

Module `indigo_agent_alpaca`; default device `Alpaca Agent`. It publishes eligible native INDIGO devices to ASCOM Alpaca HTTP clients. It is a protocol bridge, not the Imager/Mount/Guider orchestration workflow. Ordinary astronomy through MCP should use those science-agent guides; use this page when an ASCOM application specifically needs an Alpaca device endpoint.

## Bridge function inventory

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

## Setup, identify and connect

1. Prepare native drivers/ports and required hardware settings using [server/device preparation](../server-and-devices.md). Verify device INFO and CONNECTION properties; the bridge cannot invent a driver or repair a serial-port setting.
2. Load Alpaca Agent through the actual server driver item. This makes endpoints reachable on the existing INDIGO server HTTP port, normally7624. UDP discovery receives `alpacadiscovery1` and reports that HTTP port; firewalls must allow the relevant UDP and TCP paths. A discovery-port change alone does not move HTTP.
3. Query `/management/v1/configureddevices` from the Alpaca client. Use its `DeviceType`, `DeviceNumber`, `DeviceName`, `UniqueID`; do not assume camera0 or construct IDs from list order. API type/member paths are lower-case and case-sensitive: e.g. `camera`, `telescope`, `connected`.
4. Configure the ASCOM application's selected device from that result. Use `connected` with `Connected=true`, not the hidden agent CONNECTION. The native bridge waits for observed connection completion, including a paired mount guider. Inspect Alpaca error response and subsequent connected state. Do not concurrently let an Imager/Guider workflow and an unrelated Alpaca application control the same instrument.
5. Query capabilities/current values before a command. Many getters/operations require connected hardware; available properties and capability flags depend on the driver. A registered HTTP member or successful HTTP200 is not proof that the requested operation is implemented or completed.

GET arguments are query parameters; PUT uses URL-encoded form fields, not the MCP Agent's JSON-RPC body. Member-specific PUT parameter names are case-sensitive (`Connected`, `Position`, `Duration`, etc.); GET parameter matching is case-insensitive. ClientID/ClientTransactionID participate in transaction metadata; check returned ErrorNumber/ErrorMessage and the relevant device state, not just HTTP status.

Mappings are allocated/persisted automatically as eligible INFO definitions appear. `AGENT_ALPACA_DEVICES` has up to128 mapping slots; changing mapping text persists settings, but the handler does not directly rewrite every existing live device record. After deliberate setup remapping, safely reload/re-enumerate and verify `configureddevices` before clients reconnect. Avoid reassigning identifiers while an application is operating. Bayer overrides are validated and saved; they affect reported camera Bayer interpretation/offsets, not the sensor's physical mosaic. Discovery-port settings are restarted live; this agent's explicit save helper persists mapping/Bayer properties, not a promise of discovery-port persistence.

## Which devices are published

Actual INFO interface selection publishes CCD→Camera, Dome→Dome, Wheel→FilterWheel, Focuser→Focuser, Rotator→Rotator, powerbox/GPIO→Switch, Mount→Telescope, and lightbox→CoverCalibrator. Branch order matters for composite interfaces; use configureddevices as the definitive published result. Agent-interface devices are deliberately excluded: an Imager Agent is not an Alpaca Camera.

Standalone guider/AO devices are not published as independent Telescopes in current source. A mount guider whose exact native name is `<mount device name> (guider)` is paired into that mount's Telescope and follows its connection; an arbitrary standalone guider is not interchangeable with this naming-based pairing. Weather/SQM ObservingConditions, SafetyMonitor, GPS, joystick and arbitrary unsupported interface types are not supplied as equivalent devices. Do not copy the older README's broader guider/dustcap/device table into a guarantee: current publication is governed by the actual interface branches and available properties.

## Camera operation and image results

For an Alpaca camera, query connected state/capabilities, sensor/frame limits, readout modes, exposure range and cooling controls. Alpaca binning reports1×1; native INDIGO binning is represented through the chosen readout mode rather than assuming separate arbitrary binx/biny changes. Configure frame start/size in the Alpaca camera coordinate convention; the bridge translates to native `CCD_FRAME.LEFT/TOP/WIDTH/HEIGHT`, using cached native binning. Gain/offset/readout mode map to `CCD_GAIN.GAIN`, `CCD_OFFSET.OFFSET` and the selected `CCD_MODE` item where supported.

`startexposure` takes `Duration` in seconds and boolean `Light`. It sets native frame settings/type and `CCD_EXPOSURE.EXPOSURE`, clears the prior ready image and updates acquisition state. Query camerastate/imageready and image transfer only after the new exposure becomes ready. `abortexposure` maps to `CCD_ABORT_EXPOSURE.ABORT_EXPOSURE`; inspect its result and state. Do not invent a guaranteed `stopexposure` implementation merely because it appears in the accepted API registry.

`imagearray`/`imagearrayvariant` are special transfer routes; ImageBytes is supported and avoids expanding large images into JSON arrays. GZip transfer is also handled. Temperature/cooler operations depend on the camera properties; read actual reported ranges/capabilities. Client writes can change hardware state; this bridge does not automatically arbitrate against an Imager process. This draft has not validated any exposure or download.

## Motion, focus, filters and auxiliary results

| Family | Start / input and native mapping | Observe / stop / important interaction |
| --- | --- | --- |
| Telescope | Target RA in hours/declination degrees; slew/sync map through `MOUNT_ON_COORDINATES_SET` and `MOUNT_EQUATORIAL_COORDINATES.RA/DEC`, including native coordinate conversion. Park/unpark/tracking use corresponding native mount controls. | Check capabilities, park status, slewing and final coordinates. `abortslew` maps to `MOUNT_ABORT_MOTION.ABORT_MOTION`. Sync changes alignment/coordinate interpretation; it is not a slew or a safety clearance. MoveAxis is not a general supported substitute. |
| Paired guider | `pulseguide` Direction0/1/2/3=N/S/E/W, Duration milliseconds; maps to `GUIDER_GUIDE_DEC.NORTH/SOUTH` or `GUIDER_GUIDE_RA.EAST/WEST`. | Check canpulseguide/ispulseguiding. This is timed native pulse output, not a Guider Agent calibration/closed-loop guiding process. |
| FilterWheel | `position` uses a zero-based index; native `WHEEL_SLOT.SLOT` is index+1. Names/focusoffsets come from wheel definitions. | Position-1 represents unavailable/moving state; wait for the requested observed slot. No universal wheel abort command is provided. |
| Focuser | `move` targets native focus position/relative movement according to advertised absolute capability; values are native steps. Tempcomp toggles `FOCUSER_MODE.AUTOMATIC/MANUAL`. | Observe ismoving/position/temperature/tempcomp. Use `halt` if supported. A bridge move is not Imager autofocus; temperature-compensation interactions require the focuser's capabilities. |
| Rotator | Relative/absolute/mechanical positions and sync are in degrees; map through native `ROTATOR_POSITION`, raw position where available and `ROTATOR_ON_POSITION_SET`. | Observe ismoving/position/targetposition; `halt` maps to `ROTATOR_ABORT_MOTION.ABORT_MOTION`. Reverse/sync can alter future angle interpretation. |
| Dome | Supported azimuth/altitude in degrees, shutter open/close, park/home/setpark/sync/slaved operations map to the corresponding native dome properties. | Inspect capability flags and slewing/shutter/park results; use abortslew for motion where implemented. Closing a shutter or changing slaving is a physical action with mount interactions, not a harmless metadata change. |
| CoverCalibrator | calibratoron brightness maps to `AUX_LIGHT_SWITCH.ON` and `AUX_LIGHT_INTENSITY.LIGHT_INTENSITY`; off maps to OFF. Cover maps to `AUX_COVER.OPEN/CLOSE` if present. | Observe calibratorstate/coverstate and native property state. Brightness units/range derive from device. `haltcover` is not a universal hardware-cover abort; the adapter's method may be a no-op. |
| Switch | Zero-based channel IDs are generated from power outlets, heaters, USB ports, GPIO outputs, `X_ALPACA_SWITCH_VALUES`, and sensors; readable/writable flags and ranges vary. | Query maxswitch, canwrite, names, min/max/step and current value per channel before setswitch/setswitchvalue. A sensor cannot be treated as an outlet. Name changes can persist device settings. There is no generic “undo all power changes” command. |

These mappings are bridge actions for ASCOM applications. For an agent-first MCP observing workflow, consult [Imager](imager.md), [Mount](mount.md), [Guider](guider.md) and [Auxiliary](auxiliary.md) instead of reproducing a low-level camera/mount sequence here.

## Stop and recovery

Stop the owning Alpaca acquisition/motion through its implemented abort/halt member and verify the observed native/device state. Disconnecting or unloading the bridge is not a universal safe abort. On NotConnected, inspect native CONNECTION and setup before retrying; on InvalidValue/NotImplemented, inspect the actual capability and parameter names rather than repeatedly issuing the same operation. After a lost response, read state before retrying a non-idempotent action. Discovery bind failures/messages and network reachability must be checked separately from a configuration property's Ok state.

To disable the whole bridge, stop clients and active device actions, then unload `indigo_agent_alpaca` through the server. There is no separate HTTP-only CONNECTION toggle. Unload removes routes and discovery, affecting all clients. Device mappings are configuration state, not proof of a present/connected instrument; refresh management discovery after device deletion/reload.

## Source-derived member inventory and remaining semantic gap

The following names are actually handled by command-dispatch branches in the adapter C files, plus the special camera transfer routes. A name here can still return NotConnected, NotImplemented or a capability-dependent error. The front-end member registry accepts additional standard API names; registry presence alone is not implementation. This inventory is not a claim that all ASCOM members work for every device.

**Common:** `name`, `description`, `driverinfo`, `driverversion`, `connected`, `utcdate`, `sitelatitude`, `sitelongitude`, `siteelevation`.

**Camera:** `supportedactions`, `interfaceversion`, `canabortexposure`, `cangetcoolerpower`, `cansetccdtemperature`, `binx`, `biny`, `maxbinx`, `maxbiny`, `camerastate`, `cameraxsize`, `cameraysize`, `startx`, `starty`, `numx`, `numy`, `ccdtemperature`, `heatsinktemperature`, `setccdtemperature`, `cooleron`, `coolerpower`, `electronsperadu`, `fullwellcapacity`, `hasshutter`, `imageready`, `ispulseguiding`, `maxadu`, `pixelsizex`, `pixelsizey`, `sensortype`, `sensorname`, `bayeroffsetx`, `bayeroffsety`, `exposuremin`, `exposuremax`, `exposureresolution`, `lastexposureduration`, `lastexposurestarttime`, `gainmin`, `gainmax`, `gain`, `gains`, `offsetmin`, `offsetmax`, `offset`, `offsets`, `readoutmode`, `readoutmodes`, `startexposure`, `abortexposure`, `imagearray`, `imagearrayvariant`.

**Telescope:** `supportedactions`, `interfaceversion`, `canpulseguide`, `ispulseguiding`, `cansetguiderates`, `guideratedeclination`, `guideraterightascension`, `athome`, `atpark`, `canpark`, `canunpark`, `canslew`, `canslewasync`, `cansync`, `cansettracking`, `siderealtime`, `equatorialsystem`, `declination`, `rightascension`, `altitude`, `azimuth`, `targetdeclination`, `targetrightascension`, `slewing`, `tracking`, `trackingrate`, `trackingrates`, `sideofpier`, `alignmentmode`, `axisrates`, `declinationrate`, `rightascensionrate`, `pulseguide`, `park`, `unpark`, `abortslew`, `slewtotarget`, `slewtotargetasync`, `synctotarget`, `slewtocoordinates`, `slewtocoordinatesasync`, `synctocoordinates`.

**Paired guider:** `supportedactions`, `interfaceversion`, `canpulseguide`, `cansetguiderates`, `ispulseguiding`, `guideratedeclination`, `guideraterightascension`, `atpark`, `trackingrates`, `axisrates`, `declination`, `rightascension`, `declinationrate`, `rightascensionrate`, `pulseguide`.

**FilterWheel:** `supportedactions`, `interfaceversion`, `position`, `names`, `focusoffsets`.

**Focuser:** `supportedactions`, `interfaceversion`, `absolute`, `ismoving`, `maxincrement`, `maxstep`, `position`, `stepsize`, `tempcomp`, `tempcompavailable`, `temperature`, `move`, `halt`.

**Rotator:** `supportedactions`, `interfaceversion`, `canreverse`, `reverse`, `ismoving`, `position`, `targetposition`, `mechanicalposition`, `stepsize`, `sync`, `move`, `moveabsolute`, `movemechanical`, `halt`.

**Dome:** `supportedactions`, `interfaceversion`, `altitude`, `azimuth`, `athome`, `atpark`, `canfindhome`, `canpark`, `cansetaltitude`, `cansetazimuth`, `cansetpark`, `cansetshutter`, `canslave`, `cansyncazimuth`, `slaved`, `shutterstatus`, `slewing`, `abortslew`, `park`, `openshutter`, `closeshutter`, `findhome`, `setpark`, `slewtoaltitude`, `slewtoazimuth`, `synctoazimuth`.

**CoverCalibrator:** `supportedactions`, `interfaceversion`, `brightness`, `maxbrightness`, `calibratorstate`, `coverstate`, `calibratoroff`, `calibratoron`, `closecover`, `opencover`, `haltcover`.

**Switch:** `supportedactions`, `interfaceversion`, `maxswitch`, `canwrite`, `getswitchname`, `getswitchdescription`, `getswitch`, `getswitchvalue`, `minswitchvalue`, `maxswitchvalue`, `switchstep`, `setswitch`, `setswitchvalue`, `setswitchname`.

**Explicit draft gap `alpaca.member-semantics`:** this draft covers bridge configuration, device publication and major operation families, but does not yet supply an individual offline input/result/error/timeout recipe for every member in the inventory, including exact per-member optionality, image-pixel layout/encoding and all native unit conversions. Completing that requires a dedicated adapter-by-adapter semantic pass and reconciliation against the accepted Alpaca API version; older README labels/registration lists are not sufficient evidence. Until that pass, this page must not be labeled a complete ASCOM Alpaca reference or used to invent the omitted member behavior. No external specification/network availability is assumed by the operating recipes above.

Sources: [bridge and member registry](../../../indigo_drivers/agent_alpaca/indigo_agent_alpaca.c), [common mapping](../../../indigo_drivers/agent_alpaca/indigo_alpaca_common.c), [camera](../../../indigo_drivers/agent_alpaca/indigo_alpaca_ccd.c), [mount](../../../indigo_drivers/agent_alpaca/indigo_alpaca_mount.c), [paired guider](../../../indigo_drivers/agent_alpaca/indigo_alpaca_guider.c), [wheel](../../../indigo_drivers/agent_alpaca/indigo_alpaca_wheel.c), [focuser](../../../indigo_drivers/agent_alpaca/indigo_alpaca_focuser.c), [rotator](../../../indigo_drivers/agent_alpaca/indigo_alpaca_rotator.c), [dome](../../../indigo_drivers/agent_alpaca/indigo_alpaca_dome.c), [lightbox](../../../indigo_drivers/agent_alpaca/indigo_alpaca_lightbox.c), [switch](../../../indigo_drivers/agent_alpaca/indigo_alpaca_switch.c), [existing platform notes](../../../indigo_drivers/agent_alpaca/README.md), [property reference](../../PROPERTIES.md).
