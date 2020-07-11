# PMC Eight mount driver

## Supported devices

* ExploreScientific mounts (iEXOS-100, EXOS2-GT)
* Losmandy G11 with PMC Eight controller

Connection over serial port, UDP and TCP.

Single device is present on startup (no hot-plug support).

## Supported platforms

This driver is platform independent

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_mount_pcm8

## Comments

Use URL in form tcp://host:port or udp://host:port to connect to the mount over network (default port is 54372).

## Status: Under development

Driver is developed and tested with iEXOS-100
