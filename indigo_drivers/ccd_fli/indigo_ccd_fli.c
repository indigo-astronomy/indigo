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
// 2.0 Build 0 - PoC by Rumen Bogdanovski <rumen@skyarchive.org>


/** INDIGO CCD FLI driver
 \file indigo_ccd_fli.c
 */

#define DRIVER_VERSION 0x0001

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
#define MAX_X_BIN            16     /* Max Horizontal binning */
#define MAX_Y_BIN            16     /* Max Vertical binning */
#define DEFAULT_BPP          16     /* Default bits per pixel */
#define MIN_N_FLUSHES         0     /* Min number of array flushes before exposure */
#define MAX_N_FLUSHES        16     /* Max number of array flushes before exposure */
#define DEFAULT_N_FLUSHES     1     /* Default number of array flushes before exposure */

#define MAX_PATH            255     /* Maximal Path Length */

#include <libfli/libfli.h>
#include "indigo_driver_xml.h"
#include "indigo_ccd_fli.h"

#define FLI_VENDOR_ID              0x0f18

#undef PRIVATE_DATA
#define PRIVATE_DATA               ((fli_private_data *)DEVICE_CONTEXT->private_data)

#define FLI_NFLUSHES_PROPERTY        (PRIVATE_DATA->fli_nflushes_property)
#define FLI_NFLUSHES_PROPERTY_ITEM   (FLI_NFLUSHES_PROPERTY->items + 0)

#undef INDIGO_DEBUG_DRIVER
#define INDIGO_DEBUG_DRIVER(c) c

// -------------------------------------------------------------------------------- FLI USB interface implementation

#define ms2s(s)      ((s) / 1000.0)
#define s2ms(ms)     ((ms) * 1000)
#define m2um(m)      ((m) * 1e6)  /* meters to umeters */

typedef struct {
	long ul_x, ul_y, lr_x, lr_y;
} image_area;

typedef struct {
	long bin_x, bin_y;
	long width, height;
	int bpp;
} cframe_params;

typedef struct {
	flidev_t dev_id;
	char dev_file_name[MAX_PATH];
	char dev_name[MAX_PATH];
	flidomain_t domain;

	int count_open;
	indigo_timer *exposure_timer, *temperature_timer;
	double target_temperature, current_temperature;
	double cooler_power;
	unsigned char *buffer;
	long int buffer_size;
	image_area total_area;
	image_area visible_area;
	cframe_params frame_params;
	pthread_mutex_t usb_mutex;
	bool can_check_temperature;
	indigo_property *fli_nflushes_property;
} fli_private_data;


static indigo_result fli_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (indigo_property_match(FLI_NFLUSHES_PROPERTY, property))
			indigo_define_property(device, FLI_NFLUSHES_PROPERTY, NULL);
	}
	return indigo_ccd_enumerate_properties(device, NULL, NULL);
}


static bool fli_open(indigo_device *device) {
	flidev_t id;
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);

	long res = FLIOpen(&(PRIVATE_DATA->dev_id), PRIVATE_DATA->dev_file_name, PRIVATE_DATA->domain);
	id = PRIVATE_DATA->dev_id;
	if (res) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_LOG(indigo_log("indigo_ccd_fli: FLIOpen(%d) = %d", id, res));
		return false;
	}

	res = FLIGetArrayArea(PRIVATE_DATA->dev_id, &(PRIVATE_DATA->total_area.ul_x), &(PRIVATE_DATA->total_area.ul_y), &(PRIVATE_DATA->total_area.lr_x), &(PRIVATE_DATA->total_area.lr_y));
	if (res) {
		FLIClose(id);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_LOG(indigo_log("indigo_ccd_fli: FLIGetArrayArea(%d) = %d", id, res));
		return false;
	}

	res = FLIGetVisibleArea(PRIVATE_DATA->dev_id, &(PRIVATE_DATA->visible_area.ul_x), &(PRIVATE_DATA->visible_area.ul_y), &(PRIVATE_DATA->visible_area.lr_x), &(PRIVATE_DATA->visible_area.lr_y));
	if (res) {
		FLIClose(id);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_LOG(indigo_log("indigo_ccd_fli: FLIGetVisibleArea(%d) = %d", id, res));
		return false;
	}

	long height = PRIVATE_DATA->total_area.lr_y - PRIVATE_DATA->total_area.ul_y;
	long width = PRIVATE_DATA->total_area.lr_x - PRIVATE_DATA->total_area.ul_x;

	//INDIGO_LOG(indigo_log("indigo_ccd_fli: %ld %ld %ld %ld - %ld, %ld", PRIVATE_DATA->total_area.lr_x, PRIVATE_DATA->total_area.lr_y, PRIVATE_DATA->total_area.ul_x, PRIVATE_DATA->total_area.ul_y, height, width));

	if (PRIVATE_DATA->buffer == NULL) {
		PRIVATE_DATA->buffer_size = width * height * 2 + FITS_HEADER_SIZE;
		PRIVATE_DATA->buffer = (unsigned char*)malloc(PRIVATE_DATA->buffer_size);
	}

	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	return true;
}


