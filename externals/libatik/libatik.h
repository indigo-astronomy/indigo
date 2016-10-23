/*
 * libatik library
 *
 * Copyright (c) 2013-2016 CloudMakers, s. r. o. All Rights Reserved.
 *
 * It is provided by CloudMakers "AS IS", without warranty of any kind.
 */

#ifndef libatik_h
#define libatik_h

#include <stdbool.h>
#include <pthread.h>

#define GUIDE_EAST             0x04     /* RA+ */
#define GUIDE_NORTH            0x01     /* DEC+ */
#define GUIDE_SOUTH            0x02     /* DEC- */
#define GUIDE_WEST             0x08     /* RA- */

typedef enum { ATIK_IC24 = 1, ATIK_SCI, ATIK_LF, ATIK_QUICKER } atik_type;

typedef struct {
	atik_type type;
	libusb_device_handle *handle;
	bool dark_mode;
	bool has_shutter;
	bool has_cooler;
	bool has_filter_wheel;
	double requested_temp;
	bool has_guider_port;
	bool has_8bit_mode;
	bool pc_cds;
	int precharge_offset;
	short mask;
	int width, height;
	double pixel_width, pixel_height;
	int max_bin_hor, max_bin_vert;
	int well_capacity;
	int min_power, max_power;
	pthread_mutex_t lock;
} atik_context;

extern bool atik_camera(libusb_device *device, atik_type *type, const char **name);
extern bool atik_open(libusb_device *device, atik_context **context);
extern bool atik_reset(atik_context *context);
extern bool atik_start_exposure(atik_context *context, bool dark);
extern bool atik_abort_exposure(atik_context *context);
extern bool atik_read_pixels(atik_context *context, double delay, int x, int y, int width, int height, int xbin, int ybin, unsigned short *image, int *image_width, int *image_height);
extern bool atik_set_cooler(atik_context *context, bool status, double temperature);
extern bool atik_check_cooler(atik_context *context, bool *status, double *power, double *temperature);
extern bool atik_set_filter_wheel(atik_context *context, int filter);
extern bool atik_check_filter_wheel(atik_context *context, int *filter);
extern bool atik_guide_relays(atik_context *device_context, unsigned short mask);
extern void atik_close(atik_context *context);

extern bool ic24_open(libusb_device *device, atik_context **context);
extern bool ic24_reset(atik_context *context);
extern bool ic24_start_exposure(atik_context *context, bool dark);
extern bool ic24_abort_exposure(atik_context *context);
extern bool ic24_read_pixels(atik_context *context, double delay, int x, int y, int width, int height, int xbin, int ybin, unsigned short *image, int *image_width, int *image_height);
extern bool ic24_set_cooler(atik_context *context, bool status, double temperature);
extern bool ic24_check_cooler(atik_context *context, bool *status, double *power, double *temperature);
extern bool ic24_set_filter_wheel(atik_context *context, int filter);
extern bool ic24_check_filter_wheel(atik_context *context, int *filter);
extern bool ic24_guide_relays(atik_context *device_context, unsigned short mask);
extern void ic24_close(atik_context *context);

extern bool sci_open(libusb_device *device, atik_context **device_context);
extern bool sci_reset(atik_context *context);
extern bool sci_start_exposure(atik_context *context);
extern bool sci_abort_exposure(atik_context *context);
extern bool sci_read_pixels(atik_context *context, double delay, int x, int y, int width, int height, int xbin, int ybin, unsigned short *image, int *image_width, int *image_height);
extern bool sci_set_cooler(atik_context *context, bool status, double temperature);
extern bool sci_check_cooler(atik_context *device_context, bool *status, double *power, double *temperature);
extern bool sci_guide_relays(atik_context *device_context, unsigned short mask);
extern void sci_close(atik_context *context);

extern bool quicker_open(libusb_device *device, atik_context **device_context);
extern bool quicker_start_exposure(atik_context *context);
extern bool quicker_abort_exposure(atik_context *context);
extern bool quicker_read_pixels(atik_context *context, double delay, int x, int y, int width, int height, int xbin, int ybin, unsigned short *image, int *image_width, int *image_height);
extern bool quicker_guide_relays(atik_context *device_context, unsigned short mask);
extern void quicker_close(atik_context *context);

extern bool lf_open(libusb_device *device, atik_context **device_context);
extern bool lf_reset(atik_context *context);
extern bool lf_start_exposure(atik_context *context);
extern bool lf_abort_exposure(atik_context *context);
extern bool lf_read_pixels(atik_context *context, double delay, int x, int y, int width, int height, int xbin, int ybin, unsigned short *image, int *image_width, int *image_height);
extern bool lf_set_cooler(atik_context *context, bool status, double temperature);
extern bool lf_check_cooler(atik_context *device_context, bool *status, double *power, double *temperature);
extern void lf_close(atik_context *context);

extern bool atik_debug;
extern bool atik_syslog;

extern const char *atik_version;
extern const char *atik_os;
extern const char *atik_arch;

#endif /* libatik_h */
