// Copyright (c) 2021-2026 CloudMakers, s. r. o.
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

// This file generated from indigo_ccd_playerone.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

//+ include

#include <time.h>
#include <math.h>
#include <assert.h>
#include "PlayerOneCamera.h"
#include <limits.h>

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_ccd_driver.h>
#include <indigo/indigo_guider_driver.h>
#include <indigo/indigo_uni_io.h>
#include <indigo/indigo_usb_utils.h>

#include "indigo_ccd_playerone.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000012
#define DRIVER_NAME          "indigo_ccd_playerone"
#define DRIVER_LABEL         "Player One Camera"
#define CCD_DEVICE_NAME      "%s"
#define GUIDER_DEVICE_NAME   "%s (guider)"
#define MAX_DEVICES          5
#define PRIVATE_DATA         ((playerone_private_data *)device->private_data)

//+ define

#define POA_DEFAULT_BANDWIDTH 45
#define POA_MAX_FORMATS      4
#define POA_VENDOR_ID        0xa0a0
#define RAW8_NAME            "RAW 8"
#define RGB24_NAME           "RGB 24"
#define RAW16_NAME           "RAW 16"
#define MONO8_NAME           "MONO 8"
#define POA_HIGHEST_DR_NAME  "POA_HIGHEST_DR"
#define POA_UNITY_GAIN_NAME  "POA_UNITY_GAIN"
#define POA_LOWEST_RN_NAME   "POA_LOWEST_RN"
#define POA_GAIN_HCG_NAME    "POA_GAIN_HCG"
#define us2s(s)              ((s) / 1000000.0)
#define s2us(us)             ((us) * 1000000)
#if !defined(INDIGO_MACOS)
#define POA_SAFE_READOUT     1
#endif
#define POA_ENABLE_LONG_EXPOSURES 1

//- define

#pragma mark - Property definitions

#define X_PIXEL_FORMAT_PROPERTY        (PRIVATE_DATA->x_pixel_format_property)

#define X_PIXEL_FORMAT_PROPERTY_NAME   "X_PIXEL_FORMAT"

#define X_ADVANCED_PROPERTY            (PRIVATE_DATA->x_advanced_property)

#define X_ADVANCED_PROPERTY_NAME       "X_ADVANCED"

#define X_PRESETS_PROPERTY             (PRIVATE_DATA->x_presets_property)
#define POA_HIGHEST_DR_ITEM            (X_PRESETS_PROPERTY->items + 0)
#define POA_UNITY_GAIN_ITEM            (X_PRESETS_PROPERTY->items + 1)
#define POA_LOWEST_RN_ITEM             (X_PRESETS_PROPERTY->items + 2)
#define POA_GAIN_HCG_ITEM              (X_PRESETS_PROPERTY->items + 3)

#define X_PRESETS_PROPERTY_NAME        "X_PRESETS"
#define POA_HIGHEST_DR_ITEM_NAME       "POA_HIGHEST_DR"
#define POA_UNITY_GAIN_ITEM_NAME       "POA_UNITY_GAIN"
#define POA_LOWEST_RN_ITEM_NAME        "POA_LOWEST_RN"
#define POA_GAIN_HCG_ITEM_NAME         "POA_GAIN_HCG"

#define X_CUSTOM_SUFFIX_PROPERTY       (PRIVATE_DATA->x_custom_suffix_property)
#define X_CUSTOM_SUFFIX_ITEM           (X_CUSTOM_SUFFIX_PROPERTY->items + 0)

#define X_CUSTOM_SUFFIX_PROPERTY_NAME  "X_CUSTOM_SUFFIX"
#define X_CUSTOM_SUFFIX_ITEM_NAME      "SUFFIX"

#define X_SENSOR_MODE_PROPERTY         (PRIVATE_DATA->x_sensor_mode_property)

#define X_SENSOR_MODE_PROPERTY_NAME    "X_SENSOR_MODE"

#pragma mark - Private data definition

typedef struct {
	int count;
	libusb_device *usbdev;
	indigo_property *x_pixel_format_property;
	indigo_property *x_advanced_property;
	indigo_property *x_presets_property;
	indigo_property *x_custom_suffix_property;
	indigo_property *x_sensor_mode_property;
	//+ data
	char model[256];
	int dev_id;
	int exp_bin;
	int exp_frame_width, exp_frame_height;
	int exp_bpp;
	bool exp_uses_bayer_pattern;
	char *bayer_pattern;
	bool acquisition_active, streaming;
	double exposure_end;
	double target_temperature, current_temperature;
	double cooler_power;
	unsigned char *buffer;
	long int buffer_size;
	bool can_check_temperature, has_temperature_sensor;
	POACameraProperties property;
	int gain_highest_dr;
	int offset_highest_dr;
	int gain_unity_gain;
	int offset_unity_gain;
	int gain_lowerst_rn;
	int offset_lowest_rn;
	int gain_hcg;
	int offset_hcg;
	char camera_name[INDIGO_NAME_SIZE], guider_name[INDIGO_NAME_SIZE];
	//- data
} playerone_private_data;

#pragma mark - Low level code

static indigo_queue *driver_queue = NULL;
static pthread_mutex_t driver_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

//+ code

static void ccd_exposure_handler(indigo_device *device);
static void ccd_streaming_handler(indigo_device *device);

static void acquisition_finalizer(indigo_device *device);

static int get_pixel_depth(indigo_device *device) {
	int item = 0;
	while (item < POA_MAX_FORMATS) {
		if (X_PIXEL_FORMAT_PROPERTY->items[item].sw.value) {
			if (!strcmp(X_PIXEL_FORMAT_PROPERTY->items[item].name, RAW8_NAME)) {
				return 8;
			}
			if (!strcmp(X_PIXEL_FORMAT_PROPERTY->items[item].name, RGB24_NAME)) {
				return 24;
			}
			if (!strcmp(X_PIXEL_FORMAT_PROPERTY->items[item].name, RAW16_NAME)) {
				return 16;
			}
			if (!strcmp(X_PIXEL_FORMAT_PROPERTY->items[item].name, MONO8_NAME)) {
				return 8;
			}
		}
		item++;
	}
	return 8;
}

static int get_pixel_format(indigo_device *device) {
	int item = 0;
	while (item < POA_MAX_FORMATS) {
		if (X_PIXEL_FORMAT_PROPERTY->items[item].sw.value) {
			if (!strcmp(X_PIXEL_FORMAT_PROPERTY->items[item].name, RAW8_NAME)) {
				PRIVATE_DATA->exp_uses_bayer_pattern = PRIVATE_DATA->property.isColorCamera;
				return POA_RAW8;
			}
			if (!strcmp(X_PIXEL_FORMAT_PROPERTY->items[item].name, RGB24_NAME)) {
				PRIVATE_DATA->exp_uses_bayer_pattern = false;
				return POA_RGB24;
			}
			if (!strcmp(X_PIXEL_FORMAT_PROPERTY->items[item].name, RAW16_NAME)) {
				PRIVATE_DATA->exp_uses_bayer_pattern = PRIVATE_DATA->property.isColorCamera;
				return POA_RAW16;
			}
			if (!strcmp(X_PIXEL_FORMAT_PROPERTY->items[item].name, MONO8_NAME)) {
				PRIVATE_DATA->exp_uses_bayer_pattern = false;
				return POA_MONO8;
			}
		}
		item++;
	}
	return POA_END;
}

static bool pixel_format_supported(indigo_device *device, POAImgFormat type) {
	for (int i = 0; i < POA_MAX_FORMATS; i++) {
		if (PRIVATE_DATA->property.imgFormats[i] == POA_END) {
			return false;
		}
		if (type == PRIVATE_DATA->property.imgFormats[i]) {
			return true;
		}
	}
	return false;
}

static bool playerone_setup_exposure(indigo_device *device, double exposure, int frame_left, int frame_top, int frame_width, int frame_height, int bin) {
	int id = PRIVATE_DATA->dev_id;
	POAErrors res;
	/* Always stop exposure before modifying any parameters. Just to be safe. */
	res = POAStopExposure(id);
	if (res) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "POAStopExposure(%d) > %d", id, res);
		return false;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAStopExposure(%d)", id);
	int c_bin = 0;
	if (POAGetImageBin(id, &c_bin) != POA_OK) {
		return false;
	}
	if (c_bin != bin) {
		res = POASetImageBin(id, bin);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "POASetImageBin(%d, %d) > %d", id, bin, res);
			return false;
		}
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POASetImageBin(%d, %d)", id, bin);
	}
	int c_fw = 0, c_fh = 0;
	if (POAGetImageSize(id, &c_fw, &c_fh) != POA_OK) {
		return false;
	}
	int fw = frame_width / bin;
	int fh = frame_height / bin;
	if (c_fw != fw || c_fh != fh) {
		res = POASetImageSize(id, fw, fh);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "POASetImageSize(%d, %d, %d) > %d", id, fw, fh, res);
			return false;
		}
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POASetImageSize(%d, %d, %d)", id, fw, fh);
	}
	int c_fl = 0, c_ft = 0;
	if (POAGetImageStartPos(id, &c_fl, &c_ft) != POA_OK) {
		return false;
	}
	int fl = frame_left / bin;
	int ft = frame_top / bin;
	if (c_fl != fl || c_ft != ft) {
		res = POASetImageStartPos(id, fl, ft);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "POASetImageStartPos(%d, %d, %d) > %d", id, fl, ft, res);
			return false;
		}
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POASetImageStartPos(%d, %d, %d)", id, fl, ft);
	}
	int pf = get_pixel_format(device);
	res = POASetImageFormat(id, pf);
	if (res) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "POASetImageFormat(%d, %d) > %d", id, pf, res);
		return false;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POASetImageFormat(%d, %d)", id, pf);
#ifdef POA_ENABLE_LONG_EXPOSURES
	POAConfigValue exposure_value = { .floatValue = exposure };
	res = POASetConfig(id, POA_EXP, exposure_value, POA_FALSE);
	if (res) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "POASetConfig(%d, POA_EXP, %f) > %d", id, exposure_value.floatValue, res);
		return false;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POASetConfig(%d, POA_EXP, %f)", id, exposure_value.floatValue);
#else
	POAConfigValue exposure_value = { .intValue = (long)s2us(exposure) };
	res = POASetConfig(id, POA_EXPOSURE, exposure_value, POA_FALSE);
	if (res) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "POASetConfig(%d, POA_EXPOSURE, %ld) > %d", id, exposure_value.intValue, res);
		return false;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POASetConfig(%d, POA_EXPOSURE, %ld)", id, exposure_value.intValue);
#endif /* POA_ENABLE_LONG_EXPOSURES */
	PRIVATE_DATA->exp_bin = bin;
	if (POAGetImageSize(id, &PRIVATE_DATA->exp_frame_width, &PRIVATE_DATA->exp_frame_height) != POA_OK) {
		return false;
	}
	PRIVATE_DATA->exp_frame_width *= bin;
	PRIVATE_DATA->exp_frame_height *= bin;
	PRIVATE_DATA->exp_bpp = (int)CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value;
	return true;
}

