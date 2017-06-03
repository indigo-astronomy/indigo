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
// 2.0 Build 0 - PoC by Rumen G. Bogdanovski


/** INDIGO QHY CCD driver
 \file indigo_ccd_qhy.cpp
 \NOTE: This file should be .cpp as qhy headers are in C++
 */

#define DRIVER_VERSION 0x0001
#define DRIVER_NAME "indigo_ccd_qhy"

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

#include "qhyccd.h"
#include "log4z.h"
#include "indigo_ccd_qhy.h"
#include "indigo_driver_xml.h"

#define MAX_SID_LEN                255
#define QHY_MAX_FORMATS            4

#define QHY_VENDOR_ID1             0x16c0
#define QHY_VENDOR_ID2             0x1618
#define QHY_VENDOR_ID3             0x1856
#define QHY_VENDOR_ID4             0x04b4
#define QHY_VENDOR_ID5             0x0547

#define CCD_ADVANCED_GROUP         "Advanced"

#define PRIVATE_DATA               ((qhy_private_data *)device->private_data)

#define PIXEL_FORMAT_PROPERTY      (PRIVATE_DATA->pixel_format_property)
#define RAW8_NAME                  "RAW 8"
#define RGB24_NAME                 "RGB 24"
#define RAW16_NAME                 "RAW 16"
#define Y8_NAME                    "Y 8"

#define QHY_ADVANCED_PROPERTY      (PRIVATE_DATA->qhy_advanced_property)

// gp_bits is used as boolean
#define is_connected               gp_bits

// -------------------------------------------------------------------------------- QHY USB interface implementation

#define us2s(s) ((s) / 1000000.0)
#define s2us(us) ((us) * 1000000)

typedef struct {
	uint32_t width;
	uint32_t height;
	uint32_t bpp;
} img_params;

typedef struct {
	qhyccd_handle *handle;
	char dev_sid[MAX_SID_LEN];
	int count_open;
	int count_connected;
	uint32_t total_frame_width;
	uint32_t total_frame_height;
	uint32_t bpp;
	uint32_t frame_offset_x;
	uint32_t frame_offset_y;
	uint32_t frame_width;
	uint32_t frame_height;
	double pixel_width;
	double pixel_height;
	img_params ci_params;

	indigo_timer *exposure_timer, *temperature_timer, *guider_timer_ra, *guider_timer_dec;
	double target_temperature, current_temperature;
	long cooler_power;
	bool guide_relays[4];
	unsigned char *buffer;
	long int buffer_size;
	pthread_mutex_t usb_mutex;
	bool can_check_temperature, has_temperature_sensor;
	indigo_property *pixel_format_property;
	indigo_property *qhy_advanced_property;
} qhy_private_data;


static char *get_bayer_string(indigo_device *device) {
	int pattern = IsQHYCCDControlAvailable(PRIVATE_DATA->handle, CAM_COLOR);
	if (pattern != QHYCCD_ERROR) {
		if(pattern == BAYER_GB)
			return (char*)"GBRG";
		else if (pattern == BAYER_GR)
			return (char*)"GRBG";
		else if (pattern == BAYER_BG)
			return (char*)"BGGR";
		else
			return (char*)"RGGB";
	}
	return NULL;
}


static int get_pixel_depth(indigo_device *device) {
	int item = 0;
	while (item < QHY_MAX_FORMATS) {
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
	/*
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
	} */
	return -1;
}


static bool pixel_format_supported(indigo_device *device, int type) {
	/*
	for (int i = 0; i < ASI_MAX_FORMATS; i++) {
		if (i == ASI_IMG_END) return false;&PRIVATE_DATA->total_frame_width,
			&PRIVATE_DATA->total_frame_height,
		if (type == PRIVATE_DATA->info.SupportedVideoFormat[i]) return true;
	}
	*/
	return false;
}


static indigo_result qhy_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(PIXEL_FORMAT_PROPERTY, property))
			indigo_define_property(device, PIXEL_FORMAT_PROPERTY, NULL);
		if (indigo_property_match(QHY_ADVANCED_PROPERTY, property))
			indigo_define_property(device, QHY_ADVANCED_PROPERTY, NULL);
	}
	return indigo_ccd_enumerate_properties(device, NULL, NULL);
}


