# Raspberry Pi Camera Driver

https://www.raspberrypi.com/documentation/accessories/camera.html

## Supported devices

The driver supports all Official Raspberry Pi Cameras and most third-party CSI based cameras for Raspberry Pi with following caveats --

* Third party camera sensors are not detected by the driver by default. To enable detection user must make changes to the /boot/config.txt file per this [link](https://www.raspberrypi.com/documentation/computers/config_txt.html#what-is-config-txt). This will configure early-stage boot firmware for the installed hardware.

For eaxmple, if the third-party camera with mono sensor is OV9281. Then the following lines must be added to /boot/config.txt --

dtoverlay=ov9281

## Supported platforms

This driver depends on 'A complex camera support library for Linux'. This open source camera stack and framework for Linux is customized to work with INDIGO SKD requirements. The customized driver supports ARMv7 and ARMv8 platforms.

The driver supports following Pi boards --
1. RPi 2, RPi 3 and RPi 4.
2. **RPi 5** is **not** supported as of now.

## License

INDIGO Astronomy open-source license

## Use

indigo_server indigo_ccd_rpi

## Status: Stable

Driver is developed and tested with:
* Official Raspberry Pi High Quality Camera (IMX477R)
* Raspberry Pi Camera Module 3 NoIR (IMX708)
* Official Raspberry Pi Camera V2 (IMX219)
* OV9281-110 1MP Mono Camera for Raspberry Pi
