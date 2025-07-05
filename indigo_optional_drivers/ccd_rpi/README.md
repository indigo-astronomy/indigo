# Raspberry Pi Camera Driver

https://www.raspberrypi.com/documentation/accessories/camera.html

## Supported devices

The driver supports all Official Raspberry Pi Cameras and most third-party CSI based cameras for Raspberry Pi with following caveats --

* Third party camera sensors are not detected by the driver by default. To enable detection user must make changes to the /boot/config.txt file per this [link](https://www.raspberrypi.com/documentation/computers/config_txt.html#what-is-config-txt). This will configure early-stage boot firmware for the installed hardware.

For example, if the third-party camera with mono sensor is OV9281. Then the following lines must be added to /boot/config.txt (/boot/firmware/config.txt if on Bookworm) --

dtoverlay=ov9281

Additionally, CSI port number where the camera is attched must be specified. For example, if using multiple cameras on Raspberry Pi 5 --

dtoverlay=ov9281,cam0

## Supported platforms

The driver is designed to be built on ARMv7 and ARMv8 Linux platforms.

The driver was tested on following Pi boards --
1. RPi 4
2. RPi 5

## License

INDIGO Astronomy open-source license

## Build
This driver is optional and not built by the main indigo Makefile.
one should install libcamera and then in the indigo source directory do:

make

cd indigo_optional_drivers/ccd_rpi

make
make install

## How to Build and Install Libcamera
The libcamera library must be built and installed on the system before building indigo_ccd_rpi. The instructions to build and install libcamera can be found [here](https://github.com/raspberrypi/libcamera).

To enable Raspberry Pi 5 support **rpi/pisp** handler must be enabled. This can be done using meson build flags as follows --

    * meson setup build --buildtype=release -Dpipelines=rpi/vc4,rpi/pisp -Dipas=rpi/vc4,rpi/pisp -Dv4l2=true -Dgstreamer=disabled -Dtest=false -Dlc-compliance=disabled -Dcam=disabled -Dqcam=disabled -Ddocumentation=disabled -Dpycamera=disabled

## Use

indigo_server indigo_ccd_rpi

## Status: Stable

Driver is developed and tested with:
* Official Raspberry Pi High Quality Camera (IMX477R)
* Raspberry Pi Camera Module 3 NoIR (IMX708)
* Official Raspberry Pi Camera V2 (IMX219)
* OV9281-110 1MP Mono Camera for Raspberry Pi

## Notes
The driver is tested as working on 64-bit OS. There are known issues with some old 32-bit OS releases. Therefore, it is recommended to use latest 64-bit OS versions.