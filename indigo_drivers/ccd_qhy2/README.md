# QHY CCD and Filter Wheel driver (new SDK)

http://www.qhyccd.com

## Supported devices

All QHY cameras and filter wheels plugged in to the camera.

Hot-plug is disabled because of SDK lifecycle instability. Connect cameras before driver initialization; discovery runs once at startup. Restart the driver to detect newly attached cameras. SDK failures may require a USB power reset. Both SDK variants share the generated implementation in `../ccd_qhy/indigo_ccd_qhy.driver`; `cpp = true` generates C++ directly and the `QHY2` build define selects the appropriate SDK.

## Supported platforms

This driver depends on a 3rd party library and is supported on Linux and macOS. The bundled SDK 26.06.04 supports Linux x64, ARM32 and ARM64, and requires macOS 10.14 or later on Intel, or macOS 14.0 or later on Apple Silicon. Linux x86 retains the older SDK because no updated package was supplied.

## License

INDIGO Astronomy open-source license (3rd party library is closed source).

## Use

indigo_server indigo_ccd_qhy2

## Status: Unstable

Some cameras, including QHY5III178, may cause the driver to crash when disconnected in the application. This problem also occurs in earlier INDIGO versions. If the camera stops responding, unplug it from USB for a few seconds, reconnect it and restart INDIGO.

Driver is developed and tested with:
* QHY 5
* QHY 5L-II
* QHY 5III 178

## NOTES

THIS DRIVER IS MUTUALLY EXCLUSIVE WITH indigo_ccd_qhy!!!

### Firmware path (macOS)

The test cameras require firmware initialization. Set `INDIGO_FIRMWARE_BASE` to the directory containing the firmware files themselves. With a source checkout, use `indigo_drivers/ccd_qhy/bin_externals/qhyccd/firmware` (an absolute path when launching from another directory). The bundled modern SDK looks directly for files such as `QHY5III178.img` in that directory. Firmware is shared with the legacy driver package.

### Underlaying SDK is not stable
Due to instability in the vendor provided SDK problems on all platforms should be expected. Therefore it is
advised to use this driver with -i option like this:

indigo_server -i indigo_ccd_qhy2

This will execute the driver in a separate process and in case of a driver crash the server will not be affected.
This will come at the cost of somewhat reduced performance.

