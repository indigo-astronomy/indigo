# SBIG CCD and Filter Wheel driver

http://diffractionlimited.com/

## Supported devices

All SBIG CCD USB and Ethernet cameras along with connected to them filter wheels.

This driver supports hot-plug (multiple devices).

## Supported platforms

This driver depends on 3rd party library and is supported on Linux (Intel 32/64 bit and ARM v6+) and MacOS.

## License

INDIGO Astronomy open-source license (3rd party library is closed source).

## Use

indigo_server indigo_ccd_sbig

## Status: Stable

Driver is developed and tested with:
SBIG ST-2000XCM
SBIG Camera Simulator

## NOTES
### MacOS SDK distributed separately (MacOS only)

On MacOS the driver requires the SDK provided by the camera manufacturer:
ftp://ftp.sbig.com/pub/SBIGDriverInstallerUniv.dmg

This third party software is not digitally signed by the vendor, therefore it may fail to load the camera
firmware when camera is connected on the recent versions of MacOSX without disabling some security features
of the operating system. If you are affected, please refer to Apple documentation how to enable loading of
unsigned kernel extensions.
