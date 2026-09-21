// Copyright (c) 2016-2026 CloudMakers, s. r. o. and Rumen G. Bogdanovski
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

// This file generated from indigo_ccd_svb.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

//+ include

#include <assert.h>
#include <math.h>
#include <limits.h>
#include "SVBCameraSDK.h"

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_ccd_driver.h>
#include <indigo/indigo_guider_driver.h>
#include <indigo/indigo_uni_io.h>
#include <indigo/indigo_usb_utils.h>

#include "indigo_ccd_svb.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x0300001C
#define DRIVER_NAME          "indigo_ccd_svb"
#define DRIVER_LABEL         "SVBONY Camera"
#define CCD_DEVICE_NAME      "%s"
#define GUIDER_DEVICE_NAME   "%s"
#define MAX_DEVICES          5
#define PRIVATE_DATA         ((svb_private_data *)device->private_data)

//+ define

#define SVB_MAX_FORMATS      5
#define SVB_VENDOR_ID        0xf266
#define SVB_PRODUCT_ID       0x9a0a
#define RAW8_NAME            "RAW 8"
#define RGB24_NAME           "RGB 24"
#define RAW16_NAME           "RAW 16"
#define Y8_NAME              "Y 8"
#define Y16_NAME             "Y 16"
#define us2s(s)              ((s) / 1000000.0)
#define s2us(us)             ((us) * 1000000)

//- define

#pragma mark - Property definitions

#define X_PIXEL_FORMAT_PROPERTY        (PRIVATE_DATA->x_pixel_format_property)

#define X_PIXEL_FORMAT_PROPERTY_NAME   "X_PIXEL_FORMAT"

#define X_ADVANCED_PROPERTY            (PRIVATE_DATA->x_advanced_property)

#define X_ADVANCED_PROPERTY_NAME       "X_ADVANCED"

#pragma mark - Private data definition

typedef struct {
	int count;
	libusb_device *usbdev;
	indigo_property *x_pixel_format_property;
	indigo_property *x_advanced_property;
	//+ data
	int dev_id;
	SVB_CAMERA_INFO info;
	SVB_CAMERA_PROPERTY property;
	SVB_CAMERA_PROPERTY_EX property_ex;
	double pixel_size;
	char guider_name[INDIGO_NAME_SIZE];
	int exp_bin_x, exp_bin_y;
	int exp_frame_width, exp_frame_height;
	int exp_bpp;
	double target_temperature, current_temperature;
	long cooler_power;
	unsigned char *buffer;
	long buffer_size;
	bool can_check_temperature, has_temperature_sensor, has_cooler, first_frame;
	bool acquisition_active, streaming, frame_ready, video_started;
	double exposure_duration, exposure_end, readout_deadline;
	//- data
} svb_private_data;

#pragma mark - Low level code

static indigo_queue *driver_queue = NULL;
static pthread_mutex_t driver_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

//+ code

static void ccd_exposure_handler(indigo_device *device);
static void ccd_streaming_handler(indigo_device *device);
static void acquisition_finalizer(indigo_device *device);

static bool svb_valid_info(SVB_CAMERA_INFO *info, SVB_CAMERA_PROPERTY *property) {
	info->FriendlyName[sizeof(info->FriendlyName) - 1] = 0;
	info->CameraSN[sizeof(info->CameraSN) - 1] = 0;
	if (info->CameraID < 0 || !info->FriendlyName[0] || property->MaxWidth <= 0 || property->MaxHeight <= 0 || property->MaxWidth > INT_MAX || property->MaxHeight > INT_MAX || property->MaxHeight > (LONG_MAX - FITS_HEADER_SIZE - 1024) / 3 / property->MaxWidth || property->MaxBitDepth <= 0 || property->MaxBitDepth > 16) {
		return false;
	}
	bool bin1 = false;
	for (int i = 0; i < 16 && property->SupportedBins[i]; i++) {
		if (property->SupportedBins[i] < 1 || property->SupportedBins[i] > property->MaxWidth || property->SupportedBins[i] > property->MaxHeight) {
			return false;
		}
		bin1 |= property->SupportedBins[i] == 1;
	}
	bool format = false;
	for (int i = 0; i < 8 && property->SupportedVideoFormat[i] != SVB_IMG_END; i++) {
		switch (property->SupportedVideoFormat[i]) {
		case SVB_IMG_RAW8:
		case SVB_IMG_RGB24:
		case SVB_IMG_RAW16:
		case SVB_IMG_Y8:
		case SVB_IMG_Y16:
			format = true;
			break;
		default:
			break;
		}
	}
	return bin1 && format;
}

static char *get_bayer_string(indigo_device *device) {
	for (int i = 0; i < X_PIXEL_FORMAT_PROPERTY->count; i++) {
		if (X_PIXEL_FORMAT_PROPERTY->items[i].sw.value && (!strcmp(X_PIXEL_FORMAT_PROPERTY->items[i].name, Y8_NAME) || !strcmp(X_PIXEL_FORMAT_PROPERTY->items[i].name, Y16_NAME) || !strcmp(X_PIXEL_FORMAT_PROPERTY->items[i].name, RGB24_NAME))) {
			return NULL;
		}
	}
	if (!PRIVATE_DATA->property.IsColorCam) {
		return NULL;
	}
	switch (PRIVATE_DATA->property.BayerPattern) {
	case SVB_BAYER_BG:
		return "BGGR";
	case SVB_BAYER_GR:
		return "GRBG";
	case SVB_BAYER_GB:
		return "GBRG";
	case SVB_BAYER_RG:
	default:
		return "RGGB";
	}
}

static int get_pixel_depth(indigo_device *device) {
	int item = 0;
	while (item < SVB_MAX_FORMATS) {
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
			if (!strcmp(X_PIXEL_FORMAT_PROPERTY->items[item].name, Y8_NAME)) {
				return 8;
			}
			if (!strcmp(X_PIXEL_FORMAT_PROPERTY->items[item].name, Y16_NAME)) {
				return 16;
			}
		}
		item++;
	}
	return 8;
}

static int get_pixel_format(indigo_device *device) {
	int item = 0;
	while (item < SVB_MAX_FORMATS) {
		if (X_PIXEL_FORMAT_PROPERTY->items[item].sw.value) {
			if (!strcmp(X_PIXEL_FORMAT_PROPERTY->items[item].name, RAW8_NAME)) {
				return SVB_IMG_RAW8;
			}
			if (!strcmp(X_PIXEL_FORMAT_PROPERTY->items[item].name, RGB24_NAME)) {
				return SVB_IMG_RGB24;
			}
			if (!strcmp(X_PIXEL_FORMAT_PROPERTY->items[item].name, RAW16_NAME)) {
				return SVB_IMG_RAW16;
			}
			if (!strcmp(X_PIXEL_FORMAT_PROPERTY->items[item].name, Y8_NAME)) {
				return SVB_IMG_Y8;
			}
			if (!strcmp(X_PIXEL_FORMAT_PROPERTY->items[item].name, Y16_NAME)) {
				return SVB_IMG_Y16;
			}
		}
		item++;
	}
	return SVB_IMG_END;
}

static bool pixel_format_supported(indigo_device *device, SVB_IMG_TYPE type) {
	for (int i = 0; i < 8; i++) {
		if (PRIVATE_DATA->property.SupportedVideoFormat[i] == SVB_IMG_END) {
			return false;
		}
		if (type == PRIVATE_DATA->property.SupportedVideoFormat[i]) {
			return true;
		}
	}
	return false;
}

