# QHY CCD and Filter Wheel driver (new SDK)

http://www.qhyccd.com

## Supported devices

All QHY cameras and filter wheels plugged in to the camera.

This driver doesn't supports hot-plug. All devices should be connected at driver initialisation.

## Supported platforms

This driver depends on 3rd party library and is supported on Linux (Intel 32/64 bit and ARM v6+) and MacOS.

## License

INDIGO Astronomy open-source license (3rd party library is closed source).

## Use

indigo_server indigo_ccd_qhy2

## Status: Unstable

Driver is developed and tested with:
* QHY 5
* QHY 5L-II
* QHY 5III 178

## NOTES

THIS DRIVER IS MUTUALLY EXCLUSIVE WITH indigo_ccd_qhy!!!

### Underlaying SDK is not stable
Due to instability in the vendor provided SDK problems on all platforms should be expected. Therefore it is
advised to use this driver with -i option like this:

indigo_server -i indigo_ccd_qhy2

This will execute the driver in a separate process and in case of a driver crash the server will not be affected.
This will come at the cost of somewhat reduced performance.

