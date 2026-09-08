// Copyright (c) 2016-2026 CloudMakers, s. r. o.
// All rights reserved.

// You may use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

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

// This file generated from indigo_ccd_sx.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_ccd_driver.h>
#include <indigo/indigo_guider_driver.h>
#include <indigo/indigo_uni_io.h>
#include <indigo/indigo_usb_utils.h>

#include "indigo_ccd_sx.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x0300000F
#define DRIVER_NAME          "indigo_ccd_sx"
#define DRIVER_LABEL         "Starlight Xpress Camera"
#define CCD_DEVICE_NAME      "%s"
#define GUIDER_DEVICE_NAME   "%s (guider)"
#define MAX_DEVICES          5
#define PRIVATE_DATA         ((sx_private_data *)device->private_data)

//+ define

#define REQ_TYPE             0
#define REQ                  1
#define REQ_VALUE_L          2
#define REQ_VALUE_H          3
#define REQ_INDEX_L          4
#define REQ_INDEX_H          5
#define REQ_LENGTH_L         6
#define REQ_LENGTH_H         7
#define REQ_DATA             8

#define REQ_DIR(r)           ((r)&(1<<7))
#define REQ_DATAOUT          0x00
#define REQ_DATAIN           0x80
#define REQ_KIND(r)          ((r)&(3<<5))
#define REQ_VENDOR           (2<<5)
#define REQ_STD              0
#define REQ_RECIP(r)         ((r)&31)
#define REQ_DEVICE           0x00
#define REQ_IFACE            0x01
#define REQ_ENDPOINT         0x02

#define CCD_GET_FIRMWARE_VERSION 255
#define CCD_ECHO             0
#define CCD_CLEAR_PIXELS     1
#define CCD_READ_PIXELS_DELAYED 2
#define CCD_READ_PIXELS      3
#define CCD_SET_TIMER        4
#define CCD_GET_TIMER        5
#define CCD_RESET            6
#define CCD_SET_CCD          7
#define CCD_GET_CCD          8
#define CCD_SET_STAR2K       9
#define CCD_WRITE_SERIAL_PORT 10
#define CCD_READ_SERIAL_PORT 11
#define CCD_SET_SERIAL       12
#define CCD_GET_SERIAL       13
#define CCD_CAMERA_MODEL     14
#define CCD_LOAD_EEPROM      15
#define CCD_SET_A2D          16
#define CCD_RED_A2D          17
#define CCD_READ_PIXELS_GATED 18
#define CCD_BUILD_NUMBER     19
#define CCD_COOLER_CONTROL   30
#define CCD_COOLER           30
#define CCD_COOLER_TEMPERATURE 31
#define CCD_SHUTTER_CONTROL  32
#define CCD_SHUTTER          32
#define CCD_READ_I2CPORT     33
#define CCD_FLOOD_LED        43

#define CAPS_STAR2K          0x01
#define CAPS_COMPRESS        0x02
#define CAPS_EEPROM          0x04
#define CAPS_GUIDER          0x08
#define CAPS_COOLER          0x10
#define CAPS_SHUTTER         0x20

#define FLAGS_FIELD_ODD      0x01
#define FLAGS_FIELD_EVEN     0x02
#define FLAGS_FIELD_BOTH     (FLAGS_FIELD_EVEN|FLAGS_FIELD_ODD)
#define FLAGS_FIELD_MASK     FLAGS_FIELD_BOTH
#define FLAGS_SPARE2         0x04
#define FLAGS_NOWIPE_FRAME   0x08
#define FLAGS_SPARE4         0x10
#define FLAGS_TDI            0x20
#define FLAGS_NOCLEAR_FRAME  0x40
#define FLAGS_NOCLEAR_REGISTER 0x80

#define FLAGS_SPARE8         0x01
#define FLAGS_SPARE9         0x02
#define FLAGS_SPARE10        0x04
#define FLAGS_SPARE11        0x08
#define FLAGS_SPARE12        0x10
#define FLAGS_SHUTTER_MANUAL 0x20
#define FLAGS_SHUTTER_OPEN   0x40
#define FLAGS_SHUTTER_CLOSE  0x80

#define BULK_IN              0x0082
#define BULK_OUT             0x0001

#define SX_GUIDE_EAST        0x08     /* RA+ */
#define SX_GUIDE_NORTH       0x04     /* DEC+ */
#define SX_GUIDE_SOUTH       0x02     /* DEC- */
#define SX_GUIDE_WEST        0x01     /* RA- */

#define BULK_COMMAND_TIMEOUT 2000
#define BULK_DATA_TIMEOUT    10000

#define CHUNK_SIZE           (4*1024*1024)

#define SX_VENDOR_ID         0x1278

#define PRIVATE_DATA         ((sx_private_data *)device->private_data)

//- define

#pragma mark - Property definitions

#define X_CCD_FLOOD_LED_PROPERTY       (PRIVATE_DATA->x_ccd_flood_led_property)
#define X_CCD_FLOOD_LED_ON_ITEM        (X_CCD_FLOOD_LED_PROPERTY->items + 0)
#define X_CCD_FLOOD_LED_OFF_ITEM       (X_CCD_FLOOD_LED_PROPERTY->items + 1)

#define X_CCD_FLOOD_LED_PROPERTY_NAME  "X_CCD_FLOOD_LED"
#define X_CCD_FLOOD_LED_ON_ITEM_NAME   "ON"
#define X_CCD_FLOOD_LED_OFF_ITEM_NAME  "OFF"

#pragma mark - Private data definition

typedef struct {
	int count;
	libusb_device *usbdev;
	indigo_property *x_ccd_flood_led_property;
	//+ data
	libusb_device_handle *handle;
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
	bool global_lock;
	bool can_check_temperature;
	//- data
} sx_private_data;

#pragma mark - Low level code