static bool svb_open(indigo_device *device) {
	indigo_lock_master_device(device);
	SVB_ERROR_CODE result = SVBOpenCamera(PRIVATE_DATA->dev_id);
	if (result != SVB_SUCCESS) {
		indigo_unlock_master_device(device);
		return false;
	}
	result = SVBSetAutoSaveParam(PRIVATE_DATA->dev_id, SVB_FALSE);
	if (result != SVB_SUCCESS) {
		SVBCloseCamera(PRIVATE_DATA->dev_id);
		indigo_unlock_master_device(device);
		return false;
	}
	SVBStopVideoCapture(PRIVATE_DATA->dev_id);
	if (PRIVATE_DATA->property.IsTriggerCam) {
		result = SVBSetCameraMode(PRIVATE_DATA->dev_id, SVB_MODE_TRIG_SOFT);
	}
	if (result != SVB_SUCCESS) {
		SVBCloseCamera(PRIVATE_DATA->dev_id);
		indigo_unlock_master_device(device);
		return false;
	}
	PRIVATE_DATA->first_frame = true;
	PRIVATE_DATA->buffer_size = PRIVATE_DATA->property.MaxHeight * PRIVATE_DATA->property.MaxWidth * (PRIVATE_DATA->property.IsColorCam ? 3 : 2) + FITS_HEADER_SIZE + 1024;
	PRIVATE_DATA->buffer = indigo_alloc_blob_buffer(PRIVATE_DATA->buffer_size);
	if (PRIVATE_DATA->buffer == NULL) {
		SVBCloseCamera(PRIVATE_DATA->dev_id);
		indigo_unlock_master_device(device);
		return false;
	}
	indigo_unlock_master_device(device);
	return true;
}

static void svb_close(indigo_device *device) {
	indigo_lock_master_device(device);
	SVB_ERROR_CODE result = SVBCloseCamera(PRIVATE_DATA->dev_id);
	if (result != SVB_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "SVBCloseCamera(%d) = %d", PRIVATE_DATA->dev_id, result);
	}
	free(PRIVATE_DATA->buffer);
	PRIVATE_DATA->buffer = NULL;
	indigo_unlock_master_device(device);
}

static bool svb_setup_exposure(indigo_device *device, double exposure, int frame_left, int frame_top, int frame_width, int frame_height, int horizontal_bin, int vertical_bin) {
	int id = PRIVATE_DATA->dev_id;
	if (horizontal_bin != vertical_bin || horizontal_bin <= 0) {
		return false;
	}
	SVB_ERROR_CODE res = SVB_SUCCESS;
	int c_frame_left = 0, c_frame_top = 0, c_frame_width = 0, c_frame_height = 0, c_bin = 0;
	long c_exposure;
	SVB_IMG_TYPE c_pixel_format = SVB_IMG_END;
	frame_width = frame_width / horizontal_bin / 8 * 8 * horizontal_bin;
	frame_height = frame_height / vertical_bin / 2 * 2 * vertical_bin;
	int requested_format = get_pixel_format(device);
	res = SVBGetOutputImageType(id, &c_pixel_format);
	if (res == SVB_SUCCESS && (PRIVATE_DATA->first_frame || c_pixel_format != requested_format)) {
		res = SVBSetOutputImageType(id, requested_format);
		if (res == SVB_SUCCESS) {
			PRIVATE_DATA->first_frame = false;
		}
	}
	if (res == SVB_SUCCESS) {
		res = SVBGetROIFormat(id, &c_frame_left, &c_frame_top, &c_frame_width, &c_frame_height, &c_bin);
	}
	int left = frame_left / horizontal_bin;
	int top = frame_top / vertical_bin;
	int width = frame_width / horizontal_bin;
	int height = frame_height / vertical_bin;
	if (res == SVB_SUCCESS && (c_frame_left != left || c_frame_top != top || c_frame_width != width || c_frame_height != height || c_bin != horizontal_bin)) {
		res = SVBSetROIFormat(id, left, top, width, height, horizontal_bin);
	}
	SVB_BOOL pauto;
	if (res == SVB_SUCCESS) {
		res = SVBGetControlValue(id, SVB_EXPOSURE, &c_exposure, &pauto);
	}
	if (res == SVB_SUCCESS && c_exposure != (long)s2us(exposure)) {
		res = SVBSetControlValue(id, SVB_EXPOSURE, (long)s2us(exposure), SVB_FALSE);
	}
	if (res == SVB_SUCCESS) {
		res = SVBGetROIFormat(id, &c_frame_left, &c_frame_top, &c_frame_width, &c_frame_height, &c_bin);
	}
	if (res != SVB_SUCCESS || c_bin != horizontal_bin || c_frame_left != left || c_frame_top != top || c_frame_width != width || c_frame_height != height || c_frame_width <= 0 || c_frame_height <= 0 || c_frame_width > PRIVATE_DATA->property.MaxWidth / c_bin || c_frame_height > PRIVATE_DATA->property.MaxHeight / c_bin || c_frame_width % 8 || c_frame_height % 2) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Invalid ROI readback: result %d, %d,%d %dx%d bin %d", res, c_frame_left, c_frame_top, c_frame_width, c_frame_height, c_bin);
		return false;
	}
	PRIVATE_DATA->exp_bin_x = c_bin;
	PRIVATE_DATA->exp_bin_y = c_bin;
	PRIVATE_DATA->exp_frame_width = c_frame_width * c_bin;
	PRIVATE_DATA->exp_frame_height = c_frame_height * c_bin;
	PRIVATE_DATA->exp_bpp = (int)CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value;
	return true;
}

static bool svb_set_cooler(indigo_device *device, bool status, double target, double *current, long *cooler_power) {
	SVB_ERROR_CODE res = SVB_SUCCESS;
	SVB_BOOL unused;
	int id = PRIVATE_DATA->dev_id;
	long current_status;
	long temp_x10;
	bool success = true;
	if (PRIVATE_DATA->has_temperature_sensor) {
		res = SVBGetControlValue(id, SVB_CURRENT_TEMPERATURE, &temp_x10, &unused);
		if (res != SVB_SUCCESS) {
			return false;
		}
		*current = temp_x10 / 10.0;
	} else {
		*current = 0;
	}
	if (!PRIVATE_DATA->has_cooler) {
		return true;
	}
	res = SVBGetControlValue(id, SVB_COOLER_ENABLE, &current_status, &unused);
	if (res != SVB_SUCCESS) {
		return false;
	}
	if (current_status != status) {
		res = SVBSetControlValue(id, SVB_COOLER_ENABLE, status, SVB_FALSE);
		success = res == SVB_SUCCESS;
	} else if (status) {
		long current_target = 0;
		res = SVBGetControlValue(id, SVB_TARGET_TEMPERATURE, &current_target, &unused);
		if (res != SVB_SUCCESS) {
			return false;
		}
		long requested_target = lround(target * 10);
		if (requested_target != current_target) {
			res = SVBSetControlValue(id, SVB_TARGET_TEMPERATURE, requested_target, SVB_FALSE);
			success = res == SVB_SUCCESS;
		}
	}
	res = SVBGetControlValue(id, SVB_COOLER_POWER, cooler_power, &unused);
	return success && res == SVB_SUCCESS;
}

