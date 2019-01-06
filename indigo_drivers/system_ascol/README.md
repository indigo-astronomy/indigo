# ASCOL Telescope system driver

## Supported devices

Any ASCOL protocol compatible Telescope system (Zeiss 2-m RCC) connected over network.
The driver controls Telescope, Dome, Focuser and Guider.

All devices are present on connect (no hot-plug support).

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_system_ascol

## Status: Release Candidate

Driver is developed and tested with:
* ascol_simulator.pl
* Zeiss 2m RCC Telescope at NAO Rozhen

## Comments

Use URL in form tcp://host:port, ascol://host:port or simply host:port to connect to the system over the network (default port is 2002).
