# Specification

Status: approved (owner, 2026-11-07)

## Problem and outcome
During a calibration the ALTO driver stops the arm with `MOT1 MOT_STOP`. Four PrimaLuce sources define the calibration
stop as `{"req":{"cmd":{"MOT1":{"CAL_ALTO":"Motor_Stop"}}}}`: the protocol v1.3, PLAY 1.9.8, the ALTO web page (web
firmware 1.01) and the ALTO firmware 1.03.11. After the change the driver sends that request wherever it stops the arm
as a calibration step.

## Current-state audit (scaled to this fix)
- **Driver:** generated driver `indigo_drivers/aux_alto/indigo_aux_alto.driver`, version 9, INDIGO 3.0 API. It uses
  serial and HTTP transport through the static library `indigo_drivers/shared/primalucelib`. Calibration is a manual
  three-phase sequence:
  - `X_CALIBRATION.START` sends `CAL_ALTO Init`.
  - `X_CALIBRATION_MOTION` sends `Close_Fast`, `Close_Slow`, `Open_Fast` or `Open_Slow`. Its `STOP` item calls
    `primalucelib_cover_stop()`, which sends `MOT_STOP`.
  - `X_CALIBRATION.STORE` calls `primalucelib_cover_stop()` (`MOT_STOP`), then sends `StoreAsClosedPos` in phase 1 or
    `StoreAsMaxOpenPos` in phase 2.
  - `CANCEL` sends `MOT_STOP` and ends the phase.
  - A disconnect while a calibration runs sends `MOT_STOP`.
- **Protocol sources:**
  - `PROTOCOL.md` attributes the calibration row to "INDI" and lists `MOT_STOP`.
  - The PrimaLuce document v1.3 lists `Motor_Stop` as a `CAL_ALTO` value, sent after each close or open phase and before
    the store.
  - PrimaLuce's web page sends only `StoreAs...` on its store buttons; its Stop button sends `CAL_ALTO Motor_Stop`.
  - The firmware strings show a `Calib_Handler_ALTO` that accepts `Motor_Stop` and logs its own line for it.
  - Unknown: whether the firmware refuses a store, or misbehaves, after a plain `MOT_STOP`. There was no firmware
    disassembly and no hardware run.
- **Defect:** the calibration stop request differs from every PrimaLuce source. It was found by source audit against the
  document and has not been reproduced on hardware.
- **Simulator:** `alto_simulator.c` refuses `Motor_Stop` as an invalid calibration step and stops the motion itself on
  `StoreAs...`. It therefore mirrors the INDI assumption, which is why the current `calibration` case passes against it.
- **Tests:**
  - `indigo_test/integration/test_aux_alto_simulator.c`, case `calibration`: counts `MOT_STOP` only for CANCEL.
  - `indigo_test/unit/test_primalucelib.c`, case `cover_commands`: checks `CAL_ALTO Close_Fast`.
- **Platforms and build:** unchanged. Linux x64 is available here; macOS and Windows are not.
- **Concurrency and lifecycle:** unchanged. Every request remains a synchronous transaction on the device queue.

## Hardware-test decision
Initially no hardware test (non-interactive simulator run). Revised by the owner on 2026-11-07: an interactive hardware
run on the ALTO1 (application firmware 1.03.11, web 1.01, `/dev/serial/by-id/usb-Silicon_Labs_CP2102N_..._b00016cac822f011929b15d936263638-if00-port0`)
with the existing 9-case suite, a guided calibration judged by the owner's eye, and physical hot-plug.

## In scope
- primalucelib constant `PRIMALUCE_COVER_CALIBRATION_MOTOR_STOP` (`"Motor_Stop"`).
- Driver:
  - `X_CALIBRATION_MOTION.STOP` and both `X_CALIBRATION.STORE` steps use `CAL_ALTO Motor_Stop`.
  - Version 9 → 10, with regenerated outputs.
- Simulator: `Motor_Stop` is a valid calibration step. It stops the arm with the same decelerated stop as `MOT_STOP`.
- Tests and `PROTOCOL.md`.

