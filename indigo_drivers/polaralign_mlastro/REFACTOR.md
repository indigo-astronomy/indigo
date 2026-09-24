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

## Windows

Visual Studio project and filters added, registered in `indigo_windows.sln` and referenced by `indigo_server.vcxproj`; the driver is also listed in the `indigo_server.c` static driver table. No Windows machine was available, so the Windows build was not executed.
