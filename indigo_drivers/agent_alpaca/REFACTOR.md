# agent_alpaca: ASCOM Alpaca conformance fixes

Status: **done, 2026-09-25**. Driver version 0x03000005 → 0x03000006 (Linux run, 2026-09-24) → **0x0300000A** (macOS run with hardware, section 10).

This work was requested as decision D8 in `indigo_drivers/system_alpaca/REFACTOR.md`: fix the defects found by the source audit and verify `agent_alpaca` with ASCOM ConformU (https://ascom-standards.org/COMDeveloper/Conformance.htm) against every INDIGO simulator. It is a targeted fix of the Alpaca server, not a generator migration. The agent is hand-written and not generated, and none of that changes here.

## 1. Audit of the original state

- **Architecture.** `indigo_agent_alpaca.c` is an INDIGO agent device plus an INDIGO client that mirrors every INDIGO device into an `indigo_alpaca_device` record.
  - The HTTP handlers are registered on `indigo_server_tcp`: `/management/*` and `/api/v1`.
  - Discovery runs on a UDP responder.
  - The Alpaca members of each device type are implemented in `indigo_alpaca_<type>.c`.
  - Device numbering is persisted in `AGENT_ALPACA_DEVICES`.
- **Blocking behaviour.** Alpaca calls block the HTTP worker thread in `indigo_alpaca_wait_for_*` polling loops (0.5 s × N). This is unchanged, apart from the connection wait (section 3, defect AGENT-9).
- **Concurrency.** The `alpaca_devices` list and `server_transaction_id` are accessed from the HTTP worker threads without a lock. This is unchanged and noted as a residual risk.
- **Tests.** There were no automated tests. The only earlier evidence is the Windows Conform 6.5 log from 2020 (`ASCOM_CONFORMANCE.txt`).

### Hardware-test decision

- Linux run (3.0.0.6): no hardware. Validation used INDIGO simulators behind the agent, tested by ConformU 4.5.0 on Linux x64.
- macOS run (3.0.0.10, section 10): the same simulators, the LX200 driver on its NYX serial simulator, and, at the user's request, the physical devices on the bench: ZWO ASI120MC-S and ASI294MC Pro (`indigo_ccd_asi`), ZWO EFW (`indigo_wheel_asi`), Pegasus Ultimate Powerbox v1.7 (`indigo_aux_upb`) and Pegasus NYX-101 (`indigo_mount_lx200`). The user allowed the full ConformU scope on all of them, including mount motion over the whole sky and switching every UPB output, one of which powers the ASI294MC Pro; the UPB therefore ran only after the camera tests, and its outputs were compared with and restored to the state before the test. Physical hot-plug was not part of the run.

## 2. Test environment

| Item | Value |
|---|---|
| Platform | Linux x64 container, GCC, `make -C indigo_libs`, `make -C indigo_drivers/agent_alpaca -f ../../Makefile.drv` |
| Server | `build/bin/indigo_server -- -p 7624 -b- indigo_agent_alpaca indigo_ccd_simulator indigo_mount_simulator indigo_dome_simulator indigo_rotator_simulator indigo_aux_flipflat indigo_aux_ppb indigo_aux_upb3`, with a private `HOME` |
| Serial simulators | `aux_flipflat_simulator`, `aux_ppb_simulator`, `aux_upb3_simulator` from `indigo_test/build/integration` (ready-file convention) |
| ConformU | 4.5.0 linux-x64, default settings, run headless. Every device runs in its own process with its own `HOME`. |
| Commands | `conformu conformance <url> -r <json>` (full device test) and `conformu alpacaprotocol <url> -r <json>` (HTTP/Alpaca protocol test) |

The harness scripts (`start_server.sh`, `run_all.sh`, `compare.py`) are session-local and not part of the repository. Section 7 records their exact behaviour.

## 3. Found defects

Every defect listed here was reproduced by ConformU, by the image comparison or by a dedicated reproducer. The exceptions are AGENT-7 and AGENT-9, which are marked "(source audit)". AGENT-14 to AGENT-16 were found in the macOS simulator run and AGENT-17 to AGENT-20 in the macOS hardware run (section 10).

| ID | Location | Observable impact | Root cause | Fix | Regression evidence |
|---|---|---|---|---|---|
| AGENT-1 | `indigo_alpaca_ccd.c`, ImageBytes RGB24 | Colour image channels were transmitted as B, R, G. | Wrong indices `base+2, base+0, base+1`. | The rewritten `indigo_alpaca_ccd_get_imagearray()` emits R, G, B for both 8-bit and 16-bit data. | `compare.py`: the DSLR Simulator RGB24 1600×1200 image failed on the original agent and matches pixel by pixel after the fix. |
| AGENT-2 | `indigo_alpaca_ccd.c`, all imagearray paths | The image was flipped vertically: Alpaca `y = 0` was the bottom INDIGO row. Alpaca defines the origin in the top-left corner (`AlpacaDeviceAPI_v1.yaml`, ImageArray); INDIGO raw data is top-down (`ROWORDER='TOP-DOWN'`). | The loop `for (row = height - 1; row >= 0; row--)`. | The loop now iterates `y = 0..height-1`. | `compare.py`: CCD Imager MONO16, where channel order plays no role, failed on the original and matches after the fix. |
| AGENT-3 | `indigo_agent_alpaca.c`, `/management/v1/description` | The keys `ServerVersion` and `ManufacturerURL` were returned instead of the specified `ManufacturerVersion` and `Location`. | Wrong key names. | The spec keys are now returned. | `curl /management/v1/description` |
| AGENT-4 | `indigo_alpaca_ccd.c`, JSON imagearray | The JSON `imagearray` was invalid, starting `[[, 770, …`, and the last element had no separator. The JSON path for RGB48 was missing, and an ImageBytes request without an image wrote an error string with no HTTP header. | The separator was decided by `row == 0` in a loop running from `height-1` downwards. | Rewritten; RGB48 JSON added. A missing image or an unsupported format now returns a JSON error envelope with `InvalidOperation`. | `compare.py` (JSON path); ConformU `ImageArray` "Received error when ImageReady is false" |
| AGENT-5 | `indigo_agent_alpaca.c`, request parsing | ConformU `alpacaprotocol` reported between 17 and 100 issues per device:<br>• a negative or invalid `ClientTransactionID` came back as 4294899406 instead of 0;<br>• badly cased PUT parameters were accepted with 200;<br>• invalid values (`Connected=abc`, empty strings, numbers for bools) were accepted with 200;<br>• a capitalised device type, a non-numeric device number and unknown members were not rejected with 400;<br>• a badly cased `ClientTransactionID` on a PUT was passed into the command arguments, giving `InvalidValue` for `Move`. | Hand-rolled `strncmp`/`atoi` parsing, and no member table. | New dispatcher:<br>• a table of every Alpaca member per device type (verb and parameters, from `AlpacaDeviceAPI_v1.yaml`);<br>• strict URL checks (lower case, numeric device number, known member);<br>• URL decoding;<br>• case-insensitive GET parameters and case-sensitive PUT parameters;<br>• validation of bool, int and double values (400 on failure);<br>• `ClientID` and `ClientTransactionID` parsed as uint32 (invalid values give 0);<br>• only the member's own parameters are passed to the handlers;<br>• the PUT body is always read before validation. | `alpacaprotocol` on all 14 devices: 0 issues. |
| AGENT-6 | `indigo_agent_alpaca.c`, `indigo_alpaca_guider.c` | Standalone guider and AO devices were exposed as `Telescope`. They have no coordinates, tracking or time, so ConformU reported 13, 13 and 42 issues. The mount's own `Telescope` reported `CanPulseGuide=false`. | Every `GUIDER` or `AO` interface was mapped to `Telescope`. | **User decision:** a guider named `<mount> (guider)` is paired with its mount. The mount's `Telescope` implements `CanPulseGuide`, `IsPulseGuiding` and `PulseGuide` through the paired guider and connects the guider together with the mount. Standalone guiders and AO units are no longer exposed. | ConformU Telescope: "Asynchronous single/dual axis pulse guide East/North found OK", "PulseGuide threw an exception when Parked, as required". |
| AGENT-7 | `indigo_alpaca_guider.c` (source audit) | North and South pulses were sent to `GUIDER_GUIDE_RA`, East and West to `GUIDER_GUIDE_DEC`, and `guideraterightascension` was never matched because of the name `guideraterightascensionrate`. | Wrong mapping and a typo. | N/S now go to DEC and E/W to RA; the name is corrected. | ConformU PulseGuide North and East both complete (see AGENT-6). |
| AGENT-8 | `indigo_alpaca_mount.c` (source audit plus ConformU) | Guide rates were reported in % of sidereal instead of deg/s. | No unit conversion. | Converted with `ALPACA_SIDEREAL_RATE = 360 / 86164.0905` deg/s; writes are rounded to whole %, and values outside 1–100 % give `InvalidValue`. | ConformU `GuideRateDeclination Read OK 0.002089 (+00:00:07.5)`, i.e. 50 % sidereal, and write OK. |
| AGENT-9 | `indigo_alpaca_common.c` | Connecting the CCD File Simulator without a file timed out in ConformU after 10 s, because the agent waited 15 s even though INDIGO had already reported ALERT. | The connection wait ignored the ALERT state. | `CONNECTION` ALERT sets `connection_failed`, and the wait returns `UnspecifiedError` (0x4FF) immediately. | ConformU camera 4: "Connection exception" after 3 s instead of the 10 s client timeout. |
| AGENT-10 | `indigo_alpaca_common.c` | `UTCDate` had no `Z` suffix ("does not explicitly state that it is a UTC date-time"). | The INDIGO `UTC_TIME` format was passed through unchanged. | `Z` is appended. | ConformU Telescope: no `UTCDate` issue. |
| AGENT-11 | `indigo_alpaca_focuser.c` | ConformU refused to test the focusers ("can only test focusers that implement IFocuserV2 or later") because `InterfaceVersion` was 1. | Version constant. | `InterfaceVersion` is now 3. Per IFocuserV3, `Move` is accepted while temperature compensation is active. | ConformU now runs the full focuser test (see section 5 for the simulator limitation). |
| AGENT-12 | `indigo_alpaca_lightbox.c` | `CalibratorOn(-1)` did not return `InvalidValue`. | Only the upper bound was checked. | The lower bound is checked too. | ConformU CoverCalibrator: 0 issues. |
| AGENT-13 | `indigo_agent_alpaca.c` | `imagearrayvariant` was served only because `strncmp(command, "imagearray", 10)` also matched it. The ASCOM client library reads the variant image from this endpoint. | Accidental prefix match. | The member is now explicitly routed to the image handler. | ConformU `ImageArrayVariant` "Successfully read variant array". |
| AGENT-14 | `indigo_alpaca_ccd.c` (macOS run, section 10) | A camera without a `CCD_INFO` property reported `CameraXSize`, `CameraYSize`, `MaxADU`, `PixelSizeX` and `PixelSizeY` as 0 while `NumX`/`NumY` were 640/480. `StartExposure` was then rejected with `InvalidValue` and the `alpacaprotocol` run ended without a summary. The CCD File Simulator hides `CCD_INFO`. | The sensor geometry was read only from `CCD_INFO`. | Until `CCD_INFO` is seen, the sensor size is taken from the `CCD_FRAME` width/height limits and `MaxADU` from `CCD_FRAME` bits per pixel. The pixel size uses the placeholder 1 µm that the agent already used for a `CCD_INFO` reporting 0. No INDIGO property was added. | ConformU camera 4 with an image file configured through `FILE_NAME`: 0/10 before, 0/0 after; protocol aborted before, clean after. |
| AGENT-15 | `indigo_alpaca_common.c` (macOS run, section 10) | `Connected=false` returned before the INDIGO driver had disconnected. A `Connected=true` sent right afterwards was ignored by the driver and failed after 16 s with `UnspecifiedError` (0x4FF). ConformU `alpacaprotocol` reported it as an information message on the UPB3 focuser ("re-connecting using re-ordered PUT parameters"). | A BUSY `CONNECTION` already reads as disconnected, so the wait ended at the BUSY update. `indigo_ignore_connection_change()` drops every `CONNECTION` change while the property is BUSY, without any update. | The agent tracks the BUSY state of `CONNECTION`; `Connected` returns only after the property has settled in the requested state. | `reconnect.py` (connect, disconnect, connect, disconnect with no delay, 10 cycles) against UPB3 (focuser): the build without this fix fails in 7 of 10 cycles with 16 s / 0x4FF, 3.0.0.7 passes 10 of 10. ConformU focuser 13 protocol: 1 information message before, clean after (reconnect 55 ms). |
| AGENT-16 | `indigo_alpaca_mount.c`, `indigo_agent_alpaca.c` | `DestinationSideOfPier` returned `NotImplemented`. ConformU reported 5 issues in `DestinationSideOfPier` and the pier side model test. | The member was not implemented. | Implemented in the agent for mounts that report `MOUNT_SIDE_OF_PIER`, using the prediction of the INDIGO mount core (`indigo_mount_driver.c`): target hour angle LST − RA ≥ 0 gives pierEast, otherwise pierWest. Invalid RA/Dec gives `InvalidValue`. No INDIGO property was added. | ConformU telescope 7: "DestinationSideofPier OK Reports the pointing state of the mount as expected", pierWest for HA −6..0 and pierEast for HA 0..+6; issues 21 → 16. |
| AGENT-17 | `indigo_alpaca_common.c` (hardware run, section 10) | On the Pegasus NYX-101 `CanSetGuideRates` was True, `GuideRateDeclination`/`GuideRateRightAscension` read 0 and every write returned `InvalidValue`. ConformU then expected zero movement for every pulse. | `cansetguiderates` was set by the `UTC_TIME` handler, so any mount with a clock claimed settable guide rates. The LX200 driver hides `MOUNT_GUIDE_RATE` for the NYX. | `cansetguiderates` is set only by `MOUNT_GUIDE_RATE`; without it the guide-rate members return `NotImplemented` as ASCOM allows. | NYX-101 and LX200 NYX simulator: `CanSetGuideRates False`, "Optional member returned a NotImplementedException" (3.0.0.7: 2 issues each). |
| AGENT-18 | `indigo_alpaca_common.c` (hardware run, section 10) | `SiteLatitude`, `SiteLongitude` and `SiteElevation` writes did not read back ("Test value +58:09:00,0 did not round trip correctly") on the NYX-101. | The setters returned as soon as the change was sent. A serial driver processes `GEOGRAPHIC_COORDINATES` on its queue, so ConformU read the old value 1 ms later. The mount simulator answers synchronously and hid this. | The setters wait until `GEOGRAPHIC_COORDINATES` leaves BUSY (at most 10 s) and return `ValueNotSet` when the driver answers ALERT. | NYX-101 and LX200 NYX simulator: "Test value … set and read correctly" for all three members (3.0.0.7: 3 issues each). |
| AGENT-19 | `indigo_alpaca_mount.c` (hardware run, section 10) | `SyncToCoordinates` and `SyncToTarget` read back the position from before the sync (3600″ off) on the NYX-101. | `SyncToTarget` did not wait at all; `SyncToCoordinates` waited for `slewing`, which a sync never set. The LX200 driver completes a sync before it reads the new position back. | Both set `slewing` before the change, wait until `MOUNT_EQUATORIAL_COORDINATES` settles, return `UnspecifiedError` on ALERT, and wait up to 3 s for the next position update unless the published position already matches the target (the second condition keeps a sync on a driver that publishes no unchanged position at 0.5 s instead of 3.6 s). | NYX-101 and LX200 NYX simulator: "Synced to sync position OK within tolerance" (3.0.0.7: 4 and 8 issues). Mount simulator: sync 0.5 s. |
| AGENT-20 | `indigo_alpaca_switch.c` (hardware run, section 10) | The six USB ports of the UPB v1 were named "Heater #3", "Heater" and "" (switches 6–11). | `AUX_OUTLET_NAMES` was copied by position into one 8-slot array, and the name of any outlet was looked up by its overall switch index. Drivers publish more names than outlets (the UPB driver also names a third heater). `SetSwitchName` of a GPIO outlet used the item name `GPIO_OUTLET_%d` instead of `GPIO_OUTLET_NAME_%d`. | The names are assigned by item name to the power, heater, USB and GPIO sections, and looked up in the section of the switch. | UPB v1: switches 6–11 are "Port #1" … "Port #6". |

ImageBytes now transmits 8-bit data as Byte (6) and 16-bit data as UInt16 (8), with `ImageElementType` Int32 (2). This is the narrowing the spec allows, and it reduces a 16-bit image to half the size of the previous Int32 transmission.

## 4. Behaviour changes relevant to clients

- **Device list.** Standalone guiders and AO units are no longer listed in `configureddevices`. A mount guider is part of the mount's `Telescope`.
  - Existing `AGENT_ALPACA_DEVICES` entries for those devices remain in the configuration but are not exposed.
  - Numbers assigned to other devices on a fresh configuration are lower, because those devices no longer take numbers. In the test setup Telescope 9 became 7.
- **Strict requests.** Requests that the Alpaca specification defines as invalid are now answered with HTTP 400 instead of being silently accepted.

## 5. ConformU results

### 5.1 Full conformance (`conformu conformance`), each device on a fresh server

| Device (INDIGO simulator) | Baseline (errors / issues) | After fix (errors / issues) | Notes |
|---|---|---|---|
| Camera: CCD Imager, CCD Guider, Bahtinov, DSLR | 0 / 0 each | 0 / 0 each | |
| Camera: CCD File Simulator | 0 / 1 | 0 / 1 | Simulator limitation: no image file configured, so the connection fails. The agent now reports it immediately (AGENT-9). |
| FilterWheel | 0 / 0 | 0 / 0 | |
| Focuser: CCD Imager (focuser) | 1 / 1 (not tested, V1) | 0 / 13 | Simulator limitation, see below. |
| Focuser: UPB3 (focuser) | 1 / 1 (not tested, V1) | 0 / 1 | Simulator limitation, see below. |
| Telescope: Mount Simulator (+ guider) | 0 / 2 (pulse guiding, SideOfPier and slew tests unreachable, `CanPulseGuide=false`) | 0 / 21 | See 5.3 and 7a: the remaining issues come from simulator pulse guiding and from `DestinationSideOfPier` not being implemented. |
| Telescope: CCD Guider (guider), CCD Guider (AO), Mount (guider) | 0 / 13, 0 / 42, 0 / 13 | no longer exposed | AGENT-6 |
| Dome | 0 / 0 | 0 / 0 | |
| Rotator | 0 / 0 | 0 / 0 | |
| CoverCalibrator: FlipFlat | 0 / 1 | 0 / 0 | |
| Switch: Pocket Powerbox | 0 / 0 | 0 / 0 | |
| Switch: UPB3 | 0 / 0 | 0 / 0 | |

**Focuser simulator limitation.** ConformU moves an absolute focuser by `MaxStep / 10` and allows 60 s (`FocuserTimeout`).
- The INDIGO CCD Imager focuser simulator has a range of ±9 999 999 steps (`MaxStep` 19 999 998) and moves at most 100 steps per 0.1 s. The first move of about 2 000 000 steps cannot finish in time, and the remaining move tests cascade from that.
- The UPB3 focuser simulator hits the same timeout on its first move.

This is simulator behaviour, not agent behaviour. The agent reports `IsMoving` correctly and moves are accepted and completed; see the ConformU log `Move to 999999 … STANDARD`.

### 5.2 Protocol (`conformu alpacaprotocol`)

| | Baseline | After fix |
|---|---|---|
| Issues / errors, summed over all devices | 953 / 5 on 17 devices (mostly `ClientTransactionID`, parameter casing and value validation) | 0 / 1 on 14 devices. The error is camera 4 ("Not connected"), the same simulator limitation as in 5.1. |

### 5.3 Telescope details and final reruns

The first post-fix Telescope run used the mount simulator's default site (latitude 0°, longitude 0°). ConformU stopped with "The highest elevation available … is below the horizon", because the newly reachable extended pulse-guide tests use hour angle ±9 h, which is below the horizon at latitude 0 (1 issue, `conformu/final/conformance/telescope7_site_lat0.log`).

The rerun on a fresh server set a realistic site through Alpaca before the test (`SiteLatitude=48.15`, `SiteLongitude=17.1`) and got 0 errors and 23 issues. After the pier-side fix (section 7a) the result is 0 errors and 21 issues (`telescope7.log`):

| Issues | Member | Cause | Classification |
|---|---|---|---|
| 16 | `PulseGuide ±3/±9 h N/S/E/W`: "axis did not move" | The `mount_simulator` guider only times the pulse and does not move the mount coordinates. | Simulator limitation. Pulse timing, `IsPulseGuiding` and the parked check pass. |
| 2 (before the fix only) | `SideofPier`: "pierEast is returned … HA -6..0", "pierWest … HA 0..+6" | `mount_simulator` set `MOUNT_SIDE_OF_PIER.WEST` when the target was in the western sky (`az > 180`). ASCOM and INDI define pierEast as "OTA on the east side of the pier, pointing west". | **Fixed** (section 7a) |
| 5 | `DestinationSideOfPier` / `SOPPierTest` | Not implemented. INDIGO has no property to predict the pier side of a target. | Known gap (optional member). |

The final protocol rerun on a fresh server is in section 5.2 ("After fix").

## 6. Image orientation and colour verification (`compare.py`)

The same exposure is fetched twice:
- directly from INDIGO: an XML client with `enableBLOB URL`, downloading the RAW blob;
- through Alpaca: `imagearray` as JSON and as ImageBytes.

Every pixel and channel is then compared as `Alpaca[x][y][c] == INDIGO_raw[(y * width + x) * planes + c]`.

| Image | Original agent | Fixed agent |
|---|---|---|
| CCD Imager Simulator, MONO16 1600×1200 | JSON: invalid JSON. ImageBytes: FAIL (vertical flip). | JSON OK, ImageBytes OK (UInt16) |
| DSLR Simulator, RGB24 1600×1200 | JSON: invalid JSON. ImageBytes: FAIL (flip + BRG) | JSON OK, ImageBytes OK (Byte) |

The original agent was built from `HEAD` (`git archive`) into a scratch directory and loaded by full path into a separate `indigo_server` instance on port 7630.

## 7. Plan and evidence

| # | Step | State | Evidence |
|---|---|---|---|
| A1 | Build INDIGO, write the harness, run the ConformU baseline (conformance + protocol) on 17 devices | done | Section 5, "Baseline" columns |
| A2 | Rewrite the request dispatcher (AGENT-5), fix `description` (AGENT-3) | done | Clean build with `-Wall`; `alpacaprotocol` 0 issues |
| A3 | Guider pairing and pulse guiding through the mount (AGENT-6/7/8) | done | ConformU Telescope pulse guide OK |
| A4 | Common fixes: `UTCDate`, connection ALERT, focuser V3, calibrator bounds (AGENT-9..12) | done | ConformU |
| A5 | Rewrite `imagearray` (AGENT-1/2/4/13) | done | `compare.py`, ConformU `ImageArray` / `ImageArrayVariant` |
| A6 | Full conformance rerun on a fresh server | done | Section 5.1 |
| A7 | Telescope rerun with a realistic site; protocol rerun on a fresh server | done | Sections 5.2 and 5.3 |

**Discovered during A6: an incorrect build dependency.** `Makefile.drv` does not rebuild objects when `indigo_alpaca_common.h` changes. After the structure layout changed, stale objects crashed `indigo_server` (SIGSEGV in `alpaca_set_connected`, captured with gdb). A clean rebuild (`make -f ../../Makefile.drv clean`) is required after header changes. This is a repository build issue, not an agent defect.

## 7a. MOUNT_SIDE_OF_PIER semantics across INDIGO (analysis, 2026-09-24)

This analysis follows up the open SideOfPier question in section 5.3. It is based on source and protocol documentation only; no hardware was used. ASCOM and INDI both define **EAST as "OTA on the east side of the pier, pointing west"**, which is the normal (non-flipped) state for HA > 0.

| Component | Where EAST/WEST comes from | Semantics |
|---|---|---|
| `mount_ioptron` | `:GEP#` digit 18 ("0 means pier east, 1 means pier west", `Protocol_V3.10.pdf`; a separate digit gives the pointing state) and `:pS#`, mapped directly | ASCOM |
| `mount_lx200` (Gemini, OnStep/OnStepX, ZWO, NYX, TeenAstro, ESP32Go) | device-reported `:Gm#` / `:GU#` pier side, mapped directly (Gemini: "Telescope Mount's Side of Meridian") | ASCOM, as reported by the firmware |
| `mount_asi` (read only) | `:Gm#`, mapped directly | ASCOM, as reported by the firmware |
| `mount_nexstar` | `p` command. SkyWatcher: "E means no flipping (OTA is on the eastern side of meridian)" (`skywatcher.pdf`), mapped directly | ASCOM |
| `mount_synscan` | computed from the encoders: the unflipped state with HA > 0 gives EAST | ASCOM |
| `mount_pmc8` | computed from the encoders exactly as `PMC_Eight_ProgrammersReferenceManual`: "EpW … telescope is EAST of the PIER and pointing WEST … ASCOM PierEAST, HA > 0" | ASCOM |
| `mount_starbook` | `/GET_PIERSIDE`, mapped directly; the protocol notes do not define it | unknown |
| `indigo_version.c` INDI mapping | INDI `PIER_EAST` → `EAST` | ASCOM |
| `agent_mount` | message "Telescope is on east side of pier" for EAST | ASCOM (physical side) |
| `agent_alpaca` | EAST → `pierEast` (0) | ASCOM |
| **`indigo_libs/indigo_mount_driver.c`** | alignment: `side_of_pier = (ha >= 0) ? MOUNT_SIDE_WEST : MOUNT_SIDE_EAST` (`:664`, `:1060`, `:1139`); `indigo_eq_to_encoder()` treats WEST + HA ≥ 0 as the unflipped encoder state (`:910`) | **opposite** ("side of the meridian the OTA points to") |
| **`mount_simulator`** | `west = az > 180` (target in the western sky) → WEST (`indigo_mount_simulator.driver:323-330`) | **opposite**, consistent with the core |
| **`mount_temma`** | Temma reports "Side of mount telescope is on" (`Temma.pdf`), and the driver **inverts** it: `'W'` → EAST (`indigo_mount_temma.driver:210`, `:610`) | **opposite**, consistent with the core |

**Consequences:**
- Every hardware driver that reports a documented side follows ASCOM, except Temma, which inverts it.
- The core alignment code uses the opposite convention. For a mount with a visible `MOUNT_SIDE_OF_PIER`, `indigo_mount_driver.c:666` stores alignment points with the driver's (ASCOM) value. The point lookup in `indigo_translated_to_raw()` (`:1060`) and `indigo_raw_to_translated()` (`:1139`) computes the side with the core convention.
- `mount_synscan` passes its own ASCOM value into `indigo_raw_to_translated_with_lst()`.
- Correction after closer reading: the `side_of_pier` argument of the live `indigo_translated_to_raw_with_lst()` and `indigo_raw_to_translated_with_lst()` is unused, and `indigo_eq_to_encoder()` / `indigo_nearest_alignment_point()` are commented out. So the mixed conventions only affected the E/W label and the persisted side of alignment points, not the computed coordinates.
- `indigo_dome_azimuth.c:113` documents the value explicitly as "the side of the pier the OTA is on" (-1 EAST, +1 WEST), i.e. the ASCOM convention.

**Resolution (approved by the user and applied on 2026-09-24; saved alignment points are deliberately left untouched):**
1. Define the INDIGO semantics as ASCOM/INDI in `indigo_names.h` / `PROPERTIES.md`.
2. Swap the HA→side mapping in the three live places of `indigo_mount_driver.c`. The commented-out `indigo_eq_to_encoder()` is left unchanged.
3. Fix `mount_simulator` (`az > 180` → EAST).
4. Remove the inversion in `mount_temma`.
5. Existing saved alignment points (`side_of_pier` persisted in the alignment file) are not migrated, by user decision.

**Verification:**

| Test | Result |
|---|---|
| `test_mount_simulator` | 16/16 |
| `test_mount_synscan_simulator` | 19/19 |
| `test_mount_temma_simulator` | 13/16, with the pier-side cases updated. The 3 failures ("Failed to open /dev/pts/0" when the serial port is reopened) are identical with the original driver and test on this machine. |
| `test_agent_mount` | intermittent single failures in unrelated cases (`slaving`, `filter site async host`) across repeated runs |
| ConformU Telescope | `SideofPier` now "Reports the pointing state of the mount as expected"; the two SideOfPier issues are gone (23 → 21 issues) |

The test build rule of the `mount_temma_simulator` was also missing `-lm` (link error on `round`).

## 8. Test logs

[`conformu/`](conformu) holds only the text logs written by ConformU itself, from the final macOS arm64 run of 3.0.0.10 (section 10). `<type><number>.log` is `conformu conformance`, `<type><number>_protocol.log` is `conformu alpacaprotocol` of the same device.

| Folder | Content |
|---|---|
| `conformu/simulator/` | INDIGO simulators, device numbers as in section 10.1 |
| `conformu/simulator/lx200_nyx/` | `indigo_mount_lx200` on the NYX serial simulator (`telescope0`) |
| `conformu/hardware/zwo/` | ZWO ASI120MC-S (`camera0`), ASI294MC Pro (`camera1`), ZWO EFW (`filterwheel0`) |
| `conformu/hardware/upb/` | Pegasus Ultimate Powerbox v1.7: `switch0`, `focuser1` |
| `conformu/hardware/nyx101/` | Pegasus NYX-101: `telescope0`, `focuser1`, `switch2` |

The repository ignores `*.log`; `.gitignore` has an exception for `indigo_drivers/agent_alpaca/conformu/**/*.log`.

The JSON result files of the Linux run (`conformu/baseline`, `conformu/final`) were removed at the user's request, and its text logs were never committed; the Linux results survive only as the counts in sections 5 and 7a.

## 9. Residual risks and gaps

- The `alpaca_devices` list is accessed from HTTP worker threads without a lock, and device records are freed on detach. This is a pre-existing race, unchanged.
- The blocking `indigo_alpaca_wait_for_*` calls are unchanged. They hold an HTTP worker thread for up to 150 s during slews.
- A device that is both MOUNT and GUIDER would store guider state in the same union as the mount state. No INDIGO driver does this today.
- There is no automated regression test in `indigo_test`. The ConformU harness is manual and needs .NET ConformU. A portable C test of the dispatcher, image encoding and connection wait is a possible follow-up.
- Linux x64 (3.0.0.6) and macOS arm64 (3.0.0.10) were built and tested. Windows was not built, and the Windows project files are unchanged.
- `MaxADU` is reported as 2^bits (65536 for 16-bit data) instead of 2^bits − 1. ConformU does not flag it; unchanged.
- The remaining ConformU issues are simulator limitations, not agent defects: the focuser simulators move too slowly for ConformU's 60 s move timeout (section 5.1), and the mount simulator guider does not move the mount during a pulse (section 5.3).
- A Switch device is exposed for every INDIGO device with the `AUX_POWERBOX` or `AUX_GPIO` interface. The LX200 driver declares `AUX_POWERBOX` for its aux device, which on a NYX-101 publishes only weather and info, so the Alpaca Switch has `MaxSwitch` 0 (ConformU issue). The agent's device list is fixed before connection, when the outlets are not known yet. Weather would belong to an ASCOM ObservingConditions device, which the agent does not implement.
- A switch whose outlet has no entry in `AUX_OUTLET_NAMES` (the Pocket Powerbox power outlets) has an empty name. Falling back to the INDIGO item label would be a small improvement.
- The NYX-101 publishes no guide rate, so ConformU expects zero movement from every pulse and reports the real movement as an issue (section 10.2).

## 10. macOS arm64 run with hardware (2026-09-24/25, 3.0.0.7 → 3.0.0.10)

### 10.1 Environment and harness

| Item | Value |
|---|---|
| Platform | macOS 26.7 arm64, Apple clang, universal (x86_64 + arm64) build |
| ConformU | 4.5.0 (Build 53834) from the notarized `ConformU-Installer.dmg` (GitHub release v4.5.0), installed in `/Applications`, run headless through `ConformU.app/Contents/Resources/conformu` |
| Simulators | same INDIGO simulators as section 2; the CCD File Simulator gets a 640×480 MONO16 RAW file through `FILE_NAME`; the LX200 driver runs on `mount_lx200_simulator --model nyx` |
| Hardware | ZWO ASI120MC-S, ZWO ASI294MC Pro (`indigo_ccd_asi` 3.0.0.66), ZWO EFW (`indigo_wheel_asi` 3.0.0.16), Pegasus UPB v1.7 on `/dev/cu.usbserial-PA36T4RB` (`indigo_aux_upb` 3.0.0.28), Pegasus NYX-101 on `/dev/cu.usbserial-NYX467edc0c` (`indigo_mount_lx200` 3.0.0.65, mount type NYX) |

The harness scripts are session-local, as in section 2. Every device runs twice on a fresh `indigo_server` with a private `HOME`: `conformu conformance`, then `conformu alpacaprotocol`. Each ConformU process gets its own `HOME`. The telescope gets site 48.15°, 17.1° through Alpaca before the test. For hardware, each server loads only the agent and one hardware driver, so cameras, wheel and mount ran in parallel. The UPB ran alone after the cameras, because one UPB output powers the ASI294MC Pro; its outputs were saved before and compared afterwards. The ConformU runs were executed by parallel subagents on separate ports.

### 10.2 Results of the final run (3.0.0.10)

A run is one `conformance` or one `alpacaprotocol` execution; it passes with 0 errors and 0 issues. Information messages of `alpacaprotocol` (ASCOM errors the device returns as expected, for example `InvalidWhileParked`) do not fail a run.

**Simulators**

| Class | Devices | Runs / passed | Remaining issues |
|---|---|---|---|
| Camera | CCD Imager, Guider, Bahtinov, DSLR, File (0–4) | 10 / 10 | – |
| FilterWheel | CCD Imager (wheel) (5) | 2 / 2 | – |
| Focuser | CCD Imager (focuser) (6), UPB3 (focuser) (13) | 4 / 2 | 13 + 1: first move of MaxStep/10 does not finish in ConformU's 60 s (simulator speed, section 5.1) |
| Telescope | Mount Simulator (7) | 2 / 1 | 16: the simulator guider does not move the mount (section 5.3) |
| Telescope | LX200 on NYX serial simulator | 2 / 1 | 20 pulse guide (no guide rate published, see below) + 2 `SideofPier` (the simulator reports the physical side; the NYX-101 itself passes) |
| Dome | Dome Simulator (8) | 2 / 2 | – |
| Rotator | Field Rotator Simulator (9) | 2 / 2 | – |
| CoverCalibrator | FlipFlat (10) | 2 / 2 | – |
| Switch | Pocket Powerbox (11), UPB3 (12) | 4 / 4 | – |

**Hardware**

| Class | Devices | Runs / passed | Remaining issues |
|---|---|---|---|
| Camera | ASI120MC-S, ASI294MC Pro | 4 / 4 | – |
| FilterWheel | ZWO EFW | 2 / 2 | – |
| Telescope | NYX-101 | 2 / 1 | 17 pulse guide: the NYX publishes no guide rate (`CanSetGuideRates` False), so ConformU expects zero movement, while the mount moves 11–23″ in Dec and 0.6–1.3 s in RA; 1 `SyncToTarget` start slew settled 71″ from its target (not seen in the 3.0.0.8 run) |
| Focuser | UPB focuser, NYX-101 "focuser" | 4 / 1 | UPB: the position did not change on the first move (whether a motor is attached was not established). NYX-101: the LX200 driver defines a focuser device that cannot connect on a NYX (no focuser), so conformance stops at connect and `alpacaprotocol` ends without summary |
| Switch | UPB v1, NYX-101 "aux" | 4 / 2 | UPB: USB port read-back differs from the written value (defect of `indigo_aux_upb`, below); NYX aux: `MaxSwitch` 0 (section 9) |

The protocol runs of all hardware devices except the NYX focuser completed without errors or issues.

### 10.3 Hardware findings reproduced without hardware

Every agent defect found on the NYX-101 (AGENT-17, -18, -19) reproduces on `indigo_mount_lx200` with the NYX serial simulator: 3.0.0.7 gives 39 issues (guide rates 2, site 3, sync 12, pulse guide 20, pier side 2), 3.0.0.8 and later 22 (pulse guide 20, pier side 2). The mount simulator hid them because it processes site and sync synchronously and publishes a guide rate. AGENT-15 reproduces with `reconnect.py` against the UPB3 serial simulator. AGENT-20 (switch names) shows on the simulators too: on 3.0.0.7 the UPB3 simulator's 3 heaters and 8 USB ports had empty names and the Pocket Powerbox's power outlets carried the heater names; on 3.0.0.10 all 19 UPB3 switches and both Pocket Powerbox heaters are named correctly.

### 10.4 Findings in other components

- **`indigo_aux_upb` (UPB v1): the poll overwrites a pending USB port change.** The v1 poll writes the smart hub's port state into `AUX_USB_PORT` without the `upb_adopt()` BUSY guard that the v2 path uses. A change that arrives while the poll runs is replaced by the old state, the handler sends nothing ("Turning port #N" is missing in the driver log for exactly those requests), and the old value is published. Trace: 11 alternating requests on port #2 through Alpaca, 7 hub commands. Not fixed here; it is a separate driver commit.
- **`indigo_client.c`, `indigo_load_driver()`**: a driver path of `INDIGO_NAME_SIZE` or more characters overflowed a buffer and aborted `indigo_server` with SIGTRAP. Fixed and covered by `indigo_test/unit/test_driver_loader` in its own commit (6b80f7e41).
- `indigo_ccd_asi` publishes `CCD_TEMPERATURE` 0 until its first temperature read, so a client reading right after connecting sees 0 °C (ConformU did not flag it).
- `indigo_mount_lx200` reports a NYX-101 as parked after every connect, also when the previous session left it unparked.

### 10.5 Plan and evidence

| # | Step | State | Evidence |
|---|---|---|---|
| M1 | Install ConformU 4.5.0 on macOS, rebuild INDIGO and the drivers, write the harness | done | Dome 3.0.0.6: 0 errors / 0 issues |
| M2 | Simulator run on 3.0.0.6, fix AGENT-14/15/16 → 3.0.0.7 | done | Camera 4, focuser 13 protocol and telescope `DestinationSideOfPier` clean |
| M3 | Hardware run on 3.0.0.7 and NYX simulator reproduction, fix AGENT-17/18/19 → 3.0.0.8 | done | NYX-101 and NYX simulator: guide rate, site and sync clean |
| M4 | UPB switch names, AGENT-20 → 3.0.0.9; sync latency on drivers without position republishing → 3.0.0.10 | done | UPB switches named "Port #1…6"; mount simulator sync 0.5 s instead of 3.6 s |
| M5 | Full final run of 3.0.0.10: all simulators, NYX simulator, all hardware | done | Section 10.2 |
| M6 | Switch off camera cooling after the tests | done | ASI294MC Pro: `CCD_COOLER` OFF, power 0 %, 27.8 °C; the ASI120MC-S has no cooler |

## Final test summary

**macOS arm64, final state 3.0.0.10 (section 10.2)**

Simulated tests: 30 run, 26 passed.
- Camera 10/10, FilterWheel 2/2, Dome 2/2, Rotator 2/2, CoverCalibrator 2/2, Switch 4/4.
- Focuser 4/2: both simulators too slow for ConformU's 60 s move timeout.
- Telescope, mount simulator 2/1: the simulator guider does not move the mount.
- Telescope, LX200 on the NYX serial simulator 2/1: no guide rate published (pulse guide expectation), simulator pier side.

Hardware tests: 16 run, 10 passed.
- Camera (ASI120MC-S, ASI294MC Pro) 4/4, FilterWheel (ZWO EFW) 2/2.
- Telescope (NYX-101) 2/1: pulse guide expectation without a guide rate, one slew settled 71″ off.
- Focuser (UPB v1, NYX-101) 4/1: UPB focuser did not move; the NYX-101 has no focuser.
- Switch (UPB v1, NYX-101) 4/2: `indigo_aux_upb` loses USB port changes during its poll; NYX aux has no switches.

**Linux x64, 3.0.0.6 (sections 5 and 6)**

Simulated tests: 30 run, 25 passed.
- ConformU conformance: 14 device runs, 10 clean.
  - Camera 4: simulator without an image file.
  - Focusers 6 and 13: the simulators move too slowly for ConformU's 60 s timeout.
  - Telescope 7: simulator pulse guiding and pier side, `DestinationSideOfPier` not implemented.
- ConformU protocol: 14 runs, 13 clean (camera 4).
- Image comparison: 2 runs, 2 passed.

The baseline of the original 3.0.0.5 was 36 runs:
- conformance: 17 runs, 9 clean;
- protocol: 17 runs, 0 clean;
- image comparison: 2 runs, 0 passed.

Hardware tests: 0 run, 0 passed.
