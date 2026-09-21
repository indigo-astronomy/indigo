# UVC (USB Video Class) CCD driver


## Supported devices

* UVC cameras

This driver supports hot-plug (multiple devices).

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_ccd_uvc

## Status: Stable

Tested with SVBONY SV205 and TIS DMK21

## Notes

UVC driver doesn't work on macOS Monterey or later due to presence of system kernel extension com.apple.UVCService.
The only known workaround is to run the application containing UVC driver as root to allow libusb_set_auto_detach_kernel_driver()
call to detach kernel driver and subsequent execution of the application execution will work until the camera is unplugged.

Unplugging a connected camera is not safe: the process can abort while the driver closes the device
from its unplug handler. libusb fails the assertion `pthread_mutex_destroy(mutex) == 0` inside
libusb_close(), because the lock of the device handle carries an unbalanced use count, and the
crash reproduces on every attempt with an SVBONY SV205 on Linux arm64 with libusb 1.0.26. It is
older than the 3.0.0.27 bulk endpoint fix and the cause is not understood yet, so disconnect a
camera in the client before unplugging it.

## Testing

2026-09-21 14:12 3.0.0.26 mac arm64 fake SDK 21/21 OK
2026-09-21 20:24 3.0.0.27 linux arm64 SVBONY SV205 11/11 OK
2026-09-21 20:26 3.0.0.27 linux arm64 Creative Live! Cam Sync HD VF0770 11/11 OK
