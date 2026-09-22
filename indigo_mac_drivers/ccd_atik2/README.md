# Legacy Atik CCD driver

https://www.atik-cameras.com

## Supported devices
* Atik 3xx
* Atik 4xx
* Atik One
* Atik TSxx
* Atik FSxx
* Atik LF
* Artemis LF
* Atik VSx
* Atik T5
* Atik Infinity
* Atik Titan
* Atik QSxx
* Synoptics HS

This driver supports hot-plug (multiple devices).

## Supported platforms

This driver depends on 3rd party library and is supported on MacOS.

## License

INDIGO Astronomy open-source license (3rd party library is closed source).

## Use

indigo_server indigo_ccd_atik2

## Status: Stable

Driver is developed and tested with:
* Atik Titan
* Atik 383L+
* Atik One 9
* Atik VS 6
* Atik 11000

## Comments

This driver is provided only as a temporary solution until Atik will provide Apple Silicon support in their SDK.

## Testing

2026-09-22 09:45 3.0.0.14 mac arm64 Atik One 1/1 OK
2026-09-22 09:48 3.0.0.14 mac arm64 Atik VS 1/1 OK
2026-09-22 09:51 3.0.0.14 mac arm64 Atik Titan 1/1 OK
2026-09-22 09:56 3.0.0.14 mac arm64 fake SDK 24/24 OK
2026-09-22 17:05 3.0.0.15 mac arm64 fake SDK 25/25 OK
2026-09-22 17:22 3.0.0.15 mac arm64 Atik 11000 1/0 Failed
