// Copyright (C) 2016-2026 Rumen G. Bogdanovski
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

// This file generated from indigo_ccd_fli.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

//+ include

#include <libfli.h>

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_ccd_driver.h>
#include <indigo/indigo_uni_io.h>
#include <indigo/indigo_usb_utils.h>

#include "indigo_ccd_fli.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x0300000B
#define DRIVER_NAME          "indigo_ccd_fli"
#define DRIVER_LABEL         "FLI Camera"
#define CCD_DEVICE_NAME      "%s"
#define MAX_DEVICES          5
#define PRIVATE_DATA         ((fli_private_data *)device->private_data)

//+ define

#define FLI_VENDOR_ID        0x0f18
#define FLI_ENUM_DOMAIN      (FLIDOMAIN_USB | FLIDEVICE_CAMERA)
#define FLI_MAX_ENUMERATED   32
#define FLI_MAX_MODES        8
#define FLI_MAX_X_BIN        16
#define FLI_MAX_Y_BIN        16
#define FLI_BITS_PER_PIXEL   16
#define FLI_MIN_TEMPERATURE  (-55)
#define FLI_MAX_TEMPERATURE  45
// The SDK has no cooler switch; the cooler is disabled by asking for a
// temperature no camera can reach by cooling.
#define FLI_COOLER_OFF_SET_POINT 45
#define FLI_TEMPERATURE_THRESHOLD 0.15
#define FLI_TEMPERATURE_DELAY 3
#define FLI_EXPOSURE_POLL_DELAY 0.05
#define FLI_READOUT_TIMEOUT_CYCLES 400
#define m2um(m)              ((m) * 1e6)

//- define

#pragma mark - Property definitions

#define FLI_NFLUSHES_PROPERTY          (PRIVATE_DATA->fli_nflushes_property)
#define FLI_NFLUSHES_ITEM              (FLI_NFLUSHES_PROPERTY->items + 0)

#define FLI_NFLUSHES_PROPERTY_NAME     "FLI_NFLUSHES"
#define FLI_NFLUSHES_ITEM_NAME         "FLI_NFLUSHES"

#define FLI_CAMERA_MODE_PROPERTY       (PRIVATE_DATA->fli_camera_mode_property)
#define FLI_CAMERA_MODE_0_ITEM         (FLI_CAMERA_MODE_PROPERTY->items + 0)
#define FLI_CAMERA_MODE_1_ITEM         (FLI_CAMERA_MODE_PROPERTY->items + 1)
#define FLI_CAMERA_MODE_2_ITEM         (FLI_CAMERA_MODE_PROPERTY->items + 2)
#define FLI_CAMERA_MODE_3_ITEM         (FLI_CAMERA_MODE_PROPERTY->items + 3)
#define FLI_CAMERA_MODE_4_ITEM         (FLI_CAMERA_MODE_PROPERTY->items + 4)
#define FLI_CAMERA_MODE_5_ITEM         (FLI_CAMERA_MODE_PROPERTY->items + 5)
#define FLI_CAMERA_MODE_6_ITEM         (FLI_CAMERA_MODE_PROPERTY->items + 6)
#define FLI_CAMERA_MODE_7_ITEM         (FLI_CAMERA_MODE_PROPERTY->items + 7)

#define FLI_CAMERA_MODE_PROPERTY_NAME  "FLI_CAMERA_MODE"
#define FLI_CAMERA_MODE_0_ITEM_NAME    "MODE_0"
#define FLI_CAMERA_MODE_1_ITEM_NAME    "MODE_1"
#define FLI_CAMERA_MODE_2_ITEM_NAME    "MODE_2"
#define FLI_CAMERA_MODE_3_ITEM_NAME    "MODE_3"
#define FLI_CAMERA_MODE_4_ITEM_NAME    "MODE_4"
#define FLI_CAMERA_MODE_5_ITEM_NAME    "MODE_5"
#define FLI_CAMERA_MODE_6_ITEM_NAME    "MODE_6"
#define FLI_CAMERA_MODE_7_ITEM_NAME    "MODE_7"

#pragma mark - Private data definition

typedef struct {
	libusb_device *usbdev;
	indigo_property *fli_nflushes_property;
	indigo_property *fli_camera_mode_property;
	//+ data
	flidev_t dev_id;
	char dev_file_name[PATH_MAX];
	char dev_name[PATH_MAX];
	flidomain_t domain;
	bool rbi_flood_supported;
	bool abort_requested;
	unsigned char *buffer;
	long buffer_size;
	long visible_ul_x, visible_ul_y;
	long frame_width, frame_height, bin_x, bin_y;
	int bits_per_pixel;
	double target_temperature, current_temperature, cooler_power;
	double previous_set_point;
	bool rbi_phase;
	//- data
} fli_private_data;

#pragma mark - Low level code

static indigo_queue *driver_queue = NULL;
static pthread_mutex_t driver_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

//+ code

