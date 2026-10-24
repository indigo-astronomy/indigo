# PrimaLuce Lab GIOTTO flat panel driver

https://www.primalucelab.com/

## Supported devices
* PrimaLuce Lab GIOTTO flat panel

Single device is present on the first startup (no hot-plug support). The panel is connected over USB (115200 baud) or over its WiFi with an `http://host` port.

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_aux_giotto

## Status: Stable

* Developed and tested with a GIOTTO with application firmware 1.03.09

## Notes:
* The light intensity is the panel's own brightness level, from 0 to the maximum calibrated in the factory (114 on the tested panel), not a percentage. It can be set while the light is off.
* Recalling a preset sets its brightness and leaves the light as it is.
* A preset takes a brightness only after it has a name; clearing the name deletes it.
* The light is switched off when the driver disconnects; the panel keeps its brightness.
* The WiFi mode (off, access point, station) and both sets of credentials can be set from the driver; a change between station mode and the access point restarts the panel.

## Testing

- 2026-10-24 14:31 3.0.0.1 linux x64 GIOTTO 8/8 OK
- 2026-10-25 00:31 3.0.0.5 linux x64 simulator 30/30 OK
