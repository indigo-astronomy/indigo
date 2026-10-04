# ASCOM Alpaca client driver

This driver discovers ASCOM Alpaca servers on the local network and makes their devices available as INDIGO devices. This enables INDIGO applications to operate with Alpaca devices. It is the opposite direction of [ASCOM ALPACA bridge agent](../agent_alpaca), which exports INDIGO devices to ASCOM applications.

https://github.com/ASCOMInitiative/ASCOMRemote/blob/master/Documentation/ASCOM%20Alpaca%20API%20Reference.pdf

https://ascom-standards.org/api

https://ascom-standards.org/Developer/AlpacaImageBytes.pdf

## Supported devices

Any device served by an ASCOM Alpaca server (API version 1, interface versions up to ASCOM Platform 7).

The device "Alpaca" is always present. One proxy device is created at runtime for every Alpaca device that is not switched off in X_ALPACA_DEVICES. It is named "ALPACA DeviceName", with the word "alpaca" removed from DeviceName and every "@" replaced by "-", because INDIGO uses "@" for devices of remote services; devices of the same name are numbered "#2", "#3", ...

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
* Every discovered device is made available as an INDIGO device. Devices can be switched off in X_ALPACA_DEVICES; the switch is saved with the configuration of the device "Alpaca" and the device stays off after a restart. At most 32 devices can be used at a time.
* Devices exported by INDIGO ASCOM ALPACA bridge agent are ignored.
* Disconnecting an INDIGO device disconnects the Alpaca device on its server for all other clients of that server as well.
* Image is downloaded in ImageBytes format if the server supports it, in JSON format otherwise. JSON transfer of large images is very slow.
* HTTPS, authentication and IPv6 are not supported.
* A mount whose Alpaca driver moves the axes in sky directions (e.g. Pegasus NYX-101) needs X_ALPACA_MOUNT_AXES = SKY; a mount that ignores commands right after a slew needs X_ALPACA_SETTLE_TIME (2 s for the NYX-101).
* Hardware test: "python3 tools/run_driver_test.py system_alpaca --hw --port host:port[,host:port]" tests every function of every Alpaca device on those servers (without --port, of every device found by discovery) and restores what it changes; SYSTEM_ALPACA_HW_DEVICES=<text>[,<text>] after "--" restricts it to devices whose name or UniqueID contains one of the texts. Every tested device gets its own line below.

## Testing

2026-10-03 16:16 3.0.0.1 linux arm64 simulator 309/309 OK
2026-10-03 19:04 3.0.0.1 mac arm64 OmniSim 0.5.0 20/20 OK
2026-10-03 21:31 3.0.0.1 mac arm64 A simulator for the ASCOM FilterWheel API usable with Alpaca and COM 4/4 OK
2026-10-03 21:31 3.0.0.1 mac arm64 A simulator for the ASCOM Focuser API usable with Alpaca and COM 9/9 OK
2026-10-03 21:31 3.0.0.1 mac arm64 Alpaca CoverCalibrator Simulator 7/7 OK
2026-10-03 21:31 3.0.0.1 mac arm64 Alpaca Observing Conditions Simulator 5/5 OK
2026-10-03 21:31 3.0.0.1 mac arm64 ASCOM Dome Simulator .NET 12/12 OK
2026-10-03 21:31 3.0.0.1 mac arm64 ASCOM Rotator Driver for RotatorSimulator 9/9 OK
2026-10-03 21:31 3.0.0.1 mac arm64 ASCOM SafetyMonitor Simulator Driver 3/3 OK
2026-10-03 21:31 3.0.0.1 mac arm64 ASCOM SwitchV2 Simulator Driver. 4/4 OK
2026-10-03 21:31 3.0.0.1 mac arm64 Simulated Monochrome camera 8/8 OK
2026-10-03 21:31 3.0.0.1 mac arm64 Software Telescope Simulator for ASCOM 20/20 OK
2026-10-03 21:36 3.0.0.1 mac arm64 PegasusAstro NYX-101 20/20 OK
2026-10-03 21:56 3.0.0.1 linux arm64 OnStepX environmental sensors, firmware 10.28x 5/3 Failed
2026-10-03 21:56 3.0.0.1 linux arm64 OnStepX rotator, firmware 10.28x 9/6 Failed
2026-10-03 22:44 3.0.0.1 mac arm64 simulator 318/318 OK
2026-10-04 01:18 3.0.0.1 linux x64 Askar-WAF Focuser 8/8 OK
2026-10-04 00:07 3.0.0.1 linux arm64 OnStepX auxiliary features, firmware 10.28x 4/4 OK
2026-10-04 00:07 3.0.0.1 linux arm64 OnStepX focuser 1, firmware 10.28x 7/7 OK
2026-10-04 00:07 3.0.0.1 linux arm64 OnStepX mount, firmware 10.28x 20/16 Failed
2026-10-04 08:21 3.0.0.1 mac arm64 simulator 318/318 OK
