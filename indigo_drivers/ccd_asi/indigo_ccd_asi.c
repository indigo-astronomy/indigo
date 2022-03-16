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
// 2.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>
// 2.0 by Rumen G. Bogdanovski


/** INDIGO ZWO ASI CCD driver
 \file indigo_ccd_asi.c
 */

#define DRIVER_VERSION 0x001E
#define DRIVER_NAME "indigo_ccd_asi"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>

#include <indigo/indigo_driver_xml.h>

#include "indigo_ccd_asi.h"

#if !(defined(__APPLE__) && defined(__arm64__))

#if defined(INDIGO_MACOS)
#include <libusb-1.0/libusb.h>
#elif defined(INDIGO_FREEBSD)
#include <libusb.h>
#else
#include <libusb-1.0/libusb.h>
#endif

#include "ASICamera2.h"

#define ASI_DEFAULT_BANDWIDTH      45

#define ASI_MAX_FORMATS            4

#define ASI_VENDOR_ID              0x03c3

#define CCD_ADVANCED_GROUP         "Advanced"

#define PRIVATE_DATA               ((asi_private_data *)device->private_data)

#define PIXEL_FORMAT_PROPERTY      (PRIVATE_DATA->pixel_format_property)
#define RAW8_NAME                  "RAW 8"
#define RGB24_NAME                 "RGB 24"
#define RAW16_NAME                 "RAW 16"
#define Y8_NAME                    "Y 8"

#define ASI_PRESETS_PROPERTY      (PRIVATE_DATA->asi_presets_property)

#define ASI_HIGHEST_DR_ITEM       (ASI_PRESETS_PROPERTY->items+0)
#define ASI_HIGHEST_DR_NAME       "ASI_HIGHEST_DR"

#define ASI_UNITY_GAIN_ITEM       (ASI_PRESETS_PROPERTY->items+1)
#define ASI_UNITY_GAIN_NAME       "ASI_UNITY_GAIN"

#define ASI_LOWEST_RN_ITEM        (ASI_PRESETS_PROPERTY->items+2)
#define ASI_LOWEST_RN_NAME        "ASI_LOWEST_RN"

#define ASI_ADVANCED_PROPERTY      (PRIVATE_DATA->asi_advanced_property)

// gp_bits is used as boolean
#define is_connected               gp_bits

// -------------------------------------------------------------------------------- ZWO ASI USB interface implementation

#define us2s(s) ((s) / 1000000.0)
#define s2us(us) ((us) * 1000000)


typedef struct {
	int dev_id;
	int count_open;
	int exp_bin_x, exp_bin_y;
	int exp_frame_width, exp_frame_height;
	int exp_bpp;
	indigo_timer *exposure_timer, *temperature_timer, *guider_timer_ra, *guider_timer_dec;
	double target_temperature, current_temperature;
	long cooler_power;
	bool guide_relays[4];
	unsigned char *buffer;
	long int buffer_size;
	pthread_mutex_t usb_mutex;
	long is_asi120;
	bool can_check_temperature, has_temperature_sensor;
	ASI_CAMERA_INFO info;
	int gain_highest_dr;
	int offset_highest_dr;
	int gain_unity_gain;
	int offset_unity_gain;
	int gain_lowerst_rn;
	int offset_lowest_rn;
	indigo_property *pixel_format_property;
	indigo_property *asi_presets_property;
	indigo_property *asi_advanced_property;
} asi_private_data;


static int get_unity_gain(indigo_device *device) {
	if (PRIVATE_DATA->is_asi120) {
		/* ASI 120 is a special case so we tahe the published unity gain */
		return 29;
	}

	int unity_gain = 0;

	double e_per_adu = PRIVATE_DATA->info.ElecPerADU * pow(10.0, CCD_GAIN_ITEM->number.value/200.0);
	if (e_per_adu > 0) {
		unity_gain = (int)round(200 * log10(e_per_adu));
	}
	if (unity_gain < 0) {
		unity_gain = 0;
	}
	return unity_gain;
}

static void adjust_preset_switches(indigo_device *device) {
	ASI_HIGHEST_DR_ITEM->sw.value = false;
	ASI_UNITY_GAIN_ITEM->sw.value = false;
	ASI_LOWEST_RN_ITEM->sw.value = false;

	if (((int)CCD_GAIN_ITEM->number.value == PRIVATE_DATA->gain_highest_dr) &&
	    ((int)CCD_OFFSET_ITEM->number.value == PRIVATE_DATA->offset_highest_dr)) {
		ASI_HIGHEST_DR_ITEM->sw.value = true;
	} else if (((int)CCD_GAIN_ITEM->number.value == PRIVATE_DATA->gain_unity_gain) &&
	    ((int)CCD_OFFSET_ITEM->number.value == PRIVATE_DATA->offset_unity_gain)) {
		ASI_UNITY_GAIN_ITEM->sw.value = true;
	} else if (((int)CCD_GAIN_ITEM->number.value == PRIVATE_DATA->gain_lowerst_rn) &&
	    ((int)CCD_OFFSET_ITEM->number.value == PRIVATE_DATA->offset_lowest_rn)) {
		ASI_LOWEST_RN_ITEM->sw.value = true;
	}
}

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
	int item = 0;
	while (item < ASI_MAX_FORMATS) {
		if (PIXEL_FORMAT_PROPERTY->items[item].sw.value) {
			if (!strcmp(PIXEL_FORMAT_PROPERTY->items[item].name, RAW8_NAME)) {
				return 8;
			}
			if (!strcmp(PIXEL_FORMAT_PROPERTY->items[item].name, RGB24_NAME)) {
				return 24;
			}
			if (!strcmp(PIXEL_FORMAT_PROPERTY->items[item].name, RAW16_NAME)) {
				return 16;
			}
			if (!strcmp(PIXEL_FORMAT_PROPERTY->items[item].name, Y8_NAME)) {
				return 8;
			}
		}
		item++;
	}
	return 8;
}


static int get_pixel_format(indigo_device *device) {
	int item = 0;
	while (item < ASI_MAX_FORMATS) {
		if (PIXEL_FORMAT_PROPERTY->items[item].sw.value) {
			if (!strcmp(PIXEL_FORMAT_PROPERTY->items[item].name, RAW8_NAME)) {
				return ASI_IMG_RAW8;
			}
			if (!strcmp(PIXEL_FORMAT_PROPERTY->items[item].name, RGB24_NAME)) {
				return ASI_IMG_RGB24;
			}
			if (!strcmp(PIXEL_FORMAT_PROPERTY->items[item].name, RAW16_NAME)) {
				return ASI_IMG_RAW16;
			}
			if (!strcmp(PIXEL_FORMAT_PROPERTY->items[item].name, Y8_NAME)) {
				return ASI_IMG_Y8;
			}
		}
		item++;
	}
	return ASI_IMG_END;
}


static bool pixel_format_supported(indigo_device *device, ASI_IMG_TYPE type) {
	for (int i = 0; i < ASI_MAX_FORMATS; i++) {
		if (i == ASI_IMG_END) return false;
		if (type == PRIVATE_DATA->info.SupportedVideoFormat[i]) return true;
	}
	return false;
}


static indigo_result asi_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(PIXEL_FORMAT_PROPERTY, property))
			indigo_define_property(device, PIXEL_FORMAT_PROPERTY, NULL);
		if (indigo_property_match(ASI_PRESETS_PROPERTY, property))
			indigo_define_property(device, ASI_PRESETS_PROPERTY, NULL);
		if (indigo_property_match(ASI_ADVANCED_PROPERTY, property))
			indigo_define_property(device, ASI_ADVANCED_PROPERTY, NULL);
	}
	return indigo_ccd_enumerate_properties(device, NULL, NULL);
}


static bool asi_open(indigo_device *device) {
	int id = PRIVATE_DATA->dev_id;
	ASI_ERROR_CODE res;

	if (device->is_connected) return false;

	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	if (PRIVATE_DATA->count_open++ == 0) {
		if (indigo_try_global_lock(device) != INDIGO_OK) {
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_try_global_lock(): failed to get lock.");
			PRIVATE_DATA->count_open--;
			return false;
		}
		res = ASIOpenCamera(id);
		if (res) {
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASIOpenCamera(%d) = %d", id, res);
			PRIVATE_DATA->count_open--;
			return false;
		}
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ASIOpenCamera(%d) = %d", id, res);
		res = ASIInitCamera(id);
		if (res) {
			ASICloseCamera(id);
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASIInitCamera(%d) = %d", id, res);
			PRIVATE_DATA->count_open--;
			return false;
		}
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ASIInitCamera(%d) = %d", id, res);
		if (PRIVATE_DATA->buffer == NULL) {
			if(PRIVATE_DATA->info.IsColorCam)
				PRIVATE_DATA->buffer_size = PRIVATE_DATA->info.MaxHeight*PRIVATE_DATA->info.MaxWidth*3 + FITS_HEADER_SIZE;
			else
				PRIVATE_DATA->buffer_size = PRIVATE_DATA->info.MaxHeight*PRIVATE_DATA->info.MaxWidth*2 + FITS_HEADER_SIZE;

			PRIVATE_DATA->buffer = (unsigned char*)indigo_alloc_blob_buffer(PRIVATE_DATA->buffer_size);
		}
	}
	PRIVATE_DATA->is_asi120 = strstr(PRIVATE_DATA->info.Name, "ASI120M") != NULL;
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	return true;
}

