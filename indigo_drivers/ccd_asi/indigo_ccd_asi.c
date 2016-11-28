// Copyright (c) 2016 CloudMakers, s. r. o.
// Copyright (c) 2016 Rumen G. Bogdanovski
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
// 2.0 Build 0 - PoC by Peter Polakovic <peter.polakovic@cloudmakers.eu>
// 2.0 Build 0 - PoC by Rumen G. Bogdanovski


/** INDIGO ZWO ASI CCD driver
 \file indigo_ccd_asi.c
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

#include "asi_ccd/ASICamera2.h"
#include "indigo_ccd_asi.h"
#include "indigo_driver_xml.h"

#define ASI_VENDOR_ID              0x03c3

#define CCD_ADVANCED_GROUP                   "CCD advanced"

#undef PRIVATE_DATA
#define PRIVATE_DATA               ((asi_private_data *)DEVICE_CONTEXT->private_data)

#define PIXEL_FORMAT_PROPERTY      (PRIVATE_DATA->pixel_format_property)
#define PIXEL_RAW8_ITEM            (PIXEL_FORMAT_PROPERTY->items+0)
#define PIXEL_RGB24_ITEM           (PIXEL_FORMAT_PROPERTY->items+1)
#define PIXEL_RAW16_ITEM           (PIXEL_FORMAT_PROPERTY->items+2)
#define PIXEL_Y8_ITEM              (PIXEL_FORMAT_PROPERTY->items+3)

#define ASI_ADVANCED_PROPERTY      (PRIVATE_DATA->asi_advanced_property)

#undef INDIGO_DEBUG_DRIVER
#define INDIGO_DEBUG_DRIVER(c) c

// -------------------------------------------------------------------------------- ZWO ASI USB interface implementation

#define us2s(s) ((s) / 1000000.0)
#define s2us(us) ((us) * 1000000)


typedef struct {
	libusb_device *dev;
	libusb_device_handle *handle;
	int dev_id;
	int count_open;
	int count_connected;
	indigo_timer *exposure_timer, *temperture_timer, *guider_timer;
	double target_temperature, current_temperature;
	long cooler_power;
	unsigned short relay_mask;
	unsigned char *buffer;
	long int buffer_size;
	pthread_mutex_t usb_mutex;
	bool can_check_temperature;
	double exposure;
	ASI_CAMERA_INFO info;
	indigo_property *pixel_format_property;
	indigo_property *asi_advanced_property;
} asi_private_data;


static char *get_bayer_string(indigo_device *device) {
	if (!PRIVATE_DATA->info.IsColorCam) return NULL;

	switch (PRIVATE_DATA->info.BayerPattern) {
		case ASI_BAYER_BG:
			return "BGGR";
		case ASI_BAYER_GR:
			return "GRBG";
		case ASI_BAYER_GB:
			return "GBRG";
		case ASI_BAYER_RG:
		default:
			return "RGGB";
    }
}


static int get_pixel_depth(indigo_device *device) {
	if (PIXEL_RAW16_ITEM->sw.value) {
		return 16;
	} else if (PIXEL_RGB24_ITEM->sw.value) {
		return 24;
	} else {
		return 8;
	}
}


static int get_pixel_format(indigo_device *device) {
	if (PIXEL_RAW8_ITEM->sw.value) {
		return ASI_IMG_RAW8;
	} else if (PIXEL_RGB24_ITEM->sw.value) {
		return ASI_IMG_RGB24;
	} else if (PIXEL_RAW16_ITEM->sw.value) {
		return ASI_IMG_RAW16;
	} else if (PIXEL_Y8_ITEM->sw.value) {
		return ASI_IMG_Y8;
	} else {
		return ASI_IMG_RAW8;
	}
}


static indigo_result asi_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (indigo_property_match(PIXEL_FORMAT_PROPERTY, property))
			indigo_define_property(device, PIXEL_FORMAT_PROPERTY, NULL);
		if (indigo_property_match(ASI_ADVANCED_PROPERTY, property))
			indigo_define_property(device, ASI_ADVANCED_PROPERTY, NULL);
	}
	return indigo_ccd_enumerate_properties(device, NULL, NULL);
}


static bool asi_open(indigo_device *device) {
	int id = PRIVATE_DATA->dev_id;
	ASI_ERROR_CODE res;
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	if (PRIVATE_DATA->count_open++ == 0) {
		res = ASIOpenCamera(id);
		if (res) {
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			INDIGO_LOG(indigo_log("indigo_ccd_asi: ASIOpenCamera(%d) = %d", id, res));
			return false;
		}
		res = ASIInitCamera(id);
		if (res) {
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			INDIGO_LOG(indigo_log("indigo_ccd_asi: ASIInitCamera(%d) = %d", id, res));
			return false;
		}
		if (PRIVATE_DATA->buffer == NULL) {
			if(PRIVATE_DATA->info.IsColorCam)
				PRIVATE_DATA->buffer_size = PRIVATE_DATA->info.MaxHeight*PRIVATE_DATA->info.MaxWidth*3 + FITS_HEADER_SIZE;
			else
				PRIVATE_DATA->buffer_size = PRIVATE_DATA->info.MaxHeight*PRIVATE_DATA->info.MaxWidth*2 + FITS_HEADER_SIZE;

			PRIVATE_DATA->buffer = (unsigned char*)malloc(PRIVATE_DATA->buffer_size);
		}
	}
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	return true;
}

static bool asi_start_exposure(indigo_device *device, double exposure, bool dark, int frame_left, int frame_top, int frame_width, int frame_height, int horizontal_bin, int vertical_bin) {
	int id = PRIVATE_DATA->dev_id;
	ASI_ERROR_CODE res;
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);

	//start exposure - NEEDS MUCH MORE
	res = ASISetROIFormat(id, frame_width, frame_height,  horizontal_bin, get_pixel_format(device));
	if (res) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_LOG(indigo_log("indigo_ccd_asi: ASISetROIFormat(%d) = %d", id, res));
		return false;
	}
	res = ASISetStartPos(id, frame_left, frame_top);
	if (res) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_LOG(indigo_log("indigo_ccd_asi: ASISetStartPos(%d) = %d", id, res));
		return false;
	}

	res = ASISetControlValue(id, ASI_EXPOSURE, (long)s2us(exposure), ASI_FALSE);
	if (res) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_LOG(indigo_log("indigo_ccd_asi: ASISetControlValue(%d, ASI_EXPOSURE) = %d", id, res));
		return false;
	}
	res = ASIStartExposure(id, dark);
	if (res) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_LOG(indigo_log("indigo_ccd_asi: ASIStartExposure(%d) = %d", id, res));
		return false;
	}

	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	return true;
}

static bool asi_read_pixels(indigo_device *device) {
	ASI_ERROR_CODE res;
	ASI_EXPOSURE_STATUS status;
	int wait_cicles = 2000;    /* 2000*1000us = 2s */
	status = ASI_EXP_WORKING;
	while((status == ASI_EXP_WORKING) && wait_cicles--) {
			pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
			ASIGetExpStatus(PRIVATE_DATA->dev_id, &status);
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			usleep(1000);
			// INDIGO_LOG(indigo_log("indigo_ccd_asi: ASIGetExpStatus(%d), %d", PRIVATE_DATA->dev_id, wait_cicles));
	}
	if(status == ASI_EXP_SUCCESS) {
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		res = ASIGetDataAfterExp(PRIVATE_DATA->dev_id, PRIVATE_DATA->buffer + FITS_HEADER_SIZE, PRIVATE_DATA->buffer_size);
		if (res) {
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			INDIGO_LOG(indigo_log("indigo_ccd_asi: ASIGetDataAfterExp(%d) = %d", PRIVATE_DATA->dev_id, res));
			return false;
		}
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		return true;
	} else {
		INDIGO_LOG(indigo_log("indigo_ccd_asi: Exposure failed: dev_id = %d EC = %d", PRIVATE_DATA->dev_id, status));
		return false;
	}
}

