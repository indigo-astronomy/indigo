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


/** INDIGO Player One CCD driver
 \file indigo_ccd_playerone.c
 */

#define DRIVER_VERSION 0x0003
#define DRIVER_NAME "indigo_ccd_playerone"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>

#include <indigo/indigo_driver_xml.h>

#include "indigo_ccd_playerone.h"

#if !(defined(__APPLE__) && defined(__arm64__)) && !(defined(__linux__) && defined(__i386__))

#if defined(INDIGO_MACOS)
#include <libusb-1.0/libusb.h>
#elif defined(INDIGO_FREEBSD)
#include <libusb.h>
#else
#include <libusb-1.0/libusb.h>
#endif

#include "PlayerOneCamera.h"

#define POA_DEFAULT_BANDWIDTH      45

#define POA_MAX_FORMATS            4

#define POA_VENDOR_ID              0xa0a0

#define PRIVATE_DATA               ((playerone_private_data *)device->private_data)

#define PIXEL_FORMAT_PROPERTY      (PRIVATE_DATA->pixel_format_property)
#define RAW8_NAME                  "RAW 8"
#define RGB24_NAME                 "RGB 24"
#define RAW16_NAME                 "RAW 16"
#define MONO8_NAME                 "MONO 8"

#define POA_ADVANCED_PROPERTY      (PRIVATE_DATA->playerone_advanced_property)

// gp_bits is used as boolean
#define is_connected               gp_bits

// -------------------------------------------------------------------------------- Player One USB interface implementation

#define us2s(s) ((s) / 1000000.0)
#define s2us(us) ((us) * 1000000)

typedef struct {
	int dev_id;
	int count_open;
	int exp_bin;
	int exp_frame_width, exp_frame_height;
	int exp_bpp;
	bool exp_uses_bayer_pattern;
	char *bayer_pattern;
	indigo_timer *exposure_timer, *temperature_timer, *guider_timer_ra, *guider_timer_dec;
	double target_temperature, current_temperature;
	double cooler_power;
	unsigned char *buffer;
	long int buffer_size;
	pthread_mutex_t usb_mutex;
	bool can_check_temperature, has_temperature_sensor;
	POACameraProperties property;
	indigo_property *pixel_format_property;
	indigo_property *playerone_advanced_property;
} playerone_private_data;

static int get_pixel_depth(indigo_device *device) {
	int item = 0;
	while (item < POA_MAX_FORMATS) {
		if (PIXEL_FORMAT_PROPERTY->items[item].sw.value) {
			if (!strcmp(PIXEL_FORMAT_PROPERTY->items[item].name, RAW8_NAME))
				return 8;
			if (!strcmp(PIXEL_FORMAT_PROPERTY->items[item].name, RGB24_NAME))
				return 24;
			if (!strcmp(PIXEL_FORMAT_PROPERTY->items[item].name, RAW16_NAME))
				return 16;
			if (!strcmp(PIXEL_FORMAT_PROPERTY->items[item].name, MONO8_NAME))
				return 8;
		}
		item++;
	}
	return 8;
}


static int get_pixel_format(indigo_device *device) {
	int item = 0;
	while (item < POA_MAX_FORMATS) {
		if (PIXEL_FORMAT_PROPERTY->items[item].sw.value) {
			if (!strcmp(PIXEL_FORMAT_PROPERTY->items[item].name, RAW8_NAME)) {
				PRIVATE_DATA->exp_uses_bayer_pattern = PRIVATE_DATA->property.isColorCamera;
				return POA_RAW8;
			}
			if (!strcmp(PIXEL_FORMAT_PROPERTY->items[item].name, RGB24_NAME)) {
				PRIVATE_DATA->exp_uses_bayer_pattern = false;
				return POA_RGB24;
			}
			if (!strcmp(PIXEL_FORMAT_PROPERTY->items[item].name, RAW16_NAME)) {
				PRIVATE_DATA->exp_uses_bayer_pattern = PRIVATE_DATA->property.isColorCamera;
				return POA_RAW16;
			}
			if (!strcmp(PIXEL_FORMAT_PROPERTY->items[item].name, MONO8_NAME)) {
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
		if (i == POA_END)
			return false;
		if (type == PRIVATE_DATA->property.imgFormats[i])
			return true;
	}
	return false;
}


static indigo_result playerone_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(PIXEL_FORMAT_PROPERTY, property))
			indigo_define_property(device, PIXEL_FORMAT_PROPERTY, NULL);
		if (indigo_property_match(POA_ADVANCED_PROPERTY, property))
			indigo_define_property(device, POA_ADVANCED_PROPERTY, NULL);
	}
	return indigo_ccd_enumerate_properties(device, NULL, NULL);
}


static bool playerone_open(indigo_device *device) {
	int id = PRIVATE_DATA->dev_id;
	POAErrors res;

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
		res = POAOpenCamera(id);
		if (res) {
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "POAOpenCamera(%d) > %d", id, res);
			PRIVATE_DATA->count_open--;
			return false;
		}
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAOpenCamera(%d)", id);
		res = POAInitCamera(id);
		if (res) {
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "POAInitCamera(%d) > %d", id, res);
			PRIVATE_DATA->count_open--;
			return false;
		}
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAInitCamera(%d)", id);
		if (PRIVATE_DATA->buffer == NULL) {
			if (PRIVATE_DATA->property.isColorCamera)
				PRIVATE_DATA->buffer_size = PRIVATE_DATA->property.maxHeight * PRIVATE_DATA->property.maxWidth * 3 + FITS_HEADER_SIZE + 1024;
			else
				PRIVATE_DATA->buffer_size = PRIVATE_DATA->property.maxHeight * PRIVATE_DATA->property.maxWidth * 2 + FITS_HEADER_SIZE + 1024;
			PRIVATE_DATA->buffer = (unsigned char*)indigo_alloc_blob_buffer(PRIVATE_DATA->buffer_size);
		}
	}
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	return true;
}

