// Copyright (c) 2021 CloudMakers, s. r. o.
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
// 2.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu> (refactored from ASI driver by Rumen G. Bogdanovski)


/** INDIGO SVBONY CCD driver
 \file indigo_ccd_svb.c
 */

#define DRIVER_VERSION 0x0005
#define DRIVER_NAME "indigo_ccd_svb"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>

#include <indigo/indigo_driver_xml.h>

#include "indigo_ccd_svb.h"

#if !(defined(__APPLE__) && defined(__arm64__))

#if defined(INDIGO_MACOS)
#include <libusb-1.0/libusb.h>
#elif defined(INDIGO_FREEBSD)
#include <libusb.h>
#else
#include <libusb-1.0/libusb.h>
#endif

#include "SVBCameraSDK.h"

#define SVB_DEFAULT_BANDWIDTH      45

#define SVB_MAX_FORMATS            4

#define SVB_VENDOR_ID              0xf266

#define CCD_ADVANCED_GROUP         "Advanced"

#define PRIVATE_DATA               ((svb_private_data *)device->private_data)

#define PIXEL_FORMAT_PROPERTY      (PRIVATE_DATA->pixel_format_property)
#define RAW8_NAME                  "RAW 8"
#define RGB24_NAME                 "RGB 24"
#define RAW16_NAME                 "RAW 16"
#define Y8_NAME                    "Y 8"

#define SVB_ADVANCED_PROPERTY      (PRIVATE_DATA->svb_advanced_property)

// gp_bits is used as boolean
#define is_connected               gp_bits

// -------------------------------------------------------------------------------- SVBONY USB interface implementation

#define us2s(s) ((s) / 1000000.0)
#define s2us(us) ((us) * 1000000)

#define RETRY_COUNT	5

typedef struct {
	int dev_id;
	int count_open;
	int exp_bin_x, exp_bin_y;
	int exp_frame_width, exp_frame_height;
	int exp_bpp;
	indigo_timer *exposure_timer, *temperature_timer, *guider_timer_ra, *guider_timer_dec;
	double target_temperature, current_temperature;
	long cooler_power;
	unsigned char *buffer;
	long int buffer_size;
	pthread_mutex_t usb_mutex;
	bool can_check_temperature, has_temperature_sensor;
	SVB_CAMERA_INFO info;
	SVB_CAMERA_PROPERTY property;
	SVB_CAMERA_PROPERTY_EX property_ex;
	indigo_property *pixel_format_property;
	indigo_property *svb_advanced_property;
	bool first_frame;
	int retry;
} svb_private_data;

static char *get_bayer_string(indigo_device *device) {
	if (!PRIVATE_DATA->property.IsColorCam)
		return NULL;
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
		if (PIXEL_FORMAT_PROPERTY->items[item].sw.value) {
			if (!strcmp(PIXEL_FORMAT_PROPERTY->items[item].name, RAW8_NAME))
				return 8;
			if (!strcmp(PIXEL_FORMAT_PROPERTY->items[item].name, RGB24_NAME))
				return 24;
			if (!strcmp(PIXEL_FORMAT_PROPERTY->items[item].name, RAW16_NAME))
				return 16;
			if (!strcmp(PIXEL_FORMAT_PROPERTY->items[item].name, Y8_NAME))
				return 8;
		}
		item++;
	}
	return 8;
}


static int get_pixel_format(indigo_device *device) {
	int item = 0;
	while (item < SVB_MAX_FORMATS) {
		if (PIXEL_FORMAT_PROPERTY->items[item].sw.value) {
			if (!strcmp(PIXEL_FORMAT_PROPERTY->items[item].name, RAW8_NAME))
				return SVB_IMG_RAW8;
			if (!strcmp(PIXEL_FORMAT_PROPERTY->items[item].name, RGB24_NAME))
				return SVB_IMG_RGB24;
			if (!strcmp(PIXEL_FORMAT_PROPERTY->items[item].name, RAW16_NAME))
				return SVB_IMG_RAW16;
			if (!strcmp(PIXEL_FORMAT_PROPERTY->items[item].name, Y8_NAME))
				return SVB_IMG_Y8;
		}
		item++;
	}
	return SVB_IMG_END;
}


static bool pixel_format_supported(indigo_device *device, SVB_IMG_TYPE type) {
	for (int i = 0; i < SVB_MAX_FORMATS; i++) {
		if (i == SVB_IMG_END)
			return false;
		if (type == PRIVATE_DATA->property.SupportedVideoFormat[i])
			return true;
	}
	return false;
}


static indigo_result svb_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(PIXEL_FORMAT_PROPERTY, property))
			indigo_define_property(device, PIXEL_FORMAT_PROPERTY, NULL);
		if (indigo_property_match(SVB_ADVANCED_PROPERTY, property))
			indigo_define_property(device, SVB_ADVANCED_PROPERTY, NULL);
	}
	return indigo_ccd_enumerate_properties(device, NULL, NULL);
}


static bool svb_open(indigo_device *device) {
	int id = PRIVATE_DATA->dev_id;
	SVB_ERROR_CODE res;

	if (device->is_connected)
		return false;

	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	if (PRIVATE_DATA->count_open++ == 0) {
		if (indigo_try_global_lock(device) != INDIGO_OK) {
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_try_global_lock(): failed to get lock.");
			PRIVATE_DATA->count_open--;
			return false;
		}
		res = SVBOpenCamera(id);
		if (res) {
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "SVBOpenCamera(%d) = %d", id, res);
			PRIVATE_DATA->count_open--;
			return false;
		}
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "SVBOpenCamera(%d) = %d", id, res);
		SVBStopVideoCapture(id);
		if (PRIVATE_DATA->buffer == NULL) {
			if (PRIVATE_DATA->property.IsColorCam)
				PRIVATE_DATA->buffer_size = PRIVATE_DATA->property.MaxHeight * PRIVATE_DATA->property.MaxWidth * 3 + FITS_HEADER_SIZE + 1024;
			else
				PRIVATE_DATA->buffer_size = PRIVATE_DATA->property.MaxHeight * PRIVATE_DATA->property.MaxWidth * 2 + FITS_HEADER_SIZE + 1024;
			PRIVATE_DATA->buffer = (unsigned char*)indigo_alloc_blob_buffer(PRIVATE_DATA->buffer_size);
		}
	}
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	return true;
}

