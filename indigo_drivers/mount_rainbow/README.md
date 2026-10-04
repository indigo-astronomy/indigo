# RainbowAstro mount driver

## Supported devices

* RainbowAstro mounts

Connection over serial port or network.

Mount and guider devices are present on the first startup (no hot-plug support). Additional devices can be configured on runtime.

## Supported platforms

This driver is platform independent

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_mount_rainbow

## Comments

Use URL in form rainbow://host:port to connect to the mount over network (default port is 7100).

## Status: Under development

Driver is developed and tested with simulator

## Testing

2026-09-23 14:04 3.0.0.18 mac arm64 MountSim 2.3 (RainbowAstro RST135) 13/13 OK
2026-09-27 12:23 3.0.0.21 linux x64 simulator 18/18 OK
2026-10-04 21:09 3.0.0.24 mac arm64 simulator 39/39 OK
