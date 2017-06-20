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

#define DRIVER_VERSION 0x0001
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
#define MAX_X_BIN            16     /* Max Horizontal binning */
#define MAX_Y_BIN            16     /* Max Vertical binning */

#define DEFAULT_BPP          16     /* Default bits per pixel */

#define MIN_N_FLUSHES         0     /* Min number of array flushes before exposure */
#define MAX_N_FLUSHES        16     /* Max number of array flushes before exposure */
#define DEFAULT_N_FLUSHES     1     /* Default number of array flushes before exposure */

#define MIN_NIR_FLOOD         0     /* Min seconds to flood the frame with NIR light */
#define MAX_NIR_FLOOD        16     /* Max seconds to flood the frame with NIR light */
#define DEFAULT_NIR_FLOOD     3     /* Default seconds to flood the frame with NIR light */

#define MIN_FLUSH_COUNT       1     /* Min flushes after flood */
#define MAX_FLUSH_COUNT      10     /* Max flushes after flood */
#define DEFAULT_FLUSH_COUNT   2     /* Default flushes after flood */

#define MAX_PATH            255     /* Maximal Path Length */

#define TEMP_THRESHOLD     0.15
#define TEMP_CHECK_TIME       3     /* Time between teperature checks (seconds) */

#include "libdsi.h"
#include "indigo_driver_xml.h"
#include "indigo_ccd_dsi.h"

#define FLI_VENDOR_ID              0x0f18

#define MAX_MODES                  32

#define PRIVATE_DATA               ((dsi_private_data *)device->private_data)

// gp_bits is used as boolean
#define is_connected                     gp_bits

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

	bool abort_flag;
	int count_open;
	indigo_timer *exposure_timer, *temperature_timer;
	long int buffer_size;
	image_area total_area;
	image_area visible_area;
	cframe_params frame_params;
	pthread_mutex_t usb_mutex;
	bool can_check_temperature;
} dsi_private_data;


static indigo_result dsi_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	return indigo_ccd_enumerate_properties(device, NULL, NULL);
}


static bool dsi_open(indigo_device *device) {
	int id;

	if (device->is_connected) return false;

	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);

	long res = 0; //FLIOpen(&(PRIVATE_DATA->dev_id), PRIVATE_DATA->dev_file_name, PRIVATE_DATA->domain);
	id = PRIVATE_DATA->dev_id;
	if (res) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIOpen(%d) = %d", id, res);
		return false;
	}

	//res = FLIGetArrayArea(id, &(PRIVATE_DATA->total_area.ul_x), &(PRIVATE_DATA->total_area.ul_y), &(PRIVATE_DATA->total_area.lr_x), &(PRIVATE_DATA->total_area.lr_y));
	if (res) {
		//FLIClose(id);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIGetArrayArea(%d) = %d", id, res);
		return false;
	}

	//res = FLIGetVisibleArea(id, &(PRIVATE_DATA->visible_area.ul_x), &(PRIVATE_DATA->visible_area.ul_y), &(PRIVATE_DATA->visible_area.lr_x), &(PRIVATE_DATA->visible_area.lr_y));
	if (res) {
		//FLIClose(id);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIGetVisibleArea(%d) = %d", id, res);
		return false;
	}

	//INDIGO_DRIVER_ERROR(DRIVER_NAME, "%ld %ld %ld %ld - %ld, %ld", PRIVATE_DATA->total_area.lr_x, PRIVATE_DATA->total_area.lr_y, PRIVATE_DATA->total_area.ul_x, PRIVATE_DATA->total_area.ul_y, height, width);

	if (PRIVATE_DATA->buffer == NULL) {
		PRIVATE_DATA->buffer_size = width * height * 2 + FITS_HEADER_SIZE;
		PRIVATE_DATA->buffer = (unsigned char*)indigo_alloc_blob_buffer(PRIVATE_DATA->buffer_size);
	}

	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	return true;
}


