# PrimaluceLab focuser/rotator driver

https://www.primalucelab.com

## Supported devices
* SESTO SENSO 2 focusers
* ESATTO focusers
* ARCO rotators

Single device is present on the first startup (no hot-plug support). Additional devices can be configured on runtime.

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_focuser_primaluce

## Notes

This driver is tested agains firmware version 3.05!

The controller is reached either on its USB serial port or over WiFi: set the device port to `http://<address>` (for example `http://192.168.4.1` on the controller's own access point), and the driver sends the same requests to the controller's web server.

To join an existing WiFi network, set its name and password in Advanced > STA WiFi settings and select Station mode in Advanced > WiFi mode. The controller restarts to apply the mode; its new address is shown by the network router.

To calibrate Sesto Senso 2 focuser:

* set the focuser to the innermost position manually,
* navigate to PrimaluceLab focuser > Advanced > Calibrate focuser with a control panel and click either "Start" or "Start inverted",
* once the outhermost position is reached, click "End",
* disconnect and connect again.

## Status: Stable

Tested with Sesto Senso 2.

As PrimaluceLab never answered any email both simulator and driver are based only on publicly available information :(

## Testing

2026-09-20 19:54 3.0.0.13 mac arm64 SESTO SENSO 2 18/18 OK
2026-09-30 18:48 3.0.0.15 linux arm64 SESTOSENSO2 18/18 OK
2026-10-03 19:54 3.0.0.16 linux arm64 SESTOSENSO2 (WiFi) 18/18 OK
2026-10-05 22:05 3.0.0.22 mac arm64 simulator 54/54 OK
2026-10-05 22:14 3.0.0.22 mac arm64 SESTOSENSO2 18/18 OK
2026-10-05 22:15 3.0.0.22 mac arm64 SESTOSENSO2 (WiFi) 18/18 OK
