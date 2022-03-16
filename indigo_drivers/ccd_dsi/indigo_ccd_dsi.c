// Copyright (c) 2017 Rumen G. Bogdanovski
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
// 2.0 by Rumen Bogdanovski <rumen@skyarchive.org>


/** INDIGO CCD driver for Meade DSI
 \file indigo_ccd_dsi.c
 */

#define DRIVER_VERSION 0x000A
#define DRIVER_NAME		"indigo_ccd_dsi"

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


#define MAX_CCD_TEMP         45     /* Max CCD temperature */
#define MIN_CCD_TEMP        -55     /* Min CCD temperature */

#define DEFAULT_BPP          16     /* Default bits per pixel */
#define MAX_PATH            255     /* Maximal Path Length */

#define TEMP_CHECK_TIME       3     /* Time between teperature checks (seconds) */

#include <indigo/indigo_driver_xml.h>

#include "indigo_ccd_dsi.h"
#include "libdsi.h"

#define DSI_VENDOR_ID              0x156c

#define PRIVATE_DATA               ((dsi_private_data *)device->private_data)

// gp_bits is used as boolean
#define is_connected               gp_bits

#undef INDIGO_DEBUG_DRIVER
#define INDIGO_DEBUG_DRIVER(c) c

// -------------------------------------------------------------------------------- FLI USB interface implementation

typedef struct {
	char dev_sid[DSI_ID_LEN];
	enum DSI_BIN_MODE exp_bin_mode;
	dsi_camera_t *dsi;
	indigo_timer *exposure_timer, *temperature_timer;
	long int buffer_size;
	char *buffer;
	pthread_mutex_t usb_mutex;
	bool can_check_temperature;
} dsi_private_data;


static bool camera_open(indigo_device *device) {
	if (device->is_connected) return false;

	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);

	if (indigo_try_global_lock(device) != INDIGO_OK) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_try_global_lock(): failed to get lock.");
		return false;
	}

	PRIVATE_DATA->dsi = dsi_open_camera(PRIVATE_DATA->dev_sid);
	if (PRIVATE_DATA->dsi == NULL) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "dsi_open_camera(%s) = %p", PRIVATE_DATA->dev_sid, PRIVATE_DATA->dsi);
		return false;
	}

	if (PRIVATE_DATA->buffer == NULL) {
		PRIVATE_DATA->buffer_size = dsi_get_frame_width(PRIVATE_DATA->dsi) *
		                            dsi_get_frame_height(PRIVATE_DATA->dsi) *
		                            dsi_get_bytespp(PRIVATE_DATA->dsi) +
		                            FITS_HEADER_SIZE;
		PRIVATE_DATA->buffer = (char*)indigo_alloc_blob_buffer(PRIVATE_DATA->buffer_size);
		if (PRIVATE_DATA->buffer == NULL) {
			dsi_close_camera(PRIVATE_DATA->dsi);
			PRIVATE_DATA->dsi = NULL;
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			return true;
		}
	}

	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	return true;
}


static bool camera_start_exposure(indigo_device *device, double exposure, bool dark, int binning) {
	long res;
	enum DSI_BIN_MODE bin_mode = (binning > 1) ? BIN2X2 : BIN1X1;

	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);

	if (dsi_get_max_binning(PRIVATE_DATA->dsi) > 1) {
		res = dsi_set_binning(PRIVATE_DATA->dsi, bin_mode);
		if (res) {
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "dsi_set_binning(%s, %d) = %d", PRIVATE_DATA->dev_sid, bin_mode, res);
			return false;
		}
	}

	res = dsi_start_exposure(PRIVATE_DATA->dsi, exposure);
	if (res) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "dsi_start_exposure(%s) = %d", PRIVATE_DATA->dev_sid, res);
		return false;
	}

	PRIVATE_DATA->exp_bin_mode = bin_mode;
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	return true;
}


static bool camera_read_pixels(indigo_device *device) {
	long res;
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	dsi_set_image_little_endian(PRIVATE_DATA->dsi, 0);
	while ((res = dsi_read_image(PRIVATE_DATA->dsi, (unsigned char*)(PRIVATE_DATA->buffer + FITS_HEADER_SIZE), O_NONBLOCK)) != 0) {
		if (res == EWOULDBLOCK) {
			double time_left = dsi_get_exposure_time_left(PRIVATE_DATA->dsi);
			INDIGO_DRIVER_DEBUG(stderr, "Image not ready, sleeping for %.3fs...\n", time_left);
			indigo_usleep((int)(time_left * ONE_SECOND_DELAY));
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Exposure Failed! dsi_read_image(%s) = %d", PRIVATE_DATA->dev_sid, res);
			dsi_abort_exposure(PRIVATE_DATA->dsi);
			dsi_reset_camera(PRIVATE_DATA->dsi);
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			return false;
		}
	}
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	return true;
}