static bool playerone_setup_exposure(indigo_device *device, double exposure, int frame_left, int frame_top, int frame_width, int frame_height, int bin) {
	int id = PRIVATE_DATA->dev_id;
	POAErrors res;

	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	int pf = get_pixel_format(device);
	res = POASetImageFormat(id, pf);
	if (res) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "POASetImageFormat(%d, %d) > %d", id, pf, res);
		return false;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POASetImageFormat(%d, %d)", id, pf);

	res = POASetImageBin(id, bin);
	if (res) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "POASetImageBin(%d, %d) > %d", id, bin, res);
		return false;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POASetImageBin(%d, %d)", id, bin);

	int fl = frame_left / bin;
	int ft = frame_top / bin;
	res = POASetImageStartPos(id, fl, ft);
	if (res) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "POASetImageStartPos(%d, %d, %d) > %d", id, fl, ft, res);
		return false;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POASetImageStartPos(%d, %d, %d)", id, fl, ft);

	int fw = frame_width / bin;
	int fh = frame_height / bin;
	res = POASetImageSize(id, fw, fh);
	if (res) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "POASetImageSize(%d, %d, %d) > %d", id, fw, fh, res);
		return false;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POASetImageSize(%d, %d, %d)", id, fw, fh);


	POAConfigValue value = { .intValue = (long)s2us(exposure) };
	res = POASetConfig(id, POA_EXPOSURE, value, POA_FALSE);
	if (res) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "POASetConfig(%d, POA_EXPOSURE, %d) > %d", id, value.intValue, res);
		return false;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POASetConfig(%d, POA_EXPOSURE, %d)", id, value.intValue);

	PRIVATE_DATA->exp_bin = bin;
	PRIVATE_DATA->exp_frame_width = frame_width;
	PRIVATE_DATA->exp_frame_height = frame_height;
	PRIVATE_DATA->exp_bpp = (int)CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value;
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	return true;
}

static bool playerone_set_cooler(indigo_device *device, bool status, double target, double *current, double *power) {
	POAErrors res;
	POABool unused;
	POAConfigValue value;

	int id = PRIVATE_DATA->dev_id;

	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);

	if (PRIVATE_DATA->has_temperature_sensor) {
		res = POAGetConfig(id, POA_TEMPERATURE, &value, &unused);
		if (res)
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "POAGetConfig(%d, POA_CURRENT_TEMPERATURE) > %d", id, res);
		else
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetConfig(%d, POA_CURRENT_TEMPERATURE, > %g)", id, value.floatValue);
		*current = value.floatValue ;
	} else {
		*current = 0;
	}

	if (!PRIVATE_DATA->property.isHasCooler) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		return true;
	}

	res = POAGetConfig(id, POA_COOLER, &value, &unused);
	if (res) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "POAGetConfig(%d, POA_COOLER) > %d", id, res);
		return false;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetConfig(%d, POA_COOLER, > %s)", id, value.boolValue ? "true" : "false");

	if (value.boolValue != status) {
		value.boolValue = status;
		res = POASetConfig(id, POA_COOLER, value, false);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "POASetConfig(%d, POA_COOLER, %s) > %d", id, value.boolValue ? "true" : "false", res);
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			return true;
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POASetConfig(%d, POA_COOLER, %s)", id, value.boolValue ? "true" : "false");
		}
		value.intValue = status ? 100 : 0;
		res = POASetConfig(id, POA_FAN_POWER, value, false);
		if (res)
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "POASetConfig(%d, POA_FAN_POWER, %d) > %d", id, value.intValue, res);
		else
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POASetConfig(%d, POA_FAN_POWER, %d)", id, value.intValue);
	} else if (status) {
		res = POAGetConfig(id, POA_TARGET_TEMP, &value, &unused);
		if (res)
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "POAGetConfig(%d, POA_TARGET_TEMP) > %d", id, res);
		else
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetConfig(%d, POA_TARGET_TEMP, > %d)", id, value.intValue);
		if ((int)target != value.intValue) {
			value.intValue = target;
			res = POASetConfig(id, POA_TARGET_TEMP, value, false);
			if (res)
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "POASetConfig(%d, POA_TARGET_TEMP, %d) > %d", id, value.intValue, res);
			else
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POASetConfig(%d, POA_TARGET_TEMP, %d)", id, value.intValue);
		}
	}

	res = POAGetConfig(id, POA_COOLER_POWER, &value, &unused);
	if (res)
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "POAGetConfig(%d, POA_COOLER_POWER) > %d", id, res);
	else
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetConfig(%d, POA_COOLER_POWER, > %d)", id, value.intValue);
	*power = value.intValue;

	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	return true;
}

static void playerone_close(indigo_device *device) {
	if (!device->is_connected)
		return;
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	if (--PRIVATE_DATA->count_open == 0) {
		POACloseCamera(PRIVATE_DATA->dev_id);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POACloseCamera(%d)", PRIVATE_DATA->dev_id);
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
	indigo_fits_keyword keywords[] = {
		{ INDIGO_FITS_STRING, "BAYERPAT", .string = PRIVATE_DATA->bayer_pattern, "Bayer color pattern" },
		{ INDIGO_FITS_NUMBER, "XBAYROFF", .number = 0, "X offset of Bayer array" },
		{ INDIGO_FITS_NUMBER, "YBAYROFF", .number = 0, "Y offset of Bayer array" },
		{ 0 }
	};
	int id = PRIVATE_DATA->dev_id;
	POAErrors res;
	POACameraState state;
	PRIVATE_DATA->can_check_temperature = false;
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	res = POAGetCameraState(id, &state);
	if (res) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "POAGetCameraState(%d, ...) > %d", id, res);
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetCameraState(%d, > %d)", id, state);
		if (state == STATE_EXPOSING) {
			res = POAStopExposure(id);
			if (res)
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "POAStopExposure(%d) > %d", id, res);
			else
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAStopExposure(%d)", id);
		}
		while (true) {
			res = POAGetImageData(id, PRIVATE_DATA->buffer + FITS_HEADER_SIZE, PRIVATE_DATA->buffer_size, 0);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetImageData(%d, ..., > %d, 0) > %d", id, PRIVATE_DATA->buffer_size, res);
			if (res) {
				res = POA_OK;
				break;
			}
		}
	}
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	if (res == POA_OK && playerone_setup_exposure(device, CCD_EXPOSURE_ITEM->number.target, CCD_FRAME_LEFT_ITEM->number.value, CCD_FRAME_TOP_ITEM->number.value, CCD_FRAME_WIDTH_ITEM->number.value, CCD_FRAME_HEIGHT_ITEM->number.value, CCD_BIN_HORIZONTAL_ITEM->number.value)) {
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		res = POAStartExposure(id, true);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "POAStartExposure(%d, true) > %d", id, res);
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAStartExposure(%d, true)", id);
			CCD_EXPOSURE_ITEM->number.value = CCD_EXPOSURE_ITEM->number.target;
			while (CCD_EXPOSURE_ITEM->number.value > 1) {
				if (POAGetCameraState(id, &state) == POA_OK && state != STATE_EXPOSING) {
					CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
					break;
				}
				if (CCD_ABORT_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
					pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
					POAStopExposure(PRIVATE_DATA->dev_id);
					pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
					CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
					CCD_ABORT_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
					indigo_update_property(device, CCD_ABORT_EXPOSURE_PROPERTY, NULL);
					break;
				}
				indigo_usleep(1000000);
				CCD_EXPOSURE_ITEM->number.value--;
				indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
			}
			CCD_EXPOSURE_ITEM->number.value = 0;
			if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
				pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
				res = POAGetImageData(id, PRIVATE_DATA->buffer + FITS_HEADER_SIZE, PRIVATE_DATA->buffer_size, 2000);
				pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
				if (res) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "POAGetImageData(%d, ..., ..., 2000) > %d", id, res);
				} else {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetImageData(%d, ..., > %d, 2000)", id, PRIVATE_DATA->buffer_size);
					if (PRIVATE_DATA->exp_uses_bayer_pattern && PRIVATE_DATA->bayer_pattern) {
						indigo_process_image(device, PRIVATE_DATA->buffer, (int)(PRIVATE_DATA->exp_frame_width / PRIVATE_DATA->exp_bin), (int)(PRIVATE_DATA->exp_frame_height / PRIVATE_DATA->exp_bin), PRIVATE_DATA->exp_bpp, true, false, keywords, true);
					} else {
						indigo_process_image(device, PRIVATE_DATA->buffer, (int)(PRIVATE_DATA->exp_frame_width / PRIVATE_DATA->exp_bin), (int)(PRIVATE_DATA->exp_frame_height / PRIVATE_DATA->exp_bin), PRIVATE_DATA->exp_bpp, true, false, NULL, true);
					}
				}
			}
		}
	} else {
		res = POA_ERROR_EXPOSURE_FAILED;
	}
	PRIVATE_DATA->can_check_temperature = true;
	indigo_finalize_video_stream(device);
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		if (res) {
			CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
		}
	}
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_ALERT_STATE) {
		indigo_ccd_failure_cleanup(device);
	}
	indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
	PRIVATE_DATA->can_check_temperature = true;
}