// The SDK enumerates through a list that has to be created, walked and
// deleted; the results are kept here for the plug and unplug handlers,
// which the generated driver queue serialises.
static flidomain_t enumerated_domains[FLI_MAX_ENUMERATED];
static char enumerated_file_names[FLI_MAX_ENUMERATED][PATH_MAX];
static char enumerated_device_names[FLI_MAX_ENUMERATED][PATH_MAX];
static int enumerated_count;

static int fli_enumerate(void) {
	enumerated_count = 0;
	long result = FLICreateList(FLI_ENUM_DOMAIN);
	if (result) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLICreateList(%d) = %ld", FLI_ENUM_DOMAIN, result);
		return 0;
	}
	result = FLIListFirst(enumerated_domains, enumerated_file_names[0], PATH_MAX, enumerated_device_names[0], PATH_MAX);
	while (result == 0) {
		enumerated_count++;
		if (enumerated_count == FLI_MAX_ENUMERATED) {
			break;
		}
		result = FLIListNext(enumerated_domains + enumerated_count, enumerated_file_names[enumerated_count], PATH_MAX, enumerated_device_names[enumerated_count], PATH_MAX);
	}
	FLIDeleteList();
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d device(s) enumerated", enumerated_count);
	return enumerated_count;
}

static bool fli_is_enumerated(const char *file_name) {
	int count = fli_enumerate();
	for (int i = 0; i < count; i++) {
		if (!strncmp(enumerated_file_names[i], file_name, PATH_MAX)) {
			return true;
		}
	}
	return false;
}


static void exposure_finalizer(indigo_device *device);

// Reads the camera modes the SDK reports into the property. The count is
// restored on every connection, so a camera with more modes than the
// previous one publishes all of them.
static void fli_read_camera_modes(indigo_device *device) {
	flimode_t current_mode = 0;
	if (FLIGetCameraMode(PRIVATE_DATA->dev_id, &current_mode) != 0) {
		current_mode = 0;
	}
	int count = 0;
	for (int i = 0; i < FLI_MAX_MODES; i++) {
		char mode_name[INDIGO_NAME_SIZE];
		if (FLIGetCameraModeString(PRIVATE_DATA->dev_id, i, mode_name, INDIGO_NAME_SIZE) != 0) {
			break;
		}
		indigo_init_switch_item(FLI_CAMERA_MODE_PROPERTY->items + i, mode_name, mode_name, i == (int)current_mode);
		count++;
	}
	FLI_CAMERA_MODE_PROPERTY->hidden = count == 0;
	FLI_CAMERA_MODE_PROPERTY->count = count > 0 ? count : FLI_MAX_MODES;
}