static bool asi_setup_exposure(indigo_device *device, double exposure, int frame_left, int frame_top, int frame_width, int frame_height, int horizontal_bin, int vertical_bin) {
	int id = PRIVATE_DATA->dev_id;
	ASI_ERROR_CODE res;
	int c_frame_left, c_frame_top, c_frame_width, c_frame_height, c_bin;
	long c_exposure;
	ASI_IMG_TYPE c_pixel_format;

	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	res = ASIGetROIFormat(id, &c_frame_width, &c_frame_height, &c_bin, &c_pixel_format);
	if (res) INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASIGetROIFormat(%d) = %d", id, res);
	if (c_frame_width != frame_width/horizontal_bin || c_frame_height != frame_height/vertical_bin || c_bin != horizontal_bin || c_pixel_format != get_pixel_format(device)) {
		res = ASISetROIFormat(id, frame_width/horizontal_bin, frame_height/vertical_bin,  horizontal_bin, get_pixel_format(device));
		if (res) {
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASISetROIFormat(%d) = %d", id, res);
			return false;
		} else INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ASISetROIFormat(%d) = %d", id, res);
	}

	res = ASIGetStartPos(id, &c_frame_left, &c_frame_top);
	if (res) INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASIGetStartPos(%d) = %d", id, res);
	if (c_frame_left != frame_left/horizontal_bin || c_frame_top != frame_top/vertical_bin) {
		res = ASISetStartPos(id, frame_left/horizontal_bin, frame_top/vertical_bin);
		if (res) {
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASISetStartPos(%d) = %d", id, res);
			return false;
		} else INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ASISetStartPos(%d) = %d", id, res);
	}

	ASI_BOOL pauto;
	res = ASIGetControlValue(id, ASI_EXPOSURE, &c_exposure, &pauto);
	if (res) INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASIGetControlValue(%d, ASI_EXPOSURE) = %d", id, res);
	if (c_exposure != (long)s2us(exposure)) {
		res = ASISetControlValue(id, ASI_EXPOSURE, (long)s2us(exposure), ASI_FALSE);
		if (res) {
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASISetControlValue(%d, ASI_EXPOSURE) = %d", id, res);
			return false;
		} else INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ASISetControlValue(%d, ASI_EXPOSURE) = %d", id, res);
	}

	PRIVATE_DATA->exp_bin_x = horizontal_bin;
	PRIVATE_DATA->exp_bin_y = vertical_bin;
	PRIVATE_DATA->exp_frame_width = frame_width;
	PRIVATE_DATA->exp_frame_height = frame_height;
	PRIVATE_DATA->exp_bpp = (int)CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value;
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	return true;
}

static bool asi_start_exposure(indigo_device *device, double exposure, bool dark, int frame_left, int frame_top, int frame_width, int frame_height, int horizontal_bin, int vertical_bin) {
	int id = PRIVATE_DATA->dev_id;
	ASI_ERROR_CODE res;
	if (!asi_setup_exposure(device, exposure, frame_left, frame_top, frame_width, frame_height, horizontal_bin, vertical_bin)) {
		return false;
	}
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	res = ASIStartExposure(id, dark);
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	if (res) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASIStartExposure(%d) = %d", id, res);
		return false;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ASIStartExposure(%d) = %d", id, res);
	return true;
}

static bool asi_read_pixels(indigo_device *device) {
	ASI_ERROR_CODE res;
	ASI_EXPOSURE_STATUS status;
	int wait_cycles = 30000;    /* 30000*2000us = 1min */
	status = ASI_EXP_WORKING;

	/* wait for the exposure to complete */
	while((status == ASI_EXP_WORKING) && wait_cycles--) {
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		ASIGetExpStatus(PRIVATE_DATA->dev_id, &status);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		indigo_usleep(2000);
	}
	if (status == ASI_EXP_SUCCESS) {
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		res = ASIGetDataAfterExp(PRIVATE_DATA->dev_id, PRIVATE_DATA->buffer + FITS_HEADER_SIZE, PRIVATE_DATA->buffer_size);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASIGetDataAfterExp(%d) = %d", PRIVATE_DATA->dev_id, res);
			return false;
		}
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ASIGetDataAfterExp(%d) = %d", PRIVATE_DATA->dev_id, res);
		if (PRIVATE_DATA->is_asi120)
			indigo_usleep(150000);
		return true;
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Exposure failed: dev_id = %d exposure status = %d", PRIVATE_DATA->dev_id, status);
		return false;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Exposure failed: dev_id = %d exposure status = %d", PRIVATE_DATA->dev_id, status);
}

static bool asi_abort_exposure(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	ASI_ERROR_CODE res = ASIStopExposure(PRIVATE_DATA->dev_id);
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	if (res) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASIStopExposure(%d) = %d", PRIVATE_DATA->dev_id, res);
		return false;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ASIStopExposure(%d) = %d", PRIVATE_DATA->dev_id, res);
	return true;
}

static bool asi_set_cooler(indigo_device *device, bool status, double target, double *current, long *cooler_power) {
	ASI_ERROR_CODE res;
	ASI_BOOL unused;

	int id = PRIVATE_DATA->dev_id;
	long current_status;
	long temp_x10;

	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);

	if (PRIVATE_DATA->has_temperature_sensor) {
		res = ASIGetControlValue(id, ASI_TEMPERATURE, &temp_x10, &unused);
		if (res)
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASIGetControlValue(%d, ASI_TEMPERATURE) = %d", id, res);
		else
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ASIGetControlValue(%d, ASI_TEMPERATURE) = %d", id, res);
		*current = temp_x10/10.0; /* ASI_TEMPERATURE gives temp x 10 */
	} else {
		*current = 0;
	}

	if (!PRIVATE_DATA->info.IsCoolerCam) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		return true;
	}

	res = ASIGetControlValue(id, ASI_COOLER_ON, &current_status, &unused);
	if (res) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASIGetControlValue(%d, ASI_COOLER_ON) = %d", id, res);
		return false;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ASIGetControlValue(%d, ASI_COOLER_ON) = %d", id, res);

	if (current_status != status) {
		res = ASISetControlValue(id, ASI_COOLER_ON, status, false);
		if (res)
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASISetControlValue(%d, ASI_COOLER_ON) = %d", id, res);
		else
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ASISetControlValue(%d, ASI_COOLER_ON) = %d", id, res);
	} else if(status) {
		res = ASISetControlValue(id, ASI_TARGET_TEMP, (long)target, false);
		if (res)
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASISetControlValue(%d, ASI_TARGET_TEMP) = %d", id, res);
		else
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ASISetControlValue(%d, ASI_TARGET_TEMP) = %d", id, res);
	}

	res = ASIGetControlValue(id, ASI_COOLER_POWER_PERC, cooler_power, &unused);
	if (res)
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASIGetControlValue(%d, ASI_COOLER_POWER_PERC) = %d", id, res);
	else
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ASIGetControlValue(%d, ASI_COOLER_POWER_PERC) = %d", id, res);

	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	return true;
}


