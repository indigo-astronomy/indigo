# iOptron iEAF / iAFS focuser driver

https://www.ioptron.com/product-p/8453.htm

## Supported devices

* iOptron iEAF focuser
* iOptron iAFS focuser

The public device name is `iOptron iEAF`; INFO reports the detected model and firmware. One device is present at startup; additional independent serial instances can be configured. No hot-plug support.

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_focuser_ioptron

## Status: Untested

## Notes

Select the serial port before connecting; the device communicates at 115200 baud. [Manufacturer iEAF manual](https://www.ioptron.com/v/Manuals/8453_iEAF_Manual.pdf) and [iAFS product page](https://www.ioptron.com/product-p/fa20.htm) describe the hardware.

Absolute and relative moves span 0–99999 steps. Relative inward decreases the coordinate and outward increases it; reversal toggles the hardware direction mapping. `X_FOCUSER_ZERO_SYNC.SYNC` synchronizes to zero without a move; it replaces the legacy `ZERO_SYNC.SYNC`, so update client scripts accordingly. Arbitrary coordinate SYNC, speed, backlash, configurable limits and automatic compensation are not exposed.

Movement requests and zero/reverse are rejected during motion. 100 unchanged moving polls (nominally 10 seconds) trigger a stop attempt and ALERT. Disconnect attempts a stop; failed communication cannot confirm a physical halt.

## Testing

- 2026-09-27 00:32 3.0.0.10 linux x64 simulator 42/42 OK
- 2026-10-10 03:50 3.0.0.12 mac arm64 simulator 47/47 OK