static bool playerone_set_cooler(indigo_device *device, bool status, double target, double *current, double *power) {
	POAErrors res;
	POABool unused;
	POAConfigValue value;
	int id = PRIVATE_DATA->dev_id;
	if (PRIVATE_DATA->has_temperature_sensor) {
		res = POAGetConfig(id, POA_TEMPERATURE, &value, &unused);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "POAGetConfig(%d, POA_CURRENT_TEMPERATURE) > %d", id, res);
			return false;
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetConfig(%d, POA_CURRENT_TEMPERATURE, > %g)", id, value.floatValue);
		}
		*current = value.floatValue;
	} else {
		*current = 0;
	}
	if (!PRIVATE_DATA->property.isHasCooler) {
		return true;
	}
	res = POAGetConfig(id, POA_COOLER, &value, &unused);
	if (res) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "POAGetConfig(%d, POA_COOLER) > %d", id, res);
		return false;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetConfig(%d, POA_COOLER, > %s)", id, value.boolValue ? "true" : "false");
	if (value.boolValue != status) {
		value.boolValue = status;
		res = POASetConfig(id, POA_COOLER, value, false);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "POASetConfig(%d, POA_COOLER, %s) > %d", id, value.boolValue ? "true" : "false", res);
			return false;
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POASetConfig(%d, POA_COOLER, %s)", id, value.boolValue ? "true" : "false");
		}
		value.intValue = status ? 100 : 0;
		res = POASetConfig(id, POA_FAN_POWER, value, false);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "POASetConfig(%d, POA_FAN_POWER, %d) > %d", id, value.intValue, res);
			return false;
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POASetConfig(%d, POA_FAN_POWER, %d)", id, value.intValue);
		}
	} else if (status) {
		res = POAGetConfig(id, POA_TARGET_TEMP, &value, &unused);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "POAGetConfig(%d, POA_TARGET_TEMP) > %d", id, res);
			return false;
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetConfig(%d, POA_TARGET_TEMP, > %d)", id, value.intValue);
		}
		if ((int)target != value.intValue) {
			value.intValue = (int)target;
			res = POASetConfig(id, POA_TARGET_TEMP, value, false);
			if (res) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "POASetConfig(%d, POA_TARGET_TEMP, %d) > %d", id, value.intValue, res);
				return false;
			} else {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POASetConfig(%d, POA_TARGET_TEMP, %d)", id, value.intValue);
			}
		}
	}
	res = POAGetConfig(id, POA_COOLER_POWER, &value, &unused);
	if (res) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "POAGetConfig(%d, POA_COOLER_POWER) > %d", id, res);
		return false;
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetConfig(%d, POA_COOLER_POWER, > %d)", id, value.intValue);
	}
	*power = value.intValue;
	return true;
}

static double playerone_now(void) {
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	return now.tv_sec + now.tv_nsec / 1000000000.0;
}

static void acquisition_finish(indigo_device *device, bool failed, bool aborted) {
	if (!PRIVATE_DATA->acquisition_active) {
		return;
	}
	PRIVATE_DATA->acquisition_active = false;
	indigo_cancel_pending_handler(device, acquisition_finalizer);
	POAErrors result = POAStopExposure(PRIVATE_DATA->dev_id);
	failed |= result != POA_OK;
	PRIVATE_DATA->can_check_temperature = true;
	indigo_property *property = PRIVATE_DATA->streaming ? CCD_STREAMING_PROPERTY : CCD_EXPOSURE_PROPERTY;
	if (PRIVATE_DATA->streaming) {
		CCD_STREAMING_EXPOSURE_ITEM->number.value = 0;
		indigo_finalize_video_stream(device);
	} else {
		CCD_EXPOSURE_ITEM->number.value = 0;
	}
	if (aborted) {
		property->state = INDIGO_BUSY_STATE;
		indigo_ccd_abort_exposure_cleanup(device);
	} else {
		property->state = failed ? INDIGO_ALERT_STATE : INDIGO_OK_STATE;
		if (failed) {
			indigo_ccd_failure_cleanup(device);
		}
		indigo_update_property(device, property, failed ? "Exposure failed!" : NULL);
	}
}

static void acquisition_finalizer(indigo_device *device) {
	if (!IS_CONNECTED || !PRIVATE_DATA->acquisition_active) {
		return;
	}
	if (CCD_ABORT_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		acquisition_finish(device, false, true);
		return;
	}
	double now = playerone_now();
	double remaining = PRIVATE_DATA->exposure_end - now;
	indigo_property *property = PRIVATE_DATA->streaming ? CCD_STREAMING_PROPERTY : CCD_EXPOSURE_PROPERTY;
	indigo_item *item = PRIVATE_DATA->streaming ? CCD_STREAMING_EXPOSURE_ITEM : CCD_EXPOSURE_ITEM;
	if (remaining > 0) {
		if (PRIVATE_DATA->streaming && ceil(remaining) != ceil(item->number.value)) {
			item->number.value = ceil(remaining);
			indigo_update_property(device, property, NULL);
		}
		indigo_execute_handler_in(device, fmin(remaining, 1), acquisition_finalizer);
		return;
	}
	POAErrors result = POA_OK;
	POABool ready = POA_TRUE;
#ifdef POA_SAFE_READOUT
	result = POAImageReady(PRIVATE_DATA->dev_id, &ready);
#endif
	if (result == POA_OK && ready) {
		// Bound the SDK wait so abort and guider handlers can run between attempts.
		result = POAGetImageData(PRIVATE_DATA->dev_id, PRIVATE_DATA->buffer + FITS_HEADER_SIZE, PRIVATE_DATA->buffer_size - FITS_HEADER_SIZE, 10);
	}
	// SDK 3.10.1 on Mars-C II reports OPERATION_FAILED for a short wait without a frame.
	if (result == POA_ERROR_OPERATION_FAILED) {
		POACameraState state = STATE_CLOSED;
		if (POAGetCameraState(PRIVATE_DATA->dev_id, &state) == POA_OK && state == STATE_EXPOSING) {
			result = POA_ERROR_TIMEOUT;
		}
	}
	if ((!ready && result == POA_OK) || result == POA_ERROR_TIMEOUT) {
		if (now < PRIVATE_DATA->exposure_end + 12) {
			indigo_execute_handler_in(device, 0.01, acquisition_finalizer);
		} else {
			acquisition_finish(device, true, false);
		}
		return;
	}
	if (result != POA_OK) {
		acquisition_finish(device, true, false);
		return;
	}
	item->number.value = 0;
	indigo_fits_keyword keywords[] = { { INDIGO_FITS_STRING, "BAYERPAT", .string = PRIVATE_DATA->bayer_pattern, "Bayer color pattern" }, { 0 } };
	indigo_process_image(device, PRIVATE_DATA->buffer, PRIVATE_DATA->exp_frame_width / PRIVATE_DATA->exp_bin, PRIVATE_DATA->exp_frame_height / PRIVATE_DATA->exp_bin, PRIVATE_DATA->exp_bpp, true, false, PRIVATE_DATA->exp_uses_bayer_pattern && PRIVATE_DATA->bayer_pattern ? keywords : NULL, true);
	if (PRIVATE_DATA->streaming && CCD_STREAMING_COUNT_ITEM->number.value > 0) {
		CCD_STREAMING_COUNT_ITEM->number.value--;
	}
	if (!PRIVATE_DATA->streaming || CCD_STREAMING_COUNT_ITEM->number.value == 0) {
		acquisition_finish(device, false, false);
	} else {
		PRIVATE_DATA->exposure_end = now + CCD_STREAMING_EXPOSURE_ITEM->number.target;
		indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
		indigo_execute_handler_in(device, fmin(CCD_STREAMING_EXPOSURE_ITEM->number.target, 1), acquisition_finalizer);
	}
}

static void acquisition_start(indigo_device *device, bool streaming) {
	if (!IS_CONNECTED) {
		return;
	}
	PRIVATE_DATA->streaming = streaming;
	PRIVATE_DATA->acquisition_active = true;
	PRIVATE_DATA->can_check_temperature = false;
	double duration = streaming ? CCD_STREAMING_EXPOSURE_ITEM->number.target : CCD_EXPOSURE_ITEM->number.target;
	if (!playerone_setup_exposure(device, duration, CCD_FRAME_LEFT_ITEM->number.value, CCD_FRAME_TOP_ITEM->number.value, CCD_FRAME_WIDTH_ITEM->number.value, CCD_FRAME_HEIGHT_ITEM->number.value, CCD_BIN_HORIZONTAL_ITEM->number.value)) {
		acquisition_finish(device, true, false);
		return;
	}
	// Saturn-C workaround: use continuous SDK mode even for one INDIGO exposure.
	POAErrors result = POAStartExposure(PRIVATE_DATA->dev_id, POA_FALSE);
	PRIVATE_DATA->can_check_temperature = true;
	if (result != POA_OK) {
		acquisition_finish(device, true, false);
		return;
	}
	PRIVATE_DATA->exposure_end = playerone_now() + duration;
	indigo_execute_handler_in(device, fmin(duration, 1), acquisition_finalizer);
}

static void ccd_temperature_callback(indigo_device *device) {
	if (!CONNECTION_CONNECTED_ITEM->sw.value) {
		return;
	}
	if (PRIVATE_DATA->can_check_temperature) {
		if (playerone_set_cooler(device, CCD_COOLER_ON_ITEM->sw.value, PRIVATE_DATA->target_temperature, &PRIVATE_DATA->current_temperature, &PRIVATE_DATA->cooler_power)) {
			double diff = PRIVATE_DATA->current_temperature - PRIVATE_DATA->target_temperature;
			if (CCD_COOLER_ON_ITEM->sw.value) {
				CCD_TEMPERATURE_PROPERTY->state = fabs(diff) > 0.5 ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
			} else {
				CCD_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
			}
			CCD_TEMPERATURE_ITEM->number.value = round(PRIVATE_DATA->current_temperature * 10.0) / 10.0;
			CCD_COOLER_PROPERTY->state = INDIGO_OK_STATE;
			CCD_COOLER_POWER_PROPERTY->state = INDIGO_OK_STATE;
			CCD_COOLER_POWER_ITEM->number.value = PRIVATE_DATA->cooler_power;
			CCD_COOLER_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			CCD_COOLER_PROPERTY->state = INDIGO_ALERT_STATE;
			CCD_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
			CCD_COOLER_POWER_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
		indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
		indigo_update_property(device, CCD_COOLER_POWER_PROPERTY, NULL);
	}
	indigo_execute_handler_in(device, 5, ccd_temperature_callback);
}

static void guider_ra_finalizer(indigo_device *device) {
	POAConfigValue value = { .boolValue = false };
	POAErrors res;
	bool failed = false;
	int id = PRIVATE_DATA->dev_id;
	res = POASetConfig(PRIVATE_DATA->dev_id, POA_GUIDE_EAST, value, false);
	if (res) {
		failed = true;
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "POASetConfig(%d, POA_GUIDE_EAST, false, false) > %d", id, res);
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POASetConfig(%d, POA_GUIDE_EAST, false, false)", id);
	}
	res = POASetConfig(PRIVATE_DATA->dev_id, POA_GUIDE_WEST, value, false);
	if (res) {
		failed = true;
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "POASetConfig(%d, POA_GUIDE_WEST, false, false) > %d", id, res);
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POASetConfig(%d, POA_GUIDE_WEST, false, false)", id);
	}
	if (!CONNECTION_CONNECTED_ITEM->sw.value) {
		return;
	}
	GUIDER_GUIDE_EAST_ITEM->number.value = 0;
	GUIDER_GUIDE_WEST_ITEM->number.value = 0;
	GUIDER_GUIDE_RA_PROPERTY->state = failed ? INDIGO_ALERT_STATE : INDIGO_OK_STATE;
	indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
}

static void guider_dec_finalizer(indigo_device *device) {
	POAConfigValue value = { .boolValue = false };
	POAErrors res;
	bool failed = false;
	int id = PRIVATE_DATA->dev_id;
	res = POASetConfig(id, POA_GUIDE_NORTH, value, false);
	if (res) {
		failed = true;
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "POASetConfig(%d, POA_GUIDE_NORTH, false, false) > %d", id, res);
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POASetConfig(%d, POA_GUIDE_NORTH, false, false)", id);
	}
	res = POASetConfig(id, POA_GUIDE_SOUTH, value, false);
	if (res) {
		failed = true;
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "POASetConfig(%d, POA_GUIDE_SOUTH, false, false) > %d", id, res);
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POASetConfig(%d, POA_GUIDE_SOUTH, false, false)", id);
	}
	if (!CONNECTION_CONNECTED_ITEM->sw.value) {
		return;
	}
	GUIDER_GUIDE_NORTH_ITEM->number.value = 0;
	GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
	GUIDER_GUIDE_DEC_PROPERTY->state = failed ? INDIGO_ALERT_STATE : INDIGO_OK_STATE;
	indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
}

