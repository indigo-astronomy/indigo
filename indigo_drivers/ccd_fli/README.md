# FLI CCD driver

http://www.flicamera.com/

## Supported devices

All cameras by Finger Lakes Instrumentation.

This driver supports hot-plug (multiple devices).

## Supported platforms

This driver depends on 3rd party library and is supported on Linux (Intel 32/64 bit and ARM v6+) and MacOS.

## License

INDIGO Astronomy open-source license (3rd party library is open source).

## Use

indigo_server indigo_ccd_fli

## Status: Stable

Driver is developed and tested with:
* FLI PL9000

## NOTES
### Up to version 2.0.0.5 this driver requires Kernel Driver (Linux only)
This INDIGO driver up to version 2.0.0.5 requires Linux kernel driver to work. The kernel driver is available for download here:
http://indigo-astronomy.org/downloads.html

After kernel update one may need to execute:

sudo indigo-install-fliusb

In order to install the kernel driver for the new kernel.

### Since version 2.0.0.6 the driver uses libusb.
Since version 2.0.0.6 the kernel driver is not required any more as it uses libusb.
