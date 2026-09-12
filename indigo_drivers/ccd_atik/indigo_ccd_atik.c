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

// This file generated from indigo_ccd_atik.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

//+ include

#include <math.h>
#include <limits.h>
#include <stdbool.h>
#include <indigo/indigo_usb_utils.h>
#include "AtikCameras.h"

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_ccd_driver.h>
#include <indigo/indigo_guider_driver.h>
#include <indigo/indigo_wheel_driver.h>
#include <indigo/indigo_uni_io.h>
#include <indigo/indigo_usb_utils.h>

#include "indigo_ccd_atik.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000024
#define DRIVER_NAME          "indigo_ccd_atik"
#define DRIVER_LABEL         "Atik Camera"
#define CCD_DEVICE_NAME      "%s"
#define GUIDER_DEVICE_NAME   "%s (guider)"
#define WHEEL_DEVICE_NAME    "%s (wheel)"
#define MAX_DEVICES          5
#define PRIVATE_DATA         ((atik_private_data *)device->private_data)

//+ define

#define ATIK_GUIDE_NORTH     0x01
#define ATIK_GUIDE_SOUTH     0x02
#define ATIK_GUIDE_EAST      0x04
#define ATIK_GUIDE_WEST      0x08
#define ATIK_READOUT_TIMEOUT 120
#define ATIK_FLUSH_TIMEOUT   30
#define ATIK_WHEEL_TIMEOUT   60

//- define

#pragma mark - Property definitions

#define X_PRESETS_PROPERTY             (PRIVATE_DATA->x_presets_property)
#define X_PRESETS_CUSTOM_ITEM          (X_PRESETS_PROPERTY->items + 0)
#define X_PRESETS_LOW_ITEM             (X_PRESETS_PROPERTY->items + 1)
#define X_PRESETS_MED_ITEM             (X_PRESETS_PROPERTY->items + 2)
#define X_PRESETS_HIGH_ITEM            (X_PRESETS_PROPERTY->items + 3)

#define X_PRESETS_PROPERTY_NAME        "X_PRESETS"
#define X_PRESETS_CUSTOM_ITEM_NAME     "CUSTOM"
#define X_PRESETS_LOW_ITEM_NAME        "LOW"
#define X_PRESETS_MED_ITEM_NAME        "MED"
#define X_PRESETS_HIGH_ITEM_NAME       "HIGH"

#define X_WINDOW_HEATER_PROPERTY        (PRIVATE_DATA->x_window_heater_property)
#define X_WINDOW_HEATER_POWER_ITEM      (X_WINDOW_HEATER_PROPERTY->items + 0)

#define X_WINDOW_HEATER_PROPERTY_NAME   "X_WINDOW_HEATER"
#define X_WINDOW_HEATER_POWER_ITEM_NAME "POWER"

#pragma mark - Private data definition

typedef struct {
	int count;
	libusb_device *usbdev;
	indigo_property *x_presets_property;
	indigo_property *x_window_heater_property;
	//+ data
	ArtemisHandle handle;
	char serial[100];
	char guider_name[INDIGO_NAME_SIZE], wheel_name[INDIGO_NAME_SIZE];
	bool has_guider, has_wheel, has_cooler, has_shutter;
	unsigned char *buffer;
	size_t buffer_size;
	int relay_mask;
	bool acquisition_active, exposure_started, readout_pending;
	double exposure_duration, exposure_deadline, wheel_deadline;
	int exp_left, exp_top, exp_width, exp_height, exp_bx, exp_by;
	int preset;
	//- data
} atik_private_data;

#pragma mark - Low level code

static indigo_queue *driver_queue = NULL;
static pthread_mutex_t driver_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

//+ code

static void ccd_exposure_handler(indigo_device *device);
static void exposure_finalizer(indigo_device *device);
static bool atik_serial(int index, char serial[100]) {
	memset(serial, 0, 100);
	return ArtemisDeviceSerial(index, serial) && memchr(serial, 0, 100) != NULL && serial[0] != 0;
}

static void debug_log(const char *message) {
	indigo_debug("%s: SDK - %s", DRIVER_NAME, message);
}

static bool atik_open(indigo_device *device) {
	if (indigo_try_global_lock(device) != INDIGO_OK) {
		return false;
	}
	int count = ArtemisDeviceCount();
	for (int index = 0; index < count; index++) {
		char serial[100];
		if (atik_serial(index, serial) && !strcmp(serial, PRIVATE_DATA->serial)) {
			PRIVATE_DATA->handle = ArtemisConnect(index);
			break;
		}
	}
	if (!PRIVATE_DATA->handle) {
		indigo_global_unlock(device);
		return false;
	}
	return true;
}

static void atik_close(indigo_device *device) {
	ArtemisDisconnect(PRIVATE_DATA->handle);
	PRIVATE_DATA->handle = NULL;
	indigo_global_unlock(device);
}

static int atik_stop_exposure(indigo_device *device) {
	int result = ArtemisStopExposure(PRIVATE_DATA->handle);
	if (result == ARTEMIS_OK) {
		// Some models finish downloading the stopped exposure before becoming idle.
		PRIVATE_DATA->readout_pending = true;
	}
	return result;
}

static bool atik_option(indigo_device *device, int id, uint16_t *values, int length) {
	int actual = 0;
	return ArtemisCameraSpecificOptionGetData(PRIVATE_DATA->handle, id, (unsigned char *)values, length, &actual) == ARTEMIS_OK && actual == length;
}

static bool atik_gain_offset(indigo_device *device) {
	uint16_t gain[3] = { 0 }, offset[3] = { 0 };
	if (!atik_option(device, 5, gain, sizeof(gain)) || !atik_option(device, 6, offset, sizeof(offset)) || gain[0] > gain[1] || gain[2] < gain[0] || gain[2] > gain[1] || offset[0] > offset[1] || offset[2] < offset[0] || offset[2] > offset[1]) {
		return false;
	}
	CCD_GAIN_ITEM->number.min = gain[0];
	CCD_GAIN_ITEM->number.max = gain[1];
	CCD_GAIN_ITEM->number.value = CCD_GAIN_ITEM->number.target = gain[2];
	CCD_OFFSET_ITEM->number.min = offset[0];
	CCD_OFFSET_ITEM->number.max = offset[1];
	CCD_OFFSET_ITEM->number.value = CCD_OFFSET_ITEM->number.target = offset[2];
	CCD_GAIN_PROPERTY->state = CCD_OFFSET_PROPERTY->state = INDIGO_OK_STATE;
	return true;
}

