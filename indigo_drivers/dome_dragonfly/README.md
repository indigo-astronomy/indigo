# Lunatico Astronomy Dragonfly dome/GPIO driver

https://www.lunatico.es

## Supported devices

Dragonfly controllers.

No hot plug support. One Dome and one GPIO are present at startup.

## Supported platforms

This driver is supported on Linux (Intel 32/64 bit and ARM v6+) and MacOS.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_dome_dragonfly

## Status: Under Development

Driver is developed and tested with:
* Dragonfly controller

## NOTES:
* The Dome wiring should be as described in Dragonfly documentation.

* This driver can not be loaded if *indigo_aux_dragonfly* is loaded as both drivers have the same functionality.

* In devices with male DB9 connectors, pins are horizontally flipped. In software pin 1 corresponds to pin 5 on male DB9, pin 6 corresponds to pin 9 etc. There is no such flipping for the devices with female DB9 connectors.
