# system_alpaca: ASCOM Alpaca client (discovery and proxy) driver

Status: **research and design proposal, 2026-09-24**. No production code exists yet. Continuing past the decision points in section 9 needs the user's decision.

## 0. Goal and scope

- `system_alpaca` discovers ASCOM Alpaca servers on the LAN and creates one INDIGO proxy device for each Alpaca device they expose (camera, telescope, focuser, ...). INDIGO is the Alpaca **client**. It is the opposite direction of `indigo_drivers/agent_alpaca`, which exports INDIGO devices as an Alpaca **server**.
- This is a new driver, not a migration. The `AGENTS.md` / `indigo_drivers/AGENTS.override.md` rules for an original-driver baseline, a reference trace and characterization-first testing therefore have no original to apply to. The equivalent contract is conformance to the Alpaca specification, tested against a deterministic simulator (section 6).
- `system_ascol` is not used as a model: it is off limits and of poor quality. It is cited only for the `system_` naming and entry-point convention.
- Location: `indigo_drivers/system_alpaca`.
- Build registration in root `Makefile`:
  - `DEVELOPED_DRIVERS`: built, excluded from `install` / `uninstall` and so from the distribution (`Makefile.drvs:42-46`).
  - **Temporarily also `EXCLUDED_DRIVERS`.** `Makefile.drvs` collects `system_*` through a wildcard, and a folder without sources would break `make all`. The folder is also excluded because `indigo_driver_metadata` would fail on a missing library. Remove it from `EXCLUDED_DRIVERS` in the step that adds the first sources (plan step S3).
- Language: C (INDIGO 3.0 API). Platforms: Linux, macOS, Windows through the portable `indigo_uni_io` APIs, with no platform-dependent code in the driver (`AGENTS.md:57`).

## 1. Sources

Research was carried out by three subagents on 2026-09-24. `ascom-standards.org` is blocked by the egress proxy in this environment, so the official copies on GitHub were used instead.

| Source | URL |
|---|---|
| OpenAPI device API, including Platform 7 | https://github.com/ASCOMInitiative/ASCOMRemote/blob/main/Swagger/AlpacaDeviceAPI_v1.yaml |
| OpenAPI management API | https://github.com/ASCOMInitiative/ASCOMRemote/blob/main/Swagger/AlpacaManagementAPI_v1.yaml |
| ASCOM Alpaca API Reference rev. 12 (2026-02-23), AlpacaImageBytes.pdf | https://github.com/ASCOMInitiative/ASCOMRemote/tree/main/Documentation |
| ASCOMLibrary (official .NET client: discovery, `RemoteDevice.cs`, `DeviceCapabilities.cs`, `AlpacaErrors.cs`) | https://github.com/ASCOMInitiative/ASCOMLibrary |
| alpyca (official Python client) | https://github.com/ASCOMInitiative/alpyca |
| ConformU | https://github.com/ASCOMInitiative/ConformU |
| OmniSim (ASCOM Alpaca Simulators) | https://github.com/ASCOMInitiative/ASCOM.Alpaca.Simulators |
| AlpycaDevice (Python server template) | https://github.com/ASCOMInitiative/AlpycaDevice |
| alpaca-simulators (Python, FastAPI) | https://github.com/ppp-one/alpaca-simulators |
| ascom-alpaca-rs (Rust client and server, uses OmniSim in its tests) | https://github.com/RReverser/ascom-alpaca-rs |

Not verified because the site was blocked: `ascom-standards.org/newdocs` (relnotes, readall-faq) and the official list of Alpaca devices. The Platform 7 behaviour described below comes from ASCOMLibrary and alpyca, which mirror those pages.

## 2. The Alpaca standard: what the client needs

### 2.1 Discovery (API Reference §5)

- UDP port **32227**, configurable on devices, so it must be configurable on the client too.
- The request is the 16 ASCII bytes `alpacadiscovery1` with no terminator. Byte 15 is the protocol version.
- The response is unicast to the sender's IP and port, with JSON `{"AlpacaPort": <n>}`. The key has been mandatory since rev. 11. The HTTP API lives at the **source IP of the response** and `AlpacaPort`.
- IPv4: send one subnet-directed broadcast per interface. Both reference clients do this, because on Linux `255.255.255.255` only goes out through the default-route interface.
  - Skip `169.254/16`.
  - For loopback, use `127.255.255.255`. With `SO_REUSEPORT`, a unicast to 127.0.0.1 reaches only one of the local servers.
- IPv6 (optional): multicast `ff12::a1:9aca`, one socket per interface. Keep the zone ID for `connect()`, but never put it in the `Host:` header.
- Timing:
  - ASCOMLibrary: 1 poll, 100 ms interval, 1 s window. Its re-discovery uses 2 polls, 100 ms apart, 1 s window.
  - alpyca: 2 queries, 2 s timeout.
  - Recommendation: 2–3 polls, 1–2 s window, repeated periodically (30–60 s) or on demand.
- Duplicates come from:
  - repeated polls;
  - a multi-homed device answering on several NICs;
  - several servers on one host (different ports);
  - the local host answering on both loopback and its LAN address.

  Deduplicate endpoints by `ip:port`, then **devices by `UniqueID`**, compared case-insensitively.
- **Proxy loop.** INDIGO's own `agent_alpaca` answers discovery with `ServerName: "INDIGO-Alpaca Bridge"` and `Manufacturer: "The INDIGO Initiative"` (`indigo_drivers/agent_alpaca/indigo_agent_alpaca.c:279`). Unless it is filtered, INDIGO would import its own devices again, recursively. Filter it by `description` and by local address plus own port.
- Always allow a manual `host:port` entry as well, because discovery is only a "should" for devices.

### 2.2 Management API

All requests are GET. Responses have `Value`, `ClientTransactionID` and `ServerTransactionID`, but **no** `ErrorNumber` or `ErrorMessage`.

| Path | Value |
|---|---|
| `/management/apiversions` | `[1]` |
| `/management/v1/description` | `ServerName`, `Manufacturer`, `ManufacturerVersion`, `Location` |
| `/management/v1/configureddevices` | `[{DeviceName, DeviceType (PascalCase, e.g. "CoverCalibrator"), DeviceNumber (uint32), UniqueID}]` |
| `/setup/v1/{type}/{n}/setup` | HTML configuration page. Useful as an informational URL in INDIGO. |

- `DeviceType` must be lower-cased when building URLs.
- Device numbers are not guaranteed to be contiguous.
- Minor non-conformance in INDIGO's own server: `description` returns `ServerVersion` and `ManufacturerURL` instead of `ManufacturerVersion` and `Location`.

### 2.3 Device API contract