static bool asi_abort_exposure(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);

	ASI_ERROR_CODE err = ASIStopExposure(PRIVATE_DATA->dev_id);

	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	if(err) return false;
	else return true;
}

static bool asi_set_cooler(indigo_device *device, bool status, double target, double *current, long *cooler_power) {
	ASI_ERROR_CODE res;
	ASI_BOOL unused;

	int id = PRIVATE_DATA->dev_id;
	long current_status;
	long temp_x10;

	if (!PRIVATE_DATA->info.IsCoolerCam) return true;

	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);

	res = ASIGetControlValue(id, ASI_COOLER_ON, &current_status, &unused);
	if(res) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_LOG(indigo_log("indigo_ccd_asi: ASIGetControlValue(%d, ASI_COOLER_ON) = %d", id, res));
		return false;
	}

	if (current_status != status) {
		res = ASISetControlValue(id, ASI_COOLER_ON, status, false);
		if(res) INDIGO_LOG(indigo_log("indigo_ccd_asi: ASISetControlValue(%d, ASI_COOLER_ON) = %d", id, res));
	} else if(status) {
		res = ASISetControlValue(id, ASI_TARGET_TEMP, (int)target, false);
		if(res) INDIGO_LOG(indigo_log("indigo_ccd_asi: ASISetControlValue(%d, ASI_TARGET_TEMP) = %d", id, res));
	}

	res = ASIGetControlValue(id, ASI_TEMPERATURE, &temp_x10, &unused);
	if(res) INDIGO_LOG(indigo_log("indigo_ccd_asi: ASIGetControlValue(%d, ASI_TEMPERATURE) = %d", id, res));
	*current = temp_x10/10.0; /* ASI_TEMPERATURE gives temp x 10 */

	res = ASIGetControlValue(id, ASI_COOLER_POWER_PERC, cooler_power, &unused);
	if(res) INDIGO_LOG(indigo_log("indigo_ccd_asi: ASIGetControlValue(%d, ASI_COOLER_POWER_PERC) = %d", id, res));

	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	return true;
}