static bool camera_abort_exposure(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);

	indigo_cancel_timer(device, &PRIVATE_DATA->exposure_timer);
	dsi_abort_exposure(PRIVATE_DATA->dsi);
	dsi_reset_camera(PRIVATE_DATA->dsi);
	PRIVATE_DATA->can_check_temperature = true;

	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	return true;
}


static void camera_close(indigo_device *device) {
	if (!device->is_connected) return;

	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	dsi_close_camera(PRIVATE_DATA->dsi);
	indigo_global_unlock(device);
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	if (PRIVATE_DATA->buffer != NULL) {
		free(PRIVATE_DATA->buffer);
		PRIVATE_DATA->buffer = NULL;
	}
}

// -------------------------------------------------------------------------------- INDIGO CCD device implementation

// callback for image download
static void exposure_timer_callback(indigo_device *device) {
	PRIVATE_DATA->exposure_timer = NULL;
	if (!CONNECTION_CONNECTED_ITEM->sw.value) return;
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		CCD_EXPOSURE_ITEM->number.value = 0;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		if (camera_read_pixels(device)) {
			int binning = (PRIVATE_DATA->exp_bin_mode == BIN1X1) ? 1 : 2;
			const char *color_string = dsi_get_bayer_pattern(PRIVATE_DATA->dsi);
			if (color_string[0] != '\0') {
				/* NOTE: There is no need to take care about the offsets,
				   the SDK takes care the image to be in the correct bayer pattern */
				indigo_fits_keyword keywords[] = {
					{ INDIGO_FITS_STRING, "BAYERPAT", .string = color_string, "Bayer color pattern" },
					{ INDIGO_FITS_NUMBER, "XBAYROFF", .number = 0, "X offset of Bayer array" },
					{ INDIGO_FITS_NUMBER, "YBAYROFF", .number = 0, "Y offset of Bayer array" },
					{ 0 }
				};
				indigo_process_image(
					device,
					PRIVATE_DATA->buffer,
					(int)(CCD_FRAME_WIDTH_ITEM->number.value / binning),
					(int)(CCD_FRAME_HEIGHT_ITEM->number.value / binning),
					DEFAULT_BPP,
					true, true, keywords, false
				);
			} else {
				indigo_process_image(
					device,
					PRIVATE_DATA->buffer,
					(int)(CCD_FRAME_WIDTH_ITEM->number.value / binning),
					(int)(CCD_FRAME_HEIGHT_ITEM->number.value / binning),
					DEFAULT_BPP,
					true, true, NULL, false
				);
			}
			CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		} else {
			CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Exposure failed");
		}
	}
	PRIVATE_DATA->can_check_temperature = true;
}


// callback called 4s before image download (e.g. to clear vreg or turn off temperature check)
static void clear_reg_timer_callback(indigo_device *device) {
	if (!CONNECTION_CONNECTED_ITEM->sw.value) return;
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		PRIVATE_DATA->can_check_temperature = false;
		indigo_set_timer(device, 4, exposure_timer_callback, &PRIVATE_DATA->exposure_timer);
	} else {
		PRIVATE_DATA->exposure_timer = NULL;
	}
}


static void ccd_temperature_callback(indigo_device *device) {
	if (!CONNECTION_CONNECTED_ITEM->sw.value) return;
	if (PRIVATE_DATA->can_check_temperature) {
		// check temperature;
		CCD_TEMPERATURE_ITEM->number.value = dsi_get_temperature(PRIVATE_DATA->dsi);
		indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
	}
	indigo_reschedule_timer(device, TEMP_CHECK_TIME, &PRIVATE_DATA->temperature_timer);
}


static indigo_result ccd_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_ccd_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		pthread_mutex_init(&PRIVATE_DATA->usb_mutex, NULL);

		/* Use all info property fields */
		INFO_PROPERTY->count = 8;

		return indigo_ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}