static void streaming_timer_callback(indigo_device *device) {
	if (!CONNECTION_CONNECTED_ITEM->sw.value)
		return;
	indigo_fits_keyword keywords[] = {
		{ INDIGO_FITS_STRING, "BAYERPAT", .string = PRIVATE_DATA->bayer_pattern, "Bayer color pattern" },
		{ INDIGO_FITS_NUMBER, "XBAYROFF", .number = 0, "X offset of Bayer array" },
		{ INDIGO_FITS_NUMBER, "YBAYROFF", .number = 0, "Y offset of Bayer array" },
		{ 0 }
	};
	int id = PRIVATE_DATA->dev_id;
	POAErrors res;
	POACameraState state;
	PRIVATE_DATA->can_check_temperature = false;
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	res = POAGetCameraState(id, &state);
	if (res) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "POAGetCameraState(%d, ...) > %d", id, res);
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetCameraState(%d, > %d)", id, state);
		if (state == STATE_EXPOSING) {
			res = POAStopExposure(id);
			if (res)
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "POAStopExposure(%d) > %d", id, res);
			else
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAStopExposure(%d)", id);
		}
		while (true) {
			res = POAGetImageData(id, PRIVATE_DATA->buffer + FITS_HEADER_SIZE, PRIVATE_DATA->buffer_size, 0);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetImageData(%d, ..., > %d, 0) > %d", id, PRIVATE_DATA->buffer_size, res);
			if (res) {
				res = POA_OK;
				break;
			}
		}
	}
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	if (res == POA_OK && playerone_setup_exposure(device, CCD_STREAMING_EXPOSURE_ITEM->number.target, CCD_FRAME_LEFT_ITEM->number.value, CCD_FRAME_TOP_ITEM->number.value, CCD_FRAME_WIDTH_ITEM->number.value, CCD_FRAME_HEIGHT_ITEM->number.value, CCD_BIN_HORIZONTAL_ITEM->number.value)) {
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		res = POAStartExposure(id, false);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "POAStartExposure(%d, true) > %d", id, res);
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAStartExposure(%d, true) > %d", id, res);
			while (CCD_STREAMING_COUNT_ITEM->number.value != 0) {
				CCD_STREAMING_EXPOSURE_ITEM->number.value = CCD_STREAMING_EXPOSURE_ITEM->number.target;
				while (CCD_STREAMING_EXPOSURE_ITEM->number.value > 1) {
					if (POAGetCameraState(id, &state) == POA_OK && state != STATE_EXPOSING) {
						CCD_STREAMING_PROPERTY->state = INDIGO_ALERT_STATE;
						break;
					}
					if (CCD_ABORT_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
						pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
						POAStopExposure(PRIVATE_DATA->dev_id);
						pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
						CCD_STREAMING_PROPERTY->state = INDIGO_ALERT_STATE;
						CCD_ABORT_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
						indigo_update_property(device, CCD_ABORT_EXPOSURE_PROPERTY, NULL);
						break;
					}
					indigo_usleep(1000000);
					CCD_STREAMING_EXPOSURE_ITEM->number.value--;
					indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
				}
				CCD_STREAMING_EXPOSURE_ITEM->number.value = 0;
				if (CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
					pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
					res = POAGetImageData(id, PRIVATE_DATA->buffer + FITS_HEADER_SIZE, PRIVATE_DATA->buffer_size, 2000);
					pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
					if (res) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "POAGetImageData(%d, ..., ..., 2000) > %d", id, res);
						break;
					}
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetImageData(%d, ..., > %d, 2000)", id, PRIVATE_DATA->buffer_size);
					if (PRIVATE_DATA->exp_uses_bayer_pattern && PRIVATE_DATA->bayer_pattern) {
						indigo_process_image(device, PRIVATE_DATA->buffer, (int)(PRIVATE_DATA->exp_frame_width / PRIVATE_DATA->exp_bin), (int)(PRIVATE_DATA->exp_frame_height / PRIVATE_DATA->exp_bin), PRIVATE_DATA->exp_bpp, true, false, keywords, true);
					} else {
						indigo_process_image(device, PRIVATE_DATA->buffer, (int)(PRIVATE_DATA->exp_frame_width / PRIVATE_DATA->exp_bin), (int)(PRIVATE_DATA->exp_frame_height / PRIVATE_DATA->exp_bin), PRIVATE_DATA->exp_bpp, true, false, NULL, true);
					}
					if (CCD_STREAMING_COUNT_ITEM->number.value > 0)
						CCD_STREAMING_COUNT_ITEM->number.value -= 1;
					CCD_STREAMING_EXPOSURE_ITEM->number.value = CCD_STREAMING_EXPOSURE_ITEM->number.target;
					CCD_STREAMING_PROPERTY->state = INDIGO_BUSY_STATE;
					indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
				}
			}
			pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
			POAStopExposure(id);
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		}
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	} else {
		res = POA_ERROR_EXPOSURE_FAILED;
	}
	PRIVATE_DATA->can_check_temperature = true;
	indigo_finalize_video_stream(device);
	if (CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
		if (res) {
			CCD_STREAMING_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			CCD_STREAMING_PROPERTY->state = INDIGO_OK_STATE;
		}
	}
	if (CCD_STREAMING_PROPERTY->state == INDIGO_ALERT_STATE) {
		indigo_ccd_failure_cleanup(device);
	}
	indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
}