static indigo_queue *driver_queue = NULL;
static pthread_mutex_t driver_queue_mutex = PTHREAD_MUTEX_INITIALIZER;

//+ code

typedef enum {
	SX_IMAGE_FAILED,
	SX_IMAGE_DOWNLOADED
} sx_image_result;

static void sx_close(indigo_device *device);

static bool sx_open(indigo_device *device) {
	if (indigo_try_global_lock(device) != INDIGO_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_try_global_lock(): failed to get lock.");
		return false;
	}
	PRIVATE_DATA->global_lock = true;
	int rc = 0;
	libusb_device *usbdev = PRIVATE_DATA->usbdev;
	rc = libusb_open(usbdev, &PRIVATE_DATA->handle);
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
			rc = libusb_get_config_descriptor(usbdev, 0, &config);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_get_config_descriptor -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
			if (rc >= 0) {
				int interface = config->interface->altsetting->bInterfaceNumber;
				libusb_free_config_descriptor(config);
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
		if (rc >= 0 && transferred != REQ_DATA) {
			rc = LIBUSB_ERROR_IO;
		}
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer -> %d bytes %s", transferred, rc < 0 ? libusb_error_name(rc) : "OK");
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
		if (rc >= 0 && transferred != REQ_DATA) {
			rc = LIBUSB_ERROR_IO;
		}
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer -> %d bytes %s", transferred, rc < 0 ? libusb_error_name(rc) : "OK");
		if (rc >=0 && transferred == REQ_DATA) {
			rc = libusb_bulk_transfer(handle, BULK_IN, setup_data, 2, &transferred, BULK_COMMAND_TIMEOUT);
			if (rc >= 0 && transferred != 2) {
				rc = LIBUSB_ERROR_IO;
			}
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer -> %d bytes %s", transferred, rc < 0 ? libusb_error_name(rc) : "OK");
			if (rc >=0 && transferred == 2) {
				int result=setup_data[0] | (setup_data[1] << 8);
				PRIVATE_DATA->model = result & 0x1F;
				PRIVATE_DATA->is_color = result > 0x50;
				PRIVATE_DATA->is_interlaced = result & 0x40;
				if (result == 0x84) {
					PRIVATE_DATA->is_interlaced = true;
				}
				if (PRIVATE_DATA->model == 0x16 || PRIVATE_DATA->model == 0x17 || PRIVATE_DATA->model == 0x18 || PRIVATE_DATA->model == 0x19) {
					PRIVATE_DATA->is_interlaced =  false;
				}
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
		if (rc >= 0 && transferred != REQ_DATA) {
			rc = LIBUSB_ERROR_IO;
		}
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer -> %d bytes %s", transferred, rc < 0 ? libusb_error_name(rc) : "OK");
		if (rc >=0 && transferred == REQ_DATA) {
			rc = libusb_bulk_transfer(handle, BULK_IN, setup_data, 17, &transferred, BULK_COMMAND_TIMEOUT);
			if (rc >= 0 && transferred != 17) {
				rc = LIBUSB_ERROR_IO;
			}
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer -> %d bytes %s", transferred, rc < 0 ? libusb_error_name(rc) : "OK");
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
	if (rc < 0) {
		sx_close(device);
	}
	return rc >= 0;
}

static bool sx_start_exposure(indigo_device *device, double exposure, bool dark, int frame_left, int frame_top, int frame_width, int frame_height, int horizontal_bin, int vertical_bin) {
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
		if (PRIVATE_DATA->extra_caps & CAPS_SHUTTER) {
			setup_data[REQ_VALUE_H] = dark ? FLAGS_SHUTTER_CLOSE : FLAGS_SHUTTER_OPEN;
		}
		if (PRIVATE_DATA->is_interlaced) {
			if (vertical_bin > 1) {
				setup_data[REQ_DATA + 2] = (frame_top/2) & 0xFF;
				setup_data[REQ_DATA + 3] = (frame_top/2) >> 8;
				setup_data[REQ_DATA + 6] = (frame_height/2) & 0xFF;
				setup_data[REQ_DATA + 7] = (frame_height/2) >> 8;
				setup_data[REQ_DATA + 9] = vertical_bin/2;
				rc = libusb_bulk_transfer(handle, BULK_OUT, setup_data, REQ_DATA + 14, &transferred, BULK_COMMAND_TIMEOUT);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer -> %d bytes %s", transferred, rc < 0 ? libusb_error_name(rc) : "OK");
			} else {
				setup_data[REQ_VALUE_L ] = FLAGS_FIELD_EVEN | FLAGS_SPARE2;
				setup_data[REQ_DATA + 2] = (frame_top/2) & 0xFF;
				setup_data[REQ_DATA + 3] = (frame_top/2) >> 8;
				setup_data[REQ_DATA + 6] = (frame_height/2) & 0xFF;
				setup_data[REQ_DATA + 7] = (frame_height/2) >> 8;
				setup_data[REQ_DATA + 9] = 1;
				rc = libusb_bulk_transfer(handle, BULK_OUT, setup_data, REQ_DATA + 14, &transferred, BULK_COMMAND_TIMEOUT);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer -> %d bytes %s", transferred, rc < 0 ? libusb_error_name(rc) : "OK");
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
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer -> %d bytes %s", transferred, rc < 0 ? libusb_error_name(rc) : "OK");
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
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer -> %d bytes %s", transferred, rc < 0 ? libusb_error_name(rc) : "OK");
	}
	PRIVATE_DATA->frame_left = frame_left;
	PRIVATE_DATA->frame_top = frame_top;
	PRIVATE_DATA->frame_width = frame_width;
	PRIVATE_DATA->frame_height = frame_height;
	PRIVATE_DATA->horizontal_bin = horizontal_bin;
	PRIVATE_DATA->vertical_bin = vertical_bin;
	PRIVATE_DATA->exposure = exposure;
	return rc >= 0 && transferred == (exposure < 1 ? REQ_DATA + 14 : REQ_DATA);
}

static bool sx_clear_regs(indigo_device *device) {
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
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer -> %d bytes %s", transferred, rc < 0 ? libusb_error_name(rc) : "OK");
	}
	return rc >= 0 && transferred == REQ_DATA;
}

static int sx_download_pixels(indigo_device *device, unsigned char *pixels, unsigned long count) {
	libusb_device_handle *handle = PRIVATE_DATA->handle;
	int transferred;
	unsigned long read = 0;
	int rc = 0;
	while (read < count && rc >= 0) {
		int size = (int)(count - read);
		if (size > CHUNK_SIZE) {
			size = CHUNK_SIZE;
		}
		rc = libusb_bulk_transfer(handle, BULK_IN, pixels + read, size, &transferred, BULK_DATA_TIMEOUT);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer -> %d bytes %s", transferred, rc < 0 ? libusb_error_name(rc) : "OK");
		if (rc >= 0 && transferred <= 0) {
			rc = LIBUSB_ERROR_IO;
		} else if (transferred > 0) {
			read += transferred;
		}
	}
	return rc;
}

static sx_image_result sx_read_pixels(indigo_device *device) {
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
				if (rc < 0 || transferred != REQ_DATA + 10) {
					return SX_IMAGE_FAILED;
				}
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
				if (rc < 0 || transferred != REQ_DATA + 10) {
					return SX_IMAGE_FAILED;
				}
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
				if (rc < 0 || transferred != REQ_DATA + 10) {
					return SX_IMAGE_FAILED;
				}
				if (rc >= 0) {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer -> %d bytes %s", transferred, rc < 0 ? libusb_error_name(rc) : "OK");
					unsigned char *odd = PRIVATE_DATA->odd;
					rc = sx_download_pixels(device, PRIVATE_DATA->odd, size);
					if (rc >= 0) {
						unsigned long long odd_sum = 0, even_sum = 0;
						uint16_t *pnt = (uint16_t *)odd;
						for (int i = 0; i < size / 2; i += 32) {
							odd_sum += *pnt++;
						}
						pnt = (uint16_t *)even;
						for (int i = 0; i < size / 2; i += 32) {
							even_sum += *pnt++;
						}
						double ratio = even_sum ? (double)odd_sum / (double)even_sum : 1;
						pnt = (uint16_t *)even;
						for (int i = 0; i < size / 2; i ++) {
							 unsigned short value = (unsigned short)(*pnt * ratio);
							*pnt++ = value;
						}
						unsigned char *buffer = PRIVATE_DATA->buffer + FITS_HEADER_SIZE;
						int ww = frame_width * 2;
						for (int i = 0, j = 0; i < frame_height; i += 2, j++) {
							memcpy(buffer + i * ww, (char *)odd + (j * ww), ww);
							memcpy(buffer + ((i + 1) * ww), (char *)even + (j * ww), ww);
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
			if (rc < 0 || transferred != REQ_DATA + 10) {
				return SX_IMAGE_FAILED;
			}
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
	return rc >= 0 ? SX_IMAGE_DOWNLOADED : SX_IMAGE_FAILED;
}


static bool sx_abort_exposure(indigo_device *device) {
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
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer -> %d bytes %s", transferred, rc < 0 ? libusb_error_name(rc) : "OK");
	}
	setup_data[REQ_TYPE ] = REQ_VENDOR | REQ_DATAOUT;
	setup_data[REQ ] = CCD_RESET;
	setup_data[REQ_VALUE_L ] = 0;
	setup_data[REQ_VALUE_H ] = 0;
	setup_data[REQ_INDEX_L ] = 0;
	setup_data[REQ_INDEX_H ] = 0;
	setup_data[REQ_LENGTH_L] = 0;
	setup_data[REQ_LENGTH_H] = 0;
	rc = libusb_bulk_transfer(handle, BULK_OUT, setup_data, REQ_DATA, &transferred, BULK_COMMAND_TIMEOUT);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer -> %d bytes %s", transferred, rc < 0 ? libusb_error_name(rc) : "OK");
	indigo_usleep(1000);
	return rc >= 0 && transferred == REQ_DATA;
}

static bool sx_set_cooler(indigo_device *device, bool status, double target, double *current) {
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
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer -> %d bytes %s", transferred, rc < 0 ? libusb_error_name(rc) : "OK");
		if (rc >=0 && transferred == REQ_DATA) {
			rc = libusb_bulk_transfer(handle, BULK_IN, setup_data, 3, &transferred, BULK_COMMAND_TIMEOUT);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer -> %d bytes %s", transferred, rc < 0 ? libusb_error_name(rc) : "OK");
			if (rc >=0 && transferred == 3) {
				*current = ((setup_data[1]*256)+setup_data[0]-2730)/10.0;
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "cooler: %s, target: %gC, current: %gC", setup_data[2] ? "On" : "Off", target, *current);
			}
			if (rc >= 0 && transferred != 3) {
				rc = LIBUSB_ERROR_IO;
			}
		} else if (rc >= 0) {
			rc = LIBUSB_ERROR_IO;
		}
	}
	return rc >= 0;
}

static bool sx_guide_relays(indigo_device *device, unsigned short relay_mask) {
	libusb_device_handle *handle = PRIVATE_DATA->handle;
	unsigned char *setup_data = PRIVATE_DATA->setup_data;
	int transferred;
	setup_data[REQ_TYPE ] = REQ_VENDOR | REQ_DATAOUT;
	setup_data[REQ ] = CCD_SET_STAR2K;
	setup_data[REQ_VALUE_L ] = (uint8_t)relay_mask;
	setup_data[REQ_VALUE_H ] = 0;
	setup_data[REQ_INDEX_L ] = 0;
	setup_data[REQ_INDEX_H ] = 0;
	setup_data[REQ_LENGTH_L] = 0;
	setup_data[REQ_LENGTH_H] = 0;
	int rc = libusb_bulk_transfer(handle, BULK_OUT, setup_data, REQ_DATA, &transferred, BULK_COMMAND_TIMEOUT);
	return rc >= 0 && transferred == REQ_DATA;
}

static bool sx_flood_led(indigo_device *device, bool state) {
	libusb_device_handle *handle = PRIVATE_DATA->handle;
	unsigned char *setup_data = PRIVATE_DATA->setup_data;
	int transferred;
	setup_data[REQ_TYPE] = REQ_VENDOR | REQ_DATAOUT;
	setup_data[REQ] = CCD_FLOOD_LED;
	setup_data[REQ_VALUE_L] = state;
	setup_data[REQ_VALUE_H] = 0;
	setup_data[REQ_INDEX_L] = 0;
	setup_data[REQ_INDEX_H] = 0;
	setup_data[REQ_LENGTH_L] = 0;
	setup_data[REQ_LENGTH_H] = 0;
	int rc = libusb_bulk_transfer(handle, BULK_OUT, setup_data, REQ_DATA, &transferred, BULK_COMMAND_TIMEOUT);
	return rc >= 0 && transferred == REQ_DATA;
}

static void sx_close(indigo_device *device) {
	if (PRIVATE_DATA->handle != NULL) {
		libusb_close(PRIVATE_DATA->handle);
		PRIVATE_DATA->handle = NULL;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_close");
	}
	indigo_safe_free(PRIVATE_DATA->buffer);
	PRIVATE_DATA->buffer = NULL;
	indigo_safe_free(PRIVATE_DATA->even);
	PRIVATE_DATA->even = NULL;
	indigo_safe_free(PRIVATE_DATA->odd);
	PRIVATE_DATA->odd = NULL;
	if (PRIVATE_DATA->global_lock) {
		indigo_global_unlock(device);
		PRIVATE_DATA->global_lock = false;
	}
}

// -------------------------------------------------------------------------------- INDIGO CCD device implementation

static void ccd_exposure_finalizer(indigo_device *device) {
	if (!CONNECTION_CONNECTED_ITEM->sw.value) {
		return;
	}
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		// Pixel download is synchronous and may take several seconds on USB 2.0.
		indigo_set_handler_max_run_time(0);
		switch (sx_read_pixels(device)) {
			case SX_IMAGE_DOWNLOADED:
				CCD_EXPOSURE_ITEM->number.value = 0;
				indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
			indigo_process_image(device, PRIVATE_DATA->buffer,PRIVATE_DATA->frame_width / PRIVATE_DATA->horizontal_bin, PRIVATE_DATA->frame_height / PRIVATE_DATA->vertical_bin, PRIVATE_DATA->bits_per_pixel, true, true, NULL, false);
			CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
				PRIVATE_DATA->can_check_temperature = true;
				break;
			case SX_IMAGE_FAILED:
				indigo_ccd_failure_cleanup(device);
				CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Exposure failed");
				PRIVATE_DATA->can_check_temperature = true;
				break;
		}
	}
}

static void ccd_clear_registers_handler(indigo_device *device) {
	if (!CONNECTION_CONNECTED_ITEM->sw.value) {
		return;
	}
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		PRIVATE_DATA->can_check_temperature = false;
		if (!sx_clear_regs(device)) {
			PRIVATE_DATA->can_check_temperature = true;
			indigo_ccd_failure_cleanup(device);
			INDIGO_UPDATE_PROPERTY_STATE(CCD_EXPOSURE_PROPERTY, INDIGO_ALERT_STATE, "Register clear failed");
			return;
		}
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, 3, ccd_exposure_finalizer);
	}
}

static void ccd_temperature_poll_handler(indigo_device *device) {
	if (!CONNECTION_CONNECTED_ITEM->sw.value) {
		return;
	}
	if (PRIVATE_DATA->can_check_temperature) {
		if (sx_set_cooler(device, CCD_COOLER_ON_ITEM->sw.value, PRIVATE_DATA->target_temperature, &PRIVATE_DATA->current_temperature)) {
			double diff = PRIVATE_DATA->current_temperature - PRIVATE_DATA->target_temperature;
			if (CCD_COOLER_ON_ITEM->sw.value) {
				CCD_TEMPERATURE_PROPERTY->state = fabs(diff) > 0.5 ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
			} else {
				CCD_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
			}
			CCD_TEMPERATURE_ITEM->number.value = PRIVATE_DATA->current_temperature;
			CCD_COOLER_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			CCD_COOLER_PROPERTY->state = INDIGO_ALERT_STATE;
			CCD_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
		indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
	}
	indigo_execute_handler_in(device, 5, ccd_temperature_poll_handler);
}

static void guider_guide_dec_finalizer(indigo_device *device) {
	if (!CONNECTION_CONNECTED_ITEM->sw.value) {
		return;
	}
	PRIVATE_DATA->relay_mask &= ~(SX_GUIDE_NORTH | SX_GUIDE_SOUTH);
	bool ok = sx_guide_relays(device, PRIVATE_DATA->relay_mask);
	GUIDER_GUIDE_NORTH_ITEM->number.value = 0;
	GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
	INDIGO_UPDATE_PROPERTY_STATE(GUIDER_GUIDE_DEC_PROPERTY, ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE, NULL);
}

static void guider_guide_ra_finalizer(indigo_device *device) {
	if (!CONNECTION_CONNECTED_ITEM->sw.value) {
		return;
	}
	PRIVATE_DATA->relay_mask &= ~(SX_GUIDE_WEST | SX_GUIDE_EAST);
	bool ok = sx_guide_relays(device, PRIVATE_DATA->relay_mask);
	GUIDER_GUIDE_EAST_ITEM->number.value = 0;
	GUIDER_GUIDE_WEST_ITEM->number.value = 0;
	INDIGO_UPDATE_PROPERTY_STATE(GUIDER_GUIDE_RA_PROPERTY, ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE, NULL);
}
static struct {
	int product;
	const char *name;
	indigo_device_interface iface;
} sx_products[] = {
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

static bool sx_match(libusb_device *dev, const char **name) {
	struct libusb_device_descriptor descriptor;
	INDIGO_DEBUG_DRIVER(int rc =) libusb_get_device_descriptor(dev, &descriptor);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_get_device_descriptor -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
	if (rc < 0) {
		return false;
	}
	for (int i = 0; sx_products[i].name; i++) {
		if (sx_products[i].product == descriptor.idProduct) {
			static char device_name[INDIGO_NAME_SIZE];
			char usb_path[INDIGO_NAME_SIZE];
			indigo_get_usb_path(dev, usb_path);
			snprintf(device_name, sizeof(device_name), "%s #%s", sx_products[i].name, usb_path);
			*name = device_name;
			return true;
		}
	}
	return false;
}

//- code

#pragma mark - High level code (ccd)

static void ccd_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = sx_open(device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ ccd.on_connect
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
			CCD_COOLER_PROPERTY->hidden = CCD_TEMPERATURE_PROPERTY->hidden = !(PRIVATE_DATA->extra_caps & CAPS_COOLER);
			if (PRIVATE_DATA->extra_caps & CAPS_COOLER) {
				PRIVATE_DATA->target_temperature = 0;
				indigo_execute_handler(device, ccd_temperature_poll_handler);
			}
			if (PRIVATE_DATA->has_flood_led) {
				X_CCD_FLOOD_LED_PROPERTY->hidden = false;
			}
			PRIVATE_DATA->can_check_temperature = true;
			//- ccd.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, X_CCD_FLOOD_LED_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s", device->name);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				sx_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ ccd.on_disconnect
		indigo_cancel_pending_handler(device, ccd_temperature_poll_handler);
		if (!X_CCD_FLOOD_LED_PROPERTY->hidden) {
			sx_flood_led(device, false);
			X_CCD_FLOOD_LED_PROPERTY->hidden = true;
		}
		//- ccd.on_disconnect
		indigo_delete_property(device, X_CCD_FLOOD_LED_PROPERTY, NULL);
		if (--PRIVATE_DATA->count == 0) {
			sx_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_ccd_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void ccd_x_ccd_flood_led_handler(indigo_device *device) {
	X_CCD_FLOOD_LED_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.X_CCD_FLOOD_LED.on_change
	if (sx_flood_led(device, X_CCD_FLOOD_LED_ON_ITEM->sw.value)) {
		INDIGO_UPDATE_PROPERTY_STATE(X_CCD_FLOOD_LED_PROPERTY, INDIGO_OK_STATE, NULL);
	} else {
		INDIGO_UPDATE_PROPERTY_STATE(X_CCD_FLOOD_LED_PROPERTY, INDIGO_ALERT_STATE, "Flood LED control failed");
	}
	//- ccd.X_CCD_FLOOD_LED.on_change
	indigo_update_property(device, X_CCD_FLOOD_LED_PROPERTY, NULL);
}

static void ccd_exposure_handler(indigo_device *device) {
	//+ ccd.CCD_EXPOSURE.on_change
	indigo_use_shortest_exposure_if_bias(device);
	bool ok = sx_start_exposure(device, CCD_EXPOSURE_ITEM->number.target, CCD_FRAME_TYPE_DARK_ITEM->sw.value || CCD_FRAME_TYPE_DARKFLAT_ITEM->sw.value || CCD_FRAME_TYPE_BIAS_ITEM->sw.value, CCD_FRAME_LEFT_ITEM->number.value, CCD_FRAME_TOP_ITEM->number.value, CCD_FRAME_WIDTH_ITEM->number.value, CCD_FRAME_HEIGHT_ITEM->number.value, CCD_BIN_HORIZONTAL_ITEM->number.value, CCD_BIN_VERTICAL_ITEM->number.value);
	if (ok) {
		indigo_ccd_exposure_setup(device);
		if (CCD_EXPOSURE_ITEM->number.target > 3) {
			indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, CCD_EXPOSURE_ITEM->number.target - 3, ccd_clear_registers_handler);
		} else {
			PRIVATE_DATA->can_check_temperature = false;
			indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, CCD_EXPOSURE_ITEM->number.target, ccd_exposure_finalizer);
		}
	} else {
		PRIVATE_DATA->can_check_temperature = true;
		indigo_ccd_failure_cleanup(device);
		INDIGO_UPDATE_PROPERTY_STATE(CCD_EXPOSURE_PROPERTY, INDIGO_ALERT_STATE, "Exposure failed");
	}
	//- ccd.CCD_EXPOSURE.on_change
}

static void ccd_abort_exposure_handler(indigo_device *device) {
	//+ ccd.CCD_ABORT_EXPOSURE.on_change
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		indigo_cancel_pending_handler(device, ccd_clear_registers_handler);
		indigo_cancel_pending_handler(device, ccd_exposure_finalizer);
		sx_abort_exposure(device);
	}
	PRIVATE_DATA->can_check_temperature = true;
	indigo_ccd_abort_exposure_cleanup(device);
	//- ccd.CCD_ABORT_EXPOSURE.on_change
}

static void ccd_frame_handler(indigo_device *device) {
	CCD_FRAME_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_FRAME.on_change
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
	//- ccd.CCD_FRAME.on_change
	indigo_update_property(device, CCD_FRAME_PROPERTY, NULL);
}

static void ccd_bin_handler(indigo_device *device) {
	CCD_BIN_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_BIN.on_change
	indigo_ccd_change_property(device, NULL, CCD_BIN_PROPERTY);
	//- ccd.CCD_BIN.on_change
	indigo_update_property(device, CCD_BIN_PROPERTY, NULL);
}

static void ccd_cooler_handler(indigo_device *device) {
	CCD_COOLER_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_COOLER.on_change
	if (CONNECTION_CONNECTED_ITEM->sw.value && !CCD_COOLER_PROPERTY->hidden) {
		INDIGO_UPDATE_PROPERTY_STATE(CCD_COOLER_PROPERTY, INDIGO_BUSY_STATE, NULL);
	}
	//- ccd.CCD_COOLER.on_change
	indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
}

static void ccd_temperature_handler(indigo_device *device) {
	CCD_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_TEMPERATURE.on_change
	if (CONNECTION_CONNECTED_ITEM->sw.value && !CCD_COOLER_PROPERTY->hidden) {
		PRIVATE_DATA->target_temperature = CCD_TEMPERATURE_ITEM->number.target;
		CCD_TEMPERATURE_ITEM->number.value = PRIVATE_DATA->current_temperature;
		if (CCD_COOLER_OFF_ITEM->sw.value) {
			indigo_set_switch(CCD_COOLER_PROPERTY, CCD_COOLER_ON_ITEM, true);
			INDIGO_UPDATE_PROPERTY_STATE(CCD_COOLER_PROPERTY, INDIGO_BUSY_STATE, NULL);
		}
		INDIGO_UPDATE_PROPERTY_STATE(CCD_TEMPERATURE_PROPERTY, INDIGO_BUSY_STATE, NULL);
	}
	//- ccd.CCD_TEMPERATURE.on_change
	indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
}

#pragma mark - Device API (ccd)

static indigo_result ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result ccd_attach(indigo_device *device) {
	if (indigo_ccd_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		//+ ccd.on_attach
		// -------------------------------------------------------------------------------- CCD_INFO, CCD_BIN
		CCD_BIN_PROPERTY->perm = INDIGO_RW_PERM;
		CCD_BIN_HORIZONTAL_ITEM->number.max = CCD_INFO_MAX_HORIZONAL_BIN_ITEM->number.value = 4;
		CCD_BIN_VERTICAL_ITEM->number.max = CCD_INFO_MAX_VERTICAL_BIN_ITEM->number.value = 4;
		CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = 16;
		// --------------------------------------------------------------------------------
		//- ccd.on_attach
		X_CCD_FLOOD_LED_PROPERTY = indigo_init_switch_property(NULL, device->name, X_CCD_FLOOD_LED_PROPERTY_NAME, CCD_ADVANCED_GROUP, "Flood LED", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_CCD_FLOOD_LED_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_CCD_FLOOD_LED_ON_ITEM, X_CCD_FLOOD_LED_ON_ITEM_NAME, "On", false);
		indigo_init_switch_item(X_CCD_FLOOD_LED_OFF_ITEM, X_CCD_FLOOD_LED_OFF_ITEM_NAME, "Off", true);
		X_CCD_FLOOD_LED_PROPERTY->hidden = true;
		CCD_EXPOSURE_PROPERTY->hidden = false;
		CCD_ABORT_EXPOSURE_PROPERTY->hidden = false;
		CCD_FRAME_PROPERTY->hidden = false;
		CCD_BIN_PROPERTY->hidden = false;
		CCD_COOLER_PROPERTY->hidden = false;
		CCD_TEMPERATURE_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_CCD_FLOOD_LED_PROPERTY);
	}
	return indigo_ccd_enumerate_properties(device, client, property);
}

static indigo_result ccd_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			if (CONNECTION_CONNECTED_ITEM->sw.value && PRIVATE_DATA->count == 0) {
				indigo_queue_add(driver_queue, device, INDIGO_TASK_PRIORITY_NORMAL, 0, ccd_connection_handler, &driver_queue_mutex);
			} else {
				indigo_execute_handler(device, ccd_connection_handler);
			}
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_CCD_FLOOD_LED_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_CCD_FLOOD_LED_PROPERTY, ccd_x_ccd_flood_led_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_EXPOSURE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_EXPOSURE_PROPERTY, ccd_exposure_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_ABORT_EXPOSURE_PROPERTY, ccd_abort_exposure_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_FRAME_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_SYNC_CHANGE(CCD_FRAME_PROPERTY, ccd_frame_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_BIN_PROPERTY, property)) {
		//+ ccd.CCD_BIN.on_change_request
		double h = CCD_BIN_HORIZONTAL_ITEM->number.value;
		double v = CCD_BIN_VERTICAL_ITEM->number.value;
		for (int i = 0; i < property->count; i++) {
			if (!strcmp(property->items[i].name, CCD_BIN_HORIZONTAL_ITEM_NAME)) { h = property->items[i].number.value; }
			if (!strcmp(property->items[i].name, CCD_BIN_VERTICAL_ITEM_NAME)) { v = property->items[i].number.value; }
		}
		if (!(h == 1 || h == 2 || h == 4) || h != v) {
			INDIGO_UPDATE_PROPERTY_STATE(CCD_BIN_PROPERTY, INDIGO_ALERT_STATE, "Unsupported binning");
			return INDIGO_OK;
		}
		//- ccd.CCD_BIN.on_change_request
		INDIGO_COPY_VALUES_PROCESS_SYNC_CHANGE(CCD_BIN_PROPERTY, ccd_bin_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_COOLER_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_COOLER_PROPERTY, ccd_cooler_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_TEMPERATURE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_TEMPERATURE_PROPERTY, ccd_temperature_handler);
		return INDIGO_OK;
	}
	return indigo_ccd_change_property(device, client, property);
}