static bool svb_setup_exposure(indigo_device *device, double exposure, int frame_left, int frame_top, int frame_width, int frame_height, int horizontal_bin, int vertical_bin) {
	int id = PRIVATE_DATA->dev_id;
	SVB_ERROR_CODE res;
	int c_frame_left, c_frame_top, c_frame_width, c_frame_height, c_bin;
	long c_exposure;
	SVB_IMG_TYPE c_pixel_format;
	SVB_BOOL pauto;

	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	res = SVBGetOutputImageType(id, &c_pixel_format);
	if (res)
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "SVBGetOutputImageType(%d) = %d", id, res);
	if (c_pixel_format != get_pixel_format(device) || PRIVATE_DATA->first_frame) {
		res = SVBSetOutputImageType(id, get_pixel_format(device));
		if (res) {
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "SVBSetOutputImageType(%d) = %d", id, res);
			return false;
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "SVBSetOutputImageType(%d) = %d", id, res);
		}
		PRIVATE_DATA->first_frame = false;
	}
	res = SVBGetROIFormat(id, &c_frame_left, &c_frame_top, &c_frame_width, &c_frame_height, &c_bin);
	if (res)
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "SVBGetROIFormat(%d) = %d", id, res);
	if (c_frame_left != frame_left / horizontal_bin || c_frame_top != frame_top / vertical_bin || c_frame_width != frame_width / horizontal_bin || c_frame_height != frame_height / vertical_bin || c_bin != horizontal_bin) {
		res = SVBSetROIFormat(id, frame_left / horizontal_bin, frame_top / vertical_bin, frame_width / horizontal_bin, frame_height / vertical_bin,  horizontal_bin);
		if (res) {
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "SVBSetROIFormat(%d) = %d", id, res);
			return false;
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "SVBSetROIFormat(%d) = %d", id, res);
		}
	}
	res = SVBGetControlValue(id, SVB_EXPOSURE, &c_exposure, &pauto);
	if (res)
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "SVBGetControlValue(%d, SVB_EXPOSURE) = %d", id, res);
	if (c_exposure != (long)s2us(exposure)) {
		res = SVBSetControlValue(id, SVB_EXPOSURE, (long)s2us(exposure), SVB_FALSE);
		if (res) {
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "SVBSetControlValue(%d, SVB_EXPOSURE) = %d", id, res);
			return false;
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "SVBSetControlValue(%d, SVB_EXPOSURE) = %d", id, res);
		}
	}

	PRIVATE_DATA->exp_bin_x = horizontal_bin;
	PRIVATE_DATA->exp_bin_y = vertical_bin;
	PRIVATE_DATA->exp_frame_width = frame_width;
	PRIVATE_DATA->exp_frame_height = frame_height;
	PRIVATE_DATA->exp_bpp = (int)CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value;
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	return true;
}

static bool svb_set_cooler(indigo_device *device, bool status, double target, double *current, long *cooler_power) {
	SVB_ERROR_CODE res;
	SVB_BOOL unused;

	int id = PRIVATE_DATA->dev_id;
	long current_status;
	long temp_x10;

	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);

	if (PRIVATE_DATA->has_temperature_sensor) {
		res = SVBGetControlValue(id, SVB_CURRENT_TEMPERATURE, &temp_x10, &unused);
		if (res)
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "SVBGetControlValue(%d, SVB_CURRENT_TEMPERATURE) = %d", id, res);
		else
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "SVBGetControlValue(%d, SVB_CURRENT_TEMPERATURE) = %d", id, res);
		*current = temp_x10 / 10.0; /* SVB_CURRENT_TEMPERATURE gives temp x 10 */
	} else {
		*current = 0;
	}

	if (!PRIVATE_DATA->property_ex.bSupportControlTemp) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		return true;
	}

	res = SVBGetControlValue(id, SVB_COOLER_ENABLE, &current_status, &unused);
	if (res) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "SVBGetControlValue(%d, SVB_COOLER_ENABLE) = %d", id, res);
		return false;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "SVBGetControlValue(%d, SVB_COOLER_ENABLE) = %d", id, res);

	if (current_status != status) {
		res = SVBSetControlValue(id, SVB_COOLER_ENABLE, status, false);
		if (res)
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "SVBSetControlValue(%d, SVB_COOLER_ENABLE) = %d", id, res);
		else
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "SVBSetControlValue(%d, SVB_COOLER_ENABLE) = %d", id, res);
	} else if (status) {
		res = SVBSetControlValue(id, SVB_TARGET_TEMPERATURE, (long)target, false);
		if (res)
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "SVBSetControlValue(%d, SVB_TARGET_TEMPERATURE) = %d", id, res);
		else
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "SVBSetControlValue(%d, SVB_TARGET_TEMPERATURE) = %d", id, res);
	}

	res = SVBGetControlValue(id, SVB_COOLER_POWER, cooler_power, &unused);
	if (res)
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "SVBGetControlValue(%d, SVB_COOLER_POWER) = %d", id, res);
	else
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "SVBGetControlValue(%d, SVB_COOLER_POWER) = %d", id, res);

	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	return true;
}