static bool qhy_open(indigo_device *device) {
	int res;

	if (device->is_connected) return false;

	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	if (PRIVATE_DATA->count_open++ == 0) {
		/* UGLY KLUDGE !!!
		   QHY5LII segfaults at second open-close cycle after&PRIVATE_DATA->total_frame_width,
			&PRIVATE_DATA->total_frame_height, scan
		   this way there will never be second rescan. HA-HA-HA...
		   However this comes at a proce of 378kb memory leak per
		   connected caera per scan.
		*/
		ScanQHYCCD();
		PRIVATE_DATA->handle = OpenQHYCCD(PRIVATE_DATA->dev_sid);
		if (PRIVATE_DATA->handle == NULL) {
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "OpenQHYCCD('%s') = NULL", PRIVATE_DATA->dev_sid);
			PRIVATE_DATA->count_open--;
			return false;
		}

		/* Disable the stream mode */
		res = SetQHYCCDStreamMode(PRIVATE_DATA->handle, 0);
		if (res != QHYCCD_SUCCESS) {
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetQHYCCDStreamMode('%s') = %d", PRIVATE_DATA->dev_sid, res);
			PRIVATE_DATA->count_open--;
			return false;
		}
		InitQHYCCD(PRIVATE_DATA->handle);

		double chipw, chiph;
		res = GetQHYCCDChipInfo(
			PRIVATE_DATA->handle,
			&chipw,
			&chiph,
			&PRIVATE_DATA->total_frame_width,
			&PRIVATE_DATA->total_frame_height,
			&PRIVATE_DATA->pixel_width,
			&PRIVATE_DATA->pixel_height,
			&PRIVATE_DATA->bpp
		);
		if (res != QHYCCD_SUCCESS) {
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Can not open camera: GetQHYCCDChipInfo('%s') = %d", PRIVATE_DATA->dev_sid, res);
			PRIVATE_DATA->count_open--;
			return false;
		}

		res = GetQHYCCDEffectiveArea(
			PRIVATE_DATA->handle,
			&PRIVATE_DATA->frame_offset_x,
			&PRIVATE_DATA->frame_offset_y,
			&PRIVATE_DATA->frame_width,
			&PRIVATE_DATA->frame_height
		);
		if (res != QHYCCD_SUCCESS) {
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Can not open camera: GetQHYCCDEffectiveArea('%s') = %d", PRIVATE_DATA->dev_sid, res);
			PRIVATE_DATA->count_open--;
			return false;
		}
		/* kludge: GetQHYCCDEffectiveArea() is not implemented for most of the cmeras so use full frame */
		if ((PRIVATE_DATA->frame_width == 0) || (PRIVATE_DATA->frame_height == 0)) {
			PRIVATE_DATA->frame_width = PRIVATE_DATA->total_frame_width;
			PRIVATE_DATA->frame_height = PRIVATE_DATA->total_frame_height;
		}

		INDIGO_DRIVER_ERROR(DRIVER_NAME,
			"Open %s: %dx%d (%d,%d) %.2fx%.2fum %dbpp handle = %p\n",
			PRIVATE_DATA->dev_sid,
			PRIVATE_DATA->frame_width,
			PRIVATE_DATA->frame_height,
			PRIVATE_DATA->frame_offset_x,
			PRIVATE_DATA->frame_offset_y,
			PRIVATE_DATA->pixel_width,
			PRIVATE_DATA->pixel_height,
			PRIVATE_DATA->bpp,
			PRIVATE_DATA->handle
		);

		if (PRIVATE_DATA->buffer == NULL) {
			PRIVATE_DATA->buffer_size = PRIVATE_DATA->frame_height * PRIVATE_DATA->frame_width * 2 + FITS_HEADER_SIZE;
			PRIVATE_DATA->buffer = (unsigned char*)indigo_alloc_blob_buffer(PRIVATE_DATA->buffer_size);
		}
	}
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	return true;
}

static bool qhy_setup_exposure(indigo_device *device, double exposure, int frame_left, int frame_top, int frame_width, int frame_height, int horizontal_bin, int vertical_bin) {
	int res;
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);

	res = SetQHYCCDParam(PRIVATE_DATA->handle, CONTROL_EXPOSURE, (long)s2us(exposure));
	if (res != QHYCCD_SUCCESS) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetQHYCCDParam(%s) = %d", PRIVATE_DATA->dev_sid, res);
		return false;
	}

	res = SetQHYCCDBinMode(PRIVATE_DATA->handle, horizontal_bin, vertical_bin);
	if (res != QHYCCD_SUCCESS) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetQHYCCDBinMode(%s) = %d", PRIVATE_DATA->dev_sid, res);
		return false;
	}

	res = SetQHYCCDResolution(PRIVATE_DATA->handle, frame_left/horizontal_bin, frame_top/vertical_bin, frame_width/horizontal_bin, frame_height/vertical_bin);
	if (res != QHYCCD_SUCCESS) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetQHYCCDResolution(%s) = %d", PRIVATE_DATA->dev_sid, res);
		return false;
	}

	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	return true;
}

static bool qhy_start_exposure(indigo_device *device, double exposure, bool dark, int frame_left, int frame_top, int frame_width, int frame_height, int horizontal_bin, int vertical_bin) {
	int res;
	if (!qhy_setup_exposure(device, exposure, frame_left, frame_top, frame_width, frame_height, horizontal_bin, vertical_bin)) {
		return false;
	}
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	res = ExpQHYCCDSingleFrame(PRIVATE_DATA->handle);
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	if (res != QHYCCD_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ExpQHYCCDSingleFrame(%s) = %d", PRIVATE_DATA->dev_sid, res);
		return false;
	}
	return true;
}

static bool qhy_read_pixels(indigo_device *device) {
	int res;
	uint32_t ret,channels;

	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	int remaining = GetQHYCCDExposureRemaining(PRIVATE_DATA->handle);
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);

	while (remaining > 100) {
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		remaining = GetQHYCCDExposureRemaining(PRIVATE_DATA->handle);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		usleep(2000);
	}

	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	res = GetQHYCCDSingleFrame(
		PRIVATE_DATA->handle,
		&PRIVATE_DATA->ci_params.width,
		&PRIVATE_DATA->ci_params.height,
		&PRIVATE_DATA->ci_params.bpp,
		&channels,
		PRIVATE_DATA->buffer + FITS_HEADER_SIZE
	);
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	if (res != QHYCCD_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "GetQHYCCDSingleFrame(%s) = %d", PRIVATE_DATA->dev_sid, res);
		return false;
	}
    return true;
}

static bool qhy_abort_exposure(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	int err = CancelQHYCCDExposingAndReadout(PRIVATE_DATA->handle);
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	if (err !=  QHYCCD_SUCCESS) return false;
	else return true;
}

static bool qhy_set_cooler(indigo_device *device, bool status, double target, double *current, long *cooler_power) {
	//ASI_ERROR_CODE res;
	//ASI_BOOL unused;

	//int id = PRIVATE_DATA->dev_id;
	long current_status;
	long temp_x10;

	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	/*
	if (PRIVATE_DATA->has_temperature_sensor) {
		res = ASIGetControlValue(id, ASI_TEMPERATURE, &temp_x10, &unused);
		if(res) INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASIGetControlValue(%d, ASI_TEMPERATURE) = %d", id, res);
		*current = temp_x10/10.0;
	} else {
		*current = 0;
	}

	if (!PRIVATE_DATA->info.IsCoolerCam) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		return true;
	}

	res = ASIGetControlValue(id, ASI_COOLER_ON, &current_status, &unused);
	if(res) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASIGetControlValue(%d, ASI_COOLER_ON) = %d", id, res);
		return false;
	}

	if (current_status != status) {
		res = ASISetControlValue(id, ASI_COOLER_ON, status, false);
		if(res) INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASISetControlValue(%d, ASI_COOLER_ON) = %d", id, res);
	} else if(status) {
		res = ASISetControlValue(id, ASI_TARGET_TEMP, (long)target, false);
		if(res) INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASISetControlValue(%d, ASI_TARGET_TEMP) = %d", id, res);
	}

	res = ASIGetControlValue(id, ASI_COOLER_POWER_PERC, cooler_power, &unused);
	if(res) INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASIGetControlValue(%d, ASI_COOLER_POWER_PERC) = %d", id, res);
	*/
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	return true;
}