static indigo_result init_camera_property(indigo_device *device, POAConfigAttributes ctrl_caps) {
	int id = PRIVATE_DATA->dev_id;
	POAConfigValue value;
	POAErrors res;
	POABool unused;
#ifdef POA_ENABLE_LONG_EXPOSURES
	if (ctrl_caps.configID == POA_EXP) {
		CCD_EXPOSURE_PROPERTY->hidden = CCD_STREAMING_PROPERTY->hidden = false;
		CCD_STREAMING_SETTINGS_PROPERTY->hidden = false;
		CCD_EXPOSURE_PROPERTY->perm = CCD_STREAMING_PROPERTY->perm = INDIGO_RW_PERM;
		CCD_EXPOSURE_ITEM->number.min = CCD_STREAMING_EXPOSURE_ITEM->number.min = ctrl_caps.minValue.floatValue;
		CCD_EXPOSURE_ITEM->number.max = CCD_STREAMING_EXPOSURE_ITEM->number.max = ctrl_caps.maxValue.floatValue;
		unused = false;
		res = POAGetConfig(id, POA_EXP, &value, &unused);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "POAGetConfig(%d, POA_EXP) > %d", id, res);
			return INDIGO_FAILED;
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetConfig(%d, POA_EXP, > %f)", id, value.floatValue);
		}
		CCD_EXPOSURE_ITEM->number.value = CCD_EXPOSURE_ITEM->number.target = value.floatValue;
		return INDIGO_OK;
	}
#else
	if (ctrl_caps.configID == POA_EXPOSURE) {
		CCD_EXPOSURE_PROPERTY->hidden = CCD_STREAMING_PROPERTY->hidden = false;
		CCD_EXPOSURE_PROPERTY->perm = CCD_STREAMING_PROPERTY->perm = INDIGO_RW_PERM;
		CCD_EXPOSURE_ITEM->number.min = CCD_STREAMING_EXPOSURE_ITEM->number.min = us2s(ctrl_caps.minValue.intValue);
		CCD_EXPOSURE_ITEM->number.max = CCD_STREAMING_EXPOSURE_ITEM->number.max = us2s(ctrl_caps.maxValue.intValue);
		unused = false;
		res = POAGetConfig(id, POA_EXPOSURE, &value, &unused);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "POAGetConfig(%d, POA_EXPOSURE) > %d", id, res);
			return INDIGO_FAILED;
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetConfig(%d, POA_EXPOSURE, > %d)", id, value.intValue);
		}
		CCD_EXPOSURE_ITEM->number.value = CCD_EXPOSURE_ITEM->number.target = us2s(value.intValue);
		return INDIGO_OK;
	}
#endif /* POA_ENABLE_LONG_EXPOSURES */
	if (ctrl_caps.configID == POA_OFFSET) {
		CCD_OFFSET_PROPERTY->hidden = false;
		if (ctrl_caps.isWritable) {
			CCD_OFFSET_PROPERTY->perm = INDIGO_RW_PERM;
		} else {
			CCD_OFFSET_PROPERTY->perm = INDIGO_RO_PERM;
		}
		CCD_OFFSET_ITEM->number.min = ctrl_caps.minValue.intValue;
		CCD_OFFSET_ITEM->number.max = ctrl_caps.maxValue.intValue;
		unused = false;
		res = POAGetConfig(id, POA_OFFSET, &value, &unused);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "POAGetConfig(%d, POA_OFFSET) > %d", id, res);
			return INDIGO_FAILED;
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetConfig(%d, POA_OFFSET,  > %d)", id, value.intValue);
		}
		CCD_OFFSET_ITEM->number.value = CCD_OFFSET_ITEM->number.target = value.intValue;
		CCD_OFFSET_ITEM->number.step = 1;
		return INDIGO_OK;
	}
	if (ctrl_caps.configID == POA_GAIN) {
		CCD_GAIN_PROPERTY->hidden = false;
		if (ctrl_caps.isWritable) {
			CCD_GAIN_PROPERTY->perm = INDIGO_RW_PERM;
		} else {
			CCD_GAIN_PROPERTY->perm = INDIGO_RO_PERM;
		}
		CCD_GAIN_ITEM->number.min = ctrl_caps.minValue.intValue;
		CCD_GAIN_ITEM->number.max = ctrl_caps.maxValue.intValue;
		unused = false;
		res = POAGetConfig(id, POA_GAIN, &value, &unused);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "POAGetConfig(%d, POA_GAIN) > %d", id, res);
			return INDIGO_FAILED;
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetConfig(%d, POA_GAIN,  > %d)", id, value.intValue);
		}
		CCD_GAIN_ITEM->number.value = CCD_GAIN_ITEM->number.target = value.intValue;
		CCD_GAIN_ITEM->number.step = 1;
		return INDIGO_OK;
	}
	if (ctrl_caps.configID == POA_EGAIN) {
		CCD_EGAIN_PROPERTY->hidden = false;
		if (ctrl_caps.isWritable) {
			CCD_EGAIN_PROPERTY->perm = INDIGO_RW_PERM;
		} else {
			CCD_EGAIN_PROPERTY->perm = INDIGO_RO_PERM;
		}
		CCD_EGAIN_ITEM->number.min = ctrl_caps.minValue.floatValue;
		CCD_EGAIN_ITEM->number.max = ctrl_caps.maxValue.floatValue;
		unused = false;
		res = POAGetConfig(id, POA_EGAIN, &value, &unused);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "POAGetConfig(%d, POA_EGAIN) > %d", id, res);
			return INDIGO_FAILED;
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetConfig(%d, POA_EGAIN,  > %g)", id, value.floatValue);
		}
		CCD_EGAIN_ITEM->number.value = CCD_EGAIN_ITEM->number.target = value.floatValue;
		return INDIGO_OK;
	}
	if (ctrl_caps.configID == POA_TARGET_TEMP) {
		CCD_TEMPERATURE_PROPERTY->hidden = false;
		if (ctrl_caps.isWritable) {
			CCD_TEMPERATURE_PROPERTY->perm = INDIGO_RW_PERM;
		} else {
			CCD_TEMPERATURE_PROPERTY->perm = INDIGO_RO_PERM;
		}
		CCD_TEMPERATURE_ITEM->number.min = ctrl_caps.minValue.intValue;
		CCD_TEMPERATURE_ITEM->number.max = ctrl_caps.maxValue.intValue;
		CCD_TEMPERATURE_ITEM->number.value = CCD_TEMPERATURE_ITEM->number.target = ctrl_caps.defaultValue.intValue;
		PRIVATE_DATA->target_temperature = ctrl_caps.defaultValue.intValue;
		PRIVATE_DATA->can_check_temperature = true;
		return INDIGO_OK;
	}
	if (ctrl_caps.configID == POA_TEMPERATURE) {
		if (CCD_TEMPERATURE_PROPERTY->hidden) {
			PRIVATE_DATA->can_check_temperature = true;
			CCD_TEMPERATURE_PROPERTY->perm = INDIGO_RO_PERM;
			CCD_TEMPERATURE_PROPERTY->hidden = false;
		}
		PRIVATE_DATA->has_temperature_sensor = true;
		return INDIGO_OK;
	}
	if (ctrl_caps.configID == POA_COOLER) {
		CCD_COOLER_PROPERTY->hidden = false;
		if (ctrl_caps.isWritable) {
			CCD_COOLER_PROPERTY->perm = INDIGO_RW_PERM;
		} else {
			CCD_COOLER_PROPERTY->perm = INDIGO_RO_PERM;
		}
		return INDIGO_OK;
	}
	if (ctrl_caps.configID == POA_COOLER_POWER) {
		CCD_COOLER_POWER_PROPERTY->hidden = false;
		if (ctrl_caps.isWritable) {
			CCD_COOLER_POWER_PROPERTY->perm = INDIGO_RW_PERM;
		} else {
			CCD_COOLER_POWER_PROPERTY->perm = INDIGO_RO_PERM;
		}
		CCD_COOLER_POWER_ITEM->number.min = ctrl_caps.minValue.intValue;
		CCD_COOLER_POWER_ITEM->number.max = ctrl_caps.maxValue.intValue;
		res = POAGetConfig(id, POA_COOLER_POWER, &value, &unused);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "POAGetConfig(%d, POA_COOLER_POWER) > %d", id, res);
			return INDIGO_FAILED;
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetConfig(%d, POA_COOLER_POWER,  > %d)", id, value.intValue);
		}
		CCD_COOLER_POWER_ITEM->number.value = CCD_COOLER_POWER_ITEM->number.target = value.intValue;
		return INDIGO_OK;
	}
	if (ctrl_caps.configID == POA_AUTOEXPO_BRIGHTNESS || ctrl_caps.configID == POA_AUTOEXPO_MAX_GAIN || ctrl_caps.configID == POA_AUTOEXPO_MAX_EXPOSURE) {
		return INDIGO_OK;
	}
	if (ctrl_caps.configID == POA_FLIP_NONE || ctrl_caps.configID == POA_FLIP_HORI || ctrl_caps.configID == POA_FLIP_VERT || ctrl_caps.configID == POA_FLIP_BOTH) {
		return INDIGO_OK;
	}
	if (ctrl_caps.configID == POA_GUIDE_SOUTH || ctrl_caps.configID == POA_GUIDE_NORTH || ctrl_caps.configID == POA_GUIDE_WEST || ctrl_caps.configID == POA_GUIDE_EAST) {
		return INDIGO_OK;
	}
	if (ctrl_caps.isWritable) {
		int offset = X_ADVANCED_PROPERTY->count;
		unused = false;
		res = POAGetConfig(id, ctrl_caps.configID, &value, &unused);
		if (res == POA_OK && ctrl_caps.configID == POA_USB_BANDWIDTH_LIMIT && value.intValue == 100) {
			value.intValue = POA_DEFAULT_BANDWIDTH;
			res = POASetConfig(id, POA_USB_BANDWIDTH_LIMIT, value, false);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Default USB Bandwidth is 100, reducing to %d", value.intValue);
		}
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "POAGetConfig(%d, %s) > %d", id, ctrl_caps.szConfName, res);
			return INDIGO_FAILED;
		} else {
			X_ADVANCED_PROPERTY = indigo_resize_property(X_ADVANCED_PROPERTY, offset + 1);
			if (ctrl_caps.valueType == VAL_FLOAT) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetConfig(%d, %s, > %g)", id, ctrl_caps.szConfName, value.floatValue);
				indigo_init_number_item(X_ADVANCED_PROPERTY->items + offset, ctrl_caps.szConfName, ctrl_caps.szConfName, ctrl_caps.minValue.floatValue, ctrl_caps.maxValue.floatValue, 1, value.floatValue);
			} else if (ctrl_caps.valueType == VAL_BOOL) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetConfig(%d, %s, > %s)", id, ctrl_caps.szConfName, value.boolValue ? "true" : "false");
				indigo_init_number_item(X_ADVANCED_PROPERTY->items + offset, ctrl_caps.szConfName, ctrl_caps.szConfName, 0, 1, 1, value.boolValue ? 1 : 0);
			} else {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetConfig(%d, %s, > %d)", id, ctrl_caps.szConfName, value.intValue);
				if (!strncmp(ctrl_caps.szConfName, "WB_", 3) && ctrl_caps.minValue.intValue == 1) {
					/* workaround for white balance values being remapped in sdk 3.9.0 */
					/* 0 is mapped to 50% in the sdk to maintain backwards compatibility */
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Workaround for white balance applied for %s", ctrl_caps.szConfName);
					ctrl_caps.minValue.intValue = 0;
				}
				indigo_init_number_item(X_ADVANCED_PROPERTY->items + offset, ctrl_caps.szConfName, ctrl_caps.szConfName, ctrl_caps.minValue.intValue, ctrl_caps.maxValue.intValue, 1, value.intValue);
			}
		}
	}
	return INDIGO_OK;
}