static bool fli_start_exposure(indigo_device *device, double exposure, bool dark, int offset_x, int offset_y, int frame_width, int frame_height, int bin_x, int bin_y) {
	flidev_t id = PRIVATE_DATA->dev_id;
	long res;

	/* Skip the optical black area */
	offset_x += PRIVATE_DATA->visible_area.ul_x;
	offset_y += PRIVATE_DATA->visible_area.ul_y;

	long right_x  = offset_x + (frame_width / bin_x);
	long right_y = offset_y + (frame_height / bin_y);

	/* FLISetBitDepth() does not seem to work! */
	/*
	flibitdepth_t bit_depth = FLI_MODE_16BIT;
	if (CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value < 12) bit_depth = FLI_MODE_8BIT;
	*/

	/* needed to read frame data */
	PRIVATE_DATA->frame_params.width = frame_width;
	PRIVATE_DATA->frame_params.height = frame_height;
	PRIVATE_DATA->frame_params.bin_x = bin_x;
	PRIVATE_DATA->frame_params.bin_y = bin_y;
	PRIVATE_DATA->frame_params.bpp = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value;

	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);

	/* FLISetBitDepth() does not seem to work! */
	/*
	res = FLISetBitDepth(id, bit_depth);
	if (res) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_LOG(indigo_log("indigo_ccd_fli: FLISetBitDepth(%d) = %d", id, res));
		return false;
	}
	*/

	res = FLISetHBin(id, bin_x);
	if (res) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_LOG(indigo_log("indigo_ccd_fli: FLISetHBin(%d) = %d", id, res));
		return false;
	}

	res = FLISetVBin(id, bin_y);
	if (res) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_LOG(indigo_log("indigo_ccd_fli: FLISetVBin(%d) = %d", id, res));
		return false;
	}

	res = FLISetImageArea(id, offset_x, offset_y, right_x, right_y);
	if (res) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_LOG(indigo_log("indigo_ccd_fli: FLISetImageArea(%d) = %d", id, res));
		return false;
	}

	res = FLISetExposureTime(id, (long)s2ms(exposure));
	if (res) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_LOG(indigo_log("indigo_ccd_fli: FLISetExposureTime(%d) = %d", id, res));
		return false;
	}

	fliframe_t frame_type = FLI_FRAME_TYPE_NORMAL;
	if (dark) frame_type = FLI_FRAME_TYPE_DARK;
	res = FLISetFrameType(id, frame_type);
	if (res) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_LOG(indigo_log("indigo_ccd_fli: FLISetFrameType(%d) = %d", id, res));
		return false;
	}

	res = FLIExposeFrame(id);
	if (res) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_LOG(indigo_log("indigo_ccd_fli: FLIExposeFrame(%d) = %d", id, res));
		return false;
	}

	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	return true;
}


