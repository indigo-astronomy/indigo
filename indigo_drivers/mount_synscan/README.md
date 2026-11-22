# SynScan mount driver

## Supported devices

Any SynScan protocol compatible mount (SkyWatcher; Celestron; Orion; ...) connected over serial port or network.

Single device is present on the first startup (no hot-plug support). Additional devices can be configured on runtime.

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_mount_synscan

## Status: Stable

Driver is developed and tested with:
* SkyWatcher NEQ6 Pro
* SkyWatcher EQAZ6
* SkyWatcher EQ8
* SkyWatcher AZ-GTi

## Comments

Use URL in form synscan://host:port to connect to the mount over UDP (default port is 11880). Use just synscan:// for UDP autodetection.

X_MOUNT_USE_ENCODERS switches the dual (axis) encoders of the motor controller on or off; the coordinates always come from the motor step counters. When an encoder is switched on, the controller corrects the step counter of that axis from the encoder (an AZ-GTi does it at once and only to the encoder's coarse grid of about 0.34°), so sync or plate-solve again after switching it on.

## Testing

- 2026-09-24 09:35 3.0.0.7 mac arm64 MountSim 2.3 (EQMOD) 15/15 OK
- 2026-09-24 21:48 3.0.0.9 mac arm64 AstroEQ 8.25 (ESP32-S3) 16/16 OK
- 2026-09-26 22:41 3.0.0.13 linux x64 simulator 24/24 OK
- 2026-10-04 11:20 3.0.0.15 linux arm64 simulator 29/29 OK
- 2026-10-07 03:01 3.0.0.18 linux arm64 AstroEQ 8.25 (ESP32-S3) 19/19 OK
- 2026-10-07 07:05 3.0.0.18 linux arm64 AZGTi 19/19 OK
- 2026-11-22 16:33 3.0.0.32 mac arm64 AZGTi 21/21 OK
- 2026-11-22 21:18 3.0.0.34 mac arm64 simulator 144/144 OK
