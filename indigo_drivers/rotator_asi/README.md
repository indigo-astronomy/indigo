# ZWO CAA rotator driver

https://astronomy-imaging-camera.com

## Supported devices

All ZWO CAA rotators.

This driver supports hot-plug (multiple devices).

## Supported platforms

This driver depends on 3rd party library and is supported on Linux (Intel 32/64 bit and ARM v6+) and MacOS.

## License

INDIGO Astronomy open-source license (3rd party library is closed source).

## Use

`indigo_server indigo_rotator_asi`

## Motion and settings

Positions and relative moves use degrees. Targets must stay within the confirmed device limits; relative targets outside those limits are rejected without wrapping or clamping. The minimum limit is fixed at zero, and maximum-limit changes are read back from the device.

Motion remains busy while either the motor or hand controller reports movement. Abort cannot stop movement commanded by the hand controller; release the controller and retry abort.

The custom device-name suffix accepts up to eight bytes, including an empty suffix to clear it. Name changes take effect after replug. Failed suffix writes restore the last confirmed suffix.

## Status: Stable

Driver is developed and tested with:

* ZWO Camera Angle Aadjuster

