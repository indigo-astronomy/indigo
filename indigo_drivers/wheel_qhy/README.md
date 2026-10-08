# QHY CFW wheel driver

https://www.qhyccd.com/index.php?m=content&c=index&a=show&catid=137&id=33
https://www.qhyccd.com/index.php?m=content&c=index&a=show&catid=137&id=34

## Supported devices

* QHY CFW1/2/3  filter wheels

Single device is present on the first startup (no hot-plug support). Additional devices can be configured on runtime.

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_wheel_qhy

## Status: Stable

Tested with CFW3

## Testing

2026-09-27 13:05 3.0.0.12 linux x64 simulator 4/4 OK
2026-10-08 21:24 3.0.0.13 mac arm64 simulator 6/6 OK
2026-10-08 21:25 3.0.0.13 linux arm64 QHY CFW3 Arduino simulator (ESP32-S3) 11/11 OK
2026-10-08 21:27 3.0.0.13 linux arm64 QHY CFW2 Arduino simulator (ESP32-S3) 11/11 OK
2026-10-08 21:29 3.0.0.13 linux arm64 QHY CFW1 Arduino simulator (ESP32-S3) 11/11 OK