static bool atik_initialize_ccd(indigo_device *device) {
	struct ARTEMISPROPERTIES info = { 0 };
	int bx = 0, by = 0;
	if (ArtemisProperties(PRIVATE_DATA->handle, &info) != ARTEMIS_OK || info.nPixelsX <= 0 || info.nPixelsY <= 0 || !isfinite(info.PixelMicronsX) || !isfinite(info.PixelMicronsY) || info.PixelMicronsX <= 0 || info.PixelMicronsY <= 0 || ArtemisGetMaxBin(PRIVATE_DATA->handle, &bx, &by) != ARTEMIS_OK || bx < 1 || by < 1 || bx > info.nPixelsX || by > info.nPixelsY) {
		return false;
	}
	PRIVATE_DATA->has_shutter = (info.cameraflags & ARTEMIS_PROPERTIES_CAMERAFLAGS_HAS_SHUTTER) != 0;
	// Preserve the 383-family active-area workaround.
	if (info.nPixelsX == 3354 && info.nPixelsY == 2529) {
		info.nPixelsX = 3326;
		info.nPixelsY = 2504;
	}
	if ((size_t)info.nPixelsX > (SIZE_MAX - FITS_HEADER_SIZE) / 2 / info.nPixelsY) {
		return false;
	}
	CCD_INFO_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.target = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max = info.nPixelsX;
	CCD_INFO_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.target = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max = info.nPixelsY;
	CCD_FRAME_LEFT_ITEM->number.value = CCD_FRAME_LEFT_ITEM->number.target = CCD_FRAME_TOP_ITEM->number.value = CCD_FRAME_TOP_ITEM->number.target = 0;
	CCD_INFO_PIXEL_SIZE_ITEM->number.value = CCD_INFO_PIXEL_WIDTH_ITEM->number.value = round(info.PixelMicronsX * 100) / 100;
	CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = round(info.PixelMicronsY * 100) / 100;
	CCD_BIN_HORIZONTAL_ITEM->number.max = CCD_INFO_MAX_HORIZONAL_BIN_ITEM->number.value = bx;
	CCD_BIN_VERTICAL_ITEM->number.max = CCD_INFO_MAX_VERTICAL_BIN_ITEM->number.value = by;
	CCD_BIN_HORIZONTAL_ITEM->number.value = CCD_BIN_HORIZONTAL_ITEM->number.target = CCD_BIN_VERTICAL_ITEM->number.value = CCD_BIN_VERTICAL_ITEM->number.target = 1;
	int modes = 0;
	for (int bin = 1; bin <= bx && bin <= by; bin *= 2) {
		modes++;
		if (bin > INT_MAX / 2) {
			break;
		}
	}
	CCD_MODE_PROPERTY = indigo_resize_property(CCD_MODE_PROPERTY, modes);
	CCD_MODE_PROPERTY->perm = INDIGO_RW_PERM;
	for (int i = 0, bin = 1; i < modes; i++, bin *= 2) {
		char name[32], label[64];
		snprintf(name, sizeof(name), "BIN_%dx%d", bin, bin);
		snprintf(label, sizeof(label), "RAW 16 %dx%d", info.nPixelsX / bin, info.nPixelsY / bin);
		indigo_init_switch_item(CCD_MODE_PROPERTY->items + i, name, label, i == 0);
	}
	CCD_TEMPERATURE_PROPERTY->hidden = CCD_COOLER_PROPERTY->hidden = CCD_COOLER_POWER_PROPERTY->hidden = true;
	CCD_GAIN_PROPERTY->hidden = CCD_OFFSET_PROPERTY->hidden = X_PRESETS_PROPERTY->hidden = X_WINDOW_HEATER_PROPERTY->hidden = true;
	CCD_TEMPERATURE_PROPERTY->perm = INDIGO_RO_PERM;
	PRIVATE_DATA->has_cooler = false;
	int sensors = 0, temperature = 0;
	int sensor_result = ArtemisTemperatureSensorInfo(PRIVATE_DATA->handle, 0, &sensors);
	if (sensor_result != ARTEMIS_OK && sensor_result != ARTEMIS_NOT_IMPLEMENTED) {
		return false;
	}
	if (sensor_result == ARTEMIS_OK && sensors > 0) {
		if (ArtemisTemperatureSensorInfo(PRIVATE_DATA->handle, 1, &temperature) != ARTEMIS_OK) {
			return false;
		}
		CCD_TEMPERATURE_PROPERTY->hidden = false;
		CCD_TEMPERATURE_ITEM->number.value = CCD_TEMPERATURE_ITEM->number.target = round(temperature / 10.0) / 10;
		CCD_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
	}
	int flags = 0, level = 0, min = 0, max = 0, target = 0;
	int cooling_result = ArtemisCoolingInfo(PRIVATE_DATA->handle, &flags, &level, &min, &max, &target);
	if (cooling_result != ARTEMIS_OK && cooling_result != ARTEMIS_NOT_IMPLEMENTED) {
		return false;
	}
	if (cooling_result == ARTEMIS_OK && (flags & 3) == 3) {
		if (max <= min || level < min || level > max || CCD_TEMPERATURE_PROPERTY->hidden) {
			return false;
		}
		PRIVATE_DATA->has_cooler = true;
		CCD_COOLER_PROPERTY->hidden = CCD_COOLER_POWER_PROPERTY->hidden = false;
		CCD_COOLER_POWER_PROPERTY->perm = INDIGO_RO_PERM;
		CCD_TEMPERATURE_PROPERTY->perm = INDIGO_RW_PERM;
		CCD_TEMPERATURE_ITEM->number.target = target > 10000 ? CCD_TEMPERATURE_ITEM->number.value : round(target / 10.0) / 10;
		CCD_COOLER_POWER_ITEM->number.value = round(100.0 * (level - min) / (max - min));
		CCD_COOLER_POWER_PROPERTY->state = INDIGO_OK_STATE;
	}
	if (info.cameraflags & ARTEMIS_PROPERTIES_CAMERAFLAGS_HAS_WINDOW_HEATER) {
		int power = 0;
		if (ArtemisGetWindowHeaterPower(PRIVATE_DATA->handle, &power) != ARTEMIS_OK || power < 0 || power > 255) {
			return false;
		}
		X_WINDOW_HEATER_PROPERTY->hidden = false;
		X_WINDOW_HEATER_PROPERTY->state = INDIGO_OK_STATE;
		X_WINDOW_HEATER_POWER_ITEM->number.value = X_WINDOW_HEATER_POWER_ITEM->number.target = power;
	}
	if (ArtemisHasCameraSpecificOption(PRIVATE_DATA->handle, 1)) {
		uint16_t preset = 0;
		if (!atik_option(device, 1, &preset, sizeof(preset)) || preset > 3 || (preset == 0 && !atik_gain_offset(device))) {
			return false;
		}
		PRIVATE_DATA->preset = preset;
		X_PRESETS_PROPERTY->hidden = false;
		X_PRESETS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_set_switch(X_PRESETS_PROPERTY, X_PRESETS_PROPERTY->items + preset, true);
		CCD_GAIN_PROPERTY->hidden = CCD_OFFSET_PROPERTY->hidden = preset != 0;
	}
	PRIVATE_DATA->buffer_size = 2 * (size_t)info.nPixelsX * info.nPixelsY + FITS_HEADER_SIZE;
	PRIVATE_DATA->buffer = indigo_alloc_blob_buffer(PRIVATE_DATA->buffer_size);
	return PRIVATE_DATA->buffer != NULL;
}

static void atik_exposure_failure(indigo_device *device, const char *message) {
	if (PRIVATE_DATA->exposure_started) {
		PRIVATE_DATA->exposure_started = atik_stop_exposure(device) != ARTEMIS_OK;
	}
	PRIVATE_DATA->acquisition_active = false;
	CCD_EXPOSURE_ITEM->number.value = 0;
	indigo_ccd_failure_cleanup(device);
	CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "%s", message);
}