- URL: `/api/v1/{device_type}/{device_number}/{method}`. **All path elements are lower case.**
- **GET** is used for anything that does not change state. Parameters go in the query string, where keys are **case-insensitive**. This includes parameterised getters: `axisrates?Axis=`, `canmoveaxis`, `destinationsideofpier`, the Switch `get*?Id=` calls, and `sensordescription?SensorName=`.
- **PUT** is used for anything that changes state. The body is `application/x-www-form-urlencoded`, and its keys are **case-sensitive**: `BinX`, not `binx`. ConformU expects a 400 response to wrong casing.
  - The YAML intro says "not case sensitive", but the API Reference and ConformU override it.
  - The client always sends the exact casing from the spec.
- `ClientID`: uint32, random value at startup, 0 is reserved.
- `ClientTransactionID`: starts at 1 and increments, 0 is reserved. The client checks the echo in the response.
- `ServerTransactionID`: used for logging only.
- Formatting:
  - Invariant locale: `.` as decimal separator, no thousands separators. `printf` must not depend on `LC_NUMERIC`.
  - Booleans are sent as `true` / `false`.
  - All form values are URL-encoded.
- Response envelope (HTTP 200): `{"Value":…, "ClientTransactionID":n, "ServerTransactionID":n, "ErrorNumber":0, "ErrorMessage":""}`.
  - Check `ErrorNumber` **before** reading `Value`.
  - `Value` may be missing or `null` on error.
  - ASCOMLibrary maps a missing `Value` on success to driver error 4095.
  - Tolerate missing optional keys and unknown keys.
- HTTP status:
  - 200: the operation was attempted, with the result in `ErrorNumber`.
  - 400: request not understood (bad path, wrong device number, bad parameter or its casing). Plain-text body.
  - 401 / 403 / 3xx: transport or authentication.
  - 500: fatal device error.
  - An invalid parameter value produces either 400, or 200 with 0x401. The client must handle both.
- Error codes:

  | Code | Name |
  |---|---|
  | 0x400 | NotImplemented |
  | 0x401 | InvalidValue |
  | 0x402 | ValueNotSet |
  | 0x407 | NotConnected |
  | 0x408 | InvalidWhileParked |
  | 0x409 | InvalidWhileSlaved |
  | 0x40B | InvalidOperation |
  | 0x40C | ActionNotImplemented |
  | 0x40E | OperationCancelled (P7) |
  | 0x4FF | UnspecifiedError |
  | 0x500–0xFFF | driver-specific |

- Dates:
  - Telescope `UTCDate`: ISO 8601 with a mandatory `Z`.
  - Camera `LastExposureStartTime`: FITS format without `Z`.
- Headers: `Content-Type` may carry `; charset=…`, so compare by prefix. Send `Accept: application/imagebytes, application/json` only on `imagearray`.
- Authentication and HTTPS are not in the spec (`security: []`). ASCOMLibrary nevertheless supports HTTP Basic and HTTPS. Planned as an optional per-server extension (section 5.10).
- Keep-alive: not specified by the documents. The official clients reuse connections. Tolerate `Connection: close`, as ESP8266/ESP32 servers close after every response. Retry once only on a stale socket, and never blindly retry a non-idempotent PUT.
- Timeouts in ASCOMLibrary:

  | Timeout | Value | Used for |
  |---|---|---|
  | Connection establishment | 5 s | `connect`, `connected`, `connecting`, `disconnect`, `interfaceversion` |
  | Standard | 10 s | everything else |
  | Long | 100 s | `imagearray`, slews, park, findhome, move, pulseguide, action / command* |

  Retries: 1 retry after a socket error, with a 100 ms delay.
- Concurrency is not specified. Plan: at most one request in flight per device, serialised per server connection. A long `imagearray` download uses its own connection so that status polling keeps running.

### 2.4 Common members

`action`, `commandblind`, `commandbool`, `commandstring`, `connected` (GET/PUT), `description`, `driverinfo`, `driverversion`, `interfaceversion`, `name`, `supportedactions`.

Platform 7 adds `connect`, `disconnect`, `connecting` and `devicestate`.

- **Connecting on P7:**
  1. `PUT connect`.
  2. Poll `connecting` until it is false.
  3. Verify `connected`.
- **Connecting on older devices:** `PUT connected Connected=true`. The call can block, so it runs outside bus callbacks.
- **`devicestate`:** one call returns a snapshot of the operational properties, e.g. for a Telescope: Altitude, AtHome, AtPark, Azimuth, Declination, IsPulseGuiding, RightAscension, SideOfPier, SiderealTime, Slewing, Tracking, UTCDate. Every entry is optional. **It is the main polling call on P7 devices.** On older devices, or for missing entries, the client falls back to individual GETs.
- **`Connected` is device-wide, not per client.** The spec does not say what one client's disconnect does to other clients (for example NINA sharing the same server). Proposal: an INDIGO disconnect does not send `Connected=false` by default. This is a user decision (section 9).

### 2.5 Interface versions and capability detection

| Type | Platform 6 | Platform 7 | Most important additions |
|---|---|---|---|
| Camera | 3 | 4 | V2: gain, readout modes, sensor type, Bayer offsets, PercentCompleted. V3: offset, exposure min/max, SubExposureDuration |
| CoverCalibrator | 1 | 2 | V2: CalibratorChanging, CoverMoving |
| Dome | 2 | 3 | |
| FilterWheel | 2 | 3 | |
| Focuser | 3 | 4 | |
| ObservingConditions | 1 | 2 | |
| Rotator | 3 | 4 | V3: MechanicalPosition, MoveMechanical, Sync |
| SafetyMonitor | 1 | 3 | |
| Switch | 2 | 3 | V2: analog values. V3: CanAsync, SetAsync, SetAsyncValue, StateChangeComplete, CancelAsync (**missing from the YAML**) |
| Telescope | 3 | 4 | V2: MoveAxis, AxisRates, SideOfPier, tracking rates, AltAz |

Detection algorithm:

1. Read `interfaceversion`.
2. Gate every P7-only call on the interface version.
3. Read the `Can*` flags.
4. Probe optional members once and cache the result. Both 0x400 **and** HTTP 400 mean "not supported".

### 2.6 Member reference by type

Parameter names are shown with their exact casing. **P** = PUT, otherwise GET.

