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