static bool fli_read_pixels(indigo_device *device) {
	long timeleft = 0;
	long res, dev_status;
	long wait_cicles = 4000;
	flidev_t id = PRIVATE_DATA->dev_id;

	do {
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		res = FLIGetExposureStatus(id, &timeleft);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if (timeleft) usleep(timeleft);
	} while (timeleft*1000);

	do {
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		FLIGetDeviceStatus(id, &dev_status);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if((dev_status != FLI_CAMERA_STATUS_UNKNOWN) && ((dev_status & FLI_CAMERA_DATA_READY) != 0)) {
			break;
		}
		usleep(10000);
		wait_cicles--;
	} while (wait_cicles);

	if (wait_cicles == 0) {
		INDIGO_LOG(indigo_log("indigo_ccd_fli: Exposure Failed! id=%d", id));
		return false;
	}

	long row_size = PRIVATE_DATA->frame_params.width / PRIVATE_DATA->frame_params.bin_x * PRIVATE_DATA->frame_params.bpp / 8;
	long width = PRIVATE_DATA->frame_params.width / PRIVATE_DATA->frame_params.bin_x;
	long height = PRIVATE_DATA->frame_params.height / PRIVATE_DATA->frame_params.bin_y ;
	unsigned char *image = PRIVATE_DATA->buffer + FITS_HEADER_SIZE;

	bool success = true;
	for (int i = 0; i < height; i++) {
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		res = FLIGrabRow(id, image + (i * row_size), width);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if (res) {
			/* print this error once but read to the end to flush the array */
			if (success) INDIGO_LOG(indigo_log("indigo_ccd_fli: FLIGrabRow(%d) = %d at row %d.", id, res, i));
			success = false;
		}
	}

	return success;
}


static bool fli_abort_exposure(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);

	long err = FLICancelExposure(PRIVATE_DATA->dev_id);
	FLICancelExposure(PRIVATE_DATA->dev_id);
	FLICancelExposure(PRIVATE_DATA->dev_id);

	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	if(err) return false;
	else return true;
}


static bool fli_set_cooler(indigo_device *device, double target, double *current, double *cooler_power) {
	long res;

	flidev_t id = PRIVATE_DATA->dev_id;
	long current_status;
	static double old_target = 100.0;

	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);

	res = FLIGetTemperature(id, current);
	if(res) INDIGO_LOG(indigo_log("indigo_ccd_fli: FLIGetTemperature(%d) = %d", id, res));

	if (target != old_target) {
		res = FLISetTemperature(id, target);
		if(res) INDIGO_LOG(indigo_log("indigo_ccd_fli: FLISetTemperature(%d) = %d", id, res));
	}

	res = FLIGetCoolerPower(id, (double *)cooler_power);
	if(res) INDIGO_LOG(indigo_log("indigo_ccd_fli: FLIGetCoolerPower(%d) = %d", id, res));

	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	return true;
}


static void fli_close(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	long res = FLIClose(PRIVATE_DATA->dev_id);
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	if (res) {
		INDIGO_LOG(indigo_log("indigo_ccd_fli: FLIClose(%d) = %d", PRIVATE_DATA->dev_id, res));
	}
	if (PRIVATE_DATA->buffer != NULL) {
		free(PRIVATE_DATA->buffer);
		PRIVATE_DATA->buffer = NULL;
	}
}

// -------------------------------------------------------------------------------- INDIGO CCD device implementation

// callback for image download
static void exposure_timer_callback(indigo_device *device) {
	PRIVATE_DATA->exposure_timer = NULL;
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		CCD_EXPOSURE_ITEM->number.value = 0;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		if (fli_read_pixels(device)) {
			CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
			indigo_process_image(device, PRIVATE_DATA->buffer, (int)(CCD_FRAME_WIDTH_ITEM->number.value / CCD_BIN_HORIZONTAL_ITEM->number.value), (int)(CCD_FRAME_HEIGHT_ITEM->number.value / CCD_BIN_VERTICAL_ITEM->number.value), true, NULL);
		} else {
			CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Exposure failed");
		}
	}
	PRIVATE_DATA->can_check_temperature = true;
}


// callback called 4s before image download (e.g. to clear vreg or turn off temperature check)
static void clear_reg_timer_callback(indigo_device *device) {
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		PRIVATE_DATA->can_check_temperature = false;
		PRIVATE_DATA->exposure_timer = indigo_set_timer(device, 4, exposure_timer_callback);
	} else {
		PRIVATE_DATA->exposure_timer = NULL;
	}
}


