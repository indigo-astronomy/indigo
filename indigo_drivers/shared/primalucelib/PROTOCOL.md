# PrimaLuce Lab JSON protocol

This is the protocol implemented by `primalucelib`, the shared library for PrimaLuce Lab
drivers. It covers the SESTO SENSO 2 and 3 and ESATTO focusers, the ARCO rotator (motor 2 of a
focuser controller), the GIOTTO flat panel and the ALTO cover. The original SESTO SENSO (v1)
uses an unrelated ASCII protocol (`#QP!`, `#GT1000!`) and is not covered.

Each command below says how it is known:

- **HW**: verified with `indigo_focuser_primaluce` on a SESTO SENSO 2 (application firmware
  03.05.08, web firmware 03.03.00), over USB and over WiFi. Evidence:
  `indigo_drivers/focuser_primaluce/REFACTOR.md`.
- **HW (SESTO SENSO 3)**: verified on a SESTO SENSO 3 with an ARCO (application firmware 1.0.9, web
  03.06.01) over USB, tasks 011 to 013. Evidence: `indigo_drivers/focuser_primaluce/REFACTOR.md`.
- **HW (GIOTTO)**: verified on a GIOTTO (application firmware 1.03.09, web 1.1) over USB. Evidence:
  `indigo_drivers/aux_giotto/REFACTOR.md`.
- **HW (ALTO1)**: verified on an ALTO1 (application firmware 1.03.11, web 1.01) over USB. Evidence:
  `indigo_drivers/aux_alto/REFACTOR.md`.
- **DOC**: described in the PrimaLuce Lab "Communication Protocol" v4.0 (05 Dec 2022, ESATTO,
  SESTOSENSO2 and ARCO models) and not used by `indigo_focuser_primaluce`.
- **SIM**: used by `indigo_focuser_primaluce` and covered by its simulator, but not observed on
  hardware. The older angle requests remain in this class.
- **INDI**: taken from the INDI drivers (`primalucacommandset.cpp`, `alto.cpp`, `giotto.cpp`,
  `sestosenso3.cpp`). Not verified by INDIGO; no INDIGO driver uses it yet. Treat a reply format
  marked INDI as an assumption until a driver has seen it.

## Using the library

`primalucelib` is a static library, linked into every PrimaLuce driver:

| Build | Library | How a driver links it |
| --- | --- | --- |
| Make (Linux, macOS) | `build/lib/libprimalucelib.a`, from `Makefile` in this directory | The driver's `Makefile.inc` sets `SDK_LIB` and `MAKE_SDK` to the archive (see `indigo_drivers/focuser_primaluce/Makefile.inc`). The archive is rebuilt when a library source changes, and the driver is relinked. |
| Visual Studio | `primalucelib.lib`, from `primalucelib.vcxproj` (in `indigo_windows.sln`) | A `ProjectReference` to `primalucelib.vcxproj` in the driver project. |
| Xcode | compiled once into the `indigo` and `indigo_m1` library targets, which hold all drivers | Nothing more to do: the driver source is in the same target. |
| Tests | `build/lib/libprimalucelib.a` | `PRIMALUCELIB` in `indigo_test/Makefile`. |

```c
// .driver include { } block
#include "../shared/primalucelib/primalucelib.h"

// private data
primalucelib_link link;

// open, identify, use, close
if (primalucelib_open(&PRIVATE_DATA->link, DRIVER_NAME, DEVICE_PORT_ITEM->text.value, error_message)) {
	char *model = primalucelib_get_model_name(&PRIVATE_DATA->link);
	...
	primalucelib_close(&PRIVATE_DATA->link);
}
```

- **Symbols:** library functions and reply tables have normal linkage with the unique `primalucelib_` and `PRIMALUCE_` prefixes. The jsmn parser is compiled into the library with internal linkage; the header exposes only its types. Drivers are loaded without `RTLD_GLOBAL`, so each driver library uses its own copy.
- **One request at a time:** requests on one link must be serialized; the device handler queue
  does this.