static indigo_result ccd_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		ccd_connection_handler(device);
	}
	indigo_release_property(X_CCD_FLOOD_LED_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_ccd_detach(device);
}

#pragma mark - High level code (guider)

static void guider_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = sx_open(device->master_device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ guider.on_connect
			connection_result = (PRIVATE_DATA->extra_caps & CAPS_STAR2K) && sx_guide_relays(device, PRIVATE_DATA->relay_mask = 0);
			//- guider.on_connect
		}
		if (connection_result) {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s", device->name);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				sx_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ guider.on_disconnect
		indigo_cancel_pending_handler(device, guider_guide_dec_finalizer);
		indigo_cancel_pending_handler(device, guider_guide_ra_finalizer);
		sx_guide_relays(device, PRIVATE_DATA->relay_mask = 0);
		//- guider.on_disconnect
		if (--PRIVATE_DATA->count == 0) {
			sx_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_guider_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void guider_guide_dec_handler(indigo_device *device) {
	//+ guider.GUIDER_GUIDE_DEC.on_change
	indigo_cancel_pending_handler(device, guider_guide_dec_finalizer);
	PRIVATE_DATA->relay_mask &= ~(SX_GUIDE_NORTH | SX_GUIDE_SOUTH);
	int duration = (int)GUIDER_GUIDE_NORTH_ITEM->number.target;
	if (duration > 0) {
		PRIVATE_DATA->relay_mask |= SX_GUIDE_NORTH;
	} else {
		duration = (int)GUIDER_GUIDE_SOUTH_ITEM->number.target;
		if (duration > 0) {
			PRIVATE_DATA->relay_mask |= SX_GUIDE_SOUTH;
		}
	}
	if (!sx_guide_relays(device, PRIVATE_DATA->relay_mask)) {
		PRIVATE_DATA->relay_mask &= ~(SX_GUIDE_NORTH | SX_GUIDE_SOUTH);
		INDIGO_UPDATE_PROPERTY_STATE(GUIDER_GUIDE_DEC_PROPERTY, INDIGO_ALERT_STATE, "Guide command failed");
		return;
	}
	if (duration > 0) {
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, duration / 1000.0, guider_guide_dec_finalizer);
	}
	INDIGO_UPDATE_PROPERTY_STATE(GUIDER_GUIDE_DEC_PROPERTY, PRIVATE_DATA->relay_mask & (SX_GUIDE_NORTH | SX_GUIDE_SOUTH) ? INDIGO_BUSY_STATE : INDIGO_OK_STATE, NULL);
	//- guider.GUIDER_GUIDE_DEC.on_change
}

static void guider_guide_ra_handler(indigo_device *device) {
	//+ guider.GUIDER_GUIDE_RA.on_change
	indigo_cancel_pending_handler(device, guider_guide_ra_finalizer);
	PRIVATE_DATA->relay_mask &= ~(SX_GUIDE_EAST | SX_GUIDE_WEST);
	int duration = (int)GUIDER_GUIDE_EAST_ITEM->number.target;
	if (duration > 0) {
		PRIVATE_DATA->relay_mask |= SX_GUIDE_EAST;
	} else {
		duration = (int)GUIDER_GUIDE_WEST_ITEM->number.target;
		if (duration > 0) {
			PRIVATE_DATA->relay_mask |= SX_GUIDE_WEST;
		}
	}
	if (!sx_guide_relays(device, PRIVATE_DATA->relay_mask)) {
		PRIVATE_DATA->relay_mask &= ~(SX_GUIDE_EAST | SX_GUIDE_WEST);
		INDIGO_UPDATE_PROPERTY_STATE(GUIDER_GUIDE_RA_PROPERTY, INDIGO_ALERT_STATE, "Guide command failed");
		return;
	}
	if (duration > 0) {
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, duration / 1000.0, guider_guide_ra_finalizer);
	}
	INDIGO_UPDATE_PROPERTY_STATE(GUIDER_GUIDE_RA_PROPERTY, PRIVATE_DATA->relay_mask & (SX_GUIDE_WEST | SX_GUIDE_EAST) ? INDIGO_BUSY_STATE : INDIGO_OK_STATE, NULL);
	//- guider.GUIDER_GUIDE_RA.on_change
}