- **Camera**
  - Getters: `binx`/`biny` (P `BinX`/`BinY`), `camerastate` (0 Idle, 1 Waiting, 2 Exposing, 3 Reading, 4 Download, 5 Error), `cameraxsize`/`cameraysize`, `canabortexposure`, `canasymmetricbin`, `cangetcoolerpower`, `canpulseguide`, `cansetccdtemperature`, `canstopexposure`, `canfastreadout`, `ccdtemperature`, `cooleron` (P `CoolerOn`), `coolerpower`, `electronsperadu`, `fullwellcapacity`, `exposuremin`/`max`/`resolution`, `fastreadout` (P `FastReadout`), `gain` (P `Gain`; index into `gains` **or** a value in `gainmin..gainmax`), `offset`/`offsets`/`offsetmin`/`offsetmax` (same dual mode), `hasshutter`, `heatsinktemperature`, `imageready`, `ispulseguiding`, `lastexposureduration`, `lastexposurestarttime`, `maxadu`, `maxbinx`/`maxbiny`, `numx`/`numy`/`startx`/`starty` (P, **binned** pixels), `percentcompleted`, `pixelsizex`/`y`, `readoutmode` (P `ReadoutMode`) / `readoutmodes`, `sensorname`, `sensortype` (0 Mono, 1 Color, 2 RGGB, 3 CMYG, 4 CMYG2, 5 LRGB), `bayeroffsetx`/`y`, `setccdtemperature` (P `SetCCDTemperature`), `subexposureduration`.
  - Actions:
    - `startexposure` (P `Duration` s double, `Light` bool). Completion is signalled by `imageready` / `camerastate`.
    - `abortexposure` discards the image.
    - `stopexposure` keeps the image.
    - `pulseguide` (P `Direction` 0 N, 1 S, 2 E, 3 W; `Duration` **ms**). Completion is signalled by `ispulseguiding`.
  - `imagearrayvariant` must return 0x400 on Alpaca devices.
- **CoverCalibrator:** `brightness`, `maxbrightness`, `calibratorstate` (0 NotPresent, 1 Off, 2 NotReady, 3 Ready, 4 Unknown, 5 Error), `coverstate` (0 NotPresent, 1 Closed, 2 Moving, 3 Open, 4 Unknown, 5 Error), `calibratorchanging` and `covermoving` (V2); P `calibratoron` `Brightness`, `calibratoroff`, `opencover`, `closecover`, `haltcover`.
- **Dome:** `altitude`, `azimuth`, `athome`, `atpark`, `slewing`, `slaved` (P `Slaved`), `can*` (findhome, park, setaltitude, setazimuth, setpark, setshutter, slave, syncazimuth), `shutterstatus` (0 Open, 1 Closed, 2 Opening, 3 Closing, 4 Error); P `abortslew`, `openshutter`, `closeshutter`, `findhome`, `park`, `setpark`, `slewtoaltitude` `Altitude`, `slewtoazimuth` `Azimuth`, `synctoazimuth` `Azimuth`.
- **FilterWheel:** `focusoffsets`, `names`, `position` (0-based, **−1 while moving**; P `Position`).
- **Focuser:** `absolute`, `ismoving`, `maxincrement`, `maxstep`, `position` (NotImplemented on relative focusers), `stepsize`, `tempcomp` (P `TempComp`), `tempcompavailable`, `temperature`; P `halt`, `move` `Position` (absolute target, or signed relative steps).
- **ObservingConditions:** `averageperiod` (P `AveragePeriod`, hours), `cloudcover` (%; the source conflicts with a 0–1 scale), `dewpoint`, `humidity`, `pressure` (hPa), `rainrate` (mm/h), `skybrightness` (lux), `skyquality` (mag/arcsec²), `skytemperature`, `temperature`, `starfwhm`, `winddirection`, `windgust`, `windspeed` (m/s); P `refresh`; `sensordescription?SensorName=` and `timesincelastupdate?SensorName=`. Every sensor is individually optional.
- **Rotator:** `canreverse`, `ismoving`, `mechanicalposition`, `position`, `reverse` (P `Reverse`), `stepsize`, `targetposition`; P `halt`, `move` (relative) `Position`, `moveabsolute` `Position`, `movemechanical` `Position`, `sync` `Position`.
- **SafetyMonitor:** `issafe`.
- **Switch:** `maxswitch`; with `Id`: `canwrite`, `getswitch`, `getswitchdescription`, `getswitchname`, `getswitchvalue`, `minswitchvalue`, `maxswitchvalue`, `switchstep`, and (V3) `canasync` and `statechangecomplete`; P `setswitch` `Id`,`State`, `setswitchvalue` `Id`,`Value`, `setswitchname` `Id`,`Name`, and (V3) `setasync`, `setasyncvalue`, `cancelasync`.
- **Telescope**
  - Getters: `alignmentmode`, `altitude`, `azimuth`, `aperturearea`, `aperturediameter`, `focallength`, `athome`, `atpark`, `can*` (findhome, park, pulseguide, setpark, settracking, slew, slewasync, sync, unpark, setdeclinationrate, setguiderates, setpierside, setrightascensionrate, slewaltaz, slewaltazasync, syncaltaz), `canmoveaxis?Axis=`, `declination` (deg), `rightascension` (**hours**), `declinationrate`/`rightascensionrate` (P, offset rates), `doesrefraction` (P), `equatorialsystem` (0 Other, 1 JNow, 2 J2000, 3 J2050, 4 B1950), `guideratedeclination`/`guideraterightascension` (P, deg/s), `ispulseguiding`, `sideofpier` (P forces a flip; 0 East/normal, 1 West/through the pole, −1 Unknown), `siderealtime`, `siteelevation`/`sitelatitude`/`sitelongitude` (P, longitude **+E**), `slewing`, `slewsettletime`, `targetdeclination`/`targetrightascension` (P), `tracking` (P), `trackingrate` (P; 0 Sidereal, 1 Lunar, 2 Solar, 3 King) / `trackingrates`, `utcdate` (P).
  - Methods: P `abortslew`, `axisrates?Axis=` (list of `{Minimum,Maximum}` deg/s), `destinationsideofpier?RightAscension=&Declination=`, P `findhome`, P `moveaxis` `Axis`,`Rate` (signed deg/s; 0 stops), P `park`/`unpark`/`setpark`, P `pulseguide` `Direction`,`Duration` (ms), P `slewtocoordinatesasync` `RightAscension`,`Declination`, `slewtoaltazasync`, `slewtotargetasync`, P `synctocoordinates`/`synctoaltaz`/`synctotarget`.
  - Prefer the `*async` slew variants. The synchronous ones are deprecated.

### 2.7 Asynchronous semantics

- An error from the starting call means the operation did not start.
- Completion is signalled **only** by the completion property:

  | Operation | Completion signal |
  |---|---|
  | Telescope slews | `Slewing` |
  | Park, find home | `AtPark` / `AtHome` |
  | Focuser, rotator moves | `IsMoving` |
  | Dome shutter | `ShutterStatus` |
  | Cover | `CoverState` / `CoverMoving` |
  | Calibrator | `CalibratorState` / `CalibratorChanging` |
  | Filter wheel | `Position ≠ −1` |
  | Exposure | `ImageReady` |
  | Pulse guide | `IsPulseGuiding` |
  | Switch async set | `StateChangeComplete(Id)` |
  | P7 connect / disconnect | `Connecting` |