static void qhy_close(indigo_device *device) {

	if (!device->is_connected) return;

	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	if (--PRIVATE_DATA->count_open == 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Close %s: handle = %p\n", PRIVATE_DATA->dev_sid, PRIVATE_DATA->handle);
		if (PRIVATE_DATA->handle) {
			CloseQHYCCD(PRIVATE_DATA->handle);
			PRIVATE_DATA->handle = NULL;
		}
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
		if (qhy_read_pixels(device)) {
			char *color_string = get_bayer_string(device);
			if(color_string) {
				/* NOTE: There is no need to take care about the offsets,
				   the SDK takes care the image to be in the correct bayer pattern */
				indigo_fits_keyword keywords[] = {
					{ .type = INDIGO_FITS_STRING, .name = "BAYERPAT", {.string = color_string }, .comment = "Bayer color pattern" },
					{ .type = INDIGO_FITS_NUMBER, .name = "XBAYROFF", {.number = 0 }, .comment = "X offset of Bayer array" },
					{ .type = INDIGO_FITS_NUMBER, .name = "YBAYROFF", {.number = 0 }, .comment = "Y offset of Bayer array" },
					{ .type = (indigo_fits_keyword_type)0 }
				};
				if ((CCD_BIN_HORIZONTAL_ITEM->number.value == 1) &&
					(CCD_BIN_VERTICAL_ITEM->number.value == 1)) {
					keywords[1].number = ((int)(CCD_FRAME_LEFT_ITEM->number.value) % 2) ? 1 : 0; /* set XBAYROFF */
					keywords[2].number = ((int)(CCD_FRAME_TOP_ITEM->number.value) % 2) ? 1 : 0; /* set YBAYROFF */
				}
				indigo_process_image(device, PRIVATE_DATA->buffer, PRIVATE_DATA->ci_params.width, PRIVATE_DATA->ci_params.height, true, keywords);
			} else {
				indigo_process_image(device, PRIVATE_DATA->buffer, PRIVATE_DATA->ci_params.width, PRIVATE_DATA->ci_params.height, true, NULL);
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
	//indigo_fits_keyword keywords[] = {
	//	{ INDIGO_FITS_STRING, "BAYERPAT", .string = color_string, "Bayer color pattern" },
	//	{ INDIGO_FITS_NUMBER, "XBAYROFF", .number = 0, "X offset of Bayer array" },
	//	{ INDIGO_FITS_NUMBER, "YBAYROFF", .number = 0, "Y offset of Bayer array" },
	//	{ 0 }
	//};
	int timeout = 1000 * (CCD_STREAMING_EXPOSURE_ITEM->number.value * 2 + 500);
	int res;
	PRIVATE_DATA->can_check_temperature = true;
	if (qhy_setup_exposure(device, CCD_STREAMING_EXPOSURE_ITEM->number.value, CCD_FRAME_LEFT_ITEM->number.value, CCD_FRAME_TOP_ITEM->number.value, CCD_FRAME_WIDTH_ITEM->number.value, CCD_FRAME_HEIGHT_ITEM->number.value, CCD_BIN_HORIZONTAL_ITEM->number.value, CCD_BIN_VERTICAL_ITEM->number.value)) {
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		/*
		res = ASIStartVideoCapture(id);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASIStartVideoCapture(%d) = %d", id, res);
		} else {
			while (CCD_STREAMING_COUNT_ITEM->number.value != 0) {
				res = ASIGetVideoData(id, PRIVATE_DATA->buffer + FITS_HEADER_SIZE, PRIVATE_DATA->buffer_size, timeout);
				if (res) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASIGetVideoData((%d) = %d", id, res);
					break;
				}
				if(color_string) {
					indigo_process_image(device, PRIVATE_DATA->buffer, (int)(CCD_FRAME_WIDTH_ITEM->number.value / CCD_BIN_HORIZONTAL_ITEM->number.value), (int)(CCD_FRAME_HEIGHT_ITEM->number.value / CCD_BIN_VERTICAL_ITEM->number.value), true, keywords);
				} else {
					indigo_process_image(device, PRIVATE_DATA->buffer, (int)(CCD_FRAME_WIDTH_ITEM->number.value / CCD_BIN_HORIZONTAL_ITEM->number.value), (int)(CCD_FRAME_HEIGHT_ITEM->number.value / CCD_BIN_VERTICAL_ITEM->number.value), true, NULL);
				}
				if (CCD_STREAMING_COUNT_ITEM->number.value > 0)
					CCD_STREAMING_COUNT_ITEM->number.value -= 1;
				CCD_STREAMING_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
			}
			res = ASIStopVideoCapture(id);
			if (res)
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASIStopVideoCapture(%d) = %d", id, res);
		}
		*/
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	} else {
		// res = ASI_ERROR_GENERAL_ERROR;
	}
	PRIVATE_DATA->can_check_temperature = false;
	if (res)
		CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
	else
		CCD_STREAMING_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
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
		if (qhy_set_cooler(device, CCD_COOLER_ON_ITEM->sw.value, PRIVATE_DATA->target_temperature, &PRIVATE_DATA->current_temperature, &PRIVATE_DATA->cooler_power)) {
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

	if (!CONNECTION_CONNECTED_ITEM->sw.value) return;
	/*
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
	*/
}


static void guider_timer_callback_dec(indigo_device *device) {
	PRIVATE_DATA->guider_timer_dec = NULL;

	if (!CONNECTION_CONNECTED_ITEM->sw.value) return;
	/*
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
	*/
}


static indigo_result ccd_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_ccd_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		pthread_mutex_init(&PRIVATE_DATA->usb_mutex, NULL);
		// -------------------------------------------------------------------------------- PIXEL_FORMAT_PROPERTY
		PIXEL_FORMAT_PROPERTY = indigo_init_switch_property(NULL, device->name, "PIXEL_FORMAT", CCD_ADVANCED_GROUP, "Pixel Format", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, QHY_MAX_FORMATS);
		if (PIXEL_FORMAT_PROPERTY == NULL)
			return INDIGO_FAILED;

		int format_count = 0;
		/*
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
		*/
		PIXEL_FORMAT_PROPERTY->count = format_count;
		int mode_count = 0;
		char name[32], label[64];
		/*
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
		*/
		CCD_MODE_PROPERTY->count = mode_count;
		// -------------------------------------------------------------------------------- CCD_STREAMING
		CCD_STREAMING_PROPERTY->hidden = true;
		CCD_STREAMING_EXPOSURE_ITEM->number.max = 4.0;
		// -------------------------------------------------------------------------------- ASI_ADVANCED
		QHY_ADVANCED_PROPERTY = indigo_init_number_property(NULL, device->name, "ASI_ADVANCED", CCD_ADVANCED_GROUP, "Advanced", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 0);
		if (QHY_ADVANCED_PROPERTY == NULL)
			return INDIGO_FAILED;
		// --------------------------------------------------------------------------------
		return indigo_ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result handle_advanced_property(indigo_device *device, indigo_property *property) {
	int ctrl_count;
	//ASI_CONTROL_CAPS ctrl_caps;
	//ASI_ERROR_CODE res;

	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);

	/*
	res = ASIGetNumOfControls(id, &ctrl_count);
	if (res) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASIGetNumOfControls(%d) = %d", id, res);
		return INDIGO_NOT_FOUND;
	}

	for(int ctrl_no = 0; ctrl_no < ctrl_count; ctrl_no++) {
		ASIGetControlCaps(id, ctrl_no, &ctrl_caps);
		for(int item = 0; item < property->count; item++) {
			if(!strncmp(ctrl_caps.Name, property->items[item].name, INDIGO_NAME_SIZE)) {
				res = ASISetControlValue(id, ctrl_caps.ControlType,property->items[item].number.value, ASI_FALSE);
				if (res) INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASISetControlValue(%d, %s) = %d", id, ctrl_caps.Name, res);
			}
		}
	}
	*/
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	return INDIGO_OK;
}

/*
static indigo_result init_camera_property(indigo_device *device, ... ctrl_caps) {

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
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		res = ASIGetControlValue(id, ASI_GAIN, &value, &unused);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if (res) INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASIGetControlValue(%d, ASI_GAIN) = %d", id, res);
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
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		res = ASIGetControlValue(id, ASI_GAMMA, &value, &unused);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if (res) INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASIGetControlValue(%d, ASI_GAMMA) = %d", id, res);
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
		CCD_COOLER_POWER_ITEM->number.value = value;
		return INDIGO_OK;
	}

	int offset = QHY_ADVANCED_PROPERTY->count;
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	res = ASISetControlValue(id, ctrl_caps.ControlType, ctrl_caps.DefaultValue, false);
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	if (res)
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASISetControlValue(%d, %s) = %d", id, ctrl_caps.Name, res);
	else {
		QHY_ADVANCED_PROPERTY = indigo_resize_property(QHY_ADVANCED_PROPERTY, offset + 1);
		indigo_init_number_item(QHY_ADVANCED_PROPERTY->items+offset, ctrl_caps.Name, ctrl_caps.Name, ctrl_caps.MinValue, ctrl_caps.MaxValue, 1, ctrl_caps.DefaultValue);
	}

	return INDIGO_OK;
}
*/

static indigo_result ccd_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);

	// -------------------------------------------------------------------------------- CONNECTION -> CCD_INFO, CCD_COOLER, CCD_TEMPERATURE
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			if (!device->is_connected) {
				if (qhy_open(device)) {
					int ctrl_count;
					//ASI_CONTROL_CAPS ctrl_caps;

					indigo_define_property(device, PIXEL_FORMAT_PROPERTY, NULL);

					pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
					int res; // = ASIGetNumOfControls(id, &ctrl_count);
					pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
					if (res) {
						//INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASIGetNumOfControls(%d) = %d", id, res);
						return INDIGO_NOT_FOUND;
					}
					QHY_ADVANCED_PROPERTY = indigo_resize_property(QHY_ADVANCED_PROPERTY, 0);
					for(int ctrl_no = 0; ctrl_no < ctrl_count; ctrl_no++) {
						pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
						//ASIGetControlCaps(id, ctrl_no, &ctrl_caps);
						pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
						//init_camera_property(device, ctrl_caps);
					}
					indigo_define_property(device, QHY_ADVANCED_PROPERTY, NULL);

					CCD_INFO_WIDTH_ITEM->number.value = PRIVATE_DATA->frame_width;
					CCD_INFO_HEIGHT_ITEM->number.value = PRIVATE_DATA->frame_height;
					CCD_INFO_PIXEL_SIZE_ITEM->number.value = PRIVATE_DATA-> pixel_width;
					CCD_INFO_PIXEL_WIDTH_ITEM->number.value = PRIVATE_DATA->pixel_width;
					CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = PRIVATE_DATA->pixel_height;
					CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = PRIVATE_DATA->bpp;

					CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max = PRIVATE_DATA->frame_width;
					CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max = PRIVATE_DATA->frame_height;
					CCD_FRAME_BITS_PER_PIXEL_ITEM->number.min = 8;
					CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = PRIVATE_DATA->bpp;
					CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = 16;

					/* find max binning */
					int max_bin = 4;
					//for (int num = 0; (num < 16) && PRIVATE_DATA->info.SupportedBins[num]; num++) {
					//	max_bin = PRIVATE_DATA->info.SupportedBins[num];
					//}

					CCD_BIN_PROPERTY->perm = INDIGO_RW_PERM;
					CCD_BIN_HORIZONTAL_ITEM->number.value = CCD_BIN_HORIZONTAL_ITEM->number.min = 1;
					CCD_BIN_HORIZONTAL_ITEM->number.max = max_bin;
					CCD_BIN_VERTICAL_ITEM->number.value = CCD_BIN_VERTICAL_ITEM->number.min = 1;
					CCD_BIN_VERTICAL_ITEM->number.max = max_bin;

					CCD_INFO_MAX_HORIZONAL_BIN_ITEM->number.value = max_bin;
					CCD_INFO_MAX_VERTICAL_BIN_ITEM->number.value = max_bin;

					if (PRIVATE_DATA->has_temperature_sensor) {
						PRIVATE_DATA->temperature_timer = indigo_set_timer(device, 0, ccd_temperature_callback);
					}

					device->is_connected = true;
					CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
				} else {
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
				}
			}
		} else {
			if(device->is_connected) {
				PRIVATE_DATA->can_check_temperature = false;
				indigo_cancel_timer(device, &PRIVATE_DATA->temperature_timer);
				indigo_delete_property(device, PIXEL_FORMAT_PROPERTY, NULL);
				indigo_delete_property(device, QHY_ADVANCED_PROPERTY, NULL);
				qhy_close(device);
				device->is_connected = false;
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			}
		}
		// -------------------------------------------------------------------------------- CCD_EXPOSURE
	} else if (indigo_property_match(CCD_EXPOSURE_PROPERTY, property)) {
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE)
			return INDIGO_OK;
		indigo_property_copy_values(CCD_EXPOSURE_PROPERTY, property, false);
		qhy_start_exposure(device, CCD_EXPOSURE_ITEM->number.target, CCD_FRAME_TYPE_DARK_ITEM->sw.value, CCD_FRAME_LEFT_ITEM->number.value, CCD_FRAME_TOP_ITEM->number.value, CCD_FRAME_WIDTH_ITEM->number.value, CCD_FRAME_HEIGHT_ITEM->number.value, CCD_BIN_HORIZONTAL_ITEM->number.value, CCD_BIN_VERTICAL_ITEM->number.value);
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
	} else if (indigo_property_match(CCD_STREAMING_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_STREAMING
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE)
			return INDIGO_OK;
		indigo_property_copy_values(CCD_STREAMING_PROPERTY, property, false);
		CCD_STREAMING_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
		if (CCD_UPLOAD_MODE_LOCAL_ITEM->sw.value) {
			CCD_IMAGE_FILE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
		} else {
			CCD_IMAGE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
		}
		PRIVATE_DATA->exposure_timer = indigo_set_timer(device, 0, streaming_timer_callback);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- CCD_ABORT_EXPOSURE
	} else if (indigo_property_match(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_cancel_timer(device, &PRIVATE_DATA->exposure_timer);
			qhy_abort_exposure(device);
		} else if (CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE && CCD_STREAMING_COUNT_ITEM->number.value != 0) {
			CCD_STREAMING_COUNT_ITEM->number.value = 0;
		}
		PRIVATE_DATA->can_check_temperature = true;
		indigo_property_copy_values(CCD_ABORT_EXPOSURE_PROPERTY, property, false);
		// -------------------------------------------------------------------------------- CCD_COOLER
	} else if (indigo_property_match(CCD_COOLER_PROPERTY, property)) {
		//INDIGO_DRIVER_ERROR(DRIVER_NAME, "COOOLER = %d %d", CCD_COOLER_OFF_ITEM->sw.value, CCD_COOLER_ON_ITEM->sw.value));
		indigo_property_copy_values(CCD_COOLER_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value && !CCD_COOLER_PROPERTY->hidden) {
			CCD_COOLER_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
		}
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- CCD_TEMPERATURE
	} else if (indigo_property_match(CCD_TEMPERATURE_PROPERTY, property)) {
		indigo_property_copy_values(CCD_TEMPERATURE_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value && !CCD_COOLER_PROPERTY->hidden) {
			PRIVATE_DATA->target_temperature = CCD_TEMPERATURE_ITEM->number.value;
			CCD_TEMPERATURE_ITEM->number.value = PRIVATE_DATA->current_temperature;
			/* if (CCD_COOLER_OFF_ITEM->sw.value) {
				indigo_set_switch(CCD_COOLER_PROPERTY, CCD_COOLER_ON_ITEM, true);
				CCD_COOLER_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
			 } */
			CCD_TEMPERATURE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, "Target Temperature = %.2f", PRIVATE_DATA->target_temperature);
		}
		return INDIGO_OK;
		// ------------------------------------------------------------------------------- GAMMA
	} else if (indigo_property_match(CCD_GAMMA_PROPERTY, property)) {
		CCD_GAMMA_PROPERTY->state = INDIGO_IDLE_STATE;
		indigo_property_copy_values(CCD_GAMMA_PROPERTY, property, false);

		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		int res; // = ASISetControlValue(PRIVATE_DATA->dev_id, ASI_GAMMA, (long)(CCD_GAMMA_ITEM->number.value), ASI_FALSE);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		//if (res) INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASISetControlValue(%d, ASI_GAMMA) = %d", PRIVATE_DATA->dev_id, res);

		CCD_GAMMA_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_CONNECTED)
			indigo_update_property(device, CCD_GAMMA_PROPERTY, NULL);
		return INDIGO_OK;
		// ------------------------------------------------------------------------------- GAIN
	} else if (indigo_property_match(CCD_GAIN_PROPERTY, property)) {
		CCD_GAIN_PROPERTY->state = INDIGO_IDLE_STATE;
		indigo_property_copy_values(CCD_GAIN_PROPERTY, property, false);

		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		int res; // = ASISetControlValue(PRIVATE_DATA->dev_id, ASI_GAIN, (long)(CCD_GAIN_ITEM->number.value), ASI_FALSE);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		//if (res) INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASISetControlValue(%d, ASI_GAIN) = %d", PRIVATE_DATA->dev_id, res);

		CCD_GAIN_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_CONNECTED)
			indigo_update_property(device, CCD_GAIN_PROPERTY, NULL);
		return INDIGO_OK;
		// ------------------------------------------------------------------------------- CCD_FRAME
	} else if (indigo_property_match(CCD_FRAME_PROPERTY, property)) {
		indigo_property_copy_values(CCD_FRAME_PROPERTY, property, false);
		//CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.target = 8 * (int)(CCD_FRAME_WIDTH_ITEM->number.value / 8);
		//CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.target = 2 * (int)(CCD_FRAME_HEIGHT_ITEM->number.value / 2);
		if (CCD_FRAME_WIDTH_ITEM->number.value / CCD_BIN_HORIZONTAL_ITEM->number.value < 64)
			CCD_FRAME_WIDTH_ITEM->number.value = 64 * CCD_BIN_HORIZONTAL_ITEM->number.value;
		if (CCD_FRAME_HEIGHT_ITEM->number.value / CCD_BIN_VERTICAL_ITEM->number.value < 64)
			CCD_FRAME_HEIGHT_ITEM->number.value = 64 * CCD_BIN_VERTICAL_ITEM->number.value;
		CCD_FRAME_PROPERTY->state = INDIGO_OK_STATE;
		CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = (CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value > 8) ? 16 : 8;
		if (IS_CONNECTED)
			indigo_update_property(device, CCD_FRAME_PROPERTY, NULL);
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

		CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = (CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value > 8) ? 16 : 8;
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
	} else if (indigo_property_match(QHY_ADVANCED_PROPERTY, property)) {
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
			QHY_ADVANCED_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, QHY_ADVANCED_PROPERTY, "Exposure in progress, advanced settings can not be changed.");
			return INDIGO_OK;
		}
		handle_advanced_property(device, property);
		indigo_property_copy_values(QHY_ADVANCED_PROPERTY, property, false);
		QHY_ADVANCED_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_CONNECTED)
			indigo_update_property(device, QHY_ADVANCED_PROPERTY, NULL);
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
		CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.min = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = get_pixel_depth(device);
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
			indigo_save_property(device, NULL, QHY_ADVANCED_PROPERTY);
		}
	}
	return indigo_ccd_change_property(device, client, property);
}

