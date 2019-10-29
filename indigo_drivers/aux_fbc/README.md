# Lacerta Flat Box Controller USB driver

https://en.lacerta-optics.com/LacertaFBC_Flat-box-controller-FBC-regulates-a-flatfieldbox-to#m

## Supported devices
* Lacerta FBC

Single device is present on startup (no hot-plug support).

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_aux_fbc

## Comments
Lacerta FBC should be in SerialMode to be controlled over the USB. To put the device in this
mode one should turn all knobs to 0 before powering the device.

## Status: Under Development

Driver is developed and tested with:
* Lacerta FBC