- **String lifetime:** strings returned by `primalucelib_get_*()` and `primalucelib_reply_*()`
  point into the link's response buffer and are valid until the next request on that link.
- **Return values:**
  - A setter or command returns `true` only when it was acknowledged with `"done"`.
  - A few return `true` as soon as the controller answers without an `ERROR` because the
    controller does not acknowledge them (LEDs, WiFi mode and credentials, run presets,
    focuser calibration steps, ARCO enable). The header documents which is which.
- **Unit tests:** `indigo_test/unit/test_primalucelib.c`. It pins the exact bytes of every
  request the focuser driver sends.

## Transport

| | USB serial | WiFi (HTTP) |
| --- | --- | --- |
| Port | Silicon Labs CP2102N bridge with its default USB identity (`10c4:ea60`, product "CP2102N USB to UART Bridge Controller", seen on a SESTO SENSO 3 and an ESATTO2), 115200 baud 8N1 | `http://host[:port]` or `tcp://host[:port]`, port 80 by default |
| Request | The JSON text, no terminator | `GET /ajax.php?jreq=<URL-encoded JSON> HTTP/1.1`, one connection per request |
| Reply | One line ended by `\n` (a `\r` before it is ignored) | The HTTP body; the controller does not close the connection, so `Content-Length` ends the reply |
| Noise | Lines starting with `[` are diagnostic output and are skipped. `LOGLEVEL "no output"` turns them off. After `REBOOT` an ESP32 boot log follows the acknowledgement. | none |

Verification: **HW** over USB and WiFi.

## Envelope

```
request   {"req":{"<verb>":{ <payload> }}}
reply     {"res":{"<verb>":{ <payload> }}}
verbs     get (value ""), set, cmd, srv
nodes     device-wide keys at the top level, motor keys under "MOT1" (focuser, cover) or "MOT2" (ARCO)
```

- **Accepted:** an accepted `set` or `cmd` is acknowledged with `"done"` under the key that was
  set or commanded.
- **`MOVE_ABS` exception:** `MOVE_ABS` is acknowledged under `STEP` on both motors, whatever unit
  the move was given in (**HW** for `MOT1`, **SIM** for `MOT2`).
- **Refusals:** a refused request is answered in one of three ways (**HW**):
  - an `ERROR` key under the verb, for example `{"res":{"cmd":{"ERROR":"SYS: Invalid command"}}}`;
  - an error string in place of the acknowledgement, for example
    `{"res":{"set":{"PRESET_1":{"M1POS":"Error: invalid command"}}}}`;
  - a quoted string instead of a JSON object (modelled by the simulator).

  The library fails the request in all three cases and keeps the `ERROR` text as
  `primalucelib_last_error()`.
- **No 12 V supply:** a motor without its 12 V supply answers a move or stop with
  `"12V_PowerSupply_Error"` in place of `"done"` (**HW**).
- **Full state:** `{"req":{"get": ""}}` returns the whole controller state, about 3.9 kB on a
  SESTO SENSO 2 (**HW**). Some values are missing from this full reply but present in a
  targeted get: `MOT1.SPEED` reads `0` when asked directly.
- **Reply limits:** a reply is read into an 8192-byte buffer and parsed into at most 1024 JSON tokens; a larger reply
  is refused. A lookup never reads past the tokens the parser filled, also when a reply uses all 1024
  (FND-20261024121920-3bd2-repository-audit-017).

## Controller