- An error while reading the completion property means the operation failed after it started.
- The start call may return with the operation already complete, and that counts as success.
- Completion must not be decided by comparing positions.
- Old drivers block inside "async" methods such as Park or FindHome. They need the long timeout, and INDIGO must not block the bus callback. This fits the INDIGO handler + `_finalizer` pattern (`AGENTS.md:91-92`).

### 2.8 Image transfer

- **JSON `imagearray`:** `{"Type":2,"Rank":2|3,"Value":[[…]]}`.
  - Orientation: `Value[x][y]`, outer index = column (NumX), inner index = row (NumY), origin at the top left. This is column-major with respect to the image.
  - Colour: `Value[x][y][plane]`, with R, G, B planes.
  - For INDIGO / FITS row-major, a **transpose** is needed: `pix[y*NumX+x] = Value[x][y]`.
  - Performance is very poor. Benchmark from the spec for 6000×4000 over Wi‑Fi: JSON 25.6 s, ImageBytes 2.3 s (Int32) and 1.1 s (UInt16). **JSON is only an emergency fallback.**
- **ImageBytes** (`Accept: application/imagebytes`):
  - A little-endian 44-byte header, all int32 fields:

    | Offset | Field |
    |---|---|
    | 0 | MetadataVersion = 1 |
    | 4 | ErrorNumber |
    | 8 | ClientTransactionID |
    | 12 | ServerTransactionID |
    | 16 | **DataStart** (use it; do not hard-code 44) |
    | 20 | ImageElementType |
    | 24 | TransmissionElementType |
    | 28 | Rank |
    | 32 | Dimension1 = NumX |
    | 36 | Dimension2 = NumY |
    | 40 | Dimension3 = planes |

  - Element types: 0 Unknown, 1 Int16, 2 Int32, 3 Double, 4 Single, 5 UInt64, 6 Byte, 7 Int64, 8 UInt16, 9 UInt32.
  - Servers narrow Int32 data on the wire to Byte, UInt16 or Int16 when the values fit. The minimal support set is ImageElementType 2 with TransmissionElementType 2, 1, 8 and 6.
  - Data order is the same as JSON (`x` outer, `y` inner, planes innermost).
  - On error (`ErrorNumber ≠ 0`), the bytes from DataStart onwards are a UTF-8 message.
  - `Content-Type: application/json` in the reply means the server fell back to JSON.
- `Content-Encoding: gzip` can be decompressed with `indigo_uni_decompress()` (`indigo_libs/indigo_uni_io.c:2215`).
- The Base64Handoff variant is superseded. Not implemented.

## 3. Audit of INDIGO infrastructure

### 3.1 Generator (`indigo_tools/indigo_generator.c`, `indigo_docs/DRIVER_GENERATOR_MIGRATION.md`)

- **Several device classes in one driver: yes.** Examples are `ccd_simulator.driver` with 9 blocks and `mount_lx200.driver` with 4.
- **There is no `system` class.** `parse_device_block` accepts only `ccd`, `wheel`, `focuser`, `mount`, `guider`, `rotator`, `dome`, `gps`, `ao`, `aux`, `polaralign` (`indigo_generator.c:806-807`). Several names are derived from the type of the **first** device:
  - `DRIVER_NAME` (`:1459`)
  - the entry point (`:2416`)
  - the output file names (`:3446-3457`)

  `Makefile.drv:39`, on the other hand, derives `indigo_system_alpaca` from the folder name, and the loader resolves the entry symbol from the library name (`indigo_libs/indigo_client.c:229-243`). A generated driver in `system_alpaca/` would therefore not load.
- **A dynamic number and mix of device classes from network discovery: not expressible.**
  - Connection types are only `serial`, `libusb`, `hid`, `sdk` and `virtual` (`:1080-1170`).
  - `sdk` is driven by libusb events (`:2177-2245`).
  - Virtual and serial drivers attach exactly one instance of each block at INIT (`:2464-2474`).
  - The only precedent for dynamic attach inside a generated driver is a hand-written GPS in `mount_nexstar.driver:162-179, 693-728`: its own template, `attach`, `change_property` and `detach`. For Alpaca, that would mean every device is hand-written, and the generator would own nothing but an entry point with the wrong name.
- **Properties that depend on runtime capabilities: partial.**
  - `hidden = <expr>` works only for inherited properties (`:1834-1843`).
  - Everything else (`->hidden`, `->count`, `indigo_resize_property`) has to happen in hand-written code before the property is defined.
- **Conclusion:** a generator-based `system_alpaca` needs generator changes, which require the user's explicit prior approval (`AGENTS.md:76`). The minimal extension (section 9, decision D1) would be:
  1. a driver-level override of the name, entry point and file names (for example `entry_name` or a `system` class);
  2. a `custom { discover { … } }` / network connection type, which creates 0..N instances of selected device blocks from a code block run on the driver queue. It would reuse the `devices[]`, `attach_if` and `name_value` mechanics of the SDK hot-plug path.

### 3.2 Network, JSON and helpers

| Need | Status in INDIGO | Consequence |
|---|---|---|
| TCP client | `indigo_uni_open_client_socket` / `indigo_uni_open_url` (`indigo_uni_io.h:205,238`). `connect()` is blocking **with no connect timeout** (`indigo_uni_io.c:1054`). | An unreachable host blocks the queue for the OS TCP timeout. A connect timeout is needed in `indigo_uni_io` (library change). |
| HTTP client | Only `mount_starbook.driver:68-96`: GET, HTTP/1.0, `Connection: close`, 4 KB buffer, no Content-Length or chunked handling, no PUT | A small HTTP/1.1 client is needed: GET/PUT, Content-Length, chunked, keep-alive, binary body |
| UDP discovery with several responders | `indigo_perform_active_discovery` (`indigo_uni_io.c:947-1036`) returns **only the first reply** and sends to a single address. There is no interface / broadcast-address enumeration. `focuser_askar.driver:324-430` uses `getifaddrs` and `#if` in the driver, which breaks `AGENTS.md:57`. | A new portable helper in `indigo_uni_io` is needed: broadcast on every interface plus a callback for each reply |
| DNS-SD | `indigo_service_discovery` is hard-coded to `_indigo._tcp` | Not usable. Alpaca uses UDP discovery anyway. |
| **General JSON parser** | `indigo_json.c` is only a streaming parser for the INDIGO wire protocol (`indigo_json.c:110-330`). jsmn is bundled only in `focuser_primaluce/jsmn.h`. There is no cJSON. | **User requirement: the general JSON parser will live in `indigo_libs/indigo_json.c` / `indigo_json.h`** (section 5.3) |
| JSON escape | `indigo_json_escape_b` (`indigo_json.h:55`) | Usable |
| URL/form encoding | Not available | New helper, next to the HTTP client |
| gzip | `indigo_uni_decompress` | Usable |
| base64 | `indigo_base64.h` | Only for optional Basic auth |

