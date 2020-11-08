# Celestron NexStar AUX mount driver

## Supported devices

* Any Celestron mount with WiFi conntection 

Single device is present on startup (no hot-plug support).

## Supported platforms

This driver is platform independent

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_mount_nexstaraux

## Comments

Driver works only with the mounts in EQ mode!!!

Use URL in form nexstar://[host[:port]] to connect to the mount over network (if no host/port is used, driver will try to autodetect it, default port is 2000).

## Status: Stablee

Driver is developed and tested with SkyPortal WiFi module and NextStar 4SE mount in EQ mode