static void exposure_finalizer(indigo_device *device) {
	if (!PRIVATE_DATA->acquisition_active) {
		return;
	}
	if (indigo_monotonic_time() >= PRIVATE_DATA->exposure_deadline) {
		atik_exposure_failure(device, "Camera operation timed out");
		return;
	}
	if (!PRIVATE_DATA->exposure_started) {
		int state = ArtemisCameraState(PRIVATE_DATA->handle);
		if (state == CAMERA_FLUSHING || (PRIVATE_DATA->readout_pending && (state == CAMERA_WAITING || state == CAMERA_EXPOSING || state == CAMERA_READING || state == CAMERA_DOWNLOADING))) {
			indigo_execute_handler_in(device, .01, exposure_finalizer);
			return;
		}
		if (state == CAMERA_IDLE) {
			PRIVATE_DATA->readout_pending = false;
		}
		if (state != CAMERA_IDLE || ArtemisSetPreview(PRIVATE_DATA->handle, CCD_READ_MODE_HIGH_SPEED_ITEM->sw.value) != ARTEMIS_OK || (PRIVATE_DATA->has_shutter && ArtemisSetDarkMode(PRIVATE_DATA->handle, CCD_FRAME_TYPE_DARK_ITEM->sw.value || CCD_FRAME_TYPE_DARKFLAT_ITEM->sw.value || CCD_FRAME_TYPE_BIAS_ITEM->sw.value) != ARTEMIS_OK) || ArtemisBin(PRIVATE_DATA->handle, PRIVATE_DATA->exp_bx, PRIVATE_DATA->exp_by) != ARTEMIS_OK || ArtemisSubframe(PRIVATE_DATA->handle, PRIVATE_DATA->exp_left, PRIVATE_DATA->exp_top, PRIVATE_DATA->exp_width, PRIVATE_DATA->exp_height) != ARTEMIS_OK || ArtemisStartExposure(PRIVATE_DATA->handle, PRIVATE_DATA->exposure_duration) != ARTEMIS_OK) {
			atik_exposure_failure(device, "Exposure setup failed");
			return;
		}
		PRIVATE_DATA->exposure_started = true;
		PRIVATE_DATA->exposure_deadline = indigo_monotonic_time() + PRIVATE_DATA->exposure_duration + ATIK_READOUT_TIMEOUT;
		CCD_EXPOSURE_ITEM->number.value = CCD_EXPOSURE_ITEM->number.target = PRIVATE_DATA->exposure_duration;
		indigo_ccd_exposure_setup(device);
		indigo_execute_handler_in(device, PRIVATE_DATA->exposure_duration, exposure_finalizer);
		return;
	}
	if (!ArtemisImageReady(PRIVATE_DATA->handle)) {
		if (ArtemisCameraState(PRIVATE_DATA->handle) == CAMERA_ERROR) {
			atik_exposure_failure(device, "Camera readout failed");
		} else {
			indigo_execute_handler_in(device, .05, exposure_finalizer);
		}
		return;
	}
	int x = 0, y = 0, w = 0, h = 0, bx = 0, by = 0;
	if (ArtemisImageFailed(PRIVATE_DATA->handle) || ArtemisGetImageData(PRIVATE_DATA->handle, &x, &y, &w, &h, &bx, &by) != ARTEMIS_OK || w <= 0 || h <= 0 || bx != PRIVATE_DATA->exp_bx || by != PRIVATE_DATA->exp_by || x != PRIVATE_DATA->exp_left || y != PRIVATE_DATA->exp_top || w != PRIVATE_DATA->exp_width / bx || h != PRIVATE_DATA->exp_height / by || (size_t)w > (PRIVATE_DATA->buffer_size - FITS_HEADER_SIZE) / 2 / h) {
		atik_exposure_failure(device, "Invalid image data");
		return;
	}
	void *pixels = ArtemisImageBuffer(PRIVATE_DATA->handle);
	if (!pixels) {
		atik_exposure_failure(device, "Missing image buffer");
		return;
	}
	memcpy(PRIVATE_DATA->buffer + FITS_HEADER_SIZE, pixels, (size_t)w * h * 2);
	PRIVATE_DATA->exposure_started = PRIVATE_DATA->acquisition_active = false;
	indigo_process_image(device, PRIVATE_DATA->buffer, w, h, 16, true, true, NULL, false);
	CCD_EXPOSURE_ITEM->number.value = 0;
	CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
}

//- code

//+ guider.code

static void guider_ra_finalizer(indigo_device *device) {
	int mask = PRIVATE_DATA->relay_mask & ~(ATIK_GUIDE_EAST | ATIK_GUIDE_WEST);
	GUIDER_GUIDE_RA_PROPERTY->state = ArtemisGuidePort(PRIVATE_DATA->handle, mask) == ARTEMIS_OK ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	if (GUIDER_GUIDE_RA_PROPERTY->state == INDIGO_OK_STATE) {
		PRIVATE_DATA->relay_mask = mask;
	}
	GUIDER_GUIDE_EAST_ITEM->number.value = GUIDER_GUIDE_WEST_ITEM->number.value = 0;
	indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
}

static void guider_dec_finalizer(indigo_device *device) {
	int mask = PRIVATE_DATA->relay_mask & ~(ATIK_GUIDE_NORTH | ATIK_GUIDE_SOUTH);
	GUIDER_GUIDE_DEC_PROPERTY->state = ArtemisGuidePort(PRIVATE_DATA->handle, mask) == ARTEMIS_OK ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	if (GUIDER_GUIDE_DEC_PROPERTY->state == INDIGO_OK_STATE) {
		PRIVATE_DATA->relay_mask = mask;
	}
	GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
	indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
}

//- guider.code

//+ wheel.code

static bool atik_wheel_info(indigo_device *device, int *count, int *moving, int *current, int *target) {
	int result = ArtemisFilterWheelInfo(PRIVATE_DATA->handle, count, moving, current, target);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ArtemisFilterWheelInfo = %d, count %d, moving %d, current %d, target %d", result, *count, *moving, *current, *target);
	return result == ARTEMIS_OK && *count >= 1 && *count <= 64 && *current >= 0 && (*current < *count || (*moving && *current == *count));
}

static void wheel_move_finalizer(indigo_device *device) {
	int count = 0, moving = 0, current = 0, target = 0;
	if (!atik_wheel_info(device, &count, &moving, &current, &target) || target < 0 || target >= count || count != WHEEL_SLOT_ITEM->number.max || indigo_monotonic_time() >= PRIVATE_DATA->wheel_deadline) {
		WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		if (current < count) {
			WHEEL_SLOT_ITEM->number.value = current + 1;
		}
		if (moving) {
			WHEEL_SLOT_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_execute_handler_in(device, .5, wheel_move_finalizer);
		} else {
			WHEEL_SLOT_PROPERTY->state = current + 1 == WHEEL_SLOT_ITEM->number.target ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
		}
	}
	indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
}

//- wheel.code

#pragma mark - High level code (ccd)

