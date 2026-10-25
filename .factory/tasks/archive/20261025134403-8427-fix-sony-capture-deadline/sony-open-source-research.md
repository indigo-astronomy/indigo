# Sony PTP behaviour in public open-source projects

Research note for the Sony capture task. It summarises what three public projects reveal about the Sony PTP extension and compares it with `indigo_drivers/ccd_ptp/indigo_ptp_sony.c` on this branch. Sony's own Camera Remote SDK and Camera Remote Command references were deliberately not used: they are distributed only to companies under a licence that forbids redistribution and derivative works. The `docs/Camera Control PTP 3 Reference.pdf` bundled in the sony-alpha-python repository was not opened for the same reason.

Everything below is a protocol fact or an observed behaviour, not copied code. Claims are attributed to their source; none of them was verified on hardware in this research.

## Sources

| Project | Revision read | Licence | Transport | What it contributes |
|---|---|---|---|---|
| [libgphoto2](https://github.com/gphoto/libgphoto2) `camlibs/ptp2` | `ca693d113ece13fc199a0387301283e527e6ca03` (2026-10-24) | LGPL-2.1 | USB and PTP/IP | Largest code table, the handshake, the capture sequence, value stepping for the old protocol, a property-descriptor parser, live view |
| [Alpha-Fairy](https://github.com/frank26080115/alpha-fairy) `libraries/PtpIpCamera`, `doc/` | `fd1f298` (2023-05-25) | MIT | PTP/IP (Wi-Fi) | Reverse-engineering notes, value enums (drive mode, focus area, focus status), firmware gotchas |
| [sony-alpha-python](https://github.com/jakkuh/sony-alpha-python) `sony_camera/` | `d40b7a0` (2026-02-09) | none stated (all rights reserved) | PTP/IP over an SSH tunnel | Value encodings, live-view dataset layout, newer property codes |

sony-alpha-python has no licence and its comments appear to come from Sony's PTP 3 reference, so take facts from it only, never code.

## Protocol versions

- `SDIO_GetExtDeviceInfo` (0x9202) takes the requested protocol version as its first parameter: 0x00C8 (200, "for models earlier than 2020") or 0x012C (300, "for 2020 models or later"). libgphoto2 always requests 300. The camera answers with the highest version it supports in the first `uint16` of the reply.
  - Version 2: shutter speed, ISO and aperture are changed in notches through `SetControlDeviceB`, and the capture controls are 16-bit.
  - Version 3: values are written directly through `SetControlDeviceA`, and many more properties exist.
- Layout of the 0x9202 reply (libgphoto2): `uint16 version`, then a PTP `uint16` array of properties, then a second `uint16` array of controls. Each array is a `uint32` count followed by its items. libgphoto2 sorts the codes by `code & 0x7000` into operations (0x1000), events (0x4000) and properties (0x5000).
- **INDIGO difference:** INDIGO picks the protocol from the USB product ID (`>= 0x0ccc`, ILCE-7RM4) and requests that version. It never reads the version the camera returns. It also walks the reply as a flat `uint16` list, so the version word and the two `uint32` counts pass through the 0x7000 classifier. They are dropped today only because they do not match a class.
- Using the returned version would cover bodies whose product ID breaks the ordering, and old bodies that refuse version 300.

## Connection handshake

| Step | libgphoto2 (USB) | Alpha-Fairy / sony-alpha-python (PTP/IP) | INDIGO |
|---|---|---|---|
| 1 | `SDIOConnect(1,0,0)` | `GetDeviceInfo`, `GetStorageIDs`, `SDIOConnect(1)` | `SDIOConnect(1)` |
| 2 | `SDIOConnect(2,0,0)` | `SDIOConnect(2)` | `SDIOConnect(2)` |
| 3 | `GetExtDeviceInfo(300)`, retried up to 20 times while it returns no data | `GetExtDeviceInfo(0x12C)` | `GetExtDeviceInfo(200 or 300)` once |
| 4 | `SDIOConnect(3,0,0)` | `SDIOConnect(3)` | sleep 1 s, then `SDIOConnect(3)` |
| 5 | `SetControlDeviceA(PriorityMode 0xD25A, int8 1 = "application")` | `GetExtDeviceInfo(0x12C)` again | `GetAllDevicePropData` |
| 6 | Record the start time for a startup wait before the first capture | — | Record the connection time |

Differences that matter for INDIGO:

- **PriorityMode (0xD25A).** libgphoto2 sets it to 1 ("application") on every connect. INDIGO never writes it; the property is only a commented-out label (`indigo_ptp_sony.c:121`). The value probably decides whether the PC or the camera body has priority over settings. On a reconnect the camera may still be in camera priority. This is a candidate cause for writes or captures being ignored after a reconnect, which is what this task is about.
- **Empty `GetExtDeviceInfo` reply.** libgphoto2 retries until the property list is not empty. INDIGO does not retry, and an empty reply leaves Sony properties undiscovered.
- **PTP/IP only.** Sony requires `0x9280(4, 2, 2, 0, 0, 1, 1)` + `0x9281(4)` before file system access and `0x9280(4, 0, 5, …)` + `0x9281(4)` after operations. This does not apply to INDIGO's USB driver.

## Property descriptor format (`GetAllDevicePropData` 0x9209)

- The reply starts with a `uint32` entry count and a `uint32` that is zero. Each entry has this layout:
  - `uint16 code`, `uint16 datatype`
  - `uint8 GetSet`, `uint8 IsEnabled`
  - default value, current value
  - `uint8 form`, then the form data
- **IsEnabled** (libgphoto2): 0 = greyed out (read-only now), 1 = enabled, 2 = display only.
  - In protocol 3, libgphoto2 treats anything other than 1 as read-only.
  - In protocol 2, `GetSet` with bit 0x80 set marks a control. The control types are 0x81 button, 0x82 notch, 0x83 lock and 0x84 variable.
  - In protocol 2, some properties marked display-only or disabled are still settable: FocusMode and ExposureMeteringMode when display-only, FlashMode always.
  - **INDIGO difference:** INDIGO skips the IsEnabled byte and trusts `GetSet` alone. A greyed-out setting (for example one locked by the mode dial or by a running capture) is therefore offered as writable, and the camera rejects the write. Reading IsEnabled would make such properties read-only up front instead of relying on read-back after the write.
- **Second enumeration (2024+ bodies, also ILCE-7RM4).** After the first enum list, a second list may follow. libgphoto2 detects it by peeking at the next `uint16`: a value below 0x200 is a count, otherwise it is the next property code.
  - libgphoto2 replaces the first list with the second.
  - INDIGO keeps the first list and skips the second. Per INDIGO's own comment, the first list holds the values settable in the current state and the second holds all values.
  - Alpha-Fairy keeps whichever list is longer.
  - The lists change at runtime, for example with mechanical vs electronic shutter, Bulb availability, the user's ISO range, photo vs movie mode and the lens apertures (Alpha-Fairy).
- **Controls missing from 0x9209 in protocol 3.** Some controls are listed in DeviceInfo but never returned by 0x9209: 0xD2CB FocusMagnifier, 0xD2CC FocusMagnifierCancel and 0xD2CD to 0xD2D0 remote cursor keys. libgphoto2 synthesises descriptors for them: `uint16`, writable, range. It does the same for the shutter, AEL/AFL, the movie button, ManualFocusAdjust, AFAreaPosition and ZoomOperation when they are missing.

## Writing values

| Property | Protocol 3 (direct, `SetControlDeviceA` 0x9205) | Protocol 2 (notch, `SetControlDeviceB` 0x9207) |
|---|---|---|
| Shutter speed 0xD20D | `uint32` = numerator << 16 \| denominator. Whole seconds are written as `10·s/10`. libgphoto2 found that writing 0 for Bulb does not work. | Step ±1 (`uint8` 0x01 / 0xFF). On A7R4-era bodies in protocol 2 the list may come from 0xD229 ShutterSpeed2. |
| ISO 0xD21E | `uint32`, low 24 bits ISO, 0x00FFFFFF = Auto. Top byte: 0x01 Multi Frame NR, 0x02 MFNR High, bit 0x10 extended ISO. | Step ±1 |
| FNumber 0x5007 | `uint16` f-number × 100. libgphoto2 sends it with datatype `uint32`, which looks like a libgphoto2 bug. | Step ±1 |
| Exposure compensation 0x5010 | `int16` (libgphoto2). Alpha-Fairy rounds to a multiple of 100 first because the value must be on the enum list. | Step ±1 |
| CompressionSetting 0x5004 | Standard `SetDevicePropValue`, then poll for up to 2 s until it reads back | — |

Notch stepping in libgphoto2 (`_put_sony_value_*`):

- It first jumps by the whole difference in enum positions, then falls back to single ±1 steps.
- After each step it polls 0x9209 every 200 ms for up to 3 s, because the camera applies the change slowly (about 0.7 s on an SLT-A58).
- It stops on a match, on an overshoot, or after 100 steps.
- The enum may list values the current configuration does not allow (ISO is the documented case), so a computed jump can overshoot forever. For the same reason, positions are compared by enum index, not by value: Auto ISO has the largest value but comes first.
- INDIGO's old-API path exposes only − / + switches. A client that wants a specific value has to step repeatedly.

Value encodings (sony-alpha-python, Alpha-Fairy):

- **Shutter speed:**
  - 0x00000000 means Bulb, and 0xFFFFFFFF means no value.
  - A denominator of 10 means decimal seconds, for example 15/10 = 1.5″.
  - libgphoto2 normalises x/10 with x divisible by 10 to whole seconds.
- **F-number:** 0xFFFD means "closed". 0xFFFE, 0xFFFF and 0 mean no value.
- **Battery 0xD218:** `int8` percent, where 0xFF means not available.
- **ObjectInMemory 0xD215:** the image count with bit 0x8000 set when the object is ready at handle 0xFFFFC001.
  - libgphoto2 found that the counter can show 1 before the flag is set, and that downloading at that point crashes the firmware. Wait for the flag.
  - With RAW+JPEG the camera sends one notification and the counter counts 1 (Alpha-Fairy).
- **FocusFound 0xD213:**
  - Alpha-Fairy's values: 1 idle, 2 AF-S focused, 3 failed, 5 hunting, 6 AF-C focused and tracking.
  - libgphoto2 treats 2 *or* 3 as focused and says 3 is what the RX0 reports. Alpha-Fairy says 3 means failed. They contradict each other.
  - INDIGO treats 3 as failure (`indigo_ptp_sony.c:1588`) and does not wait while the value is 5.
- **AF area position 0xD2DC:** `uint32` = x << 16 \| y, with x in 0–639 and y in 0–479, written through `SetControlDeviceB`.
- **Manual focus:**
  - 0xD2D1 takes an `int16` step. Alpha-Fairy found only ±1, ±3 and ±7 work. libgphoto2 sends −7…+7.
  - 0xD2D2 AFMFHold forces manual focus. If the connection drops while it is held, the camera's focus dial stays dead until the camera restarts (Alpha-Fairy).
  - 0xD2D7 / 0xD2D8 FocusStepNear / FocusStepFar exist, but neither project uses them.
  - INDIGO's focuser sends only ±1 through 0xD2D1, so a larger move takes many transactions.
- **Focus distance 0xD24C FocalPosition:** 0–100, where 100 also means infinity (Alpha-Fairy).

## Capture sequence

libgphoto2 `camera_sony_capture`, used for every body that supports `SDIO_ControlDevice`:

1. **Startup wait.** For about 3 s after connect, drain events and poll 0x9209 before the first capture. Models: ZV-E10, ZV-1, DSC-RX100M7, ILCE-7RM4, ILCE-7RM4A, DSC-RX0M2, ILCE-7M3, ILCE-7SM3, ILCE-7M4, ILCE-7M5, ILCE-9M2, ILCE-1, ILCE-7C. The authors found no event or property that signals readiness; 2.5 s was reported too short for the ILCE-7RM4. They note it may be unnecessary in manual focus.
2. **Drain stale RAM.** If ObjectInMemory is 0x8001, download and discard 0xFFFFC001 (`DeleteObject` is not supported), and repeat. A leftover object from a failed earlier capture would otherwise be returned as the new image, and only a USB reconnect clears it.
3. Write `ShutterHalfRelease` 0xD2C1 = 2, then `ShutterRelease` 0xD2C2 = 2.
4. **Hold while focusing.** Unless FocusMode is MF, hold for at most 1 s. End the hold early on `DevicePropChanged(0xD213)`, on ObjectAdded, or when 0xD213 reads 2 or 3.
5. Write `ShutterRelease` = 1, then `ShutterHalfRelease` = 1.
6. **Wait for the image.** Poll for up to 35 s, the maximum 30 s shutter plus 5 s to save. Stop on `ObjectAdded` 0xC201, or when ObjectInMemory ≥ 0x8000; then use handle 0xFFFFC001. If no object appeared, report failure; accessing 0xFFFFC001 then crashes the camera.

Bulb in libgphoto2:
- Start: `ShutterHalfRelease` = 1, then `RequestOneShooting` 0xD2C7 = 2.
- Stop: `ShutterRelease` = 1, then `ShutterHalfRelease` = 1.

Alpha-Fairy, for Bulb and normal shots:
- It presses `Capture` 0xD2C2 alone, waits for the exposure time while the camera is busy, then releases it.
- It never presses half-release first.

Comparison with INDIGO's `ptp_sony_exposure` (`indigo_ptp_sony.c:1559`):

- **Startup wait.** INDIGO waits 3 s after connect only for the ILCE-7RM4 and 7RM4A (`needs_pre_capture_delay`). libgphoto2's list is wider; the other models above may need it too.
- **Stale RAM.** INDIGO does not drain 0xFFFFC001 before pressing the shutter. If an image is left from an aborted or failed capture, ObjectInMemory may already be 0x8001 when the exposure starts. INDIGO would then download that old image as the result. This is worth checking against the reconnect symptom.
- **Ready threshold.** INDIGO treats `> 0x8000` as ready (`indigo_ptp_sony.c:1640`); libgphoto2 uses `>= 0x8000`. They differ only for exactly 0x8000, whose meaning neither source explains.
- **Focus wait.** INDIGO waits up to 5 s for focus (1 s in MF); libgphoto2 holds at most 1 s and then shoots anyway. Alpha-Fairy wishes for a "shoot regardless of focus" mode, so the camera may refuse to fire without focus. That fits INDIGO's existing shot-retry logic for a press that never produced an image.
- **Overall deadline.** libgphoto2 uses a flat 35 s. INDIGO's `shot_deadline` adds 2 × the exposure (to cover long-exposure noise reduction) plus 3 s. That is more precise; libgphoto2 would miss long-exposure NR on exposures over 30 s.

## Events, operations and responses

Sony codes that INDIGO's `indigo_ptp_sony.h` does not define; all from libgphoto2 unless marked otherwise.

- **Operations:**
  - 0x9210 SDIO_OpenSession
  - **0x9211 SDIO_GetPartialLargeObject**, a chunked download that may help with 60–120 MB ARW files
  - 0x9212 SDIO_SetContentsTransferMode
  - 0x9215 SDIO_GetDisplayStringList
  - 0x9223 SDIO_GetLensInformation
  - 0x922F SDIO_OperationsResultsSupported
  - 0x9251 SDIO_GetExtDeviceProp, a single property (sony-alpha-python; absent on cinema cameras)
  - 0x9280 / 0x9281 for PTP/IP
  - the QX lens-camera set 0x96F2–0x96FE
- **Events:**
  - **0xC206 CapturedEvent**
  - 0xC208 CWBCaptureResult
  - 0xC209 CameraSettingReadResult
  - 0xC20A FTPSettingReadResult
  - 0xC20B MediaFormatResult
  - 0xC20C FTPDisplayNameListChanged
  - 0xC20D ContentsTransferEvent
  - 0xC20F DisplayListChangedEvent
  - 0xC218 FocusPositionResult
  - 0xC21B LensInformationChanged
  - **0xC222 OperationResult**
  - **0xC223 AFStatus**
  - 0xC224 MovieRecOperationsResult
  - Alpha-Fairy also sees 0xC206, 0xC207 and 0xC20C without knowing their meaning.
  - CapturedEvent, OperationResult and AFStatus could give an explicit capture or focus outcome instead of polling ObjectInMemory and FocusFound. Their payloads are not documented in these sources; logging them on the ILCE-7RM4 would show whether they fire.
- **Responses:**
  - 0xA101 AuthenticationFailed
  - 0xA102 PasswordLengthOverMax
  - 0xA103 PasswordIncludesInvalidChar
  - 0xA104 FeatureVersionInvalidValue
  - 0xA105 TemporaryStorageFull
  - **0xA106 CameraStatusError**
  - INDIGO logs these as unknown responses.
- **Object formats:**
  - 0xB101 ARW
  - 0xB110 HEIF
  - 0xB301 MPO

## Property codes

INDIGO names differ from libgphoto2 for these codes. Codes that INDIGO keeps as anonymous `Dxxx` are also listed:

| Code | INDIGO | libgphoto2 / others |
|---|---|---|
| 0xD20E | — | BatteryLevelIndicator |
| 0xD217 | D217 | AELockIndication |
| 0xD21D | — | MovieRecordingState (0 stop, 1 recording) |
| 0xD21F | D21F | FELockIndication |
| 0xD221 | D221 | LiveViewStatus (0 disabled, 1 enabled) |
| 0xD223 | — | DateTimeSet (write-only) |
| 0xD226 | — | ISO2 (undocumented direct ISO, 2020+) |
| 0xD227 | — | FormatMediaStatus |
| 0xD229 | — | ShutterSpeed2 (protocol 2 list on A7R4) |
| 0xD22D | ZoomState | FocusMagnifierStatus |
| 0xD22F | ZoomRatio | CurrentFocusMagnifierRatio |
| 0xD230 | — | FocusMagnifierPosition |
| 0xD232 | — | Focus point readback (Alpha-Fairy) |
| 0xD235 | — | ManualFocusAdjustEnableStatus |
| 0xD239–0xD23D | D239, D23A | PixelShiftShooting Mode2 / Number / Interval / Status / Progress |
| 0xD248 | D248 | MediaSLOT1Status |
| 0xD249 / 0xD24A | — | Slot 1 remaining shots / remaining time (s) |
| 0xD24C | — | FocalPosition (0–100) |
| 0xD24E | — | AWBLockIndication |
| 0xD250 | — | IntervalRECStatus |
| 0xD251 | — | DeviceOverheatStatus |
| 0xD254 | FocusMagnifier | FocusMagnifierSetting |
| 0xD256 | D256 | MediaSLOT2Status |
| 0xD257 / 0xD258 | — | Slot 2 remaining shots / remaining time |
| 0xD25A | D25A (commented out) | **PriorityMode**, written as 1 on connect |
| 0xD25B | D25B | ZoomEnableStatus |
| 0xD25C–0xD25E | — | ZoomScale, ZoomBarInformation (0–100 % with Sony full-frame power zoom lenses), ZoomSpeedRange |
| 0xD260 | D260 | ZoomTypeStatus (1 optical, 2 smart, 3 clear image, 4 digital) |
| 0xD261 | — | Movie time (Alpha-Fairy) |
| 0xD264 | D264 | **RemoteControlRestrictionStatus** |
| 0xD267 | — | LiveViewArea |
| 0xD26A | D26A | LiveViewImageQuality |
| 0xD26B–0xD270 | D270 | Custom WB capture area / frame / standby / execution state |
| 0xD271–0xD276 | D271–D276 | Settings and FTP-settings save/read enable and state |
| 0xD284 / 0xD285 | — | Remote touch enable / cancel-enable status |
| 0xD2C3 / 0xD2C4 / 0xD2C5 | — | AEL, AFL, ReleaseLock buttons |
| 0xD2C9 / 0xD2CA | — | FEL button, FormatMedia |
| 0xD2CB / 0xD2CC | — | FocusMagnifier / cancel |
| 0xD2CD–0xD2D0 | — | Remote cursor keys |
| 0xD2D2 | — | AFMFHold |
| 0xD2D3–0xD2D6 | — | Pixel-shift cancel / mode, HFR standby / cancel |
| 0xD2D7 / 0xD2D8 | — | **FocusStepNear / FocusStepFar** |
| 0xD2D9 | — | AWBL button |
| 0xD2DC | — | AFAreaPosition |
| 0xD2DD | — | ZoomOperation (`int8`, −1 tele / +1 wide per Alpha-Fairy) |
| 0xD2DF–0xD2E2 | — | Custom WB capture controls, FormatMediaCard |
| 0xD2E4 / 0xD2E5 | — | Remote touch at (x, y) / cancel |
| 0xD2E9 / 0xD2EA | — | Save / load zoom and focus position |

Pro-video bodies (FX3/FX30, from sony-alpha-python) add:
- 0xD010 shutter mode (speed or angle)
- 0xD00E shutter angle (× 1000)
- 0xD017 shutter speed on FX6 (read-only)
- 0xD1B5 metered manual level (`int16` × 1000)
- 0xD001, 0xD004–0xD007 iris and focus distance (libgphoto2)

Exposure program values for movie and cinema modes (sony-alpha-python):
- 0x8080–0x8083 S&Q P/A/S/M
- 0x8090 Movie
- 0x8091 Cine EI
- 0x8092 Cine EI Quick

## Live view

- The frame is fetched with `GetObject(0xFFFFC002)`. sony-alpha-python calls `GetObjectInfo(0xFFFFC002)` once before streaming.
- Errors (libgphoto2):
  - `InvalidObjectHandle` 0x2009 means live view is not ready yet; on the ZV-1 this can last up to 1 s.
  - `AccessDenied` 0x200F means the frame was requested too fast; retry after about 20 ms.
- The reply is a dataset whose header layout is:
  - `uint32` JPEG offset, `uint32` JPEG size
  - `uint32` focal-frame-info offset, `uint32` focal-frame-info size
  - reserved bytes up to the JPEG
- An image size of 0 means no frame is ready. The focal-frame info carries the focus, face and tracking frames.
- libgphoto2 switched to the header offset because a stray 0xFF 0xD8 in the header made marker scanning pick the wrong start (gphoto2 issue #389).
- **INDIGO difference:**
  - `ptp_sony_liveview` scans for `FF D8 FF DB` and then for the first `FF D9`. This fails if the JPEG starts with an APPn segment (`FF D8 FF E0`/`E1`), and it can end early on an embedded thumbnail.
  - Using the header offset and size, keeping the marker scan as a fallback, would be more robust.
  - INDIGO treats `DeviceBusy` like `AccessDenied` and retries; libgphoto2 does not mention `DeviceBusy`.

## Firmware behaviour

From Alpha-Fairy unless marked otherwise:

- Some bodies can only "save to PC + camera". Their buffer stays full until the host downloads the object; the busy LED stays on and the next shots are blocked. The A7R4 shows this over both USB and Wi-Fi.
- There are two save-destination menus, one for PC control and one for smartphone control. Both must be set to "Camera Only" to avoid the PC download, and some bodies (A6600, RX0M2) lack the option entirely.
- With RAW+JPEG only one notification arrives and ObjectInMemory counts 1, but two objects must be read. INDIGO already handles the pair (`is_dual_compression`, `pending_pair`).
- APS-C/Super35 crop mode on a full-frame body disables many PTP commands, zoom included.
- Exposure-setting commands are ignored while photos are buffered (from Alpha-Fairy's wishlist). This matches INDIGO's need to read back writes after a capture on the ILCE-7RM4.
- Over Wi-Fi, "pairing" cannot be undone without a factory reset; use "Connect without pairing". This does not affect USB.
- libgphoto2 skips listing the special storage on Sony cameras, because mode 3 cameras fail the listing and mode 2 cameras hang.

## Candidate follow-ups for `ccd_ptp`

These are suggestions for the coordinator to triage, not findings. Each needs hardware confirmation and its own task or approval:

1. Write PriorityMode 0xD25A = 1 after `SDIOConnect(3)`. Check on the ILCE-7RM4 whether it changes the reconnect behaviour this task is fixing.
2. Before pressing the shutter, drain any leftover object at 0xFFFFC001 when ObjectInMemory already has bit 0x8000 set.
3. Choose the API version from the version word in the 0x9202 reply instead of the USB product ID, and parse the reply as version + two arrays.
4. Honour the IsEnabled byte and make greyed-out or display-only properties read-only.
5. Extend the post-connect wait from the ILCE-7RM4/4A to the models libgphoto2 lists, or derive it from the camera rather than the model.
6. Locate the live-view JPEG by the dataset header.
7. Log events 0xC206, 0xC222 and 0xC223 and response 0xA106 by name, to learn whether they report capture and AF results.
8. Label the ISO Multi Frame NR variants and the extended-ISO flag. Today `0x01FFFFFF` is shown as "16777215" instead of an Auto-MFNR label.
9. Consider larger manual-focus steps (±3, ±7) or 0xD2D7/0xD2D8 for faster focuser moves.
