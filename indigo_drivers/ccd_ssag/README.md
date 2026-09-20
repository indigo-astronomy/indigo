# Orion StarShoot AutoGuider (QHY5) CCD driver

http://www.telescope.com/Orion-StarShoot-AutoGuider/p/52064.uts
http://www.qhyccd.com/index.html

## Supported devices

* Orion StarShoot AutoGuider
* QHY5

This driver supports hot-plug (multiple devices).

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_ccd_ssag

## Comments

The following VID/PID combinations are recognised by this driver: 0x1856/0x0011 (SSAG), 0x1618/0x0901 (QHY5). 
You can configure custom VID/PID by setting environment variables SSAG_VID and SSAG_PID.

## Status: Stable

Driver is developed and tested with:
* QHY5

## Testing

2026-09-20 21:31 3.0.0.14 mac arm64 fake SDK 13/13 OK
