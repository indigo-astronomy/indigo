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
This driver is not built by the main indigo Makefile.
one should install Andor SDK and then do:

cd indigo_optionl_drivers/ccd_andor

make

## Use
In case Andor SDK is not installed on the standard location, ANDOR_SDK_PATH
environment variable must be set to the correct path before starting
indigo_server. Default ANDOR_SDK_PATH is: "/usr/local/etc/andor".

indigo_server indigo_ccd_andor

## Status: Under development

Driver is developed and tested with:
* Andor iKon-L
* Andor Newton

## NOTES:
Due to Andor SDK limitation it is not possible to take simultaneous exposures with several cameras.
Only one camera can take exposure at a given time.