### 3.3 `agent_alpaca` (Alpaca server in INDIGO) as a reference

- **Type mapping** (`indigo_agent_alpaca.c:600-622`):

  | INDIGO | Alpaca |
  |---|---|
  | CCD | Camera |
  | DOME | Dome |
  | WHEEL | FilterWheel |
  | FOCUSER | Focuser |
  | ROTATOR | Rotator |
  | AUX_POWERBOX / AUX_GPIO | Switch |
  | MOUNT / GUIDER / AO | Telescope |
  | AUX_LIGHTBOX | CoverCalibrator |

  ObservingConditions and SafetyMonitor are missing.
- **Mappings between INDIGO properties and Alpaca members:** `indigo_alpaca_{ccd,mount,guider,focuser,wheel,rotator,dome,lightbox,switch}.c`. They are the inverse of what `system_alpaca` needs. Useful semantics:
  - Wheel slots are 0-based in Alpaca and 1-based in INDIGO. The Alpaca position is −1 while the wheel moves.
  - Camera Start/Num values are in binned units.
  - `maxadu = 2^bits`.
  - Readout modes map to `CCD_MODE`.
  - The Switch maps to `AUX_POWER_OUTLET`, `AUX_HEATER_OUTLET`, `AUX_USB_PORT` and `AUX_GPIO_*`.
- **What must not be copied:**
  - the blocking `indigo_alpaca_wait_for_*` helpers;
  - the non-static global symbols (`indigo_alpaca_error_string` and others), which would collide;
  - the RGB24 channel order and the row flip (section 8, defects AGENT-1 and AGENT-2).
- Reusable as a reference: the constants (`alpacadiscovery1`, `{"AlpacaPort":…}`), the error enum and the ImageBytes metadata struct (`indigo_alpaca_common.h:39-49, 245-270`).

### 3.4 Dynamic device creation: patterns to follow

- **Generated libusb / SDK hot-plug** (`indigo_generator.c:2134-2593`) is the canonical structure:
  - a per-driver `driver_queue` (`indigo_queue_create`) and a `devices[MAX_DEVICES]` array;
  - private data shared by the devices of one physical device, with `master_device` pointing at the first of them;
  - `indigo_attach_device` with rollback;
  - removal in reverse order, with the shared data freed only once;
  - a SHUTDOWN guard (`verify_devices_disconnected`) and draining of the queue.

  `system_alpaca` mirrors this structure by hand, with discovery results in place of USB events.
- **`ccd_ptp`** (`indigo_ccd_ptp.c:211-262, 904-945`): a secondary device attached on connect and detached on disconnect. This is the model for guider sub-devices.

### 3.5 CCD image path

- `indigo_process_image(device, data, w, h, bpp, little_endian, byte_order_rgb, keywords, streaming)` (`indigo_ccd_driver.h:637`).
- Buffer: pixels start at `data + FITS_HEADER_SIZE`. Allocate the size plus a reserve for the `BAYERPAT` appendix.
- Pixel layout:
  - row-major, **top-down**;
  - bpp 8 or 16 (mono), 24 or 48 (interleaved RGB);
  - Bayer is passed as a `BAYERPAT` keyword.
- For the countdown use `indigo_ccd_exposure_setup()` (`AGENTS.md:104`).

### 3.6 Build and repository integration

- `Makefile.drv` builds `indigo_system_alpaca.{a,so}` from `*.c` and makes `indigo_system_alpaca_main.c` the executable. An optional `Makefile.inc` adds flags, e.g. `LDFLAGS += -lz` if needed.
- Required files: `indigo_system_alpaca.c`, `indigo_system_alpaca.h` (declares `indigo_result indigo_system_alpaca(indigo_driver_action, indigo_driver_info *)`), `indigo_system_alpaca_main.c`, `README.md` (a user-facing document; changes need approval), and this `REFACTOR.md`.
- Xcode: a group under `indigo_drivers` (`project.pbxproj:11212`). Following the `ccd_pentax` pattern, the `.c` goes into the Sources phases of the `indigo` and `indigo_m1` targets and the `.h` into both Headers phases. README, REFACTOR and `_main.c` are group references only.
- Optional for a developed driver (per the `ccd_pentax` precedent): Windows `.vcxproj` / `indigo_windows.sln`, a row in `MIGRATION_STATUS.md`, and the `STATIC_DRIVERS` table in `indigo_server.c`.
- Mandatory: `indigo_docs/PROPERTIES.md`, a `### system_alpaca` section for every `X_` property (`AGENTS.md:24-34`).

## 4. Simulators and test options

### 4.1 OmniSim (ASCOM Alpaca Simulators): measured on linux-x64

- MIT license. .NET 8, self-contained (about 120 MB unpacked; needs libicu).
- Latest release **v0.5.0** (pre-release), with assets for linux x64, arm64 and armhf, macOS x64 and arm64, and Windows. .NET 8 LTS support ends in 11/2026.
- Covers **all 10 types** at Platform 7 (`connect`, `connecting` and `devicestate` work). Default port 32323. Answers discovery on UDP 32227.
- The interface version can be lowered per device over REST (`PUT /simulator/v1/focuser/0/interfaceversion`). This lets us test the pre-P7 fallback.
- Headless on Linux; startup takes about 1 s warm and 3.4 s cold.
- **Effectively single-instance per host** (global mutex plus a named pipe). A second launch forwards its arguments to the first instance and exits. Parallel CI runs therefore need a lock or a separate container.
- Configuration:
  - XML profiles in `$HOME/.config/ascom/alpaca/...`. If `.config` does not exist, it writes into the working directory.
  - `--urls=http://127.0.0.1:<port>` sets the address.
  - The REST settings API (`/simulator/v1/{type}/{n}/...`) exists only for Dome, FilterWheel, Focuser, Rotator and SafetyMonitor. Reset works for all types.
- Strict protocol mode by default:
  - a wrong parameter, path casing or device number returns 400;
  - ASCOM errors return 200 with `ErrorNumber`.
- Camera: 800×600 image. ImageBytes works (480,044 B). A 0.1 s exposure is ready in about 0.16 s.
- **Limitations:**
  - **No fault injection:** it cannot produce malformed JSON, HTTP errors, stalls, resets, a bad transaction ID or bad discovery replies.
  - **Not deterministic:** motion and exposures run on real-time timers.
  - ConformU reports 9 issues against its own focuser (timeouts).

### 4.2 ConformU v4.5.0 (GPL-3.0, run only as an external tool)

- CLI: `conformu conformance <uri>` and `conformu alpacaprotocol <uri>`, with `-r` writing JSON results. The exit code is the number of errors plus issues.
- Measured against OmniSim:
  - `alpacaprotocol` on the focuser: 6 s.
  - Full `conformance`: focuser 117 s, switch 479 s.