static void svb_close(indigo_device *device) {
	if (!device->is_connected)
		return;
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	if (--PRIVATE_DATA->count_open == 0) {
		SVBCloseCamera(PRIVATE_DATA->dev_id);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "SVBCloseCamera(%d)", PRIVATE_DATA->dev_id);
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
	if (!CONNECTION_CONNECTED_ITEM->sw.value)
		return;
	char *color_string = get_bayer_string(device);
	indigo_fits_keyword keywords[] = {
		{ INDIGO_FITS_STRING, "BAYERPAT", .string = color_string, "Bayer color pattern" },
		{ INDIGO_FITS_NUMBER, "XBAYROFF", .number = 0, "X offset of Bayer array" },
		{ INDIGO_FITS_NUMBER, "YBAYROFF", .number = 0, "Y offset of Bayer array" },
		{ 0 }
	};
	int id = PRIVATE_DATA->dev_id;
	SVB_ERROR_CODE res;
	PRIVATE_DATA->can_check_temperature = false;
	if (svb_setup_exposure(device, CCD_EXPOSURE_ITEM->number.target, CCD_FRAME_LEFT_ITEM->number.value, CCD_FRAME_TOP_ITEM->number.value, CCD_FRAME_WIDTH_ITEM->number.value, CCD_FRAME_HEIGHT_ITEM->number.value, CCD_BIN_HORIZONTAL_ITEM->number.value, CCD_BIN_VERTICAL_ITEM->number.value)) {
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		res = SVBSetCameraMode(id, SVB_MODE_TRIG_SOFT);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "SVBSetCameraMode(%d) = %d", id, res);
		} else {
			while (true) {
				if (SVBGetVideoData(id, PRIVATE_DATA->buffer + FITS_HEADER_SIZE, PRIVATE_DATA->buffer_size, 0) != SVB_SUCCESS)
					break;
			}
			res = SVBStartVideoCapture(id);
		}
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "SVBStartVideoCapture(%d) = %d", id, res);
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "SVBStartVideoCapture(%d) = %d", id, res);
			pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
			res = SVBSendSoftTrigger(id);
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			if (res) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "SVBSendSoftTrigger((%d) = %d", id, res);
			} else {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "SVBSendSoftTrigger((%d) = %d", id, res);
				time_t start = time(NULL);
				while (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
					pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
					res = SVBGetVideoData(id, PRIVATE_DATA->buffer + FITS_HEADER_SIZE, PRIVATE_DATA->buffer_size, 100);
					pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
					double remaining = CCD_EXPOSURE_ITEM->number.target - (time(NULL) - start);
					if (res == SVB_SUCCESS) {
						if (remaining > 0 && CCD_EXPOSURE_ITEM->number.target > 1) {
							if (PRIVATE_DATA->retry == 0) {
								indigo_send_message(device, "Exposure was retried %d times, failed", RETRY_COUNT);
								CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
								break;
							}
							PRIVATE_DATA->retry--;
							CCD_EXPOSURE_ITEM->number.value = CCD_EXPOSURE_ITEM->number.target;
							indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Exposure is terminated prematurely, retrying...");
							pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
							SVBStopVideoCapture(id);
							pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
							indigo_set_timer(device, 0, exposure_timer_callback, &PRIVATE_DATA->exposure_timer);
							return;
						}
						break;
					}
					if (res != SVB_ERROR_TIMEOUT) {
						CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
						break;
					}
					if (remaining < -0.1 * CCD_EXPOSURE_ITEM->number.target) {
						PRIVATE_DATA->retry--;
						CCD_EXPOSURE_ITEM->number.value = CCD_EXPOSURE_ITEM->number.target;
						indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Exposure is not finished on time, retrying...");
						pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
						SVBStopVideoCapture(id);
						pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
						indigo_set_timer(device, 0, exposure_timer_callback, &PRIVATE_DATA->exposure_timer);
						return;
					}
					if (CCD_EXPOSURE_ITEM->number.value > remaining && remaining >= 0) {
						CCD_EXPOSURE_ITEM->number.value = remaining;
						indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
					}
				}
				if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
					CCD_EXPOSURE_ITEM->number.value = 0;
					if (res) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "SVBGetVideoData((%d) = %d", id, res);
					} else {
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "SVBGetVideoData((%d) = %d", id, res);
						if ((color_string) &&   /* if colour (bayer) image but not RGB */
								(PRIVATE_DATA->exp_bpp != 24) &&
								(PRIVATE_DATA->exp_bpp != 48)) {
							indigo_process_image(device, PRIVATE_DATA->buffer, (int)(PRIVATE_DATA->exp_frame_width / PRIVATE_DATA->exp_bin_x), (int)(PRIVATE_DATA->exp_frame_height / PRIVATE_DATA->exp_bin_y), PRIVATE_DATA->exp_bpp, true, false, keywords, true);
						} else {
							indigo_process_image(device, PRIVATE_DATA->buffer, (int)(PRIVATE_DATA->exp_frame_width / PRIVATE_DATA->exp_bin_x), (int)(PRIVATE_DATA->exp_frame_height / PRIVATE_DATA->exp_bin_y), PRIVATE_DATA->exp_bpp, true, false, NULL, true);
						}
					}
				}
			}
			pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
			SVBStopVideoCapture(id);
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		}
	} else {
		res = SVB_ERROR_GENERAL_ERROR;
	}
	PRIVATE_DATA->can_check_temperature = true;
	indigo_finalize_video_stream(device);
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		if (res)
			CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
		else
			CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
	PRIVATE_DATA->can_check_temperature = true;
}