static bool handle_exposure_property(indigo_device *device, indigo_property *property) {
	long ok;

	ok = camera_start_exposure(
		device,
		CCD_EXPOSURE_ITEM->number.target,
		CCD_FRAME_TYPE_DARK_ITEM->sw.value || CCD_FRAME_TYPE_DARKFLAT_ITEM->sw.value || CCD_FRAME_TYPE_BIAS_ITEM->sw.value,
		CCD_BIN_VERTICAL_ITEM->number.value
	);

	if (ok) {
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
		if (CCD_EXPOSURE_ITEM->number.target > 4) {
			indigo_set_timer(device, CCD_EXPOSURE_ITEM->number.target - 4, clear_reg_timer_callback, &PRIVATE_DATA->exposure_timer);
		} else {
			PRIVATE_DATA->can_check_temperature = false;
			indigo_set_timer(device, CCD_EXPOSURE_ITEM->number.target, exposure_timer_callback, &PRIVATE_DATA->exposure_timer);
		}

	} else {
		CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Exposure failed.");
	}
	return false;
}

static void ccd_connect_callback(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (!device->is_connected) {
			if (camera_open(device)) {
				pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
				CCD_INFO_WIDTH_ITEM->number.value = dsi_get_frame_width(PRIVATE_DATA->dsi);
				CCD_INFO_HEIGHT_ITEM->number.value = dsi_get_frame_height(PRIVATE_DATA->dsi);
				CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max = CCD_INFO_WIDTH_ITEM->number.value;
				CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max = CCD_INFO_HEIGHT_ITEM->number.value;

				sprintf(INFO_DEVICE_SERIAL_NUM_ITEM->text.value, "%s", dsi_get_serial_number(PRIVATE_DATA->dsi));
				sprintf(INFO_DEVICE_MODEL_ITEM->text.value, "%s", dsi_get_model_name(PRIVATE_DATA->dsi));

				indigo_update_property(device, INFO_PROPERTY, NULL);

				CCD_INFO_PIXEL_WIDTH_ITEM->number.value = dsi_get_pixel_width(PRIVATE_DATA->dsi);
				CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = dsi_get_pixel_height(PRIVATE_DATA->dsi);
				CCD_INFO_PIXEL_SIZE_ITEM->number.value = CCD_INFO_PIXEL_WIDTH_ITEM->number.value;
				CCD_INFO_MAX_HORIZONAL_BIN_ITEM->number.value = 1;
				CCD_INFO_MAX_VERTICAL_BIN_ITEM->number.value = 1;

				CCD_FRAME_PROPERTY->perm = INDIGO_RO_PERM;
				CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = DEFAULT_BPP;
				CCD_FRAME_BITS_PER_PIXEL_ITEM->number.min = DEFAULT_BPP;
				CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = DEFAULT_BPP;

				char name[32];
				if (dsi_get_max_binning(PRIVATE_DATA->dsi) > 1) {
					CCD_BIN_PROPERTY->hidden = false;
					CCD_BIN_PROPERTY->perm = INDIGO_RW_PERM;

					CCD_MODE_PROPERTY->perm = INDIGO_RW_PERM;
					CCD_MODE_PROPERTY->count = 2;
					sprintf(name, "RAW 16 %dx%d", dsi_get_frame_width(PRIVATE_DATA->dsi), dsi_get_frame_height(PRIVATE_DATA->dsi));
					indigo_init_switch_item(CCD_MODE_ITEM, "BIN_1x1", name, true);
					sprintf(name, "RAW 16 %dx%d", dsi_get_frame_width(PRIVATE_DATA->dsi)/2, dsi_get_frame_height(PRIVATE_DATA->dsi)/2);
					indigo_init_switch_item(CCD_MODE_ITEM+1, "BIN_2x2", name, false);
				} else {
					CCD_BIN_PROPERTY->hidden = true;  // keep it hidden as device does not support binning!
					CCD_BIN_PROPERTY->perm = INDIGO_RO_PERM;

					CCD_MODE_PROPERTY->perm = INDIGO_RW_PERM;
					CCD_MODE_PROPERTY->count = 1;
					sprintf(name, "RAW 16 %dx%d", dsi_get_frame_width(PRIVATE_DATA->dsi), dsi_get_frame_height(PRIVATE_DATA->dsi));
					indigo_init_switch_item(CCD_MODE_ITEM, "BIN_1x1", name, true);
				}
				CCD_BIN_HORIZONTAL_ITEM->number.value = CCD_BIN_HORIZONTAL_ITEM->number.min = 1;
				CCD_BIN_HORIZONTAL_ITEM->number.max = dsi_get_max_binning(PRIVATE_DATA->dsi);
				CCD_BIN_VERTICAL_ITEM->number.value = CCD_BIN_VERTICAL_ITEM->number.min = 1;
				CCD_BIN_VERTICAL_ITEM->number.max = dsi_get_max_binning(PRIVATE_DATA->dsi);

				CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = DEFAULT_BPP;

				CCD_TEMPERATURE_PROPERTY->hidden = false;
				CCD_TEMPERATURE_PROPERTY->perm = INDIGO_RO_PERM;
				CCD_TEMPERATURE_ITEM->number.min = MIN_CCD_TEMP;
				CCD_TEMPERATURE_ITEM->number.max = MAX_CCD_TEMP;
				CCD_TEMPERATURE_ITEM->number.step = 0;

				CCD_GAIN_PROPERTY->hidden = false;
				CCD_GAIN_PROPERTY->perm = INDIGO_RW_PERM;
				CCD_GAIN_ITEM->number.min = 0;
				CCD_GAIN_ITEM->number.max = 100;
				CCD_GAIN_ITEM->number.value = dsi_get_amp_gain(PRIVATE_DATA->dsi);

				CCD_OFFSET_PROPERTY->hidden = false;
				CCD_OFFSET_PROPERTY->perm = INDIGO_RW_PERM;
				CCD_OFFSET_ITEM->number.min = 0;
				CCD_OFFSET_ITEM->number.max = 100;
				CCD_OFFSET_ITEM->number.value = dsi_get_amp_offset(PRIVATE_DATA->dsi);

				double temp = dsi_get_temperature(PRIVATE_DATA->dsi);
				pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);

				device->is_connected = true;
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;

				if (temp > 1000) {  /* no sensor */
					CCD_TEMPERATURE_PROPERTY->hidden = true;
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "dsi_get_temperature(%s) = NO_SENSOR", PRIVATE_DATA->dev_sid);
				} else {
					PRIVATE_DATA->can_check_temperature = true;
					indigo_set_timer(device, 0, ccd_temperature_callback, &PRIVATE_DATA->temperature_timer);
				}
			} else {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		}
	} else {
		if (device->is_connected) {
			PRIVATE_DATA->can_check_temperature = false;
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->temperature_timer);
			camera_close(device);
			device->is_connected = false;
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	}
	indigo_ccd_change_property(device, NULL, CONNECTION_PROPERTY);
}

