# QHY CCD driver

http://www.qhyccd.com

## Supported devices

All QHY cameras.

This driver supports hot-plug (multiple devices).
NOTE: Due to instability of the vendor provided SDK problems on all platforms should be expected.

## Supported platforms

This driver depends on 3rd party library and is supported on Linux (Intel 32/64 bit and ARM v6+) and MacOS.

NOTE (MacOS only): on MacOS the firmware of the cameras is expected to be in "/usr/local/lib/qhy/firmware". If it is on other
location INDIGO_FIRMWARE_BASE environment variable should be set to point to that location (without "firmware").
For example: if the firmware location is "/usr/local/firmware/" one should set INDIGO_FIRMWARE_BASE="/usr/local/"

## License

INDIGO Astronomy open-source license (3rd party library is closed source).

## Use

indigo_server indigo_ccd_qhy