static void streaming_timer_callback(indigo_device *device) {
	if (!CONNECTION_CONNECTED_ITEM->sw.value)
		return;
	char *color_string = get_bayer_string(device);
	indigo_fits_keyword keywords[] = {
		{ INDIGO_FITS_STRING, "BAYERPAT", .string = color_string, "Bayer color pattern" },
		{ INDIGO_FITS_NUMBER, "XBAYROFF", .number = 0, "X offset of Bayer array" },
		{ INDIGO_FITS_NUMBER, "YBAYROFF", .number = 0, "Y offset of Bayer array" },
		{ 0 }
	};
	int id = PRIVATE_DATA->dev_id;
	SVB_ERROR_CODE res;
	PRIVATE_DATA->can_check_temperature = false;
	if (svb_setup_exposure(device, CCD_STREAMING_EXPOSURE_ITEM->number.target, CCD_FRAME_LEFT_ITEM->number.value, CCD_FRAME_TOP_ITEM->number.value, CCD_FRAME_WIDTH_ITEM->number.value, CCD_FRAME_HEIGHT_ITEM->number.value, CCD_BIN_HORIZONTAL_ITEM->number.value, CCD_BIN_VERTICAL_ITEM->number.value)) {
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		res = SVBSetCameraMode(id, SVB_MODE_NORMAL);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "SVBSetCameraMode(%d) = %d", id, res);
		} else {
			while (true) {
				if (SVBGetVideoData(id, PRIVATE_DATA->buffer + FITS_HEADER_SIZE, PRIVATE_DATA->buffer_size, 0) != SVB_SUCCESS)
					break;
			}
			res = SVBStartVideoCapture(id);
		}
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "SVBStartVideoCapture(%d) = %d", id, res);
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "SVBStartVideoCapture(%d) = %d", id, res);
			while (CCD_STREAMING_COUNT_ITEM->number.value != 0) {
				time_t start = time(NULL);
				while (CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
					pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
					res = SVBGetVideoData(id, PRIVATE_DATA->buffer + FITS_HEADER_SIZE, PRIVATE_DATA->buffer_size, 100);
					pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
					double remaining = CCD_STREAMING_EXPOSURE_ITEM->number.target - (time(NULL) - start);
					if (res == SVB_SUCCESS) {
						if (remaining > 0 && CCD_STREAMING_EXPOSURE_ITEM->number.target) {
							if (PRIVATE_DATA->retry == 0) {
								indigo_send_message(device, "Exposure was retried %d times, failed", RETRY_COUNT);
								CCD_STREAMING_PROPERTY->state = INDIGO_ALERT_STATE;
								break;
							}
							PRIVATE_DATA->retry--;
							pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
							SVBStopVideoCapture(id);
							pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
							CCD_STREAMING_EXPOSURE_ITEM->number.value = CCD_STREAMING_EXPOSURE_ITEM->number.target;
							indigo_update_property(device, CCD_STREAMING_PROPERTY, "Exposure is terminated prematurely, retrying...");
							indigo_set_timer(device, 0, streaming_timer_callback, &PRIVATE_DATA->exposure_timer);
							return;
						}
						break;
					}
					if (res != SVB_ERROR_TIMEOUT) {
						CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
						break;
					}
					if (remaining < -0.1 * CCD_STREAMING_EXPOSURE_ITEM->number.target) {
						PRIVATE_DATA->retry--;
						CCD_STREAMING_EXPOSURE_ITEM->number.value = CCD_STREAMING_EXPOSURE_ITEM->number.target;
						indigo_update_property(device, CCD_STREAMING_PROPERTY, "Exposure is not finished on time, retrying...");
						pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
						SVBStopVideoCapture(id);
						pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
						indigo_set_timer(device, 0, exposure_timer_callback, &PRIVATE_DATA->exposure_timer);
						return;
					}
					if (CCD_STREAMING_EXPOSURE_ITEM->number.value > remaining && remaining >= 0) {
						CCD_STREAMING_EXPOSURE_ITEM->number.value = remaining;
						indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
					}
				}
				CCD_STREAMING_EXPOSURE_ITEM->number.value = 0;
				indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
				if (res) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "SVBGetVideoData((%d) = %d", id, res);
					break;
				}
				if (CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "SVBGetVideoData((%d) = %d", id, res);
					if ((color_string) &&   /* if colour (bayer) image but not RGB */
							(PRIVATE_DATA->exp_bpp != 24) &&
							(PRIVATE_DATA->exp_bpp != 48)) {
						indigo_process_image(device, PRIVATE_DATA->buffer, (int)(PRIVATE_DATA->exp_frame_width / PRIVATE_DATA->exp_bin_x), (int)(PRIVATE_DATA->exp_frame_height / PRIVATE_DATA->exp_bin_y), PRIVATE_DATA->exp_bpp, true, false, keywords, true);
					} else {
						indigo_process_image(device, PRIVATE_DATA->buffer, (int)(PRIVATE_DATA->exp_frame_width / PRIVATE_DATA->exp_bin_x), (int)(PRIVATE_DATA->exp_frame_height / PRIVATE_DATA->exp_bin_y), PRIVATE_DATA->exp_bpp, true, false, NULL, true);
					}
					if (CCD_STREAMING_COUNT_ITEM->number.value > 0)
						CCD_STREAMING_COUNT_ITEM->number.value -= 1;
					CCD_STREAMING_EXPOSURE_ITEM->number.value = CCD_STREAMING_EXPOSURE_ITEM->number.target;
					CCD_STREAMING_PROPERTY->state = INDIGO_BUSY_STATE;
					indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
				}
			}
			pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
			SVBStopVideoCapture(id);
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		}
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	} else {
		res = SVB_ERROR_GENERAL_ERROR;
	}
	PRIVATE_DATA->can_check_temperature = true;
	indigo_finalize_video_stream(device);
	if (CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
		if (res)
			CCD_STREAMING_PROPERTY->state = INDIGO_ALERT_STATE;
		else
			CCD_STREAMING_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
}

