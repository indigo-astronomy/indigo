# PrimaLuce Lab ALTO cover driver

https://www.primalucelab.com/

## Supported devices
* PrimaLuce Lab ALTO motorized cover (ALTO1 tested)

Single device is present on the first startup (no hot-plug support). The cover is connected over USB (115200 baud) or over its WiFi with an `http://host` port.

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_aux_alto

## Status: Stable

* Developed and tested with an ALTO1 with application firmware 1.03.11

## Notes:
* The cover position is a percentage: 0 is closed, 100 fully open. Open and close move to these ends; any position between them can be set as well.
* An abort stops the cover with a short deceleration.
* A disconnect stops a moving cover.
* A preset takes a position only after it has a name; recalling a preset moves the cover there.
* The calibration follows the sequence of the PrimaLuce INDI driver and has not been verified on hardware yet: start it, drive the cover fully closed with the calibration motion controls, stop and store, then drive it fully open, stop and store.
* The WiFi mode (off, access point, station) and both sets of credentials can be set from the driver; a change between station mode and the access point restarts the cover.

## Testing

- 2026-10-24 16:49 3.0.0.1 linux x64 ALTO1 9/9 OK
- 2026-10-25 02:41 3.0.0.6 linux x64 simulator 33/33 OK