static indigo_result init_sensor_mode_property(indigo_device *device) {
	int res;
	int id = PRIVATE_DATA->dev_id;
	int sensor_mode_count = 0;
	int current_mode = 0;
	res = POAGetSensorModeCount(id, &sensor_mode_count);
	if (res) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "POAGetSensorModeCount(%d) > %d", id, res);
		return INDIGO_FAILED;
	}
	if (sensor_mode_count <= 0) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "No sensor modes available");
		X_SENSOR_MODE_PROPERTY = indigo_resize_property(X_SENSOR_MODE_PROPERTY, 0);
		return INDIGO_NOT_FOUND;
	}
	res = POAGetSensorMode(id, &current_mode);
	if (res) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "POAGetSensorMode(%d) > %d", id, res);
		return INDIGO_FAILED;
	}
	X_SENSOR_MODE_PROPERTY = indigo_resize_property(X_SENSOR_MODE_PROPERTY, sensor_mode_count);
	for (int i = 0; i < sensor_mode_count; i++) {
		POASensorModeInfo pSenModeInfo;
		res = POAGetSensorModeInfo(id, i, &pSenModeInfo);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "POAGetSensorModeInfo(%d, %d) > %d", id, i, res);
			X_SENSOR_MODE_PROPERTY = indigo_resize_property(X_SENSOR_MODE_PROPERTY, 0);
			return INDIGO_FAILED;
		}
		pSenModeInfo.name[sizeof(pSenModeInfo.name) - 1] = 0;
		pSenModeInfo.desc[sizeof(pSenModeInfo.desc) - 1] = 0;
		indigo_init_switch_item(X_SENSOR_MODE_PROPERTY->items + i, pSenModeInfo.name, pSenModeInfo.desc, current_mode == i);
	}
	return INDIGO_OK;
}

static void adjust_preset_switches(indigo_device *device) {
	POA_HIGHEST_DR_ITEM->sw.value = false;
	POA_UNITY_GAIN_ITEM->sw.value = false;
	POA_LOWEST_RN_ITEM->sw.value = false;
	POA_GAIN_HCG_ITEM->sw.value = false;
	if (((int)CCD_GAIN_ITEM->number.value == PRIVATE_DATA->gain_highest_dr) && ((int)CCD_OFFSET_ITEM->number.value == PRIVATE_DATA->offset_highest_dr)) {
		POA_HIGHEST_DR_ITEM->sw.value = true;
	} else if (((int)CCD_GAIN_ITEM->number.value == PRIVATE_DATA->gain_lowerst_rn) && ((int)CCD_OFFSET_ITEM->number.value == PRIVATE_DATA->offset_lowest_rn)) {
		POA_LOWEST_RN_ITEM->sw.value = true;
	} else if (((int)CCD_GAIN_ITEM->number.value == PRIVATE_DATA->gain_unity_gain) && ((int)CCD_OFFSET_ITEM->number.value == PRIVATE_DATA->offset_unity_gain)) {
		POA_UNITY_GAIN_ITEM->sw.value = true;
	} else if ((int)CCD_GAIN_ITEM->number.value == PRIVATE_DATA->gain_hcg) {
		POA_GAIN_HCG_ITEM->sw.value = true;
	}
}

static bool playerone_open(indigo_device *device) {
	if (indigo_try_global_lock(device) != INDIGO_OK) {
		return false;
	}
	POAErrors result = POAOpenCamera(PRIVATE_DATA->dev_id);
	if (result != POA_OK) {
		indigo_global_unlock(device);
		return false;
	}
	result = POAInitCamera(PRIVATE_DATA->dev_id);
	if (result != POA_OK) {
		goto failed;
	}
	long width = PRIVATE_DATA->property.maxWidth;
	long height = PRIVATE_DATA->property.maxHeight;
	if (width <= 0 || height <= 0 || width > (LONG_MAX - FITS_HEADER_SIZE - 1024) / 3 / height) {
		goto failed;
	}
	PRIVATE_DATA->buffer_size = width * height * (PRIVATE_DATA->property.isColorCamera ? 3 : 2) + FITS_HEADER_SIZE + 1024;
	PRIVATE_DATA->buffer = indigo_alloc_blob_buffer(PRIVATE_DATA->buffer_size);
	if (PRIVATE_DATA->buffer != NULL) {
		return true;
	}
	failed:
	POACloseCamera(PRIVATE_DATA->dev_id);
	indigo_global_unlock(device);
	return false;
}

static void playerone_close(indigo_device *device) {
	indigo_lock_master_device(device);
	POACloseCamera(PRIVATE_DATA->dev_id);
	free(PRIVATE_DATA->buffer);
	PRIVATE_DATA->buffer = NULL;
	indigo_global_unlock(device);
	indigo_unlock_master_device(device);
}

static void initialize_properties(indigo_device *device) {
	X_PIXEL_FORMAT_PROPERTY = indigo_resize_property(X_PIXEL_FORMAT_PROPERTY, POA_MAX_FORMATS);
	// -------------------------------------------------------------------------------- X_PIXEL_FORMAT_PROPERTY
	int format_count = 0;
	if (pixel_format_supported(device, POA_RAW8)) {
		indigo_init_switch_item(X_PIXEL_FORMAT_PROPERTY->items + format_count, RAW8_NAME, RAW8_NAME, true);
		format_count++;
	}
	if (pixel_format_supported(device, POA_RGB24)) {
		indigo_init_switch_item(X_PIXEL_FORMAT_PROPERTY->items + format_count, RGB24_NAME, RGB24_NAME, false);
		format_count++;
	}
	if (pixel_format_supported(device, POA_RAW16)) {
		indigo_init_switch_item(X_PIXEL_FORMAT_PROPERTY->items + format_count, RAW16_NAME, RAW16_NAME, false);
		format_count++;
	}
	if (pixel_format_supported(device, POA_MONO8)) {
		indigo_init_switch_item(X_PIXEL_FORMAT_PROPERTY->items + format_count, MONO8_NAME, MONO8_NAME, false);
		format_count++;
	}
	X_PIXEL_FORMAT_PROPERTY->count = format_count;
	if (format_count > 0) {
		indigo_set_switch(X_PIXEL_FORMAT_PROPERTY, X_PIXEL_FORMAT_PROPERTY->items, true);
	}
	// -------------------------------------------------------------------------------- INFO
	INFO_PROPERTY->count = 8;
	snprintf(INFO_DEVICE_MODEL_ITEM->text.value, INDIGO_NAME_SIZE, "%s (%s)", PRIVATE_DATA->model, PRIVATE_DATA->property.sensorModelName);
	snprintf(INFO_DEVICE_FW_REVISION_ITEM->text.value, INDIGO_NAME_SIZE, "SDK %s, API %d", POAGetSDKVersion(), POAGetAPIVersion());
	snprintf(INFO_DEVICE_SERIAL_NUM_ITEM->text.value, INDIGO_NAME_SIZE, "%s", PRIVATE_DATA->property.SN);
	// -------------------------------------------------------------------------------- CCD_INFO
	CCD_INFO_WIDTH_ITEM->number.value = PRIVATE_DATA->property.maxWidth;
	CCD_INFO_HEIGHT_ITEM->number.value = PRIVATE_DATA->property.maxHeight;
	CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = PRIVATE_DATA->property.bitDepth;
	CCD_INFO_PIXEL_SIZE_ITEM->number.value = CCD_INFO_PIXEL_WIDTH_ITEM->number.value = CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = PRIVATE_DATA->property.pixelSize;
	CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.target = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max = PRIVATE_DATA->property.maxWidth;
	CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.target = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max = PRIVATE_DATA->property.maxHeight;
	CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.target = get_pixel_depth(device);
	CCD_FRAME_BITS_PER_PIXEL_ITEM->number.min = 8;
	CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = 24;
	/* find max binning */
	int max_bin = 1;
	for (int num = 0; (num < 8) && PRIVATE_DATA->property.bins[num]; num++) {
		max_bin = PRIVATE_DATA->property.bins[num];
	}
	CCD_BIN_PROPERTY->perm = INDIGO_RW_PERM;
	CCD_BIN_HORIZONTAL_ITEM->number.value = CCD_BIN_HORIZONTAL_ITEM->number.min = 1;
	CCD_BIN_HORIZONTAL_ITEM->number.max = max_bin;
	CCD_BIN_VERTICAL_ITEM->number.value = CCD_BIN_VERTICAL_ITEM->number.min = 1;
	CCD_BIN_VERTICAL_ITEM->number.max = max_bin;
	CCD_INFO_MAX_HORIZONAL_BIN_ITEM->number.value = max_bin;
	CCD_INFO_MAX_VERTICAL_BIN_ITEM->number.value = max_bin;
	int mode_count = 0;
	char name[32], label[64];
	for (int num = 0; (num < 8) && PRIVATE_DATA->property.bins[num]; num++) {
		int bin = PRIVATE_DATA->property.bins[num];
		if (pixel_format_supported(device, POA_RAW8)) {
			snprintf(name, 32, "%s %dx%d", RAW8_NAME, bin, bin);
			snprintf(label, 64, "%s %dx%d", RAW8_NAME, (int)CCD_FRAME_WIDTH_ITEM->number.value / bin, (int)CCD_FRAME_HEIGHT_ITEM->number.value / bin);
			indigo_init_switch_item(CCD_MODE_PROPERTY->items + mode_count, name, label, bin == 1);
			mode_count++;
		}
		if (pixel_format_supported(device, POA_RGB24)) {
			snprintf(name, 32, "%s %dx%d", RGB24_NAME, bin, bin);
			snprintf(label, 64, "%s %dx%d", RGB24_NAME, (int)CCD_FRAME_WIDTH_ITEM->number.value / bin, (int)CCD_FRAME_HEIGHT_ITEM->number.value / bin);
			indigo_init_switch_item(CCD_MODE_PROPERTY->items + mode_count, name, label, false);
			mode_count++;
		}
		if (pixel_format_supported(device, POA_RAW16)) {
			snprintf(name, 32, "%s %dx%d", RAW16_NAME, bin, bin);
			snprintf(label, 64, "%s %dx%d", RAW16_NAME, (int)CCD_FRAME_WIDTH_ITEM->number.value / bin, (int)CCD_FRAME_HEIGHT_ITEM->number.value / bin);
			indigo_init_switch_item(CCD_MODE_PROPERTY->items + mode_count, name, label, false);
			mode_count++;
		}
		if (pixel_format_supported(device, POA_MONO8)) {
			snprintf(name, 32, "%s %dx%d", MONO8_NAME, bin, bin);
			snprintf(label, 64, "%s %dx%d", MONO8_NAME, (int)CCD_FRAME_WIDTH_ITEM->number.value / bin, (int)CCD_FRAME_HEIGHT_ITEM->number.value / bin);
			indigo_init_switch_item(CCD_MODE_PROPERTY->items + mode_count, name, label, false);
			mode_count++;
		}
	}
	CCD_MODE_PROPERTY->count = mode_count;
	if (mode_count > 0) {
		indigo_set_switch(CCD_MODE_PROPERTY, CCD_MODE_PROPERTY->items, true);
	}
	// -------------------------------------------------------------------------------- X_PRESETS
	// --------------------------------------------------------------------------------- X_CUSTOM_SUFFIX
	char suffix[17] = { 0 };
	memcpy(suffix, PRIVATE_DATA->property.userCustomID, 16);
	INDIGO_COPY_VALUE(X_CUSTOM_SUFFIX_ITEM->text.value, suffix);
	// -------------------------------------------------------------------------------- CCD_STREAMING
	CCD_STREAMING_PROPERTY->hidden = false;
	CCD_STREAMING_SETTINGS_PROPERTY->hidden = false;
	CCD_IMAGE_FORMAT_PROPERTY->count = 7;
	// -------------------------------------------------------------------------------- X_ADVANCED
	// -------------------------------------------------------------------------------- X_SENSOR_MODE
	// --------------------------------------------------------------------------------
	switch (PRIVATE_DATA->property.bayerPattern) {
		case POA_BAYER_BG:
		PRIVATE_DATA->bayer_pattern = "BGGR";
		break;
		case POA_BAYER_GR:
		PRIVATE_DATA->bayer_pattern = "GRBG";
		break;
		case POA_BAYER_GB:
		PRIVATE_DATA->bayer_pattern = "GBRG";
		break;
		case POA_BAYER_RG:
		PRIVATE_DATA->bayer_pattern = "RGGB";
		break;
		default:
		PRIVATE_DATA->bayer_pattern = NULL;
		break;
	}
}

