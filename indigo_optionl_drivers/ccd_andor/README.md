# ANDOR CCD driver

## Supported devices
ANDOR USB CCD Cameras

All devices should be connected on startup.

## Supported platforms

This driver is Linux/Intel specific.
The user must provide his own copy of Andor SDK.
Andor SDK is not free. Please see http://www.andor.com

## License

INDIGO Astronomy open-source license.

## build
This driver is not built by the main indigo Makefile.
one should do:

cd indigo_optionl_drivers/ccd_andor
make

## Use

indigo_server indigo_ccd_andor

## Status: Under development
