# LX200 mount driver

## Supported devices

* Meade mounts
* Avalon mounts with StarGo controller
* Losmandy Gemini mounts
* 10micron mounts
* Astro-Physics GTO mounts
* OnStep controllers (http://www.stellarjourney.com/index.php?r=site/software_telescope)
* ZWO AM5 mount (https://astronomy-imaging-camera.com/)
* Pegasus NYX-101 (https://pegasusastro.com/products/nyx-101-harmonic-gear-mount/)
* TeenAstro controllers (https://groups.io/g/TeenAstro)
* aGotino controllers (https://github.com/mappite/aGotino)
* ESP32Go controllers (https://github.com/masutokw/esp32go)

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

Use URL in form lx200://host:port to connect to the mount over network (default port is 4030, 9999 for NYX-101). An ESP32Go serves LX200 on port 10001, so its port has to be given explicitly.

An ESP32Go has no command that stops tracking and none that releases a mount sent to its home position, so this driver offers neither a tracking switch nor a park control for it. Its home control sends the mount to the home position and its "set home" stores the position the mount stands on. Over a serial port the controller answers at 115200 baud, and it needs about twelve seconds after a reset before it replies.

## Status: Stable

Driver is developed and tested with:
* Meade ETX-125
* LX200GPS
* ZWO AM5
* Pegasus NYX-101
* ESP32Go
* Simulators

## Testing

2026-09-22 23:07 3.0.0.57 mac arm64 Pegasus NYX-101 30/30 OK
2026-09-23 00:11 3.0.0.58 mac arm64 OnStepX 33/33 OK
2026-09-23 10:06 3.0.0.59 mac arm64 aGotino 35/35 OK
2026-09-23 11:17 3.0.0.60 mac arm64 OpenAstroTracker 35/35 OK
2026-09-23 14:18 3.0.0.63 mac arm64 ESP32Go 35/35 OK
2026-09-23 19:30 3.0.0.64 mac arm64 simulator 97/97 OK
2026-09-23 19:34 3.0.0.64 mac arm64 MountSim 2.3 (StarGO) 14/14 OK
