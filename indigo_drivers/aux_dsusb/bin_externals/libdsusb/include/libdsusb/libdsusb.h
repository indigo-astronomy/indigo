/*
 * libdsusb - interface library for Shoestring DSUSB interface
 *
 * Copyright (c) 2018 CloudMakers, s. r. o. All Rights Reserved.
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

#ifndef libdsusb_h
#define libdsusb_h

#include <stdbool.h>
#include <pthread.h>
#include <libusb-1.0/libusb.h>
#include <hidapi/hidapi.h>

#define DSUSB_VID	  0x134A
#define DSUSB_PID1	0x9021
#define DSUSB_PID2  0x9026

typedef struct {
	hid_device *handle;
	unsigned char mask;
} libdsusb_device_context;


extern bool libdsusb_shutter(libusb_device *device, const char **name);
extern bool libdsusb_open(libusb_device *device, libdsusb_device_context **context);
extern bool libdsusb_led_green(libdsusb_device_context *context);
extern bool libdsusb_led_red(libdsusb_device_context *context);
extern bool libdsusb_led_off(libdsusb_device_context *context);
extern bool libdsusb_focus(libdsusb_device_context *context);
extern bool libdsusb_start(libdsusb_device_context *context);
extern bool libdsusb_stop(libdsusb_device_context *context);
extern void libdsusb_close(libdsusb_device_context *context);

extern void (*libdsusb_debug)(const char *message);

extern const char *libdsusb_version;
extern const char *libdsusb_os;
extern const char *libdsusb_arch;

#endif /* libdsusb_h */