static void asi_close(indigo_device *device) {

	if (!device->is_connected) return;

	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	if (--PRIVATE_DATA->count_open == 0) {
		ASICloseCamera(PRIVATE_DATA->dev_id);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ASICloseCamera(%d, ASI_COOLER_POWER_PERC)", PRIVATE_DATA->dev_id);
		indigo_global_unlock(device);
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
	PRIVATE_DATA->exposure_timer = NULL;
	if (!CONNECTION_CONNECTED_ITEM->sw.value) return;

	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		CCD_EXPOSURE_ITEM->number.value = 0;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		if (asi_read_pixels(device)) {
			char *color_string = get_bayer_string(device);
			if ((color_string) &&   /* if colour (bayer) image but not RGB */
			    (PRIVATE_DATA->exp_bpp != 24) &&
			    (PRIVATE_DATA->exp_bpp != 48)) {
				/* NOTE: There is no need to take care about the offsets,
				   the SDK takes care the image to be in the correct bayer pattern */
				indigo_fits_keyword keywords[] = {
					{ INDIGO_FITS_STRING, "BAYERPAT", .string = color_string, "Bayer color pattern" },
					{ INDIGO_FITS_NUMBER, "XBAYROFF", .number = 0, "X offset of Bayer array" },
					{ INDIGO_FITS_NUMBER, "YBAYROFF", .number = 0, "Y offset of Bayer array" },
					{ 0 }
				};
				indigo_process_image(device, PRIVATE_DATA->buffer, (int)(PRIVATE_DATA->exp_frame_width / PRIVATE_DATA->exp_bin_x), (int)(PRIVATE_DATA->exp_frame_height / PRIVATE_DATA->exp_bin_y), PRIVATE_DATA->exp_bpp, true, false, keywords, false);
			} else {
				indigo_process_image(device, PRIVATE_DATA->buffer, (int)(PRIVATE_DATA->exp_frame_width / PRIVATE_DATA->exp_bin_x), (int)(PRIVATE_DATA->exp_frame_height / PRIVATE_DATA->exp_bin_y), PRIVATE_DATA->exp_bpp, true, false, NULL, false);
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

static void streaming_timer_callback(indigo_device *device) {
	if (!CONNECTION_CONNECTED_ITEM->sw.value) return;

	char *color_string = get_bayer_string(device);
	indigo_fits_keyword keywords[] = {
		{ INDIGO_FITS_STRING, "BAYERPAT", .string = color_string, "Bayer color pattern" },
		{ INDIGO_FITS_NUMBER, "XBAYROFF", .number = 0, "X offset of Bayer array" },
		{ INDIGO_FITS_NUMBER, "YBAYROFF", .number = 0, "Y offset of Bayer array" },
		{ 0 }
	};
	int id = PRIVATE_DATA->dev_id;
	int timeout = 1000 * (CCD_STREAMING_EXPOSURE_ITEM->number.value * 2 + 500);
	ASI_ERROR_CODE res;
	PRIVATE_DATA->can_check_temperature = false;
	if (asi_setup_exposure(device, CCD_STREAMING_EXPOSURE_ITEM->number.value, CCD_FRAME_LEFT_ITEM->number.value, CCD_FRAME_TOP_ITEM->number.value, CCD_FRAME_WIDTH_ITEM->number.value, CCD_FRAME_HEIGHT_ITEM->number.value, CCD_BIN_HORIZONTAL_ITEM->number.value, CCD_BIN_VERTICAL_ITEM->number.value)) {
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		res = ASIStartVideoCapture(id);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASIStartVideoCapture(%d) = %d", id, res);
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ASIStartVideoCapture(%d) = %d", id, res);
			while (CCD_STREAMING_COUNT_ITEM->number.value != 0) {
				pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
				res = ASIGetVideoData(id, PRIVATE_DATA->buffer + FITS_HEADER_SIZE, PRIVATE_DATA->buffer_size, timeout);
				pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
				if (res) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASIGetVideoData((%d) = %d", id, res);
					break;
				}
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ASIGetVideoData((%d) = %d", id, res);
				if ((color_string) &&   /* if colour (bayer) image but not RGB */
				    (PRIVATE_DATA->exp_bpp != 24) &&
				    (PRIVATE_DATA->exp_bpp != 48)) {
					indigo_process_image(device, PRIVATE_DATA->buffer, (int)(PRIVATE_DATA->exp_frame_width / PRIVATE_DATA->exp_bin_x), (int)(PRIVATE_DATA->exp_frame_height / PRIVATE_DATA->exp_bin_y), PRIVATE_DATA->exp_bpp, true, false, keywords, true);
				} else {
					indigo_process_image(device, PRIVATE_DATA->buffer, (int)(PRIVATE_DATA->exp_frame_width / PRIVATE_DATA->exp_bin_x), (int)(PRIVATE_DATA->exp_frame_height / PRIVATE_DATA->exp_bin_y), PRIVATE_DATA->exp_bpp, true, false, NULL, true);
				}
				if (CCD_STREAMING_COUNT_ITEM->number.value > 0)
					CCD_STREAMING_COUNT_ITEM->number.value -= 1;
				CCD_STREAMING_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
			}
			pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
			res = ASIStopVideoCapture(id);
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			if (res)
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASIStopVideoCapture(%d) = %d", id, res);
			else
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ASIStopVideoCapture(%d) = %d", id, res);
		}
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	} else {
		res = ASI_ERROR_GENERAL_ERROR;
	}
	PRIVATE_DATA->can_check_temperature = true;
	indigo_finalize_video_stream(device);
	if (res) {
		CCD_STREAMING_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, CCD_STREAMING_PROPERTY, "Streaming failed");
	} else {
		CCD_STREAMING_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
	}
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
	indigo_reschedule_timer(device, 5, &PRIVATE_DATA->temperature_timer);
}


static void guider_timer_callback_ra(indigo_device *device) {
	PRIVATE_DATA->guider_timer_ra = NULL;
	int id = PRIVATE_DATA->dev_id;

	if (!CONNECTION_CONNECTED_ITEM->sw.value) return;

	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	ASIPulseGuideOff(id, ASI_GUIDE_EAST);
	ASIPulseGuideOff(id, ASI_GUIDE_WEST);
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);

	if (PRIVATE_DATA->guide_relays[ASI_GUIDE_EAST] || PRIVATE_DATA->guide_relays[ASI_GUIDE_WEST]) {
		GUIDER_GUIDE_EAST_ITEM->number.value = 0;
		GUIDER_GUIDE_WEST_ITEM->number.value = 0;
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
	}
	PRIVATE_DATA->guide_relays[ASI_GUIDE_EAST] = false;
	PRIVATE_DATA->guide_relays[ASI_GUIDE_WEST] = false;
}


static void guider_timer_callback_dec(indigo_device *device) {
	PRIVATE_DATA->guider_timer_dec = NULL;
	int id = PRIVATE_DATA->dev_id;

	if (!CONNECTION_CONNECTED_ITEM->sw.value) return;

	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	ASIPulseGuideOff(id, ASI_GUIDE_SOUTH);
	ASIPulseGuideOff(id, ASI_GUIDE_NORTH);
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);

	if (PRIVATE_DATA->guide_relays[ASI_GUIDE_NORTH] || PRIVATE_DATA->guide_relays[ASI_GUIDE_SOUTH]) {
		GUIDER_GUIDE_NORTH_ITEM->number.value = 0;
		GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
	}
	PRIVATE_DATA->guide_relays[ASI_GUIDE_SOUTH] = false;
	PRIVATE_DATA->guide_relays[ASI_GUIDE_NORTH] = false;
}


static indigo_result ccd_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_ccd_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		pthread_mutex_init(&PRIVATE_DATA->usb_mutex, NULL);
		// -------------------------------------------------------------------------------- PIXEL_FORMAT_PROPERTY
		PIXEL_FORMAT_PROPERTY = indigo_init_switch_property(NULL, device->name, "PIXEL_FORMAT", CCD_ADVANCED_GROUP, "Pixel Format", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, ASI_MAX_FORMATS);
		if (PIXEL_FORMAT_PROPERTY == NULL)
			return INDIGO_FAILED;

		int format_count = 0;
		if (pixel_format_supported(device, ASI_IMG_RAW8)) {
			indigo_init_switch_item(PIXEL_FORMAT_PROPERTY->items + format_count, RAW8_NAME, RAW8_NAME, true);
			format_count++;
		}
		if (pixel_format_supported(device, ASI_IMG_RGB24)) {
			indigo_init_switch_item(PIXEL_FORMAT_PROPERTY->items + format_count, RGB24_NAME, RGB24_NAME, false);
			format_count++;
		}
		if (pixel_format_supported(device, ASI_IMG_RAW16)) {
			indigo_init_switch_item(PIXEL_FORMAT_PROPERTY->items + format_count, RAW16_NAME, RAW16_NAME, false);
			format_count++;
		}
		if (pixel_format_supported(device, ASI_IMG_Y8)) {
			indigo_init_switch_item(PIXEL_FORMAT_PROPERTY->items + format_count, Y8_NAME, Y8_NAME, false);
			format_count++;
		}
		PIXEL_FORMAT_PROPERTY->count = format_count;

		INFO_PROPERTY->count = 6;
		indigo_copy_value(INFO_DEVICE_MODEL_ITEM->text.value, PRIVATE_DATA->info.Name);
		char *sdk_version = ASIGetSDKVersion();
		indigo_copy_value(INFO_DEVICE_FW_REVISION_ITEM->text.value, sdk_version);
		indigo_copy_value(INFO_DEVICE_FW_REVISION_ITEM->label, "SDK version");

		CCD_INFO_WIDTH_ITEM->number.value = PRIVATE_DATA->info.MaxWidth;
		CCD_INFO_HEIGHT_ITEM->number.value = PRIVATE_DATA->info.MaxHeight;
		CCD_INFO_PIXEL_SIZE_ITEM->number.value = CCD_INFO_PIXEL_WIDTH_ITEM->number.value = CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = PRIVATE_DATA->info.PixelSize;
		CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = PRIVATE_DATA->info.BitDepth;

		CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.target = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max = PRIVATE_DATA->info.MaxWidth;
		CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.target = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max = PRIVATE_DATA->info.MaxHeight;
		CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.target = get_pixel_depth(device);
		CCD_FRAME_BITS_PER_PIXEL_ITEM->number.min = 8;
		CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = 24;

		/* find max binning */
		int max_bin = 1;
		for (int num = 0; (num < 16) && PRIVATE_DATA->info.SupportedBins[num]; num++) {
			max_bin = PRIVATE_DATA->info.SupportedBins[num];
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
		for (int num = 0; (num < 16) && PRIVATE_DATA->info.SupportedBins[num]; num++) {
			int bin = PRIVATE_DATA->info.SupportedBins[num];
			if (pixel_format_supported(device, ASI_IMG_RAW8)) {
				snprintf(name, 32, "%s %dx%d", RAW8_NAME, bin, bin);
				snprintf(label, 64, "%s %dx%d", RAW8_NAME, (int)CCD_FRAME_WIDTH_ITEM->number.value / bin, (int)CCD_FRAME_HEIGHT_ITEM->number.value / bin);
				indigo_init_switch_item(CCD_MODE_PROPERTY->items + mode_count, name, label, bin == 1);
				mode_count++;
			}
			if (pixel_format_supported(device, ASI_IMG_RGB24)) {
				snprintf(name, 32, "%s %dx%d", RGB24_NAME, bin, bin);
				snprintf(label, 64, "%s %dx%d", RGB24_NAME, (int)CCD_FRAME_WIDTH_ITEM->number.value / bin, (int)CCD_FRAME_HEIGHT_ITEM->number.value / bin);
				indigo_init_switch_item(CCD_MODE_PROPERTY->items + mode_count, name, label, false);
				mode_count++;
			}
			if (pixel_format_supported(device, ASI_IMG_RAW16)) {
				snprintf(name, 32, "%s %dx%d", RAW16_NAME, bin, bin);
				snprintf(label, 64, "%s %dx%d", RAW16_NAME, (int)CCD_FRAME_WIDTH_ITEM->number.value / bin, (int)CCD_FRAME_HEIGHT_ITEM->number.value / bin);
				indigo_init_switch_item(CCD_MODE_PROPERTY->items + mode_count, name, label, false);
				mode_count++;
			}
			if (pixel_format_supported(device, ASI_IMG_Y8)) {
				snprintf(name, 32, "%s %dx%d", Y8_NAME, bin, bin);
				snprintf(label, 64, "%s %dx%d", Y8_NAME, (int)CCD_FRAME_WIDTH_ITEM->number.value / bin, (int)CCD_FRAME_HEIGHT_ITEM->number.value / bin);
				indigo_init_switch_item(CCD_MODE_PROPERTY->items + mode_count, name, label, false);
				mode_count++;
			}
		}
		CCD_MODE_PROPERTY->count = mode_count;
		// -------------------------------------------------------------------------------- CCD_STREAMING
		CCD_STREAMING_PROPERTY->hidden = false;
		CCD_IMAGE_FORMAT_PROPERTY->count = 7;
		CCD_STREAMING_EXPOSURE_ITEM->number.max = 4.0;

		// -------------------------------------------------------------------------------- ASI_PRESETS
		ASI_PRESETS_PROPERTY = indigo_init_switch_property(NULL, device->name, "ASI_PRESETS", CCD_ADVANCED_GROUP, "Presets (Gain, Offset)", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 3);
		if (ASI_PRESETS_PROPERTY == NULL)
			return INDIGO_FAILED;

		// -------------------------------------------------------------------------------- ASI_ADVANCED
		ASI_ADVANCED_PROPERTY = indigo_init_number_property(NULL, device->name, "ASI_ADVANCED", CCD_ADVANCED_GROUP, "Advanced", INDIGO_OK_STATE, INDIGO_RW_PERM, 0);
		if (ASI_ADVANCED_PROPERTY == NULL)
			return INDIGO_FAILED;
		// --------------------------------------------------------------------------------
		return indigo_ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result handle_advanced_property(indigo_device *device, indigo_property *property) {
	int ctrl_count;
	ASI_CONTROL_CAPS ctrl_caps;
	ASI_ERROR_CODE res;
	int id = PRIVATE_DATA->dev_id;

	if (!IS_CONNECTED) return INDIGO_OK;

	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);

	res = ASIGetNumOfControls(id, &ctrl_count);
	if (res) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASIGetNumOfControls(%d) = %d", id, res);
		return INDIGO_NOT_FOUND;
	}
	ASI_BOOL unused;
	long value;
	for(int ctrl_no = 0; ctrl_no < ctrl_count; ctrl_no++) {
		ASIGetControlCaps(id, ctrl_no, &ctrl_caps);
		for(int item = 0; item < property->count; item++) {
			if(!strncmp(ctrl_caps.Name, property->items[item].name, INDIGO_NAME_SIZE)) {
				res = ASISetControlValue(id, ctrl_caps.ControlType,property->items[item].number.value, ASI_FALSE);
				if (res) INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASISetControlValue(%d, %s) = %d", id, ctrl_caps.Name, res);
				res = ASIGetControlValue(id, ctrl_caps.ControlType,&value, &unused);
				property->items[item].number.value = value;
				if (res) INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASIGetControlValue(%d, %s) = %d, value = %d", id, ctrl_caps.Name, res, property->items[item].number.value);
			}
		}
	}

	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	return INDIGO_OK;
}

