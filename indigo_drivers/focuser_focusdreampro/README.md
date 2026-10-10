# AstroGadget FocusDreamPro focuser driver

https://sites.google.com/view/astro-gadget/control-of-focusers/focusdreampro

https://github.com/sirJolo/ascom-jolo-focuser

## Supported devices
* AstroGadget FocusDreamPro controller
* Jolo ASCOM focuser (the same RS232 command set)

Single device is present on the first startup (no hot-plug support). Additional devices can be configured on runtime.

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_focuser_focusdreampro

## Status: Stable

Tested with physical device provided by courtesy of AstroGadget.
You may need to install Silicon Labs CP2102 driver for host operating system).

## Testing

2026-09-22 21:05 3.0.0.9 mac arm64 AstroGadget FocusDreamPro 15/15 OK
2026-09-30 19:34 3.0.0.11 linux arm64 simulator 18/18 OK
2026-10-05 20:52 3.0.0.12 mac arm64 AGadget FocusDreamPro 15/15 OK
2026-10-10 13:56 3.0.0.14 mac arm64 simulator 36/36 OK
2026-10-10 18:53 3.0.0.14 linux arm64 AGadget FocusDreamPro 15/15 OK