static indigo_result ccd_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);

	INDIGO_DRIVER_LOG(DRIVER_NAME, "'%s' detached.", device->name);

	indigo_release_property(PIXEL_FORMAT_PROPERTY);
	indigo_release_property(QHY_ADVANCED_PROPERTY);

	return indigo_ccd_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO guider device implementation

static indigo_result guider_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_guider_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		//INDIGO_DRIVER_LOG(DRIVER_NAME, "%s attached", device->name);
		return indigo_guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	int res;

	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			if (!device->is_connected) {
				if (qhy_open(device)) {
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
				qhy_close(device);
				device->is_connected = false;
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			}
		}
	} else if (indigo_property_match(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_DEC
		indigo_property_copy_values(GUIDER_GUIDE_DEC_PROPERTY, property, false);
		indigo_cancel_timer(device, &PRIVATE_DATA->guider_timer_dec);
		int duration = GUIDER_GUIDE_NORTH_ITEM->number.value;
		if (duration > 0) {
			pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
			//res = ASIPulseGuideOn(id, ASI_GUIDE_NORTH);
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);

			//if (res) INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASIPulseGuideOn(%d, ASI_GUIDE_NORTH) = %d", id, res);
			PRIVATE_DATA->guider_timer_dec = indigo_set_timer(device, duration/1000.0, guider_timer_callback_dec);
			//PRIVATE_DATA->guide_relays[ASI_GUIDE_NORTH] = true;
		} else {
			int duration = GUIDER_GUIDE_SOUTH_ITEM->number.value;
			if (duration > 0) {
				pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
				//res = ASIPulseGuideOn(id, ASI_GUIDE_SOUTH);
				pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);

				//if (res) INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASIPulseGuideOn(%d, ASI_GUIDE_SOUTH) = %d", id, res);
				PRIVATE_DATA->guider_timer_dec = indigo_set_timer(device, duration/1000.0, guider_timer_callback_dec);
				//PRIVATE_DATA->guide_relays[ASI_GUIDE_SOUTH] = true;
			}
		}

		//if (PRIVATE_DATA->guide_relays[ASI_GUIDE_SOUTH] || PRIVATE_DATA->guide_relays[ASI_GUIDE_NORTH])
		//	GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
		//else
		//	GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;

		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDER_GUIDE_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_RA
		indigo_property_copy_values(GUIDER_GUIDE_RA_PROPERTY, property, false);
		indigo_cancel_timer(device, &PRIVATE_DATA->guider_timer_ra);
		int duration = GUIDER_GUIDE_EAST_ITEM->number.value;
		if (duration > 0) {
			pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
			//res = ASIPulseGuideOn(id, ASI_GUIDE_EAST);
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);

			//if (res) INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASIPulseGuideOn(%d, ASI_GUIDE_EAST) = %d", id, res);
			PRIVATE_DATA->guider_timer_ra = indigo_set_timer(device, duration/1000.0, guider_timer_callback_ra);
			//PRIVATE_DATA->guide_relays[ASI_GUIDE_EAST] = true;
		} else {
			int duration = GUIDER_GUIDE_WEST_ITEM->number.value;
			if (duration > 0) {
				pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
				//res = ASIPulseGuideOn(id, ASI_GUIDE_WEST);
				pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);

				//if (res) INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASIPulseGuideOn(%d, ASI_GUIDE_WEST) = %d", id, res);
				PRIVATE_DATA->guider_timer_ra = indigo_set_timer(device, duration/1000.0, guider_timer_callback_ra);
				//PRIVATE_DATA->guide_relays[ASI_GUIDE_WEST] = true;
			}
		}

		//if (PRIVATE_DATA->guide_relays[ASI_GUIDE_EAST] || PRIVATE_DATA->guide_relays[ASI_GUIDE_WEST])
		//	GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
		//else
		//	GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;

		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_guider_change_property(device, client, property);
}

