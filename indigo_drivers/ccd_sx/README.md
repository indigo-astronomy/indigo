# StarlightXpress CCD driver

http://www.sxccd.com

## Supported devices

* SXVF-Mxxx
* SXVR-Hxxx
* Trius-SXxxx
* Lodestar Autoguider
* Superstar Autoguider
* CoStar Autoguider
* Ultrastar Autoguider

This driver supports hot-plug (multiple devices).

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_ccd_sx

## Status: Stable

Driver is developed and tested with:
* SX Loderstar
* SX Lodestar X2
* SX H694

## Testing

2026-09-22 15:52 3.0.0.20 mac arm64 SXVR-H694 and LodeStar 9/9 OK
2026-09-27 08:17 3.0.0.23 linux x64 fake SDK 25/25 OK
2026-09-27 21:59 3.0.0.23 mac arm64 fake SDK 25/25 OK
2026-09-30 18:07 3.0.0.23 linux arm64 SXVR-H694 #0302 9/9 OK
2026-09-30 18:11 3.0.0.23 linux arm64 SX LodeStar #0102 9/9 OK
2026-10-02 20:52 3.0.0.23 mac arm64 SX LodeStar #01010101 and SXVR-H694 #01010103 9/9 OK
