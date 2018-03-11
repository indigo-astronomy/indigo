 // Copyright (c) 2018 Rumen G. Bogdanovski
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

 /** INDIGO CCD Andor driver
  \file indigo_ccd_andor.c
  */

#define DRIVER_VERSION 0x0001
#define DRIVER_NAME	"indigo_ccd_andor"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

#include "indigo_driver_xml.h"
#include "indigo_ccd_andor.h"

#define WIDTH               1600
#define HEIGHT              1200
#define TEMP_UPDATE         5.0
#define STARS               100
#define SIN									0.8414709848078965
#define COS									0.5403023058681398

// gp_bits is used as boolean
#define is_connected                     gp_bits

#define PRIVATE_DATA						((andor_private_data *)device->private_data)
#define DSLR_PROGRAM_PROPERTY				PRIVATE_DATA->dslr_program_property

typedef struct {
	long handle;
	int index;
	indigo_property *dslr_program_property;

	int star_x[STARS], star_y[STARS], star_a[STARS];
	char image[FITS_HEADER_SIZE + 3 * WIDTH * HEIGHT + 2880];
	unsigned char *buffer;
	long buffer_size;
	pthread_mutex_t usb_mutex;
	bool no_check_temperature;
	double target_temperature, current_temperature;
	indigo_timer *exposure_timer, *temperature_timer;
	double ra_offset, dec_offset;
} andor_private_data;

static bool use_camera(indigo_device *device) {
	at_32 res = SetCurrentCamera(PRIVATE_DATA->handle);
	if (res != DRV_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetCurrentCamera(%d): Invalid camera handle.", PRIVATE_DATA->handle);
		return false;
	}
	return true;
}

static bool andor_start_exposure(indigo_device *device, double exposure, bool dark, int offset_x, int offset_y, int frame_width, int frame_height, int bin_x, int bin_y) {
	unsigned int res;

	if (!use_camera(device)) return false;

	//Set Read Mode to --Image--
	res = SetReadMode(4);
	if (res != DRV_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetReadMode(4) = %d", res);
		return false;
	}

	//Set Acquisition mode to --Single scan--
	SetAcquisitionMode(1);
	if (res != DRV_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetAcquisitionMode(1) = %d", res);
		return false;
	}

	//Set initial exposure time
	SetExposureTime(exposure);
	if (res != DRV_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetExposureTime(%f) = %d", exposure, res);
		return false;
	}

	//Initialize Shutter
	if(dark) {
		res = SetShutter(1,2,50,50);
	} else {
		res = SetShutter(1,0,50,50);
	}
	if (res != DRV_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetShutter() = %d", res);
		return false;
	}

	//Setup Image dimensions
	res = SetImage(bin_x, bin_y, offset_x+1, frame_width, offset_y+1, frame_height);
	if (res != DRV_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetImage(%d, %d, %d, %d, %d, %d) = %d", bin_x, bin_y, offset_x, frame_width, offset_y, frame_height, res);
		return false;
	}

	res = StartAcquisition();
	if (res != DRV_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "StartAcquisition() = %d", res);
		return false;
	}

	return true;
}

static bool andor_read_pixels(indigo_device *device) {
	long timeleft = 0;
	long res, dev_status;
	long wait_cycles = 4000;

	if (!use_camera(device)) return false;
	/*
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
		wait_cycles--;
	} while (wait_cycles);

	if (wait_cycles == 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Exposure Failed! id=%d", id);
		return false;
	}

	*/

	//Loop until acquisition finished
	int status;
	GetStatus(&status);
	while(status==DRV_ACQUIRING) GetStatus(&status);

	uint16_t *image = (uint16_t *)PRIVATE_DATA->buffer + FITS_HEADER_SIZE;
	long num_pixels = (int)(CCD_FRAME_WIDTH_ITEM->number.value) *
	                  (int)(CCD_FRAME_HEIGHT_ITEM->number.value);

	res = GetAcquiredData16(image, num_pixels);
	if (res != DRV_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "GetAcquiredData16() = %d", res);
		return false;
	}
	/*
	for (int i = 0; i < num_pixels; i++) {
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		res = FLIGrabRow(id, image + (i * row_size), width);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if (res) {
			if (success) INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIGrabRow(%d) = %d at row %d.", id, res, i);
			success = false;
		}
	}
	*/
	return true;
}

