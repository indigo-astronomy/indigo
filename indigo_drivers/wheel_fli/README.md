# FLI filter wheel driver

http://www.flicamera.com/

## Supported devices

All filter wheels by Finger Lakes Instrumentation.

This driver supports hot-plug (multiple devices).

## Supported platforms

This driver depends on 3rd party library and is supported on Linux (Intel 32/64 bit and ARM v6+) and MacOS.

## License

INDIGO Astronomy open-source license (3rd party library is open source).

## Use

indigo_server indigo_wheel_fli

## NOTES
### Requires Kernel Driver (Linux only)
This INDIGO driver requires Linux kernel driver to work. The kernel driver is available for download here:
http://indigo-astronomy.org/downloads.html

After kernel update one may need to execute:

sudo indigo-install-fliusb

In order to install the kernel driver for the new kernel.