static indigo_result guider_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);
	INDIGO_DRIVER_LOG(DRIVER_NAME, "'%s' detached.", device->name);
	return indigo_guider_detach(device);
}

// -------------------------------------------------------------------------------- hot-plug support

static pthread_mutex_t device_mutex = PTHREAD_MUTEX_INITIALIZER;

#define MAX_DEVICES                   32
#define NOT_FOUND                    (-1)

static indigo_device *devices[MAX_DEVICES] = {NULL};

bool get_usb_path_str(qhyccd_handle *handle, char *path) {
	libusb_device* device = libusb_get_device(handle);
	uint8_t data[10];
	char buf[10];
	int i;

	data[0]=libusb_get_bus_number(device);
	int n = libusb_get_port_numbers(device, &data[1], 10);
	if (n != LIBUSB_ERROR_OVERFLOW) {
		sprintf(path,"%x", data[0]);
		for (i = 1; i <= n; i++) {
			sprintf(buf, "%d", data[i]);
			strcat(path, ".");
			strcat(path, buf);
		}
	} else {
		path[0] = '\0';
		return false;
	}
	return true;
}

static bool find_plugged_device_sid(char *new_sid) {
	int i;
	char sid[MAX_SID_LEN] = {0};
	bool found = false;

	int count = ScanQHYCCD();
	for(i = 0; i < count; i++) {
		GetQHYCCDId(i, sid);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME,"+ %d of %d: %s", i , count, sid);
		found = false;
		for(int slot = 0; slot < MAX_DEVICES; slot++) {
			indigo_device *device = devices[slot];
			if (device == NULL) continue;
			if (PRIVATE_DATA && (!strncmp(PRIVATE_DATA->dev_sid, sid, MAX_SID_LEN))) {
				found = true;
				break;
			}
		}

		if (!found) {
			strncpy(new_sid, sid, MAX_SID_LEN);
			return true;
		}
	}
	new_sid[0] = '\0';
	return false;
}


