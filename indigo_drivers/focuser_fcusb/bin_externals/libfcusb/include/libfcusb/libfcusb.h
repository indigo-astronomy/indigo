/*
 * libfcusb library
 *
 * Copyright (c) 2013-2016 CloudMakers, s. r. o. All Rights Reserved.
 *
 * It is provided by CloudMakers "AS IS", without warranty of any kind.
 */

#ifndef libfcusb_h
#define libfcusb_h

#include <stdbool.h>
#include <pthread.h>
#include <libusb-1.0/libusb.h>
#include <hidapi/hidapi.h>

#define FCUSB_VID		0x134A

#define FCUSB_PID1	0x9023
#define FCUSB_PID2	0x9024
#define FCUSB_PID3	0x903f

typedef struct {
	hid_device *handle;
	unsigned char mask, power;
} libfcusb_device_context;


extern bool libfcusb_focuser(libusb_device *device, const char **name);
extern bool libfcusb_open(libusb_device *device, libfcusb_device_context **context);
extern bool libfcusb_set_led(libfcusb_device_context *device_context, bool state);
extern void libfcusb_close(libfcusb_device_context *context);

extern bool libfcusb_debug;
extern bool libfcusb_syslog;

extern const char *libfcusb_version;
extern const char *libfcusb_os;
extern const char *libfcusb_arch;

#endif /* libfcusb_h */