static void ccd_temperature_callback(indigo_device *device) {
	if (!CONNECTION_CONNECTED_ITEM->sw.value) return;
	if (PRIVATE_DATA->can_check_temperature) {
		if (svb_set_cooler(device, CCD_COOLER_ON_ITEM->sw.value, PRIVATE_DATA->target_temperature, &PRIVATE_DATA->current_temperature, &PRIVATE_DATA->cooler_power)) {
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

	if (!CONNECTION_CONNECTED_ITEM->sw.value)
		return;

	GUIDER_GUIDE_EAST_ITEM->number.value = 0;
	GUIDER_GUIDE_WEST_ITEM->number.value = 0;
	GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
}


static void guider_timer_callback_dec(indigo_device *device) {
	PRIVATE_DATA->guider_timer_dec = NULL;

	if (!CONNECTION_CONNECTED_ITEM->sw.value)
		return;

	GUIDER_GUIDE_NORTH_ITEM->number.value = 0;
	GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
	GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
}


static indigo_result ccd_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_ccd_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		pthread_mutex_init(&PRIVATE_DATA->usb_mutex, NULL);
		// -------------------------------------------------------------------------------- PIXEL_FORMAT_PROPERTY
		PIXEL_FORMAT_PROPERTY = indigo_init_switch_property(NULL, device->name, "PIXEL_FORMAT", CCD_ADVANCED_GROUP, "Pixel Format", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, SVB_MAX_FORMATS);
		if (PIXEL_FORMAT_PROPERTY == NULL)
			return INDIGO_FAILED;

		int format_count = 0;
		if (pixel_format_supported(device, SVB_IMG_RAW8)) {
			indigo_init_switch_item(PIXEL_FORMAT_PROPERTY->items + format_count, RAW8_NAME, RAW8_NAME, true);
			format_count++;
		}
		if (pixel_format_supported(device, SVB_IMG_RGB24)) {
			indigo_init_switch_item(PIXEL_FORMAT_PROPERTY->items + format_count, RGB24_NAME, RGB24_NAME, false);
			format_count++;
		}
		if (pixel_format_supported(device, SVB_IMG_RAW16)) {
			indigo_init_switch_item(PIXEL_FORMAT_PROPERTY->items + format_count, RAW16_NAME, RAW16_NAME, false);
			format_count++;
		}
		if (pixel_format_supported(device, SVB_IMG_Y8)) {
			indigo_init_switch_item(PIXEL_FORMAT_PROPERTY->items + format_count, Y8_NAME, Y8_NAME, false);
			format_count++;
		}
		PIXEL_FORMAT_PROPERTY->count = format_count;

		INFO_PROPERTY->count = 6;
		indigo_copy_value(INFO_DEVICE_MODEL_ITEM->text.value, PRIVATE_DATA->info.FriendlyName);
		const char *sdk_version = SVBGetSDKVersion();
		indigo_copy_value(INFO_DEVICE_FW_REVISION_ITEM->text.value, sdk_version);
		indigo_copy_value(INFO_DEVICE_FW_REVISION_ITEM->label, "SDK version");

		CCD_INFO_WIDTH_ITEM->number.value = PRIVATE_DATA->property.MaxWidth;
		CCD_INFO_HEIGHT_ITEM->number.value = PRIVATE_DATA->property.MaxHeight;
		CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = PRIVATE_DATA->property.MaxBitDepth;

		CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.target = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max = PRIVATE_DATA->property.MaxWidth;
		CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.target = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max = PRIVATE_DATA->property.MaxHeight;
		CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.target = get_pixel_depth(device);
		CCD_FRAME_BITS_PER_PIXEL_ITEM->number.min = 8;
		CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = 24;

		/* find max binning */
		int max_bin = 1;
		for (int num = 0; (num < 16) && PRIVATE_DATA->property.SupportedBins[num]; num++) {
			max_bin = PRIVATE_DATA->property.SupportedBins[num];
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
		}
		CCD_MODE_PROPERTY->count = mode_count;
		// -------------------------------------------------------------------------------- CCD_STREAMING
		CCD_STREAMING_PROPERTY->hidden = false;
		CCD_IMAGE_FORMAT_PROPERTY->count = 7;
		// -------------------------------------------------------------------------------- SVB_ADVANCED
		SVB_ADVANCED_PROPERTY = indigo_init_number_property(NULL, device->name, "SVB_ADVANCED", CCD_ADVANCED_GROUP, "Advanced", INDIGO_OK_STATE, INDIGO_RW_PERM, 0);
		if (SVB_ADVANCED_PROPERTY == NULL)
			return INDIGO_FAILED;
		// --------------------------------------------------------------------------------
		return indigo_ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result handle_advanced_property(indigo_device *device, indigo_property *property) {
	int ctrl_count;
	SVB_CONTROL_CAPS ctrl_caps;
	SVB_ERROR_CODE res;
	int id = PRIVATE_DATA->dev_id;

	if (!IS_CONNECTED)
		return INDIGO_OK;

	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);

	res = SVBGetNumOfControls(id, &ctrl_count);
	if (res) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "SVBGetNumOfControls(%d) = %d", id, res);
		return INDIGO_NOT_FOUND;
	}
	SVB_BOOL unused;
	long value;
	for (int ctrl_no = 0; ctrl_no < ctrl_count; ctrl_no++) {
		SVBGetControlCaps(id, ctrl_no, &ctrl_caps);
		for (int item = 0; item < property->count; item++) {
			if (!strncmp(ctrl_caps.Name, property->items[item].name, INDIGO_NAME_SIZE)) {
				res = SVBSetControlValue(id, ctrl_caps.ControlType,property->items[item].number.value, SVB_FALSE);
				if (res)
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "SVBSetControlValue(%d, %s) = %d", id, ctrl_caps.Name, res);
				res = SVBGetControlValue(id, ctrl_caps.ControlType,&value, &unused);
				property->items[item].number.value = value;
				if (res)
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "SVBGetControlValue(%d, %s) = %d, value = %d", id, ctrl_caps.Name, res, property->items[item].number.value);
			}
		}
	}

	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	return INDIGO_OK;
}

