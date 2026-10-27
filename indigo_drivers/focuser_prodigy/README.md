# PegasusAstro Prodigy Microfocuser driver

http://www.pegasusastro.com

## Supported devices

* PegasusAstro Prodigy Microfocuser (firmware 1.4 and later), including its two power outlets and two USB ports

**Pegasus Prodigy Focuser** and **Pegasus Prodigy Powerbox** are present on startup and share one serial connection. Additional independent pairs can be configured through the focuser's additional instances property. No hot-plug support.

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_focuser_prodigy

## Status: Under development

## Notes

Set the port on **Pegasus Prodigy Focuser**, including when using only **Pegasus Prodigy Powerbox**; the device communicates at 19200 baud, 8N1. Either logical device can connect first, and disconnecting one preserves the other's connection.

Focuser: absolute GOTO, relative movement, coordinate SYNC, abort, speed, backlash, temperature and encoder-zero park are supported. Reverse is fixed normal; automatic compensation is not supported. Both software limits are enforced (default -999999 to 999999); they do not program a hardware endpoint. `X_FOCUSER_PARK.PARK` moves to the zero encoder position and stays BUSY until the focuser stops there; it is rejected if zero is outside the configured software limits or another motion is pending. 100 unchanged movement polls trigger a stop attempt and ALERT. After uncertain movement, use abort to confirm the stopped position before another move.

Powerbox: `AUX_POWER_OUTLET` and `AUX_USB_PORT` control both channels independently. `AUX_OUTLET_NAMES` is available before connection and the labels are persistent. `X_AUX_REBOOT.REBOOT` reboots the controller and reports completion once the device answers again; it is rejected during movement.

## Testing

- 2026-09-27 13:21 3.0.0.9 linux x64 simulator 58/58 OK
- 2026-10-09 21:53 3.0.0.11 linux arm64 Prodigy Arduino simulator (ESP32-S3) 13/13 OK
- 2026-10-10 03:54 3.0.0.12 mac arm64 simulator 62/62 OK