static void ccd_temperature_callback(indigo_device *device) {
	if (!CONNECTION_CONNECTED_ITEM->sw.value) return;
	if (PRIVATE_DATA->can_check_temperature) {
		if (playerone_set_cooler(device, CCD_COOLER_ON_ITEM->sw.value, PRIVATE_DATA->target_temperature, &PRIVATE_DATA->current_temperature, &PRIVATE_DATA->cooler_power)) {
			double diff = PRIVATE_DATA->current_temperature - PRIVATE_DATA->target_temperature;
			if (CCD_COOLER_ON_ITEM->sw.value)
				CCD_TEMPERATURE_PROPERTY->state = fabs(diff) > 0.5 ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
			else
				CCD_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
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
	indigo_reschedule_timer(device, 5, &PRIVATE_DATA->temperature_timer);
}


static void guider_timer_callback_ra(indigo_device *device) {
	POAConfigValue value = { .boolValue = false };
	POAErrors res;
	int id = PRIVATE_DATA->dev_id;
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	res = POASetConfig(PRIVATE_DATA->dev_id, POA_GUIDE_EAST, value, false);
	if (res)
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "POASetConfig(%d, POA_GUIDE_EAST, false, false) > %d", id, res);
	else
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POASetConfig(%d, POA_GUIDE_EAST, false, false)", id);
	res = POASetConfig(PRIVATE_DATA->dev_id, POA_GUIDE_WEST, value, false);
	if (res)
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "POASetConfig(%d, POA_GUIDE_WEST, false, false) > %d", id, res);
	else
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POASetConfig(%d, POA_GUIDE_WEST, false, false)", id);
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);

	PRIVATE_DATA->guider_timer_ra = NULL;

	if (!CONNECTION_CONNECTED_ITEM->sw.value)
		return;

	GUIDER_GUIDE_EAST_ITEM->number.value = 0;
	GUIDER_GUIDE_WEST_ITEM->number.value = 0;
	GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
}


static void guider_timer_callback_dec(indigo_device *device) {
	POAConfigValue value = { .boolValue = false };
	POAErrors res;
	int id = PRIVATE_DATA->dev_id;
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	res = POASetConfig(id, POA_GUIDE_NORTH, value, false);
	if (res)
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "POASetConfig(%d, POA_GUIDE_NORTH, false, false) > %d", id, res);
	else
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POASetConfig(%d, POA_GUIDE_NORTH, false, false)", id);
	res = POASetConfig(id, POA_GUIDE_SOUTH, value, false);
	if (res)
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "POASetConfig(%d, POA_GUIDE_SOUTH, false, false) > %d", id, res);
	else
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POASetConfig(%d, POA_GUIDE_SOUTH, false, false)", id);
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
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
		PIXEL_FORMAT_PROPERTY = indigo_init_switch_property(NULL, device->name, "PIXEL_FORMAT", CCD_ADVANCED_GROUP, "Pixel Format", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, POA_MAX_FORMATS);
		if (PIXEL_FORMAT_PROPERTY == NULL)
			return INDIGO_FAILED;

		int format_count = 0;
		if (pixel_format_supported(device, POA_RAW8)) {
			indigo_init_switch_item(PIXEL_FORMAT_PROPERTY->items + format_count, RAW8_NAME, RAW8_NAME, true);
			format_count++;
		}
		if (pixel_format_supported(device, POA_RGB24)) {
			indigo_init_switch_item(PIXEL_FORMAT_PROPERTY->items + format_count, RGB24_NAME, RGB24_NAME, false);
			format_count++;
		}
		if (pixel_format_supported(device, POA_RAW16)) {
			indigo_init_switch_item(PIXEL_FORMAT_PROPERTY->items + format_count, RAW16_NAME, RAW16_NAME, false);
			format_count++;
		}
		if (pixel_format_supported(device, POA_MONO8)) {
			indigo_init_switch_item(PIXEL_FORMAT_PROPERTY->items + format_count, MONO8_NAME, MONO8_NAME, false);
			format_count++;
		}
		PIXEL_FORMAT_PROPERTY->count = format_count;
		// -------------------------------------------------------------------------------- INFO
		INFO_PROPERTY->count = 8;
		snprintf(INFO_DEVICE_MODEL_ITEM->text.value, INDIGO_NAME_SIZE, "%s (%s)", PRIVATE_DATA->property.cameraModelName, PRIVATE_DATA->property.sensorModelName);
		snprintf(INFO_DEVICE_FW_REVISION_ITEM->text.value, INDIGO_NAME_SIZE, "SDK %s, API %d", POAGetSDKVersion(), POAGetAPIVersion());
		snprintf(INFO_DEVICE_SERIAL_NUM_ITEM->text.value, INDIGO_NAME_SIZE, "%s", PRIVATE_DATA->property.SN);
		// -------------------------------------------------------------------------------- CCD_INFO
		CCD_INFO_WIDTH_ITEM->number.value = PRIVATE_DATA->property.maxWidth;
		CCD_INFO_HEIGHT_ITEM->number.value = PRIVATE_DATA->property.maxHeight;
		CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = PRIVATE_DATA->property.bitDepth;

		CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.target = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max = PRIVATE_DATA->property.maxWidth;
		CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.target = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max = PRIVATE_DATA->property.maxHeight;
		CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.target = get_pixel_depth(device);
		CCD_FRAME_BITS_PER_PIXEL_ITEM->number.min = 8;
		CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = 24;

		/* find max binning */
		int max_bin = 1;
		for (int num = 0; (num < 16) && PRIVATE_DATA->property.bins[num]; num++) {
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
		for (int num = 0; (num < 16) && PRIVATE_DATA->property.bins[num]; num++) {
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
		// -------------------------------------------------------------------------------- CCD_STREAMING
		CCD_STREAMING_PROPERTY->hidden = false;
		CCD_IMAGE_FORMAT_PROPERTY->count = 7;
		// -------------------------------------------------------------------------------- POA_ADVANCED
		POA_ADVANCED_PROPERTY = indigo_init_number_property(NULL, device->name, "POA_ADVANCED", CCD_ADVANCED_GROUP, "Advanced", INDIGO_OK_STATE, INDIGO_RW_PERM, 0);
		if (POA_ADVANCED_PROPERTY == NULL)
			return INDIGO_FAILED;
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
		return indigo_ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void handle_advanced_property(indigo_device *device) {
	int ctrl_count;
	POAConfigAttributes ctrl_caps;
	POAErrors res;
	int id = PRIVATE_DATA->dev_id;

	if (!IS_CONNECTED)
		return;

	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);

	res = POAGetConfigsCount(id, &ctrl_count);
	if (res) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "POAGetNumOfControls(%d) > %d", id, res);
		POA_ADVANCED_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, POA_ADVANCED_PROPERTY, NULL);
		return;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetNumOfControls(%d, > %d)", id, ctrl_count);
	POA_ADVANCED_PROPERTY->state = INDIGO_OK_STATE;
	POABool unused;
	POAConfigValue value;
	for (int ctrl_no = 0; ctrl_no < ctrl_count; ctrl_no++) {
		POAGetConfigAttributes(id, ctrl_no, &ctrl_caps);
		for (int i = 0; i < POA_ADVANCED_PROPERTY->count; i++) {
			indigo_item *item = POA_ADVANCED_PROPERTY->items + i;
			if (!strncmp(ctrl_caps.szConfName, item->name, INDIGO_NAME_SIZE)) {
				if (ctrl_caps.valueType == VAL_BOOL)
					value.boolValue = item->number.value != 0;
				else if (ctrl_caps.valueType == VAL_FLOAT)
					value.floatValue = item->number.value;
				else
					value.intValue = item->number.value;
				res = POASetConfig(id, ctrl_caps.configID, value, POA_FALSE);
				if (res) {
					POA_ADVANCED_PROPERTY->state = INDIGO_ALERT_STATE;
					if (ctrl_caps.valueType == VAL_BOOL)
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "POASetConfig(%d, %s, %s) > %d", id, ctrl_caps.szConfName, value.boolValue ? "true" : "false", res);
					else if (ctrl_caps.valueType == VAL_FLOAT)
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "POASetConfig(%d, %s, %g) > %d", id, ctrl_caps.szConfName, value.floatValue, res);
					else
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "POASetConfig(%d, %s, %d) > %d", id, ctrl_caps.szConfName, value.intValue, res);
				} else {
					if (ctrl_caps.valueType == VAL_BOOL)
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POASetConfig(%d, %s, %s)", id, ctrl_caps.szConfName, value.boolValue ? "true" : "false");
					else if (ctrl_caps.valueType == VAL_FLOAT)
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POASetConfig(%d, %s, %g)", id, ctrl_caps.szConfName, value.floatValue);
					else
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POASetConfig(%d, %s, %d)", id, ctrl_caps.szConfName, value.intValue);
				}
				res = POAGetConfig(id, ctrl_caps.configID, &value, &unused);
				if (res) {
					POA_ADVANCED_PROPERTY->state = INDIGO_ALERT_STATE;
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
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetConfig(%d, %s, > %d)", id, ctrl_caps.szConfName, value.intValue);
					}
				}
			}
		}
	}
	indigo_update_property(device, POA_ADVANCED_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
}