static void ccd_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ ccd.on_timer
	if (PRIVATE_DATA->readout_pending) {
		int state = ArtemisCameraState(PRIVATE_DATA->handle);
		if (state != CAMERA_IDLE && state != CAMERA_ERROR) {
			indigo_execute_handler_in(device, 5, ccd_timer_callback);
			return;
		}
		PRIVATE_DATA->readout_pending = false;
	}
	// Preserve the SDK download-phase temperature exclusion; integration can still be polled.
	if (PRIVATE_DATA->acquisition_active && PRIVATE_DATA->exposure_started && indigo_monotonic_time() >= PRIVATE_DATA->exposure_deadline - ATIK_READOUT_TIMEOUT) {
		indigo_execute_handler_in(device, 5, ccd_timer_callback);
		return;
	}
	if (!CCD_TEMPERATURE_PROPERTY->hidden) {
		int temperature = 0;
		if (ArtemisTemperatureSensorInfo(PRIVATE_DATA->handle, 1, &temperature) == ARTEMIS_OK) {
			CCD_TEMPERATURE_ITEM->number.value = round(temperature / 10.0) / 10;
			CCD_TEMPERATURE_PROPERTY->state = PRIVATE_DATA->has_cooler && CCD_COOLER_ON_ITEM->sw.value && fabs(CCD_TEMPERATURE_ITEM->number.value - CCD_TEMPERATURE_ITEM->number.target) > 1 ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
		} else {
			CCD_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
	}
	if (PRIVATE_DATA->has_cooler) {
		int flags = 0, level = 0, min = 0, max = 0, target = 0;
		if (ArtemisCoolingInfo(PRIVATE_DATA->handle, &flags, &level, &min, &max, &target) == ARTEMIS_OK && max > min && level >= min && level <= max) {
			CCD_COOLER_POWER_ITEM->number.value = round(100.0 * (level - min) / (max - min));
			CCD_COOLER_POWER_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			CCD_COOLER_POWER_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, CCD_COOLER_POWER_PROPERTY, NULL);
	}
	indigo_execute_handler_in(device, 5, ccd_timer_callback);
	//- ccd.on_timer
}

