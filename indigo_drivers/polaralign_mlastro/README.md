MLAstro Robotic Polar Alignment (RPA) driver
=============================================

https://mlastro.com/

## Supported devices

MLAstro RPA polar-alignment platform (ESP32-based controller, USB-serial at
115200 8N1, CH340 or CP2102 USB-to-serial chip).

Single device is present on the mount, no hot-plug detection is possible
(the USB-serial chip is generic and shared with many other devices), so the
serial port must be selected manually.

## Supported platforms

This driver is platform independent (Linux, macOS, Windows).

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_polaralign_mlastro

## Protocol

https://github.com/MLAstroRPA/MLAstroRPA.NINA.Plugin/blob/main/Documentation/Serial-protocol.md

Command usage was cross-checked against the "MLAstro RPA" driver in the INDI
project (`drivers/auxiliary/mlastro_rpa.cpp`/`.h` in indilib/indi). Behaviour
the protocol document leaves open (the `Disconnect` command, the meaning of
`Mpos` versus `AzPH`/`AlPH`, unsolicited lines) follows the MLAstro NINA plugin
(`Services/SerialConnectionService.cs` in the same repository as the protocol).

## Properties

Beyond the standard `POLARALIGN_OFFSET`, `POLARALIGN_ABORT_MOTION`,
`POLARALIGN_STEPS_PER_DEGREE`, `POLARALIGN_DIRECTION_ALT`/`_AZ`,
`POLARALIGN_RESET_POSITION_ALT`/`_AZ` and `POLARALIGN_LIMITS` properties,
this driver exposes:

- `X_MLASTRO_SPEED` - manual jog speed level (1-5, `SLvl:X`).
- `X_MLASTRO_BACKLASH_ENABLE` / `X_MLASTRO_BACKLASH_STEPS` - backlash
  compensation enable and per-axis step count (`Back:X`, `AzBl:X`, `AlBl:X`).
- `X_MLASTRO_STATUS` - the controller's raw state word (READY, MOVING,
  ALIGNING, HOMING, ERROR, ...); ALERT while the controller is in ERROR.
- `X_MLASTRO_HOMED` - lit when a home reference has been set.
- `X_MLASTRO_GOTO_HOME` - returns both axes to the (0,0) reference
  (`RetH:1`).
- `X_MLASTRO_CLEAR_HOME` - forgets the home reference (`RstH:1`); further
  motion is rejected by the controller until a new one is set.
- `X_MLASTRO_RESET_ERROR` - clears a driver error or StallGuard hard-limit
  lock (`ReER:1`), which also stops both motors.

## Device model

`POLARALIGN_OFFSET.ALT`/`.AZ` (arcminutes) track the controller's `AzPH`/
`AlPH` telemetry, which is in degrees relative to the last position marked as
home. (`Mpos` is only the angle moved since the last motion started; it is used
just as a fallback for firmware that does not report `AzPH`/`AlPH`.) Setting a
target issues a relative move for the difference between the target and the
last known position; differences below half an arcsecond are not sent. A
single-axis change uses the
protocol's angle-input + jog commands (`ReDe`/`ReAM`/`ReAS` + `MAzL`/`MAzR`/
`MAlU`/`MAlD`), a simultaneous change on both axes uses the chained
alignment-error command (`AzED`/.../`AAll:1`), mirroring how the INDI driver
speaks the same protocol.

The controller has one shared zero reference for both axes (`SetH:1` marks
the current physical position as (0,0) on both at once); there is no way to
zero altitude or azimuth independently, so `POLARALIGN_RESET_POSITION_ALT`
and `POLARALIGN_RESET_POSITION_AZ` both invoke it and both zero the whole
device. This is a firmware limitation, not a driver shortcoming.

A move ends when the telemetry status leaves MOVING/ALIGNING/HOMING; it ends in
ALERT when the controller reports ERROR (for example a StallGuard hard limit).
Abort sends the soft `STOP:1`, so the axes decelerate after the abort and the
position they come to rest at is still published.

## Serial control

The driver retries the `[MLAstroRPA-TC]` handshake, since opening the port
resets most ESP32 boards and the firmware ignores the handshake while it
boots. On disconnect it sends `Disconnect`, as the NINA plugin does, so the
controller hands control back to its Web UI. When the Web UI takes control
back (the controller pushes `DISCONNECTED`) or the controller reboots, the
driver disconnects with an alert. Unsolicited lines (`*:COMPLETED`,
`HOME_COMPLETED`, `ERROR:...`) are logged and never taken as a command reply.

## Not implemented

The controller also exposes WiFi station/access-point configuration
(SSID/password, IP address) and a "Save & Reboot" command to persist motor
and network configuration to flash, plus per-axis motor tuning (run/hold
current, microstepping, acceleration/deceleration, start boost, CoolStep,
run mode) beyond steps-per-degree. These are configuration/provisioning
concerns rather than part of the polar-alignment control surface and are
left for a future revision; see the protocol document for the full command
set if they are needed sooner.

## Testing

2026-09-24 22:25 3.0.0.3 mac arm64 simulator 23/23 OK