static bool dsi_start_exposure(indigo_device *device, double exposure, bool dark, bool rbi_flood, int offset_x, int offset_y, int frame_width, int frame_height, int bin_x, int bin_y) {
	int id = PRIVATE_DATA->dev_id;
	long res;

	/* Skip the optical black area */
	offset_x += PRIVATE_DATA->visible_area.ul_x;
	offset_y += PRIVATE_DATA->visible_area.ul_y;

	long right_x  = offset_x + (frame_width / bin_x);
	long right_y = offset_y + (frame_height / bin_y);

	/* needed to read frame data */
	PRIVATE_DATA->frame_params.width = frame_width;
	PRIVATE_DATA->frame_params.height = frame_height;
	PRIVATE_DATA->frame_params.bin_x = bin_x;
	PRIVATE_DATA->frame_params.bin_y = bin_y;
	PRIVATE_DATA->frame_params.bpp = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value;

	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);

	//res = FLISetHBin(id, bin_x);
	if (res) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLISetHBin(%d) = %d", id, res);
		return false;
	}

	//res = FLISetVBin(id, bin_y);
	if (res) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLISetVBin(%d) = %d", id, res);
		return false;
	}

	//res = FLISetImageArea(id, offset_x, offset_y, right_x, right_y);
	if (res) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLISetImageArea(%d) = %d", id, res);
		return false;
	}

	//res = FLISetExposureTime(id, (long)s2ms(exposure));
	if (res) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLISetExposureTime(%d) = %d", id, res);
		return false;
	}

	//res = FLIExposeFrame(id);
	if (res) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIExposeFrame(%d) = %d", id, res);
		return false;
	}

	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	return true;
}


static bool dsi_read_pixels(indigo_device *device) {
	long timeleft = 0;
	long res, dev_status;
	long wait_cycles = 4000;
	int id = PRIVATE_DATA->dev_id;

	do {
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	//	res = FLIGetExposureStatus(id, &timeleft);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if (timeleft) usleep(timeleft);
	} while (timeleft*1000);

	do {
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		//FLIGetDeviceStatus(id, &dev_status);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		//if((dev_status != FLI_CAMERA_STATUS_UNKNOWN) && ((dev_status & FLI_CAMERA_DATA_READY) != 0)) {
		//	break;
		//}
		usleep(10000);
		wait_cycles--;
	} while (wait_cycles);

	if (wait_cycles == 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Exposure Failed! id=%d", id);
		return false;
	}

	long row_size = PRIVATE_DATA->frame_params.width / PRIVATE_DATA->frame_params.bin_x * PRIVATE_DATA->frame_params.bpp / 8;
	long width = PRIVATE_DATA->frame_params.width / PRIVATE_DATA->frame_params.bin_x;
	long height = PRIVATE_DATA->frame_params.height / PRIVATE_DATA->frame_params.bin_y ;
	unsigned char *image = PRIVATE_DATA->buffer + FITS_HEADER_SIZE;

	bool success = true;
	for (int i = 0; i < height; i++) {
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		//res = FLIGrabRow(id, image + (i * row_size), width);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if (res) {
			/* print this error once but read to the end to flush the array */
			if (success) INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIGrabRow(%d) = %d at row %d.", id, res, i);
			success = false;
		}
	}

	return success;
}


static bool dsi_abort_exposure(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);

	//long err = FLICancelExposure(PRIVATE_DATA->dev_id);
	//FLICancelExposure(PRIVATE_DATA->dev_id);
	//FLICancelExposure(PRIVATE_DATA->dev_id);
	PRIVATE_DATA->can_check_temperature = true;
	PRIVATE_DATA->abort_flag = true;

	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	if(err) return false;
	else return true;
}