static indigo_result init_camera_property(indigo_device *device, POAConfigAttributes ctrl_caps) {
	int id = PRIVATE_DATA->dev_id;
	POAConfigValue value;
	POAErrors res;
	POABool unused;

	if (ctrl_caps.configID == POA_EXPOSURE) {
		CCD_EXPOSURE_PROPERTY->hidden = false;
		if (ctrl_caps.isWritable)
			CCD_EXPOSURE_PROPERTY->perm = INDIGO_RW_PERM;
		else
			CCD_EXPOSURE_PROPERTY->perm = INDIGO_RO_PERM;

		CCD_EXPOSURE_ITEM->number.min = CCD_STREAMING_EXPOSURE_ITEM->number.min = us2s(ctrl_caps.minValue.intValue);
		CCD_EXPOSURE_ITEM->number.max = CCD_STREAMING_EXPOSURE_ITEM->number.max = us2s(ctrl_caps.maxValue.intValue);
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		unused = false;
		res = POAGetConfig(id, POA_EXPOSURE, &value, &unused);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if (res)
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "POAGetConfig(%d, POA_EXPOSURE) > %d", id, res);
		else
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetConfig(%d, POA_EXPOSURE, > %d)", id, value.intValue);
		CCD_EXPOSURE_ITEM->number.value = CCD_EXPOSURE_ITEM->number.target = us2s(value.intValue);
		return INDIGO_OK;
	}

	if (ctrl_caps.configID == POA_OFFSET) {
		CCD_OFFSET_PROPERTY->hidden = false;
		if (ctrl_caps.isWritable)
			CCD_OFFSET_PROPERTY->perm = INDIGO_RW_PERM;
		else
			CCD_OFFSET_PROPERTY->perm = INDIGO_RO_PERM;

		CCD_OFFSET_ITEM->number.min = ctrl_caps.minValue.intValue;
		CCD_OFFSET_ITEM->number.max = ctrl_caps.maxValue.intValue;
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		unused = false;
		res = POAGetConfig(id, POA_OFFSET, &value, &unused);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if (res)
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "POAGetConfig(%d, POA_OFFSET) > %d", id, res);
		else
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetConfig(%d, POA_OFFSET,  > %d)", id, value.intValue);
		CCD_OFFSET_ITEM->number.value = CCD_OFFSET_ITEM->number.target = value.intValue;
		CCD_OFFSET_ITEM->number.step = 1;
		return INDIGO_OK;
	}

	if (ctrl_caps.configID == POA_GAIN) {
		CCD_GAIN_PROPERTY->hidden = false;
		if (ctrl_caps.isWritable)
			CCD_GAIN_PROPERTY->perm = INDIGO_RW_PERM;
		else
			CCD_GAIN_PROPERTY->perm = INDIGO_RO_PERM;

		CCD_GAIN_ITEM->number.min = ctrl_caps.minValue.intValue;
		CCD_GAIN_ITEM->number.max = ctrl_caps.maxValue.intValue;
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		unused = false;
		res = POAGetConfig(id, POA_GAIN, &value, &unused);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if (res)
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "POAGetConfig(%d, POA_GAIN) > %d", id, res);
		else
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetConfig(%d, POA_GAIN,  > %d)", id, value.intValue);
		CCD_GAIN_ITEM->number.value = CCD_GAIN_ITEM->number.target = value.intValue;
		CCD_GAIN_ITEM->number.step = 1;
		return INDIGO_OK;
	}

	if (ctrl_caps.configID == POA_TARGET_TEMP) {
		CCD_TEMPERATURE_PROPERTY->hidden = false;
		if (ctrl_caps.isWritable)
			CCD_TEMPERATURE_PROPERTY->perm = INDIGO_RW_PERM;
		else
			CCD_TEMPERATURE_PROPERTY->perm = INDIGO_RO_PERM;

		CCD_TEMPERATURE_ITEM->number.min = ctrl_caps.minValue.intValue;
		CCD_TEMPERATURE_ITEM->number.max = ctrl_caps.maxValue.intValue;
		CCD_TEMPERATURE_ITEM->number.value = CCD_TEMPERATURE_ITEM->number.target = ctrl_caps.defaultValue.floatValue;
		PRIVATE_DATA->target_temperature = ctrl_caps.defaultValue.floatValue;
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
		if (ctrl_caps.isWritable)
			CCD_COOLER_PROPERTY->perm = INDIGO_RW_PERM;
		else
			CCD_COOLER_PROPERTY->perm = INDIGO_RO_PERM;
		return INDIGO_OK;
	}

	if (ctrl_caps.configID == POA_COOLER_POWER) {
		CCD_COOLER_POWER_PROPERTY->hidden = false;
		if (ctrl_caps.isWritable)
			CCD_COOLER_POWER_PROPERTY->perm = INDIGO_RW_PERM;
		else
			CCD_COOLER_POWER_PROPERTY->perm = INDIGO_RO_PERM;

		CCD_COOLER_POWER_ITEM->number.min = ctrl_caps.minValue.intValue;
		CCD_COOLER_POWER_ITEM->number.max = ctrl_caps.maxValue.intValue;
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		res = POAGetConfig(id, POA_COOLER_POWER, &value, &unused);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if (res)
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "POAGetConfig(%d, POA_COOLER_POWER) > %d", id, res);
		else
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetConfig(%d, POA_COOLER_POWER,  > %d)", id, value.intValue);
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
		int offset = POA_ADVANCED_PROPERTY->count;
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		unused = false;
		res = POAGetConfig(id, ctrl_caps.configID, &value, &unused);
		if (ctrl_caps.configID == POA_USB_BANDWIDTH_LIMIT && value.intValue == 100) {
			value.intValue = POA_DEFAULT_BANDWIDTH;
			POASetConfig(id, POA_USB_BANDWIDTH_LIMIT, value, false);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Default USB Bandwidth is 100, reducing to %d", value.intValue);
		}
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "POAGetConfig(%d, %s) > %d", id, ctrl_caps.szConfName, res);
		} else {
			POA_ADVANCED_PROPERTY = indigo_resize_property(POA_ADVANCED_PROPERTY, offset + 1);
			if (ctrl_caps.valueType == VAL_FLOAT) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetConfig(%d, %s, > %g)", id, ctrl_caps.szConfName, value.floatValue);
				indigo_init_number_item(POA_ADVANCED_PROPERTY->items + offset, ctrl_caps.szConfName, ctrl_caps.szConfName, ctrl_caps.minValue.floatValue, ctrl_caps.maxValue.floatValue, 1, value.floatValue);
			} else if (ctrl_caps.valueType == VAL_BOOL) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetConfig(%d, %s, > %s)", id, ctrl_caps.szConfName, value.boolValue ? "true" : "false");
				indigo_init_number_item(POA_ADVANCED_PROPERTY->items + offset, ctrl_caps.szConfName, ctrl_caps.szConfName, 0, 1, 1, value.boolValue ? 1 : 0);
			} else {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetConfig(%d, %s, > %d)", id, ctrl_caps.szConfName, value.intValue);
				indigo_init_number_item(POA_ADVANCED_PROPERTY->items + offset, ctrl_caps.szConfName, ctrl_caps.szConfName, ctrl_caps.minValue.intValue, ctrl_caps.maxValue.intValue, 1, value.intValue);
			}
		}
	}
	return INDIGO_OK;
}

