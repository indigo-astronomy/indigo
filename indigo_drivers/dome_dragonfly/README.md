# Lunatico Astronomy Dragonfly Dome / Relay Control GPIO driver

https://www.lunatico.es

## Supported devices

Dragonfly controllers.

No hot plug support. One Dome and one Relay Control GPIO devices are present at startup.

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

* Relays 1-3 and Sensors 1,2 and 8 are reserved for dome control and are not exposed in the GPIO device.

* This driver can not be loaded if *indigo_aux_dragonfly* is loaded.