static bool initialize_camera(indigo_device *device) {
	int id = PRIVATE_DATA->dev_id;
	int ctrl_count = 0;
	POAConfigAttributes ctrl_caps = { 0 };
	POAErrors res = POAGetConfigsCount(id, &ctrl_count);
	if (res) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "POAGetNumOfControls(%d) > %d", id, res);
		return false;
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetNumOfControls(%d, > %d)", id, ctrl_count);
	}
	CCD_GAIN_PROPERTY->hidden = CCD_OFFSET_PROPERTY->hidden = CCD_EGAIN_PROPERTY->hidden = true;
	CCD_TEMPERATURE_PROPERTY->hidden = CCD_COOLER_PROPERTY->hidden = CCD_COOLER_POWER_PROPERTY->hidden = true;
	PRIVATE_DATA->has_temperature_sensor = PRIVATE_DATA->can_check_temperature = false;
	X_ADVANCED_PROPERTY = indigo_resize_property(X_ADVANCED_PROPERTY, 0);
	for (int ctrl_no = 0; ctrl_no < ctrl_count; ctrl_no++) {
		if (POAGetConfigAttributes(id, ctrl_no, &ctrl_caps) != POA_OK) {
			return false;
		}
		ctrl_caps.szConfName[sizeof(ctrl_caps.szConfName) - 1] = 0;
		if (init_camera_property(device, ctrl_caps) != INDIGO_OK) {
			return false;
		}
	}
	X_SENSOR_MODE_PROPERTY->hidden = false;
	X_SENSOR_MODE_PROPERTY = indigo_resize_property(X_SENSOR_MODE_PROPERTY, 0);
	indigo_result indigo_res = init_sensor_mode_property(device);
	if (indigo_res == INDIGO_NOT_FOUND) {
		X_SENSOR_MODE_PROPERTY->hidden = true;
	} else if (indigo_res != INDIGO_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to initialize sensor mode property");
		X_SENSOR_MODE_PROPERTY->hidden = true;
	}
	res = POAGetGainsAndOffsets(id, &PRIVATE_DATA->gain_highest_dr, &PRIVATE_DATA->gain_hcg, &PRIVATE_DATA->gain_unity_gain, &PRIVATE_DATA->gain_lowerst_rn, &PRIVATE_DATA->offset_highest_dr, &PRIVATE_DATA->offset_hcg, &PRIVATE_DATA->offset_unity_gain, &PRIVATE_DATA->offset_lowest_rn);
	if (res) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "POAGetGainsAndOffsets(%d) = %d", id, res);
		return false;
	}
	char item_desc[100];
	sprintf(item_desc, "Highest Dynamic Range (%d, %d)", PRIVATE_DATA->gain_highest_dr, PRIVATE_DATA->offset_highest_dr);
	indigo_init_switch_item(POA_HIGHEST_DR_ITEM, POA_HIGHEST_DR_NAME, item_desc, false);
	sprintf(item_desc, "Unity Gain (%d, %d)", PRIVATE_DATA->gain_unity_gain, PRIVATE_DATA->offset_unity_gain);
	indigo_init_switch_item(POA_UNITY_GAIN_ITEM, POA_UNITY_GAIN_NAME, item_desc, false);
	sprintf(item_desc, "Lowest Readout Noise (%d, %d)", PRIVATE_DATA->gain_lowerst_rn, PRIVATE_DATA->offset_lowest_rn);
	indigo_init_switch_item(POA_LOWEST_RN_ITEM, POA_LOWEST_RN_NAME, item_desc, false);
	sprintf(item_desc, "High Conversion Gain (%d)", PRIVATE_DATA->gain_hcg);
	indigo_init_switch_item(POA_GAIN_HCG_ITEM, POA_GAIN_HCG_NAME, item_desc, false);
	adjust_preset_switches(device);
	return true;
}

//- code

#pragma mark - High level code (ccd)

static void ccd_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ ccd.on_timer
	if (PRIVATE_DATA->has_temperature_sensor) {
		ccd_temperature_callback(device);
	}
	//- ccd.on_timer
}

static void ccd_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = playerone_open(device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ ccd.on_connect
			indigo_lock_master_device(device);
			connection_result = initialize_camera(device);
			indigo_unlock_master_device(device);
			//- ccd.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, X_PIXEL_FORMAT_PROPERTY, NULL);
			indigo_define_property(device, X_ADVANCED_PROPERTY, NULL);
			indigo_define_property(device, X_PRESETS_PROPERTY, NULL);
			indigo_define_property(device, X_CUSTOM_SUFFIX_PROPERTY, NULL);
			indigo_define_property(device, X_SENSOR_MODE_PROPERTY, NULL);
			indigo_execute_handler(device, ccd_timer_callback);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s", device->name);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				playerone_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ ccd.on_disconnect
		indigo_lock_master_device(device);
		acquisition_finish(device, false, true);
		PRIVATE_DATA->can_check_temperature = false;
		indigo_unlock_master_device(device);
		//- ccd.on_disconnect
		indigo_delete_property(device, X_PIXEL_FORMAT_PROPERTY, NULL);
		indigo_delete_property(device, X_ADVANCED_PROPERTY, NULL);
		indigo_delete_property(device, X_PRESETS_PROPERTY, NULL);
		indigo_delete_property(device, X_CUSTOM_SUFFIX_PROPERTY, NULL);
		indigo_delete_property(device, X_SENSOR_MODE_PROPERTY, NULL);
		if (--PRIVATE_DATA->count == 0) {
			playerone_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_ccd_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void ccd_exposure_handler(indigo_device *device) {
	//+ ccd.CCD_EXPOSURE.on_change
	if (PRIVATE_DATA->acquisition_active) {
		return;
	}
	indigo_use_shortest_exposure_if_bias(device);
	CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
	indigo_ccd_exposure_setup(device);
	acquisition_start(device, false); // acquisition_finalizer owns completion
	//- ccd.CCD_EXPOSURE.on_change
}

static void ccd_streaming_handler(indigo_device *device) {
	//+ ccd.CCD_STREAMING.on_change
	if (PRIVATE_DATA->acquisition_active) {
		return;
	}
	indigo_use_shortest_exposure_if_bias(device);
	CCD_STREAMING_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
	if (CCD_UPLOAD_MODE_LOCAL_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
		CCD_IMAGE_FILE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
	}
	if (CCD_UPLOAD_MODE_CLIENT_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
		CCD_IMAGE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
	}
	acquisition_start(device, true); // acquisition_finalizer owns completion
	//- ccd.CCD_STREAMING.on_change
}

static void ccd_abort_exposure_handler(indigo_device *device) {
	//+ ccd.CCD_ABORT_EXPOSURE.on_change
	if (CCD_ABORT_EXPOSURE_ITEM->sw.value && (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE)) {
		indigo_cancel_pending_handler(device, ccd_exposure_handler);
		indigo_cancel_pending_handler(device, ccd_streaming_handler);
		CCD_ABORT_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CCD_ABORT_EXPOSURE_PROPERTY, NULL);
		if (PRIVATE_DATA->acquisition_active) {
			acquisition_finish(device, false, true); // acquisition_finalizer is canceled
		} else {
			indigo_ccd_abort_exposure_cleanup(device);
		}
	} else {
		CCD_ABORT_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
		CCD_ABORT_EXPOSURE_ITEM->sw.value = false;
		indigo_update_property(device, CCD_ABORT_EXPOSURE_PROPERTY, NULL);
	}
	//- ccd.CCD_ABORT_EXPOSURE.on_change
}

static void ccd_cooler_handler(indigo_device *device) {
	CCD_COOLER_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_COOLER.on_change
	if (CONNECTION_CONNECTED_ITEM->sw.value && !CCD_COOLER_PROPERTY->hidden) {
		CCD_COOLER_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
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
		CCD_TEMPERATURE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, "Target Temperature = %.2f", PRIVATE_DATA->target_temperature);
	}
	//- ccd.CCD_TEMPERATURE.on_change
	indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
}

static void ccd_gain_handler(indigo_device *device) {
	CCD_GAIN_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_GAIN.on_change
	if (PRIVATE_DATA->acquisition_active) {
		CCD_GAIN_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, CCD_GAIN_PROPERTY, "Exposure in progress");
		return;
	}
	POAConfigValue value;
	value.intValue = (long)(CCD_GAIN_ITEM->number.target);
	POAErrors res = POASetConfig(PRIVATE_DATA->dev_id, POA_GAIN, value, POA_FALSE);
	if (res) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "POASetConfig(%d, POA_GAIN, %d) > %d", PRIVATE_DATA->dev_id, value.intValue, res);
		CCD_GAIN_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POASetConfig(%d, POA_GAIN, %d)", PRIVATE_DATA->dev_id, value.intValue);
		CCD_GAIN_ITEM->number.value = value.intValue;
	}
	adjust_preset_switches(device);
	POABool is_auto = POA_FALSE;
	res = POAGetConfig(PRIVATE_DATA->dev_id, POA_EGAIN, &value, &is_auto);
	if (res) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "POAGetConfig(%d, POA_EGAIN) > %d", PRIVATE_DATA->dev_id, res);
		CCD_EGAIN_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetConfig(%d, POA_EGAIN, > %g)", PRIVATE_DATA->dev_id, value.floatValue);
		CCD_EGAIN_ITEM->number.value = value.floatValue;
		CCD_EGAIN_ITEM->number.target = value.floatValue;
		CCD_EGAIN_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_update_property(device, X_PRESETS_PROPERTY, NULL);
	indigo_update_property(device, CCD_EGAIN_PROPERTY, NULL);
	//- ccd.CCD_GAIN.on_change
	indigo_update_property(device, CCD_GAIN_PROPERTY, NULL);
}

static void ccd_offset_handler(indigo_device *device) {
	CCD_OFFSET_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_OFFSET.on_change
	if (PRIVATE_DATA->acquisition_active) {
		CCD_OFFSET_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, CCD_OFFSET_PROPERTY, "Exposure in progress");
		return;
	}
	POAConfigValue value;
	value.intValue = (long)(CCD_OFFSET_ITEM->number.target);
	POAErrors res = POASetConfig(PRIVATE_DATA->dev_id, POA_OFFSET, value, POA_FALSE);
	if (res) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "POASetConfig(%d, POA_OFFSET, %d) > %d", PRIVATE_DATA->dev_id, value.intValue, res);
		CCD_OFFSET_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POASetConfig(%d, POA_OFFSET, %d)", PRIVATE_DATA->dev_id, value.intValue);
		CCD_OFFSET_ITEM->number.value = value.intValue;
	}
	adjust_preset_switches(device);
	indigo_update_property(device, X_PRESETS_PROPERTY, NULL);
	//- ccd.CCD_OFFSET.on_change
	indigo_update_property(device, CCD_OFFSET_PROPERTY, NULL);
}