static void acquisition_finish(indigo_device *device, bool failed, bool aborted) {
	bool streaming = PRIVATE_DATA->streaming;
	if (PRIVATE_DATA->video_started) {
		SVB_ERROR_CODE result = SVBStopVideoCapture(PRIVATE_DATA->dev_id);
		failed = failed || result != SVB_SUCCESS;
	}
	PRIVATE_DATA->acquisition_active = false;
	PRIVATE_DATA->video_started = false;
	PRIVATE_DATA->frame_ready = false;
	PRIVATE_DATA->can_check_temperature = true;
	indigo_cancel_pending_handler(device, acquisition_finalizer);
	if (streaming) {
		indigo_finalize_video_stream(device);
	}
	if (aborted) {
		indigo_ccd_abort_exposure_cleanup(device);
		if (failed) {
			CCD_ABORT_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_ABORT_EXPOSURE_PROPERTY, "Camera stop failed");
		}
	} else {
		if (failed) {
			indigo_ccd_failure_cleanup(device);
		}
		indigo_property *property = streaming ? CCD_STREAMING_PROPERTY : CCD_EXPOSURE_PROPERTY;
		property->items->number.value = 0;
		property->state = failed ? INDIGO_ALERT_STATE : INDIGO_OK_STATE;
		indigo_update_property(device, property, failed ? "Acquisition failed" : NULL);
	}
}

static void acquisition_finalizer(indigo_device *device) {
	if (!IS_CONNECTED || !PRIVATE_DATA->acquisition_active) {
		return;
	}
	bool streaming = PRIVATE_DATA->streaming;
	double now = indigo_monotonic_time();
	if (now < PRIVATE_DATA->exposure_end) {
		if (streaming) {
			CCD_STREAMING_EXPOSURE_ITEM->number.value = ceil(fmax(0, PRIVATE_DATA->exposure_end - now));
			indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
		}
		indigo_execute_handler_in(device, fmin(0.1, PRIVATE_DATA->exposure_end - now), acquisition_finalizer);
		return;
	}
	SVB_ERROR_CODE result = SVB_SUCCESS;
	if (!PRIVATE_DATA->frame_ready) {
		PRIVATE_DATA->can_check_temperature = false;
		result = SVBGetVideoData(PRIVATE_DATA->dev_id, PRIVATE_DATA->buffer + FITS_HEADER_SIZE, PRIVATE_DATA->buffer_size - FITS_HEADER_SIZE, 20);
		PRIVATE_DATA->can_check_temperature = true;
		if (result == SVB_ERROR_TIMEOUT && indigo_monotonic_time() < PRIVATE_DATA->readout_deadline) {
			indigo_execute_handler_in(device, 0.01, acquisition_finalizer);
			return;
		}
		if (result) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Acquisition readout failed: %d", result);
			acquisition_finish(device, true, false);
			return;
		}
		PRIVATE_DATA->frame_ready = true;
	}
	// Abort/disconnect may have been requested while the SDK call was running.
	if (!CONNECTION_CONNECTED_ITEM->sw.value || CCD_ABORT_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		return;
	}
	char *bayer = get_bayer_string(device);
	indigo_fits_keyword keywords[] = { { INDIGO_FITS_STRING, "BAYERPAT", .string = bayer, "Bayer color pattern" }, { 0 } };
	indigo_process_image(device, PRIVATE_DATA->buffer, PRIVATE_DATA->exp_frame_width / PRIVATE_DATA->exp_bin_x, PRIVATE_DATA->exp_frame_height / PRIVATE_DATA->exp_bin_y, PRIVATE_DATA->exp_bpp, true, false, bayer && PRIVATE_DATA->exp_bpp != 24 ? keywords : NULL, streaming);
	PRIVATE_DATA->frame_ready = false;
	if (streaming && CCD_STREAMING_COUNT_ITEM->number.value > 0) {
		CCD_STREAMING_COUNT_ITEM->number.value--;
	}
	if (!streaming || CCD_STREAMING_COUNT_ITEM->number.value == 0) {
		acquisition_finish(device, false, false);
	} else {
		result = SVBSendSoftTrigger(PRIVATE_DATA->dev_id);
		if (result != SVB_SUCCESS) {
			acquisition_finish(device, true, false);
			return;
		}
		PRIVATE_DATA->exposure_end = indigo_monotonic_time() + PRIVATE_DATA->exposure_duration;
		PRIVATE_DATA->readout_deadline = PRIVATE_DATA->exposure_end + fmax(2, PRIVATE_DATA->exposure_duration + 0.5);
		CCD_STREAMING_EXPOSURE_ITEM->number.value = ceil(PRIVATE_DATA->exposure_duration);
		indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
		indigo_execute_handler_in(device, fmin(0.1, PRIVATE_DATA->exposure_duration), acquisition_finalizer);
	}
}

static void acquisition_start(indigo_device *device, bool streaming) {
	PRIVATE_DATA->streaming = streaming;
	PRIVATE_DATA->frame_ready = false;
	PRIVATE_DATA->exposure_duration = streaming ? CCD_STREAMING_EXPOSURE_ITEM->number.target : CCD_EXPOSURE_ITEM->number.target;
	if (streaming && CCD_STREAMING_COUNT_ITEM->number.target == 0) {
		acquisition_finish(device, false, false);
		return;
	}
	SVB_ERROR_CODE error = SVB_SUCCESS;
	for (int i = 0; error == SVB_SUCCESS && i < 8; i++) {
		error = SVBGetVideoData(PRIVATE_DATA->dev_id, PRIVATE_DATA->buffer + FITS_HEADER_SIZE, PRIVATE_DATA->buffer_size - FITS_HEADER_SIZE, 1);
	}
	// SV305 Pro short exposures can time out if the pending-frame drain is performed after stop.
	SVBStopVideoCapture(PRIVATE_DATA->dev_id);
	bool result = error == SVB_ERROR_TIMEOUT && svb_setup_exposure(device, PRIVATE_DATA->exposure_duration, CCD_FRAME_LEFT_ITEM->number.value, CCD_FRAME_TOP_ITEM->number.value, CCD_FRAME_WIDTH_ITEM->number.value, CCD_FRAME_HEIGHT_ITEM->number.value, CCD_BIN_HORIZONTAL_ITEM->number.value, CCD_BIN_VERTICAL_ITEM->number.value);
	if (result) {
		error = SVBStartVideoCapture(PRIVATE_DATA->dev_id);
		if (error == SVB_SUCCESS) {
			PRIVATE_DATA->video_started = true;
			error = SVBSendSoftTrigger(PRIVATE_DATA->dev_id);
		}
		result = error == SVB_SUCCESS;
	}
	if (!result) {
		acquisition_finish(device, true, false);
		return;
	}
	PRIVATE_DATA->acquisition_active = true;
	PRIVATE_DATA->exposure_end = indigo_monotonic_time() + PRIVATE_DATA->exposure_duration;
	PRIVATE_DATA->readout_deadline = PRIVATE_DATA->exposure_end + fmax(2, PRIVATE_DATA->exposure_duration + 0.5);
	indigo_execute_handler_in(device, fmin(0.1, PRIVATE_DATA->exposure_duration), acquisition_finalizer);
}