static void ccd_temperature_callback(indigo_device *device) {
	if (PRIVATE_DATA->can_check_temperature) {
		if (fli_set_cooler(device, PRIVATE_DATA->target_temperature, &PRIVATE_DATA->current_temperature, &PRIVATE_DATA->cooler_power)) {
			double diff = PRIVATE_DATA->current_temperature - PRIVATE_DATA->target_temperature;
			CCD_TEMPERATURE_PROPERTY->state = fabs(diff) > 0.2 ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
			CCD_TEMPERATURE_ITEM->number.value = PRIVATE_DATA->current_temperature;
			CCD_COOLER_POWER_PROPERTY->state = INDIGO_OK_STATE;
			CCD_COOLER_POWER_ITEM->number.value = PRIVATE_DATA->cooler_power;
		} else {
			CCD_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
			CCD_COOLER_POWER_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
		indigo_update_property(device, CCD_COOLER_POWER_PROPERTY, NULL);
	}
	indigo_reschedule_timer(device, 5, &PRIVATE_DATA->temperature_timer);
}


static indigo_result ccd_attach(indigo_device *device) {
	assert(device != NULL);
	assert(device->device_context != NULL);
	fli_private_data *private_data = device->device_context;
	device->device_context = NULL;
	if (indigo_ccd_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		DEVICE_CONTEXT->private_data = private_data;
		pthread_mutex_init(&PRIVATE_DATA->usb_mutex, NULL);

		/* Use all info property fields */
		INFO_PROPERTY->count = 7;

		// -------------------------------------------------------------------------------- FLI_NFLUSHES
		FLI_NFLUSHES_PROPERTY = indigo_init_number_property(NULL, device->name, "FLI_NFLUSHES", CCD_MAIN_GROUP, "Flush CCD", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 1);
		if (FLI_NFLUSHES_PROPERTY == NULL)
			return INDIGO_FAILED;

		indigo_init_number_item(FLI_NFLUSHES_PROPERTY_ITEM, "FLI_NFLUSHES", "Times (before exposure)", MIN_N_FLUSHES, MAX_N_FLUSHES, 1, DEFAULT_N_FLUSHES);
		// --------------------------------------------------------------------------------

		return indigo_ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}


static bool handle_nflushes_property(indigo_device *device, indigo_property *property) {
	int id = PRIVATE_DATA->dev_id;
	long nflushes = (long)(FLI_NFLUSHES_PROPERTY_ITEM->number.value);

	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	long res = FLISetNFlushes(id, nflushes);
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);

	if (res) {
		INDIGO_LOG(indigo_log("indigo_ccd_fli: FLISetNFlushes(%d) = %d", id, res));
		FLI_NFLUSHES_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, FLI_NFLUSHES_PROPERTY, "Can not set number of flushes to %ld", nflushes);
		return false;
	}

	FLI_NFLUSHES_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, FLI_NFLUSHES_PROPERTY, "Number of flushes set to %ld", nflushes);

	return true;
}