| Purpose | Request | Reply / notes | Known |
| --- | --- | --- | --- |
| Full state | `{"req":{"get": ""}}` | see below | HW |
| Model | `{"req":{"get":{"MODNAME":""}}}` | `SESTOSENSO2`, `SESTOSENSO3`, `ESATTO`, ... | HW |
| Firmware | `{"req":{"get":{"SWVERS":{"SWAPP":""}}}}` | `SWVERS.SWAPP` application, `SWVERS.SWWEB` web firmware; SESTO SENSO 2 / ESATTO need at least 3.05 | HW |
| Quiet serial line | `{"req":{"cmd":{"LOGLEVEL":"no output"}}}` | | HW |
| Environment | `{"req":{"get":{"EXT_T":"", "VIN_12V": "", "MOT1":{"NTC_T":""}}}}` | numeric strings; `EXT_T` `-127.00` means no external probe | HW |
| USB voltage | `VIN_USB` in the full state | ESATTO only; a SESTO SENSO 2 refuses it | HW |
| LEDs | `{"req":{"cmd":{"DIMLEDS":"off"}}}` | `off`, `low`, `middle`, `on`; read back as `DIMLEDS` | HW |
| WiFi mode | `{"req":{"set":{"LANCFG":"sta"}}}` then `{"req":{"cmd":{"REBOOT":""}}}` | `ap` or `sta`; applied only after the restart | HW |
| Access point | `{"req":{"cmd":{"AP_SET_STATUS":"on"}}}` | `on` / `off`; read back as `WIFIAP.STATUS` (HW) | SIM |
| AP credentials | `{"req":{"set":{"WIFIAP":{"SSID":"..", "PWD":".."}}}}` | read back as `WIFIAP.{SSID,PWD}` (HW) | SIM |
| Station credentials | `{"req":{"set":{"WIFISTA":{"SSID":"..", "PWD":".."}}}}` | read back as `WIFISTA.{SSID,PWD}` (HW) | SIM |
| Sub-model | `{"req":{"srv":{"GET_MODEL_SUBMODEL":""}}}` | `"Model = SESTOSENSO3, SubModel = SESTOSENSO3SC, ARCO = Not enabled"` | INDI |
| Recovery delay | `{"req":{"set":{"RECOVER_DELAY":30}}}` | SESTO SENSO 3, -1 to 120 s | INDI |

- **WiFi credentials:** SSID and password values are JSON escaped before insertion: quotes and
  backslashes are escaped, control bytes use `\u00XX`, and other bytes (including UTF-8) are preserved.
  Both setters support full INDIGO text items even at maximum escaping expansion; larger library
  inputs that exceed the encoding buffers or complete request capacity fail without sending a partial request.
- **Before restarting:** a driver must not restart the controller while another logical device
  still shares the link. `indigo_focuser_primaluce` refuses the WiFi mode change while the
  rotator is connected.
- **During a restart (HW):** a SESTO SENSO 3 (firmware 1.0.9) stays silent for about 9.5 s after a
  `CAL_DIR` write, then answers every request it received meanwhile at once (12 late `MODNAME`
  replies were observed). `primalucelib_begin_restart()` makes requests fail at once with "The
  controller is restarting". `primalucelib_probe_restart()` asks with a 0.3 s read timeout, and once
  the controller answers it reads the late replies away until the line is quiet.

Full-state keys used by the focuser driver (**HW**):
- Identity and firmware: `MODNAME`, `SN`, `SWVERS.{SWAPP,SWWEB}`.
- Network and LEDs: `LANCFG`, `WIFIAP.{SSID,PWD,STATUS}`, `WIFISTA.{SSID,PWD}`, `DIMLEDS`.
- Environment: `EXT_T`, `VIN_12V`, `VIN_USB`.
- Calibration flags: `CALRESTART.{MOT1,MOT2}`.
- Focuser motor: `MOT1.{ABS_POS_STEP, ABS_POS, BKLASH, SPEED, STATUS.MST, NTC_T, ERROR, CAL_MINPOS, CAL_MAXPOS, FnRUN_*, HOLDCURR_STATUS}`.
- Presets: `PRESET_1..9.{NAME,M1POS}`, `RUNPRESET_{L,M,S,1,2,3}.M1*`.
- ARCO: `MOT2.{ABS_POS, ABS_POS_DEG, POSITION_DEG, STATUS.MST, ERROR, CAL_STATUS}`.

