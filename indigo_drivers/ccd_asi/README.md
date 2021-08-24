# ZWO ASI CCD driver

https://astronomy-imaging-camera.com

## Supported devices

All ASI cameras with exception of non-S ASI120 models (USB 2.0).

This driver supports hot-plug (multiple devices).

## Supported platforms

This driver depends on 3rd party library and is supported on Linux (Intel 32/64 bit and ARM v6+) and MacOS.

## License

INDIGO Astronomy open-source license (3rd party library is closed source).

## Use

indigo_server indigo_ccd_asi

## Status: Stable

Driver is developed and tested with:
* ASI224MC
* ASI120MM
* ASI120MC-S
* ASI1600MC-Cool
* ASI071MC-Cool
* ASI071MC Pro
* ASI2600MM Pro
* ASI174 Mini
* ASI290 Mini

## Notes on white balance of the color cameras:
For planetary photography and sort exposures use "neutral" color balance specific to your camera (for example for ASI2600MC and ASI1600MC, WB_R=52, WB_B=95). With long exposures these settings will produce colored background (for ASI2600MC and ASI1600MC the background will be blue). This leads to complications with the calibration of the long exposure frames. Therefore for easier calibration of the deepsky images use WB_R=50 and WB_B=50, this will result in neutral gray background but the color balance should be corrected in the post processing (for ASI2600MC and ASI1600MC the blue channel should be boosted).