static void exposure_timer_callback(indigo_device *device) {
	unsigned char *frame_buffer;

	if (!CONNECTION_CONNECTED_ITEM->sw.value) return;

	PRIVATE_DATA->exposure_timer = NULL;
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		CCD_EXPOSURE_ITEM->number.value = 0;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		// read_pixels(device))
		if (andor_read_pixels(device)) {
			frame_buffer = PRIVATE_DATA->buffer;

			CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
			indigo_process_image(device, frame_buffer, (int)(CCD_FRAME_WIDTH_ITEM->number.value / CCD_BIN_HORIZONTAL_ITEM->number.value),
			                    (int)(CCD_FRAME_HEIGHT_ITEM->number.value / CCD_BIN_VERTICAL_ITEM->number.value), true, NULL);
		} else {
			CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Exposure failed");
		}
	}
	PRIVATE_DATA ->no_check_temperature = false;
}

// callback called 4s before image download (e.g. to clear vreg or turn off temperature check)
static void clear_reg_timer_callback(indigo_device *device) {
	if (!CONNECTION_CONNECTED_ITEM->sw.value) return;
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		PRIVATE_DATA->no_check_temperature = true;
		PRIVATE_DATA->exposure_timer = indigo_set_timer(device, 4, exposure_timer_callback);
	} else {
		PRIVATE_DATA->exposure_timer = NULL;
	}
}

static bool handle_exposure_property(indigo_device *device, indigo_property *property) {
	long ok;

	ok = andor_start_exposure(device,
	                         CCD_EXPOSURE_ITEM->number.target,
	                         CCD_FRAME_TYPE_DARK_ITEM->sw.value || CCD_FRAME_TYPE_BIAS_ITEM->sw.value,
	                         CCD_FRAME_LEFT_ITEM->number.value, CCD_FRAME_TOP_ITEM->number.value,
	                         CCD_FRAME_WIDTH_ITEM->number.value, CCD_FRAME_HEIGHT_ITEM->number.value,
	                         CCD_BIN_HORIZONTAL_ITEM->number.value, CCD_BIN_VERTICAL_ITEM->number.value
	);

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
			PRIVATE_DATA->no_check_temperature = true;
			PRIVATE_DATA->exposure_timer = indigo_set_timer(device, CCD_EXPOSURE_ITEM->number.target, exposure_timer_callback);
		}
	} else {
		CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Exposure failed.");
	}
	return false;
}

// -------------------------------------------------------------------------------- INDIGO CCD device implementation