## Out of scope
- Calibration CANCEL and disconnect. These are safety stops with no PrimaLuce counterpart and keep `MOT_STOP`.
- Ordinary motion, abort, presets, LEDs and WiFi.
- Firmware disassembly.
- Hardware run.
- Driver README text, except the scripted test record.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | `X_CALIBRATION_MOTION.STOP` during a calibration sends exactly `{"req":{"cmd":{"MOT1":{"CAL_ALTO":"Motor_Stop"}}}}` and no `MOT_STOP`. | Simulator integration case `calibration` (request journal) |
| AC-002 | Each `X_CALIBRATION.STORE` sends `CAL_ALTO Motor_Stop` immediately before its `StoreAsClosedPos` / `StoreAsMaxOpenPos`, and no `MOT_STOP`. The calibration completes with the new travel. | Simulator integration case `calibration` (request order) |
| AC-003 | A refused `Motor_Stop` ends STOP and STORE ALERT with the controller's reason. No store is sent after a refused stop. | Simulator integration case `calibration_stop_refused` (fault injection) |
| AC-004 | Calibration CANCEL still sends `MOT_STOP`. | Simulator integration case `calibration` |
| AC-005 | `primalucelib_cover_calibration_step(link, PRIMALUCE_COVER_CALIBRATION_MOTOR_STOP)` sends the documented request and accepts `"done"`. | Unit case `cover_commands` |
| AC-006 | The simulator accepts `Motor_Stop` only during a calibration, and stops the arm with a decelerated stop. | Simulator integration case `calibration` (position settles) |
| AC-007 | The driver version is higher than 9, and the generated outputs match the `.driver` source. | Regeneration diff and header check |
| AC-008 | The full ALTO simulator suite and the primalucelib unit suite pass. The recorded run is made with `tools/run_driver_test.py aux_alto`. | Script run |
| AC-009 | After `StoreAsMaxOpenPos`, X_CALIBRATION stays BUSY until `CAL_MAXPOS` differs from the value read before the store (polled every 0.5 s, up to 20 times); the driver then uses that travel, updates X_PRESETS and reports "Calibration complete, travel N steps"; if it never changes the calibration ends ALERT and the previous travel is kept. | Simulator `calibration`, `calibration_travel_not_reported`; unit `cover_commands` |
| AC-010 | While a calibration runs, the cover position is not polled or published; X_COVER_POSITION and AUX_COVER are IDLE from START until the calibration ends or is cancelled, then show the polled position OK. | Simulator `calibration` |
| AC-011 | Pulling the USB cable while idle makes the cover properties ALERT, the driver disconnects without hanging, and after the replug it connects, reads the position and moves. | Hardware `alto_survives_the_usb_unplug` (opt-in, `--hot-plug`) |
| AC-012 | The driver passes the ALTO1 hardware suite and the guided calibration on the real device. | Hardware runs |

## Failure behavior and edge cases
A refused or unanswered `Motor_Stop` keeps the current phase and reports ALERT with the controller's reason, as a refused
`MOT_STOP` does today. STORE outside a calibration is refused before anything is sent (unchanged).

## Permissions, data, and security requirements
No new inputs: the step string is a compile-time constant. No credentials or network changes.

## Compatibility and nonfunctional requirements
The wire change affects calibration only. Other primalucelib users (focuser, panel) are unaffected, because only a
constant is added.

## Assumptions and open questions
- Low-risk assumption: the firmware treats `Motor_Stop` as a decelerated stop, like `MOT_STOP`. The document calls it a
  "stop" and gives no sign of an abrupt one. The simulator models it that way.
- Open, hardware only: whether firmware 1.03.11 refuses or mis-stores after a plain `MOT_STOP`. Not blocking.

## Owner decisions and requirement changes
- 2026-11-07, Rumen G. Bogdanovski: interactive hardware run with guided calibration and hot-plug; then "yes fix 1 and 2
  and add hotplug" for the two defects found on hardware (AC-009, AC-010) and the hot-plug case (AC-011). Version 11.
- 2026-11-07, Rumen G. Bogdanovski: approved the change as proposed (calibration STOP and STORE use `Motor_Stop`; CANCEL
  and disconnect keep `MOT_STOP`), with a non-interactive simulator run at the end.
