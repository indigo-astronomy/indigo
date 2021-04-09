# ASCOM ALPACA bridge agent

https://github.com/ASCOMInitiative/ASCOMRemote/blob/master/Documentation/ASCOM%20Alpaca%20API%20Reference.pdf
https://ascom-standards.org/api

## Supported devices

N/A

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_agent_alpaca ...

## Status: Under development

## Notes

### General

Mapping from INDIGO names to ALPACA device numbers is maintained and made persistent in AGENT_ALPACA_DEVICES property.

All web configuration requests are redirected to INDIGO Server root.

### CCD

ICameraV3 implemented.

* no DSLR support
* ASCOM binning is always 1x1, INDIGO binning is masked by readout mode
* INDIGO RGB is mapped to Colour, other modes to Mono sensor type (no bayer offsets etc)
* INDIGO camera mode is mapped to ASCOM readout mode
* None and gzip image compression supported (no deflate)

### Wheel

IFilterWheelV2 implemented, no limitations

### Focuser

IFocuserV1 implemented, no limitations

### Mount

ITelescopeV3 implemented with exception of MoveAxis method group (not compatible with INDIGO substantially)

### Guider

Sufficient subset of ITelescopeV3 implemented, no limitations

### Lightbox

ICoverCalibratorV1 implemented, HaltCover is dummy method (no counterpart in INDIGO)

### Rotator

IRotatorV3 implemented, no limitations

### Dome

IDomeV2 implemented, no limitations

### Aux PowerBox & Aux GPIO
ISwitchV2 implemented, no limitations

### Aux Weather
Not implemented yet.
