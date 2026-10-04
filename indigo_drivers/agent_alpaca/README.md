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

IFocuserV3 implemented, no limitations

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
ISwitchV2 implemented. The switches are the power outlets, heater outlets, USB ports, GPIO outlets, the values of `X_ALPACA_SWITCH_VALUES` (writable switches with a range, from `system_alpaca`) and the GPIO sensors, in this order, at most 8 of each kind.

### Aux Weather
Not implemented yet.

## Status:
Needs more testing

## Notes

See [ASCOM ConformU test logs](conformu)

## Testing

2026-10-01 10:02 3.0.0.11 mac arm64 ConformU 4.5.0 Camera, CCD Guider Simulator 115/115 OK
2026-10-01 10:03 3.0.0.11 mac arm64 ConformU 4.5.0 Camera, CCD Bahtinov Mask Simulator 107/107 OK
2026-10-01 10:03 3.0.0.11 mac arm64 ConformU 4.5.0 Camera, DSLR Simulator 107/107 OK
2026-10-01 10:04 3.0.0.11 mac arm64 ConformU 4.5.0 Camera, CCD File Simulator 107/107 OK
2026-10-01 10:05 3.0.0.11 mac arm64 ConformU 4.5.0 FilterWheel, CCD Imager Simulator (wheel) 49/49 OK
2026-10-01 10:06 3.0.0.11 mac arm64 ConformU 4.5.0 CoverCalibrator, FlipFlat 40/40 OK
2026-10-01 10:16 3.0.0.11 mac arm64 ConformU 4.5.0 Focuser, Ultimate Powerbox 3 (focuser) 28/28 OK
2026-10-01 10:17 3.0.0.11 mac arm64 ConformU 4.5.0 Camera, CCD Imager Simulator 118/118 OK
2026-10-03 23:32 3.0.0.12 mac arm64 ConformU 4.5.0 Rotator, Field Rotator Simulator 73/73 OK
2026-10-03 23:36 3.0.0.12 mac arm64 ConformU 4.5.0 Focuser, CCD Imager Simulator (focuser) 35/35 OK
2026-10-04 09:45 3.0.0.13 mac arm64 ConformU 4.5.0 Dome, Dome Simulator 69/69 OK
2026-10-04 09:49 3.0.0.13 mac arm64 ConformU 4.5.0 Telescope, Mount Simulator 234/234 OK
2026-10-04 09:54 3.0.0.13 mac arm64 ConformU 4.5.0 Switch, Pocket Powerbox 241/241 OK
2026-10-04 10:02 3.0.0.13 mac arm64 ConformU 4.5.0 Switch, Ultimate Powerbox 3 772/772 OK
2026-10-04 10:24 3.0.0.13 mac arm64 ConformU 4.5.0 Telescope, Mount LX200 234/234 OK
2026-10-04 10:28 3.0.0.13 mac arm64 ConformU 4.5.0 Camera, ALPACA Camera Sim (OmniSim 0.5.0 via system_alpaca) 110/110 OK
2026-10-04 10:28 3.0.0.13 mac arm64 ConformU 4.5.0 CoverCalibrator, ALPACA CoverCalibrator Simulator (OmniSim 0.5.0 via system_alpaca) 40/40 OK
2026-10-04 10:29 3.0.0.13 mac arm64 ConformU 4.5.0 Dome, ALPACA Dome Simulator (OmniSim 0.5.0 via system_alpaca) 86/86 OK
2026-10-04 10:34 3.0.0.13 mac arm64 ConformU 4.5.0 FilterWheel, ALPACA Filter Wheel Simulator - 0 (OmniSim 0.5.0 via system_alpaca) 53/53 OK
2026-10-04 10:34 3.0.0.13 mac arm64 ConformU 4.5.0 Focuser, ALPACA Focuser Simulator - 0 (OmniSim 0.5.0 via system_alpaca) 35/35 OK
2026-10-04 10:39 3.0.0.13 mac arm64 ConformU 4.5.0 Rotator, ALPACA Rotator Simulator - 0 (OmniSim 0.5.0 via system_alpaca) 73/73 OK
2026-10-04 10:43 3.0.0.13 mac arm64 ConformU 4.5.0 Switch, ALPACA Switch Simulator (OmniSim 0.5.0 via system_alpaca) 331/331 OK
2026-10-04 10:52 3.0.0.13 mac arm64 ConformU 4.5.0 Telescope, ALPACA Telescope Simulator (OmniSim 0.5.0 via system_alpaca) 234/234 OK
