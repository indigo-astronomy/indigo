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
