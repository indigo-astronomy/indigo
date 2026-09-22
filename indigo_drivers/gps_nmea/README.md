# GPS NMEA driver

## Supported devices

All NMEA 0183 GPS devices connected over USB, serial port @ 9600bps, bluetooth or network.

Single device is present on the first startup (no hot-plug support). Additional devices can be configured on runtime.

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_gps_nmea

## Status: Stable

Driver is developed and tested with:
* Several U-Blox GPS modules (RS232)
* G-Mouse GPS receiver (USB)
* Nokia LD-3W (bluetooth)

## Comments
Use URL in form gps://host:port to connect to the GPS over network (default port is 9999).

To export the GPS over the network one can use Nexbridge https://sourceforge.net/projects/nexbridge

## Testing

2026-09-22 21:28 3.0.0.20 mac arm64 simulator 5/5 OK
2026-09-22 21:38 3.0.0.20 mac arm64 u-blox 7 9/9 OK