//static bool asi_guide(indigo_device *device, ...) {
// //...
//}

static void asi_close(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);

	if (--PRIVATE_DATA->count_open == 0) {
		ASICloseCamera(PRIVATE_DATA->dev_id);
		if (PRIVATE_DATA->buffer != NULL) {
			free(PRIVATE_DATA->buffer);
			PRIVATE_DATA->buffer = NULL;
		}
	}

	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
}

// -------------------------------------------------------------------------------- INDIGO CCD device implementation

// callback for image download
static void exposure_timer_callback(indigo_device *device) {
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		CCD_EXPOSURE_ITEM->number.value = 0;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		if (asi_read_pixels(device)) {
			CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Exposure done");
			char *color_string = get_bayer_string(device);
			if(color_string) {
				indigo_fits_keyword keywords[] = {
					{ INDIGO_FITS_STRING, "BAYERPAT", .string = color_string, "Bayer color pattern" },
					{ INDIGO_FITS_NUMBER, "XBAYROFF", .number = 0, "X offset of Bayer array" },
					{ INDIGO_FITS_NUMBER, "YBAYROFF", .number = 0, "Y offset of Bayer array" },
					{ 0 }
				};
				indigo_process_image(device, PRIVATE_DATA->buffer, (int)(CCD_FRAME_WIDTH_ITEM->number.value / CCD_BIN_HORIZONTAL_ITEM->number.value), (int)(CCD_FRAME_HEIGHT_ITEM->number.value / CCD_BIN_VERTICAL_ITEM->number.value), PRIVATE_DATA->exposure, true, keywords);
			} else {
				indigo_process_image(device, PRIVATE_DATA->buffer, (int)(CCD_FRAME_WIDTH_ITEM->number.value / CCD_BIN_HORIZONTAL_ITEM->number.value), (int)(CCD_FRAME_HEIGHT_ITEM->number.value / CCD_BIN_VERTICAL_ITEM->number.value), PRIVATE_DATA->exposure, true, NULL);
			}
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
		// anything else to do...
		PRIVATE_DATA->exposure_timer = indigo_set_timer(device, 4, exposure_timer_callback);
	}
}

