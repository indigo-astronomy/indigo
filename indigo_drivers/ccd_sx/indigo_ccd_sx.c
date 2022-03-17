// Copyright (c) 2016 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// version history
// 2.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO StarlighXpress CCD driver
 \file indigo_ccd_sx.c
 */

#define DRIVER_VERSION 0x000C
#define DRIVER_NAME "indigo_ccd_sx"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>

#if defined(INDIGO_MACOS)
#include <libusb-1.0/libusb.h>
#elif defined(INDIGO_FREEBSD)
#include <libusb.h>
#else
#include <libusb-1.0/libusb.h>
#endif

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_usb_utils.h>

#include "indigo_ccd_sx.h"

// -------------------------------------------------------------------------------- SX USB interface implementation

#define REQ_TYPE                      0
#define REQ                           1
#define REQ_VALUE_L                   2
#define REQ_VALUE_H                   3
#define REQ_INDEX_L                   4
#define REQ_INDEX_H                   5
#define REQ_LENGTH_L                  6
#define REQ_LENGTH_H                  7
#define REQ_DATA                      8

#define REQ_DIR(r)                    ((r)&(1<<7))
#define REQ_DATAOUT                   0x00
#define REQ_DATAIN                    0x80
#define REQ_KIND(r)                   ((r)&(3<<5))
#define REQ_VENDOR                    (2<<5)
#define REQ_STD                       0
#define REQ_RECIP(r)                  ((r)&31)
#define REQ_DEVICE                    0x00
#define REQ_IFACE                     0x01
#define REQ_ENDPOINT                  0x02

#define CCD_GET_FIRMWARE_VERSION      255
#define CCD_ECHO                      0
#define CCD_CLEAR_PIXELS              1
#define CCD_READ_PIXELS_DELAYED       2
#define CCD_READ_PIXELS               3
#define CCD_SET_TIMER                 4
#define CCD_GET_TIMER                 5
#define CCD_RESET                     6
#define CCD_SET_CCD                   7
#define CCD_GET_CCD                   8
#define CCD_SET_STAR2K                9
#define CCD_WRITE_SERIAL_PORT         10
#define CCD_READ_SERIAL_PORT          11
#define CCD_SET_SERIAL                12
#define CCD_GET_SERIAL                13
#define CCD_CAMERA_MODEL              14
#define CCD_LOAD_EEPROM               15
#define CCD_SET_A2D                   16
#define CCD_RED_A2D                   17
#define CCD_READ_PIXELS_GATED         18
#define CCD_BUILD_NUMBER              19
#define CCD_COOLER_CONTROL            30
#define CCD_COOLER                    30
#define CCD_COOLER_TEMPERATURE        31
#define CCD_SHUTTER_CONTROL           32
#define CCD_SHUTTER                   32
#define CCD_READ_I2CPORT              33
#define CCD_FLOOD_LED              		43

#define CAPS_STAR2K                   0x01
#define CAPS_COMPRESS                 0x02
#define CAPS_EEPROM                   0x04
#define CAPS_GUIDER                   0x08
#define CAPS_COOLER                   0x10
#define CAPS_SHUTTER                  0x20

#define FLAGS_FIELD_ODD               0x01
#define FLAGS_FIELD_EVEN              0x02
#define FLAGS_FIELD_BOTH              (FLAGS_FIELD_EVEN|FLAGS_FIELD_ODD)
#define FLAGS_FIELD_MASK              FLAGS_FIELD_BOTH
#define FLAGS_SPARE2                  0x04
#define FLAGS_NOWIPE_FRAME            0x08
#define FLAGS_SPARE4                  0x10
#define FLAGS_TDI                     0x20
#define FLAGS_NOCLEAR_FRAME           0x40
#define FLAGS_NOCLEAR_REGISTER        0x80

#define FLAGS_SPARE8                  0x01
#define FLAGS_SPARE9                  0x02
#define FLAGS_SPARE10                 0x04
#define FLAGS_SPARE11                 0x08
#define FLAGS_SPARE12                 0x10
#define FLAGS_SHUTTER_MANUAL          0x20
#define FLAGS_SHUTTER_OPEN            0x40
#define FLAGS_SHUTTER_CLOSE           0x80

#define BULK_IN                       0x0082
#define BULK_OUT                      0x0001

#define SX_GUIDE_EAST                 0x08     /* RA+ */
#define SX_GUIDE_NORTH                0x04     /* DEC+ */
#define SX_GUIDE_SOUTH                0x02     /* DEC- */
#define SX_GUIDE_WEST                 0x01     /* RA- */

#define BULK_COMMAND_TIMEOUT          2000
#define BULK_DATA_TIMEOUT             10000

#define CHUNK_SIZE                    (4*1024*1024)

#define PRIVATE_DATA        ((sx_private_data *)device->private_data)

// gp_bits is used as boolean
#define is_connected                   gp_bits

typedef struct {
	libusb_device *dev;
	libusb_device_handle *handle;
	int device_count;
	indigo_timer *exposure_timer, *temperture_timer, *guider_timer;
	unsigned char setup_data[22];
	int model;
	bool is_interlaced;
	bool is_color;
	bool is_icx453;
	bool has_flood_led;
	unsigned short ccd_width;
	unsigned short ccd_height;
	double pix_width;
	double pix_height;
	unsigned short bits_per_pixel;
	unsigned short color_matrix;
	char extra_caps;
	double exposure;
	unsigned short frame_left;
	unsigned short frame_top;
	unsigned short frame_width;
	unsigned short frame_height;
	unsigned short horizontal_bin;
	unsigned short vertical_bin;
	double target_temperature, current_temperature;
	unsigned short relay_mask;
	unsigned char *buffer;
	unsigned char *odd, *even;
	pthread_mutex_t usb_mutex;
	bool can_check_temperature;
} sx_private_data;