static indigo_result init_camera_property(indigo_device *device, ASI_CONTROL_CAPS ctrl_caps) {
	int id = PRIVATE_DATA->dev_id;
	long value;
	ASI_ERROR_CODE res;
	ASI_BOOL unused;

	if (ctrl_caps.ControlType == ASI_EXPOSURE) {
		CCD_EXPOSURE_PROPERTY->hidden = false;
		if(ctrl_caps.IsWritable)
			CCD_EXPOSURE_PROPERTY->perm = INDIGO_RW_PERM;
		else
			CCD_EXPOSURE_PROPERTY->perm = INDIGO_RO_PERM;

		CCD_EXPOSURE_ITEM->number.min = us2s(ctrl_caps.MinValue);
		CCD_EXPOSURE_ITEM->number.max = us2s(ctrl_caps.MaxValue);
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		res = ASIGetControlValue(id, ASI_EXPOSURE, &value, &unused);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if (res) INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASIGetControlValue(%d, ASI_EXPOSURE) = %d", id, res);
		CCD_EXPOSURE_ITEM->number.value = CCD_EXPOSURE_ITEM->number.target = us2s(value);
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
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		res = ASIGetControlValue(id, ASI_GAIN, &value, &unused);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if (res) INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASIGetControlValue(%d, ASI_GAIN) = %d", id, res);
		CCD_GAIN_ITEM->number.value = CCD_GAIN_ITEM->number.target = value;
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
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		res = ASIGetControlValue(id, ASI_GAMMA, &value, &unused);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if (res) INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASIGetControlValue(%d, ASI_GAMMA) = %d", id, res);
		CCD_GAMMA_ITEM->number.value = CCD_GAMMA_ITEM->number.target = value;
		CCD_GAMMA_ITEM->number.step = 1;
		return INDIGO_OK;
	}

	if (ctrl_caps.ControlType == ASI_BRIGHTNESS) {
		CCD_OFFSET_PROPERTY->hidden = false;
		if(ctrl_caps.IsWritable)
			CCD_OFFSET_PROPERTY->perm = INDIGO_RW_PERM;
		else
			CCD_OFFSET_PROPERTY->perm = INDIGO_RO_PERM;

		CCD_OFFSET_ITEM->number.min = ctrl_caps.MinValue;
		CCD_OFFSET_ITEM->number.max = ctrl_caps.MaxValue;
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		res = ASIGetControlValue(id, ASI_BRIGHTNESS, &value, &unused);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if (res) INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASIGetControlValue(%d, ASI_BRIGHTNESS) = %d", id, res);
		CCD_OFFSET_ITEM->number.value = CCD_OFFSET_ITEM->number.target = value;
		CCD_OFFSET_ITEM->number.step = 1;
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
		CCD_TEMPERATURE_ITEM->number.value = CCD_TEMPERATURE_ITEM->number.target = ctrl_caps.DefaultValue;
		PRIVATE_DATA->target_temperature = ctrl_caps.DefaultValue;
		PRIVATE_DATA->can_check_temperature = true;
		return INDIGO_OK;
	}

	if (ctrl_caps.ControlType == ASI_TEMPERATURE) {
		if (CCD_TEMPERATURE_PROPERTY->hidden) {
			PRIVATE_DATA->can_check_temperature = true;
			CCD_TEMPERATURE_PROPERTY->perm = INDIGO_RO_PERM;
			CCD_TEMPERATURE_PROPERTY->hidden = false;
		}
		PRIVATE_DATA->has_temperature_sensor = true;
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
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		res = ASIGetControlValue(id, ASI_COOLER_POWER_PERC, &value, &unused);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if (res) INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASIGetControlValue(%d, ASI_COOLER_POWER_PERC) = %d", id, res);
		CCD_COOLER_POWER_ITEM->number.value = CCD_COOLER_POWER_ITEM->number.target = value;
		return INDIGO_OK;
	}

	int offset = ASI_ADVANCED_PROPERTY->count;
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	res = ASIGetControlValue(id, ctrl_caps.ControlType, &value, &unused);
	int res2 = 0;
	if (ctrl_caps.ControlType == ASI_BANDWIDTHOVERLOAD && value == 100) {
		value = ASI_DEFAULT_BANDWIDTH;
		res2 = ASISetControlValue(id, ctrl_caps.ControlType, value, false);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Default USB Bandwidth is 100, reducing to %d", value);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	if (res) INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASIGetControlValue(%d, %s) = %d", id, ctrl_caps.Name, res);
	if (res2) INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASISetControlValue(%d, %s) = %d", id, ctrl_caps.Name, res2);

	ASI_ADVANCED_PROPERTY = indigo_resize_property(ASI_ADVANCED_PROPERTY, offset + 1);
	indigo_init_number_item(ASI_ADVANCED_PROPERTY->items+offset, ctrl_caps.Name, ctrl_caps.Name, ctrl_caps.MinValue, ctrl_caps.MaxValue, 1, value);
	return INDIGO_OK;
}