## Focuser motor (MOT1)

| Purpose | Request | Acknowledged under | Known |
| --- | --- | --- | --- |
| Position and status | `{"req":{"get":{"MOT1":{"ABS_POS_STEP":"","STATUS":""}}}}` | `MOT1.ABS_POS_STEP`, `MOT1.STATUS.MST` | HW |
| Position, older firmware | `{"req":{"get":{"MOT1":{"ABS_POS":"STEP","STATUS":""}}}}` | `MOT1.ABS_POS` | SIM |
| Motor state, SESTO SENSO 3 | `{"req":{"get":{"MOT1":{"STATUS":{"MST":""}}}}}` | `MOT1.STATUS.MST` | SIM |
| Move | `{"req":{"cmd":{"MOT1":{"MOVE_ABS":{"STEP":1000}}}}}` | `MOT1.STEP` | HW |
| Move, SESTO SENSO 3 | `{"req":{"cmd":{"MOT1":{"GOTO":1000}}}}` | `MOT1.GOTO` | SIM |
| Stop at once | `{"req":{"cmd":{"MOT1":{"MOT_ABORT":""}}}}` | `MOT1.MOT_ABORT` | HW |
| Decelerated stop | `{"req":{"cmd":{"MOT1":{"MOT_STOP":""}}}}` | `MOT1.MOT_STOP` | HW |
| Fast move | `{"req":{"cmd":{"MOT1":{"F_OUTW":""}}}}` / `F_INW` | | INDI |
| Backlash | `{"req":{"set":{"MOT1":{"BKLASH":120}}}}` | `MOT1.BKLASH` | HW |
| Speed | `{"req":{"set":{"MOT1":{"SPEED":3}}}}` | `MOT1.SPEED` | SIM |
| Hold current | `{"req":{"set":{"MOT1":{"HOLDCURR_STATUS":1}}}}` | `MOT1.HOLDCURR_STATUS` | HW |
| Motor settings | `{"req":{"get":{"MOT1":{"HOLDCURR_STATUS":"","FnRUN_SPD":"",...}}}}` | `FnRUN_{ACC,SPD,DEC,CURR_ACC,CURR_SPD,CURR_DEC,CURR_HOLD}` | HW |
| Set motor rates / currents | `{"req":{"set":{"MOT1":{"FnRUN_ACC":1,"FnRUN_DEC":1,"FnRUN_SPD":2}}}}` | each key | INDI |
| Run preset | `{"req":{"cmd":{"RUNPRESET":"light"}}}` | `light`, `medium`, `slow`, or `1` to `3` for the user presets | HW |
| User run preset | `{"req":{"set":{"RUNPRESET_1":{"M1ACC":..,"M1DEC":..,"M1SPD":..,"M1CACC":..,"M1CDEC":..,"M1CSPD":..,"M1HOLD":..}}}}` | | SIM |
| Stored position | `{"req":{"set":{"PRESET_1":{"M1POS":4500}}}}` | `PRESET_1.M1POS`; `0` is refused | HW |
| Stored position name | `{"req":{"set":{"PRESET_1":{"NAME":"Sun"}}}}` | `PRESET_1.NAME` | HW |
| Relative move | `{"req":{"cmd":{"MOT1":{"MOVE":{"STEP":-1000}}}}}` | `MOVE.STEP` | DOC |
| Calibration step | `{"req":{"cmd": {"MOT1": {"CAL_FOCUSER":"Init"}}}}` | SESTO SENSO 2 and 3 only | HW |
| Calibration direction | `{"req":{"set": {"MOT1": {"CAL_DIR":"normal"}}}}` | `normal` / `invert` | HW |