static int find_available_device_slot() {
	for(int slot = 0; slot < MAX_DEVICES; slot++) {
		if (devices[slot] == NULL) return slot;
	}
	return NOT_FOUND;
}


static int find_device_slot(const char *sid) {
	for(int slot = 0; slot < MAX_DEVICES; slot++) {
		indigo_device *device = devices[slot];
		if (device == NULL) continue;
		if (!strncmp(PRIVATE_DATA->dev_sid, sid, MAX_SID_LEN)) return slot;
	}
	return NOT_FOUND;
}


static int find_unplugged_device_slot() {
	int slot;
	indigo_device *device;
	char sid[MAX_SID_LEN] = {0};
	bool found = true;

	int count = ScanQHYCCD();
	for(slot = 0; slot < MAX_DEVICES; slot++) {
		device = devices[slot];
		if (device == NULL) continue;
		found = false;
		for(int i = 0; i < count; i++) {
			GetQHYCCDId(i, sid);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME,"- %d of %d: %s", i , count, sid);
			if (PRIVATE_DATA && (!strncmp(PRIVATE_DATA->dev_sid, sid, MAX_SID_LEN))) {
				found = true;
				break;
			}
		}
		if (!found) return slot;
	}
	return NOT_FOUND;
}


static void process_plug_event() {
	static indigo_device ccd_template = {
		"", false, NULL, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
		ccd_attach,
		qhy_enumerate_properties,
		ccd_change_property,
		ccd_detach
	};
	static indigo_device guider_template = {
		"", false, NULL, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
		guider_attach,
		indigo_guider_enumerate_properties,
		guider_change_property,
		guider_detach
	};

	int slot = find_available_device_slot();
	if (slot < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "No device slots available.");
		return;
	}

	char sid[MAX_SID_LEN];
	bool found = find_plugged_device_sid(sid);
	if (!found) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "No plugged device found.");
		return;
	}

	char dev_usbpath[MAX_SID_LEN];
	char dev_name[MAX_SID_LEN];
	GetQHYCCDModel(sid, dev_name);

	/* Check if there is a guider port and get usbpath */
	qhyccd_handle *handle;
	handle = OpenQHYCCD(sid);
	if(handle == NULL) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Camera %s can not be open.", sid);
		return;
	}
	int check_st4 = IsQHYCCDControlAvailable(handle, CONTROL_ST4PORT);
	get_usb_path_str(handle, dev_usbpath);
	CloseQHYCCD(handle);

	indigo_device *device = (indigo_device*)malloc(sizeof(indigo_device));
	assert(device != NULL);
	memcpy(device, &ccd_template, sizeof(indigo_device));
	sprintf(device->name, "%s #%s", dev_name, dev_usbpath);
	INDIGO_DRIVER_LOG(DRIVER_NAME, "'%s' attached.", device->name);
	qhy_private_data *private_data = (qhy_private_data*)malloc(sizeof(qhy_private_data));
	assert(private_data);
	memset(private_data, 0, sizeof(qhy_private_data));
	sprintf(private_data->dev_sid, "%s", sid);
	device->private_data = private_data;
	indigo_async((void *(*)(void *))indigo_attach_device, device);
	devices[slot]=device;

	if(check_st4 == QHYCCD_SUCCESS) {
		slot = find_available_device_slot();
		if (slot < 0) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "No device slots available.");
			return;
		}
		device = (indigo_device*)malloc(sizeof(indigo_device));
		assert(device != NULL);
		memcpy(device, &guider_template, sizeof(indigo_device));
		sprintf(device->name, "%s Guider #%s", dev_name, dev_usbpath);
		INDIGO_DRIVER_LOG(DRIVER_NAME, "'%s' attached.", device->name);
		device->private_data = private_data;
		indigo_async((void *(*)(void *))indigo_attach_device, device);
		devices[slot]=device;
	}
}


