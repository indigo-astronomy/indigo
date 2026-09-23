# RainbowAstro mount driver

## Supported devices

* RainbowAstro mounts

Connection over serial port or network.

Single device is present on the first startup (no hot-plug support). Additional devices can be configured on runtime.

## Supported platforms

This driver is platform independent

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_mount_rainbow

## Comments

Use URL in form rainbow://host:port to connect to the mount over network (default port is 4030).

## Status: Under development

Driver is developed and tested with simulator

## Testing

2026-09-23 14:04 3.0.0.18 mac arm64 MountSim 2.3 (RainbowAstro RST135) 13/13 OK
2026-09-23 14:05 3.0.0.18 mac arm64 simulator 15/15 OK