static void ccd_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = atik_open(device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ ccd.on_connect
			indigo_lock_master_device(device);
			connection_result = atik_initialize_ccd(device);
			indigo_unlock_master_device(device);
			//- ccd.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, X_PRESETS_PROPERTY, NULL);
			indigo_define_property(device, X_WINDOW_HEATER_PROPERTY, NULL);
			indigo_execute_handler(device, ccd_timer_callback);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s", device->name);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				atik_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ ccd.on_disconnect
		indigo_lock_master_device(device);
		if (PRIVATE_DATA->exposure_started) {
			atik_stop_exposure(device);
		}
		PRIVATE_DATA->exposure_started = PRIVATE_DATA->acquisition_active = false;
		if (PRIVATE_DATA->has_cooler) {
			ArtemisCoolerWarmUp(PRIVATE_DATA->handle);
		}
		indigo_safe_free(PRIVATE_DATA->buffer);
		PRIVATE_DATA->buffer = NULL;
		PRIVATE_DATA->buffer_size = 0;
		indigo_unlock_master_device(device);
		//- ccd.on_disconnect
		indigo_delete_property(device, X_PRESETS_PROPERTY, NULL);
		indigo_delete_property(device, X_WINDOW_HEATER_PROPERTY, NULL);
		if (--PRIVATE_DATA->count == 0) {
			atik_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_ccd_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void ccd_bin_handler(indigo_device *device) {
	CCD_BIN_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_BIN.on_change
	char name[32];
	snprintf(name, sizeof(name), "BIN_%dx%d", (int)CCD_BIN_HORIZONTAL_ITEM->number.value, (int)CCD_BIN_VERTICAL_ITEM->number.value);
	for (int i = 0; i < CCD_MODE_PROPERTY->count; i++) {
		CCD_MODE_PROPERTY->items[i].sw.value = !strcmp(name, CCD_MODE_PROPERTY->items[i].name);
	}
	indigo_update_property(device, CCD_MODE_PROPERTY, NULL);
	//- ccd.CCD_BIN.on_change
	indigo_update_property(device, CCD_BIN_PROPERTY, NULL);
}

static void ccd_exposure_handler(indigo_device *device) {
	//+ ccd.CCD_EXPOSURE.on_change
	if (PRIVATE_DATA->exposure_started && atik_stop_exposure(device) != ARTEMIS_OK) {
		atik_exposure_failure(device, "Previous exposure could not be stopped");
		return;
	}
	indigo_use_shortest_exposure_if_bias(device);
	PRIVATE_DATA->exposure_duration = CCD_EXPOSURE_ITEM->number.target;
	PRIVATE_DATA->exp_left = CCD_FRAME_LEFT_ITEM->number.value;
	PRIVATE_DATA->exp_top = CCD_FRAME_TOP_ITEM->number.value;
	PRIVATE_DATA->exp_width = CCD_FRAME_WIDTH_ITEM->number.value;
	PRIVATE_DATA->exp_height = CCD_FRAME_HEIGHT_ITEM->number.value;
	PRIVATE_DATA->exp_bx = CCD_BIN_HORIZONTAL_ITEM->number.value;
	PRIVATE_DATA->exp_by = CCD_BIN_VERTICAL_ITEM->number.value;
	PRIVATE_DATA->acquisition_active = true;
	PRIVATE_DATA->exposure_started = false;
	PRIVATE_DATA->exposure_deadline = indigo_monotonic_time() + (PRIVATE_DATA->readout_pending ? ATIK_READOUT_TIMEOUT : ATIK_FLUSH_TIMEOUT);
	CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
	exposure_finalizer(device);
	//- ccd.CCD_EXPOSURE.on_change
}

static void ccd_abort_exposure_handler(indigo_device *device) {
	//+ ccd.CCD_ABORT_EXPOSURE.on_change
	indigo_cancel_pending_handler(device, ccd_exposure_handler);
	indigo_cancel_pending_handler(device, exposure_finalizer);
	int result = PRIVATE_DATA->exposure_started ? atik_stop_exposure(device) : ARTEMIS_OK;
	PRIVATE_DATA->exposure_started = result != ARTEMIS_OK;
	PRIVATE_DATA->acquisition_active = false;
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		CCD_EXPOSURE_ITEM->number.value = 0;
		CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_ccd_failure_cleanup(device);
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Exposure aborted");
	}
	CCD_ABORT_EXPOSURE_ITEM->sw.value = false;
	CCD_ABORT_EXPOSURE_PROPERTY->state = result == ARTEMIS_OK ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	indigo_update_property(device, CCD_ABORT_EXPOSURE_PROPERTY, NULL);
	//- ccd.CCD_ABORT_EXPOSURE.on_change
}

static void ccd_cooler_handler(indigo_device *device) {
	CCD_COOLER_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_COOLER.on_change
	int result = CCD_COOLER_ON_ITEM->sw.value ? ArtemisSetCooling(PRIVATE_DATA->handle, (int)round(CCD_TEMPERATURE_ITEM->number.target * 100)) : ArtemisCoolerWarmUp(PRIVATE_DATA->handle);
	if (result != ARTEMIS_OK) {
		CCD_COOLER_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- ccd.CCD_COOLER.on_change
	indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
}

static void ccd_temperature_handler(indigo_device *device) {
	CCD_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_TEMPERATURE.on_change
	if (ArtemisSetCooling(PRIVATE_DATA->handle, (int)round(CCD_TEMPERATURE_ITEM->number.target * 100)) == ARTEMIS_OK) {
		indigo_set_switch(CCD_COOLER_PROPERTY, CCD_COOLER_ON_ITEM, true);
		CCD_COOLER_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
		CCD_TEMPERATURE_PROPERTY->state = INDIGO_BUSY_STATE;
	} else {
		CCD_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- ccd.CCD_TEMPERATURE.on_change
	indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
}

static void ccd_gain_handler(indigo_device *device) {
	CCD_GAIN_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_GAIN.on_change
	int value = CCD_GAIN_ITEM->number.target;
	if (ArtemisCameraSpecificOptionSetData(PRIVATE_DATA->handle, 5, (unsigned char *)&value, sizeof(value)) != ARTEMIS_OK) {
		CCD_GAIN_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		CCD_GAIN_ITEM->number.value = value;
	}
	//- ccd.CCD_GAIN.on_change
	indigo_update_property(device, CCD_GAIN_PROPERTY, NULL);
}

static void ccd_offset_handler(indigo_device *device) {
	CCD_OFFSET_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_OFFSET.on_change
	int value = CCD_OFFSET_ITEM->number.target;
	if (ArtemisCameraSpecificOptionSetData(PRIVATE_DATA->handle, 6, (unsigned char *)&value, sizeof(value)) != ARTEMIS_OK) {
		CCD_OFFSET_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		CCD_OFFSET_ITEM->number.value = value;
	}
	//- ccd.CCD_OFFSET.on_change
	indigo_update_property(device, CCD_OFFSET_PROPERTY, NULL);
}

static void ccd_x_presets_handler(indigo_device *device) {
	X_PRESETS_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.X_PRESETS.on_change
	uint16_t preset = 0;
	for (int i = 0; i < 4; i++) {
		if (X_PRESETS_PROPERTY->items[i].sw.value) {
			preset = i;
		}
	}
	if (ArtemisCameraSpecificOptionSetData(PRIVATE_DATA->handle, 1, (unsigned char *)&preset, sizeof(preset)) != ARTEMIS_OK) {
		indigo_set_switch(X_PRESETS_PROPERTY, X_PRESETS_PROPERTY->items + PRIVATE_DATA->preset, true);
		X_PRESETS_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		PRIVATE_DATA->preset = preset;
		indigo_delete_property(device, CCD_GAIN_PROPERTY, NULL);
		indigo_delete_property(device, CCD_OFFSET_PROPERTY, NULL);
		CCD_GAIN_PROPERTY->hidden = CCD_OFFSET_PROPERTY->hidden = true;
		if (preset == 0) {
			if (atik_gain_offset(device)) {
				CCD_GAIN_PROPERTY->hidden = CCD_OFFSET_PROPERTY->hidden = false;
				indigo_define_property(device, CCD_GAIN_PROPERTY, NULL);
				indigo_define_property(device, CCD_OFFSET_PROPERTY, NULL);
			} else {
				X_PRESETS_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		}
	}
	//- ccd.X_PRESETS.on_change
	indigo_update_property(device, X_PRESETS_PROPERTY, NULL);
}

static void ccd_x_window_heater_handler(indigo_device *device) {
	X_WINDOW_HEATER_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.X_WINDOW_HEATER.on_change
	int power = X_WINDOW_HEATER_POWER_ITEM->number.target;
	if (ArtemisSetWindowHeaterPower(PRIVATE_DATA->handle, power) != ARTEMIS_OK) {
		X_WINDOW_HEATER_PROPERTY->state = INDIGO_ALERT_STATE;
		if (ArtemisGetWindowHeaterPower(PRIVATE_DATA->handle, &power) == ARTEMIS_OK && power >= 0 && power <= 255) {
			X_WINDOW_HEATER_POWER_ITEM->number.value = power;
		}
	} else {
		X_WINDOW_HEATER_POWER_ITEM->number.value = power;
	}
	//- ccd.X_WINDOW_HEATER.on_change
	indigo_update_property(device, X_WINDOW_HEATER_PROPERTY, NULL);
}

#pragma mark - Device API (ccd)

static indigo_result ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result ccd_attach(indigo_device *device) {
	if (indigo_ccd_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		//+ ccd.on_attach
		CCD_EXPOSURE_ITEM->number.min = .001;
		CCD_BIN_PROPERTY->perm = INDIGO_RW_PERM;
		CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = 16;
		CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.target = 16;
		//- ccd.on_attach
		CCD_READ_MODE_PROPERTY->hidden = false;
		CCD_BIN_PROPERTY->hidden = false;
		CCD_EXPOSURE_PROPERTY->hidden = false;
		CCD_ABORT_EXPOSURE_PROPERTY->hidden = false;
		CCD_COOLER_PROPERTY->hidden = false;
		CCD_TEMPERATURE_PROPERTY->hidden = false;
		CCD_GAIN_PROPERTY->hidden = false;
		CCD_OFFSET_PROPERTY->hidden = false;
		X_PRESETS_PROPERTY = indigo_init_switch_property(NULL, device->name, X_PRESETS_PROPERTY_NAME, CCD_MAIN_GROUP, "Gain/offset presets", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 4);
		if (X_PRESETS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_PRESETS_CUSTOM_ITEM, X_PRESETS_CUSTOM_ITEM_NAME, "Custom", true);
		indigo_init_switch_item(X_PRESETS_LOW_ITEM, X_PRESETS_LOW_ITEM_NAME, "Low", false);
		indigo_init_switch_item(X_PRESETS_MED_ITEM, X_PRESETS_MED_ITEM_NAME, "Medium", false);
		indigo_init_switch_item(X_PRESETS_HIGH_ITEM, X_PRESETS_HIGH_ITEM_NAME, "High", false);
		X_PRESETS_PROPERTY->hidden = true;
		X_WINDOW_HEATER_PROPERTY = indigo_init_number_property(NULL, device->name, X_WINDOW_HEATER_PROPERTY_NAME, CCD_MAIN_GROUP, "Window heater", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (X_WINDOW_HEATER_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_WINDOW_HEATER_POWER_ITEM, X_WINDOW_HEATER_POWER_ITEM_NAME, "Power", 0, 255, 1, 0);
		X_WINDOW_HEATER_PROPERTY->hidden = true;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_PRESETS_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_WINDOW_HEATER_PROPERTY);
	}
	return indigo_ccd_enumerate_properties(device, client, property);
}

static indigo_result ccd_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			indigo_queue_add(driver_queue, device, INDIGO_TASK_PRIORITY_NORMAL, 0, ccd_connection_handler, &driver_queue_mutex);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_BIN_PROPERTY, property)) {
		//+ ccd.CCD_BIN.on_change_request
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
			CCD_BIN_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_BIN_PROPERTY, "Exposure in progress");
			return INDIGO_OK;
		}
		//- ccd.CCD_BIN.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_BIN_PROPERTY, ccd_bin_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_EXPOSURE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_EXPOSURE_PROPERTY, ccd_exposure_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(CCD_ABORT_EXPOSURE_PROPERTY, ccd_abort_exposure_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_COOLER_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_COOLER_PROPERTY, ccd_cooler_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_TEMPERATURE_PROPERTY, property)) {
		//+ ccd.CCD_TEMPERATURE.on_change_request
		CCD_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
		//- ccd.CCD_TEMPERATURE.on_change_request
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(CCD_TEMPERATURE_PROPERTY, ccd_temperature_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_GAIN_PROPERTY, property)) {
		//+ ccd.CCD_GAIN.on_change_request
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
			CCD_GAIN_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_GAIN_PROPERTY, "Exposure in progress");
			return INDIGO_OK;
		}
		//- ccd.CCD_GAIN.on_change_request
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(CCD_GAIN_PROPERTY, ccd_gain_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_OFFSET_PROPERTY, property)) {
		//+ ccd.CCD_OFFSET.on_change_request
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
			CCD_OFFSET_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_OFFSET_PROPERTY, "Exposure in progress");
			return INDIGO_OK;
		}
		//- ccd.CCD_OFFSET.on_change_request
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(CCD_OFFSET_PROPERTY, ccd_offset_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_PRESETS_PROPERTY, property)) {
		//+ ccd.X_PRESETS.on_change_request
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
			X_PRESETS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, X_PRESETS_PROPERTY, "Exposure in progress");
			return INDIGO_OK;
		}
		//- ccd.X_PRESETS.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_PRESETS_PROPERTY, ccd_x_presets_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_WINDOW_HEATER_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(X_WINDOW_HEATER_PROPERTY, ccd_x_window_heater_handler);
		return INDIGO_OK;
	}
	return indigo_ccd_change_property(device, client, property);
}

