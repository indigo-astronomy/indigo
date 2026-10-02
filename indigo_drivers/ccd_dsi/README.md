# Meade Deep Sky Imager driver

http://www.meade.com

## Supported devices

Meade DSI Pro/Color, DSI Pro/Color II and DSI Pro/Color III cameras.

This driver supports hot-plug (multiple devices).

## Supported platforms

This driver works on Linux (Intel 32/64 bit and ARM v6+) and MacOS.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_ccd_dsi

## Status: Stable

Driver is developed and tested with:
* Meade DSI
* Meade DSI II Pro
* Meade DSI III Pro

## NOTES:
DSI cameras are not supported by Meade any more. We provide this driver as they are still popular.
The firmware has issues but we tried to make the driver as stable as possible bypassing the instabilities of the firmware.

## Testing

2026-09-30 18:52 3.0.0.18 linux arm64 DSI Color 15/15 OK
2026-09-30 18:52 3.0.0.18 linux arm64 DSI Color (hot-plug) 17/17 OK
2026-10-02 21:06 3.0.0.18 mac arm64 DSI Color 15/15 OK
