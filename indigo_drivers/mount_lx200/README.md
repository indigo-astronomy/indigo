# LX200 mount driver

## Supported devices

* Meade mounts
* Avalon mounts with StarGo controller
* Losmandy Gemini mounts
* 10micron mounts
* EQMac

Connection over serial port or network.

Single device is present on startup (no hot-plug support).

## Supported platforms

This driver is platform independent

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_mount_lx200

## Comments

Use URL in form lx200://host:port to connect to the mount over network (default port is 4030).

A non-standard switch property "Alignment mode" is provided by this driver.

## Status: Stable

Driver is developed and tested with:
* Meade ETX-125
* EQMac
* Simulators