- **Position fields:**
  - `ABS_POS_STEP` is the documented mechanical position.
  - `ABS_POS` is an undocumented signed driver register on older firmware. The tested SESTO
    SENSO 2 does not report it.
  - Never write `ABS_POS`: the controller implements the write as an offset in
    `COMPENSATION_POS_STEP`, and every later move then stops short of its target by that offset
    (**HW**).
- **Motor state:** `MST` is `stop` at rest and `move` (or another state) while the motor runs.
  A stalled motor keeps reporting `move` with an unchanged position.
- **Travel range:** the range is `CAL_MINPOS` to `CAL_MAXPOS`. An uncalibrated controller
  reports no usable range.
- **ESATTO calibration and home sensor:**
  - An ESATTO has no calibration command: v4.0 lists `CAL_FOCUSER` as SESTOSENSO2 only, and an ESATTO2
    (application firmware 3.05.28) answers every `CAL_FOCUSER` with `"Error: invalid command"` (**HW**).
  - Its travel is calibrated in the factory. Hall sensors at both ends are described by `HSENDET`, the
    zones `CAL_MIN_LO`/`CAL_MIN_HI` and `CAL_MAX_LO`/`CAL_MAX_HI`, `CAL_OFFMIN` and `CAL_HOMEP`
    (0–30000 and 401668–439000 on the tested ESATTO2). Sensor errors such as
    `CalRestart_NoSensorActiveInMinPos` appear in `ERROR`.
  - The tested ESATTO2 stopped an inward move from 500 to 400, reported `NOPCMD`, and reset its
    coordinate to 0 (**HW**). Moves of ±100 around 5000 ran normally. The document does not describe
    this behaviour.
- **Calibration sequence**, SESTO SENSO 2 (**HW**):
  1. `Init`, then `CAL_DIR`, `StoreAsMinPos` and `GoOutToFindMaxPos`, about one second apart.
  2. The draw tube runs outward until `StoreAsMaxPos`.
- **SESTO SENSO 3 calibration** (**INDI**):
  - Semi-automatic: `MoveIn-<n>` / `MoveOut-<n>` jogs and `StopMotor`.
  - Automatic, SC sub-model: `start_auto_cal` / `stop_calib`.
  - `GoInToFindMinPos`.

## ARCO rotator (MOT2)

| Purpose | Request | Acknowledged under | Known |
| --- | --- | --- | --- |
| Enable / disable | `{"req":{"set": {"ARCO":1}}}` | | SIM |
| Angle, synced | `{"req":{"get":{"MOT2":{"POSITION_DEG":"","STATUS":""}}}}` | `MOT2.POSITION_DEG`; a SESTO SENSO 3 reports five decimals with a step rounding (a move to 15 reads 14.99994) | HW (SESTO SENSO 3) |
| Angle, mechanical | `{"req":{"get":{"MOT2":{"ABS_POS_DEG":"","STATUS":""}}}}` | `MOT2.ABS_POS_DEG` | SIM |
| Angle, older firmware | `{"req":{"get":{"MOT2":{"ABS_POS":"DEG","STATUS":""}}}}` | `MOT2.ABS_POS` | SIM |
| Move | `{"req":{"cmd":{"MOT2":{"MOVE_ABS":{"DEG":45}}}}}` | `MOT2.STEP`; a SESTO SENSO 3 and an ESATTO2 (3.05.28) answer `MOT2.DEG`. An ARCO that needs calibration acknowledges and does not turn on both | HW (SESTO SENSO 3, ESATTO2) |
| Sync | `{"req":{"cmd":{"MOT2":{"SYNC_POS":{"DEG":45}}}}}` | `MOT2.SYNC_POS`; a SESTO SENSO 3 answers `MOT2.DEG` | HW (SESTO SENSO 3) |
| Relative move | `{"req":{"cmd":{"MOT2":{"MOVE":{"DEG":4}}}}}` | `MOT2.MOVE` | DOC |
| Stop | `{"req":{"cmd":{"MOT2":{"MOT_STOP":""}}}}` | `MOT2.MOT_STOP` | HW (SESTO SENSO 3) |
| Calibrate | `{"req":{"set":{"MOT2":{"CAL_STATUS":"exec"}}}}` | `MOT2.CAL_STATUS`; poll `CAL_STATUS` until `stop` (about 66 s on an ESATTO2); `stop` also ends it | HW (ESATTO2) |
| Serial number | `{"req":{"get":{"ARCO_SN":""}}}` | | INDI |