static indigo_result ccd_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(device->device_context != NULL);
	assert(property != NULL);

	// -------------------------------------------------------------------------------- CONNECTION -> CCD_INFO, CCD_COOLER, CCD_TEMPERATURE
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			if (fli_open(device)) {
				flidev_t id = PRIVATE_DATA->dev_id;

				CCD_MODE_PROPERTY->hidden = true;

				indigo_define_property(device, FLI_NFLUSHES_PROPERTY, NULL);

				CCD_INFO_WIDTH_ITEM->number.value = PRIVATE_DATA->visible_area.lr_x - PRIVATE_DATA->visible_area.ul_x;
				CCD_INFO_HEIGHT_ITEM->number.value = PRIVATE_DATA->visible_area.lr_y - PRIVATE_DATA->visible_area.ul_y;
				CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max = CCD_INFO_WIDTH_ITEM->number.value;
				CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max = CCD_INFO_HEIGHT_ITEM->number.value;

				double size_x, size_y;
				pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
				long res = FLIGetPixelSize(id, &size_x, &size_y);
				if (res) {
					INDIGO_LOG(indigo_log("indigo_ccd_fli: FLIGetPixelSize(%d) = %d", id, res));
				}

				res = FLIGetModel(id, INFO_DEVICE_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE);
				if (res) {
					INDIGO_LOG(indigo_log("indigo_ccd_fli: FLIGetModel(%d) = %d", id, res));
				}

				res = FLIGetSerialString(id, INFO_DEVICE_SERIAL_NUM_ITEM->text.value, INDIGO_VALUE_SIZE);
				if (res) {
					INDIGO_LOG(indigo_log("indigo_ccd_fli: FLIGetSerialString(%d) = %d", id, res));
				}

				long hw_rev, fw_rev;
				res = FLIGetFWRevision(id, &fw_rev);
				if (res) {
					INDIGO_LOG(indigo_log("indigo_ccd_fli: FLIGetFWRevision(%d) = %d", id, res));
				}

				res = FLIGetHWRevision(id, &hw_rev);
				if (res) {
					INDIGO_LOG(indigo_log("indigo_ccd_fli: FLIGetHWRevision(%d) = %d", id, res));
				}
				pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);

				sprintf(INFO_DEVICE_FW_REVISION_ITEM->text.value, "%ld", fw_rev);
				sprintf(INFO_DEVICE_HW_REVISION_ITEM->text.value, "%ld", hw_rev);

				indigo_update_property(device, INFO_PROPERTY, NULL);

				//INDIGO_LOG(indigo_log("indigo_ccd_fli: FLIGetPixelSize(%d) = %f %f", id, size_x, size_y));
				CCD_INFO_PIXEL_WIDTH_ITEM->number.value = m2um(size_x);
				CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = m2um(size_y);
				CCD_INFO_PIXEL_SIZE_ITEM->number.value = CCD_INFO_PIXEL_WIDTH_ITEM->number.value;
				CCD_INFO_MAX_HORIZONAL_BIN_ITEM->number.value = MAX_X_BIN;
				CCD_INFO_MAX_VERTICAL_BIN_ITEM->number.value = MAX_Y_BIN;

				CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = DEFAULT_BPP;
				/* FLISetBitDepth() does not seem to work so set max and min to DEFAULT and do not chanage it! */
				CCD_FRAME_BITS_PER_PIXEL_ITEM->number.min = DEFAULT_BPP;
				CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = DEFAULT_BPP;

				CCD_BIN_PROPERTY->perm = INDIGO_RW_PERM;
				CCD_BIN_HORIZONTAL_ITEM->number.value = CCD_BIN_HORIZONTAL_ITEM->number.min = 1;
				CCD_BIN_HORIZONTAL_ITEM->number.max = MAX_X_BIN;
				CCD_BIN_VERTICAL_ITEM->number.value = CCD_BIN_VERTICAL_ITEM->number.min = 1;
				CCD_BIN_VERTICAL_ITEM->number.max = MAX_Y_BIN;

				CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = DEFAULT_BPP;

				CCD_TEMPERATURE_PROPERTY->hidden = false;
				CCD_TEMPERATURE_PROPERTY->perm = INDIGO_RW_PERM;
				CCD_TEMPERATURE_ITEM->number.min = MIN_CCD_TEMP;
				CCD_TEMPERATURE_ITEM->number.max = MAX_CCD_TEMP;
				CCD_TEMPERATURE_ITEM->number.step = 0;
				pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
				res = FLIGetTemperature(id,&(CCD_TEMPERATURE_ITEM->number.value));
				pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
				if (res) {
					INDIGO_LOG(indigo_log("indigo_ccd_fli: FLIGetTemperature(%d) = %d", id, res));
				}
				PRIVATE_DATA->target_temperature = CCD_TEMPERATURE_ITEM->number.value;
				PRIVATE_DATA->can_check_temperature = true;

				CCD_COOLER_POWER_PROPERTY->hidden = false;
				CCD_COOLER_POWER_PROPERTY->perm = INDIGO_RO_PERM;

				PRIVATE_DATA->temperature_timer = indigo_set_timer(device, 0, ccd_temperature_callback);

				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		} else {
			PRIVATE_DATA->can_check_temperature = false;
			indigo_cancel_timer(device, &PRIVATE_DATA->temperature_timer);
			indigo_delete_property(device, FLI_NFLUSHES_PROPERTY, NULL);
			fli_close(device);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	// -------------------------------------------------------------------------------- CCD_EXPOSURE
	} else if (indigo_property_match(CCD_EXPOSURE_PROPERTY, property)) {
		indigo_property_copy_values(CCD_EXPOSURE_PROPERTY, property, false);

		if (fli_start_exposure(device, CCD_EXPOSURE_ITEM->number.target, CCD_FRAME_TYPE_DARK_ITEM->sw.value || CCD_FRAME_TYPE_BIAS_ITEM->sw.value,
		                           CCD_FRAME_LEFT_ITEM->number.value, CCD_FRAME_TOP_ITEM->number.value, CCD_FRAME_WIDTH_ITEM->number.value, CCD_FRAME_HEIGHT_ITEM->number.value,
		                           CCD_BIN_HORIZONTAL_ITEM->number.value, CCD_BIN_VERTICAL_ITEM->number.value)) {
			CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
			if (CCD_UPLOAD_MODE_LOCAL_ITEM->sw.value) {
				CCD_IMAGE_FILE_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
			} else {
				CCD_IMAGE_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
			}
			if (CCD_EXPOSURE_ITEM->number.target > 4)
				PRIVATE_DATA->exposure_timer = indigo_set_timer(device, CCD_EXPOSURE_ITEM->number.target - 4, clear_reg_timer_callback);
			else {
				PRIVATE_DATA->can_check_temperature = false;
				PRIVATE_DATA->exposure_timer = indigo_set_timer(device, CCD_EXPOSURE_ITEM->number.target, exposure_timer_callback);
			}
		} else {
			CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Exposure failed.");
		}
	// -------------------------------------------------------------------------------- CCD_ABORT_EXPOSURE
	} else if (indigo_property_match(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_cancel_timer(device, &PRIVATE_DATA->exposure_timer);
			fli_abort_exposure(device);
		}
		PRIVATE_DATA->can_check_temperature = true;
		indigo_property_copy_values(CCD_ABORT_EXPOSURE_PROPERTY, property, false);
	// -------------------------------------------------------------------------------- FLI_NFLUSHES
	} else if (indigo_property_match(FLI_NFLUSHES_PROPERTY, property)) {
		indigo_property_copy_values(FLI_NFLUSHES_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			handle_nflushes_property(device, property);
		}
	// -------------------------------------------------------------------------------- CCD_TEMPERATURE
	} else if (indigo_property_match(CCD_TEMPERATURE_PROPERTY, property)) {
		indigo_property_copy_values(CCD_TEMPERATURE_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			PRIVATE_DATA->target_temperature = CCD_TEMPERATURE_ITEM->number.value;
			CCD_TEMPERATURE_ITEM->number.value = PRIVATE_DATA->current_temperature;
			CCD_TEMPERATURE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, "Target Temperature = %.2f", PRIVATE_DATA->target_temperature);
		}
		return INDIGO_OK;
	// ------------------------------------------------------------------------------- CCD_FRAME
	} else if (indigo_property_match(CCD_FRAME_PROPERTY, property)) {
		indigo_property_copy_values(CCD_FRAME_PROPERTY, property, false);
		CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.target = 8 * (int)(CCD_FRAME_WIDTH_ITEM->number.value / 8);
		CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.target = 2 * (int)(CCD_FRAME_HEIGHT_ITEM->number.value / 2);
		if (CCD_FRAME_WIDTH_ITEM->number.value / CCD_BIN_HORIZONTAL_ITEM->number.value < 64)
			CCD_FRAME_WIDTH_ITEM->number.value = 64 * CCD_BIN_HORIZONTAL_ITEM->number.value;
		if (CCD_FRAME_HEIGHT_ITEM->number.value / CCD_BIN_VERTICAL_ITEM->number.value < 64)
			CCD_FRAME_HEIGHT_ITEM->number.value = 64 * CCD_BIN_VERTICAL_ITEM->number.value;
		/* FLISetBitDepth() does not seem to work so this should be always 16 bits */
		if (CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value < 12.0) {
			CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = 8.0;
		} else {
			CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = 16.0;
		}

		CCD_FRAME_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, CCD_FRAME_PROPERTY, NULL);
		return INDIGO_OK;
	// -------------------------------------------------------------------------------- CONFIG
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, FLI_NFLUSHES_PROPERTY);
		}
	}
	// -----------------------------------------------------------------------------
	return indigo_ccd_change_property(device, client, property);
}


