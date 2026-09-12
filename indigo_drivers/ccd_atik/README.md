# Atik CCD driver

https://www.atik-cameras.com

## Supported devices
* Any Atik camera supported by vendor SDK

This driver supports hot-plug (multiple devices).

## Supported platforms

This driver depends on 3rd party library and is supported on Linux (Intel 32/64 bit and ARM v6+) and MacOS.

## License

INDIGO Astronomy open-source license (3rd party library is closed source).

## Use

indigo_server indigo_ccd_atik

## Status: Stable

Driver is developed and tested with:
* Atik Titan
* Atik 383L+
* Atik One 9
* Atik VS 6
* Atik 11000
* Atik Horizon

## Generator migration

The source of truth is `indigo_ccd_atik.driver`; regenerate the C, header and
standalone main with `indigo_generator`. Migration progress, software acceptance
and pending Titan/One/11000A/Horizon hardware tests are recorded in `REFACTOR.md`.

Version 0x03000020 renames `ATIK_PRESETS` to `X_PRESETS` and
`ATIK_WINDOW_HEATER` to `X_WINDOW_HEATER`; update client scripts accordingly.
Standard property and custom item names are unchanged.