static void ccd_frame_handler(indigo_device *device) {
	CCD_FRAME_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_FRAME.on_change
	if (PRIVATE_DATA->acquisition_active) {
		CCD_FRAME_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, CCD_FRAME_PROPERTY, "Exposure in progress");
		return;
	}
	if (CCD_FRAME_WIDTH_ITEM->number.value != CCD_FRAME_WIDTH_ITEM->number.max) {
		CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.target = 2 * (int)(CCD_FRAME_WIDTH_ITEM->number.value / 2);
	}
	if (CCD_FRAME_HEIGHT_ITEM->number.value != CCD_FRAME_HEIGHT_ITEM->number.max) {
		CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.target = 2 * (int)(CCD_FRAME_HEIGHT_ITEM->number.value / 2);
	}
	if (CCD_FRAME_WIDTH_ITEM->number.value / CCD_BIN_HORIZONTAL_ITEM->number.value < 64) {
		CCD_FRAME_WIDTH_ITEM->number.value = 64 * CCD_BIN_HORIZONTAL_ITEM->number.value;
	}
	if (CCD_FRAME_HEIGHT_ITEM->number.value / CCD_BIN_VERTICAL_ITEM->number.value < 64) {
		CCD_FRAME_HEIGHT_ITEM->number.value = 64 * CCD_BIN_VERTICAL_ITEM->number.value;
	}
	if (CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value < 12) {
		CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = 8;
	} else if (CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value < 20) {
		CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = 16;
	} else {
		CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = 24;
	}
	int horizontal_bin = (int)CCD_BIN_HORIZONTAL_ITEM->number.value;
	int vertical_bin = (int)CCD_BIN_VERTICAL_ITEM->number.value;
	char name[32] = "";
	for (int i = 0; i < X_PIXEL_FORMAT_PROPERTY->count; i++) {
		if (!strcmp(X_PIXEL_FORMAT_PROPERTY->items[i].name, RAW8_NAME) && CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value == 8) {
			indigo_set_switch(X_PIXEL_FORMAT_PROPERTY, X_PIXEL_FORMAT_PROPERTY->items + i, true);
			snprintf(name, 32, "%s %dx%d", X_PIXEL_FORMAT_PROPERTY->items[i].name, horizontal_bin, vertical_bin);
			break;
		}
		if (!strcmp(X_PIXEL_FORMAT_PROPERTY->items[i].name, RAW16_NAME) && CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value == 16) {
			indigo_set_switch(X_PIXEL_FORMAT_PROPERTY, X_PIXEL_FORMAT_PROPERTY->items + i, true);
			snprintf(name, 32, "%s %dx%d", X_PIXEL_FORMAT_PROPERTY->items[i].name, horizontal_bin, vertical_bin);
			break;
		}
		if (!strcmp(X_PIXEL_FORMAT_PROPERTY->items[i].name, RGB24_NAME) && CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value == 24) {
			indigo_set_switch(X_PIXEL_FORMAT_PROPERTY, X_PIXEL_FORMAT_PROPERTY->items + i, true);
			snprintf(name, 32, "%s %dx%d", X_PIXEL_FORMAT_PROPERTY->items[i].name, horizontal_bin, vertical_bin);
			break;
		}
	}
	for (int i = 0; i < CCD_MODE_PROPERTY->count; i++) {
		indigo_item *item = &CCD_MODE_PROPERTY->items[i];
		item->sw.value = !strcmp(item->name, name);
	}
	CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.target = get_pixel_depth(device);
	CCD_MODE_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, CCD_MODE_PROPERTY, NULL);
	indigo_update_property(device, X_PIXEL_FORMAT_PROPERTY, NULL);
	//- ccd.CCD_FRAME.on_change
	indigo_update_property(device, CCD_FRAME_PROPERTY, NULL);
}

static void ccd_mode_handler(indigo_device *device) {
	CCD_MODE_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_MODE.on_change
	if (PRIVATE_DATA->acquisition_active) {
		CCD_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, CCD_MODE_PROPERTY, "Exposure in progress");
		return;
	}
	char name[32] = "";
	int h, v;
	for (int i = 0; i < CCD_MODE_PROPERTY->count; i++) {
		indigo_item *item = &CCD_MODE_PROPERTY->items[i];
		if (item->sw.value) {
			for (int j = 0; j < X_PIXEL_FORMAT_PROPERTY->count; j++) {
				snprintf(name, 32, "%s %%dx%%d", X_PIXEL_FORMAT_PROPERTY->items[j].name);
				if (sscanf(item->name, name, &h, &v) == 2) {
					CCD_BIN_HORIZONTAL_ITEM->number.value = CCD_BIN_HORIZONTAL_ITEM->number.target = h;
					CCD_BIN_VERTICAL_ITEM->number.value = CCD_BIN_VERTICAL_ITEM->number.target = v;
					X_PIXEL_FORMAT_PROPERTY->items[j].sw.value = true;
				} else {
					X_PIXEL_FORMAT_PROPERTY->items[j].sw.value = false;
				}
			}
			break;
		}
	}
	CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.target = get_pixel_depth(device);
	X_PIXEL_FORMAT_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, X_PIXEL_FORMAT_PROPERTY, NULL);
	CCD_FRAME_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, CCD_FRAME_PROPERTY, NULL);
	CCD_BIN_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, CCD_BIN_PROPERTY, NULL);
	//- ccd.CCD_MODE.on_change
	indigo_update_property(device, CCD_MODE_PROPERTY, NULL);
}

static void ccd_bin_handler(indigo_device *device) {
	CCD_BIN_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_BIN.on_change
	if (PRIVATE_DATA->acquisition_active) {
		CCD_BIN_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, CCD_BIN_PROPERTY, "Exposure in progress");
		return;
	}
	double requested = CCD_BIN_HORIZONTAL_ITEM->number.target != CCD_BIN_HORIZONTAL_ITEM->number.value ? CCD_BIN_HORIZONTAL_ITEM->number.target : CCD_BIN_VERTICAL_ITEM->number.target;
	bool supported = false;
	for (int i = 0; i < 8 && PRIVATE_DATA->property.bins[i]; i++) {
		if (requested == PRIVATE_DATA->property.bins[i]) {
			supported = true;
			break;
		}
	}
	if (!supported) {
		CCD_BIN_HORIZONTAL_ITEM->number.target = CCD_BIN_HORIZONTAL_ITEM->number.value;
		CCD_BIN_VERTICAL_ITEM->number.target = CCD_BIN_VERTICAL_ITEM->number.value;
		CCD_BIN_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, CCD_BIN_PROPERTY, "Unsupported binning");
		return;
	}
	int prev_h_bin = (int)CCD_BIN_HORIZONTAL_ITEM->number.value;
	int prev_v_bin = (int)CCD_BIN_VERTICAL_ITEM->number.value;
	CCD_BIN_HORIZONTAL_ITEM->number.value = CCD_BIN_HORIZONTAL_ITEM->number.target;
	CCD_BIN_VERTICAL_ITEM->number.value = CCD_BIN_VERTICAL_ITEM->number.target;
	int horizontal_bin = (int)CCD_BIN_HORIZONTAL_ITEM->number.value;
	int vertical_bin = (int)CCD_BIN_VERTICAL_ITEM->number.value;
	/* Player One cameras work with binx = biny for we force it here */
	if (prev_h_bin != horizontal_bin) {
		vertical_bin = (int)(CCD_BIN_HORIZONTAL_ITEM->number.target = CCD_BIN_HORIZONTAL_ITEM->number.value = CCD_BIN_VERTICAL_ITEM->number.target = CCD_BIN_VERTICAL_ITEM->number.value = horizontal_bin);
	} else if (prev_v_bin != vertical_bin) {
		horizontal_bin = (int)(CCD_BIN_HORIZONTAL_ITEM->number.target = CCD_BIN_HORIZONTAL_ITEM->number.value = CCD_BIN_VERTICAL_ITEM->number.target = CCD_BIN_VERTICAL_ITEM->number.value = vertical_bin);
	}
	char name[32] = "";
	for (int i = 0; i < X_PIXEL_FORMAT_PROPERTY->count; i++) {
		if (X_PIXEL_FORMAT_PROPERTY->items[i].sw.value) {
			snprintf(name, 32, "%s %dx%d", X_PIXEL_FORMAT_PROPERTY->items[i].name, horizontal_bin, vertical_bin);
			break;
		}
	}
	for (int i = 0; i < CCD_MODE_PROPERTY->count; i++) {
		indigo_item *item = &CCD_MODE_PROPERTY->items[i];
		item->sw.value = !strcmp(item->name, name);
	}
	CCD_MODE_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, CCD_MODE_PROPERTY, NULL);
	//- ccd.CCD_BIN.on_change
	indigo_update_property(device, CCD_BIN_PROPERTY, NULL);
}

static void ccd_x_pixel_format_handler(indigo_device *device) {
	X_PIXEL_FORMAT_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.X_PIXEL_FORMAT.on_change
	if (PRIVATE_DATA->acquisition_active) {
		X_PIXEL_FORMAT_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_PIXEL_FORMAT_PROPERTY, "Exposure in progress");
		return;
	}
	/* NOTE: BPP can not be set directly because can not be linked to X_PIXEL_FORMAT_PROPERTY */
	CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.target = get_pixel_depth(device);
	CCD_FRAME_PROPERTY->state = INDIGO_OK_STATE;
	int horizontal_bin = (int)CCD_BIN_HORIZONTAL_ITEM->number.value;
	int vertical_bin = (int)CCD_BIN_VERTICAL_ITEM->number.value;
	char name[32] = "";
	for (int i = 0; i < X_PIXEL_FORMAT_PROPERTY->count; i++) {
		if (X_PIXEL_FORMAT_PROPERTY->items[i].sw.value) {
			snprintf(name, 32, "%s %dx%d", X_PIXEL_FORMAT_PROPERTY->items[i].name, horizontal_bin, vertical_bin);
			break;
		}
	}
	for (int i = 0; i < CCD_MODE_PROPERTY->count; i++) {
		indigo_item *item = &CCD_MODE_PROPERTY->items[i];
		item->sw.value = !strcmp(item->name, name);
	}
	CCD_MODE_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, CCD_FRAME_PROPERTY, NULL);
	indigo_update_property(device, CCD_MODE_PROPERTY, NULL);
	//- ccd.X_PIXEL_FORMAT.on_change
	indigo_update_property(device, X_PIXEL_FORMAT_PROPERTY, NULL);
}

static void ccd_x_advanced_handler(indigo_device *device) {
	X_ADVANCED_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.X_ADVANCED.on_change
	if (PRIVATE_DATA->acquisition_active) {
		X_ADVANCED_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_ADVANCED_PROPERTY, "Exposure in progress");
		return;
	}
	int ctrl_count;
	POAConfigAttributes ctrl_caps;
	POAErrors res;
	int id = PRIVATE_DATA->dev_id;
	if (!IS_CONNECTED) {
		return;
	}
	res = POAGetConfigsCount(id, &ctrl_count);
	if (res) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "POAGetNumOfControls(%d) > %d", id, res);
		X_ADVANCED_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_ADVANCED_PROPERTY, NULL);
		return;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetNumOfControls(%d, > %d)", id, ctrl_count);
	POABool unused;
	POAConfigValue value;
	for (int ctrl_no = 0; ctrl_no < ctrl_count; ctrl_no++) {
		if (POAGetConfigAttributes(id, ctrl_no, &ctrl_caps) != POA_OK) {
			X_ADVANCED_PROPERTY->state = INDIGO_ALERT_STATE;
			continue;
		}
		ctrl_caps.szConfName[sizeof(ctrl_caps.szConfName) - 1] = 0;
		for (int i = 0; i < X_ADVANCED_PROPERTY->count; i++) {
			indigo_item *item = X_ADVANCED_PROPERTY->items + i;
			if (!strncmp(ctrl_caps.szConfName, item->name, INDIGO_NAME_SIZE)) {
				if (ctrl_caps.valueType == VAL_BOOL) {
					value.boolValue = item->number.target != 0;
				} else if (ctrl_caps.valueType == VAL_FLOAT) {
					value.floatValue = item->number.target;
				} else {
					value.intValue = (long)item->number.target;
				}
				res = POASetConfig(id, ctrl_caps.configID, value, POA_FALSE);
				if (res) {
					X_ADVANCED_PROPERTY->state = INDIGO_ALERT_STATE;
					if (ctrl_caps.valueType == VAL_BOOL) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "POASetConfig(%d, %s, %s) > %d", id, ctrl_caps.szConfName, value.boolValue ? "true" : "false", res);
					} else if (ctrl_caps.valueType == VAL_FLOAT) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "POASetConfig(%d, %s, %g) > %d", id, ctrl_caps.szConfName, value.floatValue, res);
					} else {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "POASetConfig(%d, %s, %ld) > %d", id, ctrl_caps.szConfName, value.intValue, res);
					}
				} else {
					if (ctrl_caps.valueType == VAL_BOOL) {
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POASetConfig(%d, %s, %s)", id, ctrl_caps.szConfName, value.boolValue ? "true" : "false");
					} else if (ctrl_caps.valueType == VAL_FLOAT) {
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POASetConfig(%d, %s, %g)", id, ctrl_caps.szConfName, value.floatValue);
					} else {
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POASetConfig(%d, %s, %ld)", id, ctrl_caps.szConfName, value.intValue);
					}
				}
				res = POAGetConfig(id, ctrl_caps.configID, &value, &unused);
				if (res) {
					X_ADVANCED_PROPERTY->state = INDIGO_ALERT_STATE;
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "POAGetConfig(%d, %s) > %d", id, ctrl_caps.szConfName, res);
				} else {
					if (ctrl_caps.valueType == VAL_BOOL) {
						item->number.value = value.boolValue;
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetConfig(%d, %s, > %s)", id, ctrl_caps.szConfName, value.boolValue ? "true" : "false");
					} else if (ctrl_caps.valueType == VAL_FLOAT) {
						item->number.value = value.floatValue;
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetConfig(%d, %s, > %g)", id, ctrl_caps.szConfName, value.floatValue);
					} else {
						item->number.value = value.intValue;
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetConfig(%d, %s, > %ld)", id, ctrl_caps.szConfName, value.intValue);
					}
				}
			}
		}
	}
	//- ccd.X_ADVANCED.on_change
	indigo_update_property(device, X_ADVANCED_PROPERTY, NULL);
}

