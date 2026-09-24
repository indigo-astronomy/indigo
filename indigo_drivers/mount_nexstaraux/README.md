# Celestron NexStar AUX mount driver

## Supported devices

* Any Celestron mount with WiFi conntection 

Single device is present on the first startup (no hot-plug support). Additional devices can be configured on runtime.

## Supported platforms

This driver is platform independent

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_mount_nexstaraux

## Comments

Driver works only with the mounts in EQ mode!!!

Use URL in form nexstar://[host[:port]] to connect to the mount over network (if no host/port is used, driver will try to autodetect it, default port is 2000).

## Status: Stable

Driver is developed and tested with SkyPortal WiFi module and NextStar 4SE mount in EQ mode

## Testing

2026-09-20 21:29 3.0.0.14 mac arm64 simulator 30/30 OK
2026-09-24 00:12 3.0.0.18 linux arm64 simulator 40/40 OK
2026-09-24 05:44 3.0.0.18 linux arm64 NexStar SE & NexStar+ HC 36/36 OK
2026-09-24 11:00 3.0.0.18 mac arm64 NexStar SE & NexStar+ HC 36/36 OK