static void ccd_temperature_callback(indigo_device *device) {
	if (!CONNECTION_CONNECTED_ITEM->sw.value) {
		return;
	}
	if (PRIVATE_DATA->can_check_temperature) {
		if (svb_set_cooler(device, CCD_COOLER_ON_ITEM->sw.value, PRIVATE_DATA->target_temperature, &PRIVATE_DATA->current_temperature, &PRIVATE_DATA->cooler_power)) {
			double diff = PRIVATE_DATA->current_temperature - PRIVATE_DATA->target_temperature;
			if (CCD_COOLER_ON_ITEM->sw.value) {
				CCD_TEMPERATURE_PROPERTY->state = fabs(diff) > 0.5 ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
			} else {
				CCD_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
			}
			CCD_TEMPERATURE_ITEM->number.value = PRIVATE_DATA->current_temperature;
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
	GUIDER_GUIDE_EAST_ITEM->number.value = 0;
	GUIDER_GUIDE_WEST_ITEM->number.value = 0;
	GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
}

static void guider_dec_finalizer(indigo_device *device) {
	GUIDER_GUIDE_NORTH_ITEM->number.value = 0;
	GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
	GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
}

static indigo_result handle_advanced_property(indigo_device *device, indigo_property *property) {
	int ctrl_count;
	int id = PRIVATE_DATA->dev_id;
	if (!IS_CONNECTED) {
		return INDIGO_OK;
	}
	if (SVBGetNumOfControls(id, &ctrl_count) != SVB_SUCCESS || ctrl_count < 0 || ctrl_count > 1024) {
		return INDIGO_FAILED;
	}
	bool failed = false;
	for (int ctrl_no = 0; ctrl_no < ctrl_count; ctrl_no++) {
		SVB_CONTROL_CAPS caps;
		if (SVBGetControlCaps(id, ctrl_no, &caps) != SVB_SUCCESS) {
			failed = true;
			continue;
		}
		caps.Name[sizeof(caps.Name) - 1] = 0;
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = property->items + i;
			if (strcmp(caps.Name, item->name)) {
				continue;
			}
			long value;
			SVB_BOOL automatic;
			SVB_ERROR_CODE read_result = SVBGetControlValue(id, caps.ControlType, &value, &automatic);
			SVB_ERROR_CODE write_result = SVB_SUCCESS;
			if (!read_result && (value < caps.MinValue || value > caps.MaxValue)) {
				read_result = SVB_ERROR_OUTOF_BOUNDARY;
			}
			if (!read_result && item->number.value != item->number.target && value != (long)item->number.target) {
				write_result = SVBSetControlValue(id, caps.ControlType, (long)item->number.target, SVB_FALSE);
				read_result = SVBGetControlValue(id, caps.ControlType, &value, &automatic);
			}
			if (!read_result && value >= caps.MinValue && value <= caps.MaxValue) {
				item->number.value = item->number.target = value;
			} else {
				item->number.target = item->number.value;
				read_result = SVB_ERROR_OUTOF_BOUNDARY;
			}
			if (read_result || write_result) {
				failed = true;
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Advanced control %s: write %d, read %d", item->name, write_result, read_result);
			}
		}
	}
	return failed ? INDIGO_FAILED : INDIGO_OK;
}

static indigo_result init_camera_property(indigo_device *device, SVB_CONTROL_CAPS ctrl_caps) {
	ctrl_caps.Name[sizeof(ctrl_caps.Name) - 1] = 0;
	if (ctrl_caps.MinValue > ctrl_caps.MaxValue || ctrl_caps.ControlType < 0 || !ctrl_caps.Name[0]) {
		return INDIGO_FAILED;
	}
	int id = PRIVATE_DATA->dev_id;
	long value;
	SVB_ERROR_CODE res;
	SVB_BOOL unused;
	if (ctrl_caps.ControlType == SVB_EXPOSURE) {
		CCD_EXPOSURE_PROPERTY->hidden = false;
		if (ctrl_caps.IsWritable) {
			CCD_EXPOSURE_PROPERTY->perm = INDIGO_RW_PERM;
		} else {
			CCD_EXPOSURE_PROPERTY->perm = INDIGO_RO_PERM;
		}
		CCD_EXPOSURE_ITEM->number.min = us2s(ctrl_caps.MinValue);
		CCD_EXPOSURE_ITEM->number.max = us2s(ctrl_caps.MaxValue);
		res = SVBGetControlValue(id, SVB_EXPOSURE, &value, &unused);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "SVBGetControlValue(%d, SVB_EXPOSURE) = %d", id, res);
		return INDIGO_FAILED;
		}
		if (value < ctrl_caps.MinValue || value > ctrl_caps.MaxValue) {
			return INDIGO_FAILED;
		}
		CCD_EXPOSURE_ITEM->number.value = CCD_EXPOSURE_ITEM->number.target = us2s(value);
		return INDIGO_OK;
	}
	if (ctrl_caps.ControlType == SVB_GAIN) {
		CCD_GAIN_PROPERTY->hidden = false;
		if (ctrl_caps.IsWritable) {
			CCD_GAIN_PROPERTY->perm = INDIGO_RW_PERM;
		} else {
			CCD_GAIN_PROPERTY->perm = INDIGO_RO_PERM;
		}
		CCD_GAIN_ITEM->number.min = ctrl_caps.MinValue;
		CCD_GAIN_ITEM->number.max = ctrl_caps.MaxValue;
		res = SVBGetControlValue(id, SVB_GAIN, &value, &unused);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "SVBGetControlValue(%d, SVB_GAIN) = %d", id, res);
		return INDIGO_FAILED;
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "SVBGetControlValue(%d, SVB_GAIN) = %d -> %d", id, res, value);
		}
		if (value < ctrl_caps.MinValue || value > ctrl_caps.MaxValue) {
			return INDIGO_FAILED;
		}
		CCD_GAIN_ITEM->number.value = CCD_GAIN_ITEM->number.target = value;
		CCD_GAIN_ITEM->number.step = 1;
		return INDIGO_OK;
	}
	if (ctrl_caps.ControlType == SVB_GAMMA) {
		CCD_GAMMA_PROPERTY->hidden = false;
		if (ctrl_caps.IsWritable) {
			CCD_GAMMA_PROPERTY->perm = INDIGO_RW_PERM;
		} else {
			CCD_GAMMA_PROPERTY->perm = INDIGO_RO_PERM;
		}
		CCD_GAMMA_ITEM->number.min = ctrl_caps.MinValue;
		CCD_GAMMA_ITEM->number.max = ctrl_caps.MaxValue;
		res = SVBGetControlValue(id, SVB_GAMMA, &value, &unused);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "SVBGetControlValue(%d, SVB_GAMMA) = %d", id, res);
		return INDIGO_FAILED;
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "SVBGetControlValue(%d, SVB_GAMMA) = %d -> %d", id, res, value);
		}
		if (value < ctrl_caps.MinValue || value > ctrl_caps.MaxValue) {
			return INDIGO_FAILED;
		}
		CCD_GAMMA_ITEM->number.value = CCD_GAMMA_ITEM->number.target = value;
		CCD_GAMMA_ITEM->number.step = 1;
		return INDIGO_OK;
	}
	if (ctrl_caps.ControlType == SVB_BLACK_LEVEL) {
		CCD_OFFSET_PROPERTY->hidden = false;
		if (ctrl_caps.IsWritable) {
			CCD_OFFSET_PROPERTY->perm = INDIGO_RW_PERM;
		} else {
			CCD_OFFSET_PROPERTY->perm = INDIGO_RO_PERM;
		}
		CCD_OFFSET_ITEM->number.min = ctrl_caps.MinValue;
		CCD_OFFSET_ITEM->number.max = ctrl_caps.MaxValue;
		res = SVBGetControlValue(id, SVB_BLACK_LEVEL, &value, &unused);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "SVBGetControlValue(%d, SVB_BLACK_LEVEL) = %d", id, res);
		return INDIGO_FAILED;
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "SVBGetControlValue(%d, SVB_BLACK_LEVEL) = %d -> %d", id, res, value);
		}
		if (value < ctrl_caps.MinValue || value > ctrl_caps.MaxValue) {
			return INDIGO_FAILED;
		}
		CCD_OFFSET_ITEM->number.value = CCD_OFFSET_ITEM->number.target = value;
		CCD_OFFSET_ITEM->number.step = 1;
		return INDIGO_OK;
	}
	if (ctrl_caps.ControlType == SVB_TARGET_TEMPERATURE) {
		CCD_TEMPERATURE_PROPERTY->hidden = false;
		if (ctrl_caps.IsWritable) {
			CCD_TEMPERATURE_PROPERTY->perm = INDIGO_RW_PERM;
		} else {
			CCD_TEMPERATURE_PROPERTY->perm = INDIGO_RO_PERM;
		}
		CCD_TEMPERATURE_ITEM->number.min = ctrl_caps.MinValue / 10.0;
		CCD_TEMPERATURE_ITEM->number.max = ctrl_caps.MaxValue / 10.0;
		if (SVBGetControlValue(id, SVB_TARGET_TEMPERATURE, &value, &unused) != SVB_SUCCESS || value < ctrl_caps.MinValue || value > ctrl_caps.MaxValue) {
			return INDIGO_FAILED;
		}
		CCD_TEMPERATURE_ITEM->number.target = value / 10.0;
		PRIVATE_DATA->target_temperature = value / 10.0;
		PRIVATE_DATA->can_check_temperature = true;
		return INDIGO_OK;
	}
	if (ctrl_caps.ControlType == SVB_CURRENT_TEMPERATURE) {
		if (CCD_TEMPERATURE_PROPERTY->hidden) {
			PRIVATE_DATA->can_check_temperature = true;
			CCD_TEMPERATURE_PROPERTY->perm = INDIGO_RO_PERM;
			CCD_TEMPERATURE_PROPERTY->hidden = false;
		}
		PRIVATE_DATA->has_temperature_sensor = true;
		return INDIGO_OK;
	}
	if (ctrl_caps.ControlType == SVB_COOLER_ENABLE) {
		if (SVBGetControlValue(id, SVB_COOLER_ENABLE, &value, &unused) != SVB_SUCCESS || (value != 0 && value != 1)) {
			return INDIGO_FAILED;
		}
		PRIVATE_DATA->has_cooler = true;
		indigo_set_switch(CCD_COOLER_PROPERTY, CCD_COOLER_ON_ITEM, value != 0);
		CCD_COOLER_PROPERTY->hidden = false;
		if (ctrl_caps.IsWritable) {
			CCD_COOLER_PROPERTY->perm = INDIGO_RW_PERM;
		} else {
			CCD_COOLER_PROPERTY->perm = INDIGO_RO_PERM;
		}
		return INDIGO_OK;
	}
	if (ctrl_caps.ControlType == SVB_COOLER_POWER) {
		CCD_COOLER_POWER_PROPERTY->hidden = false;
		if (ctrl_caps.IsWritable) {
			CCD_COOLER_POWER_PROPERTY->perm = INDIGO_RW_PERM;
		} else {
			CCD_COOLER_POWER_PROPERTY->perm = INDIGO_RO_PERM;
		}
		CCD_COOLER_POWER_ITEM->number.min = ctrl_caps.MinValue;
		CCD_COOLER_POWER_ITEM->number.max = ctrl_caps.MaxValue;
		res = SVBGetControlValue(id, SVB_COOLER_POWER, &value, &unused);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "SVBGetControlValue(%d, SVB_COOLER_POWER) = %d", id, res);
		return INDIGO_FAILED;
		}
		if (value < ctrl_caps.MinValue || value > ctrl_caps.MaxValue) {
			return INDIGO_FAILED;
		}
		CCD_COOLER_POWER_ITEM->number.value = CCD_COOLER_POWER_ITEM->number.target = value;
		return INDIGO_OK;
	}
	if (ctrl_caps.ControlType == SVB_AUTO_TARGET_BRIGHTNESS) {
		return INDIGO_OK;
	}
	if (!ctrl_caps.IsWritable) {
		return INDIGO_OK;
	}
	int offset = X_ADVANCED_PROPERTY->count;
	res = SVBGetControlValue(id, ctrl_caps.ControlType, &value, &unused);
	if (res) {
		return INDIGO_FAILED;
	}
	if (res) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "SVBGetControlValue(%d, %s) = %d", id, ctrl_caps.Name, res);
		return INDIGO_FAILED;
	}
	if (value < ctrl_caps.MinValue || value > ctrl_caps.MaxValue) {
		return INDIGO_FAILED;
	}
	for (int i = 0; i < X_ADVANCED_PROPERTY->count; i++) {
		if (!strcmp(X_ADVANCED_PROPERTY->items[i].name, ctrl_caps.Name)) {
			return INDIGO_FAILED;
		}
	}
	X_ADVANCED_PROPERTY = indigo_resize_property(X_ADVANCED_PROPERTY, offset + 1);
	indigo_init_number_item(X_ADVANCED_PROPERTY->items + offset, ctrl_caps.Name, ctrl_caps.Name, ctrl_caps.MinValue, ctrl_caps.MaxValue, 1, value);
	return INDIGO_OK;
}