static void ccd_temperature_callback(indigo_device *device) {
	if (PRIVATE_DATA->can_check_temperature) {
		if (asi_set_cooler(device, CCD_COOLER_ON_ITEM->sw.value, PRIVATE_DATA->target_temperature, &PRIVATE_DATA->current_temperature, &PRIVATE_DATA->cooler_power)) {
			double diff = PRIVATE_DATA->current_temperature - PRIVATE_DATA->target_temperature;
			if (CCD_COOLER_ON_ITEM->sw.value)
				CCD_TEMPERATURE_PROPERTY->state = fabs(diff) > 0.5 ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
			else
				CCD_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
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
	PRIVATE_DATA->temperture_timer = indigo_set_timer(device, 5, ccd_temperature_callback);
}

static indigo_result ccd_attach(indigo_device *device) {
	assert(device != NULL);
	assert(device->device_context != NULL);
	asi_private_data *private_data = device->device_context;
	device->device_context = NULL;
	if (indigo_ccd_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		DEVICE_CONTEXT->private_data = private_data;
		DEVICE_CONTEXT->private_data = private_data;
		// -------------------------------------------------------------------------------- PIXEL_FORMAT
		PIXEL_FORMAT_PROPERTY = indigo_init_switch_property(NULL, device->name, "PIXEL_FORMAT", CCD_ADVANCED_GROUP, "Pixel Format", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 4);
		if (PIXEL_FORMAT_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(PIXEL_RAW8_ITEM, "RAW8", "RAW 8", true);
		indigo_init_switch_item(PIXEL_RGB24_ITEM, "RGB24", "RGB 24", false);
		indigo_init_switch_item(PIXEL_RAW16_ITEM, "RAW16", "RAW 16", false);
		indigo_init_switch_item(PIXEL_Y8_ITEM, "Y8", "Y 8", false);

		//if (PRIVATE_DATA->is_camera) {
			CCD_MODE_PROPERTY->perm = INDIGO_RW_PERM;
			CCD_MODE_PROPERTY->count = 0;
			CCD_INFO_WIDTH_ITEM->number.value = PRIVATE_DATA->info.MaxWidth;
			CCD_INFO_HEIGHT_ITEM->number.value = PRIVATE_DATA->info.MaxHeight;
			CCD_INFO_PIXEL_SIZE_ITEM->number.value = CCD_INFO_PIXEL_WIDTH_ITEM->number.value = CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = PRIVATE_DATA->info.PixelSize;
			CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max = PRIVATE_DATA->info.MaxWidth;
			CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max = PRIVATE_DATA->info.MaxHeight;
			CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = get_pixel_depth(device);

			CCD_BIN_HORIZONTAL_ITEM->number.value = CCD_BIN_HORIZONTAL_ITEM->number.min = 1;
			CCD_BIN_HORIZONTAL_ITEM->number.max = 16;

			CCD_BIN_VERTICAL_ITEM->number.value = CCD_BIN_VERTICAL_ITEM->number.min = 1;
			CCD_BIN_VERTICAL_ITEM->number.max = 16;

			CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = get_pixel_depth(device);

		//}

		// adjust info know before opening camera...
		// make hidden properties visible if needed
		// adjust read-write/read-only permission for properties if needed
		// adjust min/max values for properties if needed

		pthread_mutex_init(&PRIVATE_DATA->usb_mutex, NULL);
		return indigo_ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result handle_advanced_property(indigo_device *device, indigo_property *property) {
	int ctrl_count;
	ASI_CONTROL_CAPS ctrl_caps;
	int id = PRIVATE_DATA->dev_id;

	int res = ASIGetNumOfControls(id, &ctrl_count);
	if (res) {
		INDIGO_LOG(indigo_log("indigo_ccd_asi: ASIGetNumOfControls(%d) = %d", id, res));
		return INDIGO_NOT_FOUND;
	}

	for(int ctrl_no = 0; ctrl_no < ctrl_count; ctrl_no++) {
		ASIGetControlCaps(id, ctrl_no, &ctrl_caps);
		/* handle changes here */
	}
	return INDIGO_OK;
}


static indigo_result init_camera_properties(indigo_device *device, ASI_CONTROL_CAPS ctrl_caps) {
	int id = PRIVATE_DATA->dev_id;
	long value;
	ASI_BOOL unused;

	if (ctrl_caps.ControlType == ASI_EXPOSURE) {
		CCD_EXPOSURE_PROPERTY->hidden = false;
		if(ctrl_caps.IsWritable)
			CCD_EXPOSURE_PROPERTY->perm = INDIGO_RW_PERM;
		else
			CCD_EXPOSURE_PROPERTY->perm = INDIGO_RO_PERM;

		CCD_EXPOSURE_ITEM->number.min = us2s(ctrl_caps.MinValue);
		CCD_EXPOSURE_ITEM->number.max = us2s(ctrl_caps.MaxValue);
		ASIGetControlValue(id, ASI_EXPOSURE, &value, &unused);
		CCD_EXPOSURE_ITEM->number.value = us2s(value);
		return INDIGO_OK;
	}

	if (ctrl_caps.ControlType == ASI_GAIN) {
		CCD_GAIN_PROPERTY->hidden = false;
		if(ctrl_caps.IsWritable)
			CCD_GAIN_PROPERTY->perm = INDIGO_RW_PERM;
		else
			CCD_GAIN_PROPERTY->perm = INDIGO_RO_PERM;

		CCD_GAIN_ITEM->number.min = ctrl_caps.MinValue;
		CCD_GAIN_ITEM->number.max = ctrl_caps.MaxValue;
		ASIGetControlValue(id, ASI_GAIN, &value, &unused);
		CCD_GAIN_ITEM->number.value = value;
		CCD_GAIN_ITEM->number.step = 1;
		return INDIGO_OK;
	}

	if (ctrl_caps.ControlType == ASI_GAMMA) {
		CCD_GAMMA_PROPERTY->hidden = false;
		if(ctrl_caps.IsWritable)
			CCD_GAMMA_PROPERTY->perm = INDIGO_RW_PERM;
		else
			CCD_GAMMA_PROPERTY->perm = INDIGO_RO_PERM;

		CCD_GAMMA_ITEM->number.min = ctrl_caps.MinValue;
		CCD_GAMMA_ITEM->number.max = ctrl_caps.MaxValue;
		ASIGetControlValue(id, ASI_GAMMA, &value, &unused);
		CCD_GAMMA_ITEM->number.value = value;
		CCD_GAMMA_ITEM->number.step = 1;
		return INDIGO_OK;
	}

	if (ctrl_caps.ControlType == ASI_TARGET_TEMP) {
		CCD_TEMPERATURE_PROPERTY->hidden = false;
		if(ctrl_caps.IsWritable)
			CCD_TEMPERATURE_PROPERTY->perm = INDIGO_RW_PERM;
		else
			CCD_TEMPERATURE_PROPERTY->perm = INDIGO_RO_PERM;

		CCD_TEMPERATURE_ITEM->number.min = ctrl_caps.MinValue;
		CCD_TEMPERATURE_ITEM->number.max = ctrl_caps.MaxValue;
		CCD_TEMPERATURE_ITEM->number.value = ctrl_caps.DefaultValue;
		PRIVATE_DATA->target_temperature = ctrl_caps.DefaultValue;
		PRIVATE_DATA->can_check_temperature = true;
		return INDIGO_OK;
	}

	if (ctrl_caps.ControlType == ASI_COOLER_ON) {
		CCD_COOLER_PROPERTY->hidden = false;
		if(ctrl_caps.IsWritable)
			CCD_COOLER_PROPERTY->perm = INDIGO_RW_PERM;
		else
			CCD_COOLER_PROPERTY->perm = INDIGO_RO_PERM;

		return INDIGO_OK;
	}

	if (ctrl_caps.ControlType == ASI_COOLER_POWER_PERC) {
		CCD_COOLER_POWER_PROPERTY->hidden = false;
		if(ctrl_caps.IsWritable)
			CCD_COOLER_POWER_PROPERTY->perm = INDIGO_RW_PERM;
		else
			CCD_COOLER_POWER_PROPERTY->perm = INDIGO_RO_PERM;

		CCD_COOLER_POWER_ITEM->number.min = ctrl_caps.MinValue;
		CCD_COOLER_POWER_ITEM->number.max = ctrl_caps.MaxValue;
		ASIGetControlValue(id, ASI_COOLER_POWER_PERC, &value, &unused);
		CCD_COOLER_POWER_ITEM->number.value = value;
		return INDIGO_OK;
	}

	if(ASI_ADVANCED_PROPERTY == NULL) {
		int ctrl_count;
		ASI_CONTROL_CAPS ctrl_caps;
		int id = PRIVATE_DATA->dev_id;

		int res = ASIGetNumOfControls(id, &ctrl_count);
		if (res) {
			INDIGO_LOG(indigo_log("indigo_ccd_asi: ASIGetNumOfControls(%d) = %d", id, res));
			return INDIGO_NOT_FOUND;
		}

		ASI_ADVANCED_PROPERTY = indigo_init_number_property(NULL, device->name, "ASI_ADVANCED", "Advanced", "Advanced", INDIGO_IDLE_STATE, INDIGO_RW_PERM, ctrl_count - 6);
		if (ASI_ADVANCED_PROPERTY == NULL)
			return INDIGO_FAILED;

		int offset=0;
		for(int ctrl_no = 0; ctrl_no < ctrl_count; ctrl_no++) {
			ASIGetControlCaps(id, ctrl_no, &ctrl_caps);
			switch (ctrl_caps.ControlType) {
			case ASI_EXPOSURE:
			case ASI_GAIN:
			case ASI_GAMMA:
			case ASI_COOLER_POWER_PERC:
			case ASI_COOLER_ON:
			case ASI_TEMPERATURE:
				break;
			default:
				indigo_init_number_item(ASI_ADVANCED_PROPERTY->items+offset, ctrl_caps.Name, ctrl_caps.Description, 1,1,1,1);
				offset++;
				break;
			} 
			/* handle changes here */
		}
	}

	return INDIGO_OK;
}

static indigo_result ccd_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(device->device_context != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION -> CCD_INFO, CCD_COOLER, CCD_TEMPERATURE
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			if (asi_open(device)) {
				int id = PRIVATE_DATA->dev_id;
				int ctrl_count;
				ASI_CONTROL_CAPS ctrl_caps;
				// adjust info known after opening camera...
				// make hidden properties visible if needed
				// adjust read-write/read-only permission for properties if needed
				// adjust min/max values for properties if needed
				indigo_define_property(device, PIXEL_FORMAT_PROPERTY, NULL);

				int res = ASIGetNumOfControls(id, &ctrl_count);
				if (res) {
					INDIGO_LOG(indigo_log("indigo_ccd_asi: ASIGetNumOfControls(%d) = %d", id, res));
					return INDIGO_NOT_FOUND;
				}

				for(int ctrl_no = 0; ctrl_no < ctrl_count; ctrl_no++) {
					ASIGetControlCaps(id, ctrl_no, &ctrl_caps);
					init_camera_properties(device, ctrl_caps);
				}
				indigo_delete_property(device, ASI_ADVANCED_PROPERTY, NULL);

				if (PRIVATE_DATA->info.IsCoolerCam) {
					ccd_temperature_callback(device);
				}

				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		} else {
			if (PRIVATE_DATA->temperture_timer != NULL)
				indigo_cancel_timer(device, PRIVATE_DATA->temperture_timer);

			indigo_delete_property(device, PIXEL_FORMAT_PROPERTY, NULL);
			indigo_delete_property(device, ASI_ADVANCED_PROPERTY, NULL);
			asi_close(device);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	} else if (indigo_property_match(CCD_EXPOSURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_EXPOSURE
		indigo_property_copy_values(CCD_EXPOSURE_PROPERTY, property, false);
		PRIVATE_DATA->exposure = CCD_EXPOSURE_ITEM->number.value;
		asi_start_exposure(device, PRIVATE_DATA->exposure, CCD_FRAME_TYPE_DARK_ITEM->sw.value, CCD_FRAME_LEFT_ITEM->number.value, CCD_FRAME_TOP_ITEM->number.value, CCD_FRAME_WIDTH_ITEM->number.value, CCD_FRAME_HEIGHT_ITEM->number.value, CCD_BIN_HORIZONTAL_ITEM->number.value, CCD_BIN_VERTICAL_ITEM->number.value);
		CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Exposure initiated");
		if (CCD_UPLOAD_MODE_LOCAL_ITEM->sw.value) {
			CCD_IMAGE_FILE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
		} else {
			CCD_IMAGE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
		}
		if (PRIVATE_DATA->exposure > 4)
			PRIVATE_DATA->exposure_timer = indigo_set_timer(device, PRIVATE_DATA->exposure - 4, clear_reg_timer_callback);
		else {
			PRIVATE_DATA->can_check_temperature = false;
			PRIVATE_DATA->exposure_timer = indigo_set_timer(device, PRIVATE_DATA->exposure, exposure_timer_callback);
		}
	} else if (indigo_property_match(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_ABORT_EXPOSURE
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
			asi_abort_exposure(device);
			indigo_cancel_timer(device, PRIVATE_DATA->exposure_timer);
		}
		PRIVATE_DATA->can_check_temperature = true;
		indigo_property_copy_values(CCD_ABORT_EXPOSURE_PROPERTY, property, false);
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
	} else if (indigo_property_match(CCD_GAIN_PROPERTY, property)) {
		// ------------------------------------------------------------------------------- GAIN
		CCD_GAIN_PROPERTY->state == INDIGO_IDLE_STATE;
		indigo_property_copy_values(CCD_GAIN_PROPERTY, property, false);
		ASI_ERROR_CODE res = ASISetControlValue(PRIVATE_DATA->dev_id, ASI_GAIN, (long)(CCD_GAIN_ITEM->number.value), ASI_FALSE);
		if (res) INDIGO_LOG(indigo_log("indigo_ccd_asi: ASISetControlValue(%d, ASI_GAIN) = %d", PRIVATE_DATA->dev_id, res));
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- PIXEL_FORMAT
	} else if (indigo_property_match(PIXEL_FORMAT_PROPERTY, property)) {
		indigo_property_copy_values(PIXEL_FORMAT_PROPERTY, property, false);
		PIXEL_FORMAT_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, PIXEL_FORMAT_PROPERTY, NULL);

		CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = get_pixel_depth(device);
		CCD_INFO_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, CCD_INFO_PROPERTY, NULL);

		CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = get_pixel_depth(device);
		CCD_FRAME_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, CCD_FRAME_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(ASI_ADVANCED_PROPERTY, property)) {
		
	}
	return indigo_ccd_change_property(device, client, property);
}

static indigo_result ccd_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device);

	INDIGO_LOG(indigo_log("indigo_ccd_asi: '%s' detached.", device->name));

	free(PIXEL_FORMAT_PROPERTY);
	free(ASI_ADVANCED_PROPERTY);

	return indigo_ccd_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO guider device implementation

static indigo_result guider_attach(indigo_device *device) {
	assert(device != NULL);
	assert(device->device_context != NULL);
	asi_private_data *private_data = device->device_context;
	device->device_context = NULL;
	if (indigo_guider_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		DEVICE_CONTEXT->private_data = private_data;
		//INDIGO_LOG(indigo_log("%s attached", device->name));
		return indigo_guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(device->device_context != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			if (asi_open(device)) {
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		} else {
			asi_close(device);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	} else if (indigo_property_match(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_DEC
		indigo_property_copy_values(GUIDER_GUIDE_DEC_PROPERTY, property, false);
		if (PRIVATE_DATA->guider_timer != NULL)
			indigo_cancel_timer(device, PRIVATE_DATA->guider_timer);
		int duration = GUIDER_GUIDE_NORTH_ITEM->number.value;
		if (duration > 0) {
			// guide north
		} else {
			int duration = GUIDER_GUIDE_SOUTH_ITEM->number.value;
			if (duration > 0) {
				// guide north
			}
		}
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDER_GUIDE_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_RA
		indigo_property_copy_values(GUIDER_GUIDE_RA_PROPERTY, property, false);
		if (PRIVATE_DATA->guider_timer != NULL)
			indigo_cancel_timer(device, PRIVATE_DATA->guider_timer);
		int duration = GUIDER_GUIDE_EAST_ITEM->number.value;
		if (duration > 0) {
			// guide east
		} else {
			int duration = GUIDER_GUIDE_WEST_ITEM->number.value;
			if (duration > 0) {
				// guide west
			}
		}
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_guider_change_property(device, client, property);
}

static indigo_result guider_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device);
	INDIGO_LOG(indigo_log("indigo_ccd_asi: '%s' detached.", device->name));
	return indigo_guider_detach(device);
}

// -------------------------------------------------------------------------------- hot-plug support

#define MAX_DEVICES                   10
#define NO_DEVICE                 (-1000)


static int asi_products[100];
static int asi_id_count = 0;

static indigo_device *devices[MAX_DEVICES] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
static bool connected_ids[ASICAMERA_ID_MAX] = {false} ;


static int find_index_by_device_id(int id) {
	ASI_CAMERA_INFO info;
	int count = ASIGetNumOfConnectedCameras();
	for(int index = 0; index < count; index++) {
		ASIGetCameraProperty(&info, index);
		if (info.CameraID == id) return index;
	}
	return -1;
}


static int find_plugged_device_id() {
	int i, id = NO_DEVICE, new_id = NO_DEVICE;
	ASI_CAMERA_INFO info;

	int count = ASIGetNumOfConnectedCameras();
	for(i = 0; i < count; i++) {
		ASIGetCameraProperty(&info, i);
		id = info.CameraID;
		if(!connected_ids[id]) {
			new_id = id;
			connected_ids[id] = true;
			break;
		}
	}

	return new_id;
}


static int find_available_device_slot() {
	for(int slot = 0; slot < MAX_DEVICES; slot++) {
		if (devices[slot] == NULL) return slot;
	}
	return -1;
}


static int find_device_slot(int id) {
	for(int slot = 0; slot < MAX_DEVICES; slot++) {
		indigo_device *device = devices[slot];
		if (device == NULL) continue;
		if (PRIVATE_DATA->dev_id == id) return slot;
	}
	return -1;
}


static int find_unplugged_device_id() {
	bool dev_tmp[ASICAMERA_ID_MAX] = {false};
	int i;
	ASI_CAMERA_INFO info;

	int count = ASIGetNumOfConnectedCameras();
	for(i = 0; i < count; i++) {
		ASIGetCameraProperty(&info, i);
		dev_tmp[info.CameraID] = true;
	}

	int id = -1;
	for(i = 0; i < ASICAMERA_ID_MAX; i++) {
		if(connected_ids[i] && !dev_tmp[i]){
			id = i;
			connected_ids[id] = false;
			break;
		}
	}
	return id;
}


static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	ASI_CAMERA_INFO info;

	static indigo_device ccd_template = {
		"", NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
		ccd_attach,
		asi_enumerate_properties,
		ccd_change_property,
		ccd_detach
	};
	static indigo_device guider_template = {
		"", NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
		guider_attach,
		indigo_guider_enumerate_properties,
		guider_change_property,
		guider_detach
	};

	struct libusb_device_descriptor descriptor;

	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			int rc = libusb_get_device_descriptor(dev, &descriptor);
			for (int i = 0; i < asi_id_count; i++) {
				if (descriptor.idVendor != ASI_VENDOR_ID || asi_products[i] != descriptor.idProduct) continue;

				int slot = find_available_device_slot();
				if (slot < 0) {
					INDIGO_LOG(indigo_log("indigo_ccd_asi: No available device slots available."));
					return 0;
				}

				int id = find_plugged_device_id();
				if (id == NO_DEVICE) {
					INDIGO_LOG(indigo_log("indigo_ccd_asi: No plugged device found."));
					return 0;
				}

				indigo_device *device = malloc(sizeof(indigo_device));
				int index = find_index_by_device_id(id);
				if (index < 0) {
					INDIGO_LOG(indigo_log("indigo_ccd_asi: No index of plugged device found."));
					return 0;
				}
				ASIGetCameraProperty(&info, index);
				assert(device != NULL);
				memcpy(device, &ccd_template, sizeof(indigo_device));
				sprintf(device->name, "%s %d", info.Name, id);
				INDIGO_LOG(indigo_log("indigo_ccd_asi: '%s' attached.", device->name));
				void *private_data_ptr = malloc(sizeof(asi_private_data));
				device->device_context = private_data_ptr;
				assert(device->device_context);
				memset(device->device_context, 0, sizeof(asi_private_data));
				((asi_private_data*)device->device_context)->dev_id = id;
				memcpy(&(((asi_private_data*)device->device_context)->info), &info, sizeof(ASI_CAMERA_INFO));
				indigo_attach_device(device);
				devices[slot]=device;

				if (info.ST4Port) {
					slot = find_available_device_slot();
					if (slot < 0) {
						INDIGO_LOG(indigo_log("indigo_ccd_asi: No available device slots available."));
						return 0;
					}
					device = malloc(sizeof(indigo_device));
					assert(device != NULL);
					memcpy(device, &guider_template, sizeof(indigo_device));
					sprintf(device->name, "%s %d guider", info.Name, id);
					INDIGO_LOG(indigo_log("indigo_ccd_asi: '%s' attached.", device->name));
					device->device_context = private_data_ptr;
					assert(device->device_context);
					indigo_attach_device(device);
					devices[slot]=device;
				}
			}
			break;
		}
		case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: {
			int id, slot;
			bool removed = false;
			asi_private_data *private_data = NULL;
			while ((id = find_unplugged_device_id()) != -1) {
				slot = find_device_slot(id);
				while (slot >= 0) {
					indigo_device **device = &devices[slot];
					if (*device == NULL)
						return 0;

					indigo_detach_device(*device);
					if ((*device)->device_context) {
						private_data = (*device)->device_context;
					}
					free(*device);
					*device = NULL;
					removed = true;
					slot = find_device_slot(id);
				}

				if (private_data) {
					ASICloseCamera(id);
					free(private_data);
					private_data = NULL;
				}
			}
			if (!removed) {
				INDIGO_LOG(indigo_log("indigo_ccd_asi: No ASI Camera unplugged (maybe EFW wheel)!"));
			}
		}
	}
	return 0;
};


static void remove_all_devices() {
	int i;
	asi_private_data *pd;

	for(i = 0; i < MAX_DEVICES; i++) {
		indigo_device **device = &devices[i];
		if (*device == NULL) continue;
		indigo_detach_device(*device);
		pd = (asi_private_data*)(*device)->device_context;
		if(pd && connected_ids[pd->dev_id]) {  /* if camera has guider => prevent double free */
			connected_ids[pd->dev_id] = false;  /* if prevent double free */
			free(pd);
			(*device)->device_context = NULL;
		}
		free(*device);
		*device = NULL;
	}
	for(i = 0; i < ASICAMERA_ID_MAX; i++)
		connected_ids[i] = false;
}


static libusb_hotplug_callback_handle callback_handle;

indigo_result indigo_ccd_asi(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "ASI Camera", __FUNCTION__, DRIVER_VERSION, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;
		asi_id_count = ASIGetProductIDs(asi_products);
		if (asi_id_count <= 0) {
			INDIGO_LOG(indigo_log("indigo_ccd_asi: Can not get the list of supported IDs."));
			return INDIGO_FAILED;
		}
		indigo_start_usb_event_handler();
		int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, ASI_VENDOR_ID, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
		INDIGO_DEBUG_DRIVER(indigo_debug("indigo_ccd_asi: libusb_hotplug_register_callback [%d] ->  %s", __LINE__, rc < 0 ? libusb_error_name(rc) : "OK"));
		return rc >= 0 ? INDIGO_OK : INDIGO_FAILED;

	case INDIGO_DRIVER_SHUTDOWN:
		last_action = action;
		libusb_hotplug_deregister_callback(NULL, callback_handle);
		INDIGO_DEBUG_DRIVER(indigo_debug("indigo_ccd_asi: libusb_hotplug_deregister_callback [%d]", __LINE__));
		remove_all_devices();
		break;

	case INDIGO_DRIVER_INFO:
		break;
	}

	return INDIGO_OK;
}