static bool fli_open(indigo_device *device) {
	long result = FLIOpen(&PRIVATE_DATA->dev_id, PRIVATE_DATA->dev_file_name, PRIVATE_DATA->domain);
	if (result) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIOpen('%s') = %ld", PRIVATE_DATA->dev_file_name, result);
		return false;
	}
	long array_ul_x = 0, array_ul_y = 0, array_lr_x = 0, array_lr_y = 0;
	long visible_lr_x = 0, visible_lr_y = 0;
	if (FLIGetArrayArea(PRIVATE_DATA->dev_id, &array_ul_x, &array_ul_y, &array_lr_x, &array_lr_y) != 0 || FLIGetVisibleArea(PRIVATE_DATA->dev_id, &PRIVATE_DATA->visible_ul_x, &PRIVATE_DATA->visible_ul_y, &visible_lr_x, &visible_lr_y) != 0 || visible_lr_x <= PRIVATE_DATA->visible_ul_x || visible_lr_y <= PRIVATE_DATA->visible_ul_y) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Camera geometry could not be read");
		FLIClose(PRIVATE_DATA->dev_id);
		PRIVATE_DATA->dev_id = -1;
		return false;
	}
	// A camera that accepts the RBI flush frame type supports RBI flood.
	PRIVATE_DATA->rbi_flood_supported = FLISetFrameType(PRIVATE_DATA->dev_id, FLI_FRAME_TYPE_RBI_FLUSH) == 0;
	CCD_RBI_FLUSH_PROPERTY->hidden = CCD_RBI_FLUSH_ENABLE_PROPERTY->hidden = !PRIVATE_DATA->rbi_flood_supported;
	long width = array_lr_x - array_ul_x;
	long height = array_lr_y - array_ul_y;
	long buffer_size = width * height * 2 + FITS_HEADER_SIZE;
	if (PRIVATE_DATA->buffer == NULL || PRIVATE_DATA->buffer_size != buffer_size) {
		indigo_safe_free(PRIVATE_DATA->buffer);
		PRIVATE_DATA->buffer_size = buffer_size;
		PRIVATE_DATA->buffer = (unsigned char *)indigo_alloc_blob_buffer(buffer_size);
	}
	if (PRIVATE_DATA->buffer == NULL) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Image buffer could not be allocated");
		FLIClose(PRIVATE_DATA->dev_id);
		PRIVATE_DATA->dev_id = -1;
		return false;
	}
	CCD_INFO_WIDTH_ITEM->number.value = visible_lr_x - PRIVATE_DATA->visible_ul_x;
	CCD_INFO_HEIGHT_ITEM->number.value = visible_lr_y - PRIVATE_DATA->visible_ul_y;
	CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max = CCD_INFO_WIDTH_ITEM->number.value;
	CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max = CCD_INFO_HEIGHT_ITEM->number.value;
	double pixel_x = 0, pixel_y = 0;
	if (FLIGetPixelSize(PRIVATE_DATA->dev_id, &pixel_x, &pixel_y) == 0) {
		CCD_INFO_PIXEL_WIDTH_ITEM->number.value = m2um(pixel_x);
		CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = m2um(pixel_y);
		CCD_INFO_PIXEL_SIZE_ITEM->number.value = CCD_INFO_PIXEL_WIDTH_ITEM->number.value;
	}
	CCD_INFO_MAX_HORIZONAL_BIN_ITEM->number.value = FLI_MAX_X_BIN;
	CCD_INFO_MAX_VERTICAL_BIN_ITEM->number.value = FLI_MAX_Y_BIN;
	CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = FLI_BITS_PER_PIXEL;
	// FLISetBitDepth() has no effect on the supported cameras, so the
	// depth is fixed rather than offered as a choice.
	CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.min = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = FLI_BITS_PER_PIXEL;
	CCD_BIN_PROPERTY->perm = INDIGO_RW_PERM;
	CCD_BIN_HORIZONTAL_ITEM->number.value = CCD_BIN_HORIZONTAL_ITEM->number.min = 1;
	CCD_BIN_HORIZONTAL_ITEM->number.max = FLI_MAX_X_BIN;
	CCD_BIN_VERTICAL_ITEM->number.value = CCD_BIN_VERTICAL_ITEM->number.min = 1;
	CCD_BIN_VERTICAL_ITEM->number.max = FLI_MAX_Y_BIN;
	if (FLIGetModel(PRIVATE_DATA->dev_id, INFO_DEVICE_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE) != 0) {
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, PRIVATE_DATA->dev_name);
	}
	if (FLIGetSerialString(PRIVATE_DATA->dev_id, INFO_DEVICE_SERIAL_NUM_ITEM->text.value, INDIGO_VALUE_SIZE) != 0) {
		INFO_DEVICE_SERIAL_NUM_ITEM->text.value[0] = 0;
	}
	long firmware_revision = 0, hardware_revision = 0;
	if (FLIGetFWRevision(PRIVATE_DATA->dev_id, &firmware_revision) == 0) {
		snprintf(INFO_DEVICE_FW_REVISION_ITEM->text.value, INDIGO_VALUE_SIZE, "%ld", firmware_revision);
	} else {
		INFO_DEVICE_FW_REVISION_ITEM->text.value[0] = 0;
	}
	if (FLIGetHWRevision(PRIVATE_DATA->dev_id, &hardware_revision) == 0) {
		snprintf(INFO_DEVICE_HW_REVISION_ITEM->text.value, INDIGO_VALUE_SIZE, "%ld", hardware_revision);
	} else {
		INFO_DEVICE_HW_REVISION_ITEM->text.value[0] = 0;
	}
	indigo_update_property(device, INFO_PROPERTY, NULL);
	fli_read_camera_modes(device);
	if (FLISetNFlushes(PRIVATE_DATA->dev_id, (long)FLI_NFLUSHES_ITEM->number.target) != 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLISetNFlushes(%ld) failed", (long)PRIVATE_DATA->dev_id);
	}
	CCD_TEMPERATURE_ITEM->number.value = 0;
	if (FLIGetTemperature(PRIVATE_DATA->dev_id, &CCD_TEMPERATURE_ITEM->number.value) != 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIGetTemperature(%ld) failed", (long)PRIVATE_DATA->dev_id);
	}
	PRIVATE_DATA->current_temperature = PRIVATE_DATA->target_temperature = CCD_TEMPERATURE_ITEM->number.value;
	// Each camera keeps its own last set point, so two cameras cannot
	// skip each other's cooling request.
	PRIVATE_DATA->previous_set_point = FLI_COOLER_OFF_SET_POINT;
	PRIVATE_DATA->abort_requested = false;
	return true;
}

static void fli_close(indigo_device *device) {
	long result = FLIClose(PRIVATE_DATA->dev_id);
	if (result) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIClose(%ld) = %ld", (long)PRIVATE_DATA->dev_id, result);
	}
	PRIVATE_DATA->dev_id = -1;
	indigo_safe_free(PRIVATE_DATA->buffer);
	PRIVATE_DATA->buffer = NULL;
	PRIVATE_DATA->buffer_size = 0;
}