static indigo_result ccd_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);

	INDIGO_LOG(indigo_log("indigo_ccd_fli: '%s' detached.", device->name));

	indigo_release_property(FLI_NFLUSHES_PROPERTY);

	return indigo_ccd_detach(device);
}


// -------------------------------------------------------------------------------- hot-plug support

#define MAX_DEVICES                   32

const flidomain_t enum_domain = FLIDOMAIN_USB | FLIDEVICE_CAMERA;
int num_devices = 0;
char fli_file_names[MAX_DEVICES][MAX_PATH] = {""};
char fli_dev_names[MAX_DEVICES][MAX_PATH] = {""};
flidomain_t fli_domains[MAX_DEVICES] = {0};

static indigo_device *devices[MAX_DEVICES] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};


static void enumerate_devices() {
	/* There is a mem leak heree!!! 8,192 constant + 20 bytes on every new connected device */
	num_devices = 0;
	long res = FLICreateList(enum_domain);
	if (res) {
		INDIGO_LOG(indigo_log("indigo_ccd_fli: FLICreateList(%d) = %d",enum_domain , res));
	}
	if(FLIListFirst(&fli_domains[num_devices], fli_file_names[num_devices], MAX_PATH, fli_dev_names[num_devices], MAX_PATH) == 0) {
		do {
			num_devices++;
		} while((FLIListNext(&fli_domains[num_devices], fli_file_names[num_devices], MAX_PATH, fli_dev_names[num_devices], MAX_PATH) == 0) && (num_devices < MAX_DEVICES));
	}
	FLIDeleteList();
	/* FOR DEBUG only!
	FLICreateList(FLIDOMAIN_USB | FLIDEVICE_FILTERWHEEL);
	if(FLIListFirst(&fli_domains[num_devices], fli_file_names[num_devices], MAX_PATH, fli_dev_names[num_devices], MAX_PATH) == 0) {
		do {
			num_devices++;
		} while((FLIListNext(&fli_domains[num_devices], fli_file_names[num_devices], MAX_PATH, fli_dev_names[num_devices], MAX_PATH) == 0) && (num_devices < MAX_DEVICES));
	}
	FLIDeleteList();
	*/
}