static void ccd_temperature_callback(indigo_device *device) {
	double diff = PRIVATE_DATA->current_temperature - PRIVATE_DATA->target_temperature;
	if (diff > 0) {
		if (diff > 5) {
			if (CCD_COOLER_ON_ITEM->sw.value && CCD_COOLER_POWER_ITEM->number.value != 100) {
				CCD_COOLER_POWER_ITEM->number.value = 100;
				indigo_update_property(device, CCD_COOLER_POWER_PROPERTY, NULL);
			}
		} else {
			if (CCD_COOLER_ON_ITEM->sw.value && CCD_COOLER_POWER_ITEM->number.value != 50) {
				CCD_COOLER_POWER_ITEM->number.value = 50;
				indigo_update_property(device, CCD_COOLER_POWER_PROPERTY, NULL);
			}
		}
		CCD_TEMPERATURE_PROPERTY->state = CCD_COOLER_ON_ITEM->sw.value ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
		CCD_TEMPERATURE_ITEM->number.value = --(PRIVATE_DATA->current_temperature);
		indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
	} else if (diff < 0) {
		if (CCD_COOLER_POWER_ITEM->number.value > 0) {
			CCD_COOLER_POWER_ITEM->number.value = 0;
			indigo_update_property(device, CCD_COOLER_POWER_PROPERTY, NULL);
		}
		CCD_TEMPERATURE_PROPERTY->state = CCD_COOLER_ON_ITEM->sw.value ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
		CCD_TEMPERATURE_ITEM->number.value = ++(PRIVATE_DATA->current_temperature);
		indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
	} else {
		if (CCD_COOLER_ON_ITEM->sw.value) {
			if (CCD_COOLER_POWER_ITEM->number.value != 20) {
				CCD_COOLER_POWER_ITEM->number.value = 20;
				indigo_update_property(device, CCD_COOLER_POWER_PROPERTY, NULL);
			}
		} else {
			if (CCD_COOLER_POWER_ITEM->number.value != 0) {
				CCD_COOLER_POWER_ITEM->number.value = 0;
				indigo_update_property(device, CCD_COOLER_POWER_PROPERTY, NULL);
			}
		}
		CCD_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
	}
	indigo_reschedule_timer(device, TEMP_UPDATE, &PRIVATE_DATA->temperature_timer);
}

static indigo_result ccd_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_ccd_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- SIMULATION
		SIMULATION_PROPERTY->hidden = false;
		SIMULATION_PROPERTY->perm = INDIGO_RO_PERM;
		SIMULATION_ENABLED_ITEM->sw.value = true;
		SIMULATION_DISABLED_ITEM->sw.value = false;

		DSLR_PROGRAM_PROPERTY = indigo_init_switch_property(NULL, device->name, DSLR_PROGRAM_PROPERTY_NAME, "Advanced", "Program mode", INDIGO_IDLE_STATE, INDIGO_RO_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
		indigo_init_switch_item(DSLR_PROGRAM_PROPERTY->items + 0, "M", "Manual", true);

		CCD_MODE_PROPERTY->perm = INDIGO_RO_PERM;
		CCD_MODE_PROPERTY->count = 1;
		indigo_init_switch_item(CCD_MODE_PROPERTY->items + 0, "1600x1200", "1600x1200", true);


			// -------------------------------------------------------------------------------- CCD_INFO, CCD_BIN, CCD_MODE, CCD_FRAME
			CCD_INFO_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max = CCD_FRAME_WIDTH_ITEM->number.value = WIDTH;
			CCD_INFO_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max = CCD_FRAME_HEIGHT_ITEM->number.value = HEIGHT;
			CCD_BIN_PROPERTY->perm = INDIGO_RW_PERM;
			CCD_INFO_MAX_HORIZONAL_BIN_ITEM->number.value = CCD_BIN_HORIZONTAL_ITEM->number.max = 4;
			CCD_INFO_MAX_VERTICAL_BIN_ITEM->number.value = CCD_BIN_VERTICAL_ITEM->number.max = 4;
			CCD_MODE_PROPERTY->perm = INDIGO_RW_PERM;
			CCD_MODE_PROPERTY->count = 3;
			char name[32];
			sprintf(name, "RAW %dx%d", WIDTH, HEIGHT);
			indigo_init_switch_item(CCD_MODE_ITEM, "BIN_1x1", name, true);
			sprintf(name, "RAW %dx%d", WIDTH/2, HEIGHT/2);
			indigo_init_switch_item(CCD_MODE_ITEM+1, "BIN_2x2", name, false);
			sprintf(name, "RAW %dx%d", WIDTH/4, HEIGHT/4);
			indigo_init_switch_item(CCD_MODE_ITEM+2, "BIN_4x4", name, false);
			CCD_INFO_PIXEL_SIZE_ITEM->number.value = 5.2;
			CCD_INFO_PIXEL_WIDTH_ITEM->number.value = 5.2;
			CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = 5.2;
			CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = 16;
			// -------------------------------------------------------------------------------- CCD_GAIN, CCD_OFFSET, CCD_GAMMA
			CCD_GAIN_PROPERTY->hidden = CCD_OFFSET_PROPERTY->hidden = CCD_GAMMA_PROPERTY->hidden = false;
			// -------------------------------------------------------------------------------- CCD_IMAGE
			for (int i = 0; i < 10; i++) {
				PRIVATE_DATA->star_x[i] = rand() % WIDTH; // generate some star positions
				PRIVATE_DATA->star_y[i] = rand() % HEIGHT;
				PRIVATE_DATA->star_a[i] = 500 * (rand() % 100);       // and brightness
			}
			for (int i = 10; i < STARS; i++) {
				PRIVATE_DATA->star_x[i] = rand() % WIDTH; // generate some star positions
				PRIVATE_DATA->star_y[i] = rand() % HEIGHT;
				PRIVATE_DATA->star_a[i] = 50 * (rand() % 30);       // and brightness
			}
			// -------------------------------------------------------------------------------- CCD_COOLER, CCD_TEMPERATURE, CCD_COOLER_POWER
			CCD_COOLER_PROPERTY->hidden = false;
			CCD_TEMPERATURE_PROPERTY->hidden = false;
			CCD_COOLER_POWER_PROPERTY->hidden = false;
			indigo_set_switch(CCD_COOLER_PROPERTY, CCD_COOLER_OFF_ITEM, true);
			PRIVATE_DATA->target_temperature = PRIVATE_DATA->current_temperature = CCD_TEMPERATURE_ITEM->number.value = 25;
			CCD_TEMPERATURE_PROPERTY->perm = INDIGO_RO_PERM;
			CCD_COOLER_POWER_ITEM->number.value = 0;

		// --------------------------------------------------------------------------------
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

indigo_result ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	indigo_result result = INDIGO_OK;
	if ((result = indigo_ccd_enumerate_properties(device, client, property)) == INDIGO_OK) {
		if (IS_CONNECTED) {
			if (indigo_property_match(DSLR_PROGRAM_PROPERTY, property))
				indigo_define_property(device, DSLR_PROGRAM_PROPERTY, NULL);
		}
	}
	return result;
}

