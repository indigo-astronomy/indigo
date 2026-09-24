# MLAstro RPA driver review fixes

Status: completed on 2026-09-24. Baseline driver version: `0x03000002`. Fixed driver version: `0x03000003`.

## Sources

- Protocol: `Documentation/Serial-protocol.md` in [MLAstroRPA/MLAstroRPA.NINA.Plugin](https://github.com/MLAstroRPA/MLAstroRPA.NINA.Plugin).
- Behaviour the protocol document leaves open: `Services/SerialConnectionService.cs` of the same NINA plugin.
- Reference implementation: INDI `drivers/auxiliary/mlastro_rpa.cpp`/`.h` (indilib/indi).

## Defects fixed in 3.0.0.3

| Defect | Consequence | Covered by |
| :--- | :--- | :--- |
| Position was read from `Mpos`, which is the angle moved since the last motion started, not the angle from home (`AzPH`/`AlPH`). | Every move after the first was computed from the wrong position and the reported offset was wrong. | `consecutive_moves_track_position_from_home`, `single_axis_move_uses_relative_move`, `reconnect_resumes_polling`, `goto_home_requires_a_reference_and_clear_home_forgets_it` |
| `POLARALIGN_OFFSET` was only published while BUSY. | After abort (soft stop, the axes decelerate) the client kept a stale position. | `abort_decelerates_and_publishes_final_position` |
| Controller `ERROR` status ended a move as OK. | A hard-limit or driver fault looked like a completed move. `X_MLASTRO_RESET_ERROR` (`ReER:1`) added to recover. | `hard_limit_alerts_until_error_is_reset` |
| One line was read as the reply to every command. | An asynchronous `*:COMPLETED`, `ERROR:...` or `DISCONNECTED` push landing between a command and its reply was taken as the reply; the command then failed. | `completion_push_is_not_taken_for_a_reply` |
| `DISCONNECTED` (Web UI took control back) and `REBOOTING...` were ignored. | The driver kept polling a controller that no longer accepted commands. It now goes offline with an alert. | `releasing_serial_control_disconnects` |
| Serial control was not handed back on disconnect. | With the firmware communication watchdog off the Web UI stayed locked out. `Disconnect` is now sent before closing, as the NINA plugin does. | `disconnect_hands_control_back` |
| Single handshake attempt 200 ms after open. | ESP32 boards reset when the port opens and print a boot log first. The handshake is now retried up to 5 times. | `handshake_is_retried_through_boot_noise` |
| Reply lines kept their line terminator. | The serial number in `INFO` ended with a newline. | `identity_and_firmware_reach_info` |
| Exact floating-point comparison of move deltas. | A sub-arcsecond difference sent a zero-length move or turned a single-axis move into `AAll`. | `sub_arcsecond_difference_is_not_sent` |
| Telemetry overwrote properties with a change in flight. | A poll parsed while the client changed a setting could revert it before the handler sent it (seen as `AlRD:0` sent for a REVERSED request). | `direction_and_steps_per_degree_round_trip` (intermittent before) |
| `POLARALIGN_OFFSET` copied values instead of targets. | Clients briefly saw the requested target as the position. | all motion cases |
| `strtok` in the telemetry parser. | Not thread-safe when other drivers run in the same server. | - |

Running the new suite against the 3.0.0.2 driver fails 13 of 23 cases; 3.0.0.3 passes 23/23.

## Simulator model

The simulator was rewritten on `indigo_test/simulator_common/serial_motion.h`. It models, from the protocol document:
handshake before control, one reply per (chained) command line, `AzPH`/`AlPH` versus `Mpos`, `MOVING`/`ALIGNING`/`HOMING` and the `ALIGN_COMPLETED`/`HOME_COMPLETED` states, `AzAN/AlAN/AAll:COMPLETED` pushes, azimuth-before-altitude sequencing of `AAll`, soft `STOP:1` versus immediate `ESTOP:1`, ignored `STOP:0`/`ESTOP:0`, `:0` release of move commands, jog mode with the 500 ms watchdog (`JoRe:0` is the power-on default, so a driver that did not select relative mode would run to the soft limit), `Soft Limit`/`Not homed`/`System Locked`/`Hard Limit` rejections, `ReER:1`, `RstH:1` clearing status and coordinates, chained `Save&Reboot:1` followed by `REBOOTING...`, and `DISCONNECTED`.

Taken from the NINA plugin rather than the document: the `Disconnect` command, the `HOME_COMPLETED` push line and the upper-case `ERROR:<key>:<int>,...` line.

Options: `--legacy-handshake` (bare `ok`), `--boot-noise <n>` (first handshakes answered with ESP32 boot log only), `--defer-push` (pushes land between the next command and its reply), `--hard-limit-az <deg>` (StallGuard trip). Runtime action `disconnect` in `<ready-file>.control` models a Web UI reload. Every command is recorded in `<ready-file>.events`.

## Not verified

No MLAstro RPA hardware was available; all results are simulator results. The following simulator behaviour is an assumption and should be checked on hardware:

- Whether `Mpos` is reset by relative (`ReDe`+`MAz*`) moves as well as by alignment moves. The driver no longer depends on it.
- The keys of the `ERROR:` line (the simulator's `AzHL` is a placeholder; the driver only logs the line).
- What the firmware does with commands after `DISCONNECTED` (the simulator ignores them; the driver stops sending them).
- Whether `RstH:1` zeroes the reported `AzPH`/`AlPH`.
- Which status follows a completed relative move (the simulator reports `READY`; the driver treats every non-moving status alike).
- Whether the status passes through a non-moving state between the azimuth and altitude legs of `AAll`; if it does, the driver would report completion one leg early.

## Hardware run and fixes in 3.0.0.4 (2026-09-24)

Hardware: MLAstro RPA firmware 1.8.1 (`MLAstroRPA-full-1.8.1.bin` from [MLAstroRPA/Firmware-Update](https://github.com/MLAstroRPA/Firmware-Update/) release v1.8.1, SHA-256 `2fa7037fa3558ae766d8b8b3ce079e25915b756d2576f22aefb1f8f162eda763`, merged 4 MB image written at `0x0` with `--chip esp32`) on a TTGO board, ESP32-D0WDQ6 rev 1.0, 4 MB flash, CP2104 USB-UART, MAC `24:6F:28:25:54:BC`, connected to a Mac (arm64). The board has no TMC2209 motor drivers, PSRAM or FRAM, so nothing can move. The image is built for the classic ESP32 (bootloader at `0x1000`) and does not boot on an ESP32-S3. Its repository carries no license and no source, so the binary is not stored in the simulator directory; the README links the firmware repository instead. Hot-plug was not part of the run and is not covered.

Interactive hardware run, `make -C indigo_test test-polaralign-mlastro-hw` (`MLASTRO_HW_PORT=/dev/cu.usbserial-01DA93AF`), against driver 3.0.0.3: 18 cases, 14 passed. The 4 failures (`mlastro_resets_the_controller_error`, `mlastro_reconnects`, `mlastro_survives_repeated_reconnects`, `mlastro_reinitializes`) come from a wrong assumption in the new test, not from the driver: they expect the controller to fall back to ERROR after `ReER:1`, but firmware 1.8.1 stays READY until the next motion command. The test has not been corrected and the fixed driver 3.0.0.4 has not been rerun on hardware yet.

Observed on the hardware (direct serial probes and the run's debug log):

- Handshake reply `ok,firmware 1.8.1,SN:24:6F:28:25:54:BC`, then an `ERROR:Sys:2,AzNC:2,AlNC:2,AzOT:0,...,AzHL:0,AlHL:0,AzSL:0,AlSL:0,Esc:0,CmdRf:0` push. The boot banner (with blank lines) and WiFi log lines arrive unsolicited, before or after the handshake reply.
- Without motor drivers the controller is locked in ERROR; motion is refused with `error: System Locked`. `ReER:1` answers `ok` plus `ERROR:Sys:0,...` and leaves it READY until a motion command, which creeps 0.13 deg, answers `error: Driver Not Responding` and locks again.
- A refused chained command answers the error and then an extra `ok`. A refused single command answers only the error.
- `SetH:1` is followed by a `SetH:COMPLETED` push, `STOP:1` while idle by `SetH:STOPPED`. `RstH:1` clears `Home` but keeps `AzPH`/`AlPH` and `Mpos`.
- Before the handshake every command is answered `error: Not connected. Send [MLAstroRPA-TC] to take control.`, the `?` poll `error: Not connected. System is idle or controlled by Web/PC-Wireless.`. `Disconnect` answers `ok`.
- Communication watchdog (on by default): a gap of 1.12 s between commands keeps control, 1.15 s loses it; `?` counts as a command. On expiry the firmware pushes `error: Serial heartbeat timeout -> ESTOP` and answers every command `error: Not connected`.
- Opening the port through INDIGO does not reset this board; opening it with pyserial does.

| Defect | Consequence | Covered by |
| :--- | :--- | :--- |
| The idle poll ran every 1.063 s (measured), 90 ms inside the 1.15 s communication watchdog of firmware 1.8.1. | Any delay of the handler queue dropped serial control and stopped the motors. The poll now runs every 0.5 s. | `poll_keeps_serial_control_inside_the_watchdog` (watchdog set to 0.8 s) |
| A lost serial control through the watchdog was not detected: the `error: Serial heartbeat timeout` push was logged as an ordinary line and `error: Not connected` replies were taken as command errors. | The driver kept polling a controller that ignored it, and every change ended in ALERT without a reason. Both lines now take the `DISCONNECTED` path: the device goes offline with an alert. | `watchdog_expiry_disconnects_with_alert`, `not_connected_reply_disconnects_with_alert` |

Simulator additions, all from the hardware observations above: `--heartbeat <s>` watchdog (default 1.15 s) with the `heartbeat` and `release` control actions, `Not connected` replies, `ok` to `Disconnect`, the extra `ok` after a refused chained command, `SetH:COMPLETED`/`SetH:STOPPED` pushes, the real `ERROR:Sys:...` line after the handshake, `ReER:1` and a hard limit, `--no-motor-drivers` (locked from start, `Driver Not Responding` with 0.13 deg creep), `RstH:1` keeping the position, and `--log-noise` (banner and WiFi lines). New cases: `controller_without_motor_drivers_stays_locked`, `clear_home_keeps_the_reported_position`, `firmware_log_lines_are_not_taken_for_replies`.

Resolved assumptions from "Not verified": `RstH:1` does not zero `AzPH`/`AlPH`; the `ERROR:` line keys are `Sys, AzNC, AlNC, AzOT, AlOT, AzPW, AlPW, AzSA, AzSB, AlSA, AlSB, AzOL, AlOL, AzHL, AlHL, AzSL, AlSL, Esc, CmdRf`; after losing control the firmware answers `error: Not connected`.

Simulator run with 3.0.0.4: `build/integration/test_polaralign_mlastro_simulator`, 29/29 passed (mac arm64).

## Windows

Visual Studio project and filters added, registered in `indigo_windows.sln` and referenced by `indigo_server.vcxproj`; the driver is also listed in the `indigo_server.c` static driver table. No Windows machine was available, so the Windows build was not executed.

## Test summary

- Simulated: 29 cases run, 29 passed (driver 3.0.0.4).
- Hardware: 18 cases run, 14 passed (driver 3.0.0.3, firmware 1.8.1 on a TTGO ESP32 without motor drivers); the 4 failures are test-assumption errors, see above.
