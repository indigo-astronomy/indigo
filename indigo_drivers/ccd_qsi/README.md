# QSI CCD and Filter Wheel driver

http://www.qsimaging.com

## Supported devices

QSI 500, 600, and RS Series CCD Imaging Cameras (SDK version 7.6).

This driver supports hot-plug (multiple devices, only one camera and filter wheel can be active at the same time).

## Supported platforms

This driver works on Linux (Intel 32/64 bit and ARM v6+) and MacOS.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_ccd_qsi

## Status: Stable

Driver is developed and tested with:
* QSI 683ws

## NOTES:
The driver will show all plugged devices but can be connected to one camera and one filter wheel.
If one needs to switch between cameras or filter wheels the connected one should be disconnected.
This is a limitation of the underlaying QSI SDK, which can use one camera and one filter wheel at the same time.
