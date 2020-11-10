# Astromi.ch MGBox driver

https://astromi.ch

## Supported devices

Astromi.ch MBox, MGBox v1/v2, MGPBox and PBox devices

Single GPS and single Auxiliary Weather/GPIO devices are present on startup (no hot-plug support).

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_aux_mgbox

## Status: Stable

Driver is developed and tested with:
* MGPBox

## Comments
* GPS device will fail to connect if there is no GPS in the device (MBox, MGBox and PBox)
* PBox is supported through the 'Switch Control' group of the Weather/GPIO device.
* 10Micron firmware is deprecated and is not supported, however GPS, Weater and Pulse switch will still work.
* Use URL in form mgbox://host:port to connect to the MGBox over network (default port is 9999).
To export the MGBox over the network one can use Nexbridge https://sourceforge.net/projects/nexbridge