static int find_plugged_device(char *fname) {
	enumerate_devices();
	for (int dev_no = 0; dev_no < num_devices; dev_no++) {
		bool found = false;
		for(int slot = 0; slot < MAX_DEVICES; slot++) {
			indigo_device *device = devices[slot];
			if (device == NULL) continue;
			if (!strncmp(PRIVATE_DATA->dev_file_name, fli_file_names[dev_no], MAX_PATH)) {
				found = true;
				break;
			}
		}
		if (found) {
			continue;
		} else {
			assert(fname!=NULL);
			strncpy(fname, fli_file_names[dev_no], MAX_PATH);
			return dev_no;
		}
	}
	return -1;
}


static int find_index_by_device_fname(char *fname) {
	for (int dev_no = 0; dev_no < num_devices; dev_no++) {
		if (!strncmp(fli_file_names[dev_no], fname, MAX_PATH)) {
			return dev_no;
		}
	}
	return -1;
}


static int find_available_device_slot() {
	for(int slot = 0; slot < MAX_DEVICES; slot++) {
		if (devices[slot] == NULL) return slot;
	}
	return -1;
}


static int find_device_slot(char *fname) {
	for(int slot = 0; slot < MAX_DEVICES; slot++) {
		indigo_device *device = devices[slot];
		if (device == NULL) continue;
		if (!strncmp(PRIVATE_DATA->dev_file_name, fname, 255)) return slot;
	}
	return -1;
}


static int find_unplugged_device(char *fname) {
	enumerate_devices();
	for(int slot = 0; slot < MAX_DEVICES; slot++) {
		bool found = false;
		indigo_device *device = devices[slot];
		if (device == NULL) continue;
		for (int dev_no = 0; dev_no < num_devices; dev_no++) {
			if (!strncmp(PRIVATE_DATA->dev_file_name, fli_file_names[dev_no], MAX_PATH)) {
				found = true;
				break;
			}
		}
		if (found) {
			continue;
		} else {
			assert(fname!=NULL);
			strncpy(fname, PRIVATE_DATA->dev_file_name, MAX_PATH);
			return slot;
		}
	}
	return -1;
}