// Programs one frame and starts it. dark selects the closed shutter and
// rbi_flood the near infrared flood used to remove residual images.
static bool fli_start_exposure(indigo_device *device, double exposure, bool dark, bool rbi_flood) {
	long offset_x = (long)CCD_FRAME_LEFT_ITEM->number.value + PRIVATE_DATA->visible_ul_x;
	long offset_y = (long)CCD_FRAME_TOP_ITEM->number.value + PRIVATE_DATA->visible_ul_y;
	PRIVATE_DATA->frame_width = (long)CCD_FRAME_WIDTH_ITEM->number.value;
	PRIVATE_DATA->frame_height = (long)CCD_FRAME_HEIGHT_ITEM->number.value;
	PRIVATE_DATA->bin_x = (long)CCD_BIN_HORIZONTAL_ITEM->number.value;
	PRIVATE_DATA->bin_y = (long)CCD_BIN_VERTICAL_ITEM->number.value;
	PRIVATE_DATA->bits_per_pixel = (int)CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value;
	fliframe_t frame_type = FLI_FRAME_TYPE_NORMAL;
	if (dark) {
		frame_type = FLI_FRAME_TYPE_DARK;
	}
	if (rbi_flood) {
		frame_type = FLI_FRAME_TYPE_DARK | FLI_FRAME_TYPE_FLOOD;
	}
	if (FLISetHBin(PRIVATE_DATA->dev_id, PRIVATE_DATA->bin_x) != 0 || FLISetVBin(PRIVATE_DATA->dev_id, PRIVATE_DATA->bin_y) != 0 || FLISetImageArea(PRIVATE_DATA->dev_id, offset_x, offset_y, offset_x + PRIVATE_DATA->frame_width / PRIVATE_DATA->bin_x, offset_y + PRIVATE_DATA->frame_height / PRIVATE_DATA->bin_y) != 0 || FLISetExposureTime(PRIVATE_DATA->dev_id, (long)(exposure * 1000)) != 0 || FLISetFrameType(PRIVATE_DATA->dev_id, frame_type) != 0 || FLIExposeFrame(PRIVATE_DATA->dev_id) != 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Exposure could not be started");
		return false;
	}
	return true;
}

// Returns true when the frame has been read, false when it is not ready
// yet and sets failed when the camera reported an error.
static bool fli_read_frame(indigo_device *device, bool *failed) {
	*failed = false;
	long time_left = 0;
	if (FLIGetExposureStatus(PRIVATE_DATA->dev_id, &time_left) != 0) {
		*failed = true;
		return false;
	}
	if (time_left > 0) {
		return false;
	}
	long status = 0;
	if (FLIGetDeviceStatus(PRIVATE_DATA->dev_id, &status) != 0) {
		*failed = true;
		return false;
	}
	if (status != FLI_CAMERA_STATUS_UNKNOWN && (status & FLI_CAMERA_DATA_READY) == 0) {
		return false;
	}
	long width = PRIVATE_DATA->frame_width / PRIVATE_DATA->bin_x;
	long height = PRIVATE_DATA->frame_height / PRIVATE_DATA->bin_y;
	long row_size = width * PRIVATE_DATA->bits_per_pixel / 8;
	unsigned char *image = PRIVATE_DATA->buffer + FITS_HEADER_SIZE;
	bool success = true;
	for (long row = 0; row < height; row++) {
		if (FLIGrabRow(PRIVATE_DATA->dev_id, image + row * row_size, width) != 0) {
			// The remaining rows are still read so the sensor is flushed.
			if (success) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIGrabRow(%ld) failed at row %ld", (long)PRIVATE_DATA->dev_id, row);
			}
			success = false;
		}
	}
	*failed = !success;
	return true;
}

static void fli_exposure_failed(indigo_device *device, const char *message) {
	indigo_ccd_failure_cleanup(device);
	INDIGO_UPDATE_PROPERTY_STATE(CCD_EXPOSURE_PROPERTY, INDIGO_ALERT_STATE, "%s", message);
}

// Completes one acquisition. While the RBI flood is running the frame it
// produces and the following bias frames are read and discarded, and the
// real exposure is started only afterwards.
static void exposure_finalizer(indigo_device *device) {
	if (!IS_CONNECTED || CCD_EXPOSURE_PROPERTY->state != INDIGO_BUSY_STATE) {
		return;
	}
	if (PRIVATE_DATA->abort_requested) {
		return;
	}
	bool failed = false;
	if (!fli_read_frame(device, &failed)) {
		if (failed) {
			fli_exposure_failed(device, "Exposure failed");
		} else {
			indigo_execute_handler_in(device, FLI_EXPOSURE_POLL_DELAY, exposure_finalizer);
		}
		return;
	}
	if (failed) {
		fli_exposure_failed(device, "Exposure failed");
		return;
	}
	if (PRIVATE_DATA->rbi_phase) {
		// The flooded frame is discarded and the sensor is flushed with
		// bias frames before the real exposure starts.
		for (int i = 0; i < (int)CCD_RBI_FLUSH_COUNT_ITEM->number.value; i++) {
			if (PRIVATE_DATA->abort_requested || !fli_start_exposure(device, 0, true, false)) {
				break;
			}
			bool discard_failed = false;
			for (int cycle = 0; cycle < FLI_READOUT_TIMEOUT_CYCLES; cycle++) {
				if (fli_read_frame(device, &discard_failed)) {
					break;
				}
				if (discard_failed || PRIVATE_DATA->abort_requested) {
					break;
				}
				indigo_usleep(10000);
			}
		}
		PRIVATE_DATA->rbi_phase = false;
		if (PRIVATE_DATA->abort_requested) {
			return;
		}
		indigo_ccd_resume_countdown(device);
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Taking exposure...");
		if (!fli_start_exposure(device, CCD_EXPOSURE_ITEM->number.target, CCD_FRAME_TYPE_DARK_ITEM->sw.value || CCD_FRAME_TYPE_DARKFLAT_ITEM->sw.value || CCD_FRAME_TYPE_BIAS_ITEM->sw.value, false)) {
			fli_exposure_failed(device, "Exposure failed");
			return;
		}
		indigo_execute_handler_in(device, FLI_EXPOSURE_POLL_DELAY, exposure_finalizer);
		return;
	}
	CCD_EXPOSURE_ITEM->number.value = 0;
	indigo_process_image(device, PRIVATE_DATA->buffer, (int)(PRIVATE_DATA->frame_width / PRIVATE_DATA->bin_x), (int)(PRIVATE_DATA->frame_height / PRIVATE_DATA->bin_y), PRIVATE_DATA->bits_per_pixel, true, true, NULL, false);
	INDIGO_UPDATE_PROPERTY_STATE(CCD_EXPOSURE_PROPERTY, INDIGO_OK_STATE, NULL);
}

