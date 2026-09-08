# Player One CCD driver

https://player-one-astronomy.com

## Supported devices

All cameras supported by SDK (Plyer One and iOptron cameras)

This driver supports hot-plug (multiple devices).

## Supported platforms

This driver depends on 3rd party library and is supported on Linux (Intel 64 bit and ARM v6+) and MacOS.

## License

INDIGO Astronomy open-source license (3rd party library is closed source).

## Use

indigo_server indigo_ccd_playerone

## Status: Stable

## Tested with:

* MARS-C II
* Poseidon-C Pro
* Sedna-M


## INDIGO 3.0 generator migration

Custom properties use the `X_` prefix: `PIXEL_FORMAT` becomes `X_PIXEL_FORMAT`, and `POA_ADVANCED`, `POA_PRESETS`, `POA_CUSTOM_SUFFIX`, `POA_SENSOR_MODE` become `X_ADVANCED`, `X_PRESETS`, `X_CUSTOM_SUFFIX`, `X_SENSOR_MODE`. Update scripts and saved custom-property configuration to the new names, then save configuration again. Standard properties and custom item names are unchanged.

The generated implementation was validated on Mars-C II (macOS arm64, SDK 3.10.1/API 20260430), including USB removal during acquisition/guiding, suffix replug/restore and dynamic driver reload. The older Poseidon-C Pro and Sedna-M entries above are historical tests, not migration acceptance for those models. See `REFACTOR.md` and repository `TESTING.md` for current coverage and unavailable equipment/platforms.