#pragma mark - Device API (guider)

static indigo_result guider_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result guider_attach(indigo_device *device) {
	if (indigo_guider_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		GUIDER_GUIDE_DEC_PROPERTY->hidden = false;
		GUIDER_GUIDE_RA_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result guider_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	return indigo_guider_enumerate_properties(device, client, property);
}

static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			if (CONNECTION_CONNECTED_ITEM->sw.value && PRIVATE_DATA->count == 0) {
				indigo_queue_add(driver_queue, device, INDIGO_TASK_PRIORITY_NORMAL, 0, guider_connection_handler, &driver_queue_mutex);
			} else {
				indigo_execute_handler(device, guider_connection_handler);
			}
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		//+ guider.GUIDER_GUIDE_DEC.on_change_request
		indigo_cancel_pending_handler(device, guider_guide_dec_handler);
		GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_NORTH_ITEM->number.target = 0;
		GUIDER_GUIDE_SOUTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.target = 0;
		// Accept replacement and zero requests while the previous pulse is BUSY.
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
		//- guider.GUIDER_GUIDE_DEC.on_change_request
		INDIGO_COPY_VALUES_PROCESS_PRIORITY_CHANGE(GUIDER_GUIDE_DEC_PROPERTY, guider_guide_dec_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_RA_PROPERTY, property)) {
		//+ guider.GUIDER_GUIDE_RA.on_change_request
		indigo_cancel_pending_handler(device, guider_guide_ra_handler);
		GUIDER_GUIDE_EAST_ITEM->number.value = GUIDER_GUIDE_EAST_ITEM->number.target = 0;
		GUIDER_GUIDE_WEST_ITEM->number.value = GUIDER_GUIDE_WEST_ITEM->number.target = 0;
		// Accept replacement and zero requests while the previous pulse is BUSY.
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
		//- guider.GUIDER_GUIDE_RA.on_change_request
		INDIGO_COPY_VALUES_PROCESS_PRIORITY_CHANGE(GUIDER_GUIDE_RA_PROPERTY, guider_guide_ra_handler);
		return INDIGO_OK;
	}
	return indigo_guider_change_property(device, client, property);
}