static indigo_result init_camera_property(indigo_device *device, SVB_CONTROL_CAPS ctrl_caps) {
	int id = PRIVATE_DATA->dev_id;
	long value;
	SVB_ERROR_CODE res;
	SVB_BOOL unused;

	if (ctrl_caps.ControlType == SVB_EXPOSURE) {
		CCD_EXPOSURE_PROPERTY->hidden = false;
		if (ctrl_caps.IsWritable)
			CCD_EXPOSURE_PROPERTY->perm = INDIGO_RW_PERM;
		else
			CCD_EXPOSURE_PROPERTY->perm = INDIGO_RO_PERM;

		CCD_EXPOSURE_ITEM->number.min = CCD_STREAMING_EXPOSURE_ITEM->number.min = us2s(ctrl_caps.MinValue);
		CCD_EXPOSURE_ITEM->number.max = CCD_STREAMING_EXPOSURE_ITEM->number.max = us2s(ctrl_caps.MaxValue);
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		unused = false;
		res = SVBGetControlValue(id, SVB_EXPOSURE, &value, &unused);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if (res) INDIGO_DRIVER_ERROR(DRIVER_NAME, "SVBGetControlValue(%d, SVB_EXPOSURE) = %d", id, res);
		CCD_EXPOSURE_ITEM->number.value = CCD_EXPOSURE_ITEM->number.target = us2s(value);
		return INDIGO_OK;
	}

	if (ctrl_caps.ControlType == SVB_BLACK_LEVEL) {
		CCD_OFFSET_PROPERTY->hidden = false;
		if (ctrl_caps.IsWritable)
			CCD_OFFSET_PROPERTY->perm = INDIGO_RW_PERM;
		else
			CCD_OFFSET_PROPERTY->perm = INDIGO_RO_PERM;

		CCD_OFFSET_ITEM->number.min = ctrl_caps.MinValue;
		CCD_OFFSET_ITEM->number.max = ctrl_caps.MaxValue;
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		unused = false;
		res = SVBGetControlValue(id, SVB_BLACK_LEVEL, &value, &unused);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if (res) INDIGO_DRIVER_ERROR(DRIVER_NAME, "SVBGetControlValue(%d, SVB_BLACK_LEVEL) = %d", id, res);
		CCD_OFFSET_ITEM->number.value = CCD_OFFSET_ITEM->number.target = value;
		CCD_OFFSET_ITEM->number.step = 1;
		return INDIGO_OK;
	}

	if (ctrl_caps.ControlType == SVB_GAIN) {
		CCD_GAIN_PROPERTY->hidden = false;
		if (ctrl_caps.IsWritable)
			CCD_GAIN_PROPERTY->perm = INDIGO_RW_PERM;
		else
			CCD_GAIN_PROPERTY->perm = INDIGO_RO_PERM;

		CCD_GAIN_ITEM->number.min = ctrl_caps.MinValue;
		CCD_GAIN_ITEM->number.max = ctrl_caps.MaxValue;
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		unused = false;
		res = SVBGetControlValue(id, SVB_GAIN, &value, &unused);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if (res) INDIGO_DRIVER_ERROR(DRIVER_NAME, "SVBGetControlValue(%d, SVB_GAIN) = %d", id, res);
		CCD_GAIN_ITEM->number.value = CCD_GAIN_ITEM->number.target = value;
		CCD_GAIN_ITEM->number.step = 1;
		return INDIGO_OK;
	}

	if (ctrl_caps.ControlType == SVB_GAMMA) {
		CCD_GAMMA_PROPERTY->hidden = false;
		if (ctrl_caps.IsWritable)
			CCD_GAMMA_PROPERTY->perm = INDIGO_RW_PERM;
		else
			CCD_GAMMA_PROPERTY->perm = INDIGO_RO_PERM;

		CCD_GAMMA_ITEM->number.min = ctrl_caps.MinValue;
		CCD_GAMMA_ITEM->number.max = ctrl_caps.MaxValue;
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		unused = false;
		res = SVBGetControlValue(id, SVB_GAMMA, &value, &unused);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if (res) INDIGO_DRIVER_ERROR(DRIVER_NAME, "SVBGetControlValue(%d, SVB_GAMMA) = %d", id, res);
		CCD_GAMMA_ITEM->number.value = CCD_GAMMA_ITEM->number.target = value;
		CCD_GAMMA_ITEM->number.step = 1;
		return INDIGO_OK;
	}

	if (ctrl_caps.ControlType == SVB_TARGET_TEMPERATURE) {
		CCD_TEMPERATURE_PROPERTY->hidden = false;
		if (ctrl_caps.IsWritable)
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
		CCD_COOLER_PROPERTY->hidden = false;
		if (ctrl_caps.IsWritable)
			CCD_COOLER_PROPERTY->perm = INDIGO_RW_PERM;
		else
			CCD_COOLER_PROPERTY->perm = INDIGO_RO_PERM;

		return INDIGO_OK;
	}

	if (ctrl_caps.ControlType == SVB_COOLER_POWER) {
		CCD_COOLER_POWER_PROPERTY->hidden = false;
		if (ctrl_caps.IsWritable)
			CCD_COOLER_POWER_PROPERTY->perm = INDIGO_RW_PERM;
		else
			CCD_COOLER_POWER_PROPERTY->perm = INDIGO_RO_PERM;

		CCD_COOLER_POWER_ITEM->number.min = ctrl_caps.MinValue;
		CCD_COOLER_POWER_ITEM->number.max = ctrl_caps.MaxValue;
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		res = SVBGetControlValue(id, SVB_COOLER_POWER, &value, &unused);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if (res)
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "SVBGetControlValue(%d, SVB_COOLER_POWER) = %d", id, res);
		CCD_COOLER_POWER_ITEM->number.value = CCD_COOLER_POWER_ITEM->number.target = value;
		return INDIGO_OK;
	}

	int offset = SVB_ADVANCED_PROPERTY->count;
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	unused = false;
	res = SVBGetControlValue(id, ctrl_caps.ControlType, &value, &unused);
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	if (res)
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "SVBGetControlValue(%d, %s) = %d", id, ctrl_caps.Name, res);

	SVB_ADVANCED_PROPERTY = indigo_resize_property(SVB_ADVANCED_PROPERTY, offset + 1);
	indigo_init_number_item(SVB_ADVANCED_PROPERTY->items+offset, ctrl_caps.Name, ctrl_caps.Name, ctrl_caps.MinValue, ctrl_caps.MaxValue, 1, value);
	return INDIGO_OK;
}

static void handle_ccd_connect_property(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (!device->is_connected) {
			if (svb_open(device)) {
				int id = PRIVATE_DATA->dev_id;
				int ctrl_count;
				SVB_CONTROL_CAPS ctrl_caps;

				indigo_define_property(device, PIXEL_FORMAT_PROPERTY, NULL);

				pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
				int res = SVBGetNumOfControls(id, &ctrl_count);
				pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
				if (res) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "SVBGetNumOfControls(%d) = %d", id, res);
				}
				SVB_ADVANCED_PROPERTY = indigo_resize_property(SVB_ADVANCED_PROPERTY, 0);
				for (int ctrl_no = 0; ctrl_no < ctrl_count; ctrl_no++) {
					pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
					SVBGetControlCaps(id, ctrl_no, &ctrl_caps);
					pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
					init_camera_property(device, ctrl_caps);
				}
				indigo_define_property(device, SVB_ADVANCED_PROPERTY, NULL);
				PRIVATE_DATA->first_frame = true;
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
		if (device->is_connected) {
			PRIVATE_DATA->can_check_temperature = false;
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->temperature_timer);
			if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
				CCD_ABORT_EXPOSURE_ITEM->sw.value = true;
				indigo_cancel_timer_sync(device, &PRIVATE_DATA->exposure_timer);
			} else if (CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE && CCD_STREAMING_COUNT_ITEM->number.value != 0) {
				CCD_STREAMING_COUNT_ITEM->number.value = 0;
				indigo_cancel_timer_sync(device, &PRIVATE_DATA->exposure_timer);
			}
			indigo_delete_property(device, PIXEL_FORMAT_PROPERTY, NULL);
			indigo_delete_property(device, SVB_ADVANCED_PROPERTY, NULL);
			svb_close(device);
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
		PRIVATE_DATA->retry = RETRY_COUNT;
		indigo_set_timer(device, 0, exposure_timer_callback, &PRIVATE_DATA->exposure_timer);
		return INDIGO_OK;
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
		PRIVATE_DATA->retry = RETRY_COUNT;
		indigo_set_timer(device, 0, streaming_timer_callback, &PRIVATE_DATA->exposure_timer);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- CCD_ABORT_EXPOSURE
