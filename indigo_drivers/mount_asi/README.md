# ZWO AM mount driver

## Supported devices

* ZWO AM5 mount (https://astronomy-imaging-camera.com/)

Connection over serial port or network.

Single device is present on the first startup (no hot-plug support). Additional devices can be configured on runtime.

## Supported platforms

This driver is platform independent

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_mount_asi

## Comments

Use URL in form asi://192.168.4.1:4030 to connect to the mount over network.

## Status: Stable

Driver is developed and tested with:
* ZWO AM5
