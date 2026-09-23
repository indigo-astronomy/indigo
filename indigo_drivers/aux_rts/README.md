# "RTS on COM" remote shutter driver

https://www.cloudynights.com/topic/457536-usb-corded-shutter-control-for-nikon/

## Supported devices
* Any RTS on COM device

Single device is present on the first startup (no hot-plug support). Additional devices can be configured on runtime.

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_aux_rts

## Comments

## Status: Stable

Driver is developed and tested with:
* A serial loopback of two FTDI USB serial adapters joined by a null-modem cable

No RTS on COM shutter release and no camera were involved, so the line the shutter contact hangs on
is verified electrically while the shutter release itself is not.

## Testing

2026-09-21 23:49 3.0.0.10 mac arm64 simulator 9/9 OK
2026-09-23 19:19 3.0.0.11 linux arm64 simulator 10/10 OK
2026-09-23 19:22 3.0.0.11 linux arm64 FTDI serial loopback 9/9 OK