//	} else if (indigo_property_match(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
//		if (CCD_ABORT_EXPOSURE_ITEM->sw.value && (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE)) {
//			CCD_ABORT_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
//			if (CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE && CCD_STREAMING_COUNT_ITEM->number.value != 0) {
//				CCD_STREAMING_COUNT_ITEM->number.value = 0;
//			}
//			PRIVATE_DATA->can_check_temperature = true;
//		}
//		indigo_property_copy_values(CCD_ABORT_EXPOSURE_PROPERTY, property, false);
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
		if (!IS_CONNECTED)
			return INDIGO_OK;
		CCD_GAMMA_PROPERTY->state = INDIGO_OK_STATE;
		indigo_property_copy_values(CCD_GAMMA_PROPERTY, property, false);

		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		SVB_ERROR_CODE res = SVBSetControlValue(PRIVATE_DATA->dev_id, SVB_GAMMA, (long)(CCD_GAMMA_ITEM->number.value), SVB_FALSE);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "SVBSetControlValue(%d, SVB_GAMMA) = %d", PRIVATE_DATA->dev_id, res);
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
		SVB_ERROR_CODE res = SVBSetControlValue(PRIVATE_DATA->dev_id, SVB_BLACK_LEVEL, (long)(CCD_OFFSET_ITEM->number.value), SVB_FALSE);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "SVBSetControlValue(%d, SVB_BLACK_LEVEL) = %d", PRIVATE_DATA->dev_id, res);
			CCD_OFFSET_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			CCD_OFFSET_PROPERTY->state = INDIGO_OK_STATE;
		}
		indigo_update_property(device, CCD_OFFSET_PROPERTY, NULL);
		return INDIGO_OK;
		// ------------------------------------------------------------------------------- CCD_GAIN
	} else if (indigo_property_match_w(CCD_GAIN_PROPERTY, property)) {
		if (!IS_CONNECTED) return INDIGO_OK;
		CCD_GAIN_PROPERTY->state = INDIGO_OK_STATE;
		indigo_property_copy_values(CCD_GAIN_PROPERTY, property, false);

		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		SVB_ERROR_CODE res = SVBSetControlValue(PRIVATE_DATA->dev_id, SVB_GAIN, (long)(CCD_GAIN_ITEM->number.value), SVB_FALSE);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "SVBSetControlValue(%d, SVB_GAIN) = %d", PRIVATE_DATA->dev_id, res);
			CCD_GAIN_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			CCD_GAIN_PROPERTY->state = INDIGO_OK_STATE;
		}
		indigo_update_property(device, CCD_GAIN_PROPERTY, NULL);
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
	} else if (indigo_property_match(SVB_ADVANCED_PROPERTY, property)) {
		if (!IS_CONNECTED) return INDIGO_OK;
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
			SVB_ADVANCED_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, SVB_ADVANCED_PROPERTY, "Exposure in progress, advanced settings can not be changed.");
			return INDIGO_OK;
		}
		handle_advanced_property(device, property);
		indigo_property_copy_values(SVB_ADVANCED_PROPERTY, property, false);
		SVB_ADVANCED_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, SVB_ADVANCED_PROPERTY, NULL);
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
			indigo_save_property(device, NULL, SVB_ADVANCED_PROPERTY);
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
	indigo_release_property(SVB_ADVANCED_PROPERTY);

	return indigo_ccd_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO guider device implementation

static indigo_result guider_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_guider_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		INFO_PROPERTY->count = 5;
		indigo_copy_value(INFO_DEVICE_MODEL_ITEM->text.value, PRIVATE_DATA->info.FriendlyName);
		return indigo_guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}


