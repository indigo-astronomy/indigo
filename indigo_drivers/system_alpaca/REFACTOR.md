# system_alpaca: ASCOM Alpaca client (discovery and proxy) driver

Status: **implementation in progress, 2026-10-03**. The core and all device classes are implemented and covered against the deterministic simulator and OmniSim 0.5.0 (section 7). The independent review and its fixes are done (section 7.6); the decisions D11 to D14 are implemented; the final audit remains. Sections 0 to 6 are the research and design record of 2026-09-24, kept as written apart from dated corrections.

## 0. Goal and scope

- `system_alpaca` discovers ASCOM Alpaca servers on the LAN and creates one INDIGO proxy device for each Alpaca device they expose (camera, telescope, focuser, ...). INDIGO is the Alpaca **client**. It is the opposite direction of `indigo_drivers/agent_alpaca`, which exports INDIGO devices as an Alpaca **server**.
- This is a new driver, not a migration. The `AGENTS.md` / `indigo_drivers/AGENTS.override.md` rules for an original-driver baseline, a reference trace and characterization-first testing therefore have no original to apply to. The equivalent contract is conformance to the Alpaca specification, tested against a deterministic simulator (section 6).
- `system_ascol` is not used as a model: it is off limits and of poor quality. It is cited only for the `system_` naming and entry-point convention.
- Location: `indigo_drivers/system_alpaca`.
- Build registration in root `Makefile`:
  - `DEVELOPED_DRIVERS`: built, excluded from `install` / `uninstall` and so from the distribution (`Makefile.drvs:42-46`).
  - **Temporarily also `EXCLUDED_DRIVERS`** (removed on 2026-10-03 at the user's request; since then `make all` builds the driver and `install` skips it). `Makefile.drvs` collects `system_*` through a wildcard, and a folder without sources would break `make all`. The folder is also excluded because `indigo_driver_metadata` would fail on a missing library. Remove it from `EXCLUDED_DRIVERS` in the step that adds the first sources (plan step S3).
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
  - Booleans are sent as `True` / `False`, the way ASCOMLibrary does (`boolValue.ToString(CultureInfo.InvariantCulture)`); servers compare the value case-insensitively. Corrected on 2026-10-01 from the earlier `true` / `false`, see `alpaca_http_form_add_bool()`.
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

**Not part of the INDIGO project (decision D9).** OmniSim is an extra application, installed only on the machines where `system_alpaca` is developed.

- It lives **outside the working tree**, in a directory `omnisim/` next to it (for a checkout in `~/Development/indigo` that is `~/Development/omnisim`), so it can never be committed. Until 2026-10-03 it lived in the git-ignored `indigo_drivers/system_alpaca/system_alpaca_simulator/omnisim/`; it was moved at the user's request and the `.gitignore` rule was removed.
- Nothing of OmniSim is tracked: no binary, no archive, no submodule, no entry in `indigo_libs/externals`, and no download step in any Makefile or test target.
- The INDIGO build, the packages and the default test targets do not depend on it. A machine without OmniSim builds and tests INDIGO exactly as before; only the opt-in OmniSim tier (section 6/5) is skipped there.
- The developer installs, updates and removes it by hand, as described below.

**Installation.** The release is a plain archive holding a self-contained build, so there is no installer and no .NET runtime to install. The steps are the same on every platform: download the asset for the host, check it, and unpack it into `omnisim/` next to the INDIGO working tree.

1. Pick the asset of release **v0.5.0** from https://github.com/ASCOMInitiative/ASCOM.Alpaca.Simulators/releases/tag/v0.5.0:

   | Host | Asset | SHA-256 |
   |---|---|---|
   | macOS arm64 | `ascom.alpaca.simulators.macos-arm64.zip` (49 MB) | `ca8069a24e2e7c2049db15b98069308230c4d0cc15c6e41a2ea8c2cff0ad4893` |
   | macOS x64 | `ascom.alpaca.simulators.macos-x64.zip` (51 MB) | `2b6724b033130862834dc3f1816e18ba7c7cdf5d8929609dff468cbc60cad9fa` |
   | Linux x64 | `ascom.alpaca.simulators.linux-x64.tar.xz` (34 MB) | `5d9dd3ecdaefb3b36ffe17223172e656a7920c7830185f19390f5d84f051d625` |
   | Linux arm64 | `ascom.alpaca.simulators.linux-aarch64.tar.xz` (31 MB) | `7b55ebaeb662046eca6ed7dcbb0947d322894fb2ce7eedca6554f695aa160195` |
   | Linux armhf | `ascom.alpaca.simulators.linux-armhf.tar.xz` (33 MB) | `8c7eadaac9a80a0c565865de5c86561f3d910043ee48176326f30e63cda1daed` |
   | Windows x64 | `ascom.alpaca.simulators.windows-x64.zip` (51 MB) | `f808dc0d9c8d5cbfaa8b03ceaad64454a3a52728e2602795c397d847fbc367a0` |

   The checksums are the digests GitHub publishes for the release assets (read on 2026-10-01). The `*.AppImage.tar.xz` assets (experimental, need libfuse2) and `net80.zip` (needs an installed .NET 8 runtime) are not used.
2. Download, verify and unpack. From the root of the INDIGO working tree, on macOS arm64:

   ```sh
   mkdir -p ../omnisim
   cd ../omnisim
   curl -LO https://github.com/ASCOMInitiative/ASCOM.Alpaca.Simulators/releases/download/v0.5.0/ascom.alpaca.simulators.macos-arm64.zip
   shasum -a 256 ascom.alpaca.simulators.macos-arm64.zip
   unzip -q ascom.alpaca.simulators.macos-arm64.zip
   ```

   On Linux the same with `sha256sum` and `tar -xJf ascom.alpaca.simulators.linux-x64.tar.xz`, plus the ICU library from the distribution (`libicu` package), which the self-contained build still needs.
3. The archive unpacks into a directory named after the asset (about 130 MB), so the executable ends up as `omnisim/ascom.alpaca.simulators.macos-arm64/ascom.alpaca.simulators`. Verified for the macOS arm64 zip from its file listing; the Linux archives are expected to follow the same layout, to be confirmed on the first Linux install.
4. Nothing to check in git: the directory is outside the working tree.
5. macOS only: the release is a plain executable, not an application bundle. An archive downloaded with a browser carries the quarantine attribute, which may have to be removed before the first start (`xattr -dr com.apple.quarantine omnisim`). A `curl` download is not quarantined.
6. Smoke test. Start the server in one terminal with a private home; on macOS `HOME` alone is not enough (see Removal), so `CFFIXED_USER_HOME` and `ASCOM_LOGPATH` point there too:

   ```sh
   H="$(mktemp -d)"; mkdir -p "$H/.config" "$H/Library/Application Support"
   (cd "$H" && HOME="$H" CFFIXED_USER_HOME="$H" ASCOM_LOGPATH="$H/logs" "$OLDPWD/ascom.alpaca.simulators.macos-arm64/ascom.alpaca.simulators" --urls=http://127.0.0.1:32323)
   ```

   `--set-no-browser` is not a run option: it stores the setting and exits (corrected on 2026-10-02 after the first macOS start). Run it once with the same `HOME` before the real start if the browser must stay closed.

   and query it from another one:

   ```sh
   curl http://127.0.0.1:32323/management/v1/configureddevices
   ```

   The reply must list the simulated devices. Stop the server with Ctrl-C.

- **How the tests find it:** the opt-in tier looks for `omnisim/*/ascom.alpaca.simulators` next to the INDIGO working tree (`indigo_test/Makefile` passes the absolute path as `SYSTEM_ALPACA_OMNISIM_DIR`). `INDIGO_TEST_OMNISIM` (the path of the executable) overrides this for an installation kept elsewhere. If neither yields an executable, the tier is skipped and reported as not run.
- **Update:** delete the content of `omnisim/` and unpack the new version. The version used is recorded with every OmniSim result.
- **Removal:** delete `omnisim/`. OmniSim keeps its profiles in `GetFolderPath(ApplicationData)/ascom/alpaca/` and its logs in `$ASCOM_LOGPATH/logs<date>`, or in `GetFolderPath(Personal)/ascom/logs<date>` without that variable (ASCOM.Tools 2.2.0 `XMLProfile`, `TraceLogger`). On Linux .NET 8 takes these folders from `HOME` (and `XDG_CONFIG_HOME`). On macOS it asks Foundation (`NSFileManager URLsForDirectory`), which ignores `HOME` and follows only `CFFIXED_USER_HOME`, so a start without that variable writes to `~/Library/Application Support/ascom/alpaca/` and `~/Documents/ascom` of the real user; remove those after manual runs. The test tier never writes there (section 7.5).
- **State of this procedure:** the asset names, sizes, checksums and the macOS arm64 archive layout were read from the release on 2026-10-01, and the `.gitignore` rule was checked with `git check-ignore`. The measurements above come from the linux-x64 research run. The macOS arm64 install was done on 2026-10-02: the checksum matched, `git check-ignore` printed the rule, and the server started with a private `HOME` answered `configureddevices` (all ten device types) about 3 s after the start and reported version `0.5.0+05d607826b39bdee2bcbb09bcaac6f35b3c6dba9`; its focuser has InterfaceVersion 4. The quarantine step was not needed for the `curl` download.

### 4.2 ConformU v4.5.0 (GPL-3.0, run only as an external tool)

**Not relevant for `system_alpaca` (decision D9).** ConformU tests the opposite side of the protocol: it is an Alpaca client that checks **servers**, whereas `system_alpaca` is itself a client. It was the right tool for `agent_alpaca` (decision D8, done), and it is not installed, required or run for this driver. The former round-trip idea (`OmniSim → system_alpaca → INDIGO bus → agent_alpaca → ConformU`) is dropped: its result would be dominated by the limits of `agent_alpaca`, not by this driver. The notes below are kept only as a record of the research.

- CLI: `conformu conformance <uri>` and `conformu alpacaprotocol <uri>`, with `-r` writing JSON results. The exit code is the number of errors plus issues.
- Measured against OmniSim:
  - `alpacaprotocol` on the focuser: 6 s.
  - Full `conformance`: focuser 117 s, switch 479 s.
- It tests **servers only**, so it cannot test our client directly. The dropped round trip would have been `OmniSim → system_alpaca → INDIGO bus → agent_alpaca → ConformU`, evaluated **differentially** against a baseline run directly on OmniSim, with the limits of `agent_alpaca`:
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
  - `X_ALPACA_DEVICES`: list of discovered devices (name, type, server, UniqueID, state); every device is proxied unless the user switches it off, and the switches that are off are persisted by UniqueID (decision D15, replacing D4).
- **Server record** (`alpaca_server`), one per `ip:port`:
  - HTTP connection(s), a mutex, `ClientID`, the `ClientTransactionID` counter;
  - the reusable response buffer in private data (`AGENTS.md:60`);
  - the `description` data;
  - the INDIGO-bridge flag.
- **Proxy device**, one `indigo_device` per `configureddevices` item. The key is the `UniqueID`, so the device keeps its identity when its IP changes.
  - Private data points to the server record and holds the `DeviceType`, `DeviceNumber`, `InterfaceVersion`, capability cache (`Can*` plus probed optional members) and polling state.
  - `master_device` is the first device from the same server.
  - Device name: `"ALPACA <DeviceName>"` with every "alpaca" of the DeviceName removed and every `@` replaced by `-`, the DeviceType if nothing is left; duplicates numbered `#2`, `#3`, … in the order of their UniqueIDs (D17, 2026-10-03; before that `"<DeviceName> on <ServerName>"` (D14) and originally `"<DeviceName> @ <ServerName>"`).
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

**Superseded in part by decision D10 (2026-10-01).** Items 1 and 4 are implemented as driver-local modules in `indigo_drivers/system_alpaca/` with the symbol prefixes `alpaca_json_` and `alpaca_http_`; `indigo_libs` is not changed. Items 2 and 3 were already delivered by D8. The requirements listed below still describe what the modules must do; only their location and names changed.

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
5. **OmniSim tier** (opt-in `make -C indigo_test test-system-alpaca-omnisim`, on any development machine where OmniSim is installed as described in section 4.1):
   - the target never downloads or installs anything; it starts the executable found in the `omnisim/` directory next to the working tree (or named by `INDIGO_TEST_OMNISIM`) and is skipped (reported as not run) when there is none;
   - the expected version is v0.5.0, read from the running instance and recorded with the result;
   - `HOME` in a temporary directory, a pre-seeded `server/v1/instance-0.xml`;
   - one instance serialised with a lock, and a reset between cases;
   - timing tolerances.

   The results are recorded as "OmniSim 0.5.0" separately from the hardware-free counts.
6. **Hardware** (`indigo_test/hardware/`): only if hardware is available (decision D6).

### 6.1 Deterministic simulator (`system_alpaca_simulator`)

Host-side Alpaca server for the integration tests. Sources: `system_alpaca_simulator/system_alpaca_simulator.c` (framework), `system_alpaca_simulator.h` (module interface) and one `system_alpaca_simulator_<type>.c` per Alpaca device type. Built by `indigo_test/Makefile` into `$(INTEGRATION_BUILD)/system_alpaca_simulator`. `--help` lists every option, control endpoint and fault action.

**Start.** `system_alpaca_simulator --headless --ready-file <dir>/ready.env [--discovery-port 0] [--device ...]`. The ready file holds `INDIGO_SIMULATOR_HOST`, `INDIGO_SIMULATOR_TCP_PORT`, `INDIGO_SIMULATOR_TCP_URL`, `INDIGO_SIMULATOR_PID`, and with discovery `INDIGO_SIMULATOR_UDP_PORT`. HTTP listens on 127.0.0.1 on a free port. Discovery is off unless `--discovery-port` is given (0 = free port). The simulator stops on SIGTERM / SIGINT, on `PUT /simulator/v1/shutdown`, and when its parent process exits.

**Topology.** Without `--device` there is one device of each of the 10 types. `--device <type>:key=value,...` (repeatable) builds any other set: `number`, `name`, `uid`, `interface=p7|legacy|<n>` plus any state key, e.g. `--device camera:number=3,CameraXSize=64,CameraYSize=48,SensorType=2`. `--indigo-bridge` makes `description` look like `agent_alpaca`. Two servers with the same UniqueID: start two simulators with `--device focuser:uid=<same>`. Devices can be added and removed at run time with `PUT /simulator/v1/devices` and `.../devices/remove`.

**Time.** With the default `--clock manual` device time moves only by `PUT /simulator/v1/clock Advance=<seconds>` (or by `--tick-per-request`). Every move, slew, exposure, settle and connect delay is measured on that clock, so a test starts an operation, sees it in progress for as long as it likes, advances the clock and sees it complete. `--clock real` follows wall time for manual use.

**Control API** (`/simulator/v1/...`, form-encoded PUT, `Key=value` text answers split on `&` and newline, values percent-encoded): `status`, `clock`, `reset`, `requests`, `requests/clear`, `faults`, `faults/clear`, `discovery`, `server`, `devices`, `devices/remove`, `shutdown`, and per device `<type>/<n>/state`, `<type>/<n>/reset`, `<type>/<n>/error`.

```sh
. "$dir/ready.env"; B=$INDIGO_SIMULATOR_TCP_URL
curl -X PUT -d 'Position=51000&ClientID=7&ClientTransactionID=2' $B/api/v1/focuser/0/move
curl -X PUT -d 'Advance=1' $B/simulator/v1/clock                      # one second of device time
curl -X PUT -d 'InterfaceVersion=legacy' $B/simulator/v1/focuser/0/state
curl -X PUT -d 'Method=GET&Path=*/focuser/0/position&Action=http-status&Value=503' $B/simulator/v1/faults
curl -X PUT -d 'Member=temperature&ErrorNumber=1024' $B/simulator/v1/focuser/0/error
curl -X PUT -d 'Mode=delayed&Delay=300&Count=1' $B/simulator/v1/discovery
curl "$B/simulator/v1/requests?Method=PUT&Path=*/focuser/*"
```

**Faults.** A rule is `Path` (glob) + `Action`, optionally `Method`, `Skip`, `Count` (default 1), `Value`, `Delay` (ms, real time), `Bytes`, `Message`. Actions: `http-status`, `ascom-error`, `malformed-json`, `truncated-json`, `missing-value`, `null-value`, `wrong-transaction-id`, `chunked`, `connection-close`, `silent-close`, `stall-before`, `stall-within`, `truncated-body`, `drop`, `reset`, `imagebytes-version`, `imagebytes-datastart`, `imagebytes-type`, `imagebytes-rank`, `imagebytes-dimensions`, `imagebytes-short`, `imagebytes-error`, `imagebytes-json`. Except for `http-status` and `ascom-error` the request is executed and only the reply is damaged; `Dispatch=false` drops it before the device. Discovery faults are set with `PUT /simulator/v1/discovery`: `Mode=silent|malformed|duplicate|delayed`, `Copies`, `Delay`, `Count`, `Reply` (raw text), `AlpacaPort=<p>|<p>` (one reply per port, as if several servers shared the host).

**Recording.** Every Alpaca request is kept in order with its sequence number, simulated and real time, connection number, method, path, query, body, `Content-Type`, `Content-Length`, `Accept`, `Connection`, `Host`, `User-Agent`, resulting status, ErrorNumber and applied fault. `GET /simulator/v1/requests` returns them; the same goes to the event log `<ready-file>.events` (`<time> RX|TX|FAULT|CONTROL|STATE|MOVE|DISCOVERY|OPEN|CLOSE ...`). RX lines contain the driver's `ClientID`, which a reference-trace comparison has to mask.

**Camera test pattern.** `value = (3*sensor_x + 7*sensor_y + 31*colour) % (MaxADU + 1)` with `sensor_x = (StartX + x) * BinX`, `sensor_y = (StartY + y) * BinY`, origin top left. `colour` is the plane (0 R, 1 G, 2 B) for `SensorType=1`, the filter colour for a CFA sensor (`(sensor_x + BayerOffsetX) % 2 + (sensor_y + BayerOffsetY) % 2`) and 0 for mono. A dark frame is `value % 16`. Any transpose, flip, wrong subframe or swapped plane changes the values.

**Protocol strictness.** Lower-case paths only. Unknown type, device number, member or method, and a missing or wrongly cased PUT parameter, are HTTP 400. GET parameter names are case-insensitive. ASCOM errors are HTTP 200 with `ErrorNumber`. `ClientTransactionID` is echoed (0 when absent or invalid). `ServerTransactionID` grows with every Alpaca response and survives a reset. A member that the selected interface version does not have is HTTP 400 by default; `MissingMember=404` or `1024` (NotImplemented) select the other behaviours seen in the field.

**Extending a type.** Add a field to the module's state struct and a row to its `alpaca_member` table; add a handler only for methods and conditional members. See the comment at the top of `system_alpaca_simulator.h`.

**Known limits.** Not built or run on Linux or with GCC yet. No gzip, HTTPS, authentication or IPv6. Hermetic discovery on macOS: `lo0` has no broadcast, so a test probes `127.0.0.1` with one discovery port per simulator, or lets one responder announce several servers with `AlpacaPort=<p1>|<p2>`; the shared-port path is implemented but unproven. Simplifications per type (for example the deprecated synchronous slews completing at once and `reverse` of the rotator only stored) are listed in the module sources; section 7.2 names the ones left after wave 3.

## 7. Atomic plan

States: `todo`, `in progress`, `done`, `blocked`. Every step records its evidence (commands, results) here once it is completed.

| # | Step | Verification | State |
|---|---|---|---|
| S0 | Research of the standard, simulators and INDIGO infrastructure; this document; registration in `DEVELOPED_DRIVERS` + temporarily `EXCLUDED_DRIVERS`; `ccd_pentax` also added to `EXCLUDED_DRIVERS` at the user's request | Review by the user | done (commits `7e0a2cc`, `Exclude ccd_pentax from the default build`, and this commit) |
| S1 | User decisions D1–D8 (section 9) | Recorded in this document | done (2026-09-24) |
| S1a | D8: ConformU baseline against `agent_alpaca` + all INDIGO simulators, fix AGENT-1..3 and LIB-1..2, rerun ConformU, unit/regression tests | ConformU JSON results before/after; unit tests | done (2026-09-24). See `../agent_alpaca/REFACTOR.md` and section 8. `indigo_uni_discover()` and the connect timeout from WP3 / S4 already exist now. |
| S2 | General JSON parser, driver-local per D10: `indigo_system_alpaca_json.c/.h` plus `indigo_test/unit/test_system_alpaca_json.c` and fixtures | unit test, strict build, ASan/UBSan | done (2026-10-01, WP1). Evidence in section 7.1. |
| S3 | Driver skeleton: `.c/.h/_main.c`, bridge device, remove from `EXCLUDED_DRIVERS`, Xcode registration | `make -C indigo_drivers/system_alpaca -f ../../Makefile.drv`, the driver loads in `indigo_server` | done (2026-10-01, WP5). Evidence in section 7.1. Xcode: group references only; target membership follows when the driver is complete. |
| S4 | Library: `indigo_uni_io` connect timeout + multi-responder UDP discovery, with tests | unit/integration tests on loopback | done by D8 (S1a): `indigo_uni_open_client_socket_with_timeout()` and `indigo_uni_discover()` exist in `indigo_uni_io.h`, covered by `indigo_test/unit/test_uni_io.c`. No further library change (D10). |
| S5 | HTTP/1.1 client, driver-local per D10: `indigo_system_alpaca_http.c/.h` + Alpaca transport layer in the driver (envelope, errors, IDs, encoding) + tests | `test_system_alpaca_http` against a loopback HTTP server; transport tests on fixtures | done (2026-10-01, WP2 + WP5). Evidence in section 7.1. |
| S6 | Deterministic simulator: management API, discovery, fault injection, ready file | Smoke test of the simulator; cross-check with alpyca as the reference client | done (2026-10-01, WP4). Evidence in section 7.1, usage in section 6.1. alpyca is not installed, so there was no cross-check with a reference client. |
| S7 | Discovery + management + attach/detach of proxies, proxy-loop filter | Integration: dynamic devices, duplicates, removal, SHUTDOWN | done (2026-10-01, WP5). Evidence in section 7.1. |
| S8 | Focuser, Wheel, Rotator (simplest classes, validate the pattern) | Class checklists | done (2026-10-02, WP6). Evidence in section 7.2. |
| S9 | Mount + guider | Mount/guider checklist, guider timing measurement | done (2026-10-02, WP7). Evidence in sections 7.2 and 7.3. |
| S10 | CCD (+ guider), ImageBytes, JSON fallback | CCD checklist, image contract (dimensions, orientation, Bayer, RGB) | done (2026-10-02, WP8). Evidence in section 7.2. |
| S11 | Dome, CoverCalibrator, Switch, ObservingConditions, SafetyMonitor | Dome and AUX checklists | done (2026-10-02, WP9 and WP10). Evidence in section 7.2. |
| S12 | OmniSim opt-in tier (OmniSim installed on the development machine per section 4.1; no ConformU, decision D9) | Record of the OmniSim run | done (2026-10-03, WP11). Evidence in section 7.5. |
| S13 | `PROPERTIES.md`, README (only with approval), `TEST_SUMMARY.md`, optionally `MIGRATION_STATUS.md` / Windows / `STATIC_DRIVERS` | Final audit per `AGENTS.override.md` checklist | done except the Windows build: `PROPERTIES.md`, `README.md`, `TEST_SUMMARY.md`, Xcode targets and `STATIC_DRIVERS` (user, 2026-10-03), removal from `EXCLUDED_DRIVERS`, decisions D11 to D17 implemented, Linux arm64 run (section 7.9), Windows project files (not built, section 7.9) |

### 7.1 Step evidence

All runs on macOS arm64 (Apple clang), 2026-10-01. Nothing here was built or run on Linux or Windows; the x86_64 slices were compiled only. These are tests of driver-local modules and of the simulator, not driver test runs, so they are not part of the final test summary counts.

**S2, JSON parser (WP1).** `indigo_system_alpaca_json.c/.h`, 17 exported `alpaca_json_` symbols: a DOM parser in one allocation (`alpaca_json_parse`, typed getters, case-insensitive and exact member lookup, depth limit 32, size limit 4 MB) and `alpaca_json_parse_image`, which scans an `imagearray` envelope without a DOM and streams a rank 2 or 3 `Value` into a caller buffer in transmission order (`Value[x][y][plane]`; the transpose is the caller's job).

- `make -C indigo_drivers/system_alpaca -f ../../Makefile.drv all`, then `make -C indigo_test build/unit/test_system_alpaca_json` and `./build/unit/test_system_alpaca_json` from `indigo_test`: 48/48 cases pass (orchestrator rerun through the wired Makefile).
- Strict warnings build and ASan + UBSan build (subagent, by hand in a scratch directory; the sanitizer run repeated by the orchestrator): 48/48, no report. `leaks -atExit`: 0 leaks.
- Subagent's extra check, not kept in the repository: 200 000 mutated documents under ASan + UBSan compared with Python `json`, no accept/reject disagreement.
- The fixtures `indigo_test/fixtures/protocol/alpaca_*.json` are hand-written from the specification, not captured from OmniSim.
- The test is in `UNIT_TESTS`. `tools/run_driver_test.py` selects only integration and opt-in tests, so this unit test is not counted in the driver's recorded run.

**S5, HTTP client part (WP2).** `indigo_system_alpaca_http.c/.h`, 22 exported `alpaca_http_` symbols: a keep-alive connection per `host:port`, `alpaca_http_get` / `alpaca_http_put`, Content-Length, chunked and EOF-delimited bodies into a caller-owned reusable buffer with a hard maximum, separate connect / first-byte / inter-byte / total timeouts, distinct result codes, percent-encoding and a form builder with locale-independent doubles. `indigo_libs` is unchanged.

- `make -C indigo_test build/integration/test_system_alpaca_http` and `./build/integration/test_system_alpaca_http`: 44/44 cases pass (orchestrator rerun through the wired Makefile; the test is in `OPT_IN_DRIVER_TESTS` because it opens loopback sockets).
- Strict warnings build and ASan + UBSan build (subagent; the sanitizer binary rerun by the orchestrator): 44/44, no report. `leaks --atExit`: 0 leaks.
- `connect_timeout` is skipped on macOS and counted as passed: a full accept queue does not drop SYNs there. The hermetic path is written for Linux and has not run.
- Decisions the core has to respect:
  - gzip is not implemented and `Accept-Encoding` is never sent.
  - A GET is re-sent once when a reused socket fails before any response byte. A PUT is re-sent only when the caller marks it `replayable`; otherwise the result is `ALPACA_HTTP_RESET` and the device may or may not have executed it, so the driver polls the state instead of repeating a move, a pulse or an exposure start.
  - Redirects are reported, not followed.
- Limits of `indigo_uni_io` that were worked around rather than patched (D10): the connect helper does not report why it failed (refused vs. timeout is told apart by elapsed time), `getaddrinfo` has no deadline (use the numeric address from discovery), there is no write deadline API, `indigo_uni_write()` can raise SIGPIPE on Linux unless the process ignores it (`indigo_server` does; the standalone driver executable has to be checked), and read errors are logged unconditionally.

**S6, deterministic simulator (WP4).** Twelve files in `system_alpaca_simulator/`, usage in section 6.1.

- `make -C indigo_test build/integration/system_alpaca_simulator` builds it (orchestrator).
- Subagent's smoke script (curl for HTTP, Python for UDP discovery and ImageBytes decoding; kept outside the repository) against the Makefile-built binary, rerun by the orchestrator: 377 checks passed, 0 failed. It covers management, the strict-protocol answers, P7 and legacy connect, every device type from connect to an operation completing and being aborted, JSON and ImageBytes images compared pixel by pixel with the pattern formula, every fault action, request recording, the clock modes, discovery faults on loopback, concurrency and the shutdown paths.
- The subagent also reports clean strict, ASan + UBSan and TSan builds, each passing the same 377 checks, and 0 leaks.
- Not verified: Linux, GCC, a cross-check with alpyca (not installed), and broadcast discovery on macOS (see section 6.1).

**S3, S5 (transport), S7: driver core (WP5).** `indigo_system_alpaca.c/.h/_main.c` (bridge device, discovery, registry, proxy framework, entry point), `indigo_system_alpaca_transport.c/.h` (Alpaca request layer), `indigo_system_alpaca_private.h` (the class-module interface, with the recipe for writing a class module in its header comment) and eleven class stubs that attach with the right INDIGO base class, connect, poll and disconnect. `DRIVER_VERSION 0x03000001`. `system_alpaca` is removed from `EXCLUDED_DRIVERS` in the root `Makefile`, so the default build now includes it.

- `make -C indigo_drivers/system_alpaca -f ../../Makefile.drv all`: archive, library and executable build without warnings (orchestrator rerun).
- `make -C indigo_test build/integration/test_system_alpaca_simulator` and `./build/integration/test_system_alpaca_simulator`: **56 run, 56 passed** (orchestrator rerun; 47 core cases and 9 cases that exercise the class-module interface on a live proxy). Each case runs in a forked child with its own HOME, simulators and ports. The test is in `OPT_IN_DRIVER_TESTS`.
- After the archive grew to the whole driver, `test_system_alpaca_json` (48/48) and `test_system_alpaca_http` (44/44) still pass (orchestrator rerun).
- Subagent's further runs: strict warnings build of all 16 driver sources clean; ASan + UBSan build 56/56 without a report; TSan build 56/56 with 0 warnings; more than 10 repetitions with `INDIGO_TEST_JOBS` 1 to 32 without a flake; the driver loads and unloads in `indigo_server`, and an end-to-end pass through the server with `indigo_prop_tool` (manual server, camera selected, proxy and guider attached, connect, `CONFIG SAVE`, disconnect) showed `connect` and `disconnect` in the simulator's record.
- Not verified: Linux, Windows, GCC, x86_64 execution; real broadcast discovery (tests use explicit loopback targets); IPv6; the `UNSUPPORTED` device-type and API-version branches and the rollback of a failing class attach (the simulator cannot produce them); the standalone executable beyond starting it.
- The bridge properties are documented in `indigo_docs/PROPERTIES.md`, section `system_alpaca`. Beyond the plan of section 5.2 there are `X_ALPACA_DISCOVERY_TARGETS` (explicit IPv4 targets, needed for hermetic tests and for networks without broadcast), `X_ALPACA_SERVER_STATUS`, `X_ALPACA_DEVICE_STATUS`, `X_ALPACA_TIMEOUTS` and `X_ALPACA_POLLING`; the planned single `X_ALPACA_DEVICES` list is split into the selection switch and a read-only status text.
- Deviations from section 5.2, with reasons:
  - Each proxy is its own `master_device` with its own connection and handler queue (only the guider shares its primary's). With devices that can be switched off one by one (D15) the first device of a server can be detached, which would take away the queue the others run on.
  - `CONNECTION` runs on the proxy's own handler queue, not on the driver queue, so a slow device does not block discovery. The driver queue keeps discovery, management and attach/detach.
  - Disabling discovery only skips the UDP request; the cycle still refreshes the known servers, otherwise a manual server that was down at start would never be picked up.
  - On a name collision the suffix is a running number ` #2`, ` #3`… given in the order of the UniqueIDs of the devices of that name the servers list (D17, 2026-10-03), not in attach order; it is stable across restarts while the same devices are present. (Until then it was ` #<6 hex digits of the UniqueID hash>`.)
  - A listed device that does not answer `interfaceversion` is `FAILED` and retried every cycle.
  - NotConnected (0x407) takes the device to disconnected / ALERT; there is no automatic reconnect.
  - The proxy loop is filtered by `description` (ServerName or Manufacturer) only.
  - `_main.c` ignores SIGPIPE; the driver library never touches signal dispositions.
- Known limits: 32 proxies, 16 servers, 128 known devices, 64 devices per server listing, 16 discovery targets. A request with the long timeout class blocks its own device's queue for up to that time. Proxy names contain no `@` (D14), so the remote-host separator of the framework can neither split nor misroute them; `core_proxy_names_without_at` and `core_proxy_names_routing` check the routing in-process, a run through `indigo_server` with a remote service is still untested.

**Xcode.** Every new file is registered in `indigo.xcodeproj` as a group reference as soon as it exists (user's instruction, 2026-10-01). Membership in the Sources and Headers phases of the `indigo` and `indigo_m1` targets follows when the driver is complete.

### 7.2 Wave 3: device classes (2026-10-02)

All runs on macOS arm64 (Apple clang). Nothing here was built or run on Linux, Windows or with GCC, against OmniSim or against hardware.

**How the wave ran.** The class modules and simulator type modules of the WIP checkpoint (commit `62007fc2d`) had no cases of their own except Switch. The baseline of the suite on that commit was **70 run, 66 passed, 4 failed** (`interface_capability_cache`, `switch_async_abort`, `switch_async_failures`, `switch_lifecycle`); all four turned out to be defects of the cases (TEST-1, T1, T2 in section 8). Five subagents (WP6 to WP10) then worked in parallel in the one working tree, each on the files of its classes only, and each built a private tree outside the repository: a snapshot of the sources in which its own files were links to the live ones. For every class the driver module and the simulator module were audited against section 2, the cases were written, and every defect a case found was fixed in the driver module.

**Result after the merge** (orchestrator, live tree): `make -C indigo_drivers/system_alpaca -f ../../Makefile.drv all` and `make -C indigo_test build/integration/test_system_alpaca_simulator` without a warning, then `INDIGO_TEST_JOBS=4 ./build/integration/test_system_alpaca_simulator` from `indigo_test`: **222 run, 222 passed, 0 failed**.

| Cases file | Cases | Package | What the package verified in its private tree |
|---|---|---|---|
| `core_cases.h` | 47 | WP5 (wave 2) | unchanged |
| `interface_cases.h` | 9 | WP5, one case corrected by WP6 | 5 runs of the 9 cases |
| `focuser_cases.h` | 14 | WP6 | plain, ASan + UBSan and TSan without a report; 10 repetitions (5 with `INDIGO_TEST_JOBS=4`, 5 with `=1`) |
| `wheel_cases.h` | 10 | WP6 | same |
| `rotator_cases.h` | 15 | WP6 | same |
| `mount_cases.h` | 21 | WP7 | plain, strict, ASan + UBSan clean; TSan: 2 of 6 runs with one report each (see below); 10 repetitions |
| `guider_cases.h` | 9 | WP7 | same; the cases with a camera as primary ran against the camera module of the checkpoint and passed again after the merge |
| `ccd_cases.h` | 26 | WP8 | plain and ASan + UBSan clean; TSan clean only with the core change described below; 10 repetitions; `leaks`: only LIB-3 |
| `dome_cases.h` | 20 | WP9 | plain and ASan clean; TSan: one report in one case (see below); 10 repetitions |
| `lightbox_cases.h` | 15 | WP9 | same |
| `switch_cases.h` | 19 | WP10 | plain, ASan + UBSan and TSan (2 runs) without a report; 10 repetitions |
| `weather_cases.h` | 10 | WP10 | same |
| `safety_cases.h` | 7 | WP10 | same; 20 repetitions |

- **Strict build** (`-Wall -Wextra -Wshadow`): no warning in a driver, simulator or case file of any package. The 27 or 28 warnings left are `-Wunused-function` in the shared harness headers `test_runner.h`, `serial_simulator_test_common.h` and `simulator_test_common.h`, which every test of the repository includes.
- **Regression evidence.** WP6 and WP9 built a second private tree with the class modules of the checkpoint and the new cases: 13 focuser / wheel / rotator cases and 13 dome / light box cases fail there, each at the check of the defect it is named for in section 8. WP10 and WP8 verified their regression cases by breaking the fixed code again in the private tree (mutation).
- **TSan reports left after the wave**, all of one kind and none in class code: the bus thread writes the state of `CONNECTION` (`INDIGO_PROCESS_CONNECT`) or of a property a client changes (`INDIGO_COPY_*_PROCESS_CHANGE`) while a handler or the poll hook on the device queue reads it (`IS_CONNECTED`, the BUSY guard). The recipe in `indigo_system_alpaca_private.h` prescribed exactly that. WP6 and WP10 moved their modules to a class mutex and the session flag of the core; the other modules and the core followed in the core-hardening step (section 7.4), after which the whole suite is clean under TSan.
- **Whole suite under ASan and TSan** was not run by the class packages (own cases only); see section 7.4.

**Scenario-to-test mapping.** The class checklists of `indigo_test/DRIVER_TESTING_RULES.md` map onto the cases as follows. Every class also has `<class>_properties` and `<class>_legacy_properties` (Platform 7 and older interface), `<class>_capability_variants`, `<class>_request_failures`, `<class>_connect_failures`, `<class>_transport_loss` and `<class>_lifecycle`, which cover identity, capability gating, device and transport failures at connect, at a request, at readback and during an operation, loss of the server while idle and during an operation, and disconnect / detach during an operation.

| Class | Checklist area → cases | Not applicable / not covered |
|---|---|---|
| Focuser | motion: `focuser_absolute_move`, `focuser_steps_of_absolute_focuser`, `focuser_relative_focuser`; stop: `focuser_abort`; modes: `focuser_temperature_compensation`; polling and pending change: `focuser_polling`, `focuser_pending_change_survives_poll` | sync, speed, backlash, compensation coefficients: no Alpaca member (properties hidden, asserted). Not covered: CONFIG of `FOCUSER_REVERSE_MOTION` |
| Wheel | slots and names: `wheel_slot_variants`; positioning: `wheel_move_to_slot`, `wheel_move_ends_elsewhere`; `wheel_polling` | calibration, direction, speed: no Alpaca member. Not covered: a Position outside the slots, more than 32 slots (simulator limit) |
| Rotator | `rotator_absolute_move`, `rotator_relative_move`, `rotator_mechanical_move`, `rotator_sync`, `rotator_reverse`, `rotator_abort`, `rotator_polling`, `rotator_pending_change_survives_poll` | backlash, limits, offset property: hidden, the sync offset lives in the device |
| Mount | identity and coordinates: `mount_equatorial_systems`; `mount_goto`, `mount_sync`, `mount_abort`, `mount_park_and_home`, `mount_manual_motion`, `mount_manual_motion_ownership`, `mount_tracking_and_rates`, `mount_site_and_time`, `mount_side_of_pier`, `mount_polling`, `mount_steady_state_is_silent`, `mount_poll_keeps_pending_change`, `mount_configuration_roundtrip` | custom tracking rate, park / home position, PEC, alignment: no Alpaca member (hidden). Not covered: the 600 s motion timeout, slews to alt-az (not mapped) |
| Guider | `guider_directions_and_units`, `guider_completion`, `guider_replacement_and_axes`, `guider_shared_lifetime`, `guider_camera_primary`, `guider_timing_telescope`, `guider_timing_camera` | a guider connected alone: it exists only while its primary is connected |
| CCD | acquisition: `ccd_exposure`, `ccd_legacy_exposure`, `ccd_exposure_countdown`, `ccd_frame_types`, `ccd_exposure_start_failures`, `ccd_exposure_progress_failures`, `ccd_abort_exposure`, `ccd_abort_variants`, `ccd_disconnect_during_exposure`; geometry: `ccd_geometry`, `ccd_symmetric_binning`; image contract: `ccd_image_element_types`, `ccd_image_bayer`, `ccd_image_color`, `ccd_image_json`, `ccd_image_faults`, `ccd_image_transfer_channel`, `ccd_large_frame`; `ccd_controls`, `ccd_cooling`, `ccd_guider` | streaming: Alpaca has none (`CCD_STREAMING` hidden, asserted). By reading only: image data above MaxADU, negative Int16, DataStart beyond the reply, more than 64 gain names or 32 readout modes, CCD-6 |
| Dome | `dome_slew_azimuth`, `dome_slew_altitude`, `dome_relative_move`, `dome_sync`, `dome_shutter`, `dome_park_home`, `dome_abort`, `dome_slaved`, `dome_slewing_includes_shutter`, `dome_polling`, `dome_pending_requests`, `dome_completion_during_pending_request`, `dome_disconnect_after_request` | speed, flap, park position, UTC: hidden. Not covered: the 600 s operation deadline |
| AUX light box | `lightbox_cover`, `lightbox_cover_halt`, `lightbox_calibrator`, `lightbox_unknown_and_error`, `lightbox_polling`, `lightbox_pending_requests`, `lightbox_failure_during_pending_request`, `lightbox_disconnect_after_request` | Not covered: the 300 s operation deadline |
| AUX GPIO (Switch) | `switch_set_outlets`, `switch_set_values`, `switch_names`, `switch_async_change`, `switch_async_abort`, `switch_async_abort_all`, `switch_async_failures`, `switch_async_start_failures`, `switch_polling`, `switch_many_switches`, `switch_limit`, `switch_pending_change_survives_poll` | config persistence: names live in the device |
| AUX weather | `weather_optional_sensors`, `weather_polling`, `weather_average_period`, `weather_refresh_values`, `weather_sensor_failures` | — |
| AUX (SafetyMonitor) | `safety_read_failures`, `safety_stale_value` | no writable property |

**Image contract.** `ccd_image_element_types` checks every pixel of a 22×21 subframe at binned origin 10003,9005 with binning 2×1 on a 30000×20000 sensor, for the transmission types Byte, UInt16, Int16, Int32, UInt32, Single, Double, Int64 and UInt64, 8 and 16 bpp and a 20-bit camera, with `DataStart=67`. `ccd_image_bayer` and `ccd_image_color` do the same for Bayer offsets (BAYERPAT follows the frame origin) and for rank 3 (RGB24 and RGB48), `ccd_image_json` for the JSON path, `ccd_large_frame` for frames up to 3000×2000 at 20 bits (24 MB). The simulator pattern of section 6.1 makes a transpose, a flip, a wrong subframe or binning and a swapped plane visible. The orientation against OmniSim is still to be confirmed in the OmniSim tier.

**Mapping decisions taken in the wave** (they refine section 5.2):

- SafetyMonitor is `X_ALPACA_SAFETY`, a light property with the single item `SAFE`, not a `SAFE` / `UNSAFE` pair. A value older than 4 idle poll intervals + 1 s is taken back to ALERT by a timer of its own, so that a server which stops answering can not leave SAFE published (D6).
- ObservingConditions sensors without a standard item are `X_` items of `AUX_WEATHER` (`X_WIND_GUST`, `X_RAIN_RATE`, `X_CLOUD_COVER`, `X_SKY_ILLUMINANCE`, `X_STAR_FWHM`); Alpaca SkyQuality is `SKY_BRIGHTNESS`.
- Alpaca `Slaved` of a dome is `X_ALPACA_DOME_SLAVED`. The removed SNOOP / `DOME_SLAVING` behaviour (commit `2c79d79b0`) is not brought back; synchronisation stays with the mount agent.
- A filter wheel move is complete when `Position` is no longer −1 (user's decision, 2026-10-02); a move that ends in another slot than the requested one is ALERT with the real slot (WHL-1).
- Manual motion: `MOUNT_MOTION_DEC.NORTH` always moves toward the north celestial pole (D13, implemented 2026-10-03; until then the driver sent a positive secondary rate on both pier sides): the driver reads SideOfPier when a motion starts and inverts the secondary rate on pierWest, in both hemispheres; without a known side it does not invert, and a running motion is not re-sent when the side changes. `mount_manual_motion` and `mount_manual_motion_sense` assert this.
- A sync while the telescope does not track is refused by the telescope and reported as ALERT with its message; the driver switches tracking on for a slew, not for a sync.
- The camera keeps `ImageReady` as the only completion signal of an exposure; a running image transfer can not be interrupted, so abort, disconnect and detach wait for it within the long timeout.
- `PROPERTIES.md` lists every custom property and the capability-dependent use of the standard ones.

**Simulator changes of the wave.** Camera: `CanFastReadout` and readout modes are mutually exclusive, new settings `ReadoutModeCount` and `ImageBytes=false` (SIM-1, SIM-2). Rotator: `TargetTracksMechanical`. Telescope: `LinkedGuideRates`. Switch: `cancelasync` of a switch without CanAsync is NotImplemented, `devicestate` leaves out members with an armed error, 160 switches (S1 to S3). Simplifications left: camera CameraState never Waiting or Download, binned pixels are the pattern at the top-left pixel; telescope: the deprecated synchronous slews complete at once, AbortSlew does not end a pulse; rotator: `Reverse` only stored; ObservingConditions: no averaging; dome and cover: an Error end state is produced through the control API only. The SIM-1 rule and the rotator and telescope defaults were chosen from the specification and are not checked against OmniSim yet.

**Not established by this wave.** Linux, Windows, GCC, x86_64 execution; OmniSim; hardware (D6); the points listed as "not covered" above.

### 7.3 Guider timing measurement (2026-10-02)

Required by `indigo_drivers/AGENTS.override.md` for every guider interface. **This is simulator software timing only**: there is no relay and no motor, and nothing here says anything about the pulse accuracy of a real device.

- Alpaca `PulseGuide` takes a duration and the device ends the pulse itself, so the direction and duration are asserted at the entry of the device and the latency from that entry to the public completion is measured.
- Endpoints: arrival of `PUT pulseguide` in the simulator (its `RealTime`, CLOCK_MONOTONIC) → update of `GUIDER_GUIDE_DEC` / `GUIDER_GUIDE_RA` with a state other than BUSY, stamped in a bus client callback with the same clock.
- Simulator with `--clock real`, `-O2` build, macOS arm64, load average about 3 (four other packages were testing), 2026-10-02 20:31 UTC.
- Directions N, S, E, W × 20, 100, 500 ms × 12 samples after one discarded warm-up = 576 samples, all completed OK.
- Workloads: telescope idle (Platform 7, polling 1 s / 0.2 s); telescope with a busy queue (interface 3, polled every 50 ms with 12 requests per tick); camera idle; camera during a 600 s exposure.
- Repeat with `make -C indigo_test benchmark-system-alpaca-guide`.

Signed error = (entry → published completion) − requested, in ms, over the four directions (48 samples per row):

| Primary | Workload | Requested | min | mean | max | max abs | mean % |
|---|---|---|---|---|---|---|---|
| telescope | idle | 20 | +1.004 | +6.303 | +10.996 | 10.996 | +31.52 |
| telescope | idle | 100 | +1.287 | +7.446 | +11.040 | 11.040 | +7.45 |
| telescope | idle | 500 | +1.633 | +7.840 | +11.372 | 11.372 | +1.57 |
| telescope | polled at 20 Hz | 20 | +0.686 | +4.969 | +10.356 | 10.356 | +24.84 |
| telescope | polled at 20 Hz | 100 | +0.749 | +6.812 | +12.489 | 12.489 | +6.81 |
| telescope | polled at 20 Hz | 500 | +0.832 | +5.892 | +11.646 | 11.646 | +1.18 |
| camera | idle | 20 | +1.058 | +6.604 | +10.881 | 10.881 | +33.02 |
| camera | idle | 100 | +1.127 | +7.252 | +10.796 | 10.796 | +7.25 |
| camera | idle | 500 | +1.290 | +7.357 | +11.414 | 11.414 | +1.47 |
| camera | exposing | 20 | +0.639 | +5.137 | +10.644 | 10.644 | +25.68 |
| camera | exposing | 100 | +0.785 | +6.579 | +11.154 | 11.154 | +6.58 |
| camera | exposing | 500 | +0.402 | +5.842 | +11.024 | 11.024 | +1.17 |

Overall: min +0.402, mean +6.503, max +12.489 ms; every sample is late, none early. The standard deviation per row is 2.3 to 3.6 ms; with 12 samples per direction p95 and p99 equal the maximum.

The error is the lateness of the timed wake-up, not of the driver: the driver looks at `IsPulseGuiding` for the first time one duration after `PulseGuide` returned, and a bare `pthread_cond_timedwait_relative_np` loop on the same host at the same time was late by 0.65 to 10.16 ms (mean 5.1 to 7.2 ms). The share of the driver is request → device entry 0.08 to 0.24 ms mean (max 1.6 ms) and check → published completion 0.15 to 0.36 ms mean (max 1.0 ms). The workload made no visible difference.

### 7.4 Core hardening after wave 3 (2026-10-02/03, WP5b)

All five class packages reported the same weaknesses of the core and of the recipe in `indigo_system_alpaca_private.h`. One subagent fixed them in the core and moved every class module onto one pattern.

- **One lock per Alpaca device.** `alpaca_private_data.mutex` (recursive, shared by the primary and the secondary device) replaces the seven class mutexes of wave 3. It is never held across a request to the device or a call into the bus. `system_alpaca_accept()` does what `INDIGO_COPY_*_PROCESS_CHANGE` did (BUSY guard, refusal callback, copy, BUSY, publish, queue the handler) under that lock; `on_poll` and the finalizers ask the device first and then check and write a property in one step under the lock. `CONNECTION` and the session flags are written under it too.
- **`system_alpaca_is_active()`** replaces `IS_CONNECTED` in the core and in every handler, finalizer and enumeration of every module.
- **DeviceState failures.** An error answer to `devicestate` that is neither a transport error nor NotConnected no longer skips `on_poll`: the members are read one by one in that tick. After 5 such ticks in a row `devicestate` is not used for the rest of the connection, with an ALERT message; a new connection tries it again. 5 rather than 3, so that a short burst of HTTP 500 does not downgrade the polling for good. On older devices an error answer to `connected` no longer skips `on_poll` either. Focuser, rotator, wheel, dome and light box now show ALERT on the main polled property while it can not be read, keep the last value and return to OK on the next good read, as mount, camera, switch, weather and safety already did.
- **Failure reason.** `system_alpaca_reason()` captures the text right after the failing request, `system_alpaca_finish_with()` and `system_alpaca_report()` publish it; the per-module workarounds of FOC-1, ROT-1, DOME-5, LB-6 and MOUNT-1 are gone.
- **HTTP 400.** `system_alpaca_not_implemented()` takes HTTP 400 as "not implemented" only for a request without parameters; for a method with parameters it is a rejected value (CORE-8).
- **Recipe.** The header comment of `indigo_system_alpaca_private.h` describes this pattern, with a section on threads.
- **Behaviour changes** besides the defects of section 8: a camera request for an exposure setting whose own property is still BUSY is dropped like any request to a BUSY property, instead of being rejected with "Exposure in progress"; switch requests for class properties while disconnected go to the base class; when a guide pulse fails while a newer request waits, only the message is published and the property stays BUSY; a mount UTC request dropped while busy no longer replaces the pending time.
- **14 new cases**: `interface_device_state_failures`; `<class>_steady_state_is_silent` for focuser, wheel, rotator, switch, weather, safety and camera, each with a Platform 7 and an older device (no property published over 25 poll ticks, a change on the device is); `focuser_unreadable_position`, `wheel_unreadable_position`, `rotator_unreadable_position`; `rotator_rejected_parameter`, `switch_rejected_name`; `ccd_requests_in_a_row`. The harness got `sa_steady()`. Each regression case was shown to fail with its defect put back in a private copy.

Evidence (subagent; live tree unless noted):

| Run | Result |
|---|---|
| `INDIGO_TEST_JOBS=4 ./build/integration/test_system_alpaca_simulator`, 7 runs | 236 run, 236 passed each |
| the same with `INDIGO_TEST_JOBS=1` | 236 / 236 |
| strict build (private tree) | no warning in driver or `integration/system_alpaca/` files; 27 `-Wunused-function` in the shared harness headers |
| ASan + UBSan, whole suite (private tree) | 236 / 236, no report |
| TSan, whole suite, 4 runs (private tree) | 236 / 236, no report, no suppression. Before the change: 4 reports, 218 / 222 |
| `test_system_alpaca_http`, `test_system_alpaca_json` | 44 / 44, 48 / 48 |

TSan does not see accesses inside the uninstrumented `libindigo` (for example `indigo_ccd_*_cleanup`, `copy_values`); those were checked by reading. Small edge cases left as they are: a CONNECTION request accepted between the publication of a failed or lost connection and the call of the base class; a failing mount MoveAxis handler clears the items of a newer waiting motion request; a dome coordinates request is accepted while `DOME_STEPS` is BUSY; the recovery of an unreadable focuser position also clears an ALERT left by an earlier failed move. The recursive mutex attribute is POSIX; the Windows build uses pthreads4w, which has it (`agent_scripting` uses it too); not built on Windows.

**Recorded run** (orchestrator, 2026-10-03 01:18): `python3 tools/run_driver_test.py system_alpaca`, **280 / 280 OK** (`test_system_alpaca_simulator` 236 and `test_system_alpaca_http` 44; the script does not count the unit test `test_system_alpaca_json`). Recorded in `README.md` and `TEST_SUMMARY.md`.

### 7.5 OmniSim tier (2026-10-03, WP11)

The driver against an independent Alpaca implementation, ASCOM OmniSim 0.5.0 (`0.5.0+05d607826b39bdee2bcbb09bcaac6f35b3c6dba9`), installed on the development Mac as section 4.1 describes.

- **Sources:** `indigo_test/integration/test_system_alpaca_omnisim.c` (20 cases, runner) and `indigo_test/integration/system_alpaca/omnisim_test_common.h` (harness on top of `system_alpaca_test_common.h`). Opt-in target `make -C indigo_test test-system-alpaca-omnisim`; not in `INTEGRATION_TESTS` or `OPT_IN_DRIVER_TESTS`, so the ordinary recorded run does not depend on OmniSim; nothing is downloaded.
- **Finding OmniSim:** `INDIGO_TEST_OMNISIM`, else `omnisim/*/ascom.alpaca.simulators` next to the working tree (until 2026-10-03 inside `system_alpaca_simulator/`). Without it the tier prints NOT RUN, plans no case and exits 0; the recording script then records nothing.
- **Isolation:** OmniSim's own reset does not restore everything: the rotator kept interface version 2 and its position, and a halted cover stayed Unknown. So every case starts a fresh instance on a free loopback port, added as a manual server (no broadcast). The instance gets a private home under `/tmp/indigo-system-alpaca-omnisim.*`, which is also its working directory, with `HOME`, `CFFIXED_USER_HOME` and `ASCOM_LOGPATH` inside that home and `XDG_CONFIG_HOME` and `XDG_DATA_HOME` unset. Until 2026-10-03 only `HOME` was set; on macOS the profiles and logs then went to the real user's `~/Library/Application Support/ascom` and `~/Documents/ascom`, because .NET 8 takes those folders from Foundation, which ignores `HOME`. A site latitude set by one case survived into the next run, and `omnisim_mount_manual_motion` had to restore it; that workaround is gone. A run now leaves `~/Library/Application Support/ascom`, `~/Documents/ascom`, `~/.config`, `~/.dotnet` and `~/.aspnet` unchanged, checked by comparing listings and SHA-256 of all files before and after three runs. `TMPDIR` is left alone: OmniSim's single-instance check (a named pipe in `$TMPDIR`, a named mutex in `/tmp/.dotnet`) must still see an instance of the user. OmniSim is single instance per host: a run holds an exclusive `flock()` on the executable, cases run serially with a 300 s watchdog, and a reaper process kills the instance when its case ends or dies.
- **Recording:** `python3 tools/run_driver_test.py system_alpaca --hw --target test-system-alpaca-omnisim`, without `--type`: the suite writes the device record `OmniSim 0.5.0` from `management/v1/description`, which becomes its own `README.md` line. With `--type` and no OmniSim the script would record "0/0 Failed".
- **Cases:** `omnisim_discovery` (discovery to 127.0.0.1:32227 finds the instance with all 10 devices), `omnisim_focuser` / `_legacy`, `omnisim_wheel` / `_legacy`, `omnisim_rotator` / `_legacy` (Platform 7 and an older interface set through OmniSim's settings API), `omnisim_mount_goto`, `omnisim_mount_park_home_tracking`, `omnisim_mount_manual_motion`, `omnisim_mount_guider`, `omnisim_camera_image`, `omnisim_camera_abort`, `omnisim_dome` / `_legacy`, `omnisim_lightbox`, `omnisim_switch`, `omnisim_weather`, `omnisim_safety` / `_legacy`. Each one adds the server, selects, attaches, connects, runs the operations on real time, disconnects, detaches and checks that the Alpaca device was disconnected (D5).
- **Results:** 4 runs of 20 / 20 in a row (one through the script with `--dry-run`, three with `make`), about 236 s each; recorded run of 2026-10-03 02:21: **20 / 20 OK**. With the private-home isolation of 2026-10-03: three runs of 20 / 20 (16:15, 17:29, 17:34), the real home locations unchanged after each.

Questions settled against OmniSim:

- **Image orientation:** OmniSim's `Value[x][y]` has its origin top left, and the image the driver publishes equals it pixel by pixel at row y, column x. The correlation with OmniSim's source picture `m42-800x600.jpg` is 0.9995 as published, against 0.861 flipped vertically, 0.691 horizontally and 0.648 both. Plane order could not be checked: OmniSim's camera is mono and can not be configured.
- **CanFastReadout and ReadoutModes** are mutually exclusive in OmniSim too (default camera: CanFastReadout false, ReadoutModes `["Default"]`); this confirms SIM-1.
- **Rotator TargetPosition:** OmniSim reports the mechanical target, without the sync offset, also for Move and MoveAbsolute; the standard defines it as the sky angle, so that is an OmniSim defect, which OMNI-2 makes the driver independent of.
- **Dome AbortSlew while the shutter moves** leaves ShutterStatus Error; **HaltCover** leaves CoverState Unknown; both as our simulator models them.
- **PulseGuide Duration=0** is accepted and ends a running pulse of the same axis only; negative durations are 0x401.
- **MoveAxis** acts on the mechanical axes: a positive secondary rate raises the declination on pierEast and lowers it on pierWest, on a northern and a southern site alike, and a positive primary rate moves west on both sides; the driver compensates the secondary axis (D13), checked by `omnisim_mount_manual_motion` on both sides and both hemispheres.
- **Members of a later interface version** are answered with HTTP 200 and ErrorNumber 0x4FF ("requires Interface Version N"), not 400 or 0x400; the driver gates those members by interface version and never sends them.
- **devicestate** content per type matches what the class modules read; the camera of OmniSim is ICameraV3 and has none.
- **SafetyMonitor** answers IsSafe of a disconnected device with 0x407 (OMNI-3).

Not exercised by OmniSim: fault injection of any kind; a colour or Bayer camera, a camera that can pulse guide, CanFastReadout true, gain, offset, set point, a Platform 7 camera; older interfaces of Telescope, Switch, CoverCalibrator, ObservingConditions and Camera (no settings API); broadcast discovery (deliberately not used). Possible through the settings API but not done yet: relative focuser, CanHalt false, wheel without names, roll-off dome, dome slaving.

Simulator changes from this tier: the focuser keeps IsMoving for SettleTime after Halt (default 0), the rotator option `TargetIsMechanical` models OmniSim's TargetPosition, and SafetyMonitor answers IsSafe of a disconnected device with NotConnected by default (`NotConnectedError=false` keeps the old behaviour). The deterministic suite has 238 cases after it.

### 7.6 Independent review and its fixes (2026-10-03, WP12 and WP12b)

**Review (WP12).** A subagent that wrote none of the code read the whole driver, its tests and its registration against `AGENTS.md`, `indigo_drivers/AGENTS.override.md`, `indigo_test/AGENTS.md`, `DRIVER_TESTING_RULES.md` and the Alpaca specification: 3 major, 22 minor and 3 style findings (REV-1 to REV-28), all by reading. Found sound: the HTTP client, the transport (casing, encoding, transaction ID, ErrorNumber before Value, no replay of non-idempotent PUTs), locale-independent numbers, the JSON parser (non-recursive, depth limit, bounded allocation, UTF-8), discovery and the proxy-loop filter, the lock order and that the device lock is never held across a request or a bus call, the ImageBytes path, the guider units, the formatting rules, `static` symbols (`nm`: no collision with `agent_alpaca` or other archives), the `PROPERTIES.md` names, the Xcode registration, and no OS-specific code in the driver. Nothing in section 2 was found to misread the specification.

**Fixes (WP12b).** Every finding was first tried by a test. The sources before the fixes were frozen, and every regression case was run against the old driver with the new case headers, where it fails at the check of its defect (logs kept in the session scratchpad). Results per finding are in section 8. REV-7 (Sync does not switch tracking on) and the `MOUNT_MOTION_DEC` sense on pier side West were open decisions of the user and were not changed; REV-16 (the `" @ "` in proxy names) was examined but not changed (below). All three were decided on 2026-10-03 (D12, D13, D14).

- **Platform 7 disconnect** (from the OmniSim tier): a user disconnect now waits, by a finalizer bounded by the establish timeout, until `Connecting` is false; `CONNECTION` stays BUSY meanwhile, a connect request is ignored and SHUTDOWN is refused. A timeout ends disconnected with ALERT. Transport loss, a failed connect and detach do not wait. Covered by `core_connect_disconnect_platform7` and `core_shutdown_with_connected_device`; against OmniSim the disconnect took 1.11 s.
- **Simulator made stricter** (REV-25): a device-API PUT whose body is not `application/x-www-form-urlencoded` is HTTP 400 (`--any-content-type` restores the lenient behaviour), `--error-value` adds a Value to error replies as the ASCOM .NET servers do, and the fault `missing-transaction-id` exists. The whole suite runs against the strict content-type check.
- **Harness:** queue introspection and gates, sequence helpers, message marks so that a message check can not match an older identical message, `sa_disconnect_after()`; the poll-versus-pending-request cases are ordered by conditions and sequence numbers instead of sleeps. About 70 `indigo_usleep` calls remain in negative checks ("nothing happens within 300 ms") that the review did not cite.
- **Behaviour changes:** a switch named in a request is always sent, also when the cached value already equals it (REV-10; three assertions that encoded the defect were changed); a UniqueID in which a character was replaced gets a hash in its `X_ALPACA_DEVICES` key (REV-18), so a saved selection of such a device is lost once; the HTTP client rejects any transfer coding other than chunked (REV-17).
- **`connect_timeout`** of `test_system_alpaca_http` is registered only where it can run, so the HTTP suite counts 43 on macOS instead of 44.
- **REV-16:** `indigo_trim_local_service()` has no caller in the repository. A concrete problem exists in the routing of the bus (`indigo_bus.c`, `indigo_use_host_suffix` on by default): a request for the proxy "Focuser @ OmniSim" was also delivered to a device of a remote service named "Omni", because the match is a substring search, and would be forwarded as "Focuser". It needs `indigo_server` with a remote connection whose service name is a prefix of the Alpaca ServerName. Decided by the user as D14 and implemented on 2026-10-03: proxies are named `"<DeviceName> on <ServerName>"` and every `@` of the names is replaced by `-`; `core_proxy_names_without_at` and `core_proxy_names_routing` show in-process that a request for a proxy no longer reaches a device named like a remote service.
- **REV-15, CONFIG part:** the CONFIG state is written by the restore handler of the framework without a lock the driver could share; a stale read only defers work by 0.1 s. Not fixable in the driver (D10).

Evidence (subagent, final sources):

| Run | Result |
|---|---|
| `INDIGO_TEST_JOBS=4 ./build/integration/test_system_alpaca_simulator`, 5 runs; `INDIGO_TEST_JOBS=1`, 1 run | 254 / 254 each |
| ASan + UBSan, whole suite (private tree) | 254 / 254, no report |
| TSan, whole suite, 3 runs (private tree) | 254 / 254, no report, no suppression |
| strict build | no warning in driver, simulator or `integration/system_alpaca/` files; 27 `-Wunused-function` in the shared harness headers |
| `test_system_alpaca_http`, `test_system_alpaca_json` | 43 / 43 (`connect_timeout` skipped on macOS), 48 / 48 |
| `make -C indigo_test test-system-alpaca-omnisim` | 20 / 20, OmniSim 0.5.0 |

**Recorded runs** (orchestrator, 2026-10-03): `python3 tools/run_driver_test.py system_alpaca` at 04:17, **297 / 297 OK** (254 + 43); `python3 tools/run_driver_test.py system_alpaca --hw --target test-system-alpaca-omnisim` at 04:18, **OmniSim 0.5.0 20 / 20 OK**. Both in `README.md` and `TEST_SUMMARY.md`.

### 7.7 Decisions D13 and D14 (2026-10-03)

- **D13, manual motion:** `mount_move_axis()` reads SideOfPier when a non-zero motion starts (a stop reads nothing; a failed read uses the side of the last poll; an unknown or unimplemented side gives no inversion) and inverts the secondary rate on pierWest. The primary axis is unchanged: `WEST` is a positive rate on both sides. Measured on OmniSim 0.5.0 with raw MoveAxis on sites at 48° and −30° latitude: a positive secondary rate raises the declination on pierEast and lowers it on pierWest in both hemispheres, a positive primary rate lowers RA on both sides. New case `mount_manual_motion_sense` (both sides reached by a sync of another client, both hemispheres, fallback of a failed read, no re-send of a running motion, a telescope without SideOfPier); `mount_manual_motion` and `omnisim_mount_manual_motion` assert the new sense. All three fail with the previous mount module.
- **D14, names** (the format was replaced by D17 the same day, the `@` rule still holds): `"<DeviceName> on <ServerName>"`; every `@` of the two names is replaced by `-`, because the framework parses any `@` in a device name (bus routing by substring, the XML writer cuts at the last `@`, `indigo_filter.c` treats such a name as remote). The worst-case length is 125 bytes, below `INDIGO_NAME_SIZE`. New cases `core_proxy_names_without_at` and `core_proxy_names_routing` attach devices named like remote services (`"@ Omni"`, ...) and show that requests for the proxies no longer reach them, while a control request for the old name format does. The labels of the `X_ALPACA_DEVICES` items still read `"Name (Type) @ Server"`; they are labels and never routed.
- Evidence (subagent): the suite 257 / 257 three times with `INDIGO_TEST_JOBS=4` and once with `=1`; ASan + UBSan and TSan of the whole suite without a report; strict build clean in our files; OmniSim tier 20 / 20.
- **Recorded runs** (orchestrator, 2026-10-03): simulator at 09:55, **300 / 300 OK** (257 + 43); OmniSim 0.5.0 at 09:57, **20 / 20 OK**.
- Limit: a real mount whose Alpaca driver already compensates the pier side, or mirrors the secondary axis differently from OmniSim, would move the wrong way on pierWest; the driver can not detect this.

### 7.8 Decisions D15 to D17 and the loopback discovery fix (2026-10-03)

Found by the user: with OmniSim running on the same Mac (`--urls=http://127.0.0.1:32323`) nothing appeared in INDIGO. Two causes: broadcast discovery never reached a server listening on 127.0.0.1 on macOS (D16), and every device had to be switched on by hand (D4, replaced by D15). The user also found the proxy names too long (D17).

- **D16, `indigo_uni_discover()`** (`indigo_libs/indigo_uni_io.c`): with `include_loopback` the request also goes to `127.0.0.1`; macOS has no broadcast on `lo0`, so `127.255.255.255` alone never reached a local responder. A local responder may now answer twice on Linux; the driver counts one server. Regression case `discover_reaches_loopback_responder` in `indigo_test/unit/test_uni_io.c` (fails before the fix, passes after, 3 runs); driver case `core_broadcast_discovery_on_loopback` (no targets, no manual server; fails when linked against the old `indigo_uni_io.c`). `focuser_askar` calls the function without `include_loopback` and is not affected. Linux not run.
- **D15, devices on by default:** the driver remembers the UniqueIDs the user switched off; everything else is proxied. A switched-off device stays listed (so CONFIG SAVE keeps its Off item) and is never attached, not even during the restore of the saved configuration at start: reconciliation waits while the CONFIG restore of the bridge is BUSY (`core_switched_off_never_attached_at_start`, 5 restarts, 0 requests to the switched-off devices; fails with the wait removed). An old saved configuration keeps what was on on and what was off off; devices not in it come up on. Beyond 32 proxies further devices wait as `FAILED` with a message on the bridge and get a proxy as soon as another device is switched off. The harness switches off every simulator device by default so class cases still control which proxies exist.
- **D17, names:** `"ALPACA <DeviceName>"` with every "alpaca" removed in any case (also inside a word), spaces and dangling `-`, `_`, `:` tidied, `@` replaced, the DeviceType if nothing is left; duplicates `#2`, `#3`, … by UniqueID order among the devices of that name, a proxy keeps its name while attached. Every proxy starts with `ALPACA `, so none can equal the bridge `Alpaca`.
- New cases: `core_default_on_after_discovery`, `core_switch_off_persists_across_restart`, `core_device_added_at_run_time`, `core_old_saved_configuration`, `core_capacity_default_on`, `core_switched_off_never_attached_at_start`, `core_broadcast_discovery_on_loopback`, `core_proxy_names_strip_alpaca`, `core_proxy_names_numbered`; all D15 / D17 cases fail against the previous driver. Found and fixed during the verification: an overlapping `strcpy` (ASan) and a late startup reconciliation retrying an attach (TSan).
- Evidence (subagent): the suite 265 / 265 three times with `INDIGO_TEST_JOBS=4` and once with `=1`; ASan + UBSan and TSan of the whole suite without a report; strict build clean in our files; `test_system_alpaca_http` 43 / 43.
- **OmniSim moved** at the user's request to `omnisim/` next to the working tree (section 4.1); the `.gitignore` rule is gone.
- **Recorded runs** (orchestrator, 2026-10-03): simulator at 14:36, **308 / 308 OK** (265 + 43); OmniSim 0.5.0 at 14:37 from the new location, **20 / 20 OK**.

### 7.9 Linux and Windows (2026-10-03)

- **Linux arm64** (indigosky, Raspberry Pi 5, Debian 12, GCC 12.2.0, root on the SSD `/dev/sda2`): the repository pulled at `dfc745362`, `make all` (a full clean build, because framework headers had changed) finished with exit 0 and without a warning or an error in any `system_alpaca` file at the default flags. Recorded run `python3 tools/run_driver_test.py system_alpaca`: **309 / 309 OK** (`test_system_alpaca_simulator` 265, `test_system_alpaca_http` 44, because `connect_timeout` runs on Linux). `test_system_alpaca_json` and `test_uni_io` (with `discover_reaches_loopback_responder`, where both loopback targets reach the responder) pass as well. The run started at 14:16 UTC on the Pi and is recorded as 16:16 in the local time of the Mac, like every other line. Not run on Linux: sanitizers, the OmniSim tier (no OmniSim installed there), x86_64.
- **Windows:** `indigo_system_alpaca.vcxproj`, `.vcxproj.filters` and `.vcxproj.user` made from the `agent_alpaca` project (15 sources without `_main.c`, 5 headers, x64 and ARM64, linked with `ws2_32`, `pthreadVC3`, `indigo`), and the project added to `indigo_windows.sln`. **Not built**: there is no Windows machine here, so it is not yet a dependency of the `indigo` project (steps 13 and 15 of the Windows procedure in `MIGRATION_STATUS.md`). A scan of the driver for calls MSVC lacks found none (`strcasecmp` is mapped by `indigo_uni_io.h`, `pthreads4w` has recursive mutexes).

### 7.10 NYX-101 hardware fixes (2026-10-03)

The user reported that with a Pegasus NYX-101 the first slew through the mount agent only unparked the mount and the next one moved it, without any message. It was reproduced on the real mount (Alpaca server "NYX101", PegasusAstro v1.1, firmware 1.32.1, ITelescopeV2, at 192.168.111.195:80) through the mount agent and the proxy, 3 times out of 3, while the same mount through USB and `mount_lx200` reached every target on the first slew (3 of 3). Defects NYX-1 to NYX-4 in section 8; the mount agent was fixed at the same time (`agent_mount` 3.0.0.26).

- **Simulator:** the telescope module models the NYX with the options `StartDelay` (park and find home start late; until then Slewing, AtPark and AtHome are false) and `QuietParkSlew` (Slewing false while a park or find home moves), plus the existing `connection-close` fault with `Count=-1` (the NYX closes the connection after every reply). Both options are off by default.
- **Intentional behaviour change:** park, find home and pier flip no longer make `MOUNT_EQUATORIAL_COORDINATES` BUSY; a coordinates request is never dropped without an answer.
- **Evidence** (subagent, deterministic): the suite 269 / 269 three times with `INDIGO_TEST_JOBS=4` and once with `=1`; ASan + UBSan and TSan without a report; strict build clean in our files; `test_agent_mount` 72 / 72 three times; OmniSim tier 20 / 20; all four new mount cases and both new agent cases fail against the previous code.
- **Hardware** (real NYX-101 through Alpaca and the mount agent, private `HOME`): 4 of 4 park → first slew cycles (Vega, Deneb, Altair, Arcturus) reached the target with `MOUNT_PARK` OK every time and no "Park failed"; the slew request reached the NYX only after the unpark had finished. A target below the horizon (Fomalhaut, altitude −13°) ended in agent ALERT "Mount did not reach the target", because the NYX accepts it silently. The site set through the proxy (48.2236 / 16.9844 / 180) persists across a reconnect. The site 0/0 seen earlier was written by the mount agent itself: its own site was 0/0 in the private `HOME` and its site source was HOST, so it pushed 0/0 to the mount when the mount was selected.
- **Handler run time:** a request to an Alpaca server, and the UDP discovery, take as long as the network and the server need (0.5 s per request on the NYX, which closes the connection after every reply; 2 s for a discovery cycle), so the framework logged "Handler queue task ... took ..., longer than 0.100s limit" for nearly every task. At the user's request the HTTP client (`alpaca_http_request()`) and `bridge_discover()` set the run time limit of the running task to unlimited (`indigo_set_handler_max_run_time(0)`, a no-op outside a queue task). Recorded run 2026-10-03 19:20: simulator 312 / 312 OK, no such message in the log.
- **Mount agent and an unset site** (decided by the user, `agent_mount` 3.0.0.27): the agent no longer writes a site of 0° N, 0° E to the mount or the dome; it warns once while a mount or dome is selected ("The site is 0° N, 0° E, most likely not set; it is not written to the mount or the dome"). Case `filter site unset`; recorded run 73 / 73 OK.
- **NYX firmware observations** (to report to Pegasus, not changed in INDIGO): `SlewToCoordinatesAsync` to a target below the horizon returns success and does nothing; Slewing goes false about 1 s before AtPark becomes true, and in one park it was false for most of the motion; the server closes the connection after every reply; Azimuth sometimes reads as a denormal number (1.5e-312) at the pole; no `devicestate` (ITelescopeV2).

### 7.11 Hardware suite (2026-10-03)

`indigo_test/hardware/test_system_alpaca_hw.c`, target `make -C indigo_test test-system-alpaca-hw`, recorded with `python3 tools/run_driver_test.py system_alpaca --hw --port <host:port>[,…]`.

- **Selection:** the servers named by `SYSTEM_ALPACA_HW_PORT` / `--port` (several separated by commas; nothing is broadcast), otherwise one broadcast discovery. `SYSTEM_ALPACA_HW_DEVICES` keeps the devices whose DeviceName or UniqueID contains one of its texts; the others are switched off in `X_ALPACA_DEVICES`.
- **Coverage:** every function a device advertises (Can* flags, interface version, implemented members) is a case `<alpaca type>_<proxy>_<function>`, driven through the proxy properties and read back through the proxy and directly over Alpaca HTTP. A function the device does not advertise is printed as SKIP with the reason. Per class: telescope (connect and D5, site, time, unpark followed at once by a goto, tracking and rates, guide rates, slews on both sides of the meridian with arrival checked, side of pier, abort, measured axis convention and settle time, manual motion on both pier sides, pulse guiding in four directions with a duration check, sync and sync back, find home, set park and restore, park with a refused goto, restore); camera (geometry, exposure with an image check, binning and subframe, abort, cooler and set point, gain, offset, readout modes, fast readout, guider); focuser; filter wheel; rotator; dome (including shutter, set park and sync); cover calibrator; switch (every writable switch written and restored); observing conditions (plausibility of every sensor, average period, refresh); safety monitor.
- **No permission gates** (user's decision): sync, set park, the dome shutter and switch writes are always tested and restored; a telescope is left as found; a device another client had connected gets `Connected=True` again.
- **Recording one line per device** (user's decision): each case is tagged with its device (`case_device` record, `hw_record_case_device()` in `hardware_device_record.h`), and `tools/run_driver_test.py` writes one `README.md` line per device model with that device's own counts. Suites without tags record as before. Documented in `indigo_test/AGENTS.md`.
- **Mount settings instead of model names** (user's decision, no model name in the driver): `X_ALPACA_MOUNT_AXES` (`MECHANICAL` default, `SKY`) and `X_ALPACA_SETTLE_TIME` (s, default 0), saved by CONFIG. The suite measures both: OmniSim needs `MECHANICAL` and 0 s, the NYX-101 needs `SKY`. Its settle time measured 0 s in the recorded run, but direct measurements ignored a goto 0 s and 1 s after a slew and executed it after 2 s, so the NYX's behaviour is intermittent and 2 s is the setting to use.
- **Defects** HW-1 to HW-5 in section 8, each with a regression case that fails against the previous module; simulator options `SplitAxisRates`, `LateSlewing`, `SkyAxes`, `IgnoreAfterSlew`, `LateSettle` / `LateOvershoot` (all off by default).
- **NYX-101 firmware observations:** Connected=False is accepted but Connected stays true; CanSetGuideRates is true but the setter answers HTTP 400 / 0x400; the azimuth at the pole is arbitrary; the first MoveAxis after a slew is sometimes ignored.
- **Recorded runs:** OmniSim 0.5.0 through the suite, 10 devices, 81 / 81 OK (2026-10-03 21:31, one line per device); PegasusAstro NYX-101 20 / 20 OK (21:36), after failed recorded runs at 20:33, 20:41 and 20:53 while the suite and HW-3 / HW-4 were being fixed; deterministic suite 317 / 317 OK (21:44). The model names of the OmniSim devices are their Alpaca Descriptions, which OmniSim fills with long sentences.
- **Askar-WAF focuser** (Alpaca server 1.0.7, IFocuserV2, absolute, MaxStep 1000000), a run by another user on Linux x64 (2026-10-03 23:35, recorded in that user's checkout): everything passed except `focuser_..._absolute_move` (a move to 984 ended at 986), HW-5; deterministic suite 318 / 318 OK after the fix (22:44).
- **Not covered on hardware:** every class except the telescope (no such Alpaca device available), camera gain, offset, readout modes and guiding (the OmniSim camera has none), dome slaving, a relative focuser, broadcast discovery, Linux.

### 7.12 ConformU through agent_alpaca (2026-10-04)

`tools/alpaca_omnisim_conformu.py` runs ConformU on every OmniSim device twice: directly and through `system_alpaca` → `agent_alpaca`. A problem only the second run has is a loss in the INDIGO path. It found CHAIN-1 and CHAIN-2 here and AGENT-26 to AGENT-32 in the agent (`../agent_alpaca/REFACTOR.md`, sections 12 and 13). The bridge starts from a written configuration: discovery disabled, OmniSim as the only server.

## 8. Found defects

All were originally found **by source audit only**. Under decision D8 they were fixed on 2026-09-24 and verified:

- **AGENT-1..3** were reproduced and fixed. The full record is in [`../agent_alpaca/REFACTOR.md`](../agent_alpaca/REFACTOR.md), which also lists ten further defects that the ConformU baseline found (AGENT-4..13).
- **LIB-1** is fixed by `indigo_uni_open_client_socket_with_timeout()`.
- **LIB-2** is fixed by the portable `indigo_uni_discover()`. `focuser_askar` now uses it (version 3.0.0.7), and its platform-specific code is removed.
- **Regression tests:** 8 new cases in `indigo_test/unit/test_uni_io.c`, all passing. The `focuser_askar` simulator suite passes 10/10 on Linux x64.
- **Coverage gap:** the askar discovery glue (reply parsing, deduplication) has no automated test. It uses the fixed port 7676 and broadcast, so it can't be tested hermetically.
- **Pre-existing failures:** `test_timer` fails 2–4 fork-related cases, and fails the same way with the original `indigo_uni_io`. Unrelated.

The table below keeps the original audit record.

Found during the implementation (2026-10-01):

| ID | Location | Impact | Cause | Fix | Regression test |
|---|---|---|---|---|---|
| LIB-3 | `indigo_libs/indigo_ccd_driver.c:242`, `indigo_ccd_detach()` | Every detach of a CCD device leaks 4096 bytes. Reproduced with `leaks` on `test_system_alpaca_simulator`; it matters more for this driver than for others, because proxies are attached and detached at run time. | `CCD_FPS_PROPERTY` is created in attach and never released in detach. | **Not fixed**: a framework change, outside D10. Reported to the user. | none |
| CORE-1 | `indigo_system_alpaca.c` (first version, never committed) | A bus request during a list rebuild deadlocked the driver. | The driver queue published properties while holding the lock that a bus callback also takes. | The driver queue only edits items under the lock; the dynamic properties are allocated at full size so they never move. | `core_bus_requests_during_list_changes` |
| CORE-2 | same | SHUTDOWN called from a bus callback during a discovery cycle could deadlock. | Unbounded wait for the cycle. | The wait is bounded (5 s) and returns `INDIGO_BUSY`. | `core_shutdown_from_bus_callback` |
| CORE-3 | same | A restored `X_ALPACA_DEVICES` selection took 5 s and ended in ALERT. | The restore request was not answered. | Answered at once; the harness requires a prompt OK restore on every driver start. | every case, through the harness |

Further corrections of the first version of the core, each covered by a core case: coalescing of selection and server requests, the transaction-ID tolerance (`core_transaction_id_policy`), a lost reattach on a second refresh pass, D5 skipped after a single transport error, and text truncation splitting a UTF-8 character (`core_unusual_identifiers`).

Found in wave 3 (2026-10-02). "Reproduced" means a case failed before the fix and passes after it; the others were found by reading. All fixes are in the class module of the same name unless a simulator or case file is named. Each ID is the one used in section 7.2.

| ID | Location | Impact | Cause | Fix | Regression case |
|---|---|---|---|---|---|
| TEST-1 | `interface_cases.h` `interface_capability_cache` | Baseline failure. | The case connected with the real focuser class, which reads `position` and `temperature` itself, so the request counts no longer matched. | The case installs the test class before connecting, like its neighbours; the count assertions are unchanged. | the case |
| T1 | `switch_cases.h` `switch_start_number` | Baseline failures of `switch_async_abort` and `switch_async_failures`. | The case advanced the simulator clock before the PUT had arrived. | Wait for the request. | the two cases |
| T2 | `switch_cases.h` `switch_lifecycle` | Baseline failure. | The expectation ignored a state change made earlier in the case. | Corrected and made stricter. | the case |
| FOC-1, ROT-1, DOME-5, LB-6, MOUNT-1 | focuser, rotator, dome, light box, mount: start of an operation | After a failed request that may have executed, the ALERT message lost the reason of the device ("Move failed: server error" without the HTTP text). Reproduced. | `system_alpaca_finish()` prints the text of the last request of the channel, and a read of the completion member had replaced it. | The reason is kept right after the failing request. | `focuser_request_failures`, `rotator_request_failures`, `dome_request_failures`, `lightbox_request_failures`, `mount_sync`, `mount_request_failures` |
| FOC-2 | `focuser_start` / `focuser_finish` | A relative focuser accepted one move per connection; every later one was refused with "Another motion operation is pending". Reproduced. | The hidden `FOCUSER_POSITION` was set BUSY and never reset. | Its state is touched for absolute focusers only. | `focuser_relative_focuser` |
| FOC-3 | focuser `on_poll` | `FOCUSER_MODE` stayed in ALERT after a failed mode change although the poll followed the device. Reproduced. | State not set when the poll applies the mode. | Set OK. | `focuser_temperature_compensation` |
| FOC-4, ROT-4, WHL-3, D2, D5, CCD-2 | `on_poll` of focuser, rotator, wheel, switch, weather, camera | A request of a client accepted while the poll hook waited for a reply was overwritten by the polled value: the handler sent the old position or value, the request was lost and published OK. Reproduced (TSan also reported the race). | The guard was checked before the requests of the hook and the assignment came after them; nothing was atomic against the bus thread. | Ask the device first, then check and assign under a class mutex; requests are accepted under the same mutex. Camera: a counter of pending set-point requests. | `focuser_pending_change_survives_poll`, `rotator_pending_change_survives_poll`, `wheel_polling`, `switch_pending_change_survives_poll`, `ccd_cooling` |
| FOC-5, ROT-5 | abort handlers | When Halt failed during a move the driver had not started, the property went to ALERT although the device was still moving. Reproduced. | Settled whatever Halt answered. | Stays BUSY and follows the device. | `focuser_polling`, `rotator_polling` |
| WHL-1 | `wheel_slot_finalizer` | A move that ended in another slot stayed BUSY until the long timeout. Reproduced. | Completion was "Position equals the requested slot", not the specified "Position is not −1". | Completion is Position ≠ −1; another valid slot ends in ALERT with the real slot. | `wheel_move_ends_elsewhere` |
| WHL-2 | `wheel_slot_handler` | A PUT `position` with an unreadable or late reply ended in ALERT although the wheel was turning. Reproduced for HTTP 500. | No check whether the request executed. | Position is read after such a failure. | `wheel_request_failures` |
| ROT-2 | `rotator_start` | During a MoveMechanical on a device that leaves TargetPosition alone, `ROTATOR_POSITION` showed a stale target. Reproduced. | TargetPosition was read after every move. | Not read for `movemechanical`. | `rotator_mechanical_move` |
| ROT-3 | rotator sync | A Sync whose reply was lost ended in ALERT. Reproduced. | The PUT was not marked replayable. | `ALPACA_REPLAYABLE`. | `rotator_sync` |
| MOUNT-2, DOME-6, LB-4 | mount, dome, light box `on_poll` | `MOUNT_PARK`, `MOUNT_TRACKING`, `UTC_TIME`, `DOME_SHUTTER`, `AUX_COVER` and `AUX_LIGHT_SWITCH` were published on every poll tick although nothing changed. Reproduced. | `indigo_set_switch()` marks a one-of-many property changed on every call; text properties are always published. | Assign only on a change. | `mount_steady_state_is_silent`, `dome_polling`, `lightbox_polling` |
| MOUNT-3 | goto, sync, coordinate read | RA 17.3 was sent as 17.299999999999997. Reproduced. | `fmod(ra + 24, 24)`. | A value in range is left untouched. | `mount_sync`, `mount_goto` |
| MOUNT-4 | `mount_set_host_time_handler` | `UTC_TIME` stayed in ALERT after a later successful set. Reproduced. | State not reset. | Set OK. | `mount_site_and_time` |
| MOUNT-5 | `mount_change_property` and handlers | Data race on the parked check (TSan, reproduced); a goto requested during a park turned the BUSY coordinates to ALERT (by reading). | The check read property items on the bus thread. | A `parked` flag under a mutex; reject only when not BUSY. | `mount_park_and_home` |
| MOUNT-6 | `mount_park_handler` | Park on a telescope with only CanUnpark could quote the error of an unrelated request. By reading. | A local "unsupported" passed to the common finish helper. | Own message. | `mount_capability_variants` |
| MOUNT-7 | `mount_read_site`, `mount_read_guide_rates` | Members of an unusable pair were read on every settings tick. Reproduced. | No skip. | Skip. | `mount_capability_variants` |
| MOUNT-8 | `mount_move_axis` | A parked refusal inside the handler left the direction item on. By reading. | Items not cleared. | Cleared. | none |
| MOUNT-9 | `mount_utc_time_handler` | A text that is no date was answered "Time failed: request failed". | Generic text. | Explicit message, nothing sent. | `mount_site_and_time` |
| GUIDER-1 | `guider_change_property` / `guider_guide` | A handler could read the items while the bus thread wrote the next request, send nothing and publish OK. By reading. | Targets read on the device queue. | Durations of each request stored under a mutex. | indirectly `guider_replacement_and_axes` |
| CCD-1 | `ccd_bin_handler`, `ccd_mode_handler` | `CCD_FRAME` stayed ALERT after a binning change made it valid again. Reproduced. | Republished without a state. | Publish OK. | `ccd_geometry` |
| CCD-3 | `ccd_exposure_finalizer` | After a failure during an exposure the camera kept exposing and refused the next `startexposure` (0x40B). Reproduced. | No cleanup on those paths. | Abort, else stop, on all failure paths. | `ccd_exposure_progress_failures` |
| CCD-4 | `ccd_abort_exposure_handler` | No way to make a camera idle that is busy with an exposure this connection does not know. Reproduced. | Abort sent nothing without an exposure of its own. | `abortexposure` is passed on when the camera can abort. | `ccd_abort_exposure`, `ccd_transport_loss` |
| CCD-5 | `ccd_normalize_frame` | The frame lost pixels when the binning changed twice, and CONFIG LOAD did not restore a binned subframe (9,6,99,60 came back as 6,6,96,60). Reproduced. | Every normalisation started from the already normalised frame. | The requested frame is kept in the class data. | `ccd_lifecycle` |
| CCD-6 | `ccd_on_poll` | On a camera without CCDTemperature, `CCD_COOLER_POWER` was never polled. By reading. | Early return. | Each property polled on its own. | none |
| CCD-7 | `CCD_FRAME.BITS_PER_PIXEL` | Any value of a client was accepted until the next image. Reproduced. | Never restored. | Restored on normalisation. | `ccd_geometry` |
| DOME-1 | `dome_is_parked()` | A slew accepted before a park request was refused with "Dome is parked". Reproduced. | The guard read the item the bus thread sets on acceptance. | The state of the device is used. | `dome_pending_requests` |
| DOME-2 | `dome_apply_status()` | On devices whose `Slewing` is true while the shutter moves, every shutter move showed an azimuth slew, also on a roll-off roof. Reproduced. | `Slewing` always attributed to the azimuth. | Attributed to the shutter unless the azimuth or altitude is seen to change. | `dome_slewing_includes_shutter` |
| DOME-3, LB-5 | shutter, cover and calibrator finalizers | A move ending before the handler of the next request ran published OK with the old direction, so a client saw its request completed before it started. Reproduced. | Published whatever was waiting. | No publication while a request waits. | `dome_completion_during_pending_request`, `lightbox_pending_requests` |
| DOME-4, LB-1 | dome and light box methods without a parameter | HTTP 400 was not taken as "not implemented"; the request was sent again every time. Reproduced. | Only 0x400 was tested. | HTTP 400 too, for members without a parameter. | `dome_capability_variants`, `lightbox_capability_variants` |
| LB-2 | `lightbox_apply_calibrator()` | The next ON sent a stale brightness target. Reproduced. | Only the value followed the device. | The target follows too when no request waits. | `lightbox_calibrator` |
| LB-3 | `lightbox_calibrator_finalizer()` | The message named the wrong request. Reproduced. | Direction overwritten before the message. | Captured first. | `lightbox_unknown_and_error` |
| LB-7 | `lightbox_calibrator_start()` | A failed CalibratorOn rolled the target back over a waiting request. Reproduced. | Unconditional rollback. | Only when nothing waits. | `lightbox_failure_during_pending_request` |
| D1, D4 | `switch_abort_handler`, `weather_refresh_handler` | `ABORT=Off` cancelled the running changes, `REFRESH=Off` sent Refresh. Reproduced. | Item value not checked. | Checked. | `switch_async_abort_all`, `weather_refresh_values` |
| D3 | switch, weather, safety handlers | TSan: `IS_CONNECTED` read against `INDIGO_PROCESS_CONNECT`. Reproduced. | The recipe of `_private.h`. | The session flag of the core. | TSan runs of the classes |
| D6 | safety | **`SAFE` stayed published while the server did not answer**, up to two timeouts (20 s with the defaults). Reproduced. | Everything ran on the device queue, which the hanging request blocks. | A staleness timer on a timer thread: a value older than 4 idle poll intervals + 1 s goes to ALERT. | `safety_stale_value` |
| D7 | `safety_on_disconnect` | Detaching a connected device published "connection was lost". Reproduced. | "Lost" derived from `CONNECTION` only. | `detaching` checked. | `safety_lifecycle` |
| D8, D9 | safety watchdog and message | IsSafe read twice per tick while failing; the reason not published when already in ALERT. By reading (D9 weakly covered). | — | Keyed on the last attempt; a `reported` flag. | `safety_stale_value` (D9) |
| SIM-1, SIM-2 | simulator camera | The default camera had both CanFastReadout and readout modes; an index-mode Gain could point outside its list. | — | Mutually exclusive; indices limited. | `ccd_properties`, `ccd_capability_variants` |
| S1, S2, S3 | simulator switch | `cancelasync` OK without CanAsync; `devicestate` carried members with an armed error; 32 switches at most. | — | NotImplemented; left out; 160. | `switch_limit` (S3) |

Found in the core hardening (2026-10-02/03, section 7.4):

| ID | Location | Impact | Cause | Fix | Regression case |
|---|---|---|---|---|---|
| CORE-4 | core `CONNECTION` and session, all modules | TSan reports (`ccd_large_frame`, `dome_disconnect_after_request`, `lightbox_disconnect_after_request`). Reproduced. | `CONNECTION` and the session read on the device queue while a bus thread writes them. | `system_alpaca_is_active()`, the device lock around `CONNECTION` and the session, `detaching` written under the framework lock. | TSan run of the suite |
| CORE-5 | all modules (dome, light box, camera, mount and guider had no protection) | TSan report (`dome_apply_status` against `dome_change_property`); a pending change could be overwritten by a poll or a finalizer. Reproduced. | Accept and the writes of the poll were not mutually exclusive. | `system_alpaca_accept()` and the device lock. | TSan run; the pending-request cases of every class |
| CORE-6 | `proxy_poll_handler` | Polled values froze without a message while `devicestate` kept failing with an error answer. Reproduced. | `on_poll` skipped on any failure of the tick request. | Member-by-member fallback, `devicestate` disabled after 5 ticks with a message. | `interface_device_state_failures` |
| CORE-7 | `system_alpaca_finish()` | Wrong or lost failure reason (FOC-1 and the others). | It printed the last error of the channel. | `system_alpaca_reason()`, `_finish_with()`, `_report()`. | the request-failure cases of the classes |
| CORE-8 | rotator `sync` / `movemechanical`, switch `setswitchname` | HTTP 400 to a bad value removed the property or fixed the name for the connection. Reproduced. | `alpaca_is_unsupported()` used for methods with parameters. | `system_alpaca_not_implemented()`. | `rotator_rejected_parameter`, `switch_rejected_name` |
| CORE-9 | camera bin and mode handlers | `CCD_BIN` followed at once by `CCD_FRAME` lost the frame; a binning mode followed by `CCD_BIN` lost the binning. Reproduced. | The first handler overwrote the items of the waiting request. | BUSY properties are skipped under the lock. | `ccd_requests_in_a_row` |
| CORE-10 | focuser, wheel, rotator, dome, light box `on_poll` | A value that could not be read stayed OK. Reproduced. | Read failures ignored. | ALERT until it is read again. | `*_unreadable_position`, `dome_polling`, `lightbox_polling` |
| CORE-11 | guider | The end of a pulse could replace the BUSY state of a request just accepted; one mutex was shared by all guiders. | Check and finish were not atomic. | `guider_complete()` under the device lock. | TSan run |
| CORE-12 | core bridge | TSan report: `bridge_x_alpaca_discover_handler` against the enumeration. Reproduced. | Bridge property states and settings used on the driver queue without `bridge_mutex`. | `bridge_finish()`, `bridge_setting()`, enumeration under `bridge_mutex`. | `core_bus_requests_during_list_changes` under TSan |
| CORE-13 | mount, switch | The bus thread read the capability cache; switch property pointers were read while disconnected; a UTC request dropped while busy overwrote the pending target. By reading. | Data of the device queue used by the bus thread. | Under the lock; inactive guard; target set only when idle. | TSan run; by reading |
| TEST-2 | `mount_cases.h` (side of pier, park, home) | One failure under ASan. | Coordinates read right after the request property, which is published before them. | `SA_WAIT` around those reads. | 5 ASan runs of the mount cases |

Found against OmniSim (2026-10-03, section 7.5):

| ID | Location | Impact | Cause | Fix | Regression case |
|---|---|---|---|---|---|
| OMNI-1 | focuser `on_poll`, abort handler | After a confirmed Halt `FOCUSER_POSITION` went ALERT, then BUSY as a phantom motion, then OK, so the ALERT of the abort was lost. Reproduced against OmniSim, which reports IsMoving for about 1 s after Halt at the stopped position. | IsMoving true without a move of the driver was taken as an external motion. | While IsMoving is true at the halt position, for at most 3 s, the device counts as settling. The simulator models the settle. | `focuser_halt_settles`, `omnisim_focuser` |
| OMNI-2 | rotator `rotator_start` | While synced, `ROTATOR_POSITION` showed OmniSim's mechanical target as the target of a move (105 instead of 160). Reproduced. | The target was read back from TargetPosition, which OmniSim reports without the sync offset. | The driver keeps the target it computed; TargetPosition is used only for moves started by others and at connect. The simulator option `TargetIsMechanical` models OmniSim. | `rotator_target_is_mechanical`, `omnisim_rotator` |
| OMNI-3 | `system_alpaca_simulator_safetymonitor.c` | Our simulator answered IsSafe of a disconnected device with false and no error; OmniSim answers 0x407. | Modelled on older ASCOM simulators. | NotConnected by default, the old behaviour selectable. No driver change. | `safety_transport_loss` (both variants) |

Found by the independent review (2026-10-03, section 7.6). "Confirmed" means the regression case fails against the driver before the fix.

| ID | Location | Impact | Cause | Fix | Regression case |
|---|---|---|---|---|---|
| REV-1 | `proxy_detach`, guider `on_disconnect` | **Deadlock of the driver queue**: detaching a camera or telescope while a guide request was pending stopped discovery, attach and detach for good and refused SHUTDOWN. Confirmed, the case hung 7 of 7 runs against the old driver. | `proxy_detach` held the framework lock of the primary device while the guider cancelled its finalizers, and the queue worker waited for that lock. | Guider handlers and finalizers return while the device is detaching; the guider does not cancel then. No timeout. | `guider_detach_with_pulse_requested` |
| REV-2 | `ccd_read_progress` | One failed `devicestate` read failed a running exposure; with a permanent HTTP 500 every exposure failed. Confirmed. | The error of `devicestate` was returned as the result of the exposure. | Member reads in the same call; `devicestate` given up after 5 failures in a row. | `ccd_device_state_failures` |
| REV-3 | dome coordinates refusal | A slew queued behind a relative move was sent with the target of the relative move (180° sent as 20°). Confirmed. | Coordinates accepted while `DOME_STEPS` was BUSY. | Refused while steps, park or home run. | `dome_slew_during_relative_move` |
| REV-4 | guider | Durations above 10000 ms and non-finite values reached the device (60000 sent). Confirmed. | Raw request values used. | Values taken after the copy into the property, limited. | `guider_duration_limits` |
| REV-5 | camera abort cleanup | Exposure states written without the lock. By reading (inside `libindigo`, invisible to TSan). | Framework cleanup used. | Local cleanup under the lock. | TSan run |
| REV-6 | mount parked refusal | A refused manual motion turned a waiting motion request to ALERT. Confirmed. | State written on the bus thread without the lock. | `system_alpaca_reject()` leaves a BUSY property BUSY. | `mount_parked_refusal_keeps_pending_motion` |
| REV-8 | mount abort | A failed MoveAxis(0) skipped AbortSlew, and the motion items stayed selected. Confirmed. | Early return. | Every axis stopped and AbortSlew always sent; its success resets the motion. | `mount_abort_partial_failures` |
| REV-9 | dome shutter, cover and calibrator finalizers | A failure published ALERT over a newer pending request. Confirmed. | — | Only the message while a request waits. | `dome_shutter_failure_during_pending_request`, `lightbox_completion_failure_during_pending_request` |
| REV-10 | switch | A request equal to a stale cached value was never sent and published OK. Confirmed. | Comparison with the cache. | Named switches always sent. | `switch_request_after_device_change` |
| REV-11 | focuser relative move | `position + steps` overflowed `int` (Position=0 sent). Confirmed. | — | 64-bit sum. | `focuser_large_relative_move` |
| REV-12 | wheel | Slot properties were reallocated on connect while a bus thread could read them. Cause confirmed, the use-after-free not reproduced. | Resize on connect. | Room for 32 slots allocated at attach. | `wheel_slot_properties_do_not_move` |
| REV-13 | light box intensity handler | `AUX_LIGHT_SWITCH` stayed BUSY after light off until a poll. Confirmed. | — | Ended by the handler. | `lightbox_intensity_after_light_off` |
| REV-14 | focuser enumeration | A client enumerating during a mode change got the wrong set of properties. Confirmed. | Enumeration followed the property, not the device mode. | Follows the mode under the lock. | `focuser_enumeration_during_mode_change` |
| REV-15 | core `verify_devices_disconnected`, `bridge_config_busy` | Device count and CONFIG state read without a lock. By reading. | — | Count under the device lock; CONFIG documented (framework). | TSan run |
| REV-17 | HTTP client | `Transfer-Encoding: gzip, chunked` decoded as plain chunked. Confirmed. | Only "chunked" looked for. | Any other coding is a protocol error. | `headers_are_case_insensitive` |
| REV-18 | `X_ALPACA_DEVICES` keys | Two UniqueIDs differing only in replaced characters got one key; the second device was never proxied. Confirmed. | — | Hash appended when a character is replaced. | `core_similar_unique_ids` |
| REV-19 | `PROPERTIES.md` | The fallback of the wheel to FocusOffsets for its slot count was not documented. | — | Documented. | — |
| REV-20 to REV-24, REV-28 | cases | A check that could not fail (`safety_cases.h`), message checks matching older messages, cases ordered by sleeps, a skipped HTTP case reported as passed, faulty replies without checks of the published values, harness off-by-one. Confirmed. | — | Cases fixed. | `safety_read_timeout_reason` and the changed cases |
| REV-25 | simulator | More lenient than real servers. | — | Stricter content type, `--error-value`, `missing-transaction-id`. | `core_strict_server_replies` |
| REV-26, REV-27 | core, weather | Alignment with spaces; missing name defines and NULL checks. | — | Fixed. | — |
| FIX-1 | dome refusal of WP12b | TSan report: the first version of the REV-3 refusal read `hidden` flags written without the lock. | — | Reads only states. | TSan runs |

Found on the NYX-101 (2026-10-03, section 7.10):

| ID | Location | Impact | Cause | Fix | Regression case |
|---|---|---|---|---|---|
| NYX-1 | mount `mount_motion_start()`, `mount_change_property()` | A goto sent right behind an unpark was dropped without an answer; the mount agent reported a slew that never happened. Reproduced on the NYX-101 3 of 3. | Every motion made the coordinates BUSY, and a request for a BUSY property is dropped. | Only a slew makes the coordinates BUSY. Coordinates are accepted anytime with `mount_coordinates_refusal()` (a message during a slew; ALERT with the reason during park, home, flip or while parked). A goto behind an unpark waits for its end (`goto_waits`). | `mount_goto_behind_unpark`, `mount_goto_refusals` |
| NYX-2 | `mount_motion_finalizer()`, `mount_apply_state()` | "Park failed" while the NYX parked; `MOUNT_PARK` stayed ALERT after AtPark. Reproduced. | Completion decided by Slewing false; no recovery. | Park and home complete on AtPark / AtHome, unpark on AtPark and Slewing false; failure only after a 10 s stall (no slewing, Alt/Az change below 0.1°); a late arrival ends ALERT in OK with a message. | `mount_park_delayed_start` |
| NYX-3 | `mount_on_poll()` | A site of 0/0 made the telescope compute LST and RA for Greenwich without a hint. | — | Warning once per connection and whenever the site becomes 0/0; the proxy never writes the site on its own. | `mount_site_unset_warning` |
| NYX-4 | mount locking | TSan: `mount_set_park()` wrote the `MOUNT_PARK` items without the lock while the new refusal read them. | — | Items written under the device lock. | TSan run |

Found by the hardware suite (2026-10-03, section 7.11):

| ID | Location | Impact | Cause | Fix | Regression case |
|---|---|---|---|---|---|
| HW-1 | `mount_probe_axis()` | On the NYX-101 manual motion and slew rates were hidden although CanMoveAxis is true. | The NYX answers AxisRates as `[{"Maximum":4.5},{"Minimum":0}]`, one range split in two objects. | The halves are paired in order. | `mount_axis_rates_split` |
| HW-2 | `mount_motion_start()` | After find home and park the coordinates stayed BUSY for good; every goto was refused. Found on OmniSim. | AtHome reported while Slewing was still true marked an external slew, which a following park did not settle. | Any motion other than a slew ends the external-slew BUSY state. | `mount_external_slew_ended_by_motion` |
| HW-3 | `mount_move_axis()` | On the NYX-101 NORTH moved south and WEST moved east. | The NYX's MoveAxis works in sky directions (positive primary east, positive secondary south, both pier sides). | `X_ALPACA_MOUNT_AXES` = `SKY`; no model name in the driver. | `mount_manual_motion_sky_axes`, `mount_axes_and_settle_defaults` |
| HW-4 | goto, park, home, flip and MoveAxis handlers | On the NYX-101 a command right after a slew was accepted, ignored and reported as done. | The NYX ignores motion commands for about 1.5 s after a slew. | `X_ALPACA_SETTLE_TIME`: the command waits, BUSY and abortable, until that time has passed. | `mount_nyx_settle_after_slew` |
| HW-5 | `focuser_move_finalizer()` | On the Askar-WAF focuser (Alpaca server 1.0.7) a move to 984 ended OK at 986; the next poll showed 984. Found by a user's hardware run on Linux x64. | The Askar reports IsMoving false before Position has settled at the target, and the move ended with the Position read together with that IsMoving. | When IsMoving turns false at a Position other than the target, an absolute move stays BUSY until Position is the target, IsMoving is true again, or `FOCUSER_MOVE_SETTLE_TIME` (3 s) has passed; it then ends OK at the Position read last, as before. | `focuser_position_settles_after_ismoving` |

Found through `agent_alpaca` and ConformU (2026-10-04, section 7.12):

| ID | Location | Impact | Cause | Fix | Regression case |
|---|---|---|---|---|---|
| CHAIN-1 | `guider_finalize()` | A client that read the coordinates as soon as a pulse was reported over saw where the pulse started; ConformU through the agent found up to 13″ of motion on the wrong axis and pulses 6″ short (34 issues on OmniSim, none directly on OmniSim). | The coordinates of the telescope were read by the next regular poll tick, up to a second after the guider property became OK. | A pulse that is over is followed by `system_alpaca_poll_now()`, which reads the state of the primary device and serves it to its `on_poll` before the property is completed. It only notes a lost connection; the regular tick handles it, because the caller is a handler of the guider that the loss takes away. | `guider_completion`: DEC of the mount equals the simulated declination when the north pulse is reported OK. |
| CHAIN-2 | `dome_apply_status()`, `dome_motion_finalizer()` | `DOME_HOME` went OFF at the end of a successful search, so a client could not tell that the dome was at home (`agent_alpaca` reported AtHome false after FindHome). | The item was used as a trigger only; `dome_beaver`, the other INDIGO driver with `DOME_HOME`, keeps it ON while the dome is at home. | The item is ON while AtHome is true, outside a search that runs; a search that ends at home leaves it ON. | `dome_park_home`: ON after the search, OFF after the park that leaves home. |

Observations outside the driver, not changed (D10) and reported to the user:

- `indigo_libs/indigo_bus.c`, `indigo_set_switch()`: a one-of-many property is marked changed on every call, also when the selected item is selected again (cause of MOUNT-2, DOME-6, LB-4).
- `indigo_libs/indigo_bus.c`: on macOS the library exports its own `clock_gettime()` that returns `gettimeofday()` for every clock id, so `indigo_monotonic_time()` follows the wall clock there and driver deadlines follow its steps. The timing measurement of section 7.3 reads `clock_gettime_nsec_np()` instead.
- `INDIGO_COPY_*_PROCESS_CHANGE` drops a request for a property that is BUSY without an answer.

Original audit record of 2026-09-24:

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
| D4 | Variant (b): only devices selected in `X_ALPACA_DEVICES` are proxied, persisted by UniqueID. Replaced by D15 on 2026-10-03. |
| D5 | On an INDIGO disconnect, send `Connected=false` / `disconnect` to the Alpaca device. |
| D6 | Simulators only for now. No hardware testing is planned and no hardware validation is claimed. |
| D7 | No CI. The OmniSim and ConformU tiers stay manual / opt-in. |
| D8 | Fix and test defects AGENT-1..3 and LIB-1..2. **This goes first**, before the `system_alpaca` implementation. ConformU (https://ascom-standards.org/COMDeveloper/Conformance.htm) is run against `agent_alpaca` exposing every INDIGO simulator, before and after the fixes. |

Decided by the user on 2026-10-01:

| ID | Decision |
|---|---|
| D9 | OmniSim is **not installed into the INDIGO project**. It is an extra application, installed by hand only on the machines where the driver is developed (section 4.1). It may be unpacked in `system_alpaca/system_alpaca_simulator/omnisim/`, which `.gitignore` keeps out of the remote repository. ConformU is **not relevant** for `system_alpaca`, because it tests the opposite (server) side; the ConformU round trip is dropped from the test strategy and from D7. (Since 2026-10-03 it lives outside the working tree, next to it, see section 4.1.) |
| D10 | **Minimise the impact on the framework and on other drivers; keep the changes in `indigo_drivers/system_alpaca/` wherever possible.** This supersedes the library placement in D2 and D3: the general JSON parser and the HTTP/1.1 client are driver-local modules (`indigo_system_alpaca_json.c/.h`, `indigo_system_alpaca_http.c/.h`), not additions to `indigo_libs/indigo_json.c` and `indigo_uni_io`. The driver uses the existing `indigo_uni_io` API as it is (`indigo_uni_discover()` and the connect timeout already exist from D8). The requirement of D2 that everything is covered by unit tests stands. Outside the driver folder only the unavoidable registration remains: tests and fixtures under `indigo_test/`, the root `Makefile` driver lists, `indigo.xcodeproj`, `indigo_docs/PROPERTIES.md`, and the status documents. The work runs on subagents from step S2 on (section 11). |

Decided by the user on 2026-10-03:

| ID | Decision |
|---|---|
| D11 | A filter wheel move is complete when `Position` is no longer −1; a move that ends in another slot than the requested one leaves `WHEEL_SLOT` in ALERT with the real slot (WHL-1). |
| D12 | A sync on a telescope that does not track is sent as it is; the telescope refuses it and the client gets ALERT with its message. The driver does not switch tracking on for a sync (REV-7 stays as it is). |
| D13 | `MOUNT_MOTION_DEC` is compensated for the pier side: `NORTH` always moves toward the north celestial pole, also on pier side West, where Alpaca MoveAxis on the mechanical axis moves the other way. |
| D14 | Proxy devices are named `"<DeviceName> on <ServerName>"` instead of `"<DeviceName> @ <ServerName>"`, because `" @ "` is the remote-host separator of the bus (REV-16). Replaced by D17 on 2026-10-03. |
| D15 | Replaces D4: every discovered Alpaca device is proxied by default and appears without a manual selection; a device the user switches off in `X_ALPACA_DEVICES` stays off, persisted by UniqueID. |
| D16 | Fix `indigo_uni_discover()` in `indigo_libs` (variant A, rather than a driver-local workaround): with `include_loopback` it also sends to `127.0.0.1`, because macOS has no broadcast on `lo0` and a local Alpaca server listening on 127.0.0.1 (OmniSim started with `--urls=http://127.0.0.1:<port>`) was never found. Regression case `discover_reaches_loopback_responder` in `indigo_test/unit/test_uni_io.c`, failing before the fix. |
| D17 | Replaces the naming of D14: a proxy is named `"ALPACA <DeviceName>"`, where every occurrence of "alpaca" in any letter case is removed from DeviceName (DeviceType if nothing is left) and the ServerName is not part of the name; a name that is not unique gets `" #<number>"`. `@` stays replaced. |

## 10. Baseline

This is a new driver, so there is no original implementation to build or test. No baseline build was run: the folder contains no sources, and `system_alpaca` is in `EXCLUDED_DRIVERS`.

## 11. Implementation plan split between subagents

Status: **in execution since 2026-10-01**, from step S2, on the user's instruction. Changes against the proposal:

- **D10:** WP1 and WP2 own driver-local files instead of `indigo_libs` (see the tables below). WP3 is not needed: its output already exists from D8.
- **Wave 3 builds.** The five class packages cannot build in the shared driver directory at the same time, so each builds a private tree outside the repository: a frozen snapshot of the core, simulator framework and harness taken after wave 2, overlaid with the live copies of the files that package owns. A change a package needs in a file it does not own is delivered as a separate copy with a justification and merged by the orchestrator. The guider module and its cases belong to WP7 for both primaries; WP8 only attaches the guider from the camera.
- **No worktrees and no subagent commits.** The parallel packages of a wave own disjoint files, work in the one working tree, build only into private scratch directories, and never run a git command that writes. The orchestrator wires the shared files (Makefiles, Xcode project, this document) after each wave. Nothing is committed without the user's instruction.
- The `refactoring` branch named below is stale (43 commits behind `master`, nothing ahead); the work stays in the working tree of `master`.

### 11.1 Roles and ground rules

- **Orchestrator** (the main session). Its responsibilities:
  - assigns work packages and reviews their results;
  - merges them into the `refactoring` branch;
  - owns every shared file, so there are no parallel conflicts:
    - root `Makefile`, `indigo_test/Makefile`
    - `indigo.xcodeproj/project.pbxproj`
    - `indigo_docs/PROPERTIES.md`
    - `MIGRATION_STATUS.md`, `TEST_SUMMARY.md`
    - this `REFACTOR.md`
    - the shared test file `indigo_test/integration/test_system_alpaca_simulator.c` (case registration)
  - pushes. Subagents never push.
- **Subagents** follow these rules:
  - Read `AGENTS.md`, `indigo_drivers/AGENTS.override.md`, `indigo_test/AGENTS.md` and `indigo_test/DRIVER_TESTING_RULES.md`. The formatting rules apply to hand-written code as well: calls on a single line, no empty lines inside functions, `{ 0 }`.
  - Work packages in the same wave that run in parallel each get their own **git worktree** (`isolation: worktree`). Each subagent commits in its own worktree and changes only the files its package owns.
  - Files a package needs to add to shared files go into its report as ready-made snippets: Makefile rules, pbxproj entries, `PROPERTIES.md` sections, test-case registrations. The orchestrator inserts them.
  - The report states what was done, the exact build and test commands with their results, open issues, and any deviations from this document. Every claim of coverage must be backed by a test run.
  - Never edit `README.md` (approval required), the generator, the off-limits drivers, or `agent_alpaca` (except in D8).
- **Structure of the driver (D1):** hand-written, laid out the way generated code is:
  - `#pragma mark` sections in the generator's order;
  - `DRIVER_NAME` / `DRIVER_VERSION` / `DRIVER_LABEL`;
  - a per-driver `driver_queue`, `devices[MAX_DEVICES]` and shared private data per server;
  - `system_alpaca_open` / `system_alpaca_close` with the transactional contract;
  - `*_attach` / `*_change_property` / `*_detach` per class, with the `on_change`-style handler bodies of the generator;
  - `_finalizer` for asynchronous operations.
- **Driver files:** one file per class, so that parallel packages do not collide:

  | File | Contents |
  |---|---|
  | `indigo_system_alpaca.c` | core: bridge device, discovery, attach/detach, entry point |
  | `indigo_system_alpaca_json.c` / `.h` | general JSON parser (`alpaca_json_`), D10 |
| `indigo_system_alpaca_http.c` / `.h` | HTTP/1.1 client on top of `indigo_uni_io` (`alpaca_http_`), D10 |
| `indigo_system_alpaca_transport.c` / `.h` | Alpaca envelope and errors on top of the driver-local HTTP client |
  | `indigo_system_alpaca_<class>.c` | device classes |
  | `indigo_system_alpaca_private.h` | shared types |

  `Makefile.drv` picks up `*.c` automatically.

### 11.2 Wave 0: D8 (runs now, before this plan)

- **W0-A.** Build INDIGO and the ConformU harness: `indigo_server` + `agent_alpaca` + all INDIGO simulators.
- **W0-B..E.** ConformU baseline, in parallel by device group, against the unfixed `agent_alpaca`:
  - Camera
  - Telescope
  - Focuser / FilterWheel / Rotator
  - Dome / CoverCalibrator / Switch
- **Orchestrator.** Fixes AGENT-1..3 and LIB-1..2 plus the defects ConformU finds. Adds unit and regression tests, bumps the `agent_alpaca` / askar versions, and reruns ConformU. The results go into section 8 and a table of ConformU runs.

### 11.3 Wave 1: libraries and simulator (4 subagents in parallel, worktrees)

| WP | Subagent | Output (owned files) | Verification |
|---|---|---|---|
| WP1 | JSON parser | D10: `indigo_drivers/system_alpaca/indigo_system_alpaca_json.c/.h`, the `alpaca_json_` API (section 5.3/1), including the numeric-array fast path; `indigo_test/unit/test_system_alpaca_json.c`; fixtures in `indigo_test/fixtures/protocol/alpaca_*.json` (hand-written from the specification; OmniSim captures follow in WP11) | unit tests: valid and malformed JSON, UTF-8/`\uXXXX`, depth/size limits, locale (`LC_NUMERIC=de_DE`), ASan/UBSan |
| WP2 | HTTP client (D10) | `indigo_drivers/system_alpaca/indigo_system_alpaca_http.c/.h`: the `alpaca_http_` API (GET/PUT, Content-Length, chunked, keep-alive + a single retry on a stale socket, binary body, timeouts), URL / form encoding; `indigo_test/integration/test_system_alpaca_http.c` with a loopback HTTP server in the test (opt-in, it opens sockets) | unit tests: chunked, `Connection: close`, truncated body, stall→timeout, reset, 1xx/3xx/4xx/5xx, large binary body |
| WP3 | UDP discovery (**not run: delivered by D8**) | `indigo_uni_io.c/.h`: `indigo_uni_discover()` (broadcast on every interface + loopback, a callback per reply, deduplication left to the caller). If D8/LIB-2 has not already done it, move `focuser_askar` onto this helper. | unit tests: several responders on loopback, duplicates, late replies, malformed replies, timeout; askar regression tests |
| WP4 | Deterministic simulator | `indigo_drivers/system_alpaca/system_alpaca_simulator/`: HTTP server, discovery responder, management API, device framework (types registered through a table so that wave 3 only adds modules), scripted faults, request recording, explicit tick, ready file; `README`-style usage notes in `REFACTOR.md` (delivered to the orchestrator as a snippet) | Smoke test of the simulator, cross-checked with alpyca as the reference client (if available) and with curl |

Dependencies: WP2 builds on the LIB-1 connect timeout from D8. WP4 is independent; it parses JSON in its own simple way and does not depend on WP1.

### 11.4 Wave 2: driver core (1 subagent, sequential)

| WP | Output | Verification |
|---|---|---|
| WP5 | `indigo_system_alpaca.c/.h/_main.c`, `_transport.c/.h`, `_private.h`:<br>• the "Alpaca" bridge device with `X_ALPACA_DISCOVERY*`, `X_ALPACA_SERVERS`, `X_ALPACA_DEVICES` (D4: selection persisted by UniqueID)<br>• discovery and management, proxy-loop filter, deduplication<br>• attach/detach of an empty proxy with the common members: connect/disconnect with P7 `connect`/`connecting` + finalizer, the pre-P7 fallback, and D5 disconnect<br>• capability cache, `devicestate` polling framework<br>• mapping of errors onto property states<br>The package also removes the driver from `EXCLUDED_DRIVERS` (via a snippet for the orchestrator). | Build, including a strict warnings build. Integration tests against WP4: discovery, manual server, selection and its persistence, duplicates, disappearance/removal, failed attach, capacity, SHUTDOWN, transport loss idle/active, malformed replies, wrong transaction ID, a P7 vs V3 device |

### 11.5 Wave 3: device classes (5 subagents in parallel, worktrees)

Each package delivers:
- a driver module;
- a simulator module for its types;
- test cases in its own source file `indigo_test/integration/system_alpaca/<class>_cases.h`, included by the orchestrator into `test_system_alpaca_simulator.c`;
- `PROPERTIES.md` snippets.

The class acceptance checklist from `DRIVER_TESTING_RULES.md` is mandatory.

| WP | Classes | Specific requirements |
|---|---|---|
| WP6 | Focuser, FilterWheel, Rotator | absolute vs. relative focuser, temperature compensation, wheel 0/1-based and −1 while moving, rotator reverse/sync/mechanical position |
| WP7 | Telescope + Guider | JNow/J2000 (`EquatorialSystem`), async slew + finalizer, sync, park/unpark/home, tracking + rates, MoveAxis + AxisRates, SideOfPier, site/UTC, guide rates; **guider timing measurement** (`AGENTS.override.md:36`) |
| WP8 | Camera + Guider | exposure, abort vs. stop, countdown via `indigo_ccd_exposure_setup()`, binning/frame in binned units, gain/offset in index and value modes, readout modes, cooling; **ImageBytes** (all element types, rank 2/3, transpose, orientation, Bayer, error), JSON fallback through the WP1 fast path, download on a separate connection; **guider timing measurement** |
| WP9 | Dome, CoverCalibrator | shutter, azimuth/altitude slew, park/home, slaved, roll-off; cover/calibrator V1 state enums vs. V2 `*Changing` / `*Moving`, brightness |
| WP10 | Switch, ObservingConditions, SafetyMonitor | boolean vs. analog switches, canwrite, async V3 (setasync/statechangecomplete/cancelasync); weather sensors mapped to `AUX_WEATHER` + `X_` items, averageperiod/refresh; `X_ALPACA_SAFETY` |

The orchestrator merges the worktrees one at a time and runs the full integration suite after each merge.

### 11.6 Wave 4: independent validation (2 subagents in parallel) and completion

| WP | Output |
|---|---|
| WP11 | OmniSim opt-in tier: `make -C indigo_test test-system-alpaca-omnisim` (OmniSim v0.5.0 installed on the development machine per section 4.1 in the git-ignored `system_alpaca_simulator/omnisim/`, never downloaded by the target; private `HOME`, lock, reset between cases). No ConformU round trip (D9). The results are recorded as "OmniSim 0.5.0" (D7: manual only, no CI). |
| WP12 | Independent review of the whole diff, following `AGENTS.md` / `AGENTS.override.md` (formatting, lifecycle, `X_` prefixes, `PROPERTIES.md`, queues vs. bus callbacks, leaks under ASan, platform-independence of the driver). Findings go into section 8. |
| Orchestrator | Fixes from WP12, `PROPERTIES.md`, Xcode registration of every file, `README.md` (only with the user's approval), the README `## Testing` record + `python3 tools/make_test_summary.py`, optionally `MIGRATION_STATUS.md`, the final audit per the `AGENTS.override.md` checklist, and the final summary in this document |

### 11.7 Estimated scale

17 subagents in total across 5 waves, with at most 5 running in parallel:
- wave 0: 5
- wave 1: 4
- wave 2: 1
- wave 3: 5
- wave 4: 2

The critical path is W0 → WP2 → WP5 → WP8 (the camera) → WP11/12.

## Final test summary

State on 2026-10-03 after the hardware suite (section 7.11); this summary is updated with every recorded run.

- Simulated tests, macOS arm64: **318 run, 318 passed** in the recorded run of 2026-10-03 22:44 (`test_system_alpaca_simulator` 275, `test_system_alpaca_http` 43).
- Simulated tests, Linux arm64: **309 run, 309 passed** in the recorded run of 2026-10-03 16:16 (265 + 44), before the NYX-101 fixes.
- The unit tests `test_system_alpaca_json` (48) and `test_uni_io` pass on both platforms; the recording script does not count them.
- OmniSim tests, macOS arm64: **20 run, 20 passed** in the recorded run of the OmniSim tier, 2026-10-03 19:04 (OmniSim 0.5.0); through the hardware suite **81 run, 81 passed** on its 10 devices, 2026-10-03 21:31.
- Hardware tests, macOS arm64: **20 run, 20 passed** on the PegasusAstro NYX-101 through Alpaca, 2026-10-03 21:36.
- The OmniSim and ConformU runs from the research phase (subagent, linux-x64 container, outside the repository) were exploratory tool probes. They are not driver tests and are not counted.