static indigo_result guider_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		guider_connection_handler(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_guider_detach(device);
}

#pragma mark - Device templates

static indigo_device ccd_template = INDIGO_DEVICE_INITIALIZER(CCD_DEVICE_NAME, ccd_attach, ccd_enumerate_properties, ccd_change_property, NULL, ccd_detach);

static indigo_device guider_template = INDIGO_DEVICE_INITIALIZER(GUIDER_DEVICE_NAME, guider_attach, guider_enumerate_properties, guider_change_property, NULL, guider_detach);

#pragma mark - Hot-plug code

static indigo_device *devices[MAX_DEVICES];

static indigo_result verify_devices_disconnected(void) {
	for (int i = 0; i < MAX_DEVICES; i++) {
		VERIFY_NOT_CONNECTED(devices[i]);
	}
	return INDIGO_OK;
}

static void process_plug_event_handler(indigo_device *device, void *data) {
	indigo_set_handler_max_run_time(1);
	libusb_device *dev = (libusb_device *)data;
	bool dev_ref_transferred = false;
	sx_private_data *private_data = NULL;
	for (int i = 0; i < MAX_DEVICES; i++) {
		if (devices[i] && ((sx_private_data *)devices[i]->private_data)->usbdev == dev) {
			libusb_unref_device(dev);
			return;
		}
	}
	const char *name;
	if (sx_match(dev, &name)) {
		private_data = indigo_safe_malloc(sizeof(sx_private_data));
		private_data->usbdev = dev;
			indigo_device *ccd = indigo_safe_malloc_copy(sizeof(indigo_device), &ccd_template);
			ccd->private_data = private_data;
			snprintf(ccd->name, INDIGO_NAME_SIZE, "%s", name);
			bool ccd_attached = false;
			for (int j = 0; j < MAX_DEVICES; j++) {
				if (devices[j] == NULL) {
					devices[j] = ccd;
					if (indigo_attach_device(ccd) == INDIGO_OK) {
						dev_ref_transferred = true;
						ccd_attached = true;
					} else {
						devices[j] = NULL;
					}
					break;
				}
			}
			if (!ccd_attached) {
				indigo_safe_free(ccd);
				indigo_safe_free(private_data);
				libusb_unref_device(dev);
				return;
			}
			indigo_device *guider = indigo_safe_malloc_copy(sizeof(indigo_device), &guider_template);
			guider->private_data = private_data;
			guider->master_device = ccd;
			snprintf(guider->name, INDIGO_NAME_SIZE, "%s (guider)", name);
			bool guider_attached = false;
			for (int j = 0; j < MAX_DEVICES; j++) {
				if (devices[j] == NULL) {
					devices[j] = guider;
					if (indigo_attach_device(guider) == INDIGO_OK) {
						dev_ref_transferred = true;
						guider_attached = true;
					} else {
						devices[j] = NULL;
					}
					break;
				}
			}
			if (!guider_attached) {
				indigo_safe_free(guider);
			}
	}
	if (!dev_ref_transferred) {
		indigo_safe_free(private_data);
		libusb_unref_device(dev);
	}
}

