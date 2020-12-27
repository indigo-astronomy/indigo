# Raspberry Pi GPIO AUX Controller driver

https://www.raspberrypi.org

## Supported devices

GPIO port of the Raspberry Pi SBCs.

No hot plug support. One GPIO device is present at startup.

## Supported platforms

This driver is supported on Linux (ARM v6+).

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_aux_rpio

## Status: Under Development

Driver is developed and tested with:
* Raspberry Pi 3 B
* Raspberry Pi 4 B

## GPIO Pin mapping:

### Inputs
* Input 1 -> GPIO 04
* Input 2 -> GPIO 17
* Input 3 -> GPIO 27
* Input 4 -> GPIO 22
* Input 5 -> GPIO 23
* Input 6 -> GPIO 24
* Input 7 -> GPIO 25
* Input 8 -> GPIO 20

### Outputs
* Output 1 -> GPIO 18
* Output 2 -> GPIO 12
* Output 3 -> GPIO 13
* Output 4 -> GPIO 26
* Output 5 -> GPIO 16
* Output 6 -> GPIO 05
* Output 7 -> GPIO 06
* Output 8 -> GPIO 19

NOTE: As of version 2.0.0.3 pins GPIO 02 and GPIO 03 are not used by the driver, as they are the default I2C pins.
GPIO 19 and GPIO 20 are used instead.
