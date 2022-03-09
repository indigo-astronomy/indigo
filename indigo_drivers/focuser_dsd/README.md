# Deep Sky Dad AF1, AF2 and AF3 focuser driver

https://deepskydad.com

## Supported devices

AF1, AF2, AF3 focusers.

Single device is present on the first startup (no hot-plug support). Additional devices can be configured at runtime.

## Supported platforms

This driver is supported on Linux (Intel 32/64 bit and ARM v6+) and MacOS.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_focuser_dsd

## Status: Stable

Driver is developed and tested with:
* DSD AF1

## NOTES:
* AF1 and AF2 use baud rate 9600 bps (default)
* AF3 uses 115200 bps (should be specified)
* DSD_MODEL_HINT property can be used instead, it sets the baud rate according to the selected model.
