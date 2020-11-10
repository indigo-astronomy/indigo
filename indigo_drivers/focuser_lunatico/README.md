# Lunatico Astronomy Limpet/Armadillo/Platypus Focuser/Rotator/Powerbox/GPIO driver

https://www.lunatico.es

## Supported devices

Seletek, Armadillo, Platypus and Limpet controllers.

No hot plug support. All devices are present at startup:

* **Limpet** - One Focuser is present.
* **Seletek** and **Armadillo** - One Focuser is present on "Main" port and "Exp" can be configured as Rotator, Focuser or Powerbox/GPIO.
* **Platypus** - One Focuser is present on "Main" port, while "Exp" and "Third" can be configured as Rotator, Focuser or Powerbox/GPIO.

## Supported platforms

This driver is supported on Linux (Intel 32/64 bit and ARM v6+) and MacOS.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_focuser_lunatico

## Status: Stable

Driver is developed and tested with:
* Armadillo controller

## NOTES:

* This driver can not be loaded if *indigo_rotator_lunatico* is loaded as both drivers have the same functionality. The only difference is that "Main" port of *indigo_rotator_lunatico* driver is configured as Rotator.

* In devices with male DB9 connectors, pins are horizontally flipped. In software pin 1 corresponds to pin 5 on male DB9, pin 6 corresponds to pin 9 etc. There is no such flipping for the devices with female DB9 connectors.
