# Moravian Instruments CCD/CMOS and filter wheel driver

https://www.gxccd.com

## Supported devices

This device supports all USB devices (TBD Ethernet devices).

This driver supports hot-plug (multiple devices).

## Supported platforms

This driver depends on 3rd party library and is supported on Linux (Intel 32/64 bit and ARM v7/8) and MacOS (Intel 64 bit and ARM64).

## License

INDIGO Astronomy open-source license (3rd party library is closed source, Copyright 2022, Moravian Instruments Inc. All rights reserved).

## Use

indigo_server indigo_ccd_mi

## Status: Stable

Driver is developed and tested with:
* MI G0-300
* MI G3-16200 with internal filter wheel
* MI C1-1500
* MI C1-3000A
* MI C2-12000
* MI C1+12000A
* MI C1×26000
* MI C3-61000
* MI C4-16000
* MI C5A-100M

## Testing

2026-09-27 08:00 3.0.0.39 linux x64 fake SDK 18/18 OK
2026-09-27 21:39 3.0.0.39 mac arm64 fake SDK 18/18 OK
2026-09-30 18:21 3.0.0.39 linux arm64 MI G0-0300 1/1 OK
2026-10-02 20:56 3.0.0.39 mac arm64 MI G0-0300 1/1 OK
