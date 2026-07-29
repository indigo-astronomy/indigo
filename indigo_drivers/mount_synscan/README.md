# SynScan mount driver

## Supported devices

Any SynScan protocol compatible mount (SkyWatcher; Celestron; Orion; ...) connected over serial port or network.

Single device is present on the first startup (no hot-plug support). Additional devices can be configured on runtime.

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_mount_synscan

## Status: Stable

Driver is developed and tested with:
* SkyWatcher NEQ6 Pro
* SkyWatcher EQAZ6
* SkyWatcher EQ8
* SkyWatcher AZ-GTi

## Comments

Use URL in form synscan://host:port to connect to the mount over UDP (default port is 11880). Use just synscan:// for autodetection.

A non-standard switch property "Guider rate" is provided by this driver.

If the mount is not stationary (i.e. it is not part of a permanent setup, such as in an observatory) removing the existing alignment points should be part of the installation process: clear them when setting up the mount, before performing any sync and before using it. Otherwise the old alignment points, which correspond to the previous physical setup, will interfere with the new ones and the pointing accuracy will deteriorate.