static bool sx_open(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	int rc = 0;
	libusb_device *dev = PRIVATE_DATA->dev;
	rc = libusb_open(dev, &PRIVATE_DATA->handle);
	libusb_device_handle *handle = PRIVATE_DATA->handle;
	unsigned char *setup_data = PRIVATE_DATA->setup_data;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_open -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
	if (rc >= 0) {
		if (libusb_kernel_driver_active(handle, 0) == 1) {
			rc = libusb_detach_kernel_driver(handle, 0);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_detach_kernel_driver -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
		}
		if (rc >= 0) {
			struct libusb_config_descriptor *config;
			rc = libusb_get_config_descriptor(dev, 0, &config);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_get_config_descriptor -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
			if (rc >= 0) {
				int interface = config->interface->altsetting->bInterfaceNumber;
				rc = libusb_claim_interface(handle, interface);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_claim_interface(%d) -> %s", interface, rc < 0 ? libusb_error_name(rc) : "OK");
			}
		}
	}
	int transferred;
	if (rc >= 0) { // reset
		setup_data[REQ_TYPE ] = REQ_VENDOR | REQ_DATAOUT;
		setup_data[REQ ] = CCD_RESET;
		setup_data[REQ_VALUE_L ] = 0;
		setup_data[REQ_VALUE_H ] = 0;
		setup_data[REQ_INDEX_L ] = 0;
		setup_data[REQ_INDEX_H ] = 0;
		setup_data[REQ_LENGTH_L] = 0;
		setup_data[REQ_LENGTH_H] = 0;
		rc = libusb_bulk_transfer(handle, BULK_OUT, setup_data, REQ_DATA, &transferred, BULK_COMMAND_TIMEOUT);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_control_transfer -> %lu bytes %s", transferred, rc < 0 ? libusb_error_name(rc) : "OK");
		indigo_usleep(1000);
	}
	if (rc >= 0) { // read camera model
		setup_data[REQ_TYPE ] = REQ_VENDOR | REQ_DATAIN;
		setup_data[REQ ] = CCD_CAMERA_MODEL;
		setup_data[REQ_VALUE_L ] = 0;
		setup_data[REQ_VALUE_H ] = 0;
		setup_data[REQ_INDEX_L ] = 0;
		setup_data[REQ_INDEX_H ] = 0;
		setup_data[REQ_LENGTH_L] = 2;
		setup_data[REQ_LENGTH_H] = 0;
		rc = libusb_bulk_transfer(handle, BULK_OUT, setup_data, REQ_DATA, &transferred, BULK_COMMAND_TIMEOUT);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_control_transfer -> %lu bytes %s", transferred, rc < 0 ? libusb_error_name(rc) : "OK");
		if (rc >=0 && transferred == REQ_DATA) {
			rc = libusb_bulk_transfer(handle, BULK_IN, setup_data, 2, &transferred, BULK_COMMAND_TIMEOUT);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_control_transfer -> %lu bytes %s", transferred, rc < 0 ? libusb_error_name(rc) : "OK");
			if (rc >=0 && transferred == 2) {
				int result=setup_data[0] | (setup_data[1] << 8);
				PRIVATE_DATA->model = result & 0x1F;
				PRIVATE_DATA->is_color = result > 0x50;
				PRIVATE_DATA->is_interlaced = result & 0x40;
				if (result == 0x84)
					PRIVATE_DATA->is_interlaced = true;
				if (PRIVATE_DATA->model == 0x16 || PRIVATE_DATA->model == 0x17 || PRIVATE_DATA->model == 0x18 || PRIVATE_DATA->model == 0x19)
					PRIVATE_DATA->is_interlaced =  false;
				PRIVATE_DATA->is_icx453 = result == 0x59;
				PRIVATE_DATA->has_flood_led = result == 0x21 || result == 0x25 || result == 0xA5 || result == 0x2A || result == 0xAA;
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s %s model %d\n", PRIVATE_DATA->is_interlaced ? "INTERLACED" : "NON-INTERLACED", PRIVATE_DATA->is_color ? "COLOR" : "MONO", PRIVATE_DATA->model);
			}
		}
	}
	if (rc >= 0) { // read camera params
		setup_data[REQ_TYPE ] = REQ_VENDOR | REQ_DATAIN;
		setup_data[REQ ] = CCD_GET_CCD;
		setup_data[REQ_VALUE_L ] = 0;
		setup_data[REQ_VALUE_H ] = 0;
		setup_data[REQ_INDEX_L ] = 0;
		setup_data[REQ_INDEX_H ] = 0;
		setup_data[REQ_LENGTH_L] = 17;
		setup_data[REQ_LENGTH_H] = 0;
		rc = libusb_bulk_transfer(handle, BULK_OUT, setup_data, REQ_DATA, &transferred, BULK_COMMAND_TIMEOUT);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_control_transfer -> %lu bytes %s", transferred, rc < 0 ? libusb_error_name(rc) : "OK");
		if (rc >=0 && transferred == REQ_DATA) {
			rc = libusb_bulk_transfer(handle, BULK_IN, setup_data, 17, &transferred, BULK_COMMAND_TIMEOUT);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_control_transfer -> %lu bytes %s", transferred, rc < 0 ? libusb_error_name(rc) : "OK");
			if (rc >=0 && transferred == 17) {
				PRIVATE_DATA->ccd_width = setup_data[2] | (setup_data[3] << 8);
				PRIVATE_DATA->ccd_height = setup_data[6] | (setup_data[7] << 8);
				PRIVATE_DATA->pix_width = ((setup_data[8] | (setup_data[9] << 8)) / 256.0);
				PRIVATE_DATA->pix_height = ((setup_data[10] | (setup_data[11] << 8)) / 256.0);
				PRIVATE_DATA->bits_per_pixel = setup_data[14];
				PRIVATE_DATA->color_matrix = setup_data[12] | (setup_data[13] << 8);
				PRIVATE_DATA->extra_caps = setup_data[16];
				if (PRIVATE_DATA->is_interlaced) {
					PRIVATE_DATA->ccd_height *= 2;
					PRIVATE_DATA->pix_height /= 2;
				}
				PRIVATE_DATA->buffer = indigo_alloc_blob_buffer(2 * PRIVATE_DATA->ccd_width * PRIVATE_DATA->ccd_height + FITS_HEADER_SIZE + 512);
				assert(PRIVATE_DATA->buffer != NULL);
				if (PRIVATE_DATA->is_interlaced) {
					PRIVATE_DATA->even = indigo_safe_malloc(PRIVATE_DATA->ccd_width * PRIVATE_DATA->ccd_height + 512);
					PRIVATE_DATA->odd = indigo_safe_malloc(PRIVATE_DATA->ccd_width * PRIVATE_DATA->ccd_height + 512);
				} else if (PRIVATE_DATA->is_icx453) {
					PRIVATE_DATA->even = indigo_safe_malloc(2 * PRIVATE_DATA->ccd_width * PRIVATE_DATA->ccd_height + 512);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "sxGetCameraParams: is_icx453 buffer %d bytes", 2 * PRIVATE_DATA->ccd_width * PRIVATE_DATA->ccd_height);
				}
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "sxGetCameraParams: chip size: %d x %d, pixel size: %4.2f x %4.2f, matrix type: %x", PRIVATE_DATA->ccd_width, PRIVATE_DATA->ccd_height, PRIVATE_DATA->pix_width, PRIVATE_DATA->pix_height, PRIVATE_DATA->color_matrix);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "sxGetCameraParams: capabilities:%s%s%s%s", (PRIVATE_DATA->extra_caps & CAPS_GUIDER ? " GUIDER" : ""), (PRIVATE_DATA->extra_caps & CAPS_STAR2K ? " STAR2K" : ""), (PRIVATE_DATA->extra_caps & CAPS_COOLER ? " COOLER" : ""), (PRIVATE_DATA->extra_caps & CAPS_SHUTTER ? " SHUTTER" : ""));
			}
		}
	}
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	return rc >= 0;
}