static void process_unplug_event_handler(indigo_device *device, void *data) {
	libusb_device *dev = (libusb_device *)data;
	sx_private_data *private_data = NULL;
	for (int j = MAX_DEVICES - 1; j >= 0; j--) {
		if (devices[j] != NULL) {
			indigo_device *device = devices[j];
			if (PRIVATE_DATA->usbdev == dev) {
				private_data = PRIVATE_DATA;
				indigo_detach_device(device);
				indigo_safe_free(device);
				devices[j] = NULL;
			}
		}
	}
	if (private_data != NULL) {
		libusb_unref_device(dev);
		indigo_safe_free(private_data);
	}
	libusb_unref_device(dev);
}

static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			dev = libusb_ref_device(dev);
			indigo_queue_add_with_data(driver_queue, NULL, INDIGO_TASK_PRIORITY_NORMAL, 0, process_plug_event_handler, dev, &driver_queue_mutex);
			break;
		}
		case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: {
			dev = libusb_ref_device(dev);
			indigo_queue_add_with_data(driver_queue, NULL, INDIGO_TASK_PRIORITY_NORMAL, 0, process_unplug_event_handler, dev, &driver_queue_mutex);
			break;
		}
		default:
			break;
	}
	return 0;
}

static libusb_hotplug_callback_handle callback_handle;

