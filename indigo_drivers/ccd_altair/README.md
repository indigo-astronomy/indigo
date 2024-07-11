# AltairAstro CCD camera, filter wheel and focuser driver

http://www.altairastro.com

## Supported devices

AltairAstro USB 2.0 and USB 3.0 devices (cameras, filter wheels and focusers) supported by the vendor SDK

This driver supports hot-plug (multiple devices).

## Supported platforms

This driver depends on 3rd party library and is supported on Linux (Intel 32/64 bit, ARMHF and ARM64 bit) and MacOS.

## License

INDIGO Astronomy open-source license (3rd party library is closed source).

## Use

indigo_server indigo_ccd_altair

## Status: Stable

## Driver is developed and tested with:

* Altair GP 224C
* Altair GP 290C
* Altair 183M
* Altair 183C

## Comments

There is a known issue with SDK and reopening the camera - exposure is not possible until the camera is reconnected.