static void handle_ccd_connect_property(indigo_device *device) {
	indigo_lock_master_device(device);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (!device->is_connected) {
			if (playerone_open(device)) {
				int id = PRIVATE_DATA->dev_id;
				int ctrl_count;
				POAConfigAttributes ctrl_caps;

				indigo_define_property(device, PIXEL_FORMAT_PROPERTY, NULL);

				pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
				int res = POAGetConfigsCount(id, &ctrl_count);
				pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
				if (res)
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "POAGetNumOfControls(%d) > %d", id, res);
				else
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetNumOfControls(%d, > %d)", id, ctrl_count);
				POA_ADVANCED_PROPERTY = indigo_resize_property(POA_ADVANCED_PROPERTY, 0);
				for (int ctrl_no = 0; ctrl_no < ctrl_count; ctrl_no++) {
					pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
					POAGetConfigAttributes(id, ctrl_no, &ctrl_caps);
					pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
					init_camera_property(device, ctrl_caps);
				}
				indigo_define_property(device, POA_ADVANCED_PROPERTY, NULL);
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
			indigo_delete_property(device, POA_ADVANCED_PROPERTY, NULL);
			playerone_close(device);
			device->is_connected = false;
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	}
	indigo_ccd_change_property(device, NULL, CONNECTION_PROPERTY);
	indigo_unlock_master_device(device);
}

