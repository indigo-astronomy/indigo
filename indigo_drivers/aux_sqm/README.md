# Unihedron SQM sky quality meter driver

http://www.unihedron.com

## Supported devices
* Unihedron SQM device with USB or serial connection

Single device is present on the first startup (no hot-plug support). Additional devices can be configured on runtime.

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_aux_sqm

## Comments

## Status: Stable

Tested with a physical device:
* SQM-LU

## Testing

2026-10-09 18:01 3.0.0.22 mac arm64 simulator 11/11 OK
2026-10-09 18:06 3.0.0.22 linux arm64 Unihedron SQM-LU Arduino simulator (ESP32-S3) 7/7 OK
