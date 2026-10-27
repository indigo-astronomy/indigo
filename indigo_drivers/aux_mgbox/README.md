# Astromi.ch MGBox driver

https://astromi.ch

## Supported devices

Astromi.ch MBox, MGBox v1/v2, MGPBox and PBox devices

`MGBox Weather` (combined Weather/Powerbox AUX) and `MGBox GPS` are present on startup, with optional additional instances and no hot-plug support.

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_aux_mgbox

## Status: Stable

Driver is developed and tested with:
* MGPBox

## Notes

Configure the port and baud on `MGBox Weather`, even when connecting only `MGBox GPS`. Both logical devices share the physical connection; either can disconnect while the other remains active. The serial default is 38400 baud. Use `mgbox://host:port` for TCP (default port 9999); a bridge such as Nexbridge can export the serial device.

Capabilities follow the reported device type: `M` enables weather, `G` permits GPS connection and `P` permits the pulse outlet. MBox and PBox reject GPS.

Powerbox/PBox uses the `Switch Control` group of `MGBox Weather`: outlet names, the GPIO pulse switch and pulse lengths belong to Powerbox. Lengths are milliseconds. A pulse reports BUSY until its duration expires and then resets the switch. Disconnect cancels the local completion callback, but cannot abort a pulse already running in hardware. Calibration and mount forwarding wait for matching readback and report ALERT on timeout. Reboot controls reset after a two-second settling delay; the protocol does not independently acknowledge completed reboot.

10Micron firmware is deprecated and unsupported; GPS, weather and pulse control remain available where the reported model supports them.

## Testing

- 2026-09-27 07:47 3.0.0.13 linux x64 simulator 35/35 OK
- 2026-09-27 20:51 3.0.0.13 mac arm64 simulator 35/35 OK