static indigo_result ccd_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		ccd_connection_handler(device);
	}
	indigo_release_property(X_PRESETS_PROPERTY);
	indigo_release_property(X_WINDOW_HEATER_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_ccd_detach(device);
}

#pragma mark - High level code (guider)

static void guider_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = atik_open(device->master_device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ guider.on_connect
			indigo_lock_master_device(device);
			connection_result = ArtemisGuidePort(PRIVATE_DATA->handle, 0) == ARTEMIS_OK;
			PRIVATE_DATA->relay_mask = 0;
			indigo_unlock_master_device(device);
			//- guider.on_connect
		}
		if (connection_result) {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s", device->name);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				atik_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ guider.on_disconnect
		indigo_lock_master_device(device);
		ArtemisGuidePort(PRIVATE_DATA->handle, 0);
		PRIVATE_DATA->relay_mask = 0;
		indigo_unlock_master_device(device);
		//- guider.on_disconnect
		if (--PRIVATE_DATA->count == 0) {
			atik_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_guider_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void guider_guide_ra_handler(indigo_device *device) {
	//+ guider.GUIDER_GUIDE_RA.on_change
	indigo_cancel_pending_handler(device, guider_ra_finalizer);
	int mask = PRIVATE_DATA->relay_mask & ~(ATIK_GUIDE_EAST | ATIK_GUIDE_WEST);
	double duration = GUIDER_GUIDE_EAST_ITEM->number.value;
	if (duration > 0) {
		mask |= ATIK_GUIDE_EAST;
	} else if ((duration = GUIDER_GUIDE_WEST_ITEM->number.value) > 0) {
		mask |= ATIK_GUIDE_WEST;
	}
	if (ArtemisGuidePort(PRIVATE_DATA->handle, mask) != ARTEMIS_OK) {
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		PRIVATE_DATA->relay_mask = mask;
		GUIDER_GUIDE_RA_PROPERTY->state = duration > 0 ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
		if (duration > 0) {
			indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, duration / 1000.0, guider_ra_finalizer);
		}
	}
	indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
	//- guider.GUIDER_GUIDE_RA.on_change
}

static void guider_guide_dec_handler(indigo_device *device) {
	//+ guider.GUIDER_GUIDE_DEC.on_change
	indigo_cancel_pending_handler(device, guider_dec_finalizer);
	int mask = PRIVATE_DATA->relay_mask & ~(ATIK_GUIDE_NORTH | ATIK_GUIDE_SOUTH);
	double duration = GUIDER_GUIDE_NORTH_ITEM->number.value;
	if (duration > 0) {
		mask |= ATIK_GUIDE_NORTH;
	} else if ((duration = GUIDER_GUIDE_SOUTH_ITEM->number.value) > 0) {
		mask |= ATIK_GUIDE_SOUTH;
	}
	if (ArtemisGuidePort(PRIVATE_DATA->handle, mask) != ARTEMIS_OK) {
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		PRIVATE_DATA->relay_mask = mask;
		GUIDER_GUIDE_DEC_PROPERTY->state = duration > 0 ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
		if (duration > 0) {
			indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, duration / 1000.0, guider_dec_finalizer);
		}
	}
	indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
	//- guider.GUIDER_GUIDE_DEC.on_change
}

#pragma mark - Device API (guider)