- It tests **servers only**, so it cannot test our client directly. Round trip: `OmniSim → system_alpaca → INDIGO bus → agent_alpaca → ConformU`, evaluated **differentially** against a baseline run directly on OmniSim. The limits of `agent_alpaca` apply:
  - no SafetyMonitor or ObservingConditions;
  - interface V1–V3 only, so no P7;
  - no MoveAxis.

### 4.3 Other options

| Candidate | Suitability |
|---|---|
| alpyca (MIT, client) | A reference client for checking our simulator and fixtures. Not a server. |
| AlpycaDevice (MIT, Python/Falcon) | Rotator only; the other types are templates. Single-threaded. Not on PyPI. |
| alpaca-simulators (MIT, FastAPI) | All 10 types, but **no UDP discovery**, and the camera queries Gaia over TAP live, so it is not hermetic. |
| INDIGO `agent_alpaca` + INDIGO simulators | A second in-tree server written in C (Camera with ImageBytes, Telescope, Focuser, Wheel, Rotator, Dome, Switch, CoverCalibrator). It is not independent evidence of spec compliance, because both sides share INDIGO code. |
| ASCOM Remote | Windows only. |
| seestar_alp, AlpacaHub, goalpaca-devices, AlpacaBridge | Bridges to real hardware, not simulators. |
| Hardware with native Alpaca | Pegasus Falcon Rotator v2, PPBADV Gen3 / Unity, Optec (manufacturer claims, unverified) |

### 4.4 Conclusion

A dedicated **deterministic C simulator** is necessary. It is the only way to cover the fault classes that `indigo_drivers/AGENTS.override.md:34` requires for network and serial drivers, with deterministic timing. OmniSim serves as an independent semantic reference in an opt-in tier.

## 5. Proposed approach

### 5.1 Hand-written driver vs. generator

**Recommendation: a hand-written INDIGO 3.0 driver**, with the structure of a generated hot-plug driver (section 3.4).

- The generator cannot express the `system_` name, network discovery, or a runtime mix of classes and instances (section 3.1).
- Extending the generator is possible (decision D1), but it would be the largest change in this task: a new connection type plus the naming override, with an impact on the parser and the lifecycle of every driver. It would still leave most of the logic in `code` blocks, because the properties are created from capabilities at runtime.
- A later migration onto a generator extension is not ruled out, provided the driver keeps generator conventions:
  - `system_alpaca_open` / `system_alpaca_close` with the transactional contract;
  - `_finalizer` naming;
  - an `on_change`-style structure for handlers.

### 5.2 Driver architecture

- **Always-attached device "Alpaca"**, the bridge (INDIGO_INTERFACE_AUX). Planned `X_` properties (each goes into `PROPERTIES.md`):
  - `X_ALPACA_DISCOVERY`: switch `ENABLED` / `DISABLED`, plus `X_ALPACA_DISCOVERY_SETTINGS` with UDP port (32227), interval (s), timeout (ms) and number of polls.
  - `X_ALPACA_DISCOVER`: one-shot "scan now".
  - `X_ALPACA_SERVERS`: manual servers (`host:port`), with add and remove.
  - `X_ALPACA_DEVICES`: list of discovered devices (name, type, server, UniqueID, state), plus a selection of which are proxied (decision D4).
- **Server record** (`alpaca_server`), one per `ip:port`:
  - HTTP connection(s), a mutex, `ClientID`, the `ClientTransactionID` counter;
  - the reusable response buffer in private data (`AGENTS.md:60`);
  - the `description` data;
  - the INDIGO-bridge flag.
- **Proxy device**, one `indigo_device` per `configureddevices` item. The key is the `UniqueID`, so the device keeps its identity when its IP changes.
  - Private data points to the server record and holds the `DeviceType`, `DeviceNumber`, `InterfaceVersion`, capability cache (`Can*` plus probed optional members) and polling state.
  - `master_device` is the first device from the same server.
  - Device name: `"<DeviceName> @ <ServerName>"`, made unique with `indigo_make_name_unique`.
- **Type mapping:**

  | Alpaca | INDIGO device | Notes |
  |---|---|---|
  | Camera | CCD | Plus a Guider sub-device when `CanPulseGuide` |
  | Telescope | Mount | Plus a Guider sub-device when `CanPulseGuide` |
  | Focuser | Focuser | absolute / relative per `Absolute` |
  | FilterWheel | Wheel | 0-based ↔ 1-based slot |
  | Rotator | Rotator | |
  | Dome | Dome | roll-off roof when `CanSetAzimuth = false` |
  | CoverCalibrator | AUX_LIGHTBOX (+ `AUX_COVER`) | Absence of cover or calibrator per `*State == NotPresent` |
  | Switch | AUX_GPIO / POWERBOX | Boolean outputs map to `AUX_GPIO_OUTLETS`. Analog values need a `X_ALPACA_SWITCH_VALUES` number property. Read-only ones map to `AUX_GPIO_SENSORS`. |
  | ObservingConditions | AUX_WEATHER | `AUX_WEATHER` items plus `X_` items for sensors INDIGO has no item for (cloud cover, rain rate, sky quality, star FWHM, wind gust) |
  | SafetyMonitor | AUX | INDIGO has no standard property. Proposal: `X_ALPACA_SAFETY` (`SAFE` / `UNSAFE`, read-only); precedent `X_CONDITIONS_SAFETY` in `dome_beaver`. |

- **Lifecycle:**
  - INIT: attach the bridge device and start discovery on the driver queue.
  - Discovery:
    1. new endpoint;
    2. `apiversions`, `description`, `configureddevices`;
    3. filter out INDIGO bridges;
    4. deduplicate by UniqueID;
    5. attach.
  - Disappearance (the endpoint stops responding over N cycles, or the device drops out of `configureddevices`): disconnect, then detach.
  - SHUTDOWN: guard on connected devices, drain the queue, detach in reverse order.
  - CONNECTION:
    1. On P7: `connect` plus a finalizer on `connecting`. Otherwise: `PUT connected` on the queue, with the establish timeout.
    2. Read the capabilities and build the properties before they are defined.
    3. Start polling.
- **Polling:**
  - one timer per device;
  - on P7, one `devicestate` call, with individual GETs for the missing entries;
  - on older interfaces, a minimal set of GETs per class;
  - rate 0.5–2 Hz, with a faster rhythm only during active operations.
- **Operations:** handler + `_finalizer` (`AGENTS.md:91-95`). The handler sends the starting PUT and publishes BUSY. The finalizer polls the completion property within a bounded time, then publishes OK or ALERT. Abort (`abortslew`, `halt`, `abortexposure`, `haltcover`) goes through the queue.
- **Errors:**

  | Condition | Handling |
  |---|---|
  | Transport error, timeout, HTTP 5xx | ALERT, with retries per section 2.3 |
  | 0x400 | "unsupported"; cache the result and hide the item or property |
  | 0x401 | ALERT plus a message |
  | 0x407 NotConnected | the device is connected again, or disconnected |
  | 0x408 | ALERT "parked" |