static void ccd_x_presets_handler(indigo_device *device) {
	X_PRESETS_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.X_PRESETS.on_change
	if (PRIVATE_DATA->acquisition_active) {
		X_PRESETS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_PRESETS_PROPERTY, "Exposure in progress");
		return;
	}
	int gain = 0, offset = 0;
	if (POA_HIGHEST_DR_ITEM->sw.value) {
		gain = PRIVATE_DATA->gain_highest_dr;
		offset = PRIVATE_DATA->offset_highest_dr;
	} else if (POA_UNITY_GAIN_ITEM->sw.value) {
		gain = PRIVATE_DATA->gain_unity_gain;
		offset = PRIVATE_DATA->offset_unity_gain;
	} else if (POA_LOWEST_RN_ITEM->sw.value) {
		gain = PRIVATE_DATA->gain_lowerst_rn;
		offset = PRIVATE_DATA->offset_lowest_rn;
	} else if (POA_GAIN_HCG_ITEM->sw.value) {
		gain = PRIVATE_DATA->gain_hcg;
		offset = (int)CCD_OFFSET_ITEM->number.value;
	}
	CCD_GAIN_PROPERTY->state = INDIGO_OK_STATE;
	CCD_OFFSET_PROPERTY->state = INDIGO_OK_STATE;
	POAConfigValue value;
	value.intValue = (long)gain;
	POAErrors res = POASetConfig(PRIVATE_DATA->dev_id, POA_GAIN, value, POA_FALSE);
	if (res) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "POASetConfig(%d, POA_GAIN) = %d", PRIVATE_DATA->dev_id, res);
		CCD_GAIN_PROPERTY->state = INDIGO_ALERT_STATE;
		X_PRESETS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	value.intValue = (long)offset;
	res = POASetConfig(PRIVATE_DATA->dev_id, POA_OFFSET, value, POA_FALSE);
	if (res) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "POASetConfig(%d, POA_OFFSET) = %d", PRIVATE_DATA->dev_id, res);
		CCD_OFFSET_PROPERTY->state = INDIGO_ALERT_STATE;
		X_PRESETS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	if (CCD_GAIN_PROPERTY->state == INDIGO_OK_STATE) {
		CCD_GAIN_ITEM->number.value = CCD_GAIN_ITEM->number.target = gain;
	}
	if (CCD_OFFSET_PROPERTY->state == INDIGO_OK_STATE) {
		CCD_OFFSET_ITEM->number.value = CCD_OFFSET_ITEM->number.target = offset;
	}
	adjust_preset_switches(device);
	POABool is_auto = POA_FALSE;
	res = POAGetConfig(PRIVATE_DATA->dev_id, POA_EGAIN, &value, &is_auto);
	if (res) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "POAGetConfig(%d, POA_EGAIN) > %d", PRIVATE_DATA->dev_id, res);
		CCD_EGAIN_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetConfig(%d, POA_EGAIN, > %g)", PRIVATE_DATA->dev_id, value.floatValue);
		CCD_EGAIN_ITEM->number.value = value.floatValue;
		CCD_EGAIN_ITEM->number.target = value.floatValue;
		CCD_EGAIN_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_update_property(device, CCD_GAIN_PROPERTY, NULL);
	indigo_update_property(device, CCD_EGAIN_PROPERTY, NULL);
	indigo_update_property(device, CCD_OFFSET_PROPERTY, NULL);
	//- ccd.X_PRESETS.on_change
	indigo_update_property(device, X_PRESETS_PROPERTY, NULL);
}

static void ccd_x_custom_suffix_handler(indigo_device *device) {
	X_CUSTOM_SUFFIX_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.X_CUSTOM_SUFFIX.on_change
	if (PRIVATE_DATA->acquisition_active) {
		X_CUSTOM_SUFFIX_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_CUSTOM_SUFFIX_PROPERTY, "Exposure in progress");
		return;
	}
	int length = (int)strlen(X_CUSTOM_SUFFIX_ITEM->text.value);
	if (length > 16) {
		X_CUSTOM_SUFFIX_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_CUSTOM_SUFFIX_PROPERTY, "Custom suffix is too long.");
		return;
	}
	POAErrors res = POASetUserCustomID(PRIVATE_DATA->dev_id, X_CUSTOM_SUFFIX_ITEM->text.value, length);
	if (res) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "POASetUserCustomID(%d, \"%s\", %d) > %d", PRIVATE_DATA->dev_id, X_CUSTOM_SUFFIX_ITEM->text.value, length, res);
		X_CUSTOM_SUFFIX_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POASetUserCustomID(%d, \"%s\", %d) > %d", PRIVATE_DATA->dev_id, X_CUSTOM_SUFFIX_ITEM->text.value, length, res);
		if (length > 0) {
			indigo_send_message(device, OK_PROPERTY, "Camera name suffix '#%s' will be used on replug", X_CUSTOM_SUFFIX_ITEM->text.value);
		} else {
			indigo_send_message(device, OK_PROPERTY, "Camera name suffix cleared, will be used on replug");
		}
	}
	//- ccd.X_CUSTOM_SUFFIX.on_change
	indigo_update_property(device, X_CUSTOM_SUFFIX_PROPERTY, NULL);
}

static void ccd_x_sensor_mode_handler(indigo_device *device) {
	X_SENSOR_MODE_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.X_SENSOR_MODE.on_change
	if (PRIVATE_DATA->acquisition_active) {
		X_SENSOR_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_SENSOR_MODE_PROPERTY, "Exposure in progress");
		return;
	}
	int res;
	int id = PRIVATE_DATA->dev_id;
	int selected_mode = -1;
	if (!IS_CONNECTED) {
		return;
	}
	for (int i = 0; i < X_SENSOR_MODE_PROPERTY->count; i++) {
		if (X_SENSOR_MODE_PROPERTY->items[i].sw.value) {
			selected_mode = i;
			break;
		}
	}
	if (selected_mode < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "No sensor mode selected");
		X_SENSOR_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_SENSOR_MODE_PROPERTY, NULL);
		return;
	}
	res = POASetSensorMode(id, selected_mode);
	if (res) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "POASetSensorMode(%d, %d) > %d", id, selected_mode, res);
		X_SENSOR_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_SENSOR_MODE_PROPERTY, NULL);
		return;
	}
	//- ccd.X_SENSOR_MODE.on_change
	indigo_update_property(device, X_SENSOR_MODE_PROPERTY, NULL);
}

#pragma mark - Device API (ccd)

static indigo_result ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result ccd_attach(indigo_device *device) {
	if (indigo_ccd_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		CCD_EXPOSURE_PROPERTY->hidden = false;
		CCD_STREAMING_PROPERTY->hidden = false;
		CCD_ABORT_EXPOSURE_PROPERTY->hidden = false;
		CCD_COOLER_PROPERTY->hidden = true;
		CCD_TEMPERATURE_PROPERTY->hidden = true;
		CCD_GAIN_PROPERTY->hidden = true;
		CCD_OFFSET_PROPERTY->hidden = true;
		CCD_FRAME_PROPERTY->hidden = false;
		CCD_MODE_PROPERTY->hidden = false;
		CCD_BIN_PROPERTY->hidden = false;
		X_PIXEL_FORMAT_PROPERTY = indigo_init_switch_property(NULL, device->name, X_PIXEL_FORMAT_PROPERTY_NAME, CCD_ADVANCED_GROUP, "Pixel Format", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 0);
		if (X_PIXEL_FORMAT_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		X_ADVANCED_PROPERTY = indigo_init_number_property(NULL, device->name, X_ADVANCED_PROPERTY_NAME, CCD_ADVANCED_GROUP, "Advanced", INDIGO_OK_STATE, INDIGO_RW_PERM, 0);
		if (X_ADVANCED_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		X_PRESETS_PROPERTY = indigo_init_switch_property(NULL, device->name, X_PRESETS_PROPERTY_NAME, CCD_ADVANCED_GROUP, "Presets (Gain, Offset)", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 4);
		if (X_PRESETS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(POA_HIGHEST_DR_ITEM, POA_HIGHEST_DR_ITEM_NAME, "POA_HIGHEST_DR", false);
		indigo_init_switch_item(POA_UNITY_GAIN_ITEM, POA_UNITY_GAIN_ITEM_NAME, "POA_UNITY_GAIN", false);
		indigo_init_switch_item(POA_LOWEST_RN_ITEM, POA_LOWEST_RN_ITEM_NAME, "POA_LOWEST_RN", false);
		indigo_init_switch_item(POA_GAIN_HCG_ITEM, POA_GAIN_HCG_ITEM_NAME, "POA_GAIN_HCG", false);
		X_CUSTOM_SUFFIX_PROPERTY = indigo_init_text_property(NULL, device->name, X_CUSTOM_SUFFIX_PROPERTY_NAME, CCD_ADVANCED_GROUP, "Device name custom suffix", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (X_CUSTOM_SUFFIX_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(X_CUSTOM_SUFFIX_ITEM, X_CUSTOM_SUFFIX_ITEM_NAME, "Suffix", "");
		X_SENSOR_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, X_SENSOR_MODE_PROPERTY_NAME, CCD_ADVANCED_GROUP, "Sensor readout mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 0);
		if (X_SENSOR_MODE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		//+ ccd.X_SENSOR_MODE.on_attach
		initialize_properties(device);
		//- ccd.X_SENSOR_MODE.on_attach
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_PIXEL_FORMAT_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_ADVANCED_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_PRESETS_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_CUSTOM_SUFFIX_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_SENSOR_MODE_PROPERTY);
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
	} else if (indigo_property_match_changeable(CCD_EXPOSURE_PROPERTY, property)) {
		//+ ccd.CCD_EXPOSURE.on_change_request
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
			return INDIGO_OK;
		}
		//- ccd.CCD_EXPOSURE.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_EXPOSURE_PROPERTY, ccd_exposure_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_STREAMING_PROPERTY, property)) {
		//+ ccd.CCD_STREAMING.on_change_request
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
			return INDIGO_OK;
		}
		//- ccd.CCD_STREAMING.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_STREAMING_PROPERTY, ccd_streaming_handler);
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
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
			CCD_GAIN_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_GAIN_PROPERTY, "Exposure in progress");
			return INDIGO_OK;
		}
		//- ccd.CCD_GAIN.on_change_request
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(CCD_GAIN_PROPERTY, ccd_gain_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_OFFSET_PROPERTY, property)) {
		//+ ccd.CCD_OFFSET.on_change_request
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
			CCD_OFFSET_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_OFFSET_PROPERTY, "Exposure in progress");
			return INDIGO_OK;
		}
		//- ccd.CCD_OFFSET.on_change_request
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(CCD_OFFSET_PROPERTY, ccd_offset_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_FRAME_PROPERTY, property)) {
		//+ ccd.CCD_FRAME.on_change_request
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
			CCD_FRAME_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_FRAME_PROPERTY, "Exposure in progress");
			return INDIGO_OK;
		}
		//- ccd.CCD_FRAME.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_FRAME_PROPERTY, ccd_frame_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_MODE_PROPERTY, property)) {
		//+ ccd.CCD_MODE.on_change_request
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
			CCD_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_MODE_PROPERTY, "Exposure in progress");
			return INDIGO_OK;
		}
		//- ccd.CCD_MODE.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_MODE_PROPERTY, ccd_mode_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_BIN_PROPERTY, property)) {
		//+ ccd.CCD_BIN.on_change_request
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
			CCD_BIN_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_BIN_PROPERTY, "Exposure in progress");
			return INDIGO_OK;
		}
		//- ccd.CCD_BIN.on_change_request
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(CCD_BIN_PROPERTY, ccd_bin_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_PIXEL_FORMAT_PROPERTY, property)) {
		//+ ccd.X_PIXEL_FORMAT.on_change_request
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
			X_PIXEL_FORMAT_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, X_PIXEL_FORMAT_PROPERTY, "Exposure in progress");
			return INDIGO_OK;
		}
		//- ccd.X_PIXEL_FORMAT.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_PIXEL_FORMAT_PROPERTY, ccd_x_pixel_format_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_ADVANCED_PROPERTY, property)) {
		//+ ccd.X_ADVANCED.on_change_request
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
			X_ADVANCED_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, X_ADVANCED_PROPERTY, "Exposure in progress");
			return INDIGO_OK;
		}
		//- ccd.X_ADVANCED.on_change_request
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(X_ADVANCED_PROPERTY, ccd_x_advanced_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_PRESETS_PROPERTY, property)) {
		//+ ccd.X_PRESETS.on_change_request
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
			X_PRESETS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, X_PRESETS_PROPERTY, "Exposure in progress");
			return INDIGO_OK;
		}
		//- ccd.X_PRESETS.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_PRESETS_PROPERTY, ccd_x_presets_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_CUSTOM_SUFFIX_PROPERTY, property)) {
		//+ ccd.X_CUSTOM_SUFFIX.on_change_request
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
			X_CUSTOM_SUFFIX_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, X_CUSTOM_SUFFIX_PROPERTY, "Exposure in progress");
			return INDIGO_OK;
		}
		//- ccd.X_CUSTOM_SUFFIX.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_CUSTOM_SUFFIX_PROPERTY, ccd_x_custom_suffix_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_SENSOR_MODE_PROPERTY, property)) {
		//+ ccd.X_SENSOR_MODE.on_change_request
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
			X_SENSOR_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, X_SENSOR_MODE_PROPERTY, "Exposure in progress");
			return INDIGO_OK;
		}
		//- ccd.X_SENSOR_MODE.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_SENSOR_MODE_PROPERTY, ccd_x_sensor_mode_handler);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, X_PIXEL_FORMAT_PROPERTY);
			indigo_save_property(device, NULL, X_ADVANCED_PROPERTY);
			indigo_save_property(device, NULL, X_SENSOR_MODE_PROPERTY);
		}
	}
	return indigo_ccd_change_property(device, client, property);
}

