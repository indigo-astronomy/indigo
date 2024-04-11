# QHY Q-Focuser focuser driver

https://www.qhyccd.com/q-focuser/

## Supported devices

* QHY Q-Focuser

Single device is present on the first startup (no hot-plug support). Additional devices can be configured at runtime.

## Supported platforms

This driver is supported on Linux (Intel 32/64 bit and ARM v6+) and MacOS.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_focuser_qhy

## Status: Stable

Driver is developed and tested with:
* QHY Q-Focuser standard

Known issues: In very rare occasions, because of a firmware issue, the focuser may not halt at the desired position. This is not a driver bug and is not something we can fix from our end.
