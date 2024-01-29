# ASCOM ALPACA bridge agent

This driver provides ASCOM Alpaca interface to INDIGO devices. This enables ASCOM applications to operate with INDIGO devices.

https://github.com/ASCOMInitiative/ASCOMRemote/blob/master/Documentation/ASCOM%20Alpaca%20API%20Reference.pdf

https://ascom-standards.org/api

https://ascom-standards.org/Developer/AlpacaImageBytes.pdf

## Supported devices

N/A

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_agent_alpaca ...

## INDIGO - Alpaca Device Mapping

|               | Camera | CoverCalibrator | Dome | FilterWheel | Focuser | ObservingConditions | Rotator | SafetyMonitor [3] | Switch | Telescope |
|---------------|:------:|:---------------:|:----:|:-----------:|:-------:|:-------------------:|:-------:|:-----------------:|:------:|:---------:|
| **CCD**       | OK [1] |                 |      |             |         |                     |         |                   |        |           |
| **Lightbox**  |        | OK [1]          |      |             |         |                     |         |                   |        |           |
| **Dustcap**   |        | OK              |      |             |         |                     |         |                   |        |           |
| **Dome**      |        |                 |  OK  |             |         |                     |         |                   |        |           |
| **Fileter**   |        |                 |      |     OK      |         |                     |         |                   |        |           |
| **Focuser**   |        |                 |      |             |    OK   |                     |         |                   |        |           |
| **Weather**   |        |                 |      |             |         |       Not Ready     |         |                   |        |           |
| **SQM**       |        |                 |      |             |         |       Not Ready     |         |                   |        |           |
| **Rotator**   |        |                 |      |             |         |                     |   OK    |                   |        |           |
| **Powerbox**  |        |                 |      |             |         |                     |         |                   |   OK   |           |
| **GPIO**      |        |                 |      |             |         |                     |         |                   |   OK   |           |
| **Mount**     |        |                 |      |             |         |                     |         |                   |        |    OK     |
| **Guider**    |        |                 |      |             |         |                     |         |                   |        |   OK [4]  |
| **AO**        |        |                 |      |             |         |                     |         |                   |        | Not Ready |
| **Joystick**  |        |                 |      |             |         |                     |         |                   |        | Not Ready |
| **GPS** [2]   |        |                 |      |             |         |                     |         |                   |        |           |
| **Shutter** [2]  |     |                 |      |             |         |                     |         |                   |        |           |

[1] Some device API can not be mapped 1:1, but ASCOM conformance passes. See Notes below.

[2] INDIGO device has no equivalent in Alpaca/ASCOM and can not be mapped.

[3] Alpaca/ASCOM Device has no equivalent in INDIGO and can not be mapped.

[4] INDIGO device has no equivalent in Alpaca/ASCOM but can be mapped to a subset of another device. See Notes below.


### General

Mapping from INDIGO names to ALPACA device numbers is maintained and made persistent in AGENT_ALPACA_DEVICES property.

All web configuration requests are redirected to INDIGO Server root.

### CCD

ICameraV3 implemented.

* no DSLR support
* ASCOM binning is always 1x1, INDIGO binning is masked by readout mode
* INDIGO RGB is mapped to Colour, other modes to Mono sensor type (no bayer offsets etc)
* INDIGO camera mode is mapped to ASCOM readout mode
* None and GZip image compression supported (no deflate)
* application/imagebytes transfer mode supported

#### Selecting "Image array transfer transfer method"
Alpaca supports several methods for image transfer, but some of them are mostly useless especially for large images. We strongly recommend to select "ImageBytes" as a transfer method.

### Wheel

IFilterWheelV2 implemented, no limitations

### Focuser

IFocuserV1 implemented, no limitations

### Mount

ITelescopeV3 implemented with exception of MoveAxis method group (not compatible with INDIGO substantially)

### Guider

Sufficient subset of ITelescopeV3 implemented, no limitations

### Aux Lightbox

ICoverCalibratorV1 implemented, HaltCover is dummy method (no counterpart in INDIGO)

### Rotator

IRotatorV3 implemented, no limitations

### Dome

IDomeV2 implemented, no limitations

### Aux Powerbox & Aux GPIO
ISwitchV2 implemented, no limitations

### Aux Weather
Not implemented yet.

## Status:
Needs more testing

## Notes

See [Driver ASCOM Conformance Test Results](ASCOM_CONFORMANCE.txt)
