# libcamera CCD driver

https://libcamera.org

## Supported devices

Every camera handled by libcamera, in particular the Raspberry Pi CSI cameras (Camera Module 1, 2, 3, HQ, Global Shutter) and third-party CSI sensors with a kernel driver and a libcamera pipeline handler, on Raspberry Pi and other boards libcamera supports.

Each camera found at driver start is one CCD device named after its sensor model, e.g. `OV5647` or `IMX477`; a second camera with the same sensor gets a ` #1` suffix. CSI cameras are not hot-pluggable, so connect them before the driver starts.

Third-party sensors usually need a `dtoverlay=<sensor>` line (and `camera_auto_detect=0`) in `/boot/firmware/config.txt`.

## Features

- RAW sensor data only, delivered as 16-bit samples with the native sensor values (no ISP processing); colour sensors add the `BAYERPAT` FITS keyword.
- Sensor modes (full resolution and binned modes) as `CCD_MODE`.
- Exposure time and analogue gain within the limits reported by the camera.
- Software subframe (`CCD_FRAME`), aligned to the Bayer cell on colour sensors.
- Streaming.

## Supported platforms

Linux only. The driver needs libcamera at run time; it is built with libcamera support when `pkg-config libcamera` finds the development files (`libcamera-dev`), otherwise it builds without it and reports no cameras.

The driver is independent of `indigo_ccd_rpi`; do not run both at the same time, a camera can be used by one application only.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_ccd_libcamera

## Status: Under development

Driver is developed and tested with:
* Raspberry Pi 3 B+ with Raspberry Pi Camera Module 1 (OV5647), libcamera 0.7.2

## Testing

- 2026-10-25 00:08 3.0.0.1 mac arm64 fake SDK 15/15 OK
- 2026-10-25 00:08 3.0.0.1 linux arm64 fake SDK 15/15 OK
- 2026-10-25 08:23 3.0.0.1 linux arm64 ov5647 10/10 OK
