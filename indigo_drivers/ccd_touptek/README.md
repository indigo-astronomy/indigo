# ToupTek CCD camera, fileter wheel and focuser driver

http://www.touptek.com

## Supported devices

ToupTek USB 2.0 and USB 3.0 devices (cameras, fileter wheels and focusers) supported by the vendor SDK

Untested support for Meade DSI IV, LPI-G and Meade LPI-G Advanced

This driver supports hot-plug (multiple devices).

## Supported platforms

This driver depends on 3rd party library and is supported on Linux (Intel 32/64 bit, ARMHF and ARM64 bit) and MacOS.

## License

INDIGO Astronomy open-source license (3rd party library is closed source).

## Use

indigo_server indigo_ccd_touptek

## Status: Stable

Tested with:
* GP-1200KMB
* GPCMOS02000KMA
* GP3M678C
* GPM462M
* ATR585M
* Touptek FILTERWHEEL
* Touptek AAF

## Comments

* There is a known issue with SDK and reopening the camera - exposure is not possible until the camera is reconnected.
* Filter wheel does not report the number of slots, so it shuld be selected by the user.