// One cooling cycle. The SDK has no cooler switch, so switching it off is
// expressed as a set point no camera reaches by cooling.
static void fli_update_cooling(indigo_device *device) {
	double target = CCD_COOLER_ON_ITEM->sw.value ? PRIVATE_DATA->target_temperature : FLI_COOLER_OFF_SET_POINT;
	bool ok = FLIGetTemperature(PRIVATE_DATA->dev_id, &PRIVATE_DATA->current_temperature) == 0;
	if (target != PRIVATE_DATA->previous_set_point) {
		if (FLISetTemperature(PRIVATE_DATA->dev_id, target) == 0) {
			PRIVATE_DATA->previous_set_point = target;
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLISetTemperature(%ld, %f) failed", (long)PRIVATE_DATA->dev_id, target);
			ok = false;
		}
	}
	if (FLIGetCoolerPower(PRIVATE_DATA->dev_id, &PRIVATE_DATA->cooler_power) != 0) {
		ok = false;
	}
	if (ok) {
		CCD_TEMPERATURE_ITEM->number.value = PRIVATE_DATA->current_temperature;
		CCD_COOLER_POWER_ITEM->number.value = PRIVATE_DATA->cooler_power;
		CCD_TEMPERATURE_PROPERTY->state = CCD_COOLER_ON_ITEM->sw.value && fabs(PRIVATE_DATA->current_temperature - PRIVATE_DATA->target_temperature) > FLI_TEMPERATURE_THRESHOLD ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
		CCD_COOLER_POWER_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		CCD_TEMPERATURE_PROPERTY->state = CCD_COOLER_POWER_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	if (CCD_COOLER_PROPERTY->state != INDIGO_OK_STATE) {
		INDIGO_UPDATE_PROPERTY_STATE(CCD_COOLER_PROPERTY, INDIGO_OK_STATE, NULL);
	}
	indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
	indigo_update_property(device, CCD_COOLER_POWER_PROPERTY, NULL);
}

//- code

#pragma mark - High level code (ccd)

static void ccd_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ ccd.on_timer
	fli_update_cooling(device);
	indigo_execute_handler_in(device, FLI_TEMPERATURE_DELAY, ccd_timer_callback);
	//- ccd.on_timer
}

static void ccd_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = fli_open(device);
		if (connection_result) {
			indigo_define_property(device, FLI_NFLUSHES_PROPERTY, NULL);
			indigo_define_property(device, FLI_CAMERA_MODE_PROPERTY, NULL);
			indigo_execute_handler(device, ccd_timer_callback);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s", device->name);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ ccd.on_disconnect
		PRIVATE_DATA->abort_requested = true;
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
			FLICancelExposure(PRIVATE_DATA->dev_id);
			indigo_ccd_failure_cleanup(device);
			INDIGO_UPDATE_PROPERTY_STATE(CCD_EXPOSURE_PROPERTY, INDIGO_ALERT_STATE, NULL);
		}
		PRIVATE_DATA->rbi_phase = false;
		//- ccd.on_disconnect
		indigo_delete_property(device, FLI_NFLUSHES_PROPERTY, NULL);
		indigo_delete_property(device, FLI_CAMERA_MODE_PROPERTY, NULL);
		fli_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_ccd_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void ccd_exposure_handler(indigo_device *device) {
	//+ ccd.CCD_EXPOSURE.on_change
	indigo_use_shortest_exposure_if_bias(device);
	PRIVATE_DATA->abort_requested = false;
	PRIVATE_DATA->rbi_phase = PRIVATE_DATA->rbi_flood_supported && CCD_RBI_FLUSH_ENABLED_ITEM->sw.value;
	CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
	indigo_ccd_exposure_setup(device);
	if (CCD_UPLOAD_MODE_LOCAL_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
		INDIGO_UPDATE_PROPERTY_STATE(CCD_IMAGE_FILE_PROPERTY, INDIGO_BUSY_STATE, NULL);
	}
	if (CCD_UPLOAD_MODE_CLIENT_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
		INDIGO_UPDATE_PROPERTY_STATE(CCD_IMAGE_PROPERTY, INDIGO_BUSY_STATE, NULL);
	}
	bool started;
	if (PRIVATE_DATA->rbi_phase) {
		indigo_ccd_suspend_countdown(device);
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Flushing CCD to remove RBI, this takes some time...");
		started = fli_start_exposure(device, CCD_RBI_FLUSH_EXPOSURE_ITEM->number.value, true, true);
	} else {
		started = fli_start_exposure(device, CCD_EXPOSURE_ITEM->number.target, CCD_FRAME_TYPE_DARK_ITEM->sw.value || CCD_FRAME_TYPE_DARKFLAT_ITEM->sw.value || CCD_FRAME_TYPE_BIAS_ITEM->sw.value, false);
	}
	if (started) {
		// exposure_finalizer owns completion
		indigo_execute_handler_in(device, FLI_EXPOSURE_POLL_DELAY, exposure_finalizer);
	} else {
		PRIVATE_DATA->rbi_phase = false;
		fli_exposure_failed(device, "Exposure failed");
	}
	//- ccd.CCD_EXPOSURE.on_change
}