static bool initialize_camera(indigo_device *device) {
	PRIVATE_DATA->has_temperature_sensor = false;
	PRIVATE_DATA->has_cooler = false;
	PRIVATE_DATA->can_check_temperature = false;
	CCD_GAIN_PROPERTY->hidden = CCD_GAMMA_PROPERTY->hidden = CCD_OFFSET_PROPERTY->hidden = true;
	CCD_TEMPERATURE_PROPERTY->hidden = CCD_COOLER_PROPERTY->hidden = CCD_COOLER_POWER_PROPERTY->hidden = true;
	int id = PRIVATE_DATA->dev_id;
	int ctrl_count = 0;
	SVB_CONTROL_CAPS ctrl_caps;
	int res = SVBGetNumOfControls(id, &ctrl_count);
	if (res || ctrl_count < 0 || ctrl_count > 1024) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "SVBGetNumOfControls(%d) = %d", id, res);
		return false;
	}
	X_ADVANCED_PROPERTY = indigo_resize_property(X_ADVANCED_PROPERTY, 0);
	for (int ctrl_no = 0; ctrl_no < ctrl_count; ctrl_no++) {
		res = SVBGetControlCaps(id, ctrl_no, &ctrl_caps);
		if (res || init_camera_property(device, ctrl_caps) != INDIGO_OK) {
			return false;
		}
	}
	return true;
}

static bool svb_set_number(indigo_device *device, indigo_property *property, SVB_CONTROL_TYPE control, long target) {
	long value;
	SVB_BOOL automatic;
	SVB_ERROR_CODE write_result = SVBSetControlValue(PRIVATE_DATA->dev_id, control, target, SVB_FALSE);
	SVB_ERROR_CODE read_result = SVBGetControlValue(PRIVATE_DATA->dev_id, control, &value, &automatic);
	if (!read_result && (value < property->items->number.min || value > property->items->number.max)) {
		read_result = SVB_ERROR_OUTOF_BOUNDARY;
	}
	if (!read_result) {
		property->items->number.value = property->items->number.target = value;
	} else {
		property->items->number.target = property->items->number.value;
	}
	property->state = write_result || read_result ? INDIGO_ALERT_STATE : INDIGO_OK_STATE;
	return property->state == INDIGO_OK_STATE;
}