static bool sx_start_exposure(indigo_device *device, double exposure, bool dark, int frame_left, int frame_top, int frame_width, int frame_height, int horizontal_bin, int vertical_bin) {
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	libusb_device_handle *handle = PRIVATE_DATA->handle;
	unsigned char *setup_data = PRIVATE_DATA->setup_data;
	int rc = 0;
	int transferred;
	if (exposure < 1) {
		int milis = (int)round(1000 * exposure);
		setup_data[REQ_TYPE ] = REQ_VENDOR | REQ_DATAOUT;
		setup_data[REQ ] = CCD_READ_PIXELS_DELAYED;
		setup_data[REQ_VALUE_L ] = FLAGS_FIELD_BOTH;
		setup_data[REQ_VALUE_H ] = 0;
		setup_data[REQ_INDEX_L ] = 0;
		setup_data[REQ_INDEX_H ] = 0;
		setup_data[REQ_LENGTH_L] = 10;
		setup_data[REQ_LENGTH_H] = 0;
		setup_data[REQ_DATA + 0] = frame_left & 0xFF;
		setup_data[REQ_DATA + 1] = frame_left >> 8;
		setup_data[REQ_DATA + 2] = frame_top & 0xFF;
		setup_data[REQ_DATA + 3] = frame_top >> 8;
		setup_data[REQ_DATA + 4] = frame_width & 0xFF;
		setup_data[REQ_DATA + 5] = frame_width >> 8;
		setup_data[REQ_DATA + 6] = frame_height & 0xFF;
		setup_data[REQ_DATA + 7] = frame_height >> 8;
		setup_data[REQ_DATA + 8] = horizontal_bin;
		setup_data[REQ_DATA + 9] = vertical_bin;
		setup_data[REQ_DATA + 10] = milis & 0xFF;
		setup_data[REQ_DATA + 11] = (milis>>8) & 0xFF;
		setup_data[REQ_DATA + 12] = (milis>>16) & 0xFF;
		setup_data[REQ_DATA + 13] = (milis>>24) & 0xFF;
		if (PRIVATE_DATA->extra_caps & CAPS_SHUTTER)
			setup_data[REQ_VALUE_H] = dark ? FLAGS_SHUTTER_CLOSE : FLAGS_SHUTTER_OPEN;
		if (PRIVATE_DATA->is_interlaced) {
			if (vertical_bin > 1) {
				setup_data[REQ_DATA + 2] = (frame_top/2) & 0xFF;
				setup_data[REQ_DATA + 3] = (frame_top/2) >> 8;
				setup_data[REQ_DATA + 6] = (frame_height/2) & 0xFF;
				setup_data[REQ_DATA + 7] = (frame_height/2) >> 8;
				setup_data[REQ_DATA + 9] = vertical_bin/2;
				rc = libusb_bulk_transfer(handle, BULK_OUT, setup_data, REQ_DATA + 14, &transferred, BULK_COMMAND_TIMEOUT);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer -> %lu bytes %s", transferred, rc < 0 ? libusb_error_name(rc) : "OK");
			} else {
				setup_data[REQ_VALUE_L ] = FLAGS_FIELD_EVEN | FLAGS_SPARE2;
				setup_data[REQ_DATA + 2] = (frame_top/2) & 0xFF;
				setup_data[REQ_DATA + 3] = (frame_top/2) >> 8;
				setup_data[REQ_DATA + 6] = (frame_height/2) & 0xFF;
				setup_data[REQ_DATA + 7] = (frame_height/2) >> 8;
				setup_data[REQ_DATA + 9] = 1;
				rc = libusb_bulk_transfer(handle, BULK_OUT, setup_data, REQ_DATA + 14, &transferred, BULK_COMMAND_TIMEOUT);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer -> %lu bytes %s", transferred, rc < 0 ? libusb_error_name(rc) : "OK");
			}
		} else {
			if (PRIVATE_DATA->is_icx453) {
				setup_data[REQ_DATA + 0] = (frame_left * 2) & 0xFF;
				setup_data[REQ_DATA + 1] = (frame_left * 2) >> 8;
				setup_data[REQ_DATA + 2] = (frame_top / 2) & 0xFF;
				setup_data[REQ_DATA + 3] = (frame_top / 2) >> 8;
				setup_data[REQ_DATA + 4] = (frame_width * 2) & 0xFF;
				setup_data[REQ_DATA + 5] = (frame_width * 2) >> 8;
				setup_data[REQ_DATA + 6] = (frame_height / 2) & 0xFF;
				setup_data[REQ_DATA + 7] = (frame_height / 2) >> 8;
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "sx_start_exposure: is_icx453 setup");
			}
			rc = libusb_bulk_transfer(handle, BULK_OUT, setup_data, REQ_DATA + 14, &transferred, BULK_COMMAND_TIMEOUT);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer -> %lu bytes %s", transferred, rc < 0 ? libusb_error_name(rc) : "OK");
		}
	} else {
		setup_data[REQ_TYPE ] = REQ_VENDOR | REQ_DATAOUT;
		setup_data[REQ ] = CCD_CLEAR_PIXELS;
		setup_data[REQ_VALUE_L ] = FLAGS_FIELD_BOTH;
		setup_data[REQ_VALUE_H ] = 0;
		setup_data[REQ_INDEX_L ] = 0;
		setup_data[REQ_INDEX_H ] = 0;
		setup_data[REQ_LENGTH_L] = 0;
		setup_data[REQ_LENGTH_H] = 0;
		rc = libusb_bulk_transfer(handle, BULK_OUT, setup_data, REQ_DATA, &transferred, BULK_COMMAND_TIMEOUT);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer -> %lu bytes %s", transferred, rc < 0 ? libusb_error_name(rc) : "OK");
	}
	PRIVATE_DATA->frame_left = frame_left;
	PRIVATE_DATA->frame_top = frame_top;
	PRIVATE_DATA->frame_width = frame_width;
	PRIVATE_DATA->frame_height = frame_height;
	PRIVATE_DATA->horizontal_bin = horizontal_bin;
	PRIVATE_DATA->vertical_bin = vertical_bin;
	PRIVATE_DATA->exposure = exposure;
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	return rc >= 0;
}

static bool sx_clear_regs(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	libusb_device_handle *handle = PRIVATE_DATA->handle;
	unsigned char *setup_data = PRIVATE_DATA->setup_data;
	int rc = 0;
	int transferred;
	if (rc >= 0) {
		setup_data[REQ_TYPE ] = REQ_VENDOR | REQ_DATAOUT;
		setup_data[REQ ] = CCD_CLEAR_PIXELS;
		setup_data[REQ_VALUE_L ] = FLAGS_NOWIPE_FRAME;
		setup_data[REQ_VALUE_H ] = 0;
		setup_data[REQ_INDEX_L ] = 0;
		setup_data[REQ_INDEX_H ] = 0;
		setup_data[REQ_LENGTH_L] = 0;
		setup_data[REQ_LENGTH_H] = 0;
		rc = libusb_bulk_transfer(handle, BULK_OUT, setup_data, REQ_DATA, &transferred, BULK_COMMAND_TIMEOUT);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer -> %lu bytes %s", transferred, rc < 0 ? libusb_error_name(rc) : "OK");
	}
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	return rc >= 0;
}

static bool sx_download_pixels(indigo_device *device, unsigned char *pixels, unsigned long count) {
	libusb_device_handle *handle = PRIVATE_DATA->handle;
	int transferred;
	unsigned long read = 0;
	int rc=0;
	while (read < count && rc >= 0) {
		int size = (int)(count - read);
		if (size > CHUNK_SIZE)
			size = CHUNK_SIZE;
		rc = libusb_bulk_transfer(handle, BULK_IN, pixels + read, size, &transferred, BULK_DATA_TIMEOUT);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer -> %lu bytes %s", transferred, rc < 0 ? libusb_error_name(rc) : "OK");
		if (transferred >= 0) {
			read += transferred;
		}
	}
	return rc >= 0;
}

