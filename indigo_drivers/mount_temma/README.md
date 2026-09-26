# Takahashi Temma mount driver

## Supported devices

Any Temma protocol compatible mount connected over serial port.

Single device is present on the first startup (no hot-plug support). Additional devices can be configured on runtime.

## Supported platforms

This driver is platform independent

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_mount_temma

## Status: Stable

Driver is developed and tested with:
* Takahashi Temma EM-11

## Testing

2026-09-23 13:37 3.0.0.15 mac arm64 MountSim 2.3 (Temma) 12/12 OK
2026-09-24 18:01 3.0.0.16 linux x64 simulator 16/13 Failed
2026-09-26 14:52 3.0.0.18 mac arm64 simulator 16/16 OK