static void ccd_abort_exposure_handler(indigo_device *device) {
	//+ ccd.CCD_ABORT_EXPOSURE.on_change
	if (CCD_ABORT_EXPOSURE_ITEM->sw.value && CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		// The urgent abort can overtake an exposure that is still queued.
		indigo_cancel_pending_handler(device, ccd_exposure_handler);
		indigo_cancel_pending_handler(device, exposure_finalizer);
		PRIVATE_DATA->abort_requested = true;
		PRIVATE_DATA->rbi_phase = false;
		if (FLICancelExposure(PRIVATE_DATA->dev_id) != 0) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLICancelExposure(%ld) failed", (long)PRIVATE_DATA->dev_id);
			CCD_ABORT_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_ccd_failure_cleanup(device);
		INDIGO_UPDATE_PROPERTY_STATE(CCD_EXPOSURE_PROPERTY, INDIGO_ALERT_STATE, NULL);
	}
	CCD_ABORT_EXPOSURE_ITEM->sw.value = false;
	indigo_update_property(device, CCD_ABORT_EXPOSURE_PROPERTY, NULL);
	//- ccd.CCD_ABORT_EXPOSURE.on_change
}

static void ccd_temperature_handler(indigo_device *device) {
	CCD_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_TEMPERATURE.on_change
	PRIVATE_DATA->target_temperature = CCD_TEMPERATURE_ITEM->number.target;
	CCD_TEMPERATURE_ITEM->number.value = PRIVATE_DATA->current_temperature;
	CCD_TEMPERATURE_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, CCD_COOLER_ON_ITEM->sw.value ? "Target temperature = %.2f" : "Target temperature = %.2f but the cooler is off", PRIVATE_DATA->target_temperature);
	//- ccd.CCD_TEMPERATURE.on_change
	indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
}

static void ccd_cooler_handler(indigo_device *device) {
	//+ ccd.CCD_COOLER.on_change
	CCD_COOLER_PROPERTY->state = INDIGO_BUSY_STATE;
	//- ccd.CCD_COOLER.on_change
	indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
}

static void ccd_frame_handler(indigo_device *device) {
	CCD_FRAME_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_FRAME.on_change
	// The sensor is read in eight pixel wide and two pixel high
	// blocks, and never in frames narrower than 64 binned pixels.
	if (CCD_FRAME_WIDTH_ITEM->number.value != CCD_FRAME_WIDTH_ITEM->number.max) {
		CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.target = 8 * (int)(CCD_FRAME_WIDTH_ITEM->number.value / 8);
	}
	if (CCD_FRAME_HEIGHT_ITEM->number.value != CCD_FRAME_HEIGHT_ITEM->number.max) {
		CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.target = 2 * (int)(CCD_FRAME_HEIGHT_ITEM->number.value / 2);
	}
	if (CCD_FRAME_WIDTH_ITEM->number.value / CCD_BIN_HORIZONTAL_ITEM->number.value < 64) {
		CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.target = 64 * CCD_BIN_HORIZONTAL_ITEM->number.value;
	}
	if (CCD_FRAME_HEIGHT_ITEM->number.value / CCD_BIN_VERTICAL_ITEM->number.value < 64) {
		CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.target = 64 * CCD_BIN_VERTICAL_ITEM->number.value;
	}
	CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.target = FLI_BITS_PER_PIXEL;
	//- ccd.CCD_FRAME.on_change
	indigo_update_property(device, CCD_FRAME_PROPERTY, NULL);
}

static void ccd_fli_nflushes_handler(indigo_device *device) {
	FLI_NFLUSHES_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.FLI_NFLUSHES.on_change
	long flushes = (long)FLI_NFLUSHES_ITEM->number.target;
	if (FLISetNFlushes(PRIVATE_DATA->dev_id, flushes) != 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLISetNFlushes(%ld, %ld) failed", (long)PRIVATE_DATA->dev_id, flushes);
		INDIGO_UPDATE_PROPERTY_STATE(FLI_NFLUSHES_PROPERTY, INDIGO_ALERT_STATE, "Can not set number of flushes to %ld", flushes);
		return;
	}
	//- ccd.FLI_NFLUSHES.on_change
	indigo_update_property(device, FLI_NFLUSHES_PROPERTY, NULL);
}