static void handle_ccd_connect_property(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (!device->is_connected) {
			if (asi_open(device)) {
				int id = PRIVATE_DATA->dev_id;
				int ctrl_count;
				ASI_CONTROL_CAPS ctrl_caps;

				indigo_define_property(device, PIXEL_FORMAT_PROPERTY, NULL);

				pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
				int res = ASIGetNumOfControls(id, &ctrl_count);
				pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
				if (res) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASIGetNumOfControls(%d) = %d", id, res);
				}
				ASI_ADVANCED_PROPERTY = indigo_resize_property(ASI_ADVANCED_PROPERTY, 0);
				for(int ctrl_no = 0; ctrl_no < ctrl_count; ctrl_no++) {
					pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
					ASIGetControlCaps(id, ctrl_no, &ctrl_caps);
					pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
					init_camera_property(device, ctrl_caps);
					adjust_preset_switches(device);
				}
				indigo_define_property(device, ASI_ADVANCED_PROPERTY, NULL);

				pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
				res = ASIGetGainOffset(
					id,
					&PRIVATE_DATA->offset_highest_dr,
					&PRIVATE_DATA->offset_unity_gain,
					&PRIVATE_DATA->gain_lowerst_rn,
					&PRIVATE_DATA->offset_lowest_rn
				);
				pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
				if (res) {
					INDIGO_DRIVER_LOG( DRIVER_NAME, "ASIGetGainOffset(%d) = %d", id, res);
				}

				PRIVATE_DATA->gain_unity_gain = get_unity_gain(device);
				PRIVATE_DATA->gain_highest_dr = 0;

				char item_desc[100];
				sprintf(item_desc, "Highest Dynamic Range (%d, %d)", PRIVATE_DATA->gain_highest_dr, PRIVATE_DATA->offset_highest_dr);
				indigo_init_switch_item(ASI_HIGHEST_DR_ITEM, ASI_HIGHEST_DR_NAME, item_desc, false);

				sprintf(item_desc, "Unity Gain (%d, %d)", PRIVATE_DATA->gain_unity_gain, PRIVATE_DATA->offset_unity_gain);
				indigo_init_switch_item(ASI_UNITY_GAIN_ITEM, ASI_UNITY_GAIN_NAME, item_desc, false);

				sprintf(item_desc, "Lowest Readout Noise (%d, %d)", PRIVATE_DATA->gain_lowerst_rn, PRIVATE_DATA->offset_lowest_rn);
				indigo_init_switch_item(ASI_LOWEST_RN_ITEM, ASI_LOWEST_RN_NAME, item_desc, false);

				adjust_preset_switches(device);
				indigo_define_property(device, ASI_PRESETS_PROPERTY, NULL);

				device->is_connected = true;
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
				if (PRIVATE_DATA->has_temperature_sensor) {
					indigo_set_timer(device, 0, ccd_temperature_callback, &PRIVATE_DATA->temperature_timer);
				}
			} else {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		}
	} else {
		if(device->is_connected) {
			PRIVATE_DATA->can_check_temperature = false;
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->temperature_timer);
			if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
				indigo_cancel_timer_sync(device, &PRIVATE_DATA->exposure_timer);
				asi_abort_exposure(device);
			} else if (CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE && CCD_STREAMING_COUNT_ITEM->number.value != 0) {
				CCD_STREAMING_COUNT_ITEM->number.value = 0;
				indigo_cancel_timer_sync(device, &PRIVATE_DATA->exposure_timer);
			}
			indigo_delete_property(device, PIXEL_FORMAT_PROPERTY, NULL);
			indigo_delete_property(device, ASI_PRESETS_PROPERTY, NULL);
			indigo_delete_property(device, ASI_ADVANCED_PROPERTY, NULL);
			asi_close(device);
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
		indigo_set_timer(device, 0, handle_ccd_connect_property, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- CCD_EXPOSURE
	} else if (indigo_property_match_w(CCD_EXPOSURE_PROPERTY, property)) {
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE)
			return INDIGO_OK;
		indigo_property_copy_values(CCD_EXPOSURE_PROPERTY, property, false);
		indigo_use_shortest_exposure_if_bias(device);
		asi_start_exposure(device, CCD_EXPOSURE_ITEM->number.target, CCD_FRAME_TYPE_DARK_ITEM->sw.value || CCD_FRAME_TYPE_DARKFLAT_ITEM->sw.value || CCD_FRAME_TYPE_BIAS_ITEM->sw.value, CCD_FRAME_LEFT_ITEM->number.value, CCD_FRAME_TOP_ITEM->number.value, CCD_FRAME_WIDTH_ITEM->number.value, CCD_FRAME_HEIGHT_ITEM->number.value, CCD_BIN_HORIZONTAL_ITEM->number.value, CCD_BIN_VERTICAL_ITEM->number.value);
		CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		if (CCD_UPLOAD_MODE_LOCAL_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
			CCD_IMAGE_FILE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
		}
		if (CCD_UPLOAD_MODE_CLIENT_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
			CCD_IMAGE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
		}
		if (CCD_EXPOSURE_ITEM->number.target > 4)
			indigo_set_timer(device, CCD_EXPOSURE_ITEM->number.target - 4, clear_reg_timer_callback, &PRIVATE_DATA->exposure_timer);
		else {
			PRIVATE_DATA->can_check_temperature = false;
			indigo_set_timer(device, CCD_EXPOSURE_ITEM->number.target, exposure_timer_callback, &PRIVATE_DATA->exposure_timer);
		}
	} else if (indigo_property_match(CCD_STREAMING_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_STREAMING
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE)
			return INDIGO_OK;
		indigo_property_copy_values(CCD_STREAMING_PROPERTY, property, false);
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
		indigo_set_timer(device, 0, streaming_timer_callback, &PRIVATE_DATA->exposure_timer);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- CCD_ABORT_EXPOSURE
	} else if (indigo_property_match(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_cancel_timer(device, &PRIVATE_DATA->exposure_timer);
			asi_abort_exposure(device);
		} else if (CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE && CCD_STREAMING_COUNT_ITEM->number.value != 0) {
			CCD_STREAMING_COUNT_ITEM->number.value = 0;
		}
		PRIVATE_DATA->can_check_temperature = true;
		indigo_property_copy_values(CCD_ABORT_EXPOSURE_PROPERTY, property, false);
		// -------------------------------------------------------------------------------- CCD_COOLER
	} else if (indigo_property_match_w(CCD_COOLER_PROPERTY, property)) {
		indigo_property_copy_values(CCD_COOLER_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value && !CCD_COOLER_PROPERTY->hidden) {
			CCD_COOLER_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
		}
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- CCD_TEMPERATURE
	} else if (indigo_property_match_w(CCD_TEMPERATURE_PROPERTY, property)) {
		indigo_property_copy_values(CCD_TEMPERATURE_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value && !CCD_COOLER_PROPERTY->hidden) {
			PRIVATE_DATA->target_temperature = CCD_TEMPERATURE_ITEM->number.value;
			CCD_TEMPERATURE_ITEM->number.value = PRIVATE_DATA->current_temperature;
			CCD_TEMPERATURE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, "Target Temperature = %.2f", PRIVATE_DATA->target_temperature);
		}
		return INDIGO_OK;
		// ------------------------------------------------------------------------------- CCD_GAMMA
	} else if (indigo_property_match_w(CCD_GAMMA_PROPERTY, property)) {
		if (!IS_CONNECTED) return INDIGO_OK;
		CCD_GAMMA_PROPERTY->state = INDIGO_OK_STATE;
		indigo_property_copy_values(CCD_GAMMA_PROPERTY, property, false);

		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		ASI_ERROR_CODE res = ASISetControlValue(PRIVATE_DATA->dev_id, ASI_GAMMA, (long)(CCD_GAMMA_ITEM->number.value), ASI_FALSE);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASISetControlValue(%d, ASI_GAMMA) = %d", PRIVATE_DATA->dev_id, res);
			CCD_GAMMA_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			CCD_GAMMA_PROPERTY->state = INDIGO_OK_STATE;
		}
		indigo_update_property(device, CCD_GAMMA_PROPERTY, NULL);
		return INDIGO_OK;
		// ------------------------------------------------------------------------------- CCD_OFFSET
	} else if (indigo_property_match_w(CCD_OFFSET_PROPERTY, property)) {
		if (!IS_CONNECTED) return INDIGO_OK;
		CCD_OFFSET_PROPERTY->state = INDIGO_OK_STATE;
		indigo_property_copy_values(CCD_OFFSET_PROPERTY, property, false);

		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		ASI_ERROR_CODE res = ASISetControlValue(PRIVATE_DATA->dev_id, ASI_BRIGHTNESS, (long)(CCD_OFFSET_ITEM->number.value), ASI_FALSE);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASISetControlValue(%d, ASI_BRIGHTNESS) = %d", PRIVATE_DATA->dev_id, res);
			CCD_OFFSET_PROPERTY->state = INDIGO_ALERT_STATE;
			ASI_PRESETS_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			CCD_OFFSET_PROPERTY->state = INDIGO_OK_STATE;
			ASI_PRESETS_PROPERTY->state = INDIGO_OK_STATE;
		}
		adjust_preset_switches(device);

		indigo_update_property(device, CCD_OFFSET_PROPERTY, NULL);
		indigo_update_property(device, ASI_PRESETS_PROPERTY, NULL);
		return INDIGO_OK;
		// ------------------------------------------------------------------------------- CCD_GAIN
	} else if (indigo_property_match_w(CCD_GAIN_PROPERTY, property)) {
		if (!IS_CONNECTED) return INDIGO_OK;
		CCD_GAIN_PROPERTY->state = INDIGO_OK_STATE;
		indigo_property_copy_values(CCD_GAIN_PROPERTY, property, false);

		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		ASI_ERROR_CODE res = ASISetControlValue(PRIVATE_DATA->dev_id, ASI_GAIN, (long)(CCD_GAIN_ITEM->number.value), ASI_FALSE);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASISetControlValue(%d, ASI_GAIN) = %d", PRIVATE_DATA->dev_id, res);
			CCD_GAIN_PROPERTY->state = INDIGO_ALERT_STATE;
			ASI_PRESETS_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			CCD_GAIN_PROPERTY->state = INDIGO_OK_STATE;
			ASI_PRESETS_PROPERTY->state = INDIGO_OK_STATE;
		}
		adjust_preset_switches(device);

		indigo_update_property(device, CCD_GAIN_PROPERTY, NULL);
		indigo_update_property(device, ASI_PRESETS_PROPERTY, NULL);
		return INDIGO_OK;
		// ------------------------------------------------------------------------------- ASI_PRESETS
	} else if (indigo_property_match(ASI_PRESETS_PROPERTY, property)) {
		if (!IS_CONNECTED) return INDIGO_OK;
		ASI_PRESETS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_property_copy_values(ASI_PRESETS_PROPERTY, property, false);
		int gain = 0, offset = 0;
		if (ASI_HIGHEST_DR_ITEM->sw.value) {
			gain = PRIVATE_DATA->gain_highest_dr;
			offset = PRIVATE_DATA->offset_highest_dr;
		} else if (ASI_UNITY_GAIN_ITEM->sw.value) {
			gain = PRIVATE_DATA->gain_unity_gain;
			offset = PRIVATE_DATA->offset_unity_gain;
		} else if (ASI_LOWEST_RN_ITEM->sw.value) {
			gain = PRIVATE_DATA->gain_lowerst_rn;
			offset = PRIVATE_DATA->offset_lowest_rn;
		}

		CCD_GAIN_PROPERTY->state = INDIGO_OK_STATE;
		CCD_OFFSET_PROPERTY->state = INDIGO_OK_STATE;
		ASI_PRESETS_PROPERTY->state = INDIGO_OK_STATE;

		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		ASI_ERROR_CODE res = ASISetControlValue(PRIVATE_DATA->dev_id, ASI_GAIN, (long)gain, ASI_FALSE);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASISetControlValue(%d, ASI_GAIN) = %d", PRIVATE_DATA->dev_id, res);
			CCD_GAIN_PROPERTY->state = INDIGO_ALERT_STATE;
			ASI_PRESETS_PROPERTY->state = INDIGO_ALERT_STATE;
		}

		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		res = ASISetControlValue(PRIVATE_DATA->dev_id, ASI_BRIGHTNESS, (long)offset, ASI_FALSE);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASISetControlValue(%d, ASI_BRIGHTNESS) = %d", PRIVATE_DATA->dev_id, res);
			CCD_OFFSET_PROPERTY->state = INDIGO_ALERT_STATE;
			ASI_PRESETS_PROPERTY->state = INDIGO_ALERT_STATE;
		}

		CCD_GAIN_ITEM->number.value = gain;
		CCD_OFFSET_ITEM->number.value = offset;

		indigo_update_property(device, CCD_GAIN_PROPERTY, NULL);
		indigo_update_property(device, CCD_OFFSET_PROPERTY, NULL);
		indigo_update_property(device, ASI_PRESETS_PROPERTY, NULL);
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
		CCD_FRAME_PROPERTY->state = INDIGO_OK_STATE;

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
		for (int i = 0; i < PIXEL_FORMAT_PROPERTY->count; i++) {
			if (!strcmp(PIXEL_FORMAT_PROPERTY->items[i].name, RAW8_NAME) && CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value == 8) {
				indigo_set_switch(PIXEL_FORMAT_PROPERTY, PIXEL_FORMAT_PROPERTY->items + i, true);
				snprintf(name, 32, "%s %dx%d", PIXEL_FORMAT_PROPERTY->items[i].name, horizontal_bin, vertical_bin);
				break;
			}
			if (!strcmp(PIXEL_FORMAT_PROPERTY->items[i].name, RAW16_NAME) && CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value == 16) {
				indigo_set_switch(PIXEL_FORMAT_PROPERTY, PIXEL_FORMAT_PROPERTY->items + i, true);
				snprintf(name, 32, "%s %dx%d", PIXEL_FORMAT_PROPERTY->items[i].name, horizontal_bin, vertical_bin);
				break;
			}
			if (!strcmp(PIXEL_FORMAT_PROPERTY->items[i].name, RGB24_NAME) && CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value == 24) {
				indigo_set_switch(PIXEL_FORMAT_PROPERTY, PIXEL_FORMAT_PROPERTY->items + i, true);
				snprintf(name, 32, "%s %dx%d", PIXEL_FORMAT_PROPERTY->items[i].name, horizontal_bin, vertical_bin);
				break;
			}
		}
		for (int i = 0; i < CCD_MODE_PROPERTY->count; i++) {
			indigo_item *item = &CCD_MODE_PROPERTY->items[i];
			item->sw.value = !strcmp(item->name, name);
		}
		CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.target = get_pixel_depth(device);
		CCD_MODE_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_CONNECTED) {
			indigo_update_property(device, CCD_FRAME_PROPERTY, NULL);
			indigo_update_property(device, CCD_MODE_PROPERTY, NULL);
			indigo_update_property(device, PIXEL_FORMAT_PROPERTY, NULL);
		}
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- PIXEL_FORMAT
	} else if (indigo_property_match(PIXEL_FORMAT_PROPERTY, property)) {
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
			PIXEL_FORMAT_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, PIXEL_FORMAT_PROPERTY, "Exposure in progress, pixel format can not be changed.");
			return INDIGO_OK;
		}
		indigo_property_copy_values(PIXEL_FORMAT_PROPERTY, property, false);
		PIXEL_FORMAT_PROPERTY->state = INDIGO_OK_STATE;

		/* NOTE: BPP can not be set directly because can not be linked to PIXEL_FORMAT_PROPERTY */
		CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.target = get_pixel_depth(device);
		CCD_FRAME_PROPERTY->state = INDIGO_OK_STATE;

		int horizontal_bin = (int)CCD_BIN_HORIZONTAL_ITEM->number.value;
		int vertical_bin = (int)CCD_BIN_VERTICAL_ITEM->number.value;
		char name[32] = "";
		for (int i = 0; i < PIXEL_FORMAT_PROPERTY->count; i++) {
			if (PIXEL_FORMAT_PROPERTY->items[i].sw.value) {
				snprintf(name, 32, "%s %dx%d", PIXEL_FORMAT_PROPERTY->items[i].name, horizontal_bin, vertical_bin);
				break;
			}
		}
		for (int i = 0; i < CCD_MODE_PROPERTY->count; i++) {
			indigo_item *item = &CCD_MODE_PROPERTY->items[i];
			item->sw.value = !strcmp(item->name, name);
		}
		CCD_MODE_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_CONNECTED) {
			indigo_update_property(device, CCD_FRAME_PROPERTY, NULL);
			indigo_update_property(device, CCD_MODE_PROPERTY, NULL);
			indigo_update_property(device, PIXEL_FORMAT_PROPERTY, NULL);
		}
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- ADVANCED
	} else if (indigo_property_match(ASI_ADVANCED_PROPERTY, property)) {
		if (!IS_CONNECTED) return INDIGO_OK;
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
			ASI_ADVANCED_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, ASI_ADVANCED_PROPERTY, "Exposure in progress, advanced settings can not be changed.");
			return INDIGO_OK;
		}
		handle_advanced_property(device, property);
		indigo_property_copy_values(ASI_ADVANCED_PROPERTY, property, false);
		ASI_ADVANCED_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, ASI_ADVANCED_PROPERTY, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- CCD_MODE
	} else if (indigo_property_match(CCD_MODE_PROPERTY, property)) {
		indigo_property_copy_values(CCD_MODE_PROPERTY, property, false);
		char name[32] = "";
		int h, v;
		for (int i = 0; i < CCD_MODE_PROPERTY->count; i++) {
			indigo_item *item = &CCD_MODE_PROPERTY->items[i];
			if (item->sw.value) {
				for (int j = 0; j < PIXEL_FORMAT_PROPERTY->count; j++) {
					snprintf(name, 32, "%s %%dx%%d", PIXEL_FORMAT_PROPERTY->items[j].name);
					if (sscanf(item->name, name, &h, &v) == 2) {
						CCD_BIN_HORIZONTAL_ITEM->number.value = CCD_BIN_HORIZONTAL_ITEM->number.target = h;
						CCD_BIN_VERTICAL_ITEM->number.value = CCD_BIN_VERTICAL_ITEM->number.target = v;
						PIXEL_FORMAT_PROPERTY->items[j].sw.value = true;
					} else {
						PIXEL_FORMAT_PROPERTY->items[j].sw.value = false;
					}
				}
				break;
			}
		}
		CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.target = get_pixel_depth(device);
		if (IS_CONNECTED) {
			PIXEL_FORMAT_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, PIXEL_FORMAT_PROPERTY, NULL);
			CCD_FRAME_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_FRAME_PROPERTY, NULL);
			CCD_BIN_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_BIN_PROPERTY, NULL);
			CCD_MODE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_MODE_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(CCD_BIN_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_BIN
		indigo_property_copy_values(CCD_BIN_PROPERTY, property, false);
		CCD_BIN_PROPERTY->state = INDIGO_OK_STATE;
		int horizontal_bin = (int)CCD_BIN_HORIZONTAL_ITEM->number.value;
		int vertical_bin = (int)CCD_BIN_VERTICAL_ITEM->number.value;
		char name[32] = "";
		for (int i = 0; i < PIXEL_FORMAT_PROPERTY->count; i++) {
			if (PIXEL_FORMAT_PROPERTY->items[i].sw.value) {
				snprintf(name, 32, "%s %dx%d", PIXEL_FORMAT_PROPERTY->items[i].name, horizontal_bin, vertical_bin);
				break;
			}
		}
		for (int i = 0; i < CCD_MODE_PROPERTY->count; i++) {
			indigo_item *item = &CCD_MODE_PROPERTY->items[i];
			item->sw.value = !strcmp(item->name, name);
		}
		CCD_MODE_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_CONNECTED) {
			indigo_update_property(device, CCD_MODE_PROPERTY, NULL);
			indigo_update_property(device, CCD_BIN_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, PIXEL_FORMAT_PROPERTY);
			indigo_save_property(device, NULL, ASI_PRESETS_PROPERTY);
			indigo_save_property(device, NULL, ASI_ADVANCED_PROPERTY);
		}
	}
	return indigo_ccd_change_property(device, client, property);
}

