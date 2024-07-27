# LX200 mount driver

## Supported devices

* Meade mounts
* Avalon mounts with StarGo controller
* Losmandy Gemini mounts
* 10micron mounts
* EQMac
* Astro-Physics GTO mounts
* OnStep controllers (http://www.stellarjourney.com/index.php?r=site/software_telescope)
* ZWO AM5 mount (https://astronomy-imaging-camera.com/)
* Pegasus NYX-101 (https://pegasusastro.com/products/nyx-101-harmonic-gear-mount/)
* TeenAstro controllers (https://groups.io/g/TeenAstro)

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

Use URL in form lx200://host:port to connect to the mount over network (default port is 4030, 9999 for NYX-101). This URL is set automatically, if EQMac mount type is selected.

## Status: Stable

Driver is developed and tested with:
* Meade ETX-125
* LX200GPS
* EQMac
* ZWO AM5
* Pegasus NYX-101
* Simulators
