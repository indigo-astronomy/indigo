# ANDOR CCD driver
http://www.andor.com

## Supported devices
ANDOR CCD Cameras

This driver supports up to 8 cameras but does not support hot-plug due to Andor SDK limitation.
All devices should be connected on driver startup.

## Supported platforms

This driver is Linux/Intel (32-bit and 64-bit) specific. The user must provide his own copy of Andor SDK.
Andor SDK is not free. Please see http://www.andor.com/scientific-software/software-development-kit

## License

INDIGO Astronomy open-source license.

## Build
This driver is optional and not built by the main indigo Makefile.
one should install Andor SDK and then in the indigo source directory do:

make

cd indigo_optional_drivers/ccd_andor

make


Tested to compile with:
* Andor SDK v.2.95.30003.0
* Andor SDK v.2.102.30001.0

## Use
In case Andor SDK is not installed on the standard location, ANDOR_SDK_PATH
environment variable must be set to the correct path before starting
indigo_server. Default ANDOR_SDK_PATH is: "/usr/local/etc/andor".

indigo_server indigo_ccd_andor

## Status: Stable

Driver is developed and tested with:
* Andor iKon-L
* Andor Newton

## NOTES:
Due to Andor SDK limitation it is not possible to take simultaneous exposures with several cameras.
Only one camera can take exposure at a given time.