static indigo_result ccd_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		handle_ccd_connect_property(device);
	}

	if (device == device->master_device)
		indigo_global_unlock(device);

	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);

	indigo_release_property(PIXEL_FORMAT_PROPERTY);
	indigo_release_property(ASI_PRESETS_PROPERTY);
	indigo_release_property(ASI_ADVANCED_PROPERTY);

	return indigo_ccd_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO guider device implementation

static indigo_result guider_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_guider_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		INFO_PROPERTY->count = 5;
		indigo_copy_value(INFO_DEVICE_MODEL_ITEM->text.value, PRIVATE_DATA->info.Name);
		return indigo_guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}


static void handle_guider_connection_property(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (!device->is_connected) {
			if (asi_open(device)) {
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
				GUIDER_GUIDE_DEC_PROPERTY->hidden = false;
				GUIDER_GUIDE_RA_PROPERTY->hidden = false;
				device->is_connected = true;
			} else {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		}
	} else {
		if (device->is_connected) {
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->guider_timer_ra);
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->guider_timer_dec);
			asi_close(device);
			device->is_connected = false;
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	}
	indigo_guider_change_property(device, NULL, CONNECTION_PROPERTY);
}


static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	ASI_ERROR_CODE res;
	int id = PRIVATE_DATA->dev_id;

	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, handle_guider_connection_property, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_DEC
		indigo_property_copy_values(GUIDER_GUIDE_DEC_PROPERTY, property, false);
		indigo_cancel_timer(device, &PRIVATE_DATA->guider_timer_dec);
		int duration = GUIDER_GUIDE_NORTH_ITEM->number.value;
		if (duration > 0) {
			pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
			res = ASIPulseGuideOn(id, ASI_GUIDE_NORTH);
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);

			if (res) INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASIPulseGuideOn(%d, ASI_GUIDE_NORTH) = %d", id, res);
			indigo_set_timer(device, duration/1000.0, guider_timer_callback_dec, &PRIVATE_DATA->guider_timer_dec);
			PRIVATE_DATA->guide_relays[ASI_GUIDE_NORTH] = true;
		} else {
			int duration = GUIDER_GUIDE_SOUTH_ITEM->number.value;
			if (duration > 0) {
				pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
				res = ASIPulseGuideOn(id, ASI_GUIDE_SOUTH);
				pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);

				if (res) INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASIPulseGuideOn(%d, ASI_GUIDE_SOUTH) = %d", id, res);
				indigo_set_timer(device, duration/1000.0, guider_timer_callback_dec, &PRIVATE_DATA->guider_timer_dec);
				PRIVATE_DATA->guide_relays[ASI_GUIDE_SOUTH] = true;
			}
		}

		if (PRIVATE_DATA->guide_relays[ASI_GUIDE_SOUTH] || PRIVATE_DATA->guide_relays[ASI_GUIDE_NORTH])
			GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
		else
			GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;

		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDER_GUIDE_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_RA
		indigo_property_copy_values(GUIDER_GUIDE_RA_PROPERTY, property, false);
		indigo_cancel_timer(device, &PRIVATE_DATA->guider_timer_ra);
		int duration = GUIDER_GUIDE_EAST_ITEM->number.value;
		if (duration > 0) {
			pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
			res = ASIPulseGuideOn(id, ASI_GUIDE_EAST);
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);

			if (res) INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASIPulseGuideOn(%d, ASI_GUIDE_EAST) = %d", id, res);
			indigo_set_timer(device, duration/1000.0, guider_timer_callback_ra, &PRIVATE_DATA->guider_timer_ra);
			PRIVATE_DATA->guide_relays[ASI_GUIDE_EAST] = true;
		} else {
			int duration = GUIDER_GUIDE_WEST_ITEM->number.value;
			if (duration > 0) {
				pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
				res = ASIPulseGuideOn(id, ASI_GUIDE_WEST);
				pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);

				if (res) INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASIPulseGuideOn(%d, ASI_GUIDE_WEST) = %d", id, res);
				indigo_set_timer(device, duration/1000.0, guider_timer_callback_ra, &PRIVATE_DATA->guider_timer_ra);
				PRIVATE_DATA->guide_relays[ASI_GUIDE_WEST] = true;
			}
		}

		if (PRIVATE_DATA->guide_relays[ASI_GUIDE_EAST] || PRIVATE_DATA->guide_relays[ASI_GUIDE_WEST])
			GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
		else
			GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;

		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_guider_change_property(device, client, property);
}