static void handle_guider_connection_property(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (!device->is_connected) {
			if (svb_open(device)) {
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
			svb_close(device);
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
	SVB_ERROR_CODE res;
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
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
		int duration = GUIDER_GUIDE_NORTH_ITEM->number.value;
		if (duration > 0) {
			pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
			res = SVBPulseGuide(id, SVB_GUIDE_NORTH, duration);
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			if (res)
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "SVBPulseGuideOn(%d, SVB_GUIDE_NORTH) = %d", id, res);
			indigo_set_timer(device, duration/1000.0, guider_timer_callback_dec, &PRIVATE_DATA->guider_timer_dec);
			GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
		} else {
			int duration = GUIDER_GUIDE_SOUTH_ITEM->number.value;
			if (duration > 0) {
				pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
				res = SVBPulseGuide(id, SVB_GUIDE_SOUTH, duration);
				pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
				if (res)
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "SVBPulseGuideOn(%d, SVB_GUIDE_SOUTH) = %d", id, res);
				indigo_set_timer(device, duration/1000.0, guider_timer_callback_dec, &PRIVATE_DATA->guider_timer_dec);
				GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
			}
		}
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDER_GUIDE_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_RA
		indigo_property_copy_values(GUIDER_GUIDE_RA_PROPERTY, property, false);
		indigo_cancel_timer(device, &PRIVATE_DATA->guider_timer_ra);
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
		int duration = GUIDER_GUIDE_EAST_ITEM->number.value;
		if (duration > 0) {
			pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
			res = SVBPulseGuide(id, SVB_GUIDE_EAST, duration);
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			if (res)
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "SVBPulseGuideOn(%d, SVB_GUIDE_EAST) = %d", id, res);
			indigo_set_timer(device, duration/1000.0, guider_timer_callback_ra, &PRIVATE_DATA->guider_timer_ra);
			GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
		} else {
			int duration = GUIDER_GUIDE_WEST_ITEM->number.value;
			if (duration > 0) {
				pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
				res = SVBPulseGuide(id, SVB_GUIDE_WEST, duration);
				pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
				if (res)
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "SVBPulseGuideOn(%d, SVB_GUIDE_WEST) = %d", id, res);
				indigo_set_timer(device, duration/1000.0, guider_timer_callback_ra, &PRIVATE_DATA->guider_timer_ra);
				GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
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


static indigo_device *devices[MAX_DEVICES] = {NULL};
static bool connected_ids[SVBCAMERA_ID_MAX] = {false};


static int find_index_by_device_id(int id) {
	SVB_CAMERA_INFO info;
	int count = SVBGetNumOfConnectedCameras();
	for (int index = 0; index < count; index++) {
		SVBGetCameraInfo(&info, index);
		if (info.CameraID == id) return index;
	}
	return -1;
}


static int find_plugged_device_id() {
	int i, id = NO_DEVICE, new_id = NO_DEVICE;
	SVB_CAMERA_INFO info;

	int count = SVBGetNumOfConnectedCameras();
	for (i = 0; i < count; i++) {
		SVBGetCameraInfo(&info, i);
		id = info.CameraID;
		if (!connected_ids[id]) {
			new_id = id;
			connected_ids[id] = true;
			break;
		}
	}
	return new_id;
}


static int find_available_device_slot() {
	for (int slot = 0; slot < MAX_DEVICES; slot++) {
		if (devices[slot] == NULL) return slot;
	}
	return -1;
}


static int find_device_slot(int id) {
	for (int slot = 0; slot < MAX_DEVICES; slot++) {
		indigo_device *device = devices[slot];
		if (device == NULL) continue;
		if (PRIVATE_DATA->dev_id == id) return slot;
	}
	return -1;
}


static int find_unplugged_device_id() {
	bool dev_tmp[SVBCAMERA_ID_MAX] = {false};
	int i;
	SVB_CAMERA_INFO info;

	int count = SVBGetNumOfConnectedCameras();
	for (i = 0; i < count; i++) {
		SVBGetCameraInfo(&info, i);
		dev_tmp[info.CameraID] = true;
	}

	int id = -1;
	for (i = 0; i < SVBCAMERA_ID_MAX; i++) {
		if (connected_ids[i] && !dev_tmp[i]){
			id = i;
			connected_ids[id] = false;
			break;
		}
	}
	return id;
}

static void process_plug_event(indigo_device *unused) {
	SVB_CAMERA_INFO info;
	SVB_CAMERA_PROPERTY property;
	SVB_BOOL is_guider = false;

	static indigo_device ccd_template = INDIGO_DEVICE_INITIALIZER(
		"",
		ccd_attach,
		svb_enumerate_properties,
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
	SVB_ERROR_CODE res = SVBGetCameraInfo(&info, index);
	if (res == SVB_SUCCESS) {
		res = SVBOpenCamera(info.CameraID);
		if (res == SVB_SUCCESS) {
			SVBGetCameraProperty(info.CameraID, &property);
			SVBCanPulseGuide(info.CameraID, &is_guider);
			SVBCloseCamera(info.CameraID);
		}
	}
	if (res == SVB_SUCCESS) {
		char *p = strstr(info.FriendlyName, "(CAM");
		if (p != NULL)
			*p = '\0';
		device->master_device = master_device;
		sprintf(device->name, "%s #%d", info.FriendlyName, id);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		svb_private_data *private_data = indigo_safe_malloc(sizeof(svb_private_data));
		private_data->dev_id = id;
		memcpy(&(private_data->info), &info, sizeof(SVB_CAMERA_INFO));
		memcpy(&(private_data->property), &property, sizeof(SVB_CAMERA_PROPERTY));
		device->private_data = private_data;
		indigo_attach_device(device);
		devices[slot]=device;
		if (is_guider) {
		slot = find_available_device_slot();
		if (slot < 0) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "No device slots available.");
			pthread_mutex_unlock(&device_mutex);
			return;
		}
		device = indigo_safe_malloc_copy(sizeof(indigo_device), &guider_template);
		device->master_device = master_device;
		sprintf(device->name, "%s Guider #%d", info.FriendlyName, id);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		device->private_data = private_data;
		indigo_attach_device(device);
		devices[slot]=device;
	}
	}
	pthread_mutex_unlock(&device_mutex);
}

static void process_unplug_event(indigo_device *unused) {
	pthread_mutex_lock(&device_mutex);
	int id, slot;
	bool removed = false;
	svb_private_data *private_data = NULL;
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
			SVBCloseCamera(id);
			if (private_data->buffer != NULL) {
				free(private_data->buffer);
				private_data->buffer = NULL;
			}
			free(private_data);
			private_data = NULL;
		}
	}
	if (!removed) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "No SVB Camera unplugged");
	}
	pthread_mutex_unlock(&device_mutex);
}

static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	struct libusb_device_descriptor descriptor;
	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			libusb_get_device_descriptor(dev, &descriptor);
			if (descriptor.idVendor == SVB_VENDOR_ID && descriptor.idProduct == 0x9a0a)
				indigo_set_timer(NULL, 0.5, process_plug_event, NULL);
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
	svb_private_data *pds[SVBCAMERA_ID_MAX] = {NULL};

	for (i = 0; i < MAX_DEVICES; i++) {
		indigo_device *device = devices[i];
		if (device == NULL) continue;
		if (PRIVATE_DATA) pds[PRIVATE_DATA->dev_id] = PRIVATE_DATA; /* preserve pointers to private data */
		indigo_detach_device(device);
		free(device);
		devices[i] = NULL;
	}

	/* free private data */
	for (i = 0; i < SVBCAMERA_ID_MAX; i++) {
		if (pds[i]) {
			if (pds[i]->buffer != NULL) {
				SVBCloseCamera(pds[i]->dev_id);
				free(pds[i]->buffer);
				pds[i]->buffer = NULL;
			}
			free(pds[i]);
		}
	}

	for (i = 0; i < SVBCAMERA_ID_MAX; i++)
		connected_ids[i] = false;
}


static libusb_hotplug_callback_handle callback_handle;

indigo_result indigo_ccd_svb(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "SVBONY Camera", __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;

			const char *sdk_version = SVBGetSDKVersion();
			INDIGO_DRIVER_LOG(DRIVER_NAME, "SVB SDK v. %s", sdk_version);

			indigo_start_usb_event_handler();
			int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, SVB_VENDOR_ID, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
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

indigo_result indigo_ccd_svb(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "SVBONY Camera", __FUNCTION__, DRIVER_VERSION, true, last_action);

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