static void initialize_properties(indigo_device *device) {
	X_PIXEL_FORMAT_PROPERTY = indigo_resize_property(X_PIXEL_FORMAT_PROPERTY, SVB_MAX_FORMATS);
	int format_count = 0;
	if (pixel_format_supported(device, SVB_IMG_RAW8)) {
		indigo_init_switch_item(X_PIXEL_FORMAT_PROPERTY->items + format_count, RAW8_NAME, RAW8_NAME, true);
		format_count++;
	}
	if (pixel_format_supported(device, SVB_IMG_RGB24)) {
		indigo_init_switch_item(X_PIXEL_FORMAT_PROPERTY->items + format_count, RGB24_NAME, RGB24_NAME, false);
		format_count++;
	}
	if (pixel_format_supported(device, SVB_IMG_RAW16)) {
		indigo_init_switch_item(X_PIXEL_FORMAT_PROPERTY->items + format_count, RAW16_NAME, RAW16_NAME, false);
		format_count++;
	}
	if (pixel_format_supported(device, SVB_IMG_Y8)) {
		indigo_init_switch_item(X_PIXEL_FORMAT_PROPERTY->items + format_count, Y8_NAME, Y8_NAME, false);
		format_count++;
	}
	if (pixel_format_supported(device, SVB_IMG_Y16)) {
		indigo_init_switch_item(X_PIXEL_FORMAT_PROPERTY->items + format_count, Y16_NAME, Y16_NAME, false);
		format_count++;
	}
	X_PIXEL_FORMAT_PROPERTY->count = format_count;
	indigo_set_switch(X_PIXEL_FORMAT_PROPERTY, X_PIXEL_FORMAT_PROPERTY->items, true);
	INFO_PROPERTY->count = 6;
	INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, PRIVATE_DATA->info.FriendlyName);
	const char *sdk_version = SVBGetSDKVersion();
	INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, sdk_version);
	INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->label, "SDK version");
	if (PRIVATE_DATA->info.CameraSN[0] != '\0') {
		INFO_PROPERTY->count = 8;
		INDIGO_COPY_VALUE(INFO_DEVICE_SERIAL_NUM_ITEM->text.value, PRIVATE_DATA->info.CameraSN);
	}
	CCD_INFO_WIDTH_ITEM->number.value = PRIVATE_DATA->property.MaxWidth;
	CCD_INFO_HEIGHT_ITEM->number.value = PRIVATE_DATA->property.MaxHeight;
	CCD_INFO_PIXEL_SIZE_ITEM->number.value = CCD_INFO_PIXEL_WIDTH_ITEM->number.value = CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = PRIVATE_DATA->pixel_size;
	CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = PRIVATE_DATA->property.MaxBitDepth;
	CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.target = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max = PRIVATE_DATA->property.MaxWidth;
	CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.target = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max = PRIVATE_DATA->property.MaxHeight;
	CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.target = get_pixel_depth(device);
	CCD_FRAME_BITS_PER_PIXEL_ITEM->number.min = 8;
	CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = 24;
	/* find max binning */
	int max_bin = 1;
	for (int num = 0; (num < 16) && PRIVATE_DATA->property.SupportedBins[num]; num++) {
		max_bin = fmax(max_bin, PRIVATE_DATA->property.SupportedBins[num]);
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
	for (int num = 0; (num < 16) && PRIVATE_DATA->property.SupportedBins[num]; num++) {
		int bin = PRIVATE_DATA->property.SupportedBins[num];
		if (pixel_format_supported(device, SVB_IMG_RAW8)) {
			snprintf(name, 32, "%s %dx%d", RAW8_NAME, bin, bin);
			snprintf(label, 64, "%s %dx%d", RAW8_NAME, (int)CCD_FRAME_WIDTH_ITEM->number.value / bin, (int)CCD_FRAME_HEIGHT_ITEM->number.value / bin);
			indigo_init_switch_item(CCD_MODE_PROPERTY->items + mode_count, name, label, bin == 1);
			mode_count++;
		}
		if (pixel_format_supported(device, SVB_IMG_RGB24)) {
			snprintf(name, 32, "%s %dx%d", RGB24_NAME, bin, bin);
			snprintf(label, 64, "%s %dx%d", RGB24_NAME, (int)CCD_FRAME_WIDTH_ITEM->number.value / bin, (int)CCD_FRAME_HEIGHT_ITEM->number.value / bin);
			indigo_init_switch_item(CCD_MODE_PROPERTY->items + mode_count, name, label, false);
			mode_count++;
		}
		if (pixel_format_supported(device, SVB_IMG_RAW16)) {
			snprintf(name, 32, "%s %dx%d", RAW16_NAME, bin, bin);
			snprintf(label, 64, "%s %dx%d", RAW16_NAME, (int)CCD_FRAME_WIDTH_ITEM->number.value / bin, (int)CCD_FRAME_HEIGHT_ITEM->number.value / bin);
			indigo_init_switch_item(CCD_MODE_PROPERTY->items + mode_count, name, label, false);
			mode_count++;
		}
		if (pixel_format_supported(device, SVB_IMG_Y8)) {
			snprintf(name, 32, "%s %dx%d", Y8_NAME, bin, bin);
			snprintf(label, 64, "%s %dx%d", Y8_NAME, (int)CCD_FRAME_WIDTH_ITEM->number.value / bin, (int)CCD_FRAME_HEIGHT_ITEM->number.value / bin);
			indigo_init_switch_item(CCD_MODE_PROPERTY->items + mode_count, name, label, false);
			mode_count++;
		}
		if (pixel_format_supported(device, SVB_IMG_Y16)) {
			snprintf(name, 32, "%s %dx%d", Y16_NAME, bin, bin);
			snprintf(label, 64, "%s %dx%d", Y16_NAME, (int)CCD_FRAME_WIDTH_ITEM->number.value / bin, (int)CCD_FRAME_HEIGHT_ITEM->number.value / bin);
			indigo_init_switch_item(CCD_MODE_PROPERTY->items + mode_count, name, label, false);
			mode_count++;
		}
	}
	CCD_MODE_PROPERTY->count = mode_count;
	indigo_set_switch(CCD_MODE_PROPERTY, CCD_MODE_PROPERTY->items, true);
	// -------------------------------------------------------------------------------- CCD_STREAMING
	CCD_STREAMING_PROPERTY->hidden = false;
	CCD_STREAMING_SETTINGS_PROPERTY->hidden = false;
	CCD_IMAGE_FORMAT_PROPERTY->count = 7;
	CCD_STREAMING_EXPOSURE_ITEM->number.max = 5.0;
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
			connection_result = svb_open(device);
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
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s", device->name);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				svb_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ ccd.on_disconnect
		indigo_lock_master_device(device);
		if (PRIVATE_DATA->acquisition_active || CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
			acquisition_finish(device, false, true);
		}
		PRIVATE_DATA->can_check_temperature = false;
		indigo_unlock_master_device(device);
		//- ccd.on_disconnect
		indigo_delete_property(device, X_PIXEL_FORMAT_PROPERTY, NULL);
		indigo_delete_property(device, X_ADVANCED_PROPERTY, NULL);
		if (--PRIVATE_DATA->count == 0) {
			svb_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_ccd_change_property(device, NULL, CONNECTION_PROPERTY);
	if (IS_CONNECTED) {
		indigo_execute_handler(device, ccd_timer_callback);
	}
}

static void ccd_exposure_handler(indigo_device *device) {
	//+ ccd.CCD_EXPOSURE.on_change
	indigo_use_shortest_exposure_if_bias(device);
	CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
	indigo_ccd_exposure_setup(device);
	acquisition_start(device, false); // acquisition_finalizer owns completion
	//- ccd.CCD_EXPOSURE.on_change
}

static void ccd_streaming_handler(indigo_device *device) {
	//+ ccd.CCD_STREAMING.on_change
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
		if (PRIVATE_DATA->acquisition_active) {
			acquisition_finish(device, false, true); // acquisition_finalizer is canceled
		} else {
			indigo_ccd_abort_exposure_cleanup(device);
		}
	} else {
		CCD_ABORT_EXPOSURE_ITEM->sw.value = false;
		CCD_ABORT_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, CCD_ABORT_EXPOSURE_PROPERTY, NULL);
	}
	//- ccd.CCD_ABORT_EXPOSURE.on_change
}

static void ccd_cooler_handler(indigo_device *device) {
	//+ ccd.CCD_COOLER.on_change
	CCD_COOLER_PROPERTY->state = INDIGO_BUSY_STATE;
	//- ccd.CCD_COOLER.on_change
	indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
}