### 5.3 Library changes (outside the driver folder)

Each is a separate commit with its own tests in `indigo_test` (unit and fixture tests).

1. **General JSON parser in `indigo_libs/indigo_json.c` / `indigo_json.h`** (user requirement). Proposed API (DOM over a single allocation, portable C):
   - `indigo_json_value *indigo_json_parse_value(const char *text, long length, char *error, int error_size);`
   - `void indigo_json_free_value(indigo_json_value *value);`
   - accessors:
     - `indigo_json_get(object, key)`, case-insensitive per ASCOMLibrary, plus a case-sensitive variant;
     - `indigo_json_array_size` / `indigo_json_array_get`;
     - `indigo_json_is_null`;
     - `indigo_json_get_bool` / `_int` / `_double` / `_string` returning a `bool` success flag.
   - Numbers are parsed locale-independently. Strings are unescaped to UTF-8, including `\uXXXX` surrogates.
   - A depth limit and a size limit.
   - A fast path for numeric 2D/3D arrays (the JSON `imagearray` fallback): `indigo_json_parse_int_array()` straight into a target buffer, with no DOM.
   - Must not collide with the existing `indigo_json_parse(device, client)` for the wire protocol. The new names use a `_value` suffix.
   - Coverage: unit tests on fixtures (OmniSim replies, malformed JSON, BOM, UTF-8, deep nesting).
2. **UDP discovery with several responders:** `indigo_uni_discover(port, payload, polls, interval_ms, timeout_ms, callback, context)`. It broadcasts on every IPv4 interface (directed broadcast plus loopback) and calls the callback for every reply, with the source IP and body. OS-specific code stays in `indigo_uni_io.c`: `getifaddrs` on POSIX, `GetAdaptersAddresses` / `SIO_GET_INTERFACE_LIST` on Windows. IPv6 is optional, in a later step.
3. **TCP connect timeout** in `indigo_uni_open_client_socket`, or a new variant with a timeout (non-blocking connect + poll/select).
4. **HTTP/1.1 client:** either in the driver (`alpaca_http.c`) or as a library helper `indigo_uni_http_request()`.
   - GET/PUT, Content-Length, chunked, keep-alive with a single retry on a stale socket, gzip, binary body into a caller buffer, URL / form encoding, locale-independent number formatting.
   - **Recommendation:** keep it in the driver for now and move it to the library only when a second consumer appears (decision D3).

### 5.4 Image transfer

- Always request `Accept: application/imagebytes, application/json`.
- ImageBytes path:
  1. header validation;
  2. `DataStart`;
  3. element types 6 / 8 / 1 / 2 (and 3 / 4 / 9 / 7 / 5 for completeness);
  4. a transpose from column-major to top-down row-major;
  5. narrowing to 8 or 16 bpp according to `MaxADU` and the data range;
  6. for Rank 3, interleaving into RGB24 / RGB48 with `byte_order_rgb = true`.
- Bayer: `SensorType` + `BayerOffsetX/Y` → `BAYERPAT`.
- The download runs on a separate connection so that polling of the other devices keeps running.
- JSON fallback through the numeric array fast path (section 5.3/1).
- Orientation must be verified against OmniSim, which is the reference. `agent_alpaca` flips rows (defect AGENT-2).

### 5.5 Guider interface

The Camera and Telescope `pulseguide` calls take their duration in integer **ms**, and completion is signalled by `ispulseguiding`. Following `AGENTS.override.md:36`, a guider timing measurement is mandatory. It measures **simulator software timing only**, never hardware accuracy.

### 5.6 Out of scope for the first version

`action` / `command*` (exposed at most as a generic `X_ALPACA_ACTION`), IPv6, HTTPS and Basic auth (per-server extension), the camera `subexposureduration`, Switch async (V3) beyond the basics, and the Telescope `destinationsideofpier`.

## 6. Test strategy

Following `indigo_test/AGENTS.md` and `indigo_test/DRIVER_TESTING_RULES.md`:

1. **Unit tests** (per PR, all platforms):
   - JSON parser (section 5.3/1);
   - Alpaca envelope and `ErrorNumber` mapping;
   - ImageBytes decoding (all element types, ranks, errors, orientation);
   - URL and form encoding;
   - parsing of discovery replies.

   Fixtures are captured from OmniSim and stored in `indigo_test/fixtures/protocol/`.
2. **Deterministic C simulator** `indigo_drivers/system_alpaca/system_alpaca_simulator/`, following the `mount_starbook_simulator` pattern and the ready-file convention (`serial_simulator_common.h:72-96`, keys `INDIGO_SIMULATOR_HOST` / `INDIGO_SIMULATOR_TCP_PORT`).
   - HTTP/1.1 on 127.0.0.1 with an ephemeral port, plus a UDP discovery responder on a port chosen by the test.
   - Management API and all 10 types with minimal state.
   - Time advanced by an explicit step/tick, not by timers.
   - **Script-driven fault injection:**
     - malformed or partial JSON, a missing `Value`, a wrong transaction ID;
     - HTTP 400 / 404 / 500 / 503;
     - chunked responses, `Connection: close`;
     - stall, reset, a truncated body;
     - ImageBytes errors;
     - duplicate, late or malformed discovery replies, several servers, an INDIGO-bridge `description`.
   - Recording of requests, to check parameter casing, `ClientID`, `ClientTransactionID` monotonicity and form encoding.
   - Selectable interface version (P7 vs. V3).
3. **Integration tests** `indigo_test/integration/test_system_alpaca_simulator.c`:
   - one case per logical device;
   - class checklists (CCD, mount, guider, focuser, wheel, rotator, dome, AUX);
   - dynamic devices (identity, duplicates, failed attach, capacity, removal);
   - transport loss while idle and during an operation;
   - reconnect, SHUTDOWN, proxy-loop filtering;
   - the guider timing measurement.

   Cases that use a fixed or broadcast port run with `jobs = 1`, or as opt-in.
4. **Loopback test with `agent_alpaca`** (opt-in): `system_alpaca` against an in-process `agent_alpaca` fronting the INDIGO simulators, on a non-default discovery port.
5. **OmniSim tier** (opt-in `make -C indigo_test test-system-alpaca-omnisim`, Linux x64):
   - a pinned v0.5.0 with a checksum, `HOME` in a temporary directory, a pre-seeded `server/v1/instance-0.xml`;
   - one instance serialised with a lock, and a reset between cases;
   - timing tolerances.

   The results are recorded as "OmniSim 0.5.0" separately from the hardware-free counts.