static indigo_result ccd_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			if (!device->is_connected) { /* Do not double open device */
				if (indigo_try_global_lock(device) != INDIGO_OK) {
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_CONNECTED_ITEM, false);
					indigo_update_property(device, CONNECTION_PROPERTY, "Device is locked");
					return INDIGO_OK;
				}

				indigo_define_property(device, DSLR_PROGRAM_PROPERTY, NULL);

				if(use_camera(device) == false) {
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_CONNECTED_ITEM, false);
					indigo_update_property(device, CONNECTION_PROPERTY, NULL);
					indigo_global_unlock(device);
					return INDIGO_OK;
				}

				int width, height;
				GetDetector(&width, &height);
				CCD_INFO_WIDTH_ITEM->number.value = width;
				CCD_INFO_HEIGHT_ITEM->number.value = height;
				CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max = CCD_INFO_WIDTH_ITEM->number.value;
				CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max = CCD_INFO_HEIGHT_ITEM->number.value;
				if (PRIVATE_DATA->buffer == NULL) {
					PRIVATE_DATA->buffer_size = width * height * 4 + FITS_HEADER_SIZE;
					PRIVATE_DATA->buffer = (unsigned char*)indigo_alloc_blob_buffer(PRIVATE_DATA->buffer_size);
				}

				float x_size, y_size;
				GetPixelSize(&x_size, &y_size);
				CCD_INFO_PIXEL_WIDTH_ITEM->number.value = x_size;
				CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = y_size;
				CCD_INFO_PIXEL_SIZE_ITEM->number.value = CCD_INFO_PIXEL_WIDTH_ITEM->number.value;

				int max_bin;
				// 4 is Image mode, 0 is horizontal binning
				GetMaximumBinning (4, 0, &max_bin);
				CCD_INFO_MAX_HORIZONAL_BIN_ITEM->number.value = max_bin;
				// 4 is Image mode, 1 is vertical binning
				GetMaximumBinning (4, 1, &max_bin);
				CCD_INFO_MAX_VERTICAL_BIN_ITEM->number.value = max_bin;

				PRIVATE_DATA->temperature_timer = indigo_set_timer(device, TEMP_UPDATE, ccd_temperature_callback);
				device->is_connected = true;
			}
		} else {
			if (device->is_connected) {  /* Do not double close device */
				indigo_global_unlock(device);
				indigo_delete_property(device, DSLR_PROGRAM_PROPERTY, NULL);
				indigo_cancel_timer(device, &PRIVATE_DATA->temperature_timer);
				if (PRIVATE_DATA->buffer != NULL) {
					free(PRIVATE_DATA->buffer);
					PRIVATE_DATA->buffer = NULL;
				}
				device->is_connected = false;
			}
		}
	} else if (indigo_property_match(CCD_EXPOSURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_EXPOSURE
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE)
			return INDIGO_OK;
		indigo_property_copy_values(CCD_EXPOSURE_PROPERTY, property, false);
		if (IS_CONNECTED) {
			handle_exposure_property(device, property);
		}
	} else if (indigo_property_match(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_ABORT_EXPOSURE
		indigo_property_copy_values(CCD_ABORT_EXPOSURE_PROPERTY, property, false);
		if (CCD_ABORT_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_cancel_timer(device, &PRIVATE_DATA->exposure_timer);
		}
	} else if (indigo_property_match(CCD_BIN_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_BIN
		indigo_property_copy_values(CCD_BIN_PROPERTY, property, false);
		indigo_update_property(device, CCD_BIN_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(CCD_COOLER_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_COOLER
		indigo_property_copy_values(CCD_COOLER_PROPERTY, property, false);
		if (CCD_COOLER_ON_ITEM->sw.value) {
			CCD_TEMPERATURE_PROPERTY->perm = INDIGO_RW_PERM;
			CCD_TEMPERATURE_PROPERTY->state = INDIGO_BUSY_STATE;
			PRIVATE_DATA->target_temperature = CCD_TEMPERATURE_ITEM->number.value;
		} else {
			CCD_TEMPERATURE_PROPERTY->perm = INDIGO_RO_PERM;
			CCD_TEMPERATURE_PROPERTY->state = INDIGO_IDLE_STATE;
			CCD_COOLER_POWER_ITEM->number.value = 0;
			PRIVATE_DATA->target_temperature = CCD_TEMPERATURE_ITEM->number.value = 25;
		}
		indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
		indigo_update_property(device, CCD_COOLER_POWER_PROPERTY, NULL);
		indigo_delete_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
		indigo_define_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(CCD_TEMPERATURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_TEMPERATURE
		indigo_property_copy_values(CCD_TEMPERATURE_PROPERTY, property, false);
		PRIVATE_DATA->target_temperature = CCD_TEMPERATURE_ITEM->number.value;
		CCD_TEMPERATURE_ITEM->number.value = PRIVATE_DATA->current_temperature;
		CCD_TEMPERATURE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, "Target temperature %g", PRIVATE_DATA->target_temperature);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_ccd_change_property(device, client, property);
}

static indigo_result ccd_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		indigo_device_disconnect(NULL, device->name);

		indigo_release_property(DSLR_PROGRAM_PROPERTY);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_ccd_detach(device);
}

// --------------------------------------------------------------------------------

#define MAX_DEVICES 8
static indigo_device *devices[MAX_DEVICES] = {NULL};
at_32 device_num = 0;

indigo_result indigo_ccd_andor(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device imager_camera_template = INDIGO_DEVICE_INITIALIZER(
		CCD_ANDOR_CAMERA_NAME,
		ccd_attach,
		ccd_enumerate_properties,
		ccd_change_property,
		NULL,
		ccd_detach
	);

	at_32 res;
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, CCD_ANDOR_CAMERA_NAME, __FUNCTION__, DRIVER_VERSION, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch(action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;

			const char default_path[] = "/usr/local/etc/andor";
			char *andor_path = getenv("ANDOR_SDK_PATH");
			if (andor_path == NULL) andor_path = (char *)default_path;
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ANDOR_SDK_PATH = %s", andor_path);

			res = Initialize(andor_path);
			if(res != DRV_SUCCESS) {
				switch (res) {
				case DRV_ERROR_NOCAMERA:
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "ANDOR SDK initialization error: No camera found.");
					break;
				case DRV_USBERROR:
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "ANDOR SDK initialization error: Unable to detect USB device or not USB2.0");
					break;
				case DRV_ERROR_PAGELOCK:
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "ANDOR SDK initialization error: Unable to acquire lock on requested memory.");
					break;
				case DRV_INIERROR:
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "ANDOR SDK initialization error: Unable to load DETECTOR.INI.");
					break;
				case DRV_VXDNOTINSTALLED:
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "ANDOR SDK initialization error: VxD not loaded.");
					break;
				case DRV_COFERROR:
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "ANDOR SDK initialization error: Unable to load *.COF");
					break;
				case DRV_FLEXERROR:
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "ANDOR SDK initialization error: Unable to load *.RBF");
					break;
				case DRV_ERROR_FILELOAD:
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "ANDOR SDK initialization error: Unable to load “*.COF” or “*.RBF” files.");
					break;
				default:
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "ANDOR SDK initialisation error: %d", res);
				}
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "No camera connected or ANDOR_SDK_PATH is not valid.");
				break;
			}

			res = GetAvailableCameras(&device_num);
			if (res!= DRV_SUCCESS) INDIGO_DRIVER_ERROR(DRIVER_NAME, "GetAvailableCameras() error: %d", res);
			else INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d cameras detected.", device_num);

			for (int i = 0; i < device_num; i++) {
				andor_private_data *private_data = malloc(sizeof(andor_private_data));
				pthread_mutex_init(&private_data->usb_mutex, NULL);
				assert(private_data != NULL);
				memset(private_data, 0, sizeof(andor_private_data));
				indigo_device *device = malloc(sizeof(indigo_device));
				assert(device != NULL);
				memcpy(device, &imager_camera_template, sizeof(indigo_device));

				at_32 handle;
				res = GetCameraHandle(i, &handle);
				if (res!= DRV_SUCCESS) INDIGO_DRIVER_ERROR(DRIVER_NAME, "GetCameraHandle() error: %d", res);

				res = SetCurrentCamera(handle);
				if (res!= DRV_SUCCESS) INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetCurrentCamera() error: %d", res);

				char head_name[255];
				res = GetHeadModel(head_name);
				if (res!= DRV_SUCCESS) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "GetHeadModel() error: %d", res);
					head_name[0] = '\0';
				}
				snprintf(device->name, sizeof(device->name), "Andor%s #%d", head_name, i);

				private_data->index = i;
				private_data->handle = handle;
				device->private_data = private_data;
				indigo_attach_device(device);
				devices[i] = device;
			}
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			last_action = action;
			ShutDown();
			for (int i = 0; i < device_num; i++) {
				if (devices[i] != NULL) {
					andor_private_data *private_data = devices[i]->private_data;
					indigo_detach_device(devices[i]);
					free(devices[i]);
					devices[i] = NULL;

					if (private_data != NULL) {
						pthread_mutex_destroy(&private_data->usb_mutex);
						free(private_data);
					}
				}
			}
			break;

		case INDIGO_DRIVER_INFO:
			break;
	}
	return INDIGO_OK;
}
