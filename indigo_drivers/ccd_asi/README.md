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

## Notes

### Notes on white balance of the color cameras:
For planetary photography and sort exposures use "neutral" color balance specific to your camera (for example for ASI2600MC and ASI1600MC, WB_R=52, WB_B=95). With long exposures these settings will produce colored background (for ASI2600MC and ASI1600MC the background will be blue). This leads to complications with the calibration of the long exposure frames. Therefore for easier calibration of the deepsky images use WB_R=50 and WB_B=50 ("Camera name"->Advanced), this will result in neutral gray background but the color balance should be corrected in the post processing (for ASI2600MC and ASI1600MC the blue channel should be boosted). Doing this in post processing may result in noisier images. So we recommend to keep the "neutral" setting despite the non neutral background.

### Notes on "Exposure Failed" issue

If you see occasional or frequent "Exposure failed" errors reported, one of the following may be the cause:

1. If you are running more than one ASI camera on the same host lowering USB bandwidth ("Camera name"->Advanced->Bandwidth) to around 40 or 50 for all cameras may help. If you are running one camera, lower the BandWidth to around 80 or lower.

2. Some cooled ASI cameras require the cooler power to be connected for a reliable operation, even if the cooling is not enabled.

3. Often lower grade USB cables can cause unstable camera operation. Try better USB cable. We have never had issues with the cables supplied by ZWO but some users claim that they had to replace them to fix the problem.

4. If you have a second camera connected to the USB port of another camera, please reconnect it directly to the computer. According to ZWO, USB ports of the cameras are intended for focusers, filter wheels etc. but not for cameras.

5. Some USB hubs can cause this problem. Try to replace the USB hub or connect the camera directly to the computer USB port.

6. If you are using a Raspberry Pi or some other SBC, the power adapter may not be powerful enough. Please use more powerful adapter. We recommend at least 5.1V @ 2.5A for RPi3 and 5.1 @ 3A for RPi4.

If none of the above helps, enable ZWO ASI debug log as described [here](https://www.indigo-astronomy.org/download/How%20to%20get%20log%20file%20on%20Linux.pdf). You do not need ASI Studio, you can connect and disconnect the camera from INDIGO. Once the ASI debug log is enabled start INDIGO as usual and when the issue is reproduced please send the log file to indigo@cloudmakers.eu
