# LACERTA Motorfocus focuser driver

https://en.lacerta-optics.com/MFOC_LACERTA-Motorfocus---precision-stepper-motor-with#m

## Supported devices

* LACERTA MFOC
* LACERTA FMC

One focuser is present at startup; additional independent instances can be configured at runtime. No hot-plug support.

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_focuser_lacerta

## Status: Untested

## Notes

Select the serial port before connecting; the device communicates at 9600 baud.

Absolute and relative moves, coordinate SYNC, abort, reversal, backlash and maximum position are supported. Backlash is 0–255 steps. Maximum position is 300–65535 for firmware v1 and 300–250000 for v2/v3; minimum position is zero. Settings are loaded again on connection.

Temperature is read-only; when no sensor is connected it is published IDLE and the last valid value is kept. Speed, automatic mode and temperature compensation are not supported.

Abort reads the stopped position and ends the move there. Motion with no observed progress for 100 polls (nominally about 10 seconds) triggers a stop attempt and ALERT. If a stop cannot be confirmed, retry abort or reconnect before another move. Disconnect attempts to halt active motion; transport failure cannot confirm a physical stop.

## Testing

- 2026-10-10 02:23 3.0.0.10 mac arm64 simulator 50/50 OK