static indigo_result ccd_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);

	// -------------------------------------------------------------------------------- CONNECTION -> CCD_INFO, CCD_COOLER, CCD_TEMPERATURE
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, ccd_connect_callback, NULL);
		return INDIGO_OK;
	// -------------------------------------------------------------------------------- CCD_EXPOSURE
	} else if (indigo_property_match(CCD_EXPOSURE_PROPERTY, property)) {
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE)
			return INDIGO_OK;
		indigo_property_copy_values(CCD_EXPOSURE_PROPERTY, property, false);
		if (IS_CONNECTED) {
			indigo_use_shortest_exposure_if_bias(device);
			handle_exposure_property(device, property);
		}
	// -------------------------------------------------------------------------------- CCD_ABORT_EXPOSURE
	} else if (indigo_property_match(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
			camera_abort_exposure(device);
		}
		PRIVATE_DATA->can_check_temperature = true;
		indigo_property_copy_values(CCD_ABORT_EXPOSURE_PROPERTY, property, false);
	// ------------------------------------------------------------------------------- GAIN
	} else if (indigo_property_match(CCD_GAIN_PROPERTY, property)) {
		if (!IS_CONNECTED) return INDIGO_OK;
		CCD_GAIN_PROPERTY->state = INDIGO_OK_STATE;
		indigo_property_copy_values(CCD_GAIN_PROPERTY, property, false);

		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		dsi_set_amp_gain(PRIVATE_DATA->dsi, (int)(CCD_GAIN_ITEM->number.value));
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);

		CCD_GAIN_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, CCD_GAIN_PROPERTY, NULL);
		return INDIGO_OK;
	// -------------------------------------------------------------------------------- CCD_BIN
	} else if (indigo_property_match_w(CCD_BIN_PROPERTY, property)) {
		if (!IS_CONNECTED) return INDIGO_OK;
		int prev_bin_x = (int)CCD_BIN_HORIZONTAL_ITEM->number.value;
		int prev_bin_y = (int)CCD_BIN_VERTICAL_ITEM->number.value;

		indigo_property_copy_values(CCD_BIN_PROPERTY, property, false);

		/* DSI requires BIN_X and BIN_Y to be equal, so keep them entangled */
		if ((int)CCD_BIN_HORIZONTAL_ITEM->number.value != prev_bin_x) {
			CCD_BIN_VERTICAL_ITEM->number.value = CCD_BIN_HORIZONTAL_ITEM->number.value;
		} else if ((int)CCD_BIN_VERTICAL_ITEM->number.value != prev_bin_y) {
			CCD_BIN_HORIZONTAL_ITEM->number.value = CCD_BIN_VERTICAL_ITEM->number.value;
		}
		/* let the base base class handle the rest with the manipulated property values */
		return indigo_ccd_change_property(device, client, CCD_BIN_PROPERTY);
	// ------------------------------------------------------------------------------- CCD_OFFSET
	} else if (indigo_property_match(CCD_OFFSET_PROPERTY, property)) {
		if (!IS_CONNECTED) return INDIGO_OK;
		CCD_OFFSET_PROPERTY->state = INDIGO_OK_STATE;
		indigo_property_copy_values(CCD_OFFSET_PROPERTY, property, false);

		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		dsi_set_amp_offset(PRIVATE_DATA->dsi, (int)(CCD_OFFSET_ITEM->number.value));
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);

		CCD_OFFSET_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, CCD_OFFSET_PROPERTY, NULL);
		return INDIGO_OK;
	}
	// -----------------------------------------------------------------------------
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


