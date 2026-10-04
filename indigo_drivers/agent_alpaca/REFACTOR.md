# agent_alpaca: ASCOM Alpaca conformance fixes

Status: **done, 2026-10-01**. Driver version 0x03000005 → 0x03000006 (Linux run, 2026-09-24) → 0x0300000A (macOS run with hardware, section 10) → **0x0300000B** (residual defects, section 11).

This work was requested as decision D8 in `indigo_drivers/system_alpaca/REFACTOR.md`: fix the defects found by the source audit and verify `agent_alpaca` with ASCOM ConformU (https://ascom-standards.org/COMDeveloper/Conformance.htm) against every INDIGO simulator. It is a targeted fix of the Alpaca server, not a generator migration. The agent is hand-written and not generated, and none of that changes here.

## 1. Audit of the original state

- **Architecture.** `indigo_agent_alpaca.c` is an INDIGO agent device plus an INDIGO client that mirrors every INDIGO device into an `indigo_alpaca_device` record.
  - The HTTP handlers are registered on `indigo_server_tcp`: `/management/*` and `/api/v1`.
  - Discovery runs on a UDP responder.
  - The Alpaca members of each device type are implemented in `indigo_alpaca_<type>.c`.
  - Device numbering is persisted in `AGENT_ALPACA_DEVICES`.
- **Blocking behaviour.** Alpaca calls block the HTTP worker thread in `indigo_alpaca_wait_for_*` polling loops (0.5 s × N). This is unchanged, apart from the connection wait (section 3, defect AGENT-9).
- **Concurrency.** The `alpaca_devices` list and `server_transaction_id` are accessed from the HTTP worker threads without a lock. This is unchanged and noted as a residual risk.
- **Tests.** There were no automated tests. The only earlier evidence was the Windows Conform 6.5 log from 2020 (`ASCOM_CONFORMANCE.txt`, removed on 2026-10-01 at the user's request).

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

Every defect listed here was reproduced by ConformU, by the image comparison or by a dedicated reproducer. The exceptions are AGENT-7, AGENT-9 and AGENT-23, which are marked "(source audit)". AGENT-14 to AGENT-16 were found in the macOS simulator run and AGENT-17 to AGENT-20 in the macOS hardware run (section 10). AGENT-21 to AGENT-24 are the residual defects of section 9 and AGENT-25 was found by ConformU in the same work; all are fixed in section 11. AGENT-26 and AGENT-27 were found by ConformU through `system_alpaca` and OmniSim and are fixed in section 12, AGENT-28 to AGENT-32 the same way in section 13.

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
| AGENT-21 | `indigo_agent_alpaca.c`, device records (section 11) | `indigo_server` read freed memory when a device was detached while an Alpaca request used it: a connected hot-plug device that is unplugged, or a device of a remote INDIGO server that goes away. The request polls the record for up to 150 s. The device list was also walked by the HTTP workers while the bus callbacks linked and unlinked records. | `agent_delete_property` freed the record at once; neither the list nor the records had an owner across threads. | The list is guarded by `alpaca_devices_mutex`. Every request and every bus callback registers as a user (`alpaca_devices_enter()` / `alpaca_devices_leave()`); an unlinked record goes to `alpaca_released_devices` and is freed, with its mutex destroyed, when the last user leaves. Lookup, creation, guider pairing and the `configureddevices` listing run under the mutex. | `repro_unplug.py` with an AddressSanitizer build: 3.0.0.10 aborts with heap-use-after-free in `indigo_alpaca_wait_for_bool` 1 s after the unplug (freed by `agent_delete_property`); 3.0.0.11 returns `ValueNotSet` after its 15 s wait, the server stays up, the device leaves the list and its URL answers 400. `stress_remote.py` (a remote INDIGO server with ten devices restarts six times while eight clients poll the agent): no report. |
| AGENT-22 | `indigo_agent_alpaca.c`, `ServerTransactionID` (section 11) | Two responses could carry the same `ServerTransactionID`. | `server_transaction_id++` ran unlocked on every HTTP worker thread. | `next_server_transaction_id()` increments the counter under `alpaca_devices_mutex`. | `stress_remote.py`: 3.0.0.10 returned 1 duplicate in 93 838 and 1 in 93 831 responses (two runs), 3.0.0.11 none in 156 714 and 170 080. |
| AGENT-23 | `indigo_alpaca_ccd.c` (source audit, section 11) | `MaxADU` was 2^bits, one more than the largest value a pixel can hold (65536 for 16-bit data, 256 for 8-bit), and 1 for a camera whose `CCD_INFO` reports 0 bits per pixel (DSLR Simulator). ConformU accepts any value from 1. | `pow(2, bits)` from `CCD_INFO` and `CCD_FRAME`. | `max_adu()` returns 2^bits − 1, limited to `INT_MAX`. A `CCD_INFO` reporting 0 bits leaves the value to `CCD_FRAME`, and a 24 or 48 bit frame counts as RGB with 8 or 16 bits per channel. The first version of the fix returned 0 for the DSLR Simulator; ConformU reported it ("Invalid value below expected minimum (1): 0") and the `CCD_FRAME` fallback was added. | ConformU camera logs: `MaxADU` 65535, 65535, 255, 255 and 65535 for cameras 0 to 4. |
| AGENT-24 | `indigo_alpaca_switch.c` (section 11) | `GetSwitchName` and `GetSwitchDescription` returned an empty string for a switch without an entry in `AUX_OUTLET_NAMES` / `AUX_SENSOR_NAMES`: both power outlets of the Pocket Powerbox, and every outlet of a driver that does not publish the property. | Only the names properties were used. | The label of the INDIGO item is kept per switch (`sw.switchlabel`) and returned when the name is empty. | `repro_unplug.py`: the test powerbox has no `AUX_OUTLET_NAMES`; 3.0.0.10 returns `''` for both switches, 3.0.0.11 `'Mount power'` and `'Camera power'`. ConformU switch `Pocket Powerbox`: switches 0 and 1 are named "Power outlets" and "DSLR outlet". |
| AGENT-25 | `indigo_alpaca_focuser.c` (section 11) | `Move` with `TempComp` true never finished: `IsMoving` stayed true and ConformU's "Move - TempComp True V3" ran into its 60 s timeout. Hidden until the focuser simulator could finish a move at all (section 11.4). | An INDIGO focuser in `FOCUSER_MODE` AUTOMATIC has no `FOCUSER_POSITION` and `FOCUSER_STEPS` properties, so the move request went nowhere while the agent had already set `ismoving`. IFocuserV3 requires `Move` to work with temperature compensation on. | `Move` switches `FOCUSER_MODE` to MANUAL first and marks the compensation as suspended; `TempComp` keeps reading true. The first focuser request that finds the move finished switches the mode back to AUTOMATIC; a `TempComp` write ends the suspension. | ConformU focuser `CCD Imager Simulator (focuser)`: "Move - TempComp True V3 OK Absolute move OK" (ISSUE before). |
| AGENT-26 | `indigo_alpaca_focuser.c` (section 12) | With `TempComp` true, `Position` answered NotImplemented and `Move` sent a relative move to a focuser without `FOCUSER_STEPS`; ConformU reported "Move - TempComp True V3" as NotImplemented. It affects every driver that makes `FOCUSER_POSITION` read-only in AUTOMATIC mode, which is the INDIGO convention (`focuser_asi`, `focuser_astroasis`, `focuser_dsd`, `focuser_lakeside`, `focuser_mypro2`, `focuser_nstep`, `focuser_optec`, `focuser_qhy`, `ccd_touptek`, `system_alpaca`); the focuser of the CCD simulator keeps the property writable and hid it. | `Absolute` was taken from the permission of `FOCUSER_POSITION`, so a read-only position made the focuser relative. AGENT-25 assumed the property is removed in AUTOMATIC mode. | A defined `FOCUSER_POSITION` makes the focuser absolute whatever its permission (a relative focuser does not define it); the permission is kept as `positionwritable`. `Move` switches to MANUAL as before and waits until the position is writable before it sends the target, because a driver may apply the mode asynchronously and a change of a read-only property is ignored; `resume_tempcomp` leaves the mode alone while that move is pending. | ConformU focuser `ALPACA Focuser Simulator - 0` (OmniSim 0.5.0 via `system_alpaca`): "Move - TempComp True V3 OK Absolute move OK" (ISSUE before); `CCD Imager Simulator (focuser)` unchanged, 35/35. |
| AGENT-27 | `indigo_alpaca_rotator.c` (section 12) | `InterfaceVersion` was 1 although README.md claims IRotatorV3, so ConformU did not test `MechanicalPosition`, `MoveMechanical` and `Sync` (43 results instead of 73). Reporting 3 showed that `Sync` was a relative move (ISSUE "Invalid operation" on every sync after the first), `MechanicalPosition` was the sky position and `MoveMechanical` was `MoveAbsolute`. | `alpaca_sync` was commented out and the request was routed to `alpaca_move_relative`; `ROTATOR_RAW_POSITION` was not used. | `InterfaceVersion` 3. `Sync` sets `ROTATOR_ON_POSITION_SET` to SYNC, writes `ROTATOR_POSITION`, sets GOTO back and waits until `Position` reports the new angle. `MechanicalPosition` is `ROTATOR_RAW_POSITION` when the driver defines it, otherwise the position. `MoveMechanical` goes to the mechanical target plus the current offset (`Position` − `MechanicalPosition`). | ConformU rotator `Field Rotator Simulator` and `ALPACA Rotator Simulator - 0` (OmniSim via `system_alpaca`): 73/73, every `Sync` and `MoveMechanical` OK. |
| AGENT-28 | `indigo_alpaca_mount.c` (section 13) | ConformU abandoned the telescope test: Unpark never finished ("Waiting for scope to unpark" timed out after 300 s), then everything failed with "Invalid while parked". | `Park` of a parked telescope was sent to the driver again; `system_alpaca` keeps `MOUNT_PARK` busy until the poll confirms AtPark, and the `Unpark` that ConformU sends 20 ms later was ignored as a request for a busy property. | `Park` of a parked telescope and `Unpark` of an unparked one return at once and send nothing; Alpaca defines both as harmless. | ConformU telescope through `system_alpaca`: "Calling Park twice is harmless", "Unparked successfully". |
| AGENT-29 | `indigo_alpaca_common.c`, `indigo_agent_alpaca.c`, `indigo_alpaca_mount.c` (section 13) | `CanPulseGuide` was False for a telescope whose guider is attached while the mount connects (`system_alpaca`), and `PulseGuide` answered NotConnected instead of NotImplemented. | `Connected` connects the guider only when it is paired already; the guider of `system_alpaca` appears during the connection and stayed disconnected. | A guider that pairs with a connected mount, and the guider of a mount whose connection completes, is connected (from a timer, by name); `Connected` waits for a guider paired during the connection; a record that leaves the list reads as disconnected; `PulseGuide` answers NotImplemented when the guider can not guide. | ConformU telescope through `system_alpaca`: `CanPulseGuide` True and every pulse guide test OK; `Mount Simulator` (guider always attached) unchanged, 234/234. |
| AGENT-30 | `indigo_alpaca_dome.c` (section 13) | "Dome did not sync, Azimuth didn't change value after sync command". | `SyncToAzimuth` returned as soon as the change was sent; ConformU reads `Azimuth` within milliseconds, before a driver that syncs asynchronously has published it. | `SyncToAzimuth` waits until `Azimuth` reports the new value (15 s at most). | ConformU dome through `system_alpaca`: "Dome synced OK to within +- 1 degree". |
| AGENT-31 | `indigo_alpaca_dome.c` (section 13) | "Home command completed but AtHome is false" and "Park command completed but AtPark is false". | `Slewing` ignored `DOME_PARK` and `DOME_HOME`, so a client saw the motion over before it started; `AtHome` was the `DOME_HOME` item, which `system_alpaca` used as a trigger only (CHAIN-2 there). | Park and FindHome count as `Slewing` until their property settles; a search that ends OK leaves `AtHome` true until the dome rotates, whether the driver keeps the item ON (`dome_beaver`, `system_alpaca` now) or not; `Park` of a parked dome sends nothing. | ConformU dome through `system_alpaca`: "Dome homed successfully", "Dome parked successfully"; `Dome Simulator` unchanged, 69/69. |
| AGENT-32 | `indigo_alpaca_mount.c` (section 13) | A telescope whose system is the equinox of date (`mount_lx200`, `system_alpaca` with OmniSim, real mounts) reported J2000 coordinates as `EquatorialSystem` Topocentric; a pure declination move showed as a right ascension move of up to 1.7 s near the pole (8 pulse guide issues through `system_alpaca`). | `MOUNT_EQUATORIAL_COORDINATES` is J2000 on the INDIGO bus and `MOUNT_EPOCH` is the equinox of the telescope; the agent passed the J2000 values on and reported the system from `MOUNT_EPOCH`. | Coordinates go through `indigo_j2k_to_eq()` with the epoch of `MOUNT_EPOCH` on the way to Alpaca and targets through `indigo_eq_to_j2k()` on the way back; J2000 until `MOUNT_EPOCH` is seen. | ConformU telescope through `system_alpaca`: no pulse guide issue; `Mount LX200` on the NYX simulator (epoch 0) unchanged, 234/234. |

ImageBytes now transmits 8-bit data as Byte (6) and 16-bit data as UInt16 (8), with `ImageElementType` Int32 (2). This is the narrowing the spec allows, and it reduces a 16-bit image to half the size of the previous Int32 transmission.

## 4. Behaviour changes relevant to clients

- **Device list.** Standalone guiders and AO units are no longer listed in `configureddevices`. A mount guider is part of the mount's `Telescope`.
  - Existing `AGENT_ALPACA_DEVICES` entries for those devices remain in the configuration but are not exposed.
  - Numbers assigned to other devices on a fresh configuration are lower, because those devices no longer take numbers. In the test setup Telescope 9 became 7.
- **Strict requests.** Requests that the Alpaca specification defines as invalid are now answered with HTTP 400 instead of being silently accepted.

## 5. ConformU findings of the first runs

The result tables of the Linux run of 3.0.0.6 and of the macOS runs of 3.0.0.10 were removed on 2026-10-01 at the user's request, together with their logs; they remain in the git history (commit 10f82794d). The current results are in section 11.5. What those runs established and is still referenced by the defect table:

- **Protocol.** The original 3.0.0.5 produced 953 `alpacaprotocol` issues on 17 devices, mostly `ClientTransactionID` handling, parameter casing and value validation (AGENT-5). Since 3.0.0.6 every device is clean.
- **Guiders.** Standalone guider and AO devices exposed as `Telescope` produced 13 to 42 issues each and are no longer exposed (AGENT-6).
- **Telescope site.** The extended pulse-guide tests use hour angles of ±9 h, which are below the horizon at latitude 0°, so the harness sets a realistic site (48.15°, 17.1°) through Alpaca before the telescope test.
- **Simulator limitations found then, and their state now.** The focuser simulators could not finish ConformU's moves within its 60 s timeout, the mount simulator's guider did not move the mount, and the NYX serial simulator reported a constant pier side. All three were simulator behaviour, not agent behaviour; section 11.4 records what was changed.

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
| A6 | Full conformance rerun on a fresh server | done | Section 5 |
| A7 | Telescope rerun with a realistic site; protocol rerun on a fresh server | done | Section 5 |

**Discovered during A6: an incorrect build dependency.** `Makefile.drv` does not rebuild objects when `indigo_alpaca_common.h` changes. After the structure layout changed, stale objects crashed `indigo_server` (SIGSEGV in `alpaca_set_connected`, captured with gdb). A clean rebuild (`make -f ../../Makefile.drv clean`) is required after header changes. This is a repository build issue, not an agent defect.

## 7a. MOUNT_SIDE_OF_PIER semantics across INDIGO (analysis, 2026-09-24)

This analysis follows up the SideOfPier issues of the first ConformU telescope run (section 5). It is based on source and protocol documentation only; no hardware was used. ASCOM and INDI both define **EAST as "OTA on the east side of the pier, pointing west"**, which is the normal (non-flipped) state for HA > 0.

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

[`conformu/`](conformu) holds one file per device: the detailed log ConformU writes for `conformu conformance`, the one with an `OK`, `INFO` or `ISSUE` result on every line, named `<device type>_<INDIGO device>.log` with every run of other characters in the device name replaced by `_`. Nothing else is kept: no `alpacaprotocol` log, no JSON result file and no console output. The logs are those of the final run of section 11.5.

The repository ignores `*.log`; `.gitignore` has an exception for `indigo_drivers/agent_alpaca/conformu/**/*.log`.

## 9. Residual risks and gaps

- The `alpaca_devices` list and the device records are now owned across threads (AGENT-21, section 11). The scalar fields of a record are still written by the bus callbacks and read by the HTTP workers mostly without the per-device mutex; that is unchanged.
- The blocking `indigo_alpaca_wait_for_*` calls are unchanged. They hold an HTTP worker thread for up to 150 s during slews, and they do not end early when the device is detached: the request returns its timeout error (15 s for `SetSwitch`).
- A device that is both MOUNT and GUIDER would store guider state in the same union as the mount state. No INDIGO driver does this today.
- There is no automated regression test in `indigo_test`. The ConformU harness is manual and needs .NET ConformU. A portable C test of the dispatcher, image encoding and connection wait is a possible follow-up.
- Linux x64 (3.0.0.6) and macOS arm64 (3.0.0.10) were built and tested. Windows was not built, and the Windows project files are unchanged.
- The simulator limitations ConformU showed in the earlier runs (focuser speed and range, mount simulator pulse guiding, NYX simulator pier side) were removed in the simulators, see section 11.4.
- A Switch device is exposed for every INDIGO device with the `AUX_POWERBOX` or `AUX_GPIO` interface. The LX200 driver declares `AUX_POWERBOX` for its aux device, which on a NYX-101 publishes only weather and info, so the Alpaca Switch has `MaxSwitch` 0 (ConformU issue). The agent's device list is fixed before connection, when the outlets are not known yet. Weather would belong to an ASCOM ObservingConditions device, which the agent does not implement. The user decided to keep this as a known limitation (section 11.1).
- A mount that publishes no guide rate (NYX-101, and the LX200 driver on its NYX simulator) is expected by ConformU not to move at all on a pulse, within a default tolerance of 1″ (section 10.2). The harness runs ConformU with `TelescopePulseGuideTolerance` 30″ (section 11.3), so for such a mount the test verifies the direction of the movement only.

## 10. macOS arm64 run with hardware (2026-09-24/25, 3.0.0.7 → 3.0.0.10)

### 10.1 Environment and harness

| Item | Value |
|---|---|
| Platform | macOS 26.7 arm64, Apple clang, universal (x86_64 + arm64) build |
| ConformU | 4.5.0 (Build 53834) from the notarized `ConformU-Installer.dmg` (GitHub release v4.5.0), installed in `/Applications`, run headless through `ConformU.app/Contents/Resources/conformu` |
| Simulators | same INDIGO simulators as section 2; the CCD File Simulator gets a 640×480 MONO16 RAW file through `FILE_NAME`; the LX200 driver runs on `mount_lx200_simulator --model nyx` |
| Hardware | ZWO ASI120MC-S, ZWO ASI294MC Pro (`indigo_ccd_asi` 3.0.0.66), ZWO EFW (`indigo_wheel_asi` 3.0.0.16), Pegasus UPB v1.7 on `/dev/cu.usbserial-PA36T4RB` (`indigo_aux_upb` 3.0.0.28), Pegasus NYX-101 on `/dev/cu.usbserial-NYX467edc0c` (`indigo_mount_lx200` 3.0.0.65, mount type NYX) |

The harness scripts are session-local, as in section 2. Every device runs twice on a fresh `indigo_server` with a private `HOME`: `conformu conformance`, then `conformu alpacaprotocol`. Each ConformU process gets its own `HOME`. The telescope gets site 48.15°, 17.1° through Alpaca before the test. For hardware, each server loads only the agent and one hardware driver, so cameras, wheel and mount ran in parallel. The UPB ran alone after the cameras, because one UPB output powers the ASI294MC Pro; its outputs were saved before and compared afterwards. The ConformU runs were executed by parallel subagents on separate ports.

### 10.2 Hardware observations of the 3.0.0.10 run

The result tables and the logs of this run were removed on 2026-10-01 at the user's request (git history, commit 10f82794d). Its observations that are not agent defects:

- **Cameras (ASI120MC-S, ASI294MC Pro) and filter wheel (ZWO EFW):** no issue.
- **NYX-101 telescope, pulse guiding:** the NYX publishes no guide rate (`CanSetGuideRates` False), so ConformU expects zero movement within 1″, while the mount moves 11–23″ in Dec and 0.6–1.3 s in RA. `SideofPier` passed on the mount. One `SyncToTarget` start slew settled 71″ from its target.
- **UPB v1 focuser:** the position did not change on the first move; whether a motor was attached was not established.
- **NYX-101 "focuser" and "aux":** the LX200 driver defines a focuser device that cannot connect on a NYX, and an aux device that publishes only weather and info, so the Alpaca Switch has `MaxSwitch` 0 (section 9).
- **UPB v1 switch:** the USB port read-back differed from the written value, a defect of `indigo_aux_upb` (section 10.4).

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

## 11. Residual defects from section 9 (2026-10-01, 3.0.0.10 → 3.0.0.11)

The user asked to finish the agent by fixing the residual defects listed in section 9. The scope and the mode were confirmed before the work started.

### 11.1 Scope, audit and decisions

| Item of section 9 | Audit of 3.0.0.10 | Decision |
|---|---|---|
| Device list accessed without a lock, records freed on detach | `alpaca_devices` is a singly linked list. It is walked by the HTTP worker threads (`alpaca_v1_configureddevices_handler`, `alpaca_v1_api_request`) and changed by the bus client callbacks (`agent_define_property` links a record, `agent_delete_property` unlinks and frees it). A request keeps its record for its whole duration, which is up to 150 s in the `indigo_alpaca_wait_for_*` loops, and a Telescope request also uses the record of the paired guider. `server_transaction_id++` runs unlocked on every worker thread. The per-device mutex is never destroyed. | Fix (AGENT-21, AGENT-22) |
| `MaxADU` is 2^bits | `indigo_alpaca_ccd.c` sets `maxadu = pow(2, bits)` from both `CCD_INFO` and `CCD_FRAME`. | Fix (AGENT-23) |
| Empty switch name without an `AUX_OUTLET_NAMES` entry | `alpaca_get_switchname()` returns the `AUX_OUTLET_NAMES` / `AUX_SENSOR_NAMES` value only. | Fix (AGENT-24): fall back to the label of the INDIGO item |
| Switch with `MaxSwitch` 0 | The outlets of a device are known only after it connects (`indigo_mount_lx200` discovers them in `on_connect`), and the Alpaca device list has to be complete before a client connects. | **User decision: left as a known limitation.** Exposing `AUX_WEATHER` / `AUX_INFO` as read-only switches and remembering empty devices in the configuration were offered and declined. |
| Blocking `indigo_alpaca_wait_for_*` calls | unchanged | not in scope |
| Automated regression test in `indigo_test`, ConformU harness in the repository | none | **User decision: not in scope.** The harness stays session-local. |
| ObservingConditions device | not implemented | **User decision: not in scope.** |

### 11.2 Hardware-test decision

No hardware. The user selected the non-interactive simulator mode: INDIGO simulators and the serial simulators behind the agent, tested by ConformU 4.5.0 on macOS arm64. `tools/run_driver_test.py agent_alpaca` reports "no tests named test_agent_alpaca", so the ConformU runs are recorded by hand, as in section 10.

### 11.3 Harness, counting and ConformU settings

The harness is session-local, as before. Every device runs `conformu conformance` and then `conformu alpacaprotocol`, each on a fresh `indigo_server` with a private `HOME`; ConformU gets its own `HOME` too. The simulator server loads `indigo_agent_alpaca`, `indigo_ccd_simulator`, `indigo_mount_simulator`, `indigo_dome_simulator`, `indigo_rotator_simulator`, `indigo_aux_flipflat`, `indigo_aux_ppb` and `indigo_aux_upb3`, the last three on their serial simulators (`aux_upb3_simulator --focuser-rate 500`); the LX200 server loads the agent and `indigo_mount_lx200` on `mount_lx200_simulator --model nyx`. The CCD File Simulator gets a 640×480 MONO16 RAW file through `FILE_NAME`, and a telescope gets the site 48.15°, 17.1° through Alpaca before its test.

Rules the user set on 2026-10-01:

- **Counting.** Every result line of the detailed conformance log is a test: `OK` and `INFO` lines passed, `ISSUE` and `ERROR` lines failed. Counting stops at "Conformance test has finished", because the summary after it repeats the issues. The earlier records counted one conformance and one protocol execution per device as two tests.
- **Logs.** Only the detailed conformance log is kept, one per device, as described in section 8. The `alpacaprotocol` test still runs for every device as a check, but its log is not kept and its lines are not counted; section 11.5 gives its result.
- **Record.** `README.md` has one line per device, written after each device finished.
- **ConformU settings.** Defaults, except `TelescopePulseGuideTolerance` 30″ instead of 1″, passed with `-s`. A mount without a published guide rate is expected by ConformU not to move on a pulse; the LX200 driver on its NYX simulator moves 15–20″ in Dec and 1 s in RA for a 5 s pulse, as the NYX-101 does.

### 11.4 Simulator changes made for this run

ConformU issues that came from a simulator and not from the agent were removed in the simulator, at the user's request. Each driver is its own commit with its own test record.

| Component | ConformU finding | Change | Validation |
|---|---|---|---|
| `indigo_ccd_simulator` focuser (3.0.0.42 → 3.0.0.43) | `MaxStep` 19 999 998 at 10 steps per second: the first move of `MaxStep`/10 could not finish in ConformU's 60 s (`FocuserTimeout`), 13 of 42 tests failed | Travel -20 000 to 20 000 steps, default `FOCUSER_SPEED` 100 (1000 steps per second), relative moves end at the end of the travel | `run_driver_test.py ccd_simulator` 26/26; `agent_imager` 55/55 and `agent_scripting` 61/61 (not recorded); ConformU focuser 35/35 |
| `aux_upb3_simulator` | The driver offers 0 to 9 999 999 steps and the simulator moved 1 step per millisecond: the first move of 999 999 steps could not finish in 60 s, 1 of 28 tests failed | New option `--focuser-rate <steps>` (steps per millisecond, default 1, so the driver's own tests are unchanged); the harness uses 500 | `run_driver_test.py aux_upb3` 33/33; ConformU focuser 28/28 |
| `mount_lx200_simulator` | `SideofPier` pierWest on both sides of the meridian; the `:GU#` status and `:Gm#` carried a constant `W` | The side is derived from the hour angle (`../mount_lx200/REFACTOR.md`); the driver is unchanged | `run_driver_test.py mount_lx200` 103/103; `mount_asi` 10/10 (not recorded); ConformU telescope 234/234 |
| `indigo_mount_simulator` | In the 3.0.0.10 run 16 pulse-guide tests failed because the guider did not move the mount | None here: the simulator moves on guide pulses since its 3.0.0.21 (commit 6e6bd4124) | ConformU telescope 234/234 |

### 11.5 Results of the final run (3.0.0.11, macOS arm64, 2026-10-01)

| Class | INDIGO device | Log | Tests | Passed | Failed |
|---|---|---|---|---|---|
| Camera | CCD Bahtinov Mask Simulator | `camera_CCD_Bahtinov_Mask_Simulator.log` | 107 | 107 | 0 |
| Camera | CCD File Simulator | `camera_CCD_File_Simulator.log` | 107 | 107 | 0 |
| Camera | CCD Guider Simulator | `camera_CCD_Guider_Simulator.log` | 115 | 115 | 0 |
| Camera | CCD Imager Simulator | `camera_CCD_Imager_Simulator.log` | 118 | 118 | 0 |
| Camera | DSLR Simulator | `camera_DSLR_Simulator.log` | 107 | 107 | 0 |
| FilterWheel | CCD Imager Simulator (wheel) | `filterwheel_CCD_Imager_Simulator_wheel.log` | 49 | 49 | 0 |
| Focuser | CCD Imager Simulator (focuser) | `focuser_CCD_Imager_Simulator_focuser.log` | 35 | 35 | 0 |
| Focuser | Ultimate Powerbox 3 (focuser) | `focuser_Ultimate_Powerbox_3_focuser.log` | 28 | 28 | 0 |
| Telescope | Mount LX200 | `telescope_Mount_LX200.log` | 234 | 234 | 0 |
| Telescope | Mount Simulator | `telescope_Mount_Simulator.log` | 234 | 234 | 0 |
| Dome | Dome Simulator | `dome_Dome_Simulator.log` | 69 | 69 | 0 |
| Rotator | Field Rotator Simulator | `rotator_Field_Rotator_Simulator.log` | 43 | 43 | 0 |
| CoverCalibrator | FlipFlat | `covercalibrator_FlipFlat.log` | 40 | 40 | 0 |
| Switch | Pocket Powerbox | `switch_Pocket_Powerbox.log` | 241 | 241 | 0 |
| Switch | Ultimate Powerbox 3 | `switch_Ultimate_Powerbox_3.log` | 772 | 772 | 0 |
| **Total** | 15 devices | | **2299** | **2299** | **0** |

`Mount LX200` is `indigo_mount_lx200` on the NYX serial simulator; every other device belongs to the simulator server of section 11.3.

The `alpacaprotocol` test of every device ended with no error, issue or information alert, and `indigo_server` was still running after every test.

### 11.6 Plan

| # | Step | State | Evidence |
|---|---|---|---|
| R1 | Install ConformU 4.5.0, build the baseline 3.0.0.10 and the serial simulators, recreate the session-local harness | done | ConformU 4.5.0 (Build 53834) from the notarized `ConformU-Installer.dmg` of GitHub release v4.5.0, in `/Applications`; Dome on 3.0.0.10: no issue |
| R2 | Reproduce the device-record race on 3.0.0.10 with an AddressSanitizer build of the agent | done | `repro_unplug.py`: heap-use-after-free in `indigo_alpaca_wait_for_bool`, freed by `agent_delete_property` |
| R3 | Fix AGENT-21/22: lock the device list, free unlinked records only when no request or bus callback uses them, lock the transaction counter | done | `repro_unplug.py` and `stress_remote.py` clean under AddressSanitizer |
| R4 | Fix AGENT-23 (`MaxADU`) and AGENT-24 (switch name fallback) | done | Defect table; Pocket Powerbox switches 0 and 1 are named "Power outlets" and "DSLR outlet" |
| R5 | Version 3.0.0.11, strict build, reproducers rerun | done | `clang -Wall` on all sources: no warning, as on 3.0.0.10; clean rebuild after the header change |
| R6 | Full ConformU simulator run of 3.0.0.11 (the locking change is on the path of every request), logs, test record | done | Section 11.5 |
| R7 | Rules set by the user during the run: old results and logs removed, one detailed log per device, tests counted by result line, simulators fixed where ConformU hit a simulator limitation, wider pulse guide tolerance | done | Sections 8, 11.3 and 11.4 |
| R8 | Fix AGENT-25 (`Move` with temperature compensation), found once the focuser simulator could finish its moves | done | Defect table |

## 12. ConformU through system_alpaca and OmniSim (2026-10-03, 3.0.0.11 → 3.0.0.12)

The agent was tested on devices that `system_alpaca` proxies from ASCOM OmniSim 0.5.0: OmniSim → `system_alpaca` → `agent_alpaca` → ConformU, all on one Mac (arm64). The same ConformU test run directly against OmniSim is the reference, so a difference between the two runs is a loss in the INDIGO path. This first manual run covered the focuser and the rotator.

- `indigo_server` loads `indigo_system_alpaca` and `indigo_agent_alpaca`; `system_alpaca` has discovery disabled and OmniSim as a manual server. Discovery runs once at startup before it can be disabled and proxies any Alpaca server on the LAN; those devices stay disconnected, but they shift the device numbers of the agent. `system_alpaca` ignores the Alpaca server of the agent itself, so the chain has no loop.
- OmniSim, `indigo_server` and every ConformU process run with a private `HOME` (OmniSim also `CFFIXED_USER_HOME` and `ASCOM_LOGPATH`).
- ConformU settings: `FocuserTimeout` and `RotatorTimeout` 300 s instead of 60 s. The OmniSim focuser moves about 480 steps/s, so a move from 0 to MaxStep (50000) cannot finish in 60 s; with the default the reference run itself has 9 issues.
- The agent exports neither ObservingConditions nor SafetyMonitor, so 2 of the 10 OmniSim devices are not reachable through the chain.

Reference (OmniSim directly): focuser and rotator without an issue. Chain on 3.0.0.11: focuser 1 issue (AGENT-26), rotator without an issue but at interface version 1 (AGENT-27). Chain and INDIGO simulators on 3.0.0.12: no issue (README.md, Testing). The focuser chain log was recorded before the AGENT-27 change; the focuser code is the same as in 3.0.0.12.

Differences to the reference that remain:

- `InterfaceVersion` 3 instead of 4 for both: `DeviceState`, `Connect`/`Disconnect` and `Connecting` are not tested.
- `StepSize` answers NotImplemented for both (an optional member). INDIGO has no step size property; `system_alpaca` publishes it as `X_ALPACA_STEP_SIZE`.
- README.md claims IDomeV2, the agent reports 1; to be checked by the dome run of this chain.

## 13. All OmniSim devices through system_alpaca (2026-10-04, 3.0.0.12 → 3.0.0.13)

`tools/alpaca_omnisim_conformu.py` automates section 12 for every OmniSim device: OmniSim, `indigo_server` with `system_alpaca` and the agent, and ConformU, each with a private `HOME` on free ports; the bridge starts from a written configuration (discovery disabled, OmniSim the only server), so no other Alpaca server on the network is proxied. Every device runs directly on OmniSim (the reference) and through the chain with the same ConformU settings (defaults, `FocuserTimeout` and `RotatorTimeout` 300 s); the report lists the problems only one of the two runs has. `--only`, `--no-reference` and `--trace` (ConformU `Debug`/`TraceAlpacaCalls`, `indigo_server -vv`) select what runs.

The first full run on 3.0.0.12 found AGENT-28 to AGENT-32 and two findings in `system_alpaca` (CHAIN-1: the coordinates after a pulse came a poll tick late; CHAIN-2: `DOME_HOME` was a trigger only), which are fixed there. Writable switches with a range (`X_ALPACA_SWITCH_VALUES` of `system_alpaca`, OmniSim's Light Box and Flat Panel) were not exported; the switch code is now table driven, and these values are a section of their own between the GPIO outlets and the GPIO sensors, so the switch numbers of devices without them are unchanged.

Results on 3.0.0.13 (README.md, Testing; logs in `conformu/`):

| Device | Reference (OmniSim directly) | Chain | Remaining difference |
|---|---|---|---|
| Camera | 288/288 | 110/110 | binning is always 1x1 (section CCD of README.md), interface version 3 |
| CoverCalibrator | 47/47 | 40/40 | interface version 1 instead of 2 |
| Dome | 97/101 | 86/86 | interface version 1 instead of 3; the 4 reference issues are OmniSim's (Slewing while the shutter moves, IDomeV3) |
| FilterWheel | 58/58 | 53/53 | interface version 2 instead of 3 |
| Focuser | 42/42 | 35/35 | interface version 3 instead of 4, no StepSize |
| Rotator | 80/80 | 73/73 | interface version 3 instead of 4, no StepSize |
| Switch | 407/407 | 331/331 | ISwitchV2: no asynchronous methods |
| Telescope | 413/413 | 234/234 | interface version 3 instead of 4; MoveAxis group not implemented (section 3) |
| ObservingConditions, SafetyMonitor | 96/96, 22/22 | not exported | no agent class |

Regression of the native devices of section 11 that the changes touch, same harness and settings as there (`TelescopePulseGuideTolerance` 30″): `Dome Simulator` 69/69, `Mount Simulator` 234/234, `Pocket Powerbox` 241/241, `Ultimate Powerbox 3` 772/772, `Mount LX200` on the NYX simulator 234/234, all unchanged.

## Final test summary

The results of the earlier runs were removed on 2026-10-01 at the user's request (sections 5, 8 and 10.2); this summary covers the run that is recorded now.

**macOS arm64, 3.0.0.11, ConformU 4.5.0 (section 11.5)**

Simulated tests: 2299 run, 2299 passed. A test is a result line of the detailed ConformU conformance log (section 11.3); 15 devices, none with an issue or an error. The `alpacaprotocol` check of all 15 devices is clean and is not part of the count.

Hardware tests: 0 run, 0 passed.

Reproducers of section 3, not part of the count: `repro_unplug.py` and `stress_remote.py` under AddressSanitizer fail on 3.0.0.10 and pass on 3.0.0.11.

**macOS arm64, 3.0.0.12, ConformU 4.5.0 (section 12)**

Simulated tests: 216 run, 216 passed: `CCD Imager Simulator (focuser)` 35, `Field Rotator Simulator` 73, and through `system_alpaca` the OmniSim 0.5.0 focuser 35 and rotator 73. The other devices were not rerun; the change is limited to the focuser and the rotator.

**macOS arm64, 3.0.0.13, ConformU 4.5.0 (section 13)**

Simulated tests: 2512 run, 2512 passed: 1550 on the native simulators of section 11 that the changes touch, and 962 on the 8 OmniSim 0.5.0 devices the agent exports through `system_alpaca`.
