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

The mount's Main group includes "RTS/CTS flow control" (`FLOW_CONTROL`),
with On and Off options. It defaults to On. Disable it when the serial
connection does not support RTS/CTS hardware flow control. Disconnect both the mount
and guider before changing this setting. Use the configuration Save command
to persist the selection.

## Status: Stable

Driver is developed and tested with:
* Takahashi Temma EM-11
