# Shoestring FCUSB focuser driver

http://www.store.shoestringastronomy.com/products_fc.htm

## Supported devices
* FCUSB
* FCUSB2
* FCUSB3

This driver supports hot-plug (single device of each type).

## Supported platforms

This driver depends on 3rd party library and is supported on Linux (Intel 32/64 bit and ARM v6+) and MacOS.

## License

INDIGO Astronomy open-source license (3rd party library is closed source).

## Use

indigo_server indigo_focuser_fcusb

## Comments

A non-standard switch property "Frequency" is provided by this driver.

## Status: Stable

Driver is tested with the physical hardware.

## Testing

2026-09-21 23:46 3.0.0.9 mac arm64 FCUSB 10/10 OK
2026-09-30 19:16 3.0.0.10 linux arm64 FCUSB Focuser 10/10 OK
2026-10-05 15:13 3.0.0.11 mac arm64 fake SDK 24/24 OK
2026-10-05 21:02 3.0.0.11 mac arm64 FCUSB Focuser 10/10 OK
