# Interactive Astronomy SkyAlert driver

https://interactiveastronomy.com/skyalertindex.html

## Supported devices
* Interactive Astronomy SkyAlert device with serial connection

Single device is present on the first startup (no hot-plug support). Additional devices can be configured on runtime.

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_aux_skyalert

## Notes

The driver classifies the readings into the weather conditions and the weather safety verdict. The cloud condition uses the sky temperature minus the ambient temperature. The rain (dampness), wind and sky brightness readings are raw 0–1023 values whose scale depends on the sensors, so check their thresholds for your unit. The wind thresholds default to 1024, which turns the wind condition off until you set them.

A failed or missing sensor is reported as `nan`. A sensor that has not given a valid reading since the device was connected counts as not fitted: its condition is IDLE and it does not affect the weather safety verdict. Once the sensor has given a valid reading during the connection, a later `nan` (or a reading that cannot be received at all) counts as a sensor failure: its condition turns ALERT and the weather is reported unsafe. Each new connection starts again from the sensors that give valid readings when it is made. The cloud condition needs valid readings of both the sky and the ambient temperature.

## Status: Untested

Developed with a simulator only

## Testing

- 2026-10-09 15:03 3.0.0.9 mac arm64 simulator 11/11 OK
- 2026-10-09 15:07 3.0.0.9 linux arm64 Interactive Astronomy SkyAlert Arduino simulator (ESP32-S3) 6/6 OK
