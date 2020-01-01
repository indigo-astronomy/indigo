# ZWO EAF focuser driver

https://astronomy-imaging-camera.com

## Supported devices

All ZWO EAF focusers.

This driver supports hot-plug (multiple devices).

## Supported platforms

This driver depends on 3rd party library and is supported on Linux (Intel 32/64 bit and ARM v6+) and MacOS.

## License

INDIGO Astronomy open-source license (3rd party library is closed source).

## Use

`indigo_server indigo_focuser_asi`

## Status: Stable

Driver is developed and tested with:
* ASI EAF

## NOTE! MacOS users only.

If your Mac does not recognize the attached EAF and you see message like this:

`22:03:49.250247 indigo_server: indigo_focuser_asi[process_plug_event:707, 0x70000276c000]: No plugged device found.`

Please follow these steps:

1. Connect the power to EAF.
2. Connect the EAF to your Mac via USB cable.
3. Power OFF and then power ON your Mac while the focuser is connected.
4. The EAF should be discovered -> `indigo_focuser_asi: 'EAF #0' attached`

This seems to be a MacOS issue. Looks like this procedure fixes some device permissions.
This procedure should be followed only once, then your EAF should be auto discovered.