static indigo_result ccd_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		ccd_connection_handler(device);
	}
	indigo_release_property(X_PIXEL_FORMAT_PROPERTY);
	indigo_release_property(X_ADVANCED_PROPERTY);
	indigo_release_property(X_PRESETS_PROPERTY);
	indigo_release_property(X_CUSTOM_SUFFIX_PROPERTY);
	indigo_release_property(X_SENSOR_MODE_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_ccd_detach(device);
}

#pragma mark - High level code (guider)

static void guider_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = playerone_open(device->master_device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s", device->name);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				playerone_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ guider.on_disconnect
		indigo_lock_master_device(device);
		guider_ra_finalizer(device);
		guider_dec_finalizer(device);
		indigo_unlock_master_device(device);
		//- guider.on_disconnect
		if (--PRIVATE_DATA->count == 0) {
			playerone_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_guider_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void guider_guide_ra_handler(indigo_device *device) {
	//+ guider.GUIDER_GUIDE_RA.on_change
	GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
	indigo_cancel_pending_handler(device, guider_ra_finalizer);
	POAConfigValue off = { .boolValue = false };
	POAErrors first = POASetConfig(PRIVATE_DATA->dev_id, POA_GUIDE_EAST, off, POA_FALSE);
	POAErrors second = POASetConfig(PRIVATE_DATA->dev_id, POA_GUIDE_WEST, off, POA_FALSE);
	int duration = GUIDER_GUIDE_EAST_ITEM->number.value > 0 ? GUIDER_GUIDE_EAST_ITEM->number.value : GUIDER_GUIDE_WEST_ITEM->number.value;
	POAConfig direction = GUIDER_GUIDE_EAST_ITEM->number.value > 0 ? POA_GUIDE_EAST : POA_GUIDE_WEST;
	if (first != POA_OK || second != POA_OK) {
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_ALERT_STATE;
	} else if (duration > 0) {
		POAConfigValue on = { .boolValue = true };
		if (POASetConfig(PRIVATE_DATA->dev_id, direction, on, POA_FALSE) == POA_OK) {
			GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_execute_handler_in(device, duration / 1000.0, guider_ra_finalizer);
		} else {
			GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
	//- guider.GUIDER_GUIDE_RA.on_change
}

static void guider_guide_dec_handler(indigo_device *device) {
	//+ guider.GUIDER_GUIDE_DEC.on_change
	GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
	indigo_cancel_pending_handler(device, guider_dec_finalizer);
	POAConfigValue off = { .boolValue = false };
	POAErrors first = POASetConfig(PRIVATE_DATA->dev_id, POA_GUIDE_NORTH, off, POA_FALSE);
	POAErrors second = POASetConfig(PRIVATE_DATA->dev_id, POA_GUIDE_SOUTH, off, POA_FALSE);
	int duration = GUIDER_GUIDE_NORTH_ITEM->number.value > 0 ? GUIDER_GUIDE_NORTH_ITEM->number.value : GUIDER_GUIDE_SOUTH_ITEM->number.value;
	POAConfig direction = GUIDER_GUIDE_NORTH_ITEM->number.value > 0 ? POA_GUIDE_NORTH : POA_GUIDE_SOUTH;
	if (first != POA_OK || second != POA_OK) {
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
	} else if (duration > 0) {
		POAConfigValue on = { .boolValue = true };
		if (POASetConfig(PRIVATE_DATA->dev_id, direction, on, POA_FALSE) == POA_OK) {
			GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_execute_handler_in(device, duration / 1000.0, guider_dec_finalizer);
		} else {
			GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
	//- guider.GUIDER_GUIDE_DEC.on_change
}

#pragma mark - Device API (guider)

static indigo_result guider_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result guider_attach(indigo_device *device) {
	if (indigo_guider_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		//+ guider.on_attach
		INFO_PROPERTY->count = 5;
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, PRIVATE_DATA->model);
		//- guider.on_attach
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
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
		//- guider.GUIDER_GUIDE_RA.on_change_request
		INDIGO_COPY_VALUES_PROCESS_PRIORITY_CHANGE(GUIDER_GUIDE_RA_PROPERTY, guider_guide_ra_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		//+ guider.GUIDER_GUIDE_DEC.on_change_request
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
		entry = indigo_safe_malloc(sizeof(*entry));
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
	sdk_discovery_retry *entry = data;
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
	playerone_private_data *private_data = NULL;
	bool plug_result = true;
	char name[INDIGO_NAME_SIZE] = DRIVER_LABEL;
	private_data = indigo_safe_malloc(sizeof(playerone_private_data));
	private_data->usbdev = dev;
	struct libusb_device_descriptor descriptor;
	if (!(libusb_get_device_descriptor(dev, &descriptor) == LIBUSB_SUCCESS && descriptor.idVendor == POA_VENDOR_ID)) {
		plug_result = false;
	}
	bool discovery_eligible = plug_result;
	if (plug_result) {
		//+ sdk.plug
		plug_result = false;
		int slots = 0;
		for (int i = 0; i < MAX_DEVICES; i++) {
			if (devices[i] == NULL) {
				slots++;
			}
		}
		int count = POAGetCameraCount();
		for (int index = 0; index < count; index++) {
			POACameraProperties info = { 0 };
			if (POAGetCameraProperties(index, &info) != POA_OK || info.cameraID < 0 || slots < (info.isHasST4Port ? 2 : 1)) {
				continue;
			}
			bool attached = false;
			for (int slot = 0; slot < MAX_DEVICES; slot++) {
				if (devices[slot] != NULL && ((playerone_private_data *)devices[slot]->private_data)->dev_id == info.cameraID) {
					attached = true;
					break;
				}
			}
			if (attached) {
				continue;
			}
			info.cameraModelName[sizeof(info.cameraModelName) - 1] = 0;
			info.sensorModelName[sizeof(info.sensorModelName) - 1] = 0;
			info.SN[sizeof(info.SN) - 1] = 0;
			private_data->dev_id = info.cameraID;
			private_data->property = info;
			snprintf(private_data->model, sizeof(private_data->model), "%s", info.cameraModelName);
			char suffix[17] = { 0 };
			memcpy(suffix, info.userCustomID, 16);
			char *start = strchr(private_data->model, '[');
			char *end = strrchr(private_data->model, ']');
			if (start && end && end > start && end[1] == 0) {
				if (!suffix[0]) {
					snprintf(suffix, sizeof(suffix), "%.*s", (int)(end - start - 1), start + 1);
				}
				while (start > private_data->model && start[-1] == ' ') {
					start--;
				}
				*start = 0;
			}
			snprintf(name, INDIGO_NAME_SIZE, "%.*s%s%s", INDIGO_NAME_SIZE - 20, private_data->model, suffix[0] ? " #" : "", suffix);
			snprintf(private_data->guider_name, INDIGO_NAME_SIZE, "%.*s (guider)%s%s", INDIGO_NAME_SIZE - 29, private_data->model, suffix[0] ? " #" : "", suffix);
			indigo_make_name_unique(name, "%d", info.cameraID);
			indigo_make_name_unique(private_data->guider_name, "%d", info.cameraID);
			plug_result = true;
			break;
		}
		//- sdk.plug
	}
	if (plug_result) {
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
		}
		if (ccd_attached && private_data->property.isHasST4Port) {
		indigo_device *guider = indigo_safe_malloc_copy(sizeof(indigo_device), &guider_template);
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
	playerone_private_data *private_data = NULL;
	playerone_private_data *removed[MAX_DEVICES];
	int removed_count = 0;
	for (int j = MAX_DEVICES - 1; j >= 0; j--) {
		if (devices[j] != NULL) {
			indigo_device *device = devices[j];
			private_data = PRIVATE_DATA;
			bool unplug_result = private_data->usbdev == dev;
			if (last_action != INDIGO_DRIVER_SHUTDOWN) {
				//+ sdk.unplug_match
				int count = POAGetCameraCount();
				unplug_result = count >= 0;
				for (int index = 0; index < count; index++) {
					POACameraProperties info = { 0 };
					if (POAGetCameraProperties(index, &info) != POA_OK || info.cameraID == private_data->dev_id) {
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

indigo_result indigo_ccd_playerone(indigo_driver_action action, indigo_driver_info *info) {

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			//+ on_init
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Player One camera SDK %s, API %d", POAGetSDKVersion(), POAGetAPIVersion());
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
			int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, POA_VENDOR_ID, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
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
			break;

		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
