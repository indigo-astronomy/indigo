# iOptron mount driver

## Supported devices

iOptron ZEQ25, iEQ30, iEQ45, SmartEQ, CEM25, CEM60, CEM60-CE or iEQ45 Pro mount connected over serial port or network.

Single device is present on the first startup (no hot-plug support). Additional devices can be configured on runtime.

## Supported platforms

This driver is platform independent

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_mount_ioptron

## Comments

Use URL in form ieq://host:port to connect to the mount over network (default port is 4030).

## Status: Stable

Driver is tested with:
* CEM60-EC
* iEQ 45 Pro

