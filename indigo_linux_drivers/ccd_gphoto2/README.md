# Gphoto2 CCD driver

https://github.com/gphoto/libgphoto2

## Supported devices

Cameras supported by libgphoto2

## Supported platforms

This driver is Linux/Intel (32-bit and 64-bit) specific.

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

### Shutterspeed and bulb mode
Most cameras are operated either in shutterspeed mode or bulb mode.
In shutterspeed mode, typical predefined exposure value are: 1/4000, 1/3200, 1/2500, ..., 1/5, 1/4, 0.3, 0.4, ..., 20, 25, 30 seconds.
If the camera is in shutterspeed mode, then an exposure value closest to the predefined values is selected. For instance,
if an exposure value of 24 seconds is chosen, then the predefined value of 25 seconds is selected by the
indigo_ccd_gphoto2 driver. In bulb mode, any non-negative exposure value can be chosen,
for instance exactly 24 seconds or 180 seconds. If highly precise exposure values
below 1 seconds are required it is advised to operate in the shutterspeed mode, otherwise in bulb mode.

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
* Linear interpolation.
* Disabled LibRaw's default histogram transformation.
* Disables LibRaw's default gamma curve transformation.
* Disabled automatic white balance obtained after averaging over the entire image.
* Disabled white balance from the camera (if possible).
* No embedded color profile application.

This FITS output is currently 3 colors (RGB) each 8-bit.

## Status: Development

Driver is developed and tested with:
* Nikon D50 (USB)
* Nikon D7000 (USB)
* Canon EOS 700D (USB)
* Canon EOS 1100D (USB)
