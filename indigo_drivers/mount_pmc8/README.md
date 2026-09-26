# PMC Eight mount driver

## Supported devices

* ExploreScientific iEXOS-100, iEXOS-300 and EXOS2-GT, 
* Losmandy G11 and Titan with PMC Eight controller

Connection over serial port, UDP and TCP.

Single device is present on the first startup (no hot-plug support). Additional devices can be configured on runtime.

## Supported platforms

This driver is platform independent

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_mount_pmc8

## Comments

Use URL in form tcp://host:port or udp://host:port to connect to the mount over network (default port is 54372).

## Status: Stable

Driver is developed and tested with iEXOS-100

## Testing

2026-09-24 23:45 3.0.0.12 mac arm64 iEXOS-100 38/38 OK
2026-09-26 14:52 3.0.0.13 mac arm64 simulator 19/19 OK
