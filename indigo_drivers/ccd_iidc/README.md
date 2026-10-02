# IIDC CCD driver

https://damien.douxchamps.net/ieee1394

## Supported devices

Firewire and USB IIDC cameras supported by libdc1394 (PointGrey, ImagingSource, Atik GP and many other).

This driver supports hot-plug (multiple devices).

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license (libdc1394 is released under LGPL).

## Use

indigo_server indigo_ccd_iidc

## Status: Stable

Driver is developed and tested with:
* Atik GP (USB)
* PTGrey Flea2 (firewire)
* Imaging Source DMK 31BF03 (firewire)

## Testing

2026-09-22 07:51 3.0.0.20 mac arm64 Atik GP 1/1 OK
2026-09-27 21:34 3.0.0.22 mac arm64 fake SDK 16/16 OK
2026-09-30 18:41 3.0.0.22 linux arm64 Chameleon CMLN-13S2M 1/1 OK
2026-10-02 21:02 3.0.0.22 mac arm64 Chameleon CMLN-13S2M 1/1 OK
