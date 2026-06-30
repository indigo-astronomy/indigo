# Askar-WAF USB CDC focuser driver

https://www.askarscope.com/

## Supported devices

* Askar-WAF (USB CDC virtual serial port, e.g. `/dev/ttyACM0`)

Single device is present on the first startup (no hot-plug support). Additional devices can be configured at runtime.

## Supported platforms

This driver is supported on Linux (Intel 32/64 bit and ARM v6+) and MacOS.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_focuser_askar

## Comments

The focuser can be connected over its USB CDC virtual serial port (e.g. `/dev/ttyACM0`)
or over the network. Use a URL of the form `askar://host:port` to connect to a WiFi
focuser over TCP (default port is 8080). Use just `askar://` for autodiscovery: the
driver broadcasts `WAF:discover` on UDP port 7676, the focuser answers
`WAF:<ip>:<port>`, and the driver then connects to that address over TCP. The
discovered address is written back into the port field, so a saved configuration
reconnects directly without re-broadcasting.

## Status: Stable

Driver is developed and tested with:
* Askar-WAF focuser