static void ccd_fli_camera_mode_handler(indigo_device *device) {
	FLI_CAMERA_MODE_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.FLI_CAMERA_MODE.on_change
	int mode = 0;
	for (int i = 0; i < FLI_CAMERA_MODE_PROPERTY->count; i++) {
		if (FLI_CAMERA_MODE_PROPERTY->items[i].sw.value) {
			mode = i;
			break;
		}
	}
	if (FLISetCameraMode(PRIVATE_DATA->dev_id, mode) != 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLISetCameraMode(%ld, %d) failed", (long)PRIVATE_DATA->dev_id, mode);
		INDIGO_UPDATE_PROPERTY_STATE(FLI_CAMERA_MODE_PROPERTY, INDIGO_ALERT_STATE, "Can not set camera mode %d", mode);
		return;
	}
	//- ccd.FLI_CAMERA_MODE.on_change
	indigo_update_property(device, FLI_CAMERA_MODE_PROPERTY, NULL);
}

#pragma mark - Device API (ccd)

static indigo_result ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result ccd_attach(indigo_device *device) {
	if (indigo_ccd_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		//+ ccd.on_attach
		INFO_PROPERTY->count = 8;
		CCD_COOLER_PROPERTY->hidden = false;
		CCD_TEMPERATURE_PROPERTY->hidden = false;
		CCD_TEMPERATURE_PROPERTY->perm = INDIGO_RW_PERM;
		CCD_TEMPERATURE_ITEM->number.min = FLI_MIN_TEMPERATURE;
		CCD_TEMPERATURE_ITEM->number.max = FLI_MAX_TEMPERATURE;
		CCD_TEMPERATURE_ITEM->number.step = 1;
		CCD_COOLER_POWER_PROPERTY->hidden = false;
		CCD_COOLER_POWER_PROPERTY->perm = INDIGO_RO_PERM;
		CCD_RBI_FLUSH_EXPOSURE_ITEM->number.min = 0;
		CCD_RBI_FLUSH_EXPOSURE_ITEM->number.max = 16;
		CCD_RBI_FLUSH_EXPOSURE_ITEM->number.value = CCD_RBI_FLUSH_EXPOSURE_ITEM->number.target = 3;
		CCD_RBI_FLUSH_COUNT_ITEM->number.min = 1;
		CCD_RBI_FLUSH_COUNT_ITEM->number.max = 10;
		CCD_RBI_FLUSH_COUNT_ITEM->number.value = CCD_RBI_FLUSH_COUNT_ITEM->number.target = 2;
		//- ccd.on_attach
		CCD_EXPOSURE_PROPERTY->hidden = false;
		CCD_ABORT_EXPOSURE_PROPERTY->hidden = false;
		CCD_TEMPERATURE_PROPERTY->hidden = false;
		CCD_COOLER_PROPERTY->hidden = false;
		CCD_FRAME_PROPERTY->hidden = false;
		FLI_NFLUSHES_PROPERTY = indigo_init_number_property(NULL, device->name, FLI_NFLUSHES_PROPERTY_NAME, CCD_ADVANCED_GROUP, "Flush CCD", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (FLI_NFLUSHES_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(FLI_NFLUSHES_ITEM, FLI_NFLUSHES_ITEM_NAME, "Times (before exposure)", 0, 16, 1, 1);
		FLI_CAMERA_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, FLI_CAMERA_MODE_PROPERTY_NAME, CCD_ADVANCED_GROUP, "Camera mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 8);
		if (FLI_CAMERA_MODE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(FLI_CAMERA_MODE_0_ITEM, FLI_CAMERA_MODE_0_ITEM_NAME, "Mode 0", true);
		indigo_init_switch_item(FLI_CAMERA_MODE_1_ITEM, FLI_CAMERA_MODE_1_ITEM_NAME, "Mode 1", false);
		indigo_init_switch_item(FLI_CAMERA_MODE_2_ITEM, FLI_CAMERA_MODE_2_ITEM_NAME, "Mode 2", false);
		indigo_init_switch_item(FLI_CAMERA_MODE_3_ITEM, FLI_CAMERA_MODE_3_ITEM_NAME, "Mode 3", false);
		indigo_init_switch_item(FLI_CAMERA_MODE_4_ITEM, FLI_CAMERA_MODE_4_ITEM_NAME, "Mode 4", false);
		indigo_init_switch_item(FLI_CAMERA_MODE_5_ITEM, FLI_CAMERA_MODE_5_ITEM_NAME, "Mode 5", false);
		indigo_init_switch_item(FLI_CAMERA_MODE_6_ITEM, FLI_CAMERA_MODE_6_ITEM_NAME, "Mode 6", false);
		indigo_init_switch_item(FLI_CAMERA_MODE_7_ITEM, FLI_CAMERA_MODE_7_ITEM_NAME, "Mode 7", false);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(FLI_NFLUSHES_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(FLI_CAMERA_MODE_PROPERTY);
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
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_EXPOSURE_PROPERTY, ccd_exposure_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(CCD_ABORT_EXPOSURE_PROPERTY, ccd_abort_exposure_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_TEMPERATURE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_TEMPERATURE_PROPERTY, ccd_temperature_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_COOLER_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_COOLER_PROPERTY, ccd_cooler_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_FRAME_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_FRAME_PROPERTY, ccd_frame_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FLI_NFLUSHES_PROPERTY, property)) {
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
			for (int i = 0; i < FLI_NFLUSHES_PROPERTY->count; i++) {
				FLI_NFLUSHES_PROPERTY->items[i].do_update = true;
			}
			FLI_NFLUSHES_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FLI_NFLUSHES_PROPERTY, "Exposure in progress, number of flushes can not be changed");
			return INDIGO_OK;
		}
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FLI_NFLUSHES_PROPERTY, ccd_fli_nflushes_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FLI_CAMERA_MODE_PROPERTY, property)) {
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
			for (int i = 0; i < FLI_CAMERA_MODE_PROPERTY->count; i++) {
				FLI_CAMERA_MODE_PROPERTY->items[i].do_update = true;
			}
			FLI_CAMERA_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FLI_CAMERA_MODE_PROPERTY, "Exposure in progress, camera mode can not be changed");
			return INDIGO_OK;
		}
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FLI_CAMERA_MODE_PROPERTY, ccd_fli_camera_mode_handler);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, FLI_NFLUSHES_PROPERTY);
			indigo_save_property(device, NULL, FLI_CAMERA_MODE_PROPERTY);
		}
	}
	return indigo_ccd_change_property(device, client, property);
}