static indigo_result guider_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		handle_guider_connection_property(device);
	}

	if (device == device->master_device)
		indigo_global_unlock(device);

	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_guider_detach(device);
}

// -------------------------------------------------------------------------------- hot-plug support

static pthread_mutex_t device_mutex = PTHREAD_MUTEX_INITIALIZER;

#define MAX_DEVICES                   12
#define NO_DEVICE                 (-1000)


static int asi_products[ASICAMERA_ID_MAX];
static int asi_id_count = 0;

static indigo_device *devices[MAX_DEVICES] = {NULL};
static bool connected_ids[ASICAMERA_ID_MAX] = {false};


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

static void process_plug_event(indigo_device *unused) {
	ASI_CAMERA_INFO info;
	static indigo_device ccd_template = INDIGO_DEVICE_INITIALIZER(
		"",
		ccd_attach,
		asi_enumerate_properties,
		ccd_change_property,
		NULL,
		ccd_detach
		);
	static indigo_device guider_template = INDIGO_DEVICE_INITIALIZER(
		"",
		guider_attach,
		indigo_guider_enumerate_properties,
		guider_change_property,
		NULL,
		guider_detach
		);
	pthread_mutex_lock(&device_mutex);
	int slot = find_available_device_slot();
	if (slot < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "No device slots available.");
		pthread_mutex_unlock(&device_mutex);
		return;
	}

	int id = find_plugged_device_id();
	if (id == NO_DEVICE) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "No plugged device found.");
		pthread_mutex_unlock(&device_mutex);
		return;
	}

	indigo_device *device = indigo_safe_malloc_copy(sizeof(indigo_device), &ccd_template);
	indigo_device *master_device = device;
	int index = find_index_by_device_id(id);
	if (index < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "No index of plugged device found.");
		pthread_mutex_unlock(&device_mutex);
		return;
	}
	ASIGetCameraProperty(&info, index);

	/* If more than one camera of the same type is connected
	   SDK adds (CAM%d) to the name, we do not need it! */
	char *p = strstr(info.Name, "(CAM");
	if (p != NULL) *p = '\0';

	device->master_device = master_device;
	sprintf(device->name, "%s #%d", info.Name, id);
	INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
	asi_private_data *private_data = indigo_safe_malloc(sizeof(asi_private_data));
	private_data->dev_id = id;
	memcpy(&(private_data->info), &info, sizeof(ASI_CAMERA_INFO));
	device->private_data = private_data;
	indigo_attach_device(device);
	devices[slot]=device;
	if (info.ST4Port) {
		slot = find_available_device_slot();
		if (slot < 0) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "No device slots available.");
			pthread_mutex_unlock(&device_mutex);
			return;
		}
		device = indigo_safe_malloc_copy(sizeof(indigo_device), &guider_template);
		device->master_device = master_device;
		sprintf(device->name, "%s Guider #%d", info.Name, id);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		device->private_data = private_data;
		indigo_attach_device(device);
		devices[slot]=device;
	}
	pthread_mutex_unlock(&device_mutex);
}


