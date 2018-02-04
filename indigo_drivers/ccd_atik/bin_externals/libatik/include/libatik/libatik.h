/*
 * libatik - interface library for ATIK cameras
 *
 * Copyright (c) 2016 CloudMakers, s. r. o. All Rights Reserved.
 *
 * Redistribution and use in binary form is permitted provided that
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

#ifndef libatik_h
#define libatik_h

#include <stdbool.h>
#include <pthread.h>

#include <libusb-1.0/libusb.h>
#include <hidapi/hidapi.h>

#define ATIK_VID1	0x20E7
#define ATIK_VID2 0x04b4

#define ATIK_GUIDE_EAST             0x04     /* RA+ */
#define ATIK_GUIDE_NORTH            0x01     /* DEC+ */
#define ATIK_GUIDE_SOUTH            0x02     /* DEC- */
#define ATIK_GUIDE_WEST             0x08     /* RA- */

typedef enum { ATIK_IC24 = 1, ATIK_SCI, ATIK_LF, ATIK_QUICKER } libatik_camera_type;

typedef struct {
	libatik_camera_type type;
	libusb_device_handle *handle;
	bool dark_mode;
	bool has_shutter;
	bool has_cooler;
	bool has_filter_wheel;
	bool has_guider_port;
	bool has_8bit_mode;
	bool pc_cds;
	int precharge_offset;
	int filter_count;
	short mask;
	int width, height;
	double pixel_width, pixel_height;
	int max_bin_hor, max_bin_vert;
	int well_capacity;
	int min_power, max_power;
	double min_exposure;
	pthread_mutex_t lock;
} libatik_device_context;

extern bool libatik_camera(libusb_device *device, libatik_camera_type *type, const char **name, bool *is_guider, bool *has_filterwheel);
extern bool libatik_open(libusb_device *device, libatik_device_context **context);
extern bool libatik_reset(libatik_device_context *context);
extern bool libatik_start_exposure(libatik_device_context *context, bool dark);
extern bool libatik_abort_exposure(libatik_device_context *context);
extern bool libatik_read_pixels(libatik_device_context *context, double delay, int x, int y, int width, int height, int xbin, int ybin, unsigned short *image, int *image_width, int *image_height);
extern bool libatik_set_cooler(libatik_device_context *context, bool status, double temperature);
extern bool libatik_check_cooler(libatik_device_context *context, bool *status, double *power, double *temperature);
extern bool libatik_set_filter_wheel(libatik_device_context *context, int filter);
extern bool libatik_check_filter_wheel(libatik_device_context *context, int *filter);
extern bool libatik_guide_relays(libatik_device_context *device_context, unsigned short mask);
extern void libatik_close(libatik_device_context *context);

extern bool ic24_open(libusb_device *device, libatik_device_context **context);
extern bool ic24_reset(libatik_device_context *context);
extern bool ic24_start_exposure(libatik_device_context *context, bool dark);
extern bool ic24_abort_exposure(libatik_device_context *context);
extern bool ic24_read_pixels(libatik_device_context *context, double delay, int x, int y, int width, int height, int xbin, int ybin, unsigned short *image, int *image_width, int *image_height);
extern bool ic24_set_cooler(libatik_device_context *context, bool status, double temperature);
extern bool ic24_check_cooler(libatik_device_context *context, bool *status, double *power, double *temperature);
extern bool ic24_set_filter_wheel(libatik_device_context *context, int filter);
extern bool ic24_check_filter_wheel(libatik_device_context *context, int *filter);
extern bool ic24_guide_relays(libatik_device_context *device_context, unsigned short mask);
extern void ic24_close(libatik_device_context *context);

extern bool sci_open(libusb_device *device, libatik_device_context **device_context);
extern bool sci_reset(libatik_device_context *context);
extern bool sci_start_exposure(libatik_device_context *context);
extern bool sci_abort_exposure(libatik_device_context *context);
extern bool sci_read_pixels(libatik_device_context *context, double delay, int x, int y, int width, int height, int xbin, int ybin, unsigned short *image, int *image_width, int *image_height);
extern bool sci_set_cooler(libatik_device_context *context, bool status, double temperature);
extern bool sci_check_cooler(libatik_device_context *device_context, bool *status, double *power, double *temperature);
extern bool sci_guide_relays(libatik_device_context *device_context, unsigned short mask);
extern void sci_close(libatik_device_context *context);

extern bool quicker_open(libusb_device *device, libatik_device_context **device_context);
extern bool quicker_start_exposure(libatik_device_context *context);
extern bool quicker_abort_exposure(libatik_device_context *context);
extern bool quicker_read_pixels(libatik_device_context *context, double delay, int x, int y, int width, int height, int xbin, int ybin, unsigned short *image, int *image_width, int *image_height);
extern bool quicker_guide_relays(libatik_device_context *device_context, unsigned short mask);
extern void quicker_close(libatik_device_context *context);

extern bool lf_open(libusb_device *device, libatik_device_context **device_context);
extern bool lf_reset(libatik_device_context *context);
extern bool lf_start_exposure(libatik_device_context *context);
extern bool lf_abort_exposure(libatik_device_context *context);
extern bool lf_read_pixels(libatik_device_context *context, double delay, int x, int y, int width, int height, int xbin, int ybin, unsigned short *image, int *image_width, int *image_height);
extern bool lf_set_cooler(libatik_device_context *context, bool status, double temperature);
extern bool lf_check_cooler(libatik_device_context *device_context, bool *status, double *power, double *temperature);
extern void lf_close(libatik_device_context *context);

extern bool libatik_wheel_query(hid_device *handle, int *slot_count, int *current_slot);
extern bool libatik_wheel_set(hid_device *handle, int slot);

extern bool libatik_debug_level;
extern bool libatik_use_syslog;
extern void (*atik_log)(const char *format, ...);

extern const char *libatik_version;
extern const char *libatik_os;
extern const char *libatik_arch;

#endif /* libatik_h */