static indigo_result ccd_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);

	// -------------------------------------------------------------------------------- CONNECTION -> CCD_INFO, CCD_COOLER, CCD_TEMPERATURE
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, handle_ccd_connect_property, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- CCD_EXPOSURE
	} else if (indigo_property_match_changeable(CCD_EXPOSURE_PROPERTY, property)) {
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
		indigo_set_timer(device, 0, exposure_timer_callback, &PRIVATE_DATA->exposure_timer);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_STREAMING_PROPERTY, property)) {
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
	} else if (indigo_property_match_changeable(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		if (CCD_ABORT_EXPOSURE_ITEM->sw.value && (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE)) {
			CCD_ABORT_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
			if (CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE && CCD_STREAMING_COUNT_ITEM->number.value != 0) {
				CCD_STREAMING_COUNT_ITEM->number.value = 0;
			}
			PRIVATE_DATA->can_check_temperature = true;
		}
		indigo_property_copy_values(CCD_ABORT_EXPOSURE_PROPERTY, property, false);
		// -------------------------------------------------------------------------------- CCD_COOLER
	} else if (indigo_property_match_changeable(CCD_COOLER_PROPERTY, property)) {
		indigo_property_copy_values(CCD_COOLER_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value && !CCD_COOLER_PROPERTY->hidden) {
			CCD_COOLER_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
		}
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- CCD_TEMPERATURE
	} else if (indigo_property_match_changeable(CCD_TEMPERATURE_PROPERTY, property)) {
		indigo_property_copy_values(CCD_TEMPERATURE_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value && !CCD_COOLER_PROPERTY->hidden) {
			PRIVATE_DATA->target_temperature = CCD_TEMPERATURE_ITEM->number.value;
			CCD_TEMPERATURE_ITEM->number.value = PRIVATE_DATA->current_temperature;
			CCD_TEMPERATURE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, "Target Temperature = %.2f", PRIVATE_DATA->target_temperature);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_OFFSET_PROPERTY, property)) {
		CCD_OFFSET_PROPERTY->state = INDIGO_OK_STATE;
		indigo_property_copy_values(CCD_OFFSET_PROPERTY, property, false);
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		POAConfigValue value;
		value.intValue = (long)(CCD_OFFSET_ITEM->number.value);
		POAErrors res = POASetConfig(PRIVATE_DATA->dev_id, POA_OFFSET, value, POA_FALSE);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "POASetConfig(%d, POA_BLACK_LEVEL, %d) > %d", PRIVATE_DATA->dev_id, value.intValue, res);
			CCD_OFFSET_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POASetConfig(%d, POA_BLACK_LEVEL, %d)", PRIVATE_DATA->dev_id, value.intValue);
			CCD_OFFSET_PROPERTY->state = INDIGO_OK_STATE;
		}
		indigo_update_property(device, CCD_OFFSET_PROPERTY, NULL);
		return INDIGO_OK;
		// ------------------------------------------------------------------------------- CCD_GAIN
	} else if (indigo_property_match_changeable(CCD_GAIN_PROPERTY, property)) {
		CCD_GAIN_PROPERTY->state = INDIGO_OK_STATE;
		indigo_property_copy_values(CCD_GAIN_PROPERTY, property, false);
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		POAConfigValue value;
		value.intValue = (long)(CCD_GAIN_ITEM->number.value);
		POAErrors res = POASetConfig(PRIVATE_DATA->dev_id, POA_GAIN, value, POA_FALSE);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "POASetConfig(%d, POA_GAIN, %d) > %d", PRIVATE_DATA->dev_id, value.intValue, res);
			CCD_GAIN_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POASetConfig(%d, POA_GAIN, %d)", PRIVATE_DATA->dev_id, value.intValue);
			CCD_GAIN_PROPERTY->state = INDIGO_OK_STATE;
		}
		indigo_update_property(device, CCD_GAIN_PROPERTY, NULL);
		return INDIGO_OK;
		// ------------------------------------------------------------------------------- CCD_FRAME
	} else if (indigo_property_match_changeable(CCD_FRAME_PROPERTY, property)) {
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
		indigo_update_property(device, CCD_FRAME_PROPERTY, NULL);
		indigo_update_property(device, CCD_MODE_PROPERTY, NULL);
		indigo_update_property(device, PIXEL_FORMAT_PROPERTY, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- PIXEL_FORMAT
	} else if (indigo_property_match_changeable(PIXEL_FORMAT_PROPERTY, property)) {
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
		indigo_update_property(device, CCD_FRAME_PROPERTY, NULL);
		indigo_update_property(device, CCD_MODE_PROPERTY, NULL);
		indigo_update_property(device, PIXEL_FORMAT_PROPERTY, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- ADVANCED
	} else if (indigo_property_match_changeable(POA_ADVANCED_PROPERTY, property)) {
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
			POA_ADVANCED_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, POA_ADVANCED_PROPERTY, "Exposure in progress, advanced settings can not be changed.");
			return INDIGO_OK;
		}
		indigo_property_copy_values(POA_ADVANCED_PROPERTY, property, false);
		POA_ADVANCED_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, POA_ADVANCED_PROPERTY, NULL);
		indigo_set_timer(device, 0, handle_advanced_property, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- CCD_MODE
	} else if (indigo_property_match_changeable(CCD_MODE_PROPERTY, property)) {
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
		PIXEL_FORMAT_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, PIXEL_FORMAT_PROPERTY, NULL);
		CCD_FRAME_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, CCD_FRAME_PROPERTY, NULL);
		CCD_BIN_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, CCD_BIN_PROPERTY, NULL);
		CCD_MODE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, CCD_MODE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_BIN_PROPERTY, property)) {
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
		indigo_update_property(device, CCD_MODE_PROPERTY, NULL);
		indigo_update_property(device, CCD_BIN_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, PIXEL_FORMAT_PROPERTY);
			indigo_save_property(device, NULL, POA_ADVANCED_PROPERTY);
		}
	}
	return indigo_ccd_change_property(device, client, property);
}

static indigo_result ccd_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		handle_ccd_connect_property(device);
	}

	if (device == device->master_device)
		indigo_global_unlock(device);

	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);

	indigo_release_property(PIXEL_FORMAT_PROPERTY);
	indigo_release_property(POA_ADVANCED_PROPERTY);

	return indigo_ccd_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO guider device implementation

static indigo_result guider_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_guider_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		INFO_PROPERTY->count = 5;
		indigo_copy_value(INFO_DEVICE_MODEL_ITEM->text.value, PRIVATE_DATA->property.cameraModelName);
		return indigo_guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}