static void process_unplug_event() {
	int slot;
	bool removed = false;
	qhy_private_data *private_data = NULL;
	while ((slot = find_unplugged_device_slot()) != NOT_FOUND) {
		indigo_device **device = &devices[slot];
		if (*device == NULL) {
			pthread_mutex_unlock(&device_mutex);
			return;
		}
		indigo_detach_device(*device);
		if ((*device)->private_data) {
			private_data = (qhy_private_data*)((*device)->private_data);
		}
		free(*device);
		*device = NULL;
		removed = true;
	}

	if (private_data) {
		CloseQHYCCD(private_data->handle);
		free(private_data);
		private_data = NULL;
	}

	if (!removed) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "No QHY Camera unplugged!");
	}
}


#ifdef __APPLE__
void *plug_thread_func(void *sid) {
	pthread_mutex_lock(&device_mutex);
	char firmware_base_dir[255] = "/usr/local/lib/qhy";
	if (getenv("INDIGO_QHY_FIRMWARE_BASE") != NULL) {
		strncpy(firmware_base_dir, getenv("INDIGO_QHY_FIRMWARE_BASE"), 255);
	}
	OSXInitQHYCCDFirmware(firmware_base_dir);
	process_plug_event();
	pthread_mutex_unlock(&device_mutex);
	pthread_exit(NULL);
	return NULL;
}