static bool sx_read_pixels(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	libusb_device_handle *handle = PRIVATE_DATA->handle;
	unsigned char *setup_data = PRIVATE_DATA->setup_data;
	int rc = 0;
	int transferred;
	int frame_left = PRIVATE_DATA->frame_left;
	int frame_top = PRIVATE_DATA->frame_top;
	int frame_width = PRIVATE_DATA->frame_width;
	int frame_height = PRIVATE_DATA->frame_height;
	int horizontal_bin = PRIVATE_DATA->horizontal_bin;
	int vertical_bin = PRIVATE_DATA->vertical_bin;
	int size = (frame_width/horizontal_bin)*(frame_height/vertical_bin);
	if (PRIVATE_DATA->is_interlaced) {
		if (vertical_bin > 1) {
			if (PRIVATE_DATA->exposure >= 1) {
				setup_data[REQ ] = CCD_READ_PIXELS;
				setup_data[REQ_VALUE_L ] = FLAGS_FIELD_EVEN | FLAGS_SPARE2;
				setup_data[REQ_VALUE_H ] = 0;
				setup_data[REQ_INDEX_L ] = 0;
				setup_data[REQ_INDEX_H ] = 0;
				setup_data[REQ_LENGTH_L] = 10;
				setup_data[REQ_LENGTH_H] = 0;
				setup_data[REQ_DATA + 0] = frame_left & 0xFF;
				setup_data[REQ_DATA + 1] = frame_left >> 8;
				setup_data[REQ_DATA + 2] = (frame_top / vertical_bin) & 0xFF;
				setup_data[REQ_DATA + 3] = (frame_top / vertical_bin) >> 8;
				setup_data[REQ_DATA + 4] = frame_width & 0xFF;
				setup_data[REQ_DATA + 5] = frame_width >> 8;
				setup_data[REQ_DATA + 6] = (frame_height / 2) & 0xFF;
				setup_data[REQ_DATA + 7] = (frame_height / 2) >> 8;
				setup_data[REQ_DATA + 8] = horizontal_bin;
				setup_data[REQ_DATA + 9] = vertical_bin / 2;
				rc = libusb_bulk_transfer(handle, BULK_OUT, setup_data, REQ_DATA + 10, &transferred, BULK_COMMAND_TIMEOUT);
			}
			rc = sx_download_pixels(device, PRIVATE_DATA->buffer + FITS_HEADER_SIZE, 2 * size);
		} else {
			unsigned char *even = PRIVATE_DATA->even;
			if (PRIVATE_DATA->exposure >= 1) {
				setup_data[REQ ] = CCD_READ_PIXELS;
				setup_data[REQ_VALUE_L ] = FLAGS_FIELD_EVEN | FLAGS_SPARE2;
				setup_data[REQ_VALUE_H ] = 0;
				setup_data[REQ_INDEX_L ] = 0;
				setup_data[REQ_INDEX_H ] = 0;
				setup_data[REQ_LENGTH_L] = 10;
				setup_data[REQ_LENGTH_H] = 0;
				setup_data[REQ_DATA + 0] = frame_left & 0xFF;
				setup_data[REQ_DATA + 1] = frame_left >> 8;
				setup_data[REQ_DATA + 2] = (frame_top / 2) & 0xFF;
				setup_data[REQ_DATA + 3] = (frame_top / 2) >> 8;
				setup_data[REQ_DATA + 4] = frame_width & 0xFF;
				setup_data[REQ_DATA + 5] = frame_width >> 8;
				setup_data[REQ_DATA + 6] = (frame_height / 2) & 0xFF;
				setup_data[REQ_DATA + 7] = (frame_height / 2) >> 8;
				setup_data[REQ_DATA + 8] = horizontal_bin;
				setup_data[REQ_DATA + 9] = vertical_bin;
				rc = libusb_bulk_transfer(handle, BULK_OUT, setup_data, REQ_DATA + 10, &transferred, BULK_COMMAND_TIMEOUT);
			}
			rc = sx_download_pixels(device, PRIVATE_DATA->even, size);
			if (rc >= 0) {
				setup_data[REQ ] = CCD_READ_PIXELS;
				setup_data[REQ_VALUE_L ] = FLAGS_FIELD_ODD | FLAGS_SPARE2;
				setup_data[REQ_VALUE_H ] = 0;
				setup_data[REQ_INDEX_L ] = 0;
				setup_data[REQ_INDEX_H ] = 0;
				setup_data[REQ_LENGTH_L] = 10;
				setup_data[REQ_LENGTH_H] = 0;
				setup_data[REQ_DATA + 0] = frame_left & 0xFF;
				setup_data[REQ_DATA + 1] = frame_left >> 8;
				setup_data[REQ_DATA + 2] = (frame_top / 2) & 0xFF;
				setup_data[REQ_DATA + 3] = (frame_top / 2) >> 8;
				setup_data[REQ_DATA + 4] = frame_width & 0xFF;
				setup_data[REQ_DATA + 5] = frame_width >> 8;
				setup_data[REQ_DATA + 6] = (frame_height / 2) & 0xFF;
				setup_data[REQ_DATA + 7] = (frame_height / 2) >> 8;
				setup_data[REQ_DATA + 8] = horizontal_bin;
				setup_data[REQ_DATA + 9] = vertical_bin;
				rc = libusb_bulk_transfer(handle, BULK_OUT, setup_data, REQ_DATA + 10, &transferred, BULK_COMMAND_TIMEOUT);
				if (rc >= 0) {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer -> %lu bytes %s", transferred, rc < 0 ? libusb_error_name(rc) : "OK");
					unsigned char *odd = PRIVATE_DATA->odd;
					rc = sx_download_pixels(device, PRIVATE_DATA->odd, size);
					if (rc >= 0) {
						unsigned long long odd_sum = 0, even_sum = 0;
						uint16_t *pnt = (uint16_t *)odd;
						for (int i = 0; i < size / 2; i += 32)
							odd_sum += *pnt++;
						pnt = (uint16_t *)even;
						for (int i = 0; i < size / 2; i += 32)
							even_sum += *pnt++;
						double ratio = (double)odd_sum/(double)even_sum;
						pnt = (uint16_t *)even;
						for (int i = 0; i < size / 2; i ++) {
							 unsigned short value = (unsigned short)(*pnt * ratio);
							*pnt++ = value;
						}
						unsigned char *buffer = PRIVATE_DATA->buffer + FITS_HEADER_SIZE;
						int ww = frame_width * 2;
						for (int i = 0, j = 0; i < frame_height; i += 2, j++) {
							memcpy(buffer + i * ww, (void *)odd + (j * ww), ww);
							memcpy(buffer + ((i + 1) * ww), (void *)even + (j * ww), ww);
						}
					}
				}
			}
		}
	} else {
		if (PRIVATE_DATA->exposure >= 1) {
			setup_data[REQ ] = CCD_READ_PIXELS;
			setup_data[REQ_VALUE_L ] = FLAGS_FIELD_BOTH;
			setup_data[REQ_VALUE_H ] = 0;
			setup_data[REQ_INDEX_L ] = 0;
			setup_data[REQ_INDEX_H ] = 0;
			setup_data[REQ_LENGTH_L] = 10;
			setup_data[REQ_LENGTH_H] = 0;
			setup_data[REQ_DATA + 0] = frame_left & 0xFF;
			setup_data[REQ_DATA + 1] = frame_left >> 8;
			setup_data[REQ_DATA + 2] = frame_top & 0xFF;
			setup_data[REQ_DATA + 3] = frame_top >> 8;
			setup_data[REQ_DATA + 4] = frame_width & 0xFF;
			setup_data[REQ_DATA + 5] = frame_width >> 8;
			setup_data[REQ_DATA + 6] = frame_height & 0xFF;
			setup_data[REQ_DATA + 7] = frame_height >> 8;
			setup_data[REQ_DATA + 8] = horizontal_bin;
			setup_data[REQ_DATA + 9] = vertical_bin;
			if (PRIVATE_DATA->is_icx453) {
				setup_data[REQ_DATA + 0] = (frame_left * 2) & 0xFF;
				setup_data[REQ_DATA + 1] = (frame_left * 2) >> 8;
				setup_data[REQ_DATA + 2] = (frame_top / 2) & 0xFF;
				setup_data[REQ_DATA + 3] = (frame_top / 2) >> 8;
				setup_data[REQ_DATA + 4] = (frame_width * 2) & 0xFF;
				setup_data[REQ_DATA + 5] = (frame_width * 2) >> 8;
				setup_data[REQ_DATA + 6] = (frame_height / 2) & 0xFF;
				setup_data[REQ_DATA + 7] = (frame_height / 2) >> 8;
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "sx_read_pixels: is_icx453 setup");
			}
			rc = libusb_bulk_transfer(handle, BULK_OUT, setup_data, REQ_DATA + 10, &transferred, BULK_COMMAND_TIMEOUT);
		}
		if (PRIVATE_DATA->is_icx453 && vertical_bin == 1) {
			rc = sx_download_pixels(device, PRIVATE_DATA->even, 2 * size);
			uint16_t *buf16 = (uint16_t *)(PRIVATE_DATA->buffer + FITS_HEADER_SIZE);
			uint16_t *evenBuf16 = (uint16_t *)(PRIVATE_DATA->even);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "sx_read_pixels: is_icx453 %d x %d", frame_width, frame_height);
			for (int i = 0; i < frame_height; i += 2) {
				for (int j = 0; j < frame_width; j += 2) {
					int isubW = i * frame_width;
					int i1subW = (i + 1) * frame_width;
					int j2 = j * 2;
					buf16[isubW + j]  = evenBuf16[isubW + j2];
					buf16[isubW + j + 1]  = evenBuf16[isubW + j2 + 3];
					buf16[i1subW + j]  = evenBuf16[isubW + j2 + 1];
					buf16[i1subW + j + 1]  = evenBuf16[isubW + j2 + 2];
				}
			}
		} else {
			rc = sx_download_pixels(device, PRIVATE_DATA->buffer + FITS_HEADER_SIZE, 2 * size);
		}
	}
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	return rc >= 0;
}