static void handle_guider_connection_property(indigo_device *device) {
	indigo_lock_master_device(device);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (!device->is_connected) {
			if (playerone_open(device)) {
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
			playerone_close(device);
			device->is_connected = false;
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	}
	indigo_guider_change_property(device, NULL, CONNECTION_PROPERTY);
	indigo_unlock_master_device(device);
}


static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	POAErrors res;
	int id = PRIVATE_DATA->dev_id;

	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, handle_guider_connection_property, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_DEC
		indigo_property_copy_values(GUIDER_GUIDE_DEC_PROPERTY, property, false);
		indigo_cancel_timer(device, &PRIVATE_DATA->guider_timer_dec);
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
		int duration = GUIDER_GUIDE_NORTH_ITEM->number.value;
		POAConfigValue value = { .boolValue = true };
		if (duration > 0) {
			pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
			res = POASetConfig(id, POA_GUIDE_NORTH, value, false);
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			if (res)
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "POASetConfig(%d, POA_GUIDE_NORTH, true, false) > %d", id, res);
			else
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POASetConfig(%d, POA_GUIDE_NORTH, true, false)", id);
			indigo_set_timer(device, duration/1000.0, guider_timer_callback_dec, &PRIVATE_DATA->guider_timer_dec);
			GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
		} else {
			int duration = GUIDER_GUIDE_SOUTH_ITEM->number.value;
			if (duration > 0) {
				pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
				res = POASetConfig(id, POA_GUIDE_SOUTH, value, false);
				pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
				if (res)
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "POASetConfig(%d, POA_GUIDE_SOUTH, true, false) > %d", id, res);
				else
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POASetConfig(%d, POA_GUIDE_SOUTH, true, false)", id);
				indigo_set_timer(device, duration/1000.0, guider_timer_callback_dec, &PRIVATE_DATA->guider_timer_dec);
				GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
			}
		}
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_RA
		indigo_property_copy_values(GUIDER_GUIDE_RA_PROPERTY, property, false);
		indigo_cancel_timer(device, &PRIVATE_DATA->guider_timer_ra);
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
		int duration = GUIDER_GUIDE_EAST_ITEM->number.value;
		POAConfigValue value = { .boolValue = true };
		if (duration > 0) {
			pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
			res = POASetConfig(id, POA_GUIDE_EAST, value, false);
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			if (res)
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "POASetConfig(%d, POA_GUIDE_EAST, true, false) > %d", id, res);
			else
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POASetConfig(%d, POA_GUIDE_EAST, true, false)", id);
			indigo_set_timer(device, duration/1000.0, guider_timer_callback_ra, &PRIVATE_DATA->guider_timer_ra);
			GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
		} else {
			int duration = GUIDER_GUIDE_WEST_ITEM->number.value;
			if (duration > 0) {
				pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
				res = POASetConfig(id, POA_GUIDE_WEST, value, false);
				pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
				if (res)
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "POASetConfig(%d, POA_GUIDE_WEST, true, false) > %d", id, res);
				else
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POASetConfig(%d, POA_GUIDE_WEST, true, false)", id);
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
	if (IS_CONNECTED) {
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


static indigo_device *devices[MAX_DEVICES] = { NULL };
static bool connected_ids[MAX_DEVICES] = { false };


static int find_index_by_device_id(int id) {
	POACameraProperties properties;
	int count = POAGetCameraCount();
	for (int i = 0; i < count; i++) {
		POAGetCameraProperties(i, &properties);
		if (properties.cameraID == id) return i;
	}
	return -1;
}


static int find_plugged_device_id() {
	int i, id = NO_DEVICE, new_id = NO_DEVICE;
	POACameraProperties properties;
	int count = POAGetCameraCount();
	for (i = 0; i < count; i++) {
		POAGetCameraProperties(i, &properties);
		id = properties.cameraID;
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
	bool dev_tmp[MAX_DEVICES] = { false };
	int i;
	POACameraProperties properties;

	int count = POAGetCameraCount();
	for (i = 0; i < count; i++) {
		POAGetCameraProperties(i, &properties);
		dev_tmp[properties.cameraID] = true;
	}

	int id = -1;
	for (i = 0; i < MAX_DEVICES; i++) {
		if (connected_ids[i] && !dev_tmp[i]){
			id = i;
			connected_ids[id] = false;
			break;
		}
	}
	return id;
}

static void process_plug_event(indigo_device *unused) {
	POACameraProperties property;

	static indigo_device ccd_template = INDIGO_DEVICE_INITIALIZER(
		"",
		ccd_attach,
		playerone_enumerate_properties,
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
	POAErrors res = POAGetCameraProperties(index, &property);
	if (res == POA_OK) {
		res = POAOpenCamera(property.cameraID);
		if (res == POA_OK) {
			POACloseCamera(property.cameraID);
		}
	}
	if (res == POA_OK) {
//		char *p = strstr(info.FriendlyName, "(CAM");
//		if (p != NULL)
//			*p = '\0';
		device->master_device = master_device;
		sprintf(device->name, "%s #%d", property.cameraModelName, id);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		playerone_private_data *private_data = indigo_safe_malloc(sizeof(playerone_private_data));
		private_data->dev_id = id;
		memcpy(&(private_data->property), &property, sizeof(POACameraProperties));
		device->private_data = private_data;
		indigo_attach_device(device);
		devices[slot]=device;
		if (property.isHasST4Port) {
			slot = find_available_device_slot();
			if (slot < 0) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "No device slots available.");
				pthread_mutex_unlock(&device_mutex);
				return;
			}
			device = indigo_safe_malloc_copy(sizeof(indigo_device), &guider_template);
			device->master_device = master_device;
			sprintf(device->name, "%s Guider #%d", property.cameraModelName, id);
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
	playerone_private_data *private_data = NULL;
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
			POACloseCamera(id);
			if (private_data->buffer != NULL) {
				free(private_data->buffer);
				private_data->buffer = NULL;
			}
			free(private_data);
			private_data = NULL;
		}
	}
	if (!removed) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "No POA Camera unplugged");
	}
	pthread_mutex_unlock(&device_mutex);
}

static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	struct libusb_device_descriptor descriptor;
	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			libusb_get_device_descriptor(dev, &descriptor);
			if (descriptor.idVendor == POA_VENDOR_ID)
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
	playerone_private_data *pds[MAX_DEVICES] = { NULL };

	for (i = 0; i < MAX_DEVICES; i++) {
		indigo_device *device = devices[i];
		if (device == NULL) continue;
		if (PRIVATE_DATA) pds[PRIVATE_DATA->dev_id] = PRIVATE_DATA; /* preserve pointers to private data */
		indigo_detach_device(device);
		free(device);
		devices[i] = NULL;
	}

	/* free private data */
	for (i = 0; i < MAX_DEVICES; i++) {
		if (pds[i]) {
			if (pds[i]->buffer != NULL) {
				POACloseCamera(pds[i]->dev_id);
				free(pds[i]->buffer);
				pds[i]->buffer = NULL;
			}
			free(pds[i]);
		}
	}

	for (i = 0; i < MAX_DEVICES; i++)
		connected_ids[i] = false;
}


static libusb_hotplug_callback_handle callback_handle;

indigo_result indigo_ccd_playerone(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "Player One Camera", __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;

			const char *sdk_version = POAGetSDKVersion();
			INDIGO_DRIVER_LOG(DRIVER_NAME, "POA SDK v. %s", sdk_version);

			indigo_start_usb_event_handler();
			int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, POA_VENDOR_ID, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
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

indigo_result indigo_ccd_playerone(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "Player One Camera", __FUNCTION__, DRIVER_VERSION, true, last_action);

	switch(action) {
		case INDIGO_DRIVER_INIT:
		case INDIGO_DRIVER_SHUTDOWN:
			INDIGO_DRIVER_LOG(DRIVER_NAME, "This driver is not supported on this architecture");
			return INDIGO_UNSUPPORTED_ARCH;
		case INDIGO_DRIVER_INFO:
			break;
	}
	return INDIGO_OK;
}

#endif