static void dsi_close(indigo_device *device) {
	if (!device->is_connected) return;

	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	//long res = FLIClose(PRIVATE_DATA->dev_id);
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	if (res) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIClose(%d) = %d", PRIVATE_DATA->dev_id, res);
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
	if (!CONNECTION_CONNECTED_ITEM->sw.value) return;
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		CCD_EXPOSURE_ITEM->number.value = 0;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		if (dsi_read_pixels(device)) {
			indigo_process_image(device, PRIVATE_DATA->buffer, (int)(CCD_FRAME_WIDTH_ITEM->number.value / CCD_BIN_HORIZONTAL_ITEM->number.value), (int)(CCD_FRAME_HEIGHT_ITEM->number.value / CCD_BIN_VERTICAL_ITEM->number.value), true, NULL);
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
		PRIVATE_DATA->exposure_timer = indigo_set_timer(device, 4, exposure_timer_callback);
	} else {
		PRIVATE_DATA->exposure_timer = NULL;
	}
}


static void ccd_temperature_callback(indigo_device *device) {
	if (!CONNECTION_CONNECTED_ITEM->sw.value) return;
	if (PRIVATE_DATA->can_check_temperature) {
		// check temperature;
		CCD_TEMPERATURE_ITEM->number.value = PRIVATE_DATA->current_temperature;
		indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
	}
	indigo_reschedule_timer(device, TEMP_CHECK_TIME, &PRIVATE_DATA->temperature_timer);
}


static indigo_result ccd_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_ccd_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		pthread_mutex_init(&PRIVATE_DATA->usb_mutex, NULL);

		/* Use all info property fields */
		INFO_PROPERTY->count = 7;

		return indigo_ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}


static bool handle_exposure_property(indigo_device *device, indigo_property *property) {
	long ok;
	PRIVATE_DATA->abort_flag = false;

	ok = dsi_start_exposure(device, CCD_EXPOSURE_ITEM->number.target, CCD_FRAME_TYPE_DARK_ITEM->sw.value || CCD_FRAME_TYPE_BIAS_ITEM->sw.value, 0,
	                                CCD_FRAME_LEFT_ITEM->number.value, CCD_FRAME_TOP_ITEM->number.value, CCD_FRAME_WIDTH_ITEM->number.value, CCD_FRAME_HEIGHT_ITEM->number.value,
	                                CCD_BIN_HORIZONTAL_ITEM->number.value, CCD_BIN_VERTICAL_ITEM->number.value);

	if (ok) {
		if (CCD_UPLOAD_MODE_LOCAL_ITEM->sw.value) {
			CCD_IMAGE_FILE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
		} else {
			CCD_IMAGE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
		}

		CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;

		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		if (CCD_EXPOSURE_ITEM->number.target > 4) {
			PRIVATE_DATA->exposure_timer = indigo_set_timer(device, CCD_EXPOSURE_ITEM->number.target - 4, clear_reg_timer_callback);
		} else {
			PRIVATE_DATA->can_check_temperature = false;
			PRIVATE_DATA->exposure_timer = indigo_set_timer(device, CCD_EXPOSURE_ITEM->number.target, exposure_timer_callback);
		}

	} else {
		CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Exposure failed.");
	}
	return false;
}