static bool sx_abort_exposure(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	libusb_device_handle *handle = PRIVATE_DATA->handle;
	unsigned char *setup_data = PRIVATE_DATA->setup_data;
	int rc = 0;
	int transferred;
	if (PRIVATE_DATA->extra_caps & CAPS_SHUTTER) {
		setup_data[REQ_TYPE ] = REQ_VENDOR;
		setup_data[REQ ] = CCD_SHUTTER;
		setup_data[REQ_VALUE_L ] = 0;
		setup_data[REQ_VALUE_H ] = FLAGS_SHUTTER_CLOSE;
		setup_data[REQ_INDEX_L ] = 0;
		setup_data[REQ_INDEX_H ] = 0;
		setup_data[REQ_LENGTH_L] = 0;
		setup_data[REQ_LENGTH_H] = 0;
		rc = libusb_bulk_transfer(handle, BULK_OUT, setup_data, REQ_DATA, &transferred, BULK_COMMAND_TIMEOUT);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer -> %lu bytes %s", transferred, rc < 0 ? libusb_error_name(rc) : "OK");
	}
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	return rc >= 0;
}

static bool sx_set_cooler(indigo_device *device, bool status, double target, double *current) {
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	libusb_device_handle *handle = PRIVATE_DATA->handle;
	unsigned char *setup_data = PRIVATE_DATA->setup_data;
	int rc = 0;
	int transferred;
	if (PRIVATE_DATA->extra_caps & CAPS_COOLER) {
		unsigned short setTemp = (unsigned short) (target * 10 + 2730);
		setup_data[REQ_TYPE ] = REQ_VENDOR;
		setup_data[REQ ] = CCD_COOLER;
		setup_data[REQ_VALUE_L ] = setTemp & 0xFF;
		setup_data[REQ_VALUE_H ] = (setTemp >> 8) & 0xFF;
		setup_data[REQ_INDEX_L ] = status ? 1 : 0;
		setup_data[REQ_INDEX_H ] = 0;
		setup_data[REQ_LENGTH_L] = 0;
		setup_data[REQ_LENGTH_H] = 0;
		rc = libusb_bulk_transfer(handle, BULK_OUT, setup_data, REQ_DATA, &transferred, BULK_COMMAND_TIMEOUT);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer -> %lu bytes %s", transferred, rc < 0 ? libusb_error_name(rc) : "OK");
		if (rc >=0 && transferred == REQ_DATA) {
			rc = libusb_bulk_transfer(handle, BULK_IN, setup_data, 3, &transferred, BULK_COMMAND_TIMEOUT);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer -> %lu bytes %s", transferred, rc < 0 ? libusb_error_name(rc) : "OK");
			if (rc >=0 && transferred == 3) {
				*current = ((setup_data[1]*256)+setup_data[0]-2730)/10.0;
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "cooler: %s, target: %gC, current: %gC", setup_data[2] ? "On" : "Off", target, *current);
			}
		}
	}
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	return rc >= 0;
}

static bool sx_guide_relays(indigo_device *device, unsigned short relay_mask) {
	libusb_device_handle *handle = PRIVATE_DATA->handle;
	unsigned char *setup_data = PRIVATE_DATA->setup_data;
	int transferred;
	setup_data[REQ_TYPE ] = REQ_VENDOR | REQ_DATAOUT;
	setup_data[REQ ] = CCD_SET_STAR2K;
	setup_data[REQ_VALUE_L ] = relay_mask;
	setup_data[REQ_VALUE_H ] = 0;
	setup_data[REQ_INDEX_L ] = 0;
	setup_data[REQ_INDEX_H ] = 0;
	setup_data[REQ_LENGTH_L] = 0;
	setup_data[REQ_LENGTH_H] = 0;
	int rc = libusb_bulk_transfer(handle, BULK_OUT, setup_data, REQ_DATA, &transferred, BULK_COMMAND_TIMEOUT);
	return rc >= 0;
}

//static bool sx_flood_led(indigo_device *device, bool state) {
//	libusb_device_handle *handle = PRIVATE_DATA->handle;
//	unsigned char *setup_data = PRIVATE_DATA->setup_data;
//	int transferred;
//	setup_data[REQ_TYPE ] = REQ_VENDOR;
//	setup_data[REQ ] = CCD_FLOOD_LED;
//	setup_data[REQ_VALUE_L ] = state;
//	setup_data[REQ_VALUE_H ] = 0;
//	setup_data[REQ_INDEX_L ] = 0;
//	setup_data[REQ_INDEX_H ] = 0;
//	setup_data[REQ_LENGTH_L] = 0;
//	setup_data[REQ_LENGTH_H] = 0;
//	int rc = libusb_bulk_transfer(handle, BULK_OUT, setup_data, REQ_DATA, &transferred, BULK_COMMAND_TIMEOUT);
//	return rc >= 0;
//}

static void sx_close(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	libusb_close(PRIVATE_DATA->handle);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_close");
	free(PRIVATE_DATA->buffer);
	PRIVATE_DATA->buffer = NULL;
	if (PRIVATE_DATA->is_interlaced) {
		free(PRIVATE_DATA->even);
		PRIVATE_DATA->even = NULL;
		free(PRIVATE_DATA->odd);
		PRIVATE_DATA->odd = NULL;
	}
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
}

// -------------------------------------------------------------------------------- INDIGO CCD device implementation

static void exposure_timer_callback(indigo_device *device) {
	if (!CONNECTION_CONNECTED_ITEM->sw.value) return;
	PRIVATE_DATA->exposure_timer = NULL;
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		CCD_EXPOSURE_ITEM->number.value = 0;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		if (sx_read_pixels(device)) {
			indigo_process_image(device, PRIVATE_DATA->buffer,PRIVATE_DATA->frame_width / PRIVATE_DATA->horizontal_bin, PRIVATE_DATA->frame_height / PRIVATE_DATA->vertical_bin, PRIVATE_DATA->bits_per_pixel, true, true, NULL, false);
			CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		} else {
			CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Exposure failed");
		}
	}
	PRIVATE_DATA->can_check_temperature = true;
}

static void clear_reg_timer_callback(indigo_device *device) {
	if (!CONNECTION_CONNECTED_ITEM->sw.value) return;
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		PRIVATE_DATA->can_check_temperature = false;
		sx_clear_regs(device);
		indigo_set_timer(device, 3, exposure_timer_callback, &PRIVATE_DATA->exposure_timer);
	} else {
		PRIVATE_DATA->exposure_timer = NULL;
	}
}

