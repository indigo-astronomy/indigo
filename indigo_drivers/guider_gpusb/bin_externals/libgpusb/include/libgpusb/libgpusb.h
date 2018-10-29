/*
 * libgpusb - interface library for Shoestring GPUSB guiders
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

#ifndef libgpusb_h
#define libgpusb_h

#include <stdbool.h>
#include <pthread.h>
#include <libusb-1.0/libusb.h>
#include <hidapi/hidapi.h>

#define GPUSB_VID	        0x134A
#define GPUSB_PID	        0x9020

#define GPUSB_RA_EAST		  0x01
#define GPUSB_RA_WEST     0x02
#define GPUSB_DEC_SOUTH   0x04
#define GPUSB_DEC_NORTH   0x08

typedef struct {
	hid_device *handle;
	unsigned char mask, power;
} libgpusb_device_context;


extern bool libgpusb_guider(libusb_device *device, const char **name);
extern bool libgpusb_open(libusb_device *device, libgpusb_device_context **context);
extern bool libgpusb_led_green(libgpusb_device_context *context);
extern bool libgpusb_led_red(libgpusb_device_context *context);
extern bool libgpusb_led_off(libgpusb_device_context *context);
extern bool libgpusb_set(libgpusb_device_context *context, int mask);
extern void libgpusb_close(libgpusb_device_context *context);

extern void (*libgpusb_debug)(const char *message);

extern const char *libgpusb_version;
extern const char *libgpusb_os;
extern const char *libgpusb_arch;

#endif /* libgpusb_h */