6. **ConformU round trip** (manual or nightly): a differential comparison against a direct OmniSim baseline.
7. **Hardware** (`indigo_test/hardware/`): only if hardware is available (decision D6).

## 7. Atomic plan

States: `todo`, `in progress`, `done`, `blocked`. Every step records its evidence (commands, results) here once it is completed.

| # | Step | Verification | State |
|---|---|---|---|
| S0 | Research of the standard, simulators and INDIGO infrastructure; this document; registration in `DEVELOPED_DRIVERS` + temporarily `EXCLUDED_DRIVERS`; `ccd_pentax` also added to `EXCLUDED_DRIVERS` at the user's request | Review by the user | done (commits `7e0a2cc`, `Exclude ccd_pentax from the default build`, and this commit) |
| S1 | User decisions D1–D8 (section 9) | Recorded in this document | done (2026-09-24) |
| S1a | D8: ConformU baseline against `agent_alpaca` + all INDIGO simulators, fix AGENT-1..3 and LIB-1..2, rerun ConformU, unit/regression tests | ConformU JSON results before/after; unit tests | in progress |
| S2 | Library: general JSON parser in `indigo_json.c/.h` plus unit tests and fixtures | `make -C indigo_test test-unit`, strict build, ASan | todo |
| S3 | Driver skeleton: `.c/.h/_main.c`, bridge device, remove from `EXCLUDED_DRIVERS`, Xcode registration | `make -C indigo_drivers/system_alpaca -f ../../Makefile.drv`, the driver loads in `indigo_server` | todo |
| S4 | Library: `indigo_uni_io` connect timeout + multi-responder UDP discovery, with tests | unit/integration tests on loopback | todo |
| S5 | HTTP/1.1 client in `indigo_uni_io` (D3) + Alpaca transport layer in the driver (envelope, errors, IDs, encoding) + unit tests | unit tests on fixtures and a loopback HTTP server | todo |
| S6 | Deterministic simulator: management API, discovery, fault injection, ready file | Smoke test of the simulator; cross-check with alpyca as the reference client | todo |
| S7 | Discovery + management + attach/detach of proxies, proxy-loop filter | Integration: dynamic devices, duplicates, removal, SHUTDOWN | todo |
| S8 | Focuser, Wheel, Rotator (simplest classes, validate the pattern) | Class checklists | todo |
| S9 | Mount + guider | Mount/guider checklist, guider timing measurement | todo |
| S10 | CCD (+ guider), ImageBytes, JSON fallback | CCD checklist, image contract (dimensions, orientation, Bayer, RGB) | todo |
| S11 | Dome, CoverCalibrator, Switch, ObservingConditions, SafetyMonitor | Dome and AUX checklists | todo |
| S12 | OmniSim opt-in tier + ConformU round trip | Record of the OmniSim run | todo |
| S13 | `PROPERTIES.md`, README (only with approval), `TEST_SUMMARY.md`, optionally `MIGRATION_STATUS.md` / Windows / `STATIC_DRIVERS` | Final audit per `AGENTS.override.md` checklist | todo |

## 8. Found defects

All were found **by source audit only**. None has been reproduced. They are outside the scope of `system_alpaca`, and fixing them needs a separate decision.

| ID | Location | Impact | Cause | Proposed fix | Regression test |
|---|---|---|---|---|---|
| AGENT-1 | `indigo_drivers/agent_alpaca/indigo_alpaca_ccd.c:1402-1406` | An RGB24 image in ImageBytes has its channels in B, R, G order. The JSON path (`:1505-1515`) and RGB48 (`:1424-1428`) use R, G, B. | Wrong indices `base+2, base+0, base+1` | `base+0, base+1, base+2` | The loopback test in `system_alpaca` (section 6/4) with a colour CCD simulator |
| AGENT-2 | `indigo_alpaca_ccd.c:1366-1369` and other paths | Alpaca `y = 0` corresponds to the **bottom** INDIGO row (`row = height-1`), but Alpaca defines the origin at the top left. Suspected vertical flip; to be verified against OmniSim / ConformU. | The loop `for (row = height - 1; row >= 0; row--)` | After verification, iterate `row = 0..height-1` | Loopback plus an OmniSim orientation fixture |
| AGENT-3 | `indigo_agent_alpaca.c:279` | `description` returns `ServerVersion` / `ManufacturerURL` instead of the specified `ManufacturerVersion` / `Location`. Minor non-conformance. | Wrong key names | Add the specified keys | ConformU `alpacaprotocol` |
| LIB-1 | `indigo_libs/indigo_uni_io.c:1054` | `connect()` has no timeout, so an unreachable host blocks the caller for the OS TCP timeout | Blocking connect | Section 5.3/3 | Test against an unreachable address |
| LIB-2 | `indigo_drivers/focuser_askar/focuser_askar.driver:324-430` | Platform-dependent code (`getifaddrs`, `WSAIoctl`, `#if`) in a driver, contrary to `AGENTS.md:57` | No portable helper exists | Move it to the helper from section 5.3/2 | Existing askar tests |

## 9. Decisions for the user

Decided by the user on 2026-09-24:

| ID | Decision |
|---|---|
| D1 | Hand-written driver, **with the same structure as generated code** (the generated hot-plug layout: driver queue, `devices[]`, shared private data, `<driver>_open` / `<driver>_close`, handler + `_finalizer`, on_change-style handlers). No generator changes. |
| D2 | Library changes approved (general JSON parser in `indigo_json.c`, multi-responder UDP discovery, connect timeout). **Everything must be covered by unit tests.** |
| D3 | The HTTP client goes into `indigo_uni_io`. |
| D4 | Variant (b): only devices selected in `X_ALPACA_DEVICES` are proxied, persisted by UniqueID. |
| D5 | On an INDIGO disconnect, send `Connected=false` / `disconnect` to the Alpaca device. |
| D6 | Simulators only for now. No hardware testing is planned and no hardware validation is claimed. |
| D7 | No CI. The OmniSim and ConformU tiers stay manual / opt-in. |
| D8 | Fix and test defects AGENT-1..3 and LIB-1..2. **This goes first**, before the `system_alpaca` implementation. ConformU (https://ascom-standards.org/COMDeveloper/Conformance.htm) is run against `agent_alpaca` exposing every INDIGO simulator, before and after the fixes. |

## 10. Baseline

This is a new driver, so there is no original implementation to build or test. No baseline build was run: the folder contains no sources, and `system_alpaca` is in `EXCLUDED_DRIVERS`.

## Final test summary

- Simulated tests: 0 run, 0 passed.
- Hardware tests: 0 run, 0 passed.
- The OmniSim and ConformU runs from the research phase (subagent, linux-x64 container, outside the repository) were exploratory tool probes. They are not driver tests and are not counted.
