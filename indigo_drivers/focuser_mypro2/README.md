# myFocuserPro2 focuser driver

https://sourceforge.net/projects/arduinoascomfocuserpro2diy/

## Supported devices

* myFocuserPro2

This driver supports up to 8 devices, no hot plug support. By default the driver exposes one device.
In order to change that one should export environment variable FOCUSER_MFP_DEVICE_NUMBER and set it to the desired number. For example:

export FOCUSER_MFP_DEVICE_NUMBER=3

## Supported platforms

This driver is supported on Linux (Intel 32/64 bit and ARM v6+) and MacOS.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_focuser_mypro2

## Status: Under Development

Driver is developed and tested with:
* myFocuserPro2 on Arduino Micro
