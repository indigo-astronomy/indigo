# Celestron / PlaneWave EFA focuser driver

* https://www.celestron.com/products/focus-motor-for-sct-and-edgehd
* https://planewave.com/products-page/general-accessories/efa-kit-electronic-focuser/

## Supported devices

* Celestron Focus Motor for SCT and EdgeHD
* EFA control unit

Single device is present on the first startup (no hot-plug support). Additional devices can be configured on runtime.

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_focuser_efa

## Status: Stable

Open issues: wrong temperature report, can't set min value, connection via AUX port not currently supported
