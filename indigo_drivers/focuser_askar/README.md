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

## Status: Under development

Driver is developed and tested with:
* Askar-WAF focuser simulator (`askar_focuser_simulator`) shipping under this directory
