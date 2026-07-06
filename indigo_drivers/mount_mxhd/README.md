# MX-HD mount driver

## Supported devices

* MX-HD equatorial mount

Connection over serial port. USB serial connections typically appear as `/dev/ttyUSB0` or `/dev/ttyACM0`.
Bluetooth serial connections are supported when the operating system exposes the mount as an RFCOMM serial device such as `/dev/rfcomm0`.

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_mount_mxhd

Connect `MX-HD Mount` first. The `MX-HD Mount (guider)` device reuses the already-open mount serial connection.

## Status: Tested

Tested on Raspberry Pi 5 with real MX-HD hardware:

* serial connection
* RA/Dec polling
* park, unpark and home
* tracking on/off
* manual NSEW motion
* goto
* abort
* guider-device connection
* pulse guide command/state flow