// -------------------------------------------------------------------------------- hot-plug support
static pthread_mutex_t device_mutex = PTHREAD_MUTEX_INITIALIZER;

#define MAX_DEVICES                   32
#define NOT_FOUND                    (-1)

static indigo_device *devices[MAX_DEVICES] = {NULL};


static bool find_plugged_device_sid(char *new_sid) {
	int i;
	bool found = false;
	dsi_device_list dev_list;

	int count = dsi_scan_usb(dev_list);
	for(i = 0; i < count; i++) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME,"+ %d of %d: %s", i , count, dev_list[i]);
		found = false;
		for(int slot = 0; slot < MAX_DEVICES; slot++) {
			indigo_device *device = devices[slot];
			if (device == NULL) continue;
			if (PRIVATE_DATA && (!strncmp(PRIVATE_DATA->dev_sid, dev_list[i], DSI_ID_LEN))) {
				found = true;
				break;
			}
		}

		if (!found) {
			strncpy(new_sid, dev_list[i], DSI_ID_LEN);
			return true;
		}
	}
	new_sid[0] = '\0';
	return false;
}


static int find_available_device_slot() {
	for(int slot = 0; slot < MAX_DEVICES; slot++) {
		if (devices[slot] == NULL) return slot;
	}
	return NOT_FOUND;
}

/*
static int find_device_slot(const char *sid) {
	for(int slot = 0; slot < MAX_DEVICES; slot++) {
		indigo_device *device = devices[slot];
		if (device == NULL) continue;
		if (!strncmp(PRIVATE_DATA->dev_sid, sid, DSI_ID_LEN)) return slot;
	}
	return NOT_FOUND;
}
*/

static int find_unplugged_device_slot() {
	int slot;
	indigo_device *device;
	bool found = true;
	dsi_device_list dev_list;

	int count = dsi_scan_usb(dev_list);
	for(slot = 0; slot < MAX_DEVICES; slot++) {
		device = devices[slot];
		if (device == NULL) continue;
		found = false;
		for(int i = 0; i < count; i++) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME,"- %d of %d: %s", i , count, dev_list[i]);
			if (PRIVATE_DATA && (!strncmp(PRIVATE_DATA->dev_sid, dev_list[i], DSI_ID_LEN))) {
				found = true;
				break;
			}
		}
		if (!found) return slot;
	}
	return NOT_FOUND;
}


