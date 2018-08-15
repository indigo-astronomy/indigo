# iOptron mount driver

## Supported devices

iOptron ZEQ25, iEQ30, iEQ45 or SmartEQ mount connected over serial port or network.

Single device is present on startup (no hot-plug support).

## Supported platforms

This driver is platform independent

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_mount_ioptron

## Comments

Use URL in form ieq://host:port to connect to the mount over network (default port is 4030).

A non-standard switch property "Alignment mode" is provided by this driver.

## Status: Under development