static indigo_result ccd_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);

	// -------------------------------------------------------------------------------- CONNECTION -> CCD_INFO, CCD_COOLER, CCD_TEMPERATURE
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			if (!device->is_connected) {
				if (dsi_open(device)) {
					//flidev_t id = PRIVATE_DATA->dev_id;
					long res;

					flimode_t current_mode;
					int i;
					char mode_name[INDIGO_NAME_SIZE];

					CCD_INFO_WIDTH_ITEM->number.value = 0; //TBD
					CCD_INFO_HEIGHT_ITEM->number.value = 0; //TBD
					CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max = CCD_INFO_WIDTH_ITEM->number.value;
					CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max = CCD_INFO_HEIGHT_ITEM->number.value;

					double size_x, size_y;
					pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
					//res = FLIGetPixelSize(id, &size_x, &size_y);
					if (res) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIGetPixelSize(%d) = %d", id, res);
					}

					//res = FLIGetModel(id, INFO_DEVICE_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE);
					if (res) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIGetModel(%d) = %d", id, res);
					}

					//res = FLIGetSerialString(id, INFO_DEVICE_SERIAL_NUM_ITEM->text.value, INDIGO_VALUE_SIZE);
					if (res) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIGetSerialString(%d) = %d", id, res);
					}

					long hw_rev, fw_rev;
					//res = FLIGetFWRevision(id, &fw_rev);
					if (res) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIGetFWRevision(%d) = %d", id, res);
					}

					//res = FLIGetHWRevision(id, &hw_rev);
					if (res) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIGetHWRevision(%d) = %d", id, res);
					}
					pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);

					sprintf(INFO_DEVICE_FW_REVISION_ITEM->text.value, "%ld", fw_rev);
					sprintf(INFO_DEVICE_HW_REVISION_ITEM->text.value, "%ld", hw_rev);

					indigo_update_property(device, INFO_PROPERTY, NULL);

					//INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIGetPixelSize(%d) = %f %f", id, size_x, size_y);
					CCD_INFO_PIXEL_WIDTH_ITEM->number.value = m2um(size_x);
					CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = m2um(size_y);
					CCD_INFO_PIXEL_SIZE_ITEM->number.value = CCD_INFO_PIXEL_WIDTH_ITEM->number.value;
					CCD_INFO_MAX_HORIZONAL_BIN_ITEM->number.value = 1;
					CCD_INFO_MAX_VERTICAL_BIN_ITEM->number.value = 1;

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
					//res = FLIGetTemperature(id,&(CCD_TEMPERATURE_ITEM->number.value));
					pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
					if (res) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIGetTemperature(%d) = %d", id, res);
					}
					PRIVATE_DATA->target_temperature = CCD_TEMPERATURE_ITEM->number.value;
					PRIVATE_DATA->can_check_temperature = true;

					PRIVATE_DATA->temperature_timer = indigo_set_timer(device, 0, ccd_temperature_callback);

					device->is_connected = true;
					CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
				} else {
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
					indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_CONNECTED_ITEM, false);
					indigo_update_property(device, CONNECTION_PROPERTY, NULL);
					return INDIGO_FAILED;
				}
			}
		} else {
			if (device->is_connected) {
				PRIVATE_DATA->can_check_temperature = false;
				indigo_cancel_timer(device, &PRIVATE_DATA->temperature_timer);
				dsi_close(device);
				device->is_connected = false;
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			}
		}
	// -------------------------------------------------------------------------------- CCD_EXPOSURE
	} else if (indigo_property_match(CCD_EXPOSURE_PROPERTY, property)) {
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE)
			return INDIGO_OK;
		indigo_property_copy_values(CCD_EXPOSURE_PROPERTY, property, false);
		if (IS_CONNECTED) {
			handle_exposure_property(device, property);
		}
	// -------------------------------------------------------------------------------- CCD_ABORT_EXPOSURE
	} else if (indigo_property_match(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
			dsi_abort_exposure(device);
		}
		PRIVATE_DATA->can_check_temperature = true;
		indigo_property_copy_values(CCD_ABORT_EXPOSURE_PROPERTY, property, false);
	// -------------------------------------------------------------------------------- CCD_TEMPERATURE
	} else if (indigo_property_match(CCD_TEMPERATURE_PROPERTY, property)) {
		indigo_property_copy_values(CCD_TEMPERATURE_PROPERTY, property, false);
		if (IS_CONNECTED) {
			PRIVATE_DATA->target_temperature = CCD_TEMPERATURE_ITEM->number.value;
			CCD_TEMPERATURE_ITEM->number.value = PRIVATE_DATA->current_temperature;
			CCD_TEMPERATURE_PROPERTY->state = INDIGO_BUSY_STATE;
			if (CCD_COOLER_ON_ITEM->sw.value)
				indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, "Target Temperature = %.2f", PRIVATE_DATA->target_temperature);
			else
				indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, "Target Temperature = %.2f but the cooler is OFF, ", PRIVATE_DATA->target_temperature);
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
		}
	}
	// -----------------------------------------------------------------------------
	return indigo_ccd_change_property(device, client, property);
}


