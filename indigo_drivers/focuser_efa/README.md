# Celestron / PlaneWave EFA focuser driver

https://planewave.com/products-page/general-accessories/efa-kit-electronic-focuser/#.XXPOEi2B3OQ

## Supported devices

* Celestron Focus Motor for SCT and EdgeHD
* PlaneWave EFA control unit

One `EFA Focuser` is present at startup; additional independent serial instances can be configured. INFO identifies the actual model.

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_focuser_efa

## Status: Under development

## Notes

Connect through the PC/USB serial port at 19200 baud and select the serial port before connecting. AUX-port connection is not supported. Keep actual hardware movement within its usable calibrated travel.

Both models support absolute/relative integer movement and abort. Measured position is separate from target. PlaneWave moves farther than 50000 encoder counts use coarse slew, stop, then fine GOTO. 100 unchanged motion polls trigger a stop attempt. After uncertain motion, use abort to confirm the stopped position before another move.

PlaneWave exposes coordinate SYNC, `X_FOCUSER_FANS` and temperature. `FOCUSER_LIMITS` sets both local software endpoints (default 0–3799422); these limits constrain driver movement and are not programmed into the device. Without a temperature sensor the temperature is published IDLE.

Celestron exposes `X_FOCUSER_CALIBRATION.CALIBRATE`, with measured progress and read-only device limits. Calibration has a 180-second deadline and can be aborted. SYNC, fans and temperature are hidden for this model. Speed, reversal, backlash and automatic compensation are not implemented.

Disconnect attempts a hardware stop; a communication failure cannot confirm a physical halt.

## Testing

- 2026-10-09 20:01 3.0.0.23 linux arm64 PlaneWave EFA Arduino simulator (ESP32-S3) 11/11 OK
- 2026-10-09 20:03 3.0.0.23 linux arm64 Celestron Focus Motor Arduino simulator (ESP32-S3) 11/11 OK
- 2026-10-10 14:04 3.0.0.25 mac arm64 simulator 66/66 OK