static indigo_result guider_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result guider_attach(indigo_device *device) {
	if (indigo_guider_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		GUIDER_GUIDE_RA_PROPERTY->hidden = false;
		GUIDER_GUIDE_DEC_PROPERTY->hidden = false;
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
			indigo_queue_add(driver_queue, device, INDIGO_TASK_PRIORITY_NORMAL, 0, guider_connection_handler, &driver_queue_mutex);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_RA_PROPERTY, property)) {
		//+ guider.GUIDER_GUIDE_RA.on_change_request
		// Guide requests replace a pulse on the same axis, including zero/stop.
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
		//- guider.GUIDER_GUIDE_RA.on_change_request
		INDIGO_COPY_VALUES_PROCESS_PRIORITY_CHANGE(GUIDER_GUIDE_RA_PROPERTY, guider_guide_ra_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		//+ guider.GUIDER_GUIDE_DEC.on_change_request
		// Guide requests replace a pulse on the same axis, including zero/stop.
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
		//- guider.GUIDER_GUIDE_DEC.on_change_request
		INDIGO_COPY_VALUES_PROCESS_PRIORITY_CHANGE(GUIDER_GUIDE_DEC_PROPERTY, guider_guide_dec_handler);
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

#pragma mark - High level code (wheel)

static void wheel_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = atik_open(device->master_device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ wheel.on_connect
			indigo_lock_master_device(device);
			int count = 0, moving = 0, current = 0, target = 0;
			connection_result = atik_wheel_info(device, &count, &moving, &current, &target);
			if (connection_result) {
				WHEEL_SLOT_NAME_PROPERTY = indigo_resize_property(WHEEL_SLOT_NAME_PROPERTY, count);
				WHEEL_SLOT_OFFSET_PROPERTY = indigo_resize_property(WHEEL_SLOT_OFFSET_PROPERTY, count);
				for (int i = 0; i < count; i++) {
					if (!WHEEL_SLOT_NAME_PROPERTY->items[i].name[0]) {
						char name[32], label[32];
						snprintf(name, sizeof(name), WHEEL_SLOT_NAME_ITEM_NAME, i + 1);
						snprintf(label, sizeof(label), "Slot %d", i + 1);
						indigo_init_text_item(WHEEL_SLOT_NAME_PROPERTY->items + i, name, label, "Filter #%d", i + 1);
						snprintf(name, sizeof(name), WHEEL_SLOT_OFFSET_ITEM_NAME, i + 1);
						indigo_init_number_item(WHEEL_SLOT_OFFSET_PROPERTY->items + i, name, label, -1000000, 1000000, 1, 0);
					}
				}
				WHEEL_SLOT_ITEM->number.max = count;
				if (current < count) {
					WHEEL_SLOT_ITEM->number.value = current + 1;
				}
				bool target_known = target >= 0 && target < count;
				WHEEL_SLOT_ITEM->number.target = moving && target_known ? target + 1 : WHEEL_SLOT_ITEM->number.value;
				WHEEL_SLOT_PROPERTY->state = moving ? (target_known ? INDIGO_BUSY_STATE : INDIGO_ALERT_STATE) : INDIGO_OK_STATE;
				if (moving && target_known) {
					PRIVATE_DATA->wheel_deadline = indigo_monotonic_time() + ATIK_WHEEL_TIMEOUT;
					indigo_execute_handler_in(device, .5, wheel_move_finalizer);
				}
			}
			indigo_unlock_master_device(device);
			//- wheel.on_connect
		}
		if (connection_result) {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s", device->name);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				atik_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		if (--PRIVATE_DATA->count == 0) {
			atik_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_wheel_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void wheel_slot_handler(indigo_device *device) {
	//+ wheel.WHEEL_SLOT.on_change
	if (ArtemisFilterWheelMove(PRIVATE_DATA->handle, WHEEL_SLOT_ITEM->number.target - 1) != ARTEMIS_OK) {
		WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		WHEEL_SLOT_PROPERTY->state = INDIGO_BUSY_STATE;
		PRIVATE_DATA->wheel_deadline = indigo_monotonic_time() + ATIK_WHEEL_TIMEOUT;
		indigo_execute_handler_in(device, .5, wheel_move_finalizer);
	}
	indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
	//- wheel.WHEEL_SLOT.on_change
}

#pragma mark - Device API (wheel)

static indigo_result wheel_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result wheel_attach(indigo_device *device) {
	if (indigo_wheel_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		WHEEL_SLOT_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return wheel_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result wheel_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	return indigo_wheel_enumerate_properties(device, client, property);
}

static indigo_result wheel_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			indigo_queue_add(driver_queue, device, INDIGO_TASK_PRIORITY_NORMAL, 0, wheel_connection_handler, &driver_queue_mutex);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(WHEEL_SLOT_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(WHEEL_SLOT_PROPERTY, wheel_slot_handler);
		return INDIGO_OK;
	}
	return indigo_wheel_change_property(device, client, property);
}

static indigo_result wheel_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		wheel_connection_handler(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_wheel_detach(device);
}

#pragma mark - Device templates

static indigo_device ccd_template = INDIGO_DEVICE_INITIALIZER(CCD_DEVICE_NAME, ccd_attach, ccd_enumerate_properties, ccd_change_property, NULL, ccd_detach);

static indigo_device guider_template = INDIGO_DEVICE_INITIALIZER(GUIDER_DEVICE_NAME, guider_attach, guider_enumerate_properties, guider_change_property, NULL, guider_detach);

static indigo_device wheel_template = INDIGO_DEVICE_INITIALIZER(WHEEL_DEVICE_NAME, wheel_attach, wheel_enumerate_properties, wheel_change_property, NULL, wheel_detach);

#pragma mark - Hot-plug code

static indigo_device *devices[MAX_DEVICES];

static indigo_result verify_devices_disconnected(void) {
	for (int i = 0; i < MAX_DEVICES; i++) {
		VERIFY_NOT_CONNECTED(devices[i]);
	}
	return INDIGO_OK;
}

#define SDK_DISCOVERY_RETRIES (6)
typedef struct sdk_discovery_retry {
	libusb_device *dev;
	int remaining;
	bool active, queued;
	struct sdk_discovery_retry *next;
} sdk_discovery_retry;

static sdk_discovery_retry *sdk_discovery_retries;
static bool sdk_discovery_stopping;
static void process_plug_event_handler(indigo_device *device, void *data);
static void process_sdk_retry_handler(indigo_device *device, void *data);

static void update_sdk_discovery_retry(libusb_device *dev, bool retry) {
	sdk_discovery_retry *entry = sdk_discovery_retries;
	while (entry && entry->dev != dev) {
		entry = entry->next;
	}
	if (!retry || sdk_discovery_stopping) {
		if (entry) {
			entry->active = false;
		}
		return;
	}
	if (!entry && SDK_DISCOVERY_RETRIES <= 0) {
		return;
	}
	if (!entry) {
		entry = (sdk_discovery_retry *)indigo_safe_malloc(sizeof(*entry));
		entry->dev = libusb_ref_device(dev);
		entry->remaining = SDK_DISCOVERY_RETRIES;
		entry->active = true;
		entry->next = sdk_discovery_retries;
		sdk_discovery_retries = entry;
	}
	if (entry->active && !entry->queued && entry->remaining > 0) {
		entry->remaining--;
		entry->queued = true;
		indigo_queue_add_with_data(driver_queue, NULL, INDIGO_TASK_PRIORITY_NORMAL, 0.5, process_sdk_retry_handler, entry, &driver_queue_mutex);
	}
}

static void process_sdk_retry_handler(indigo_device *device, void *data) {
	sdk_discovery_retry *entry = (sdk_discovery_retry *)data;
	entry->queued = false;
	if (entry->active && !sdk_discovery_stopping) {
		process_plug_event_handler(NULL, libusb_ref_device(entry->dev));
	}
	if (!entry->queued) {
		sdk_discovery_retry **link = &sdk_discovery_retries;
		while (*link != entry) {
			link = &(*link)->next;
		}
		*link = entry->next;
		libusb_unref_device(entry->dev);
		indigo_safe_free(entry);
	}
}

static void clear_sdk_discovery_retries(void) {
	while (sdk_discovery_retries) {
		sdk_discovery_retry *entry = sdk_discovery_retries;
		sdk_discovery_retries = entry->next;
		libusb_unref_device(entry->dev);
		indigo_safe_free(entry);
	}
}
static void process_plug_event_handler(indigo_device *device, void *data) {
	indigo_set_handler_max_run_time(1);
	libusb_device *dev = (libusb_device *)data;
	if (sdk_discovery_stopping) {
		libusb_unref_device(dev);
		return;
	}
	bool dev_ref_transferred = false;
	atik_private_data *private_data = NULL;
	bool plug_result = true;
	char name[INDIGO_NAME_SIZE] = DRIVER_LABEL;
	private_data = (atik_private_data *)indigo_safe_malloc(sizeof(atik_private_data));
	private_data->usbdev = dev;
	struct libusb_device_descriptor descriptor;
	if (libusb_get_device_descriptor(dev, &descriptor) != LIBUSB_SUCCESS) {
		plug_result = false;
	}
	bool discovery_eligible = plug_result;
	if (plug_result) {
		//+ sdk.plug
		plug_result = false;
		if (descriptor.idVendor == 0x20e7 || descriptor.idVendor == 0x04b4) {
			int slots = 0;
			for (int i = 0; i < MAX_DEVICES; i++) {
				if (!devices[i]) {
					slots++;
				}
			}
			int count = ArtemisDeviceCount();
			for (int index = 0; index < count; index++) {
				char serial[100], model[100] = { 0 };
				if (!ArtemisDeviceIsCamera(index) || !atik_serial(index, serial) || !ArtemisDeviceName(index, model) || !memchr(model, 0, sizeof(model)) || !model[0]) {
					continue;
				}
				bool found = false;
				for (int i = 0; i < MAX_DEVICES; i++) {
					if (devices[i] && !strcmp(((atik_private_data *)devices[i]->private_data)->serial, serial)) {
						found = true;
						break;
					}
				}
				if (found) {
					continue;
				}
				private_data->has_guider = ArtemisDeviceHasGuidePort(index);
				private_data->has_wheel = ArtemisDeviceHasFilterWheel(index);
				if (slots < 1 + private_data->has_guider + private_data->has_wheel) {
					continue;
				}
				memcpy(private_data->serial, serial, sizeof(serial));
				snprintf(name, INDIGO_NAME_SIZE, "%s", model);
				snprintf(private_data->guider_name, INDIGO_NAME_SIZE, "%.*s (guider)", INDIGO_NAME_SIZE - 10, model);
				snprintf(private_data->wheel_name, INDIGO_NAME_SIZE, "%.*s (wheel)", INDIGO_NAME_SIZE - 9, model);
				indigo_make_name_unique(name, "%s", serial);
				indigo_make_name_unique(private_data->guider_name, "%s", serial);
				indigo_make_name_unique(private_data->wheel_name, "%s", serial);
				plug_result = true;
				break;
			}
		}
		//- sdk.plug
	}
	if (plug_result) {
		indigo_device *ccd = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &ccd_template);
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
		}
		if (ccd_attached && private_data->has_guider) {
		indigo_device *guider = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &guider_template);
		guider->private_data = private_data;
		guider->master_device = ccd;
		snprintf(guider->name, INDIGO_NAME_SIZE, "%s", private_data->guider_name);
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
		if (ccd_attached && private_data->has_wheel) {
		indigo_device *wheel = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &wheel_template);
		wheel->private_data = private_data;
		wheel->master_device = ccd;
		snprintf(wheel->name, INDIGO_NAME_SIZE, "%s", private_data->wheel_name);
		bool wheel_attached = false;
		for (int j = 0; j < MAX_DEVICES; j++) {
			if (devices[j] == NULL) {
				devices[j] = wheel;
				if (indigo_attach_device(wheel) == INDIGO_OK) {
					dev_ref_transferred = true;
					wheel_attached = true;
				} else {
					devices[j] = NULL;
				}
				break;
			}
		}
		if (!wheel_attached) {
			indigo_safe_free(wheel);
		}
		}
	}
	update_sdk_discovery_retry(dev, discovery_eligible && !dev_ref_transferred);
	if (!dev_ref_transferred) {
		indigo_safe_free(private_data);
		libusb_unref_device(dev);
	}
}