void *unplug_thread_func(void *sid) {
	pthread_mutex_lock(&device_mutex);
	process_unplug_event();
	pthread_mutex_unlock(&device_mutex);
	pthread_exit(NULL);
	return NULL;
}
#endif /* __APPLE__ */


static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	struct libusb_device_descriptor descriptor;

	pthread_mutex_lock(&device_mutex);
	libusb_get_device_descriptor(dev, &descriptor);
	if ((descriptor.idVendor != QHY_VENDOR_ID1) &&
		(descriptor.idVendor != QHY_VENDOR_ID2) &&
		(descriptor.idVendor != QHY_VENDOR_ID3) &&
		(descriptor.idVendor != QHY_VENDOR_ID4) &&
		(descriptor.idVendor != QHY_VENDOR_ID5)) {
		pthread_mutex_unlock(&device_mutex);
		return 0;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Hotplug: vid=%x pid=%x", descriptor.idVendor, descriptor.idProduct);
	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			#ifdef __APPLE__
				pthread_t plug_thread;
				/* This is ugly hack but otherwise QHY5IIL does not work!!!
				   The camera does not respond in the hotpliug callback on MacOS,
				   so the thread waits the callback to complete and initializes
				   the camera.
				 */
				if (pthread_create(&plug_thread, NULL, plug_thread_func, NULL)) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME,"Error creating thread for firmware loader");
				}
			#else
				process_plug_event();
			#endif /* __APPLE__ */
			break;
		}
		case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: {
			#ifdef __APPLE__
				pthread_t unplug_thread;
				/* This is ugly hack but otherwise QHY5IIL does not work!!!
				   See the note in LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED case.
				*/
				if (pthread_create(&unplug_thread, NULL, unplug_thread_func, NULL)) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME,"Error creating thread for firmware loader");
				}
			#else
				process_unplug_event();
			#endif /* __APPLE__ */
			break;
		}
	}
	pthread_mutex_unlock(&device_mutex);
	return 0;
};


static void remove_all_devices() {
	int i, k;
	qhy_private_data *pds[MAX_DEVICES] = {NULL};
	qhy_private_data *freed_pds[MAX_DEVICES] = {NULL};

	for(i = 0; i < MAX_DEVICES; i++) {
		indigo_device *device = devices[i];
		if (device == NULL) continue;
		if (PRIVATE_DATA) pds[i] = PRIVATE_DATA; /* preserve pointers to private data */
		indigo_detach_device(device);
		free(device);
		device = NULL;
		devices[i] = NULL;
	}
	/* free private data */
	bool found;
	for(i = 0; i < MAX_DEVICES; i++) {
		found = false;
		for(k = 0; k <= i; k++) {
			if (freed_pds[k] == pds[i]) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME,"freed_pds[%d] = %10p  pds[%d] = %10p => prevent free", k,freed_pds[k],  i, pds[i]);
				found = true; /* prevent double free */
				break;
			}
		}
		if (!found && pds[i]) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME,"freed_pds[%d] = %10p  pds[%d] = %10p => FREE", k,freed_pds[k],  i, pds[i]);
			freed_pds[i] = pds[i];
			free(pds[i]);
		}
	}
}


static libusb_hotplug_callback_handle callback_handle;

indigo_result indigo_ccd_qhy(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	int rc;
	SET_DRIVER_INFO(info, "QHY Camera", __FUNCTION__, DRIVER_VERSION, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			SetQHYCCDLogLevel(LOG_LEVEL_FATAL);
			rc = InitQHYCCDResource();
			if (rc != QHYCCD_SUCCESS) return INDIGO_FAILED;
			indigo_start_usb_event_handler();
			rc = libusb_hotplug_register_callback(NULL, (libusb_hotplug_event)(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT), LIBUSB_HOTPLUG_ENUMERATE, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_register_callback ->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
			return rc >= 0 ? INDIGO_OK : INDIGO_FAILED;

		case INDIGO_DRIVER_SHUTDOWN:
			last_action = action;
			libusb_hotplug_deregister_callback(NULL, callback_handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_deregister_callback");
			remove_all_devices();
			ReleaseQHYCCDResource();
			break;

		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
