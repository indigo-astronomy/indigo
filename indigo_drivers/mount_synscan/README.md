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

## Testing

2026-09-24 09:35 3.0.0.7 mac arm64 MountSim 2.3 (EQMOD) 15/15 OK
2026-09-24 21:27 3.0.0.8 linux arm64 AstroEQ 8.25 (ESP32-S3) 16/16 OK
2026-09-24 21:48 3.0.0.9 mac arm64 AstroEQ 8.25 (ESP32-S3) 16/16 OK
2026-09-26 22:41 3.0.0.13 linux x64 simulator 24/24 OK
2026-10-04 11:05 3.0.0.15 mac arm64 AZGTi 18/18 OK
2026-10-04 11:20 3.0.0.15 linux arm64 simulator 29/29 OK
2026-10-04 19:40 3.0.0.17 mac arm64 simulator 46/46 OK