static void ccd_temperature_handler(indigo_device *device) {
	CCD_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_TEMPERATURE.on_change
	PRIVATE_DATA->target_temperature = CCD_TEMPERATURE_ITEM->number.target;
	CCD_TEMPERATURE_PROPERTY->state = INDIGO_BUSY_STATE;
	//- ccd.CCD_TEMPERATURE.on_change
	indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
}

static void ccd_gain_handler(indigo_device *device) {
	CCD_GAIN_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_GAIN.on_change
	svb_set_number(device, CCD_GAIN_PROPERTY, SVB_GAIN, CCD_GAIN_ITEM->number.target);
	//- ccd.CCD_GAIN.on_change
	indigo_update_property(device, CCD_GAIN_PROPERTY, NULL);
}

static void ccd_gamma_handler(indigo_device *device) {
	CCD_GAMMA_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_GAMMA.on_change
	svb_set_number(device, CCD_GAMMA_PROPERTY, SVB_GAMMA, CCD_GAMMA_ITEM->number.target);
	//- ccd.CCD_GAMMA.on_change
	indigo_update_property(device, CCD_GAMMA_PROPERTY, NULL);
}

static void ccd_offset_handler(indigo_device *device) {
	CCD_OFFSET_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_OFFSET.on_change
	svb_set_number(device, CCD_OFFSET_PROPERTY, SVB_BLACK_LEVEL, CCD_OFFSET_ITEM->number.target);
	//- ccd.CCD_OFFSET.on_change
	indigo_update_property(device, CCD_OFFSET_PROPERTY, NULL);
}

static void ccd_frame_handler(indigo_device *device) {
	CCD_FRAME_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_FRAME.on_change
	if (CCD_FRAME_WIDTH_ITEM->number.value != CCD_FRAME_WIDTH_ITEM->number.max) {
		CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.target = 8 * (int)(CCD_FRAME_WIDTH_ITEM->number.value / 8);
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
	// -------------------------------------------------------------------------------- PIXEL_FORMAT
	//- ccd.CCD_FRAME.on_change
	indigo_update_property(device, CCD_FRAME_PROPERTY, NULL);
}

static void ccd_mode_handler(indigo_device *device) {
	CCD_MODE_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_MODE.on_change
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
	int prev_h_bin = CCD_BIN_HORIZONTAL_ITEM->number.value;
	int prev_v_bin = CCD_BIN_VERTICAL_ITEM->number.value;
	int horizontal_bin = (int)CCD_BIN_HORIZONTAL_ITEM->number.target;
	int vertical_bin = (int)CCD_BIN_VERTICAL_ITEM->number.target;
	int requested = prev_h_bin != horizontal_bin ? horizontal_bin : vertical_bin;
	bool supported = false;
	for (int i = 0; i < 16 && PRIVATE_DATA->property.SupportedBins[i]; i++) {
		supported |= PRIVATE_DATA->property.SupportedBins[i] == requested;
	}
	if (!supported) {
		CCD_BIN_HORIZONTAL_ITEM->number.target = CCD_BIN_HORIZONTAL_ITEM->number.value;
		CCD_BIN_VERTICAL_ITEM->number.target = CCD_BIN_VERTICAL_ITEM->number.value;
		CCD_BIN_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, CCD_BIN_PROPERTY, "Binning is not supported by the camera");
		return;
	}
	/* SVB cameras work with binx = biny for we force it here */
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
	if (handle_advanced_property(device, X_ADVANCED_PROPERTY) != INDIGO_OK) {
		X_ADVANCED_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- ccd.X_ADVANCED.on_change
	indigo_update_property(device, X_ADVANCED_PROPERTY, NULL);
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
		CCD_GAMMA_PROPERTY->hidden = true;
		CCD_OFFSET_PROPERTY->hidden = true;
		CCD_FRAME_PROPERTY->hidden = false;
		CCD_MODE_PROPERTY->hidden = false;
		CCD_BIN_PROPERTY->hidden = false;
		X_PIXEL_FORMAT_PROPERTY = indigo_init_switch_property(NULL, device->name, X_PIXEL_FORMAT_PROPERTY_NAME, CCD_ADVANCED_GROUP, "Pixel Format", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 0);
		if (X_PIXEL_FORMAT_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		//+ ccd.X_PIXEL_FORMAT.on_attach
		initialize_properties(device);
		//- ccd.X_PIXEL_FORMAT.on_attach
		X_ADVANCED_PROPERTY = indigo_init_number_property(NULL, device->name, X_ADVANCED_PROPERTY_NAME, CCD_ADVANCED_GROUP, "Advanced", INDIGO_OK_STATE, INDIGO_RW_PERM, 0);
		if (X_ADVANCED_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_PIXEL_FORMAT_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_ADVANCED_PROPERTY);
	}
	return indigo_ccd_enumerate_properties(device, client, property);
}

static indigo_result ccd_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_QUEUED_CONNECT(driver_queue, &driver_queue_mutex, ccd_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_EXPOSURE_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE, CCD_EXPOSURE_PROPERTY, "Streaming in progress, an exposure can not be started");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_EXPOSURE_PROPERTY, ccd_exposure_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_STREAMING_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE, CCD_STREAMING_PROPERTY, "Exposure in progress, streaming can not be started");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_STREAMING_PROPERTY, ccd_streaming_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(CCD_ABORT_EXPOSURE_PROPERTY, ccd_abort_exposure_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_COOLER_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_COOLER_PROPERTY, ccd_cooler_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_TEMPERATURE_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(CCD_TEMPERATURE_PROPERTY, ccd_temperature_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_GAIN_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE, CCD_GAIN_PROPERTY, "Acquisition in progress");
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(CCD_GAIN_PROPERTY, ccd_gain_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_GAMMA_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE, CCD_GAMMA_PROPERTY, "Acquisition in progress");
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(CCD_GAMMA_PROPERTY, ccd_gamma_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_OFFSET_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE, CCD_OFFSET_PROPERTY, "Acquisition in progress");
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(CCD_OFFSET_PROPERTY, ccd_offset_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_FRAME_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE, CCD_FRAME_PROPERTY, "Acquisition in progress");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_FRAME_PROPERTY, ccd_frame_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_MODE_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE, CCD_MODE_PROPERTY, "Acquisition in progress");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_MODE_PROPERTY, ccd_mode_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_BIN_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE, CCD_BIN_PROPERTY, "Acquisition in progress");
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(CCD_BIN_PROPERTY, ccd_bin_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_PIXEL_FORMAT_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE, X_PIXEL_FORMAT_PROPERTY, "Acquisition in progress");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_PIXEL_FORMAT_PROPERTY, ccd_x_pixel_format_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_ADVANCED_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE, X_ADVANCED_PROPERTY, "Acquisition in progress");
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(X_ADVANCED_PROPERTY, ccd_x_advanced_handler);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, X_PIXEL_FORMAT_PROPERTY);
			indigo_save_property(device, NULL, X_ADVANCED_PROPERTY);
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
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_ccd_detach(device);
}

#pragma mark - High level code (guider)

static void guider_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = svb_open(device->master_device);
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
				svb_close(device);
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
			svb_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_guider_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void guider_guide_ra_handler(indigo_device *device) {
	//+ guider.GUIDER_GUIDE_RA.on_change
	indigo_cancel_pending_handler(device, guider_ra_finalizer);
	int duration = (int)GUIDER_GUIDE_EAST_ITEM->number.value;
	SVB_GUIDE_DIRECTION direction = SVB_GUIDE_EAST;
	if (duration <= 0) {
		duration = (int)GUIDER_GUIDE_WEST_ITEM->number.value;
		direction = SVB_GUIDE_WEST;
	}
	if (duration <= 0) {
		guider_ra_finalizer(device);
		return;
	}
	SVB_ERROR_CODE res = SVBPulseGuide(PRIVATE_DATA->dev_id, direction, duration);
	if (res != SVB_SUCCESS) {
		GUIDER_GUIDE_EAST_ITEM->number.value = 0;
		GUIDER_GUIDE_WEST_ITEM->number.value = 0;
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, "Guide pulse failed");
		return;
	}
	GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
	indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, duration / 1000.0, guider_ra_finalizer);
	//- guider.GUIDER_GUIDE_RA.on_change
}

