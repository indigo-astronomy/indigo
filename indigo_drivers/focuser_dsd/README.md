# Deep Sky Dad AF1/AF2 focuser driver

https://deepskydad.com

## Supported devices

AF1, AF2, AF3 focusers.

This driver supports up to 8 devices, no hot plug support. By default the driver exposes one device.
In order to change that one should export environment variable FOCUSER_DSD_DEVICE_NUMBER and set it to the desired number. For example:

export FOCUSER_DSD_DEVICE_NUMBER=3

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
