# Gphoto2 CCD driver

https://github.com/gphoto/libgphoto2

## Supported devices

Cameras supported by libgphoto2

## Supported platforms

This driver depends on 3rd party library and is supported on Linux (Intel 32/64 bit and ARM v6/7/8).

## License

INDIGO Astronomy open-source license (libgphoto2 is released under LGPL-2.1).

## Use

indigo_server indigo_ccd_gphoto2

## Functionality and issues

### Camera detection and support
The driver indigo_ccd_gphoto2 uses the camera detection method from [libgphoto2](https://github.com/gphoto/libgphoto2).
If the camera is not detected, then the used libgphoto2 version is perhaps outdated or
the camera is currently not supported by libgphoto2.
This can be verified with [gphoto2](https://github.com/gphoto/gphoto2) and the following commands:
```
>gphoto2 --version
```
and
```
>gphoto2 --auto-detect
```
Please visit the website [libgphoto2](https://github.com/gphoto/libgphoto2) and check for the
up-to-date version or in case the camera is not supported by the latest version, please
file an [issue](https://github.com/gphoto/libgphoto2/issues).

### Manual mode and focus
Make sure the camera is in "M" mode, such that both aperture and shutterspeed can be controlled by the driver.
In addition make sure *manual* focus is enabled and *not* auto focus. The indigo_ccd_gphoto2 driver is
developed for the domain of amateur astronomy, therefore any *auto* settings limit the functionality of the driver.

### Shutterspeed and bulb setting
Most cameras are operated either in shutterspeed setting or bulb setting.
In shutterspeed setting, typical predefined exposure value are: 1/4000, 1/3200, 1/2500, ..., 1/5, 1/4, 0.3, 0.4, ..., 20, 25, 30 seconds.
If the camera is in shutterspeed setting, then an exposure value closest to the predefined values is selected. For instance,
if an exposure value of 24 seconds is chosen, then the predefined value of 25 seconds is selected by the
indigo_ccd_gphoto2 driver. In bulb setting, any non-negative exposure value can be chosen,
for instance exactly 24 seconds or 180 seconds. If highly precise exposure values
below 1 seconds are required it is advised to operate in the shutterspeed setting, otherwise in bulb setting.
Note, if exposure time is set to 0 seconds and camera is in non-bulb, then current shutterspeed value is used. For example, the
current shutterspeed value is set to 5 seconds and the exposure time is set to 0 seconds. The camera exposures for 5 seconds
although an exposure time of 0 seconds was specified. In other words, triggering an exposure of 0 seconds actually,
triggers an exposure of current shutterspeed value.

### Mirror lockup
Fast-flipping mirror can cause vibrations of the camera and the mount.
Mirror lockup makes the mirror flip up for a couple of seconds before the shutter is activated to avoid this problem.
Currently mirror lockup works only for EOS cameras which support the "Custom Functions Ex" functionality, namely
the command to enable mirror lockup: "20,1,3,14,1,60f,1,1", and the command to disable mirror lockup: "20,1,3,14,1,60f,1,0".
For testing whether your camera supports this functionality, connect your camera
in INDIGO with the indigo_ccd_gphoto2 driver and try to enable enable mirror lockup. If the mirror lockup light turns green,
then mirror lockup functionality works for your camera.
Alternatively, test the following commands with gphoto2 for enabling mirror lockup:
```
>gphoto2 --set-config customfuncex=20,1,3,14,1,60f,1,1
```
and for disabling mirror lockup:
```
>gphoto2 --set-config customfuncex=20,1,3,14,1,60f,1,0
```

### FITS format and debayering
FITS conversion and debayering is performed with library [LibRaw](https://github.com/LibRaw/LibRaw).
The debayering options are currently fixed and set as follows:
* Disable LibRaw's default histogram transformation.
* Disable LibRaw's default gamma curve transformation.
* Disable automatic white balance obtained after averaging over the entire image.
* Disable white balance from the camera (if possible).
* No embedded color profile application.

For debayering one can choose between algorithms:
* Linear interpolation,
* VNG,
* PPG,
* AHD,
* DCB,
* DHT.

The default debayering algorithm is VNG which quite computationally
intensive however very good. Linear interpolation is a very basic interpolation but it is much faster compared to VNG.
This FITS output is currently 3 colors (RGB) each 16-bit.

### Image format FITS/RAW/JPEG
Setting the INDIGO image format to FITS, RAW or JPEG requires a corresponding format on the DSLR camera (also called compression format).
Typical compression formats for Canon EOS are:
* Large Fine JPEG
* Large Normal JPEG
* Medium Fine JPEG
* Medium Normal JPEG
* Small Fine JPEG
* Small Normal JPEG
* Smaller JPEG
* Tiny JPEG
* RAW + Large Fine JPEG
* RAW

or for Nikon:
* JPEG Basic
* JPEG Normal
* JPEG Fine
* NEF (Raw)
* NEF+Basic
* NEF+Normal
* NEF+Fine

Setting INDIGO image format to FITS or RAW sets the compression format
on the DSLR camera to 'RAW' for Canon EOS and 'NEF (Raw)' or 'NEF+Basic' for Nikon cameras.
Setting INDIGO image format to JPEG sets the compression format to 'Large Fine JPEG', 'JPEG Fine' respectively.

## Status: Stable

Driver is developed and tested with:
* Nikon D50 (USB)
* Nikon D7000 (USB)
* Canon EOS 600D (USB)
* Canon EOS 700D (USB)
* Canon EOS 1100D (USB)