- **Synced angle:** `MOVE_ABS` targets the synced angle `POSITION_DEG`.
- **Relative move:** v4.0 documents a relative `MOVE` for the ARCO of an ESATTO or a SESTO SENSO 2.
  The SESTO SENSO 3 document v1.0 does not list it and it has not been tried, so the driver computes
  a relative move from the current angle with `MOVE_ABS` on every model.
- **Calibration is a `set`:** ARCO calibration is started with `set`, not `cmd`; sent as `cmd`
  it completes at once.
- **Debug text (HW, SESTO SENSO 3 firmware 1.0.9):** after quick repeated ARCO syncs the controller
  printed `mJsonAPI_ParseCmdReq_Motor` several times in front of the reply on the same line; the
  library parses a reply from its first `{`.
- **Without an ARCO:** a controller without an ARCO refuses ARCO commands with
  "Error: invalid command; ARCO is not connected" (**HW**, sync request).
- **Native reverse:** INDI does not use the ARCO's native `REVERSE` setting, reporting that it
  is offset by 90 degrees. The library offers no reverse either.
- **Angle range:** INDI maps 0–360° to the ±180° range the ARCO uses, counter-clockwise positive
  when seen from behind (**INDI**).

## GIOTTO flat panel

Sources: the PrimaLuce Lab "GIOTTO Communication Protocol" v2.1 (20 Feb 2023) and a GIOTTO with application
firmware 1.03.09, web 1.1, probed on 2026-10-24 (**HW (GIOTTO)**; transcript summary in
`indigo_drivers/aux_giotto/REFACTOR.md`). Used by `indigo_aux_giotto`.

| Purpose | Request | Reply | Known |
| --- | --- | --- | --- |
| Full state | `{"req":{"get": ""}}` | as v2.1, plus `MOTOR`, `LIGHT_FADING_TIME`, `STRIPLIGHT_1`/`_2`, LAN fields; `NTC_T`, `VIN_12V`, `VIN_USB` are top-level strings | HW (GIOTTO) |
| Light and brightness | `{"req":{"get":{"LIGHT":"","BRIGHTNESS":""}}}` | both fields in one reply | HW (GIOTTO) |
| Temperature and supplies | `{"req":{"get":{"NTC_T":"","VIN_12V":"","VIN_USB":""}}}` | strings | HW (GIOTTO) |
| Maximum brightness | `{"req":{"get":{"MAX_BRIGHTNESS":""}}}` | number (114 on the unit) | HW (GIOTTO) |
| Light on / off | `{"req":{"set":{"LIGHT":1}}}` | `"done"`; any number is acknowledged, only `1` switches on | HW (GIOTTO) |
| Brightness | `{"req":{"set":{"BRIGHTNESS":60}}}` | `"done"` for 0 to `MAX_BRIGHTNESS`, otherwise `"Error: invalid command"`; works with the light off | HW (GIOTTO) |
| Preset | `{"req":{"set":{"PRESET_1":{"NAME":"low","BRIGHTNESS":20}}}}` | `"done"` per member, `NAME` applied first; the brightness of a preset without a name is refused (`"Error: invalid command"`), a named one takes 0 to `MAX_BRIGHTNESS`; an empty name deletes and keeps the brightness | HW (GIOTTO) |
| Recall preset | `{"req":{"cmd":{"PRESET":1}}}` | `"done"`, sets the brightness only; `"Error: empty preset"`; above 9 `"Error: invalid command"` | HW (GIOTTO) |
| Fading | `{"req":{"set":{"LIGHT_FADING":0}}}` | `"done"` | HW (GIOTTO) |
| Light at power on | `{"req":{"set":{"LIGHT_ATPWON":1}}}` | `"done"` | HW (GIOTTO) |
| LEDs | `{"req":{"cmd":{"DIMLEDS":"low"}}}` | `{"cmd":{"cmd":"done"}}` | HW (GIOTTO) |

