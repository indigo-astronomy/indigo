# indigo_aux_giotto: development record

New driver for the PrimaLuce Lab GIOTTO flat panel, task 20261024092953-7ee4-primaluce-giotto-alto-drivers
(started 2026-10-24). The ALTO cover gets its own driver (`indigo_aux_alto`) in a follow-up task; the
requester chose two drivers because the GIOTTO and the ALTO are separate USB devices and the generator
shares one connection between the devices of a driver.

## Audit of the starting point

- **No INDIGO driver existed.** `primalucelib` (`indigo_drivers/shared/primalucelib`) had five panel calls
  taken from the INDI `giotto.cpp` (maximum brightness, brightness get/set, light get/set), unit-tested
  against scripted replies only and used by no driver.
- **Protocol sources:**
  - PrimaLuce Lab "GIOTTO Communication Protocol" v2.1 (20 Feb 2023, software 1.3), supplied by the
    requester (`~/Downloads`, not in the repository);
  - INDI `giotto.cpp` and `primalucacommandset.cpp`;
  - the requester's GIOTTO, probed on 2026-10-24 (below).
- **Transport:** the shared `primalucelib` JSON link: USB serial at 115200 8N1, or WiFi over HTTP
  (`http://host[:port]`). The library skips `[`-prefixed log lines.
- **Platforms:** the driver is generated and portable C on top of `primalucelib`; it builds on Linux, macOS and
  Windows through the same project files as `indigo_focuser_primaluce`. Only Linux is available to run here.

## Hardware observations (requester's GIOTTO, 2026-10-24)

Unit: `MODNAME` "GIOTTO", serial `GIOTTO12000415`, `SWAPP` 1.03.09, `SWWEB` 1.1, USB bridge Silicon Labs
CP2102N `10c4:ea60` "CP2102N USB to UART Bridge Controller" (the same identity as the PrimaLuce focusers).
Probed on `/dev/ttyUSB0` with scripted requests while the requester watched; raw transcripts are in the
session scratchpad (`giotto/hw/`).

| Area | Observation |
| --- | --- |
| Full state (`get ""`) | As in the document, plus `SWID`, `LANCFG`/`LANSTATUS`/`GHOSTMODE`/`WIFISTA`, `MOTOR` 0, `LIGHT_FADING_TIME` 10 and `STRIPLIGHT_1`/`_2` calibration (`LT_CALMIN` 8, `LT_CALMAX` 122). `NTC_T`, `VIN_12V`, `VIN_USB` are top-level strings. `CALRESTART` has `CH1`/`CH2`. `LOGLEVEL` "errors". |
| Combined get | `{"get":{"LIGHT":"","BRIGHTNESS":"","LIGHT_STATUS":"","NTC_T":"","VIN_12V":""}}` answers all fields in one reply. |
| Unknown key | `"Error: Obj's name not found or invalid command"` in its field. |
| `set LIGHT` | `1` / `0` acknowledged `"done"`. Any other value (`2`) is also `"done"` but the light stays off. |
| `set BRIGHTNESS` | 0 to `MAX_BRIGHTNESS` (114) `"done"`. 115, 200 and -1 give `"Error: invalid command"` and keep the value. Works with the light off; switching off keeps the value. |
| `LIGHT_STATUS` | "off", "on" (light on, brightness 0), "lighting". "fading" never appeared, although `LIGHT_FADING` was 1 and the brightness changed 0→20→60. |
| Combined set | Applied field by field: `{"LIGHT":1,"BRIGHTNESS":200}` switched the light on and refused the brightness. |
| Presets | `PRESET_n` `NAME`+`BRIGHTNESS` (`"done"` per field, `NAME` applied first). The brightness of a preset without a name is refused, whatever the value; a named preset takes 0 to the maximum (see found defect 1). `cmd PRESET n` sets the brightness only (the light stays off); an empty preset gives `"Error: empty preset"`, an index above 9 `"Error: invalid command"`. A deleted preset keeps its brightness. |
| Settings | `LIGHT_FADING`, `LIGHT_FADING_TIME` (undocumented) and `LIGHT_ATPWON` acknowledged `"done"`. |
| `DIMLEDS` | Acknowledged as `{"cmd":{"cmd":"done"}}`, as on the SESTO SENSO 3. |
| `LOGLEVEL` | Prints `[mLogs_DefaultSetup]` before `"done"`; `get LOGLEVEL` is not supported. |
| Malformed input | Garbage and a request split across two writes get no reply; the next request is answered normally. |