static indigo_result ccd_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		ccd_connection_handler(device);
	}
	indigo_release_property(FLI_NFLUSHES_PROPERTY);
	indigo_release_property(FLI_CAMERA_MODE_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_ccd_detach(device);
}

#pragma mark - Device templates

static indigo_device ccd_template = INDIGO_DEVICE_INITIALIZER(CCD_DEVICE_NAME, ccd_attach, ccd_enumerate_properties, ccd_change_property, NULL, ccd_detach);

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
	fli_private_data *private_data = NULL;
	bool plug_result = true;
	char name[INDIGO_NAME_SIZE] = DRIVER_LABEL;
	private_data = (fli_private_data *)indigo_safe_malloc(sizeof(fli_private_data));
	private_data->usbdev = dev;
	struct libusb_device_descriptor descriptor;
	if (!(libusb_get_device_descriptor(dev, &descriptor) == LIBUSB_SUCCESS && descriptor.idVendor == FLI_VENDOR_ID)) {
		plug_result = false;
	}
	if (plug_result) {
		//+ sdk.plug
		plug_result = false;
		int count = fli_enumerate();
		for (int i = 0; i < count; i++) {
			bool attached = false;
			for (int slot = 0; slot < MAX_DEVICES; slot++) {
				if (devices[slot] != NULL && !strncmp(((fli_private_data *)devices[slot]->private_data)->dev_file_name, enumerated_file_names[i], PATH_MAX)) {
					attached = true;
					break;
				}
			}
			if (attached) {
				continue;
			}
			private_data->dev_id = -1;
			private_data->domain = enumerated_domains[i];
			snprintf(private_data->dev_file_name, PATH_MAX, "%s", enumerated_file_names[i]);
			snprintf(private_data->dev_name, PATH_MAX, "%s", enumerated_device_names[i]);
			snprintf(name, INDIGO_NAME_SIZE, "%s", enumerated_device_names[i]);
			indigo_make_name_unique(name, "%s", enumerated_file_names[i]);
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
	}
	if (!dev_ref_transferred) {
		indigo_safe_free(private_data);
		libusb_unref_device(dev);
	}
}

static void process_unplug_event_handler(indigo_device *device, void *data) {
	libusb_device *dev = (libusb_device *)data;
	fli_private_data *private_data = NULL;
	fli_private_data *removed[MAX_DEVICES];
	int removed_count = 0;
	for (int j = MAX_DEVICES - 1; j >= 0; j--) {
		if (devices[j] != NULL) {
			indigo_device *device = devices[j];
			private_data = PRIVATE_DATA;
			bool unplug_result = private_data->usbdev == dev;
			if (last_action != INDIGO_DRIVER_SHUTDOWN) {
				//+ sdk.unplug_match
				unplug_result = !fli_is_enumerated(private_data->dev_file_name);
				//- sdk.unplug_match
			}
			if (unplug_result) {
				private_data = PRIVATE_DATA;
				//+ sdk.unplug
				indigo_safe_free(private_data->buffer);
				private_data->buffer = NULL;
				//- sdk.unplug
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

indigo_result indigo_ccd_fli(indigo_driver_action action, indigo_driver_info *info) {

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
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
			int rc = libusb_hotplug_register_callback(NULL, (libusb_hotplug_event)(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT), LIBUSB_HOTPLUG_ENUMERATE, FLI_VENDOR_ID, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
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

		}
		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