- **Members are independent:** a combined set is applied member by member, so `{"LIGHT":1,"BRIGHTNESS":200}`
  switches the light on and refuses the brightness. The library sets one value per request.
- **`LIGHT_STATUS`:** "off", "on" (light on, brightness 0) or "lighting"; "fading" never appeared with fading
  enabled, so the driver does not use it.
- **Unknown keys** answer `"Error: Obj's name not found or invalid command"` in their field; `get LOGLEVEL` is one.
- **Malformed input** (garbage, a request split across writes) gets no reply.
- **`LIGHT_FADING_TIME`** (10 on the unit) is accepted but undocumented; its unit is unknown.

## ALTO cover

Sources: no PrimaLuce document; the INDI `alto.cpp` and an ALTO1 with application firmware 1.03.11, web 1.01, probed
on 2026-10-24 (**HW (ALTO1)**; summary in `indigo_drivers/aux_alto/REFACTOR.md`). Used by `indigo_aux_alto`.

| Purpose | Request | Reply | Known |
| --- | --- | --- | --- |
| Motion | `{"req":{"get":{"MOT1":{"POSITION":"","STATUS":""}}}}` | `POSITION` 0 (closed) to 100 (open), `STATUS.MST` stop/acc/CstSpeed/dec, `STATUS.BUSY` | HW (ALTO1) |
| Environment | `{"req":{"get":{"VIN_12V":"","VIN_USB":"","MOT1":{"NTC_T":""}}}}` | supplies as strings, `NTC_T` as a number | HW (ALTO1) |
| Move | `{"req":{"set":{"MOT1":{"POSITION":50}}}}` | `"done"`; about 7400 steps/s, a full stroke takes about 34 s; a new target during a move decelerates to it | HW (ALTO1) |
| Stop | `{"req":{"cmd":{"MOT1":{"MOT_STOP":""}}}}` | `"done"`, decelerated stop within about 1 s; `MOT_ABORT` stops at once | HW (ALTO1) |
| Preset | `{"req":{"set":{"PRESET_1":{"NAME":"Flats","M1POS":119804}}}}` | `"done"` per member, `M1POS` in steps, refused without a name; 3 presets | HW (ALTO1) |
| Recall preset | `{"req":{"cmd":{"PRESET":1}}}` | `"done"` and the cover moves; `"Error: empty preset"` | HW (ALTO1) |
| Calibration | `{"req":{"cmd":{"MOT1":{"CAL_ALTO":"Init"}}}}` | `Init`, `Close_Fast`/`Close_Slow`, `MOT_STOP`, `StoreAsClosedPos`, `Open_Fast`/`Open_Slow`, `MOT_STOP`, `StoreAsMaxOpenPos` | INDI |

- **`POSITION` is not validated:** 101, -1, 50.5 and `"x"` are all acknowledged `"done"` and can start a move; send only
  integers 0 to 100.
- **Readback lag:** `ABS_POS_STEP` changes only about every 0.5 s; the first stopped readback can carry the previous
  step count while `POSITION` is already final.
- **`SPEED`** is the current motor speed: a `set` is acknowledged and ignored.
- **Unknown keys:** top level `"ERROR: Obj's name not found or invalid command"`, inside `MOT1` `"Error: parameter not found"`.
- INDI fixed two inversions in its first version: park/unpark (0 is closed) and swapped open/close calibration steps.
