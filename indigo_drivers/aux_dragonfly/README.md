# Lunatico Astronomy Dragonfly AUX Relay Controller driver

https://www.lunatico.es

## Supported devices

Dragonfly controllers.

No hot plug support. One Relay Controller GPIO device is present at startup.

## Supported platforms

This driver is supported on Linux (Intel 32/64 bit and ARM v6+) and MacOS.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_aux_dragonfly

## Status: Stable

Driver is developed and tested with:
* Dragonfly controller

## NOTES:

* Requires firmware version 5.29 or newer.

* This driver can not be loaded if *indigo_dome_dragonfly* is loaded.

## Testing

2026-09-27 01:32 3.0.0.8 linux x64 simulator 17/17 OK
2026-09-27 20:48 3.0.0.8 mac arm64 simulator 17/17 OK