static indigo_result ccd_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);

	INDIGO_DRIVER_LOG(DRIVER_NAME, "'%s' detached.", device->name);

	return indigo_ccd_detach(device);
}


// -------------------------------------------------------------------------------- hot-plug support

static pthread_mutex_t device_mutex = PTHREAD_MUTEX_INITIALIZER;

#define MAX_DEVICES                   32

static const flidomain_t enum_domain = FLIDOMAIN_USB | FLIDEVICE_CAMERA;
static int num_devices = 0;
static char fli_file_names[MAX_DEVICES][MAX_PATH] = {""};
static char fli_dev_names[MAX_DEVICES][MAX_PATH] = {""};
static flidomain_t fli_domains[MAX_DEVICES] = {0};

static indigo_device *devices[MAX_DEVICES] = {NULL};


static void enumerate_devices() {
	/* There is a mem leak heree!!! 8,192 constant + 20 bytes on every new connected device */
	num_devices = 0;
	long res = FLICreateList(enum_domain);
	if (res) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLICreateList(%d) = %d",enum_domain , res);
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
		"", false, NULL, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
		ccd_attach,
		fli_enumerate_properties,
		ccd_change_property,
		NULL,
		ccd_detach
	};

	struct libusb_device_descriptor descriptor;

	pthread_mutex_lock(&device_mutex);
	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			INDIGO_DEBUG_DRIVER(int rc =) libusb_get_device_descriptor(dev, &descriptor);
			if (descriptor.idVendor != DSI_VENDOR_ID) break;

			int slot = find_available_device_slot();
			if (slot < 0) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "No available device slots available.");
				pthread_mutex_unlock(&device_mutex);
				return 0;
			}

			char file_name[MAX_PATH];
			int idx = find_plugged_device(file_name);
			if (idx < 0) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "No FLI Camera plugged.");
				pthread_mutex_unlock(&device_mutex);
				return 0;
			}

			indigo_device *device = malloc(sizeof(indigo_device));
			assert(device != NULL);
			memcpy(device, &ccd_template, sizeof(indigo_device));
			sprintf(device->name, "%s #%d", fli_dev_names[idx], slot);
			INDIGO_DRIVER_LOG(DRIVER_NAME, "'%s' @ %s attached.", device->name , fli_file_names[idx]);
			fli_private_data *private_data = malloc(sizeof(fli_private_data));
			assert(private_data);
			memset(private_data, 0, sizeof(fli_private_data));
			private_data->dev_id = 0;
			private_data->domain = fli_domains[idx];
			strncpy(private_data->dev_file_name, fli_file_names[idx], MAX_PATH);
			strncpy(private_data->dev_name, fli_dev_names[idx], MAX_PATH);
			device->private_data = private_data;
			indigo_async((void *)(void *)indigo_attach_device, device);
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
				if (*device == NULL) {
					pthread_mutex_unlock(&device_mutex);
					return 0;
				}
				indigo_detach_device(*device);
				free((*device)->private_data);
				free(*device);
				libusb_unref_device(dev);
				*device = NULL;
				removed = true;
			}
			if (!removed) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "No DSI Camera unplugged!");
			}
		}
	}
	pthread_mutex_unlock(&device_mutex);
	return 0;
};


static void remove_all_devices() {
	int i;
	for(i = 0; i < MAX_DEVICES; i++) {
		indigo_device **device = &devices[i];
		if (*device == NULL)
			continue;
		indigo_detach_device(*device);
		free((*device)->private_data);
		free(*device);
		*device = NULL;
	}
}


static libusb_hotplug_callback_handle callback_handle;

indigo_result indigo_ccd_dsi(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "Meade DSI Camera", __FUNCTION__, DRIVER_VERSION, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;
		indigo_start_usb_event_handler();
		int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, FLI_VENDOR_ID, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_register_callback ->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
		return rc >= 0 ? INDIGO_OK : INDIGO_FAILED;

	case INDIGO_DRIVER_SHUTDOWN:
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
