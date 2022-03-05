# LX200 mount driver

## Supported devices

* Meade mounts
* Avalon mounts with StarGo controller
* Losmandy Gemini mounts
* 10micron mounts
* EQMac
* Astro-Physics GTO mounts
* OnStep controllers (http://www.stellarjourney.com/index.php?r=site/software_telescope)

Connection over serial port or network.

Single device is present on the first startup (no hot-plug support). Additional devices can be configured on runtime.

## Supported platforms

This driver is platform independent

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_mount_lx200

## Comments

A non-standard switch properties "Alignment mode" and "Mount type" are provided by this driver.

Astro-Physics mount can't be detected automatically, use Mount type property om nount device to select it first.

Use URL in form lx200://host:port to connect to the mount over network (default port is 4030). This URL is set automatically, if EQMac mount type is selected.

## Status: Stable

Driver is developed and tested with:
* Meade ETX-125
* LX200GPS
* EQMac
* Simulators
