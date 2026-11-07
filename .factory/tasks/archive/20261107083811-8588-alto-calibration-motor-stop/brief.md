# Task brief

- Task ID: 20261107083811-8588-alto-calibration-motor-stop
- Owner: Rumen G. Bogdanovski
- Created: 2026-11-07

## Problem and desired outcome
`indigo_aux_alto` stops the cover arm during a calibration with the ordinary motor stop
`{"req":{"cmd":{"MOT1":{"MOT_STOP":""}}}}`. That sequence came from the INDI driver and was never run on hardware. The
PrimaLuce ALTO communication protocol v1.3 and PrimaLuce's own software stop a calibration step with
`{"req":{"cmd":{"MOT1":{"CAL_ALTO":"Motor_Stop"}}}}`. The driver must use the documented calibration stop.

## Users and use cases
ALTO owners who calibrate the cover (closed and fully open positions) from an INDIGO client.

## Scope and non-goals
In scope: the calibration stop (`X_CALIBRATION_MOTION.STOP` and the stop sent before each `X_CALIBRATION.STORE`), the
shared primalucelib constant, the ALTO simulator, the ALTO simulator test and the primalucelib unit test, `PROTOCOL.md`,
and the driver version. Non-goals: ordinary moves, abort, presets and WiFi. The safety stops on calibration CANCEL and
disconnect keep `MOT_STOP`. No hardware run.

## Constraints and references
- Owner request on 2026-11-07: "yes" to the proposed change, with a non-interactive simulator run at the end.
- Sources:
  - `PrimaluceLab - ALTO - Communication Protocol - v1.3.pdf` (supplied by the owner, not checked in).
  - PrimaLuce ALTO software package 1.4 and PLAY 1.9.8 from https://www.primalucelab.com/downloads/. These were
    inspected by string extraction only; nothing was executed. PLAY `TelescopeManager.exe`, the ALTO web firmware
    `Alto_web_v1.01.bin` and the ALTO application firmware `Alto_v1.03.11.bin` all use `CAL_ALTO` `Motor_Stop` for the
    calibration stop. The ASCOM driver `PLL ASCOM CoverCalibrator v1.4` has no calibration.

## Initial questions
None open; the owner approved the change and the test mode.
