# ASCOM Alpaca client driver

This driver discovers ASCOM Alpaca servers on the local network and makes their devices available as INDIGO devices. This enables INDIGO applications to operate with Alpaca devices. It is the opposite direction of [ASCOM ALPACA bridge agent](../agent_alpaca), which exports INDIGO devices to ASCOM applications.

https://github.com/ASCOMInitiative/ASCOMRemote/blob/master/Documentation/ASCOM%20Alpaca%20API%20Reference.pdf

https://ascom-standards.org/api

https://ascom-standards.org/Developer/AlpacaImageBytes.pdf

## Supported devices

Any device served by an ASCOM Alpaca server (API version 1, interface versions up to ASCOM Platform 7).

The device "Alpaca" is always present. One proxy device named "DeviceName @ ServerName" is created at runtime for every Alpaca device selected in X_ALPACA_DEVICES property.

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_system_alpaca

## Alpaca - INDIGO Device Mapping

| Alpaca device       | INDIGO device                                     |
|---------------------|---------------------------------------------------|
| Camera              | CCD, Guider if the camera can pulse guide         |
| Telescope           | Mount, Guider if the mount can pulse guide        |
| Focuser             | Focuser                                           |
| FilterWheel         | Wheel                                             |
| Rotator             | Rotator                                           |
| Dome                | Dome                                              |
| CoverCalibrator     | Aux Lightbox with a cover                         |
| Switch              | Aux GPIO                                          |
| ObservingConditions | Aux Weather                                       |
| SafetyMonitor       | Aux with X_ALPACA_SAFETY property                 |

## Status: Under development

Driver is developed and tested with simulators only:
* system_alpaca_simulator (part of INDIGO test suite)
* ASCOM Alpaca Simulators (OmniSim)

## NOTES:
* Servers are found by Alpaca discovery (UDP port 32227 by default, see X_ALPACA_DISCOVERY_SETTINGS). A server which does not answer the discovery can be added as host:port in X_ALPACA_SERVERS.
* Discovered devices are listed in X_ALPACA_DEVICES, only the selected ones are made available as INDIGO devices. The selection is saved with the configuration of the device "Alpaca".
* Devices exported by INDIGO ASCOM ALPACA bridge agent are ignored.
* Disconnecting an INDIGO device disconnects the Alpaca device on its server for all other clients of that server as well.
* Image is downloaded in ImageBytes format if the server supports it, in JSON format otherwise. JSON transfer of large images is very slow.
* HTTPS, authentication and IPv6 are not supported.

## Testing

2026-10-03 04:17 3.0.0.1 mac arm64 simulator 297/297 OK
2026-10-03 04:18 3.0.0.1 mac arm64 OmniSim 0.5.0 20/20 OK
