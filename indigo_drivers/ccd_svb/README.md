# SVBONY CCD driver

https://www.svbony.com

## Supported devices

SV-305 and newer.

This driver supports hot-plug (multiple devices).

## Supported platforms

This driver depends on 3rd party library and is supported on Linux (Intel 32/64 bit and ARM v6+) and MacOS (Intel only).

## License

INDIGO Astronomy open-source license (3rd party library is closed source).

## Use

indigo_server indigo_ccd_svb

## Status: Stable

Driver is developed and tested with:
* SV305Pro
* SV405CC

## Testing

2026-09-27 09:06 3.0.0.33 linux x64 fake SDK 48/48 OK
2026-09-30 18:34 3.0.0.33 linux arm64 SVBONY SV305PRO 1/1 OK
2026-10-01 23:07 3.0.0.34 linux x64 SVBONY SV405CC 1/1 OK
2026-10-02 20:43 3.0.0.34 mac arm64 fake SDK 48/48 OK
2026-10-02 20:44 3.0.0.34 mac arm64 SVBONY SV305PRO 1/1 OK
