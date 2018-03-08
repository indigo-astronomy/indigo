# ANDOR CCD driver
http://www.andor.com

## Supported devices
ANDOR USB CCD Cameras

This driver does not support hot-plug due to Andor SDK limitation.
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

indigo_server indigo_ccd_andor

## Status: Under development