static void process_plug_event(indigo_device *unusued) {
	static indigo_device ccd_template = INDIGO_DEVICE_INITIALIZER(
		"",
		ccd_attach,
		indigo_ccd_enumerate_properties,
		ccd_change_property,
		NULL,
		ccd_detach
	);
#ifdef __APPLE__
  if (dsi_load_firmware()) {
		return;
	}
#endif
	pthread_mutex_lock(&device_mutex);
	int slot = find_available_device_slot();
	if (slot < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "No device slots available.");
		pthread_mutex_unlock(&device_mutex);
		return;
	}
	char sid[DSI_ID_LEN];
	bool found = find_plugged_device_sid(sid);
	if (!found) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "No plugged device found.");
		pthread_mutex_unlock(&device_mutex);
		return;
	}
	char dev_name[DSI_NAME_LEN + 1];
#ifdef __APPLE__
	strcpy(dev_name, "Meade DSI");
#else
// doesn't work on macOS, dsi_open_camera resets the device what leads to duplicate plug/unplug
	dsi_camera_t *dsi;
	dsi = dsi_open_camera(sid);
	if(dsi == NULL) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Camera %s can not be open.", sid);
		pthread_mutex_unlock(&device_mutex);
		return;
	}
	strncpy(dev_name, dsi_get_model_name(dsi), DSI_NAME_LEN);
	dsi_close_camera(dsi);
#endif
	indigo_device *device = (indigo_device*)malloc(sizeof(indigo_device));
	assert(device != NULL);
	memcpy(device, &ccd_template, sizeof(indigo_device));
	sprintf(device->name, "%s #%s", dev_name, sid);
	INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
	dsi_private_data *private_data = indigo_safe_malloc(sizeof(dsi_private_data));
	assert(private_data);
	memset(private_data, 0, sizeof(dsi_private_data));
	sprintf(private_data->dev_sid, "%s", sid);
	device->private_data = private_data;
	indigo_attach_device(device);
	devices[slot]=device;
	pthread_mutex_unlock(&device_mutex);
}


static void process_unplug_event(indigo_device *unusued) {
	int slot;
	bool removed = false;
	dsi_private_data *private_data = NULL;
	pthread_mutex_lock(&device_mutex);
	while ((slot = find_unplugged_device_slot()) != NOT_FOUND) {
		indigo_device **device = &devices[slot];
		if (*device == NULL) {
			pthread_mutex_unlock(&device_mutex);
			return;
		}
		indigo_detach_device(*device);
		if ((*device)->private_data) {
			private_data = (dsi_private_data*)((*device)->private_data);
		}
		free(*device);
		*device = NULL;
		removed = true;
	}
	if (private_data) {
		if (private_data->buffer != NULL) {
			free(private_data->buffer);
			private_data->buffer = NULL;
		}
		free(private_data);
		private_data = NULL;
	}
	if (!removed) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "No DSI Camera unplugged!");
	}
	pthread_mutex_unlock(&device_mutex);
}

static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	struct libusb_device_descriptor descriptor;
	libusb_get_device_descriptor(dev, &descriptor);
	if (descriptor.idVendor != DSI_VENDOR_ID) {
		return 0;
	}
	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Hot plug: vid=%x pid=%x", descriptor.idVendor, descriptor.idProduct);
			indigo_set_timer(NULL, 1, process_plug_event, NULL);
			break;
		}
		case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Hot unplug: vid=%x pid=%x", descriptor.idVendor, descriptor.idProduct);
			indigo_set_timer(NULL, 1, process_unplug_event, NULL);
			break;
		}
	}
	return 0;
};


static void remove_all_devices() {
	int i;
	for(i = 0; i < MAX_DEVICES; i++) {
		indigo_device **device = &devices[i];
		if (*device == NULL)
			continue;
		indigo_detach_device(*device);
		camera_close(*device);
		free((*device)->private_data);
		free(*device);
		*device = NULL;
	}
}


static libusb_hotplug_callback_handle callback_handle;

indigo_result indigo_ccd_dsi(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "Meade DSI Camera", __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;
		indigo_start_usb_event_handler();
		int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, DSI_VENDOR_ID, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_register_callback ->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
		return rc >= 0 ? INDIGO_OK : INDIGO_FAILED;

	case INDIGO_DRIVER_SHUTDOWN:
		for (int i = 0; i < MAX_DEVICES; i++)
			VERIFY_NOT_CONNECTED(devices[i]);
		last_action = action;
		libusb_hotplug_deregister_callback(NULL, callback_handle);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_deregister_callback");
		remove_all_devices();
		break;

	case INDIGO_DRIVER_INFO:
		break;
	}

	return INDIGO_OK;
}