#pragma mark - Main code

indigo_result indigo_ccd_sx(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			for (int i = 0; i < MAX_DEVICES; i++) {
				devices[i] = NULL;
			}
			driver_queue = indigo_queue_create(NULL);
			if (driver_queue == NULL) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to create driver queue");
				last_action = INDIGO_DRIVER_SHUTDOWN;
				return INDIGO_FAILED;
			}
			indigo_queue_set_name(driver_queue, "Queue " DRIVER_LABEL);
			indigo_start_usb_event_handler();
			int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, SX_VENDOR_ID, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_register_callback ->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
			if (rc < 0) {
				indigo_queue_delete(&driver_queue);
				last_action = INDIGO_DRIVER_SHUTDOWN;
				return INDIGO_FAILED;
			}
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			pthread_mutex_lock(&driver_queue_mutex);
			indigo_result shutdown_result = verify_devices_disconnected();
			pthread_mutex_unlock(&driver_queue_mutex);
			if (shutdown_result != INDIGO_OK) {
				return shutdown_result;
			}
			last_action = action;
			libusb_hotplug_deregister_callback(NULL, callback_handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_deregister_callback");
			indigo_queue_drain(driver_queue);
			for (int i = 0; i < MAX_DEVICES; i++) {
				if (devices[i] != NULL) {
					indigo_device *device = devices[i];
					process_unplug_event_handler(NULL, libusb_ref_device(PRIVATE_DATA->usbdev));
				}
			}
			indigo_queue_delete(&driver_queue);
			break;

		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
