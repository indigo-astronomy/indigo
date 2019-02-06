# SBIG CCD, Filter Wheel and Adaptive Optics driver

http://diffractionlimited.com/

## Supported devices

All SBIG CCD USB and Ethernet cameras along with connected to them filter wheels and adaptive optics.

This driver supports hot-plug (multiple devices).

## Supported platforms

This driver depends on 3rd party library and is supported on Linux (Intel 32/64 bit and ARM v6+) and MacOS.

## License

INDIGO Astronomy open-source license (3rd party library is closed source).

## Use

indigo_server indigo_ccd_sbig

## Status: Stable

Driver is developed and tested with:
* SBIG ST-2000XCM
* SBIG ST-7XE
* SBIG CFW-8A
* SBIG AO-7
* SBIG Camera Simulator

## NOTES
### Legacy Filter wheels

CFW8 and CFW6A are legacy filter wheels and can not be auto detected. In order to use
them one should set environment variable SBIG_LEGACY_CFW to CFW8 or CFW6A like:

export SBIG_LEGACY_CFW=CFW8

This will expose the device even it is not really present.

### AO-7

AO-8 and later are auto detected, but AO-7 cannot be auto detected. In order to use it 
one should set environment variable SBIG_LEGACY_AO to AO7 like:

export SBIG_LEGACY_AO=AO7

This will expose the device even if it is not really present.

### MacOS SDK distributed separately (MacOS only)

On MacOS the driver requires the SDK provided by the camera manufacturer:
ftp://ftp.sbig.com/pub/SBIGDriverInstallerUniv.dmg

This third party software is not digitally signed by the vendor, therefore it may fail to load the camera
firmware when camera is connected on the recent versions of MacOSX without disabling some security features
of the operating system. If you are affected, please refer to Apple documentation how to enable loading of
unsigned kernel extensions.
