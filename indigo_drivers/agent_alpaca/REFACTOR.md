# agent_alpaca: ASCOM Alpaca conformance fixes

Status: **done, 2026-09-24**. Driver version 0x03000005 → **0x03000006**.

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

No hardware testing. Validation uses INDIGO simulators behind the agent, tested by ConformU 4.5.0 on Linux x64. No hardware validation is claimed.

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

Every defect listed here was reproduced by ConformU or by the image comparison. The exceptions are AGENT-7 and AGENT-9, which are marked "(source audit)".

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

All ConformU logs are in [`conformu/`](conformu). `.log` is the ConformU text log and `.json` the machine-readable result (conformance only; `alpacaprotocol` writes no result file).

| Folder | Content |
|---|---|
| `conformu/baseline/devices.tsv` | Device numbering of the original agent: 17 devices, including standalone guiders and AO |
| `conformu/baseline/conformance/` | Full conformance of the original agent 3.0.0.5 |
| `conformu/baseline/protocol/` | `alpacaprotocol` of the original agent |
| `conformu/final/devices.tsv` | Device numbering after the fix: 14 devices |
| `conformu/final/conformance/` | Full conformance of 3.0.0.6. `telescope7.log` is the run with a realistic site; `telescope7_site_lat0.log` is the run at the simulator's default site. |
| `conformu/final/protocol/` | `alpacaprotocol` of 3.0.0.6 |

## 9. Residual risks and gaps

- The `alpaca_devices` list is accessed from HTTP worker threads without a lock, and device records are freed on detach. This is a pre-existing race, unchanged.
- The blocking `indigo_alpaca_wait_for_*` calls are unchanged. They hold an HTTP worker thread for up to 150 s during slews.
- A device that is both MOUNT and GUIDER would store guider state in the same union as the mount state. No INDIGO driver does this today.
- There is no automated regression test in `indigo_test`. The ConformU harness is manual and needs .NET ConformU. A portable C test of the dispatcher and image encoding is a possible follow-up.
- Only Linux x64 was built and tested. macOS and Windows were not built, and the Windows project files are unchanged.

## Final test summary

Simulated tests, final state 3.0.0.6: 30 run, 25 passed.
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