All values were restored afterwards. `PRESET_1.BRIGHTNESS` stayed 40 at first, because the first probe cleared the
name and set brightness 0 in one request; it was reset after the rule above was found (name, brightness 0, empty
name).

## Design

- One AUX device "PrimaluceLab GIOTTO", interface `INDIGO_INTERFACE_AUX_LIGHTBOX`, generated from
  `indigo_aux_giotto.driver`, linked with `primalucelib`.
- **Connect:** open the link, require a `MODNAME` starting with "GIOTTO", switch the log output off, read the
  full state. Model, firmware and serial number go to `INFO`. `CALRESTART` channels report "needs calibration".
  A missing or non-positive `MAX_BRIGHTNESS` fails the connection.
- **`AUX_LIGHT_SWITCH`:** `set LIGHT` 1/0; the panel keeps its state, so the switch is read from the device
  at connect and not stored by INDIGO.
- **`AUX_LIGHT_INTENSITY`:** the device's brightness level, 0 to `MAX_BRIGHTNESS` (read at connect), not a
  percentage: the levels map one to one, and a percentage of 114 levels would lose or duplicate levels. It
  can be changed with the light off, as the panel allows.
- **Presets:** `X_PRESETS` (brightness of presets 1–9), `X_PRESET_NAMES` and `X_PRESET_RECALL`, as the focuser
  driver's stored positions. Recall sets the brightness only, as the panel does. A brightness refused for a preset
  the driver knows has no name is reported as "Preset #n needs a name before its brightness can be set".
- **Connection helper:** `giotto_open()` checks the model, switches the log output off, reads the full state and
  requires a brightness range; it releases the port itself on any failure, so `on_connect` cannot fail. A
  generated single-device driver does not close the port when `on_connect` fails.
- **Settings:** `X_FADING` (`LIGHT_FADING`), `X_POWER_ON_LIGHT` (`LIGHT_ATPWON`), `X_LEDS` (`DIMLEDS`).
  `LIGHT_FADING_TIME` is not exposed: it is undocumented and its unit is unknown.
- **State:** `X_STATE` (panel temperature `NTC_T`, 12 V and USB supply), read every fifth poll.
- **Polling:** light and brightness every 2 s in one request; changes made elsewhere (web page, WiFi client)
  are published. A failed poll publishes ALERT with the last values; the next good poll restores OK.
- **Disconnect:** the light is switched off, as `aux_flatmaster`, `aux_wcv4ec` and `aux_fbc` do (`aux_arteskyflat`,
  `aux_flipflat` and `aux_geoptikflat` leave it); the brightness is kept, because the panel stores it. Requested
  by the requester on 2026-10-24.
- **Failures:** a refused request ends ALERT with the controller's reason and the property shows the device's
  value again.
- **USB preference:** the CP2102N pattern with `10c4:ea60`, as `indigo_focuser_primaluce`; the model check
  at connect refuses a focuser.

## Hardware-test decision

Hardware testing will be done on the requester's GIOTTO (above): connect, light on/off, brightness range and
refusal, presets, settings, LEDs, polling of an external change, disconnect/reconnect, and the opt-in
`test_aux_giotto_hw` suite through `python3 tools/run_driver_test.py aux_giotto --hw`. Physical hot-plug is
covered only if the requester asks for it.

## Plan