static void guider_guide_dec_handler(indigo_device *device) {
	//+ guider.GUIDER_GUIDE_DEC.on_change
	indigo_cancel_pending_handler(device, guider_dec_finalizer);
	int duration = (int)GUIDER_GUIDE_NORTH_ITEM->number.value;
	SVB_GUIDE_DIRECTION direction = SVB_GUIDE_NORTH;
	if (duration <= 0) {
		duration = (int)GUIDER_GUIDE_SOUTH_ITEM->number.value;
		direction = SVB_GUIDE_SOUTH;
	}
	if (duration <= 0) {
		guider_dec_finalizer(device);
		return;
	}
	SVB_ERROR_CODE res = SVBPulseGuide(PRIVATE_DATA->dev_id, direction, duration);
	if (res != SVB_SUCCESS) {
		GUIDER_GUIDE_NORTH_ITEM->number.value = 0;
		GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, "Guide pulse failed");
		return;
	}
	GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
	indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, duration / 1000.0, guider_dec_finalizer);
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
		INDIGO_PROCESS_QUEUED_CONNECT(driver_queue, &driver_queue_mutex, guider_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_RA_PROPERTY, property)) {
		//+ guider.GUIDER_GUIDE_RA.on_change_request
		GUIDER_GUIDE_EAST_ITEM->number.value = GUIDER_GUIDE_EAST_ITEM->number.target = 0;
		GUIDER_GUIDE_WEST_ITEM->number.value = GUIDER_GUIDE_WEST_ITEM->number.target = 0;
		//- guider.GUIDER_GUIDE_RA.on_change_request
		INDIGO_COPY_VALUES_PROCESS_PRIORITY_CHANGE_ANYTIME(GUIDER_GUIDE_RA_PROPERTY, guider_guide_ra_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		//+ guider.GUIDER_GUIDE_DEC.on_change_request
		GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_NORTH_ITEM->number.target = 0;
		GUIDER_GUIDE_SOUTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.target = 0;
		//- guider.GUIDER_GUIDE_DEC.on_change_request
		INDIGO_COPY_VALUES_PROCESS_PRIORITY_CHANGE_ANYTIME(GUIDER_GUIDE_DEC_PROPERTY, guider_guide_dec_handler);
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
	svb_private_data *private_data = NULL;
	bool plug_result = true;
	char name[INDIGO_NAME_SIZE] = DRIVER_LABEL;
	private_data = (svb_private_data *)indigo_safe_malloc(sizeof(svb_private_data));
	private_data->usbdev = dev;
	struct libusb_device_descriptor descriptor;
	if (!(libusb_get_device_descriptor(dev, &descriptor) == LIBUSB_SUCCESS && descriptor.idVendor == SVB_VENDOR_ID && descriptor.idProduct == SVB_PRODUCT_ID)) {
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
		SVBGetNumOfConnectedCameras();
		int count = SVBGetNumOfConnectedCameras();
		if (count < 0 || count > SVBCAMERA_ID_MAX) {
			count = 0;
		}
		for (int index = 0; index < count; index++) {
			SVB_CAMERA_INFO info = { 0 };
			if (SVBGetCameraInfo(&info, index) != SVB_SUCCESS) {
				continue;
			}
			bool attached = false;
			for (int slot = 0; slot < MAX_DEVICES; slot++) {
				if (devices[slot] != NULL && ((svb_private_data *)devices[slot]->private_data)->dev_id == info.CameraID) {
					attached = true;
					break;
				}
			}
			if (attached) {
				continue;
			}
			SVB_CAMERA_PROPERTY property = { 0 };
			SVB_CAMERA_PROPERTY_EX property_ex = { 0 };
			SVB_BOOL is_guider = SVB_FALSE;
			float pixel_size = 0;
			SVB_ERROR_CODE result = SVBOpenCamera(info.CameraID);
			bool opened = result == SVB_SUCCESS;
			if (result == SVB_SUCCESS) {
				result = SVBSetAutoSaveParam(info.CameraID, SVB_FALSE);
			}
			if (result == SVB_SUCCESS) {
				result = SVBGetCameraProperty(info.CameraID, &property);
			}
			if (result == SVB_SUCCESS) {
				SVBGetCameraPropertyEx(info.CameraID, &property_ex);
				SVBCanPulseGuide(info.CameraID, &is_guider);
				SVBGetSensorPixelSize(info.CameraID, &pixel_size);
			}
			SVB_ERROR_CODE close_result = opened ? SVBCloseCamera(info.CameraID) : SVB_SUCCESS;
			if (result != SVB_SUCCESS || close_result != SVB_SUCCESS || !svb_valid_info(&info, &property) || slots < (is_guider ? 2 : 1)) {
				continue;
			}
			private_data->dev_id = info.CameraID;
			private_data->info = info;
			private_data->property = property;
			private_data->property_ex = property_ex;
			private_data->property_ex.bSupportPulseGuide = is_guider;
			private_data->pixel_size = isfinite(pixel_size) && pixel_size > 0 ? pixel_size : 0;
			char *model_suffix = strstr(private_data->info.FriendlyName, "(CAM");
			if (model_suffix != NULL) {
				*model_suffix = 0;
			}
			snprintf(name, INDIGO_NAME_SIZE, "%.*s", INDIGO_NAME_SIZE - 1, private_data->info.FriendlyName);
			snprintf(private_data->guider_name, INDIGO_NAME_SIZE, "%.*s (guider)", INDIGO_NAME_SIZE - 10, private_data->info.FriendlyName);
			indigo_make_name_unique(name, "%d", info.CameraID);
			indigo_make_name_unique(private_data->guider_name, "%d", info.CameraID);
			plug_result = true;
			break;
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
		if (ccd_attached && private_data->property_ex.bSupportPulseGuide) {
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
	svb_private_data *private_data = NULL;
	svb_private_data *removed[MAX_DEVICES];
	int removed_count = 0;
	for (int j = MAX_DEVICES - 1; j >= 0; j--) {
		if (devices[j] != NULL) {
			indigo_device *device = devices[j];
			private_data = PRIVATE_DATA;
			bool unplug_result = private_data->usbdev == dev;
			if (last_action != INDIGO_DRIVER_SHUTDOWN) {
				//+ sdk.unplug_match
				SVBGetNumOfConnectedCameras();
				int count = SVBGetNumOfConnectedCameras();
				unplug_result = count >= 0 && count <= SVBCAMERA_ID_MAX;
				for (int index = 0; index < count; index++) {
					SVB_CAMERA_INFO info = { 0 };
					if (SVBGetCameraInfo(&info, index) != SVB_SUCCESS || info.CameraID == private_data->dev_id) {
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

indigo_result indigo_ccd_svb(indigo_driver_action action, indigo_driver_info *info) {

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			//+ on_init
			INDIGO_DRIVER_LOG(DRIVER_NAME, "SVB SDK %s", SVBGetSDKVersion());
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
			int rc = libusb_hotplug_register_callback(NULL, (libusb_hotplug_event)(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT), LIBUSB_HOTPLUG_ENUMERATE, SVB_VENDOR_ID, SVB_PRODUCT_ID, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
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
			break;

		}
		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
