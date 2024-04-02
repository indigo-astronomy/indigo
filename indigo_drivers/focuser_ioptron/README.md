# iOptron iEAF focuser driver

https://www.ioptron.com/product-p/8453.htm

## Supported devices
* iOptron iEAF focuser

Single device is present on the first startup (no hot-plug support). Additional devices can be configured on runtime.

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_focuser_ioptron

## Status: Untested

## Protocol

protocol based on iOptron LX200 dialect syntax

:DeviceInfo# → PPPPPPPMMLLLL

where PPPPPPP is position, MM is model (should be 2) and LLLL is uknown.

Get status

:FI# → PPPPPPPMTTTTTD#

where PPPPPPP is position, M is movement state (0 = stopped, 1 = moving), TTTTT is temperature (temp in celsius = value / 100 - 273.15), D is direction (0 = normal, 1 = reversed).

Reverse direction

:FR# → nothing

Move

:FMPPPPPPP# → nothing

where PPPPPPP is position.

Sync zero position

:FZ# → nothing

Abort movement

:FQ# → nothing
