# ASCOL Telescope system driver

## Supported devices

Any ASCOL protocol compatible Telescope system (Zeiss 2m RCC) connected over network.

Single device is present on startup (no hot-plug support).

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_system_ascol

## Status: Under Development

Driver is developed and tested with:
* ascol_simulator.pl

## Comments

Use URL in form tcp://host:port, ascol://host:port or simply host:port to connect to the system over the network (default port is 2001).
