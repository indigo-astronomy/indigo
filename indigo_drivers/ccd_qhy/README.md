# QHY CCD and Filter Wheel driver

http://www.qhyccd.com

## Supported devices

All QHY cameras and filter wheels plugged in to the camera.

Hot-plug is disabled because of SDK lifecycle instability. Connect cameras before driver initialization; discovery runs once at startup. Restart the driver to detect newly attached cameras. SDK failures may require a USB power reset.

## Supported platforms

This driver depends on 3rd party library and is supported on Linux (Intel 32/64 bit and ARM v6+) and MacOS.

## License

INDIGO Astronomy open-source license (3rd party library is closed source).

## Use

indigo_server indigo_ccd_qhy

## Status: Unstable

Mode and bit-depth changes now reuse the current camera connection. Hardware validation of this workaround is pending.

Some cameras, including QHY5III178, may cause the driver to crash when disconnected in the application. If the camera stops responding, unplug it from USB for a few seconds, reconnect it and restart INDIGO.

Driver is developed and tested with:
* QHY 5
* QHY 5L-II
* QHY 6
* QHY 8 Pro
* QHY 5III 178
* QHY 9M & CFW

## NOTES

THIS DRIVER IS MUTUALLY EXCLUSIVE WITH indigo_ccd_qhy2!!!

### Firmware path (MacOS only)
On MacOS the firmware of the cameras is expected to be in "/usr/local/lib/qhy/firmware". If it is on other
location INDIGO_FIRMWARE_BASE environment variable should be set to point to that location (without "firmware").
For example: if the firmware location is "/usr/local/firmware/" one should set INDIGO_FIRMWARE_BASE="/usr/local/"

### Underlaying SDK is not stable
Due to instability in the vendor provided SDK problems on all platforms should be expected. Therefore it is
advised to use this driver with -i option like this:

indigo_server -i indigo_ccd_qhy

This will execute the driver in a separate process and in case of a driver crash the server will not be affected.
This will come at the cost of substantially reduced performance.