| Step | Change | Verification | State |
| --- | --- | --- | --- |
| 1 | Baseline: `primalucelib` unit tests at `83899ad9b` | 27/27 pass | done |
| 2 | `primalucelib`: panel state, environment, presets, settings, calibration flags; refusals through `reply_is_acknowledged` | unit tests | done: new case "flat panel state, presets and settings", 28/28 |
| 3 | Simulator `giotto_simulator` (jsmn-parsed requests, member-by-member answers) modelled on the document and the hardware observations | builds, manual exchange | done: the hardware probe sequence replayed against it gives the same replies |
| 4 | `indigo_aux_giotto.driver`, generated outputs | builds | done: no warnings |
| 5 | Integration suite `test_aux_giotto_simulator.c` | all cases pass; failure cases proven by mutation | done: 25/25; eight driver mutations each failed their case (below) |
| 6 | Build and project registration: `Makefile.inc`, root `Makefile`, `indigo_server.c`, `indigo_test/Makefile`, Windows project and solution, Xcode | builds; project files parse | done: driver, server, suites build on Linux; the `.vcxproj`/`.filters` parse as XML; Xcode objects checked structurally |
| 7 | Documentation: `PROPERTIES.md`, `PROTOCOL.md`, `MIGRATION_STATUS.md`, `README.md` (with the requester's approval) | review | done; `README.md` added with the requester's approval (2026-10-24) |
| 8 | Opt-in hardware suite `test_aux_giotto_hw.c` | 8/8 on the unit | done: recorded `2026-10-24 14:31 3.0.0.1 linux x64 GIOTTO 8/8 OK` |
| 9 | Recorded simulator run | `run_driver_test.py aux_giotto` | done: recorded `2026-10-24 14:29 3.0.0.1 linux x64 simulator 25/25 OK` |

There is no original driver, so the characterization-first and reference-trace rules for migrations do not
apply; the hardware observations above are the compatibility contract.

## Scenario mapping

| Scenario (DRIVER_TESTING_RULES, AUX and common) | Simulator case | Hardware case |
| --- | --- | --- |
| Interface bit, metadata, connect sequence, INFO | `metadata` | `giotto_publishes_the_property_contract` |
| USB preference | `usb_match_pattern` | (`indigo_focuser_primaluce` hardware check of the same pattern) |
| Property contract before/after connect, ranges, read-only state | `property_contract` | `giotto_publishes_the_property_contract` |
| Values read at connect, nothing written | `connect_reads_the_panel` | `giotto_reconnects` |
| Wrong device, no brightness range, silent handshake (rollback, retry) | `unsupported_device`, `no_brightness_range`, `handshake_timeout` | not applicable |
| Calibration flag message | `needs_calibration` | not applicable (the unit reports none) |
| INIT/SHUTDOWN, shutdown refused while connected, reconnect, second disconnect | `repeated_init_shutdown`, `shutdown_rejected_while_connected`, `reconnect` | `giotto_rejects_shutdown_while_connected`, `giotto_reinitializes`, `giotto_reconnects` |
| Light off at disconnect, brightness kept | `disconnect_switches_the_light_off` | `giotto_reconnects` |
| Light switch, refusal with reason | `light_switch`, `light_switch_refused` | `giotto_switches_the_light` |
| Intensity, range end, kept while off, refusal, unanswered request | `intensity`, `intensity_refused`, `intensity_unanswered` | `giotto_sets_the_intensity` |
| External change, poll failure and recovery, environment poll and failure | `external_change_polled`, `poll_failure`, `environment_polled` | idle polls confirm values in `giotto_sets_the_intensity`; an external change needs the web page, not done |
| Presets: write, recall, delete, empty recall, unnamed preset, refusal, unsendable name | `presets`, `presets_refused` | `giotto_uses_presets` |
| Settings with readback after reconnect, refusals, unanswered LED command | `settings`, `settings_refused` | `giotto_changes_the_settings` |
| Transport loss | `transport_loss` | not done (no hot-plug requested) |

Generic range and type validation is the framework's and is not tested here. There is no motion, so the
abort, BUSY and request-versus-poll scenarios of motion classes do not apply.

Mutation checks (each change made to the driver, the named case run, the driver restored): refusal reason
dropped (`light_switch_refused`), failed poll not ALERT (`poll_failure`), recall not reading the brightness back
(`presets`, after the case was tightened to check the intensity before any poll), model check removed
(`unsupported_device`), external change ignored (`external_change_polled`), refused intensity not restored
(`intensity_refused`), unnamed-preset message removed (`presets_refused`), light not switched off at disconnect
(`disconnect_switches_the_light_off`). All eight failed as expected.

## Hardware runs

- 2026-10-24, development run of `test_aux_giotto_hw` (`GIOTTO_HW_PORT=/dev/ttyUSB0 make test-aux-giotto-hw`):
  7/8, `giotto_uses_presets` failed. Setting brightness 0 on the named preset ended OK where the test expected
  the refusal the first probe seemed to show; see found defect 1.
- After the fix, the same run: 8/8. The panel reported 51.68 °C and 13.13 V; a read-only probe afterwards found
  light off, brightness 0, fading on, light at power-on off, LEDs on and presets 8 and 9 empty with brightness 0,
  as before the run.
- After the light-off-at-disconnect change, the same run: 8/8. The panel was on at brightness 100 when the run
  started (left by the requester's own session); the run restored that, and its final disconnect switched the
  light off with the brightness kept at 100, as intended.

## Found defects

1. **Preset brightness rule misread** (found by the hardware run, reproduced in the simulator).
   - Impact: the simulator and the first design assumed the panel refuses a preset brightness of 0. In fact it
     refuses any brightness for a preset without a name and accepts 0 for a named one, so a client setting a
     brightness on an unnamed preset got only "Error: invalid command", and the simulator refused 0 on named presets.
   - Root cause: the first probe sent `{"NAME":"","BRIGHTNESS":0}` in one request; the panel applies `NAME` first,
     so the brightness met an unnamed preset. A re-probe on 2026-10-24 separated the two.
   - Fix: the simulator refuses the brightness of an unnamed preset and accepts 0 to the maximum otherwise; the
     driver reports "Preset #n needs a name before its brightness can be set" for a refusal on a preset it knows is
     unnamed; the library comment is corrected.
   - Regression tests: `presets_refused` (unnamed preset refused with the message, 0 accepted on a named preset,
     another refusal passes the panel's reason on; fails with the message special case removed) and the hardware
     case `giotto_uses_presets`.

## Final test summary (3.0.0.1)

- Simulated tests: 25 run, 25 passed: `2026-10-24 14:29 3.0.0.1 linux x64 simulator 25/25 OK`.
- Hardware tests: 8 run, 8 passed on the requester's GIOTTO (firmware 1.03.09):
  `2026-10-24 14:31 3.0.0.1 linux x64 GIOTTO 8/8 OK`.
- Library unit tests: 28 run, 28 passed (`test_primalucelib`).
- Not verified: Windows and macOS builds (no tools here), the WiFi connection, physical hot-plug (not requested).

## Poll versus pending request (3.0.0.2, 2026-10-24, task 20261024161128-3861-giotto-request-versus-poll)

Found by the repository audit 20261024121920-3bd2-repository-audit as FND-20261024121920-3bd2-repository-audit-021.

2. **A poll replaced a queued light or brightness request** (reproduced, then fixed).
   - Impact: a client request accepted while an idle poll was in flight (the framework copies the request into the
     property, marks it BUSY and queues the handler) was overwritten when that poll found a change made elsewhere or
     recovered from ALERT: the poll published its readback over the value and target, and the handler then sent the old
     value and reported success. Reproduced: a brightness of 40 requested during a poll that read 77 sent
     `{"set":{"BRIGHTNESS":77}}` and never 40.
   - Root cause: the poll published both light properties without checking whether a request was pending.
   - Fix: after its exchange the poll leaves a BUSY property, and the value the driver remembers for it, untouched; the
     ALERT of a failed poll skips it too. The next poll compares again. The switch and the intensity are handled
     separately, so the other one is still published.
   - Simulator: `external <light> <brightness> slow` delays the reply to the poll that sees the change by 400 ms, so a
     request arrives while it is in flight.
   - Regression test: `request_versus_poll` (40 sent once, 77 never, intensity OK at 40 with target 40); it failed on
     3.0.0.1 with the journal above.
   - Hardware: not rerun; the race needs the simulator's delayed reply to be reproducible, and the light and brightness
     handling the hardware suite covers is unchanged otherwise.

## Final test summary (3.0.0.2)

- Simulated tests: 26 run, 26 passed: `2026-10-24 19:17 3.0.0.2 linux x64 simulator 26/26 OK`.
- Hardware tests: 0 run for this change (the last hardware run, 8/8, was on 3.0.0.1).

## Token lookup bound in primalucelib (3.0.0.3, 2026-10-24, task 20261024164718-0e69-primalucelib-token-bounds)

FND-20261024121920-3bd2-repository-audit-017: a lookup in a reply that used all 1024 JSON tokens could read past the
token array of the shared `primalucelib` (reproduced with AddressSanitizer at `primalucelib.c:399`). The library now
bounds every lookup by the number of tokens the parser filled; the regression test is the library unit case "lookups
stay within a full token array". This driver links the library, so its version is raised; nothing else in the driver
changed. The recorded simulator run of this version is in README.md.

## WiFi configuration (3.0.0.4, 2026-10-24, task 20261024194838-6cb6-primaluce-panel-cover-wifi)

The driver could use the panel's WiFi (an `http://host` port) but not configure it: the specification of 3.0.0.1 left
the WiFi configuration out ("WiFi untested with the GIOTTO"). The panel reports the same network keys as the PrimaLuce
focusers (`LANCFG`, `WIFIAP`, `WIFISTA`, captured on the requester's GIOTTO), so it gets the focuser's properties under
the same names:

- `X_WIFI` (OFF, AP, STA) through `primalucelib_select_wifi()`: OFF and AP switch the access point in the `ap` mode; a
  change between station mode and the access point writes `LANCFG` and restarts the panel, waiting over USB until it
  answers again. Over HTTP the driver says that the panel leaves its network. Read from the full state at connect.
- After a restart the light comes back as `LIGHT_ATPWON` selects; the poll publishes it.
- `X_WIFI_AP` and `X_WIFI_STA` write the credentials; the library refuses an SSID over 32 characters (or an empty access
  point SSID), an access point password outside 8 to 63 characters, and a station password over 63, with the reason as
  the message. `"` and `\` are sent escaped (merged from `indigo_focuser_primaluce` 3.0.0.42); the GIOTTO firmware
  decodes them.

Hardware (requester's GIOTTO, application firmware 1.03.09, over USB, 2026-10-24; driver session through
`indigo_prop_tool`): the connect values matched the panel (`ap`, access point on, `GIOTTO12000415`); a 3 character
access point password was refused with the reason and not sent; station credentials were written and read back on
reconnect; OFF and AP took about 0.1 s, STA and back about 4.3 s each (one restart each), and the reconnect showed AP.
The light stayed off (`LIGHT_ATPWON` 0) with the brightness kept, and `CALRESTART` stayed 0. All values were restored.

The same requests were probed in detail on the ALTO1, which runs the same controller firmware family:

Hardware (requester's ALTO1, application firmware 1.03.11, over USB, 2026-10-24; raw probe and driver session):
- `WIFISTA` and `WIFIAP` writes are acknowledged per field (`{"SSID":"done","PWD":"done"}`) and read back; `AP_SET_STATUS`
  `off`/`on` is acknowledged and read back as `WIFIAP.STATUS`.
- The firmware checks nothing: a 3 character access point password and the mode `xyz` were stored and acknowledged
  (both restored at once, before any restart). The library now refuses what WiFi cannot use before sending it.
- `LANCFG` `sta` plus `REBOOT`: acknowledged, ESP32 boot log, `MODNAME` answered again after about 2 s, `LANCFG` `sta`
  with `WIFIAP.STATUS` `off`; back to `ap` the same way. The calibration (`CALRESTART` 0) and the position were kept.
- The access point reports `off` for about 4 s after the controller answers and `on` from about 6 s after `REBOOT`; a
  reconnect within that time shows OFF.

Simulator: stores `LANCFG` and the credentials as sent, applies `LANCFG` at `REBOOT` (boot log, 1 s without answers,
light as `LIGHT_ATPWON`), reports `WIFIAP.STATUS` `off` in station mode, and answers `AP_SET_STATUS`.

| Scenario | Test |
| --- | --- |
| WiFi properties and connect values | `property_contract` |
| Credentials written, read back on reconnect, refused before sending | `wifi_credentials` |
| OFF without restart, STA and OFF from STA with one restart each, AP, panel served after a restart | `wifi_modes` |
| Refused mode keeps the mode and restarts nothing | `wifi_mode_refused` |
| Library: mode selection, restart only on change, credential limits | `test_primalucelib` |

Not verified: the HTTP path of a mode change, Windows and macOS builds.

## Final test summary (3.0.0.4)

- Simulated tests: 29 run, 29 passed: `2026-10-24 23:12 3.0.0.4 linux x64 simulator 29/29 OK`.
- Hardware tests: the WiFi driver session above; the recorded hardware suite was not rerun.

## Firmware text bound (3.0.0.5, 2026-10-25, task 20261024213010-b356-primaluce-panel-cover-firmware-bound)

The connect handler appended the web firmware version to the application version in `INFO` with two `strcat()` calls,
copied from `indigo_focuser_primaluce` before that driver bounded it (FND-20261024121920-3bd2-repository-audit-029,
fixed there in 3.0.0.42). Versions longer than the 511 characters of the item overflowed it.

**Fixed: long firmware versions overflowed the INFO item** (reproduced, the same flaw as audit finding 029).
- Fix: the web version is appended with `snprintf()` into the space left in the item, so a long version is cut off.
- Simulator: profile `long-firmware` reports 300 character application and web versions.
- Regression test: `long_firmware` (the item holds the first 511 characters of "<app> / <web>" and the serial number is
  intact); it aborted with signal 6 (fortified buffer overflow check) on 3.0.0.4.

## Final test summary (3.0.0.5)

- Simulated tests: 30 run, 30 passed: `2026-10-25 00:31 3.0.0.5 linux x64 simulator 30/30 OK`.
- Hardware tests: 0 run for this change (real firmware versions are short).

## WiFi status (3.0.0.6, 2026-10-25, task 20261024221319-1235-primaluce-wifi-status)

The driver showed no address to put in an `http://` port. The read-only `X_WIFI_STATUS` now shows where to connect:
`IP_ADDRESS` (the address on the joined network in station mode, the access point address while the access point is on,
empty when it is off), `PORT` (80, the controller's web server, shown with an address) and `CONNECTION` (in station mode
`LANSTATUS.LANCONN`, e.g. `disconnected` or `gotip`; otherwise `access point` or `off`). It is read from the full state
at connect, after a WiFi mode change and with the panel state every 10 s, which also shows an address the panel gets some
time after a restart; it is published only when it changes. A SESTO SENSO 3 in access point mode still reported the
station address of an earlier session, so that address is not used outside station mode.

`X_WIFI_AP` and `X_WIFI_STA` now show what the panel stored after a change: the credentials are read back, a kept access
point SSID is shown with the message "The controller kept the SSID ...", and a refused change shows the stored values
again. The library decodes the JSON escapes of reply strings, so a credential with `"` or `\` reads back as entered.

Hardware (requester's GIOTTO, firmware 1.03.09, over USB, 2026-10-25; driver session through `indigo_prop_tool`, with
the first layout of this property): at connect the access point address and no station address, although the panel
reported the address of an earlier station session in access point mode (the case the rule is for). Seven OFF/AP
switches: six showed the new state at once; one OFF sent about 5 s after AP was still read back `on` by the panel and
was shown by the next 10 s refresh. Right after AP the panel took about 3 s to answer. The requester found that the panel
keeps its access point SSID in access point mode while the property kept the requested one; this led to the read-back
below.

Final layout on the same panel (2026-10-25, driver session through `indigo_prop_tool`): at connect `IP_ADDRESS`
192.168.4.1, `PORT` 80, `CONNECTION` `access point`, and the stored credentials. A new access point SSID was acknowledged
but the panel kept its own; the read-back showed the kept SSID. A 3 character password ended ALERT with the reason and the
stored password shown again. A station password with `\` was sent escaped and read back as entered. After OFF the panel
again reported the access point `on` at once, so the status showed it until the next refresh. All values were restored.

| Scenario | Test |
| --- | --- |
| Connect values in access point mode (address, port 80, `access point`), read-only | `property_contract` |
| Station address, port and connection after a switch to station mode | `wifi_modes` |
| Library: request, address rules, credentials with decoded escapes, a failed reply | `test_primalucelib` (`wifi status shows where the controller is`) |
| OFF leaves no address and shows `off`; a kept SSID is shown with a message; a refused change shows the stored values | `wifi_modes`, `wifi_credentials` |

Not verified: the station address on hardware (needs a network the device joins), the periodic refresh picking up a
later address (the simulator assigns the address at the restart).

## Final test summary (3.0.0.6)

- Simulated tests: 30 run, 30 passed: `2026-10-25 02:39 3.0.0.6 linux x64 simulator 30/30 OK`.
- Hardware tests: the driver sessions above; the recorded hardware suite was not rerun.