static void process_unplug_event_handler(indigo_device *device, void *data) {
	libusb_device *dev = (libusb_device *)data;
	update_sdk_discovery_retry(dev, false);
	atik_private_data *private_data = NULL;
	atik_private_data *removed[MAX_DEVICES];
	int removed_count = 0;
	for (int j = MAX_DEVICES - 1; j >= 0; j--) {
		if (devices[j] != NULL) {
			indigo_device *device = devices[j];
			private_data = PRIVATE_DATA;
			bool unplug_result = private_data->usbdev == dev;
			if (last_action != INDIGO_DRIVER_SHUTDOWN) {
				//+ sdk.unplug_match
				int count = ArtemisDeviceCount();
				unplug_result = count >= 0;
				for (int index = 0; index < count; index++) {
					char serial[100];
					if (!atik_serial(index, serial) || !strcmp(serial, private_data->serial)) {
						unplug_result = false;
						break;
					}
				}
				//- sdk.unplug_match
			}
			if (unplug_result) {
				private_data = PRIVATE_DATA;
				indigo_detach_device(device);
				indigo_safe_free(device);
				devices[j] = NULL;
				bool recorded = false;
				for (int k = 0; k < removed_count; k++) {
					if (removed[k] == private_data) {
						recorded = true;
						break;
					}
				}
				if (!recorded) {
					removed[removed_count++] = private_data;
				}
			}
		}
	}
	for (int k = 0; k < removed_count; k++) {
		libusb_unref_device(removed[k]->usbdev);
		indigo_safe_free(removed[k]);
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

indigo_result indigo_ccd_atik(indigo_driver_action action, indigo_driver_info *info) {

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			//+ on_init
			ArtemisSetDebugCallback(debug_log);
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Artemis SDK %d, API %d", ArtemisDLLVersion(), ArtemisAPIVersion());
			//- on_init
			for (int i = 0; i < MAX_DEVICES; i++) {
				devices[i] = NULL;
			}
			sdk_discovery_stopping = false;
			driver_queue = indigo_queue_create(NULL);
			if (driver_queue == NULL) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to create driver queue");
				last_action = INDIGO_DRIVER_SHUTDOWN;
				return INDIGO_FAILED;
			}
			indigo_queue_set_name(driver_queue, "Queue " DRIVER_LABEL);
			indigo_start_usb_event_handler();
			int rc = libusb_hotplug_register_callback(NULL, (libusb_hotplug_event)(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT), LIBUSB_HOTPLUG_ENUMERATE, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_register_callback ->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
			if (rc < 0) {
				indigo_queue_delete(&driver_queue);
				last_action = INDIGO_DRIVER_SHUTDOWN;
				return INDIGO_FAILED;
			}
			break;

		}
		case INDIGO_DRIVER_SHUTDOWN: {
			pthread_mutex_lock(&driver_queue_mutex);
			indigo_result shutdown_result = verify_devices_disconnected();
			if (shutdown_result == INDIGO_OK) {
				sdk_discovery_stopping = true;
			}
			pthread_mutex_unlock(&driver_queue_mutex);
			if (shutdown_result != INDIGO_OK) {
				return shutdown_result;
			}
			last_action = action;
			libusb_hotplug_deregister_callback(NULL, callback_handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_deregister_callback");
			indigo_queue_remove(driver_queue, NULL, (indigo_timer_callback)process_sdk_retry_handler);
			indigo_queue_drain(driver_queue);
			for (int i = 0; i < MAX_DEVICES; i++) {
				if (devices[i] != NULL) {
					indigo_device *device = devices[i];
					process_unplug_event_handler(NULL, libusb_ref_device(PRIVATE_DATA->usbdev));
				}
			}
			indigo_queue_delete(&driver_queue);
			clear_sdk_discovery_retries();
		//+ on_shutdown
		ArtemisShutdown();
		//- on_shutdown
			break;

		}
		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