static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {

	static indigo_device ccd_template = {
		"", NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
		ccd_attach,
		indigo_ccd_enumerate_properties,
		ccd_change_property,
		ccd_detach
	};

	struct libusb_device_descriptor descriptor;

	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			INDIGO_DEBUG_DRIVER(int rc =) libusb_get_device_descriptor(dev, &descriptor);
			if (descriptor.idVendor != FLI_VENDOR_ID) break;

			int slot = find_available_device_slot();
			if (slot < 0) {
				INDIGO_LOG(indigo_log("indigo_camera_fli: No available device slots available."));
				return 0;
			}

			char file_name[MAX_PATH];
			int idx = find_plugged_device(file_name);
			if (idx < 0) {
				INDIGO_DEBUG(indigo_debug("indigo_ccd_fli: No FLI Camera plugged."));
				return 0;
			}

			indigo_device *device = malloc(sizeof(indigo_device));
			assert(device != NULL);
			memcpy(device, &ccd_template, sizeof(indigo_device));
			sprintf(device->name, "%s #%d", fli_dev_names[idx], slot);
			INDIGO_LOG(indigo_log("indigo_ccd_fli: '%s' @ %s attached.", device->name , fli_file_names[idx]));
			device->device_context = malloc(sizeof(fli_private_data));
			assert(device->device_context);
			memset(device->device_context, 0, sizeof(fli_private_data));
			((fli_private_data*)device->device_context)->dev_id = 0;
			((fli_private_data*)device->device_context)->domain = fli_domains[idx];
			strncpy(((fli_private_data*)device->device_context)->dev_file_name, fli_file_names[idx], MAX_PATH);
			strncpy(((fli_private_data*)device->device_context)->dev_name, fli_dev_names[idx], MAX_PATH);
			indigo_attach_device(device);
			devices[slot]=device;
			break;
		}
		case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: {
			int slot, id;
			char file_name[MAX_PATH];
			bool removed = false;
			while ((id = find_unplugged_device(file_name)) != -1) {
				slot = find_device_slot(file_name);
				if (slot < 0) continue;
				indigo_device **device = &devices[slot];
				if (*device == NULL)
					return 0;
				indigo_detach_device(*device);
				free((*device)->device_context);
				free(*device);
				libusb_unref_device(dev);
				*device = NULL;
				removed = true;
			}
			if (!removed) {
				INDIGO_DEBUG(indigo_debug("indigo_ccd_fli: No FLI Camera unplugged!"));
			}
		}
	}
	return 0;
};


static void remove_all_devices() {
	int i;
	for(i = 0; i < MAX_DEVICES; i++) {
		indigo_device **device = &devices[i];
		if (*device == NULL) continue;
		indigo_detach_device(*device);
		free((*device)->device_context);
		free(*device);
		*device = NULL;
	}
}


static libusb_hotplug_callback_handle callback_handle;

indigo_result indigo_ccd_fli(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "FLI Camera", __FUNCTION__, DRIVER_VERSION, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;
		indigo_start_usb_event_handler();
		int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, FLI_VENDOR_ID, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
		INDIGO_DEBUG_DRIVER(indigo_debug("indigo_ccd_fli: libusb_hotplug_register_callback [%d] ->  %s", __LINE__, rc < 0 ? libusb_error_name(rc) : "OK"));
		return rc >= 0 ? INDIGO_OK : INDIGO_FAILED;

	case INDIGO_DRIVER_SHUTDOWN:
		last_action = action;
		libusb_hotplug_deregister_callback(NULL, callback_handle);
		INDIGO_DEBUG_DRIVER(indigo_debug("indigo_ccd_fli: libusb_hotplug_deregister_callback [%d]", __LINE__));
		remove_all_devices();
		break;

	case INDIGO_DRIVER_INFO:
		break;
	}

	return INDIGO_OK;
}