static void process_unplug_event(indigo_device *unused) {
	pthread_mutex_lock(&device_mutex);
	int id, slot;
	bool removed = false;
	asi_private_data *private_data = NULL;
	while ((id = find_unplugged_device_id()) != -1) {
		slot = find_device_slot(id);
		while (slot >= 0) {
			indigo_device **device = &devices[slot];
			if (*device == NULL) {
				pthread_mutex_unlock(&device_mutex);
				return;
			}
			indigo_detach_device(*device);
			if ((*device)->private_data) {
				private_data = (*device)->private_data;
			}
			free(*device);
			*device = NULL;
			removed = true;
			slot = find_device_slot(id);
		}

		if (private_data) {
			ASICloseCamera(id);
			if (private_data->buffer != NULL) {
				free(private_data->buffer);
				private_data->buffer = NULL;
			}
			free(private_data);
			private_data = NULL;
		}
	}
	if (!removed) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "No ASI Camera unplugged (maybe EFW wheel)!");
	}
	pthread_mutex_unlock(&device_mutex);
}

static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	struct libusb_device_descriptor descriptor;
	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			libusb_get_device_descriptor(dev, &descriptor);
			for (int i = 0; i < asi_id_count; i++) {
				if (descriptor.idVendor != ASI_VENDOR_ID || asi_products[i] != descriptor.idProduct)
					continue;
				indigo_set_timer(NULL, 2.0, process_plug_event, NULL);
			}
			break;
		}
		case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: {
			indigo_set_timer(NULL, 0.5, process_unplug_event, NULL);
			break;
		}
	}
	return 0;
};


static void remove_all_devices() {
	int i;
	asi_private_data *pds[ASICAMERA_ID_MAX] = {NULL};

	for(i = 0; i < MAX_DEVICES; i++) {
		indigo_device *device = devices[i];
		if (device == NULL) continue;
		if (PRIVATE_DATA) pds[PRIVATE_DATA->dev_id] = PRIVATE_DATA; /* preserve pointers to private data */
		indigo_detach_device(device);
		free(device);
		devices[i] = NULL;
	}

	/* free private data */
	for(i = 0; i < ASICAMERA_ID_MAX; i++) {
		if (pds[i]) {
			if (pds[i]->buffer != NULL) {
				ASICloseCamera(pds[i]->dev_id);
				free(pds[i]->buffer);
				pds[i]->buffer = NULL;
			}
			free(pds[i]);
		}
	}

	for(i = 0; i < ASICAMERA_ID_MAX; i++)
		connected_ids[i] = false;
}


static libusb_hotplug_callback_handle callback_handle;

indigo_result indigo_ccd_asi(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "ZWO ASI Camera", __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;

			char *sdk_version = ASIGetSDKVersion();
			INDIGO_DRIVER_LOG(DRIVER_NAME, "ASI SDK v. %s", sdk_version);

			asi_id_count = ASIGetProductIDs(asi_products);
			if (asi_id_count <= 0) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Can not get the list of supported product IDs.");
				return INDIGO_FAILED;
			}
			indigo_start_usb_event_handler();
			int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, ASI_VENDOR_ID, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_register_callback ->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
			return rc >= 0 ? INDIGO_OK : INDIGO_FAILED;

		case INDIGO_DRIVER_SHUTDOWN:
			for (int i = 0; i < MAX_DEVICES; i++) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "devices[%d] = %p", i, devices[i]);
				VERIFY_NOT_CONNECTED(devices[i]);
				//if (devices[i] && devices[i]->is_connected > 0) return INDIGO_BUSY;
			}
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


#else

indigo_result indigo_ccd_asi(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "ZWO ASI Camera", __FUNCTION__, DRIVER_VERSION, true, last_action);

	switch(action) {
		case INDIGO_DRIVER_INIT:
		case INDIGO_DRIVER_SHUTDOWN:
			return INDIGO_UNSUPPORTED_ARCH;
		case INDIGO_DRIVER_INFO:
			break;
	}
	return INDIGO_OK;
}

#endif