static void ccd_temperature_callback(indigo_device *device) {
	if (!CONNECTION_CONNECTED_ITEM->sw.value) return;
	if (PRIVATE_DATA->can_check_temperature) {
		if (sx_set_cooler(device, CCD_COOLER_ON_ITEM->sw.value, PRIVATE_DATA->target_temperature, &PRIVATE_DATA->current_temperature)) {
			double diff = PRIVATE_DATA->current_temperature - PRIVATE_DATA->target_temperature;
			if (CCD_COOLER_ON_ITEM->sw.value)
				CCD_TEMPERATURE_PROPERTY->state = fabs(diff) > 0.5 ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
			else
				CCD_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
			CCD_TEMPERATURE_ITEM->number.value = PRIVATE_DATA->current_temperature;
			CCD_COOLER_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			CCD_COOLER_PROPERTY->state = INDIGO_ALERT_STATE;
			CCD_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
		indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
	}
	indigo_reschedule_timer(device, 5, &PRIVATE_DATA->temperture_timer);
}

static indigo_result ccd_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_ccd_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- CCD_INFO, CCD_BIN
		CCD_BIN_PROPERTY->perm = INDIGO_RW_PERM;
		CCD_BIN_HORIZONTAL_ITEM->number.max = CCD_INFO_MAX_HORIZONAL_BIN_ITEM->number.value = 4;
		CCD_BIN_VERTICAL_ITEM->number.max = CCD_INFO_MAX_VERTICAL_BIN_ITEM->number.value = 4;
		CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = 16;
		// --------------------------------------------------------------------------------
		pthread_mutex_init(&PRIVATE_DATA->usb_mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void ccd_connect_callback(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (!device->is_connected) {
			bool result = true;
			if (PRIVATE_DATA->device_count++ == 0) {
				if (indigo_try_global_lock(device) != INDIGO_OK) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_try_global_lock(): failed to get lock.");
					result = false;
				} else {
					result = sx_open(device);
				}
			}
			if (result) {
				CCD_INFO_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max = PRIVATE_DATA->ccd_width;
				CCD_INFO_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max = PRIVATE_DATA->ccd_height;
				CCD_INFO_PIXEL_SIZE_ITEM->number.value = CCD_INFO_PIXEL_WIDTH_ITEM->number.value = round(PRIVATE_DATA->pix_width * 100)/100;
				CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = round(PRIVATE_DATA->pix_height * 100) / 100;
				CCD_MODE_PROPERTY->perm = INDIGO_RW_PERM;
				CCD_MODE_PROPERTY->count = 3;
				char name[32];
				sprintf(name, "RAW 16 %dx%d", PRIVATE_DATA->ccd_width, PRIVATE_DATA->ccd_height);
				indigo_init_switch_item(CCD_MODE_ITEM, "BIN_1x1", name, true);
				sprintf(name, "RAW 16 %dx%d", PRIVATE_DATA->ccd_width/2, PRIVATE_DATA->ccd_height/2);
				indigo_init_switch_item(CCD_MODE_ITEM+1, "BIN_2x2", name, false);
				sprintf(name, "RAW 16 %dx%d", PRIVATE_DATA->ccd_width/4, PRIVATE_DATA->ccd_height/4);
				indigo_init_switch_item(CCD_MODE_ITEM+2, "BIN_4x4", name, false);
				if (PRIVATE_DATA->extra_caps & CAPS_COOLER) {
					CCD_COOLER_PROPERTY->hidden = false;
					CCD_TEMPERATURE_PROPERTY->hidden = false;
					PRIVATE_DATA->target_temperature = 0;
					indigo_set_timer(device, 0, ccd_temperature_callback, &PRIVATE_DATA->temperture_timer);
				}
				PRIVATE_DATA->can_check_temperature = true;
				device->is_connected = true;
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				PRIVATE_DATA->device_count--;
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		}
	} else {
		if (device->is_connected) {
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->temperture_timer);
			if (--PRIVATE_DATA->device_count == 0) {
				sx_close(device);
				indigo_global_unlock(device);
			}
			device->is_connected = false;
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	}	indigo_ccd_change_property(device, NULL, CONNECTION_PROPERTY);
}

static indigo_result ccd_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION -> CCD_INFO, CCD_COOLER, CCD_TEMPERATURE
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, ccd_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(CCD_EXPOSURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_EXPOSURE
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE)
			return INDIGO_OK;
		indigo_property_copy_values(CCD_EXPOSURE_PROPERTY, property, false);
		indigo_use_shortest_exposure_if_bias(device);
		sx_start_exposure(device, CCD_EXPOSURE_ITEM->number.target, CCD_FRAME_TYPE_DARK_ITEM->sw.value || CCD_FRAME_TYPE_DARKFLAT_ITEM->sw.value || CCD_FRAME_TYPE_BIAS_ITEM->sw.value, CCD_FRAME_LEFT_ITEM->number.value, CCD_FRAME_TOP_ITEM->number.value, CCD_FRAME_WIDTH_ITEM->number.value, CCD_FRAME_HEIGHT_ITEM->number.value, CCD_BIN_HORIZONTAL_ITEM->number.value, CCD_BIN_VERTICAL_ITEM->number.value);
		if (CCD_UPLOAD_MODE_LOCAL_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
			CCD_IMAGE_FILE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
		}
		if (CCD_UPLOAD_MODE_CLIENT_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
			CCD_IMAGE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
		}
		CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		if (CCD_EXPOSURE_ITEM->number.target > 3)
			indigo_set_timer(device, CCD_EXPOSURE_ITEM->number.target - 3, clear_reg_timer_callback, &PRIVATE_DATA->exposure_timer);
		else {
			PRIVATE_DATA->can_check_temperature = false;
			indigo_set_timer(device, CCD_EXPOSURE_ITEM->number.target, exposure_timer_callback, &PRIVATE_DATA->exposure_timer);
		}
	} else if (indigo_property_match(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_ABORT_EXPOSURE
		if (indigo_cancel_timer(device, &PRIVATE_DATA->exposure_timer)) {
			sx_abort_exposure(device);
		}
		PRIVATE_DATA->can_check_temperature = true;
		indigo_property_copy_values(CCD_ABORT_EXPOSURE_PROPERTY, property, false);
	} else if (indigo_property_match(CCD_FRAME_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_FRAME
		indigo_property_copy_values(CCD_FRAME_PROPERTY, property, false);
		if (PRIVATE_DATA->is_interlaced) {
			CCD_FRAME_WIDTH_ITEM->number.value = ((int)CCD_FRAME_WIDTH_ITEM->number.value / 2) * 2;
			CCD_FRAME_HEIGHT_ITEM->number.value = ((int)CCD_FRAME_HEIGHT_ITEM->number.value / 2) * 2;
		}
		CCD_FRAME_WIDTH_ITEM->number.value = ((int)CCD_FRAME_WIDTH_ITEM->number.value / (int)CCD_BIN_HORIZONTAL_ITEM->number.value) * (int)CCD_BIN_HORIZONTAL_ITEM->number.value;
		CCD_FRAME_HEIGHT_ITEM->number.value = ((int)CCD_FRAME_HEIGHT_ITEM->number.value / (int)CCD_BIN_VERTICAL_ITEM->number.value) * (int)CCD_BIN_VERTICAL_ITEM->number.value;
		CCD_FRAME_PROPERTY->state = INDIGO_OK_STATE;
		if (CCD_FRAME_LEFT_ITEM->number.value + CCD_FRAME_WIDTH_ITEM->number.value > CCD_INFO_WIDTH_ITEM->number.value) {
			CCD_FRAME_WIDTH_ITEM->number.value = CCD_INFO_WIDTH_ITEM->number.value - CCD_FRAME_LEFT_ITEM->number.value;
			CCD_FRAME_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		if (CCD_FRAME_TOP_ITEM->number.value + CCD_FRAME_HEIGHT_ITEM->number.value > CCD_INFO_HEIGHT_ITEM->number.value) {
			CCD_FRAME_HEIGHT_ITEM->number.value = CCD_INFO_HEIGHT_ITEM->number.value - CCD_FRAME_TOP_ITEM->number.value;
			CCD_FRAME_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		if (IS_CONNECTED) {
			indigo_update_property(device, CCD_FRAME_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(CCD_BIN_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_BIN
		int h = CCD_BIN_HORIZONTAL_ITEM->number.value;
		int v = CCD_BIN_VERTICAL_ITEM->number.value;
		indigo_property_copy_values(CCD_BIN_PROPERTY, property, false);
		if (!(h == 1 || h == 2 || h == 4) || h != v) {
			CCD_BIN_HORIZONTAL_ITEM->number.value = CCD_BIN_VERTICAL_ITEM->number.value = h;
			CCD_BIN_PROPERTY->state = INDIGO_ALERT_STATE;
			if (IS_CONNECTED) {
				indigo_update_property(device, CCD_BIN_PROPERTY, NULL);
			}
			return INDIGO_OK;
		}
	} else if (indigo_property_match(CCD_COOLER_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_COOLER
		indigo_property_copy_values(CCD_COOLER_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value && !CCD_COOLER_PROPERTY->hidden) {
			CCD_COOLER_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(CCD_TEMPERATURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_TEMPERATURE
		indigo_property_copy_values(CCD_TEMPERATURE_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value && !CCD_COOLER_PROPERTY->hidden) {
			PRIVATE_DATA->target_temperature = CCD_TEMPERATURE_ITEM->number.value;
			CCD_TEMPERATURE_ITEM->number.value = PRIVATE_DATA->current_temperature;
			if (CCD_COOLER_OFF_ITEM->sw.value) {
				indigo_set_switch(CCD_COOLER_PROPERTY, CCD_COOLER_ON_ITEM, true);
				CCD_COOLER_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
			}
			CCD_TEMPERATURE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
		}
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_ccd_change_property(device, client, property);
}

static indigo_result ccd_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		ccd_connect_callback(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_ccd_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO guider device implementation

static void guider_timer_callback(indigo_device *device) {
	if (!CONNECTION_CONNECTED_ITEM->sw.value) return;
	PRIVATE_DATA->guider_timer = NULL;
	sx_guide_relays(device, 0);
	if (PRIVATE_DATA->relay_mask & (SX_GUIDE_NORTH | SX_GUIDE_SOUTH)) {
		GUIDER_GUIDE_NORTH_ITEM->number.value = 0;
		GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
	}
	if (PRIVATE_DATA->relay_mask & (SX_GUIDE_WEST | SX_GUIDE_EAST)) {
		GUIDER_GUIDE_EAST_ITEM->number.value = 0;
		GUIDER_GUIDE_WEST_ITEM->number.value = 0;
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
	}
	PRIVATE_DATA->relay_mask = 0;
}

static indigo_result guider_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_guider_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void guider_connect_callback(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (!device->is_connected) {
			bool result = true;
			if (PRIVATE_DATA->device_count++ == 0) {
				if (indigo_try_global_lock(device) != INDIGO_OK) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_try_global_lock(): failed to get lock.");
					result = false;
				} else {
					result = sx_open(device);
				}
			}
			if (result) {
				assert(PRIVATE_DATA->extra_caps & CAPS_STAR2K);
				sx_guide_relays(device, PRIVATE_DATA->relay_mask = 0);
				device->is_connected = true;
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				PRIVATE_DATA->device_count--;
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		}
	} else {
		if (device->is_connected) {
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->guider_timer);
			if (--PRIVATE_DATA->device_count == 0) {
				sx_close(device);
				indigo_global_unlock(device);
			}
			device->is_connected = false;
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	}
	indigo_guider_change_property(device, NULL, CONNECTION_PROPERTY);
}

static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, guider_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_DEC
		indigo_property_copy_values(GUIDER_GUIDE_DEC_PROPERTY, property, false);
		indigo_cancel_timer(device, &PRIVATE_DATA->guider_timer);
		PRIVATE_DATA->relay_mask &= ~(SX_GUIDE_NORTH | SX_GUIDE_SOUTH);
		int duration = GUIDER_GUIDE_NORTH_ITEM->number.value;
		if (duration > 0) {
			PRIVATE_DATA->relay_mask |= SX_GUIDE_NORTH;
			indigo_set_timer(device, duration/1000.0, guider_timer_callback, &PRIVATE_DATA->guider_timer);
		} else {
			int duration = GUIDER_GUIDE_SOUTH_ITEM->number.value;
			if (duration > 0) {
				PRIVATE_DATA->relay_mask |= SX_GUIDE_SOUTH;
				indigo_set_timer(device, duration/1000.0, guider_timer_callback, &PRIVATE_DATA->guider_timer);
			}
		}
		sx_guide_relays(device, PRIVATE_DATA->relay_mask);
		GUIDER_GUIDE_DEC_PROPERTY->state = PRIVATE_DATA->relay_mask & (SX_GUIDE_NORTH | SX_GUIDE_SOUTH) ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDER_GUIDE_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_RA
		indigo_property_copy_values(GUIDER_GUIDE_RA_PROPERTY, property, false);
		indigo_cancel_timer(device, &PRIVATE_DATA->guider_timer);
		PRIVATE_DATA->relay_mask &= ~(SX_GUIDE_EAST | SX_GUIDE_WEST);
		int duration = GUIDER_GUIDE_EAST_ITEM->number.value;
		if (duration > 0) {
			PRIVATE_DATA->relay_mask |= SX_GUIDE_EAST;
			indigo_set_timer(device, duration/1000.0, guider_timer_callback, &PRIVATE_DATA->guider_timer);
		} else {
			int duration = GUIDER_GUIDE_WEST_ITEM->number.value;
			if (duration > 0) {
				PRIVATE_DATA->relay_mask |= SX_GUIDE_WEST;
				indigo_set_timer(device, duration/1000.0, guider_timer_callback, &PRIVATE_DATA->guider_timer);
			}
		}
		sx_guide_relays(device, PRIVATE_DATA->relay_mask);
		GUIDER_GUIDE_RA_PROPERTY->state = PRIVATE_DATA->relay_mask & (SX_GUIDE_WEST | SX_GUIDE_EAST) ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_guider_change_property(device, client, property);
}

static indigo_result guider_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		guider_connect_callback(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_guider_detach(device);
}

// -------------------------------------------------------------------------------- hot-plug support

#define SX_VENDOR_ID                  0x1278

#define MAX_DEVICES                   10

static struct {
	int product;
	const char *name;
	indigo_device_interface iface;
} SX_PRODUCTS[] = {
	{ 0x0100, "SX-Generic", INDIGO_INTERFACE_CCD },
	{ 0x0105, "SXVF-M5", INDIGO_INTERFACE_CCD },
	{ 0x0305, "SXVF-M5C", INDIGO_INTERFACE_CCD },
	{ 0x0107, "SXVF-M7", INDIGO_INTERFACE_CCD },
	{ 0x0307, "SXVF-M7C", INDIGO_INTERFACE_CCD },
	{ 0x0308, "SXVF-M8C", INDIGO_INTERFACE_CCD },
	{ 0x0109, "SXVF-M9", INDIGO_INTERFACE_CCD },
	{ 0x0325, "SXVR-M25C", INDIGO_INTERFACE_CCD },
	{ 0x0326, "SXVR-M26C", INDIGO_INTERFACE_CCD },
	{ 0x0115, "SXVR-H5", INDIGO_INTERFACE_CCD },
	{ 0x0119, "SXVR-H9", INDIGO_INTERFACE_CCD },
	{ 0x0319, "SXVR-H9C", INDIGO_INTERFACE_CCD },
	{ 0x0100, "SXVR-H9", INDIGO_INTERFACE_CCD },
	{ 0x0300, "SXVR-H9C", INDIGO_INTERFACE_CCD },
	{ 0x0126, "SXVR-H16", INDIGO_INTERFACE_CCD },
	{ 0x0128, "SXVR-H18", INDIGO_INTERFACE_CCD },
	{ 0x0135, "SXVR-H35", INDIGO_INTERFACE_CCD },
	{ 0x0136, "SXVR-H36", INDIGO_INTERFACE_CCD },
	{ 0x0137, "SXVR-H360", INDIGO_INTERFACE_CCD },
	{ 0x0139, "SXVR-H390", INDIGO_INTERFACE_CCD },
	{ 0x0194, "SXVR-H694", INDIGO_INTERFACE_CCD },
	{ 0x0394, "SXVR-H694C", INDIGO_INTERFACE_CCD },
	{ 0x0174, "SXVR-H674", INDIGO_INTERFACE_CCD },
	{ 0x0374, "SXVR-H674C", INDIGO_INTERFACE_CCD },
	{ 0x0198, "SX-814", INDIGO_INTERFACE_CCD },
	{ 0x0398, "SX-814C", INDIGO_INTERFACE_CCD },
	{ 0x0189, "SX-825", INDIGO_INTERFACE_CCD },
	{ 0x0389, "SX-825C", INDIGO_INTERFACE_CCD },
	{ 0x0184, "SX-834", INDIGO_INTERFACE_CCD },
	{ 0x0384, "SX-834C", INDIGO_INTERFACE_CCD },
	{ 0x0601, "SX-56", INDIGO_INTERFACE_CCD },
	{ 0x0604, "SX-46", INDIGO_INTERFACE_CCD },
	{ 0x0605, "SX-46C", INDIGO_INTERFACE_CCD },
	{ 0x0606, "SX-50", INDIGO_INTERFACE_CCD },
	{ 0x0607, "SX-50C", INDIGO_INTERFACE_CCD },

	{ 0x0507, "SX LodeStar", INDIGO_INTERFACE_CCD | INDIGO_INTERFACE_GUIDER },
	{ 0x0517, "SX CoStar", INDIGO_INTERFACE_CCD | INDIGO_INTERFACE_GUIDER },
	{ 0x0509, "SX SuperStar", INDIGO_INTERFACE_CCD | INDIGO_INTERFACE_GUIDER },
	{ 0x0525, "SX UltraStar", INDIGO_INTERFACE_CCD | INDIGO_INTERFACE_GUIDER },
	{ 0x0519, "SX Oculus", INDIGO_INTERFACE_CCD | INDIGO_INTERFACE_GUIDER },

	{ 0x0719, "LSI9", INDIGO_INTERFACE_CCD },
	{ 0x0720, "HLSI9", INDIGO_INTERFACE_CCD },
	{ 0, NULL }
};

static indigo_device *devices[MAX_DEVICES];
static pthread_mutex_t device_mutex = PTHREAD_MUTEX_INITIALIZER;

static void process_plug_event(libusb_device *dev) {
	struct libusb_device_descriptor descriptor;
	static indigo_device ccd_template = INDIGO_DEVICE_INITIALIZER(
		"",
		ccd_attach,
		indigo_ccd_enumerate_properties,
		ccd_change_property,
		NULL,
		ccd_detach
	);
	static indigo_device guider_template = INDIGO_DEVICE_INITIALIZER(
		"",
		guider_attach,
		indigo_guider_enumerate_properties,
		guider_change_property,
		NULL,
		guider_detach
	);
	pthread_mutex_lock(&device_mutex);
	INDIGO_DEBUG_DRIVER(int rc =) libusb_get_device_descriptor(dev, &descriptor);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_get_device_descriptor ->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
	for (int i = 0; SX_PRODUCTS[i].name; i++) {
		if (descriptor.idVendor == SX_VENDOR_ID && SX_PRODUCTS[i].product == descriptor.idProduct) {
			sx_private_data *private_data = indigo_safe_malloc(sizeof(sx_private_data));
			private_data->dev = dev;
			libusb_ref_device(dev);
			indigo_device *device = indigo_safe_malloc_copy(sizeof(indigo_device), &ccd_template);
			indigo_device *master_device = device;
			device->master_device = master_device;
			char usb_path[INDIGO_NAME_SIZE];
			indigo_get_usb_path(dev, usb_path);
			snprintf(device->name, INDIGO_NAME_SIZE, "%s #%s", SX_PRODUCTS[i].name, usb_path);
			device->private_data = private_data;
			for (int j = 0; j < MAX_DEVICES; j++) {
				if (devices[j] == NULL) {
					indigo_attach_device(devices[j] = device);
					break;
				}
			}
			device = indigo_safe_malloc_copy(sizeof(indigo_device), &guider_template);
			device->master_device = master_device;
			snprintf(device->name, INDIGO_NAME_SIZE, "%s (guider) #%s", SX_PRODUCTS[i].name, usb_path);
			device->private_data = private_data;
			for (int j = 0; j < MAX_DEVICES; j++) {
				if (devices[j] == NULL) {
					indigo_attach_device(devices[j] = device);
					break;
				}
			}
			break;
		}
	}
	pthread_mutex_unlock(&device_mutex);
}

static void process_unplug_event(libusb_device *dev) {
	pthread_mutex_lock(&device_mutex);
	sx_private_data *private_data = NULL;
	for (int j = 0; j < MAX_DEVICES; j++) {
		if (devices[j] != NULL) {
			indigo_device *device = devices[j];
			if (PRIVATE_DATA->dev == dev) {
				private_data = PRIVATE_DATA;
				indigo_detach_device(device);
				free(device);
				devices[j] = NULL;
			}
		}
	}
	if (private_data != NULL) {
		libusb_unref_device(dev);
		if (private_data->buffer != NULL)
			free(private_data->buffer);
		if (private_data->even != NULL)
			free(private_data->even);
		if (private_data->odd != NULL)
			free(private_data->odd);
		free(private_data);
	}
	pthread_mutex_unlock(&device_mutex);
}

static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			INDIGO_ASYNC(process_plug_event, dev);
			break;
		}
		case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: {
			process_unplug_event(dev);
			break;
		}
	}
	return 0;
}

static libusb_hotplug_callback_handle callback_handle;

indigo_result indigo_ccd_sx(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "Starlight Xpress Camera", __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;
		for (int i = 0; i < MAX_DEVICES; i++) {
			devices[i] = 0;
		}
		indigo_start_usb_event_handler();
		int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, SX_VENDOR_ID, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_register_callback ->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
		return rc >= 0 ? INDIGO_OK : INDIGO_FAILED;

	case INDIGO_DRIVER_SHUTDOWN:
		for (int i = 0; i < MAX_DEVICES; i++)
			VERIFY_NOT_CONNECTED(devices[i]);
		last_action = action;
		libusb_hotplug_deregister_callback(NULL, callback_handle);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_deregister_callback");
		for (int j = 0; j < MAX_DEVICES; j++) {
			if (devices[j] != NULL) {
				indigo_device *device = devices[j];
				hotplug_callback(NULL, PRIVATE_DATA->dev, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, NULL);
			}
		}
		break;

	case INDIGO_DRIVER_INFO:
		break;
	}

	return INDIGO_OK;
}
