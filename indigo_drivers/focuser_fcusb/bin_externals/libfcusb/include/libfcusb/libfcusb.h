/*
 * libfcusb - interface library for Shoestring FCUSB motor controllers
 *
 * Copyright (c) 2016 CloudMakers, s. r. o. All Rights Reserved.
 *
 * Redistribution and use in binary forms are permitted provided that
 * the above copyright notice and this paragraph are duplicated in all
 * such forms and that any documentation, advertising materials, and
 * other materials related to such distribution and use acknowledge that
 * the software was developed by the  CloudMakers, s. r. o. The name of
 * the CloudMakers, s. r. o. may not be used to endorse or promote products
 * derived from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED 'AS IS' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef libfcusb_h
#define libfcusb_h

#include <stdbool.h>
#include <pthread.h>
#include <libusb-1.0/libusb.h>
#include <hidapi/hidapi.h>

#define FCUSB_VID	0x134A

#define FCUSB_PID1	0x9023
#define FCUSB_PID2	0x9024
#define FCUSB_PID3	0x903f

typedef struct {
	hid_device *handle;
	unsigned char mask, power;
} libfcusb_device_context;


extern bool libfcusb_focuser(libusb_device *device, const char **name);
extern bool libfcusb_open(libusb_device *device, libfcusb_device_context **context);
extern bool libfcusb_led_green(libfcusb_device_context *context);
extern bool libfcusb_led_red(libfcusb_device_context *context);
extern bool libfcusb_led_off(libfcusb_device_context *context);
extern bool libfcusb_set_power(libfcusb_device_context *context, unsigned power);
extern bool libfcusb_set_frequency(libfcusb_device_context *context, unsigned frequency);
extern bool libfcusb_move_out(libfcusb_device_context *context);
extern bool libfcusb_move_in(libfcusb_device_context *context);
extern bool libfcusb_stop(libfcusb_device_context *context);
extern void libfcusb_close(libfcusb_device_context *context);

extern void (*libfcusb_debug)(const char *message);

extern const char *libfcusb_version;
extern const char *libfcusb_os;
extern const char *libfcusb_arch;

#endif /* libfcusb_h */
