# GPS NMEA driver

## Supported devices

All NMEA 0183 GPS devices connected over USB, serial port @ 9600bps or network.

Single device is present on startup (no hot-plug support).

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_gps_nmea

## Status: Under Development

## Comments
Use URL in form gps://host:port to connect to the GPS over network (default port is 9999).

To export the GPS over the network one can use Nexbridge https://sourceforge.net/projects/nexbridge
