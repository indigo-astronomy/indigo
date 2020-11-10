# Lunatico Astronomy Dragonfly Dome / Relay Controller GPIO driver

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

## Status: Stable

Driver is developed and tested with:
* Dragonfly controller

## Hardware Configuration

The driver supports three shutter/roof wiring configurations:

* One button - push & release operation:
	* *Relay \#1* - Open / Close / Stop


* Two buttons - push & hold operation:
	* *Relay \#2* - Open (push Open, release Stop)
	* *Relay \#3* - Close (push Close, release Stop)


* Three buttons - push & release operation:
	* *Relay \#1* - Stop
	* *Relay \#2* - Open
	* *Relay \#3* - Close


Sensor configuration:
 * *Sensor \#1* - On when dome is Open
 * *Sensor \#2* - On when dome is Closed
 * *Sensor \#8* - Mount is parked when above the threshold.

*Sensor \#3 - \#7* and *Relay \#4 - \#8* are available for general purpose use and are exposed by the Relay Control GPIO device.

*IN* and *COM* pins of the *Power inputs \#1 - \#3* should be short circuited in order to simulate the push of the button when *Relays \#1 - \#3* are switched on.

## NOTES:
* **Do not control the slit / roof with the physical buttons when the driver is connected. This will make the driver behave strangely, as there is no feedback from the roof controller whether the roof is moving or is stopped.**

* The dome wiring should be similar to the described in Dragonfly documentation.

* Requires firmware version 5.29 or newer.

* *Relays \#1 - \#3* and *Sensors \#1, \#2* and *\#8* are reserved for dome control and are not exposed in the Relay Control GPIO device.

* This driver can not be loaded if *indigo_aux_dragonfly* driver is loaded.
