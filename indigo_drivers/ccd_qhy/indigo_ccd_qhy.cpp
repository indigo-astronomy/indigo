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
// 2.0 by Rumen G. Bogdanovski


/** INDIGO QHY CCD driver
 \file indigo_ccd_qhy.cpp
 \NOTE: This file should be .cpp as qhy headers are in C++
 */

#define DRIVER_VERSION 0x0013

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_usb_utils.h>

#ifdef QHY2
#include "../ccd_qhy2/indigo_ccd_qhy.h"
#else
#include "indigo_ccd_qhy.h"
#endif

#if !(defined(__APPLE__) && defined(__arm64__))

#if defined(INDIGO_MACOS)
#include <libusb-1.0/libusb.h>
#elif defined(INDIGO_FREEBSD)
#include <libusb.h>
#else
#include <libusb-1.0/libusb.h>
#endif

#include "qhyccd.h"

#ifdef USE_LOG4Z
#include "log4z.h"
#endif


//#define HOTPLUG

#define MAX_SID_LEN                255
#define QHY_MAX_FORMATS            4

#define QHY_VENDOR_ID1             0x16c0
#define QHY_VENDOR_ID2             0x1618
#define QHY_VENDOR_ID3             0x1856
#define QHY_VENDOR_ID4             0x04b4
#define QHY_VENDOR_ID5             0x0547

#define CCD_ADVANCED_GROUP         "Advanced"
#define USBTRAFFIC_NAME            "USBTRAFFIC"
#define USBTRAFFIC_DESC            "USB Traffic"
#define USBSPEED_NAME              "USBSPEED"
#define USBSPEED_DESC              "USB Speed"
#define SHUTTERHEATING_NAME        "SHUTTERMOTORHEATING"
#define SHUTTERHEATING_DESC        "Shutter Motor Heating"


#define QHY_GUIDE_NORTH            1
#define QHY_GUIDE_SOUTH            2
#define QHY_GUIDE_EAST             0
#define QHY_GUIDE_WEST             3

#define MIN_CCD_TEMP               -50.0
#define MAX_CCD_TEMP               40.0
#define TEMP_THRESHOLD             0.3

#define FW_COUNT									 7

#define PRIVATE_DATA               ((qhy_private_data *)device->private_data)

#define PIXEL_FORMAT_PROPERTY      (PRIVATE_DATA->pixel_format_property)
#define RAW8_NAME                  "RAW 8"
#define RAW16_NAME                 "RAW 16"

#ifdef QHY2
#define READ_MODE_PROPERTY      (PRIVATE_DATA->read_mode_property)
#endif

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
	bool has_shutter;
	bool has_cooler;
	bool cooler_on;
	int last_bpp;
	int last_live;
	indigo_timer *exposure_timer, *temperature_timer, *guider_timer_ra, *guider_timer_dec;
	double target_temperature, current_temperature;
	long cooler_power;
	unsigned char *buffer;
	long int buffer_size;
	pthread_mutex_t usb_mutex;
	bool can_check_temperature;
	int max_bin;
	bool bins_ok[4];
	/* Filter wheel specific */
	indigo_timer *wheel_timer;
	int fw_count;
	int fw_current_slot;
	char fw_target_slot;

	indigo_property *pixel_format_property;
#ifdef QHY2
	indigo_property *read_mode_property;
#endif
	indigo_property *qhy_advanced_property;
} qhy_private_data;

static char *get_bayer_string(indigo_device *device) {
	int pattern = IsQHYCCDControlAvailable(PRIVATE_DATA->handle, CAM_COLOR);
	if (pattern != QHYCCD_ERROR) {
		if (pattern == BAYER_GB)
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

static bool bpp_supported(indigo_device *device, int bpp) {
	if ((CCD_FRAME_BITS_PER_PIXEL_ITEM->number.min <= bpp) &&
	    (CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max >= bpp)) {
		return true;
	}
	return false;
}

static indigo_result qhy_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(PIXEL_FORMAT_PROPERTY, property))
			indigo_define_property(device, PIXEL_FORMAT_PROPERTY, NULL);
#ifdef QHY2
		if (indigo_property_match(READ_MODE_PROPERTY, property))
			indigo_define_property(device, READ_MODE_PROPERTY, NULL);
#endif
		if (indigo_property_match(QHY_ADVANCED_PROPERTY, property))
			indigo_define_property(device, QHY_ADVANCED_PROPERTY, NULL);
	}
	return indigo_ccd_enumerate_properties(device, NULL, NULL);
}

static bool qhy_open(indigo_device *device) {
	int res;
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
		PRIVATE_DATA->last_live = false;
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
			PRIVATE_DATA->buffer_size = /* PRIVATE_DATA->frame_height * PRIVATE_DATA->frame_width * 2 */ 128 * 1024 * 1024 + FITS_HEADER_SIZE;
			PRIVATE_DATA->buffer = (unsigned char*)indigo_alloc_blob_buffer(PRIVATE_DATA->buffer_size);
		}
	}
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	return true;
}

static bool qhy_setup_exposure(indigo_device *device, double exposure, int frame_left, int frame_top, int frame_width, int frame_height, int horizontal_bin, int vertical_bin, bool live) {
	int res;
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);

	int requested_bpp = PIXEL_FORMAT_PROPERTY->items[0].sw.value ? 8 : 16;
	if (PRIVATE_DATA->handle && (PRIVATE_DATA->last_bpp != requested_bpp || PRIVATE_DATA->last_live != live)) {
		CloseQHYCCD(PRIVATE_DATA->handle);
		indigo_usleep(500000);
		ScanQHYCCD();
		PRIVATE_DATA->handle = OpenQHYCCD(PRIVATE_DATA->dev_sid);
		if (PRIVATE_DATA->handle == NULL) {
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "OpenQHYCCD('%s') = NULL", PRIVATE_DATA->dev_sid);
			return false;
		}
		res = SetQHYCCDStreamMode(PRIVATE_DATA->handle, live ? 1 : 0);
		if (res != QHYCCD_SUCCESS) {
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetQHYCCDStreamMode('%s') = %d", PRIVATE_DATA->dev_sid, res);
			return false;
		}
		InitQHYCCD(PRIVATE_DATA->handle);
		res = SetQHYCCDBitsMode(PRIVATE_DATA->handle, requested_bpp);
		if (res != QHYCCD_SUCCESS) {
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetQHYCCDBitsMode(%s) = %d", PRIVATE_DATA->dev_sid, res);
			return false;
		}
		PRIVATE_DATA->last_bpp = requested_bpp;
		PRIVATE_DATA->last_live = live;
	}

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

static bool qhy_start_exposure(indigo_device *device, double exposure, bool dark, int frame_left, int frame_top, int frame_width, int frame_height, int horizontal_bin, int vertical_bin, bool live) {
	int res;
	if (!qhy_setup_exposure(device, exposure, frame_left, frame_top, frame_width, frame_height, horizontal_bin, vertical_bin, live)) {
		return false;
	}

	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	if (PRIVATE_DATA->has_shutter) {
		if (dark) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME,"Taking DARK Frame.");
			res = ControlQHYCCDShutter(PRIVATE_DATA->handle, MACHANICALSHUTTER_CLOSE);
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME,"Taking LIGHT frame.");
			res = ControlQHYCCDShutter(PRIVATE_DATA->handle, MACHANICALSHUTTER_FREE);
		}
		if (res != QHYCCD_SUCCESS) INDIGO_DRIVER_ERROR(DRIVER_NAME, "ControlQHYCCDShutter(%s) = %d", PRIVATE_DATA->dev_sid, res);
	}
	res = live ? BeginQHYCCDLive(PRIVATE_DATA->handle) : ExpQHYCCDSingleFrame(PRIVATE_DATA->handle);
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	if (res != QHYCCD_SUCCESS && res != QHYCCD_READ_DIRECTLY) {
		if (live)
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "BeginQHYCCDLive(%s) = %d", PRIVATE_DATA->dev_sid, res);
		else
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ExpQHYCCDSingleFrame(%s) = %d", PRIVATE_DATA->dev_sid, res);
		return false;
	}
	return true;
}

static bool qhy_read_pixels(indigo_device *device, bool live) {
	int res;
	uint32_t channels;
	if (!live) {
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		int remaining = GetQHYCCDExposureRemaining(PRIVATE_DATA->handle);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);

		while (remaining > 100) {
			pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
			remaining = GetQHYCCDExposureRemaining(PRIVATE_DATA->handle);
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			indigo_usleep(2000);
		}
	}
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	res = live ? GetQHYCCDLiveFrame(
		 PRIVATE_DATA->handle,
		 &PRIVATE_DATA->ci_params.width,
		 &PRIVATE_DATA->ci_params.height,
		 &PRIVATE_DATA->ci_params.bpp,
		 &channels,
		 PRIVATE_DATA->buffer + FITS_HEADER_SIZE
		 )
	: GetQHYCCDSingleFrame(
		PRIVATE_DATA->handle,
		&PRIVATE_DATA->ci_params.width,
		&PRIVATE_DATA->ci_params.height,
		&PRIVATE_DATA->ci_params.bpp,
		&channels,
		PRIVATE_DATA->buffer + FITS_HEADER_SIZE
	);
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	if (res != QHYCCD_SUCCESS) {
		if (!live)
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "GetQHYCCDSingleFrame(%s) = %d", PRIVATE_DATA->dev_sid, res);
		return false;
	}
	return true;
}

static bool qhy_abort_exposure(indigo_device *device, bool live) {
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	int err = live ? StopQHYCCDLive(PRIVATE_DATA->handle) : CancelQHYCCDExposingAndReadout(PRIVATE_DATA->handle);
	indigo_usleep(500000);
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	if (err !=  QHYCCD_SUCCESS) return false;
	else return true;
}

static bool qhy_set_cooler(indigo_device *device, bool status, double target, double *current, long *cooler_power) {
	int res;
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);

	*current = GetQHYCCDParam(PRIVATE_DATA->handle, CONTROL_CURTEMP);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "GetQHYCCDParam(%s, CONTROL_CURTEMP) = %f", PRIVATE_DATA->dev_sid, *current);

	if (!PRIVATE_DATA->has_cooler) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		return true;
	}

	if (PRIVATE_DATA->cooler_on) {
		*cooler_power = (GetQHYCCDParam(PRIVATE_DATA->handle, CONTROL_CURPWM) / 2.55); /* make it in percent (PWM is 0-255) */
		/* ControlQHYCCDTemp() should be called once in a couple of seconds
		   in order to maintain the requested temperature
		 */
		res = ControlQHYCCDTemp(PRIVATE_DATA->handle, target);
		if (res != QHYCCD_SUCCESS) INDIGO_DRIVER_ERROR(DRIVER_NAME, "ControlQHYCCDTemp(%s) = %d", PRIVATE_DATA->dev_sid, res);
	}

	if (!status) {
		/* Stop Temperature Control */
		SetQHYCCDParam(PRIVATE_DATA->handle, CONTROL_MANULPWM, 0);
		*cooler_power = 0;
		PRIVATE_DATA->cooler_on = false;
	} else {
		PRIVATE_DATA->cooler_on = true;
	}

	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	return true;
}

static void qhy_close(indigo_device *device) {
	if (!device->is_connected)
		return;
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	if (--PRIVATE_DATA->count_open == 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Close %s: handle = %p\n", PRIVATE_DATA->dev_sid, PRIVATE_DATA->handle);
		if (PRIVATE_DATA->handle) {
			CloseQHYCCD(PRIVATE_DATA->handle);
			PRIVATE_DATA->handle = NULL;
		}
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
	if (!CONNECTION_CONNECTED_ITEM->sw.value)
		return;
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		CCD_EXPOSURE_ITEM->number.value = 0;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		if (qhy_read_pixels(device, false)) {
			char *color_string = get_bayer_string(device);
			CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = PIXEL_FORMAT_PROPERTY->items[0].sw.value ? 8 : 16;
			if (color_string) {
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
				indigo_process_image(device, PRIVATE_DATA->buffer, PRIVATE_DATA->ci_params.width, PRIVATE_DATA->ci_params.height, PRIVATE_DATA->ci_params.bpp, true, true, keywords, false);
			} else {
				indigo_process_image(device, PRIVATE_DATA->buffer, PRIVATE_DATA->ci_params.width, PRIVATE_DATA->ci_params.height, PRIVATE_DATA->ci_params.bpp, true, true, NULL, false);
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
	if (!CONNECTION_CONNECTED_ITEM->sw.value)
		return;
	char *color_string = get_bayer_string(device);
	indigo_fits_keyword keywords[] = {
		{ .type = INDIGO_FITS_STRING, .name = "BAYERPAT", {.string = color_string }, .comment = "Bayer color pattern" },
		{ .type = INDIGO_FITS_NUMBER, .name = "XBAYROFF", {.number = 0 }, .comment = "X offset of Bayer array" },
		{ .type = INDIGO_FITS_NUMBER, .name = "YBAYROFF", {.number = 0 }, .comment = "Y offset of Bayer array" },
		{ .type = (indigo_fits_keyword_type)0 }
	};
	PRIVATE_DATA->can_check_temperature = false;
	if (qhy_start_exposure(device, CCD_STREAMING_EXPOSURE_ITEM->number.value, (CCD_FRAME_TYPE_DARK_ITEM->sw.value || CCD_FRAME_TYPE_DARKFLAT_ITEM->sw.value || CCD_FRAME_TYPE_BIAS_ITEM->sw.value), CCD_FRAME_LEFT_ITEM->number.value, CCD_FRAME_TOP_ITEM->number.value, CCD_FRAME_WIDTH_ITEM->number.value, CCD_FRAME_HEIGHT_ITEM->number.value, CCD_BIN_HORIZONTAL_ITEM->number.value, CCD_BIN_VERTICAL_ITEM->number.value, true)) {
		while (CCD_STREAMING_COUNT_ITEM->number.value != 0) {
			if (qhy_read_pixels(device, true)) {
				if (color_string) {
					indigo_process_image(device, PRIVATE_DATA->buffer, PRIVATE_DATA->ci_params.width, PRIVATE_DATA->ci_params.height, PRIVATE_DATA->ci_params.bpp, true, true, keywords, true);
				} else {
					indigo_process_image(device, PRIVATE_DATA->buffer, PRIVATE_DATA->ci_params.width, PRIVATE_DATA->ci_params.height, PRIVATE_DATA->ci_params.bpp, true, true, NULL, true);
				}
				if (CCD_STREAMING_COUNT_ITEM->number.value > 0)
					CCD_STREAMING_COUNT_ITEM->number.value -= 1;
				CCD_STREAMING_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
			}
		}
		qhy_abort_exposure(device, true);
	}
	PRIVATE_DATA->can_check_temperature = true;
	indigo_finalize_video_stream(device);
	CCD_STREAMING_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
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
	if (!CONNECTION_CONNECTED_ITEM->sw.value)
		return;
	if (PRIVATE_DATA->can_check_temperature) {
		if (qhy_set_cooler(device, CCD_COOLER_ON_ITEM->sw.value, PRIVATE_DATA->target_temperature, &PRIVATE_DATA->current_temperature, &PRIVATE_DATA->cooler_power)) {
			double diff = PRIVATE_DATA->current_temperature - PRIVATE_DATA->target_temperature;
			if (CCD_COOLER_ON_ITEM->sw.value) {
				CCD_TEMPERATURE_PROPERTY->state = fabs(diff) > TEMP_THRESHOLD ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
				CCD_COOLER_POWER_ITEM->number.value = PRIVATE_DATA->cooler_power;
			} else {
				CCD_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
				CCD_COOLER_POWER_ITEM->number.value = 0;
			}
			CCD_TEMPERATURE_ITEM->number.value = PRIVATE_DATA->current_temperature;
			CCD_COOLER_PROPERTY->state = INDIGO_OK_STATE;
			CCD_COOLER_POWER_PROPERTY->state = INDIGO_OK_STATE;
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
	indigo_reschedule_timer(device, 2, &PRIVATE_DATA->temperature_timer);
}

static void guider_timer_callback_ra(indigo_device *device) {
	PRIVATE_DATA->guider_timer_ra = NULL;
	int res;
	if (!CONNECTION_CONNECTED_ITEM->sw.value) return;

	indigo_cancel_timer(device, &PRIVATE_DATA->guider_timer_ra);
	int duration = GUIDER_GUIDE_EAST_ITEM->number.value;
	if (duration > 0) {
		/* No sync possible here, ControlQHYCCDGuide is blocking. Let us hope is will work... */
		res = ControlQHYCCDGuide(PRIVATE_DATA->handle, QHY_GUIDE_EAST, duration);

		if (res != QHYCCD_SUCCESS)
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "ControlQHYCCDGuide(%s, GUIDE_EAST) = %d", PRIVATE_DATA->dev_sid, res);
	} else {
		int duration = GUIDER_GUIDE_WEST_ITEM->number.value;
		if (duration > 0) {
			/* No sync possible here, ControlQHYCCDGuide is blocking. Let us hope is will work... */
			res = ControlQHYCCDGuide(PRIVATE_DATA->handle, QHY_GUIDE_WEST, duration);

			if (res != QHYCCD_SUCCESS)
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "ControlQHYCCDGuide(%s, GUIDE_WEST) = %d", PRIVATE_DATA->dev_sid, res);
		}
	}

	GUIDER_GUIDE_EAST_ITEM->number.value = 0;
	GUIDER_GUIDE_WEST_ITEM->number.value = 0;
	GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
}

static void guider_timer_callback_dec(indigo_device *device) {
	PRIVATE_DATA->guider_timer_dec = NULL;
	int res;
	if (!CONNECTION_CONNECTED_ITEM->sw.value)
			return;
	int duration = GUIDER_GUIDE_NORTH_ITEM->number.value;
	if (duration > 0) {
		/* No sync possible here, ControlQHYCCDGuide is blocking. Let us hope is will work... */
		res = ControlQHYCCDGuide(PRIVATE_DATA->handle, QHY_GUIDE_NORTH, duration);

		if (res != QHYCCD_SUCCESS)
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "ControlQHYCCDGuide(%s, GUIDE_NORTH) = %d", PRIVATE_DATA->dev_sid, res);
	} else {
		int duration = GUIDER_GUIDE_SOUTH_ITEM->number.value;
		if (duration > 0) {
			/* No sync possible here, ControlQHYCCDGuide is blocking. Let us hope is will work... */
			res = ControlQHYCCDGuide(PRIVATE_DATA->handle, QHY_GUIDE_SOUTH, duration);

			if (res != QHYCCD_SUCCESS)
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "ControlQHYCCDGuide(%s, GUIDE_SOUTH) = %d", PRIVATE_DATA->dev_sid, res);
		}
	}

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
		// -------------------------------------------------------------------------------- CCD_STREAMING
		CCD_STREAMING_PROPERTY->hidden = false;
		CCD_STREAMING_EXPOSURE_ITEM->number.max = 4.0;
		CCD_IMAGE_FORMAT_PROPERTY->count = 7;
		// --------------------------------------------------------------------------------- PIXEL_FORMAT
		PIXEL_FORMAT_PROPERTY = indigo_init_switch_property(NULL, device->name, "PIXEL_FORMAT", CCD_ADVANCED_GROUP, "Pixel Format", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (PIXEL_FORMAT_PROPERTY == NULL)
			return INDIGO_FAILED;
		// -------------------------------------------------------------------------------- ASI_ADVANCED
		QHY_ADVANCED_PROPERTY = indigo_init_number_property(NULL, device->name, "QHY_ADVANCED", CCD_ADVANCED_GROUP, "Advanced", INDIGO_OK_STATE, INDIGO_RW_PERM, 0);
		if (QHY_ADVANCED_PROPERTY == NULL)
			return INDIGO_FAILED;
#ifdef QHY2
// -------------------------------------------------------------------------------- ASI_ADVANCED
		READ_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, "READ_MODE", CCD_ADVANCED_GROUP, "Read mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 16);
		if (READ_MODE_PROPERTY == NULL)
			return INDIGO_FAILED;
#endif
		// --------------------------------------------------------------------------------
		return indigo_ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result handle_advanced_property(indigo_device *device, indigo_property *property) {
	int res;
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	for (int item = 0; item < property->count; item++) {
		if (!strncmp(USBSPEED_NAME, property->items[item].name, INDIGO_NAME_SIZE)) {
			res = SetQHYCCDParam(PRIVATE_DATA->handle, CONTROL_SPEED, property->items[item].number.value);
			if (res != QHYCCD_SUCCESS) INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetQHYCCDParam(%s, %s) = %d", PRIVATE_DATA->dev_sid, USBSPEED_NAME, res);
		} else if (!strncmp(USBTRAFFIC_NAME, property->items[item].name, INDIGO_NAME_SIZE)) {
			res = SetQHYCCDParam(PRIVATE_DATA->handle, CONTROL_USBTRAFFIC, property->items[item].number.value);
			if (res != QHYCCD_SUCCESS) INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetQHYCCDParam(%s, %s) = %d", PRIVATE_DATA->dev_sid, USBTRAFFIC_NAME, res);
		} else if (!strncmp(SHUTTERHEATING_NAME, property->items[item].name, INDIGO_NAME_SIZE)) {
			res = SetQHYCCDParam(PRIVATE_DATA->handle, CAM_SHUTTERMOTORHEATING_INTERFACE, property->items[item].number.value);
			if (res != QHYCCD_SUCCESS) INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetQHYCCDParam(%s, %s) = %d", PRIVATE_DATA->dev_sid, SHUTTERHEATING_NAME, res);
		}
	}

	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	return INDIGO_OK;
}

static void ccd_connect_callback(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (!device->is_connected) {
			if (qhy_open(device)) {
				CCD_INFO_WIDTH_ITEM->number.value = PRIVATE_DATA->frame_width;
				CCD_INFO_HEIGHT_ITEM->number.value = PRIVATE_DATA->frame_height;
				CCD_INFO_PIXEL_SIZE_ITEM->number.value = PRIVATE_DATA-> pixel_width;
				CCD_INFO_PIXEL_WIDTH_ITEM->number.value = PRIVATE_DATA->pixel_width;
				CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = PRIVATE_DATA->pixel_height;
				CCD_FRAME_LEFT_ITEM->number.value = CCD_FRAME_TOP_ITEM->number.value = 0;
				CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max = PRIVATE_DATA->frame_width;
				CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max = PRIVATE_DATA->frame_height;
				CCD_FRAME_BITS_PER_PIXEL_ITEM->number.min = CCD_INFO_BITS_PER_PIXEL_ITEM->number.min = PRIVATE_DATA->bpp;
				CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = CCD_INFO_BITS_PER_PIXEL_ITEM->number.max = PRIVATE_DATA->bpp;
				CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = PRIVATE_DATA->bpp;

				pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);

				PRIVATE_DATA->has_shutter = (IsQHYCCDControlAvailable(PRIVATE_DATA->handle, CAM_MECHANICALSHUTTER) == QHYCCD_SUCCESS);

				PRIVATE_DATA->has_cooler = (IsQHYCCDControlAvailable(PRIVATE_DATA->handle, CONTROL_COOLER) == QHYCCD_SUCCESS);
				if (PRIVATE_DATA->has_cooler) {
					CCD_COOLER_PROPERTY->hidden = false;
					CCD_COOLER_POWER_PROPERTY->hidden = false;
					CCD_TEMPERATURE_PROPERTY->hidden = false;
					CCD_TEMPERATURE_PROPERTY->perm = INDIGO_RW_PERM;
					CCD_TEMPERATURE_ITEM->number.min = MIN_CCD_TEMP;
					CCD_TEMPERATURE_ITEM->number.max = MAX_CCD_TEMP;
					CCD_TEMPERATURE_ITEM->number.step = 0;
					PRIVATE_DATA->cooler_on = (GetQHYCCDParam(PRIVATE_DATA->handle, CONTROL_CURPWM) > 0);
				} else {
					CCD_COOLER_PROPERTY->hidden = true;
					CCD_COOLER_POWER_PROPERTY->hidden = true;
					CCD_TEMPERATURE_PROPERTY->hidden = false;
					CCD_TEMPERATURE_PROPERTY->perm = INDIGO_RO_PERM;
				}
				// --------------------------------------------------------------------------------- PIXEL_FORMAT
				indigo_init_switch_item(PIXEL_FORMAT_PROPERTY->items + 0, RAW8_NAME, RAW8_NAME, PRIVATE_DATA->bpp == 8);
				indigo_init_switch_item(PIXEL_FORMAT_PROPERTY->items + 1, RAW16_NAME, RAW16_NAME, PRIVATE_DATA->bpp == 16);
				CCD_FRAME_BITS_PER_PIXEL_ITEM->number.min = CCD_INFO_BITS_PER_PIXEL_ITEM->number.min = 8;
				CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = CCD_INFO_BITS_PER_PIXEL_ITEM->number.max = 16;
				CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = PRIVATE_DATA->last_bpp = PRIVATE_DATA->bpp;
				if (IsQHYCCDControlAvailable(PRIVATE_DATA->handle, CAM_8BITS) == QHYCCD_SUCCESS && IsQHYCCDControlAvailable(PRIVATE_DATA->handle, CAM_16BITS) == QHYCCD_SUCCESS) {
				} else {
					PIXEL_FORMAT_PROPERTY->hidden = true;
				}
				indigo_define_property(device, PIXEL_FORMAT_PROPERTY, NULL);

				// --------------------------------------------------------------------------------- BINNING
				PRIVATE_DATA->bins_ok[0] = PRIVATE_DATA->bins_ok[1] = PRIVATE_DATA->bins_ok[2] = PRIVATE_DATA->bins_ok[3] = false;
				PRIVATE_DATA->max_bin = 0;
				if (IsQHYCCDControlAvailable(PRIVATE_DATA->handle, CAM_BIN1X1MODE) == QHYCCD_SUCCESS) {
					PRIVATE_DATA->max_bin = 1;
					PRIVATE_DATA->bins_ok[0] = true;
				}
				if (IsQHYCCDControlAvailable(PRIVATE_DATA->handle, CAM_BIN2X2MODE) == QHYCCD_SUCCESS) {
					PRIVATE_DATA->max_bin = 2;
					PRIVATE_DATA->bins_ok[1] = true;
				}
				if (IsQHYCCDControlAvailable(PRIVATE_DATA->handle, CAM_BIN3X3MODE) == QHYCCD_SUCCESS) {
					PRIVATE_DATA->max_bin = 3;
					PRIVATE_DATA->bins_ok[2] = true;
				}
				if (IsQHYCCDControlAvailable(PRIVATE_DATA->handle, CAM_BIN4X4MODE) == QHYCCD_SUCCESS) {
					PRIVATE_DATA->max_bin = 4;
					PRIVATE_DATA->bins_ok[3] = true;
				}

				CCD_BIN_PROPERTY->perm = INDIGO_RW_PERM;
				CCD_BIN_HORIZONTAL_ITEM->number.value = CCD_BIN_HORIZONTAL_ITEM->number.min = 1;
				CCD_BIN_HORIZONTAL_ITEM->number.max = PRIVATE_DATA->max_bin;
				CCD_BIN_VERTICAL_ITEM->number.value = CCD_BIN_VERTICAL_ITEM->number.min = 1;
				CCD_BIN_VERTICAL_ITEM->number.max = PRIVATE_DATA->max_bin;

				CCD_INFO_MAX_HORIZONAL_BIN_ITEM->number.value = PRIVATE_DATA->max_bin;
				CCD_INFO_MAX_VERTICAL_BIN_ITEM->number.value = PRIVATE_DATA->max_bin;

				// --------------------------------------------------------------------------------- MODE
				int count = 0;
				char name[32], label[64];
				for (int bin = 1; bin <= PRIVATE_DATA->max_bin; bin++) {
					if (!PRIVATE_DATA->bins_ok[bin-1])
						continue;
					if (bpp_supported(device, 8)) {
						snprintf(name, 32, "%s %dx%d", RAW8_NAME, bin, bin);
						snprintf(label, 64, "%s %dx%d", RAW8_NAME, (int)CCD_FRAME_WIDTH_ITEM->number.value / bin, (int)CCD_FRAME_HEIGHT_ITEM->number.value / bin);
						indigo_init_switch_item(CCD_MODE_PROPERTY->items + count, name, label, bin == 1 && PIXEL_FORMAT_PROPERTY->items[0].sw.value);
						count++;
					}
					if (bpp_supported(device, 16)) {
						snprintf(name, 32, "%s %dx%d", RAW16_NAME, bin, bin);
						snprintf(label, 64, "%s %dx%d", RAW16_NAME, (int)CCD_FRAME_WIDTH_ITEM->number.value / bin, (int)CCD_FRAME_HEIGHT_ITEM->number.value / bin);
						indigo_init_switch_item(CCD_MODE_PROPERTY->items + count, name, label, bin == 1  && PIXEL_FORMAT_PROPERTY->items[1].sw.value);
						count++;
					}
				}
				CCD_MODE_PROPERTY->count = count;

				// --------------------------------------------------------------------------------- CCD_GAIN
				if (IsQHYCCDControlAvailable(PRIVATE_DATA->handle, CONTROL_GAIN) == QHYCCD_SUCCESS) {
					CCD_GAIN_PROPERTY->hidden = false;
					GetQHYCCDParamMinMaxStep(PRIVATE_DATA->handle, CONTROL_GAIN, &CCD_GAIN_ITEM->number.min, &CCD_GAIN_ITEM->number.max, &CCD_GAIN_ITEM->number.step);
					CCD_GAIN_ITEM->number.value = GetQHYCCDParam(PRIVATE_DATA->handle, CONTROL_GAIN);
				} else {
					CCD_GAIN_PROPERTY->hidden = true;
				}

				// --------------------------------------------------------------------------------- CCD_OFFSET
				if (IsQHYCCDControlAvailable(PRIVATE_DATA->handle, CONTROL_OFFSET) == QHYCCD_SUCCESS) {
					CCD_OFFSET_PROPERTY->hidden = false;
					GetQHYCCDParamMinMaxStep(PRIVATE_DATA->handle, CONTROL_OFFSET, &CCD_OFFSET_ITEM->number.min, &CCD_OFFSET_ITEM->number.max, &CCD_OFFSET_ITEM->number.step);
					CCD_OFFSET_ITEM->number.value = GetQHYCCDParam(PRIVATE_DATA->handle, CONTROL_OFFSET);
				} else {
					CCD_OFFSET_PROPERTY->hidden = true;
				}

				// --------------------------------------------------------------------------------- CCD_GAMMA
				if (IsQHYCCDControlAvailable(PRIVATE_DATA->handle, CONTROL_GAMMA) == QHYCCD_SUCCESS) {
					CCD_GAMMA_PROPERTY->hidden = false;
					GetQHYCCDParamMinMaxStep(PRIVATE_DATA->handle, CONTROL_GAMMA, &CCD_GAMMA_ITEM->number.min, &CCD_GAMMA_ITEM->number.max, &CCD_GAMMA_ITEM->number.step);
					CCD_GAMMA_ITEM->number.value = GetQHYCCDParam(PRIVATE_DATA->handle, CONTROL_GAMMA);
				} else {
					CCD_GAMMA_PROPERTY->hidden = true;
				}

				// ---------------------------------------------------------------------------------- EXPOSURE
				GetQHYCCDParamMinMaxStep(PRIVATE_DATA->handle, CONTROL_EXPOSURE,
					&(CCD_EXPOSURE_ITEM->number.min),
					&(CCD_EXPOSURE_ITEM->number.max),
					&(CCD_EXPOSURE_ITEM->number.step)
				);
				/* convert to seconds */
				CCD_EXPOSURE_ITEM->number.min /= 1e6;
				CCD_EXPOSURE_ITEM->number.max /= 1e6;
				CCD_EXPOSURE_ITEM->number.step /= 1e6;
				/* Kludge: some cameras report ridiculous max exposure times like QHY6 (~80s) while it can happily can do 900s */
				CCD_EXPOSURE_ITEM->number.max = (CCD_EXPOSURE_ITEM->number.max < 900) ? 900 : CCD_EXPOSURE_ITEM->number.max;
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Exposure params min = %fs max = %fs step = %fs", CCD_EXPOSURE_ITEM->number.min, CCD_EXPOSURE_ITEM->number.max, CCD_EXPOSURE_ITEM->number.step);

#ifdef QHY2
				// ---------------------------------------------------------------------------------- READ_MODE
				uint32_t mode_count = 0;
				if (GetQHYCCDNumberOfReadModes(PRIVATE_DATA->handle, &mode_count) == QHYCCD_SUCCESS && mode_count > 0) {
					uint32_t current_mode = 0;
					GetQHYCCDReadMode(PRIVATE_DATA->handle, &current_mode);
					READ_MODE_PROPERTY->count = mode_count;
					READ_MODE_PROPERTY->hidden = false;
					for (int i = 0; i < mode_count; i++) {
						char name[INDIGO_NAME_SIZE], label[INDIGO_NAME_SIZE];
						sprintf(name, "%d", i);
						GetQHYCCDReadModeName(PRIVATE_DATA->handle, i, label);
						indigo_init_switch_item(READ_MODE_PROPERTY->items + i, name, label, i == current_mode);
					}
				} else {
					READ_MODE_PROPERTY->hidden = true;
				}
				indigo_define_property(device, READ_MODE_PROPERTY, NULL);
#endif
				// --------------------------------------------------------------------------------- ADVANCED
				count = 0;
				if (IsQHYCCDControlAvailable(PRIVATE_DATA->handle, CONTROL_USBTRAFFIC) == QHYCCD_SUCCESS) {
					QHY_ADVANCED_PROPERTY = indigo_resize_property(QHY_ADVANCED_PROPERTY, count+1);
					indigo_init_number_item(QHY_ADVANCED_PROPERTY->items+count, USBTRAFFIC_NAME, USBTRAFFIC_DESC, 0, 0, 1, 0);
					GetQHYCCDParamMinMaxStep(PRIVATE_DATA->handle, CONTROL_USBTRAFFIC,
						&(QHY_ADVANCED_PROPERTY->items[count].number.min),
						&(QHY_ADVANCED_PROPERTY->items[count].number.max),
						&(QHY_ADVANCED_PROPERTY->items[count].number.step)
					);
					QHY_ADVANCED_PROPERTY->items[count].number.value = GetQHYCCDParam(PRIVATE_DATA->handle, CONTROL_USBTRAFFIC);
					/* Kludge - default is 30 but 5LII hangs on readout with exposeures > 15s and usbtraffic < 40 so set it to 50 :) */
					QHY_ADVANCED_PROPERTY->items[count].number.value =
						(QHY_ADVANCED_PROPERTY->items[count].number.value < 50) ? 50 : QHY_ADVANCED_PROPERTY->items[count].number.value;
					SetQHYCCDParam(PRIVATE_DATA->handle, CONTROL_USBTRAFFIC, QHY_ADVANCED_PROPERTY->items[count].number.value);
					count++;
				}

				if (IsQHYCCDControlAvailable(PRIVATE_DATA->handle, CONTROL_SPEED) == QHYCCD_SUCCESS) {
					QHY_ADVANCED_PROPERTY = indigo_resize_property(QHY_ADVANCED_PROPERTY, count+1);
					indigo_init_number_item(QHY_ADVANCED_PROPERTY->items+count, USBSPEED_NAME, USBSPEED_DESC, 0, 0, 1, 0);
					GetQHYCCDParamMinMaxStep(PRIVATE_DATA->handle, CONTROL_SPEED,
						&(QHY_ADVANCED_PROPERTY->items[count].number.min),
						&(QHY_ADVANCED_PROPERTY->items[count].number.max),
						&(QHY_ADVANCED_PROPERTY->items[count].number.step)
					);
					QHY_ADVANCED_PROPERTY->items[count].number.value = GetQHYCCDParam(PRIVATE_DATA->handle, CONTROL_SPEED);
					count++;
				}

				if (IsQHYCCDControlAvailable(PRIVATE_DATA->handle, CAM_SHUTTERMOTORHEATING_INTERFACE) == QHYCCD_SUCCESS) {
					QHY_ADVANCED_PROPERTY = indigo_resize_property(QHY_ADVANCED_PROPERTY, count+1);
					indigo_init_number_item(QHY_ADVANCED_PROPERTY->items+count, SHUTTERHEATING_NAME, SHUTTERHEATING_DESC, 0, 0, 1, 0);
					GetQHYCCDParamMinMaxStep(PRIVATE_DATA->handle, CAM_SHUTTERMOTORHEATING_INTERFACE,
						&(QHY_ADVANCED_PROPERTY->items[count].number.min),
						&(QHY_ADVANCED_PROPERTY->items[count].number.max),
						&(QHY_ADVANCED_PROPERTY->items[count].number.step)
					);
					QHY_ADVANCED_PROPERTY->items[count].number.value = GetQHYCCDParam(PRIVATE_DATA->handle, CAM_SHUTTERMOTORHEATING_INTERFACE);
					count++;
				}
				// -------------------------------------------------------------------------------------- END

				indigo_define_property(device, QHY_ADVANCED_PROPERTY, NULL);
				pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);

				PRIVATE_DATA->can_check_temperature = true;
				indigo_set_timer(device, 0, ccd_temperature_callback, &PRIVATE_DATA->temperature_timer);

				device->is_connected = true;
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		}
	} else {
		if (device->is_connected) {
			if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
				indigo_cancel_timer_sync(device, &PRIVATE_DATA->exposure_timer);
				qhy_abort_exposure(device, false);
			}
			PRIVATE_DATA->can_check_temperature = false;
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->temperature_timer);
			indigo_delete_property(device, PIXEL_FORMAT_PROPERTY, NULL);
#ifdef QHY2
			indigo_delete_property(device, READ_MODE_PROPERTY, NULL);
#endif
			indigo_delete_property(device, QHY_ADVANCED_PROPERTY, NULL);
			qhy_close(device);
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
		indigo_set_timer(device, 0, ccd_connect_callback, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- CCD_EXPOSURE
	} else if (indigo_property_match(CCD_EXPOSURE_PROPERTY, property)) {
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE)
			return INDIGO_OK;
		indigo_property_copy_values(CCD_EXPOSURE_PROPERTY, property, false);
		indigo_use_shortest_exposure_if_bias(device);
		CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		qhy_start_exposure(
			device, CCD_EXPOSURE_ITEM->number.target, (CCD_FRAME_TYPE_DARK_ITEM->sw.value || CCD_FRAME_TYPE_DARKFLAT_ITEM->sw.value || CCD_FRAME_TYPE_BIAS_ITEM->sw.value),
			CCD_FRAME_LEFT_ITEM->number.value, CCD_FRAME_TOP_ITEM->number.value,
			CCD_FRAME_WIDTH_ITEM->number.value, CCD_FRAME_HEIGHT_ITEM->number.value,
			CCD_BIN_HORIZONTAL_ITEM->number.value, CCD_BIN_VERTICAL_ITEM->number.value,
			false
		);
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
			qhy_abort_exposure(device, false);
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
	} else if (indigo_property_match_w(CCD_TEMPERATURE_PROPERTY, property)) {
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
		// -------------------------------------------------------------------------------- CCD_GAMMA
	} else if (indigo_property_match(CCD_GAMMA_PROPERTY, property)) {
		CCD_GAMMA_PROPERTY->state = INDIGO_OK_STATE;
		indigo_property_copy_values(CCD_GAMMA_PROPERTY, property, false);

		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		int res = SetQHYCCDParam(PRIVATE_DATA->handle, CONTROL_GAMMA, CCD_GAMMA_ITEM->number.value);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if (res != QHYCCD_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetQHYCCDParam(%s, CONTROL_GAMMA) = %d", PRIVATE_DATA->dev_sid, res);
			CCD_GAMMA_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			CCD_GAMMA_PROPERTY->state = INDIGO_OK_STATE;
		}
		if (IS_CONNECTED)
			indigo_update_property(device, CCD_GAMMA_PROPERTY, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- CCD_GAIN
	} else if (indigo_property_match(CCD_GAIN_PROPERTY, property)) {
		if (!IS_CONNECTED) return INDIGO_OK;
		CCD_GAIN_PROPERTY->state = INDIGO_OK_STATE;
		indigo_property_copy_values(CCD_GAIN_PROPERTY, property, false);
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		int res = SetQHYCCDParam(PRIVATE_DATA->handle, CONTROL_GAIN, CCD_GAIN_ITEM->number.value);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if (res != QHYCCD_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetQHYCCDParam(%s, CONTROL_GAIN) = %d", PRIVATE_DATA->dev_sid, res);
			CCD_GAIN_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			CCD_GAIN_PROPERTY->state = INDIGO_OK_STATE;
		}
		indigo_update_property(device, CCD_GAIN_PROPERTY, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- CCD_OFFSET
	} else if (indigo_property_match(CCD_OFFSET_PROPERTY, property)) {
		if (!IS_CONNECTED) return INDIGO_OK;
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
			CCD_OFFSET_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_OFFSET_PROPERTY, "Exposure in progress, settings can not be changed.");
			return INDIGO_OK;
		}
		indigo_property_copy_values(CCD_OFFSET_PROPERTY, property, false);
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		int res = SetQHYCCDParam(PRIVATE_DATA->handle, CONTROL_OFFSET, CCD_OFFSET_ITEM->number.value);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if (res != QHYCCD_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetQHYCCDParam(%s, %s) = %d", PRIVATE_DATA->dev_sid, "OFFSET", res);
			CCD_OFFSET_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			CCD_OFFSET_PROPERTY->state = INDIGO_OK_STATE;
		}
		indigo_update_property(device, CCD_OFFSET_PROPERTY, NULL);
		return INDIGO_OK;
		// ------------------------------------------------------------------------------- CCD_FRAME
	} else if (indigo_property_match(CCD_FRAME_PROPERTY, property)) {
		indigo_property_copy_values(CCD_FRAME_PROPERTY, property, false);
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

		int horizontal_bin = (int)CCD_BIN_HORIZONTAL_ITEM->number.value;
		int vertical_bin = (int)CCD_BIN_VERTICAL_ITEM->number.value;
		char name[32] = "";
		if (PIXEL_FORMAT_PROPERTY->items[0].sw.value) {
			snprintf(name, 32, "RAW 8 %dx%d", horizontal_bin, vertical_bin);
			CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = 8;
		} else {
			snprintf(name, 32, "RAW 16 %dx%d", horizontal_bin, vertical_bin);
			CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = 16;
		}
		for (int i = 0; i < CCD_MODE_PROPERTY->count; i++) {
			indigo_item *item = &CCD_MODE_PROPERTY->items[i];
			item->sw.value = !strcmp(item->name, name);
		}
		if (IS_CONNECTED) {
			CCD_FRAME_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_FRAME_PROPERTY, NULL);
			CCD_MODE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_MODE_PROPERTY, NULL);
			PIXEL_FORMAT_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, PIXEL_FORMAT_PROPERTY, NULL);
		}
		return INDIGO_OK;
#ifdef QHY2
	// -------------------------------------------------------------------------------- READ_MODE
	} else if (indigo_property_match(READ_MODE_PROPERTY, property)) {
		if (!IS_CONNECTED) return INDIGO_OK;
		indigo_property_copy_values(READ_MODE_PROPERTY, property, false);
		READ_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
		for (int i = 0; i < READ_MODE_PROPERTY->count; i++) {
			indigo_item *item = READ_MODE_PROPERTY->items + i;
			if (item->sw.value) {
				pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
				if (SetQHYCCDReadMode(PRIVATE_DATA->handle, i) == QHYCCD_SUCCESS) {
					double chipw, chiph;
					GetQHYCCDChipInfo(PRIVATE_DATA->handle, &chipw, &chiph, &PRIVATE_DATA->total_frame_width, &PRIVATE_DATA->total_frame_height, &PRIVATE_DATA->pixel_width, &PRIVATE_DATA->pixel_height, &PRIVATE_DATA->bpp);
					CCD_INFO_WIDTH_ITEM->number.value = PRIVATE_DATA->total_frame_width;
					CCD_INFO_HEIGHT_ITEM->number.value = PRIVATE_DATA->total_frame_height;
					CCD_INFO_PIXEL_SIZE_ITEM->number.value = PRIVATE_DATA-> pixel_width;
					CCD_INFO_PIXEL_WIDTH_ITEM->number.value = PRIVATE_DATA->pixel_width;
					CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = PRIVATE_DATA->pixel_height;
					indigo_update_property(device, CCD_INFO_PROPERTY, NULL);
					PRIVATE_DATA->frame_width = PRIVATE_DATA->frame_height = 0;
					GetQHYCCDEffectiveArea(PRIVATE_DATA->handle, &PRIVATE_DATA->frame_offset_x, &PRIVATE_DATA->frame_offset_y, &PRIVATE_DATA->frame_width, &PRIVATE_DATA->frame_height);
					if ((PRIVATE_DATA->frame_width == 0) || (PRIVATE_DATA->frame_height == 0)) {
						PRIVATE_DATA->frame_width = PRIVATE_DATA->total_frame_width;
						PRIVATE_DATA->frame_height = PRIVATE_DATA->total_frame_height;
					}
					CCD_FRAME_LEFT_ITEM->number.value = CCD_FRAME_TOP_ITEM->number.value = 0;
					CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max = PRIVATE_DATA->frame_width;
					CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max = PRIVATE_DATA->frame_height;
					indigo_update_property(device, CCD_FRAME_PROPERTY, NULL);
					indigo_delete_property(device, CCD_MODE_PROPERTY, NULL);
					int count = 0;
					char name[32], label[64];
					for (int bin = 1; bin <= PRIVATE_DATA->max_bin; bin++) {
						if (!PRIVATE_DATA->bins_ok[bin-1])
							continue;
						if (bpp_supported(device, 8)) {
							snprintf(name, 32, "%s %dx%d", RAW8_NAME, bin, bin);
							snprintf(label, 64, "%s %dx%d", RAW8_NAME, (int)CCD_FRAME_WIDTH_ITEM->number.value / bin, (int)CCD_FRAME_HEIGHT_ITEM->number.value / bin);
							indigo_init_switch_item(CCD_MODE_PROPERTY->items + count, name, label, bin == 1 && PIXEL_FORMAT_PROPERTY->items[0].sw.value);
							count++;
						}
						if (bpp_supported(device, 16)) {
							snprintf(name, 32, "%s %dx%d", RAW16_NAME, bin, bin);
							snprintf(label, 64, "%s %dx%d", RAW16_NAME, (int)CCD_FRAME_WIDTH_ITEM->number.value / bin, (int)CCD_FRAME_HEIGHT_ITEM->number.value / bin);
							indigo_init_switch_item(CCD_MODE_PROPERTY->items + count, name, label, bin == 1  && PIXEL_FORMAT_PROPERTY->items[1].sw.value);
							count++;
						}
					}
					CCD_MODE_PROPERTY->count = count;
					indigo_define_property(device, CCD_MODE_PROPERTY, NULL);
					READ_MODE_PROPERTY->state = INDIGO_OK_STATE;
				}
				pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
				break;
			}
		}
		indigo_update_property(device, READ_MODE_PROPERTY, NULL);
		return INDIGO_OK;
#endif
		// -------------------------------------------------------------------------------- ADVANCED
	} else if (indigo_property_match(QHY_ADVANCED_PROPERTY, property)) {
		if (!IS_CONNECTED) return INDIGO_OK;
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
			QHY_ADVANCED_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, QHY_ADVANCED_PROPERTY, "Exposure in progress, advanced settings can not be changed.");
			return INDIGO_OK;
		}
		handle_advanced_property(device, property);
		indigo_property_copy_values(QHY_ADVANCED_PROPERTY, property, false);
		QHY_ADVANCED_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, QHY_ADVANCED_PROPERTY, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- CCD_MODE
	} else if (indigo_property_match(CCD_MODE_PROPERTY, property)) {
		indigo_property_copy_values(CCD_MODE_PROPERTY, property, false);
		int bpp, h, v;
		for (int i = 0; i < CCD_MODE_PROPERTY->count; i++) {
			indigo_item *item = &CCD_MODE_PROPERTY->items[i];
			if (item->sw.value) {
				if (sscanf(item->name, "RAW %d %dx%d", &bpp, &h, &v) == 3) {
					CCD_BIN_HORIZONTAL_ITEM->number.value = CCD_BIN_HORIZONTAL_ITEM->number.target = h;
					CCD_BIN_VERTICAL_ITEM->number.value = CCD_BIN_VERTICAL_ITEM->number.target = v;
					CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = bpp;
					if (!PIXEL_FORMAT_PROPERTY->hidden) {
						if (bpp == 8) {
							PIXEL_FORMAT_PROPERTY->items[0].sw.value = true;
							PIXEL_FORMAT_PROPERTY->items[1].sw.value = false;
						} else {
							PIXEL_FORMAT_PROPERTY->items[0].sw.value = false;
							PIXEL_FORMAT_PROPERTY->items[1].sw.value = true;
						}
					}
				}
				break;
			}
		}
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
		int prev_bin_x = (int)CCD_BIN_HORIZONTAL_ITEM->number.value;
		int prev_bin_y = (int)CCD_BIN_VERTICAL_ITEM->number.value;

		indigo_property_copy_values(CCD_BIN_PROPERTY, property, false);

		/* Some QHY cameras requires BIN_X and BIN_Y to be equal, there is no way to
		   tell which ones, so keep them entangled */
		if ((int)CCD_BIN_HORIZONTAL_ITEM->number.value != prev_bin_x) {
			CCD_BIN_VERTICAL_ITEM->number.value = CCD_BIN_HORIZONTAL_ITEM->number.value;
		} else if ((int)CCD_BIN_VERTICAL_ITEM->number.value != prev_bin_y) {
			CCD_BIN_HORIZONTAL_ITEM->number.value = CCD_BIN_VERTICAL_ITEM->number.value;
		}

		int horizontal_bin = (int)CCD_BIN_HORIZONTAL_ITEM->number.value;
		int vertical_bin = (int)CCD_BIN_VERTICAL_ITEM->number.value;
		char name[32] = "";
		if (PIXEL_FORMAT_PROPERTY->hidden) {
			snprintf(name, 32, "RAW %d %dx%d", (int)CCD_INFO_BITS_PER_PIXEL_ITEM->number.value, horizontal_bin, vertical_bin);
		} else if (PIXEL_FORMAT_PROPERTY->items[0].sw.value) {
			snprintf(name, 32, "RAW 8 %dx%d", horizontal_bin, vertical_bin);
		} else {
			snprintf(name, 32, "RAW 16 %dx%d", horizontal_bin, vertical_bin);
		}
		for (int i = 0; i < CCD_MODE_PROPERTY->count; i++) {
			indigo_item *item = &CCD_MODE_PROPERTY->items[i];
			item->sw.value = !strcmp(item->name, name);
		}
		if (IS_CONNECTED) {
			CCD_MODE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_MODE_PROPERTY, NULL);
			CCD_BIN_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_BIN_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, PIXEL_FORMAT_PROPERTY);
#ifdef QHY2
			indigo_save_property(device, NULL, READ_MODE_PROPERTY);
#endif
			indigo_save_property(device, NULL, QHY_ADVANCED_PROPERTY);
		}
	}
	return indigo_ccd_change_property(device, client, property);
}


static indigo_result ccd_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		ccd_connect_callback(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);

	indigo_release_property(PIXEL_FORMAT_PROPERTY);
#ifdef QHY2
	indigo_release_property(READ_MODE_PROPERTY);
#endif
	indigo_release_property(QHY_ADVANCED_PROPERTY);

	return indigo_ccd_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO guider device implementation

static indigo_result guider_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_guider_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		//INDIGO_DRIVER_LOG(DRIVER_NAME, "%s attached", device->name);
		return indigo_guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void guider_connect_callback(indigo_device *device) {
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
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->guider_timer_dec);
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->guider_timer_ra);
			qhy_close(device);
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

	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, guider_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_DEC
		indigo_property_copy_values(GUIDER_GUIDE_DEC_PROPERTY, property, false);
		indigo_cancel_timer(device, &PRIVATE_DATA->guider_timer_dec);
		if ((GUIDER_GUIDE_NORTH_ITEM->number.value > 0) || (GUIDER_GUIDE_SOUTH_ITEM->number.value > 0)) {
			indigo_set_timer(device, 0, guider_timer_callback_dec, &PRIVATE_DATA->guider_timer_dec);
			GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
		} else {
			GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
		}
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDER_GUIDE_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_RA
		indigo_property_copy_values(GUIDER_GUIDE_RA_PROPERTY, property, false);
		indigo_cancel_timer(device, &PRIVATE_DATA->guider_timer_ra);
		if ((GUIDER_GUIDE_EAST_ITEM->number.value > 0) || (GUIDER_GUIDE_WEST_ITEM->number.value > 0)) {
			indigo_set_timer(device, 0, guider_timer_callback_ra, &PRIVATE_DATA->guider_timer_ra);
			GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
		} else {
			GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
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
		guider_connect_callback(device);
	}
	if (device == device->master_device)
		indigo_global_unlock(device);

	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_guider_detach(device);
}

// -------------------------------------------------------------------------------- FILTER WHEEL

static void wheel_timer_callback(indigo_device *device) {
	int res;
	char currentpos[64];

	if (!CONNECTION_CONNECTED_ITEM->sw.value) return;

	int checktimes = 0;
	while(checktimes++ < 90) {
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		res = GetQHYCCDCFWStatus(PRIVATE_DATA->handle, currentpos);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);

		if (res != QHYCCD_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "GetQHYCCDCFWStatus(%s) = %d.", PRIVATE_DATA->dev_sid, res);
			return;
		}
		PRIVATE_DATA->fw_current_slot = WHEEL_SLOT_ITEM->number.value;
		//WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->fw_current_slot;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "GetQHYCCDCFWStatus(%s) fw_target_slot = %d %d", PRIVATE_DATA->dev_sid, PRIVATE_DATA->fw_target_slot, currentpos[0]);

		if (currentpos[0] == (PRIVATE_DATA->fw_target_slot + 1)) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "BREAK");
			break;
		}
	}
	WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s filter = %d.", PRIVATE_DATA->dev_sid, PRIVATE_DATA->fw_current_slot);
}

static indigo_result wheel_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_wheel_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		return indigo_wheel_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void wheel_connect_callback(indigo_device *device) {
	int res;
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (!device->is_connected) {
			CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CONNECTION_PROPERTY, NULL);
			if (qhy_open(device)) {
				char targetpos = '1';
				pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
				res = SendOrder2QHYCCDCFW(PRIVATE_DATA->handle, &targetpos, 1);
				pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
				if (res != QHYCCD_SUCCESS) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "SendOrder2QHYCCDCFW(%s) = %d.", PRIVATE_DATA->dev_sid, res);
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
					return;
				}
				PRIVATE_DATA->fw_current_slot = -1;
				WHEEL_SLOT_ITEM->number.value = 1;
				PRIVATE_DATA->fw_count = FW_COUNT;
				PRIVATE_DATA->fw_target_slot = targetpos;

				WHEEL_SLOT_ITEM->number.max = WHEEL_SLOT_NAME_PROPERTY->count = WHEEL_SLOT_OFFSET_PROPERTY->count = PRIVATE_DATA->fw_count;
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "SendOrder2QHYCCDCFW(%s) fw_current_slot = %d", PRIVATE_DATA->dev_sid, PRIVATE_DATA->fw_current_slot);
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
				indigo_set_timer(device, 0.5, wheel_timer_callback, &PRIVATE_DATA->wheel_timer);
				device->is_connected = true;
			} else {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		}
	} else { /* disconnect */
		if (device->is_connected) {
			qhy_close(device);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			device->is_connected = false;
		}
	}
	CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	indigo_wheel_change_property(device, NULL, CONNECTION_PROPERTY);
}

static indigo_result wheel_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	int res;
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, wheel_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(WHEEL_SLOT_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- WHEEL_SLOT
		indigo_property_copy_values(WHEEL_SLOT_PROPERTY, property, false);
		if (WHEEL_SLOT_ITEM->number.value < 1 || WHEEL_SLOT_ITEM->number.value > WHEEL_SLOT_ITEM->number.max) {
			WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			WHEEL_SLOT_PROPERTY->state = INDIGO_BUSY_STATE;
			int fw_target_slot = WHEEL_SLOT_ITEM->number.value;
			PRIVATE_DATA->fw_current_slot = WHEEL_SLOT_ITEM->number.value;

			char targetpos = '0' + (fw_target_slot - 1);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Requested filter %d %c", fw_target_slot, targetpos);
			PRIVATE_DATA->fw_target_slot = targetpos;

			pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
			res = SendOrder2QHYCCDCFW(PRIVATE_DATA->handle, &targetpos, 1);
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			if (res != QHYCCD_SUCCESS) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "SendOrder2QHYCCDCFW(%s) = %d.", PRIVATE_DATA->dev_sid, res);
				WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
				pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
				return INDIGO_FAILED;
			}
			indigo_set_timer(device, 0.5, wheel_timer_callback, &PRIVATE_DATA->wheel_timer);
		}
		indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_wheel_change_property(device, client, property);
}

static indigo_result wheel_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		wheel_connect_callback(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_wheel_detach(device);
}

// -------------------------------------------------------------------------------- hot-plug support

//static pthread_mutex_t device_mutex = PTHREAD_MUTEX_INITIALIZER;

#define MAX_DEVICES                   32
#define NOT_FOUND                    (-1)

static indigo_device *devices[MAX_DEVICES] = {NULL};

static indigo_device ccd_template = INDIGO_DEVICE_INITIALIZER(
	"",
	ccd_attach,
	qhy_enumerate_properties,
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

static indigo_device wheel_template = INDIGO_DEVICE_INITIALIZER(
	"",
	wheel_attach,
	indigo_wheel_enumerate_properties,
	wheel_change_property,
	NULL,
	wheel_detach
);

#ifdef HOTPLUG

static bool find_plugged_device_sid(char *new_sid) {
	int i;
	char sid[MAX_SID_LEN] = { 0 };
	bool found = false;

	int count = ScanQHYCCD();
	for (i = 0; i < count; i++) {
		GetQHYCCDId(i, sid);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME,"+ %d of %d: %s", i , count, sid);
		found = false;
		for (int slot = 0; slot < MAX_DEVICES; slot++) {
			indigo_device *device = devices[slot];
			if (device == NULL)
				continue;
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
	for (int slot = 0; slot < MAX_DEVICES; slot++) {
		if (devices[slot] == NULL)
			return slot;
	}
	return NOT_FOUND;
}

static int find_unplugged_device_slot() {
	int slot;
	indigo_device *device;
	char sid[MAX_SID_LEN] = {0};
	bool found = true;

	int count = ScanQHYCCD();
	for (slot = 0; slot < MAX_DEVICES; slot++) {
		device = devices[slot];
		if (device == NULL) continue;
		found = false;
		for (int i = 0; i < count; i++) {
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

	pthread_mutex_lock(&device_mutex);
	int slot = find_available_device_slot();
	if (slot < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "No device slots available.");
		pthread_mutex_unlock(&device_mutex);
		return;
	}

	char sid[MAX_SID_LEN];
	bool found = find_plugged_device_sid(sid);
	if (!found) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "No plugged device found.");
		pthread_mutex_unlock(&device_mutex);
		return;
	}

	char dev_usbpath[MAX_SID_LEN];
	char dev_name[MAX_SID_LEN];
	GetQHYCCDModel(sid, dev_name);

	/* Check if there is a guider port and get usbpath */
	qhyccd_handle *handle;
	handle = OpenQHYCCD(sid);
	if (handle == NULL) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Camera %s can not be open.", sid);
		pthread_mutex_unlock(&device_mutex);
		return;
	}
	int check_st4 = IsQHYCCDControlAvailable(handle, CONTROL_ST4PORT);
	int check_wheel = IsQHYCCDControlAvailable(handle, CONTROL_CFWPORT);
	indigo_get_usb_path(libusb_get_device((libusb_device_handle *)handle), dev_usbpath);
	CloseQHYCCD(handle);

	indigo_device *device = (indigo_device*)malloc(sizeof(indigo_device));
	indigo_device *master_device = device;
	assert(device != NULL);
	memcpy(device, &ccd_template, sizeof(indigo_device));
	device->master_device = master_device;
	sprintf(device->name, "%s #%s", dev_name, dev_usbpath);
	INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
	qhy_private_data *private_data = (qhy_private_data*)indigo_safe_malloc(sizeof(qhy_private_data));
	sprintf(private_data->dev_sid, "%s", sid);
	device->private_data = private_data;
	indigo_attach_device(device);
	devices[slot]=device;

	if (check_st4 == QHYCCD_SUCCESS) {
		slot = find_available_device_slot();
		if (slot < 0) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "No device slots available.");
			pthread_mutex_unlock(&device_mutex);
			return;
		}
		device = indigo_safe_malloc_copy(sizeof(indigo_device), &guider_template);
		device->master_device = master_device;
		sprintf(device->name, "%s Guider #%s", dev_name, dev_usbpath);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		private_data->fw_count = FW_COUNT; /* No way to get it from SDK but all QHY FWs have 5 or 7 slots */
		device->private_data = private_data;
		indigo_attach_device(device);
		devices[slot]=device;
	}

	if (check_wheel == QHYCCD_SUCCESS) {
		slot = find_available_device_slot();
		if (slot < 0) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "No device slots available.");
			pthread_mutex_unlock(&device_mutex);
			return;
		}
		device = indigo_safe_malloc_copy(sizeof(indigo_device), &wheel_template);
		device->master_device = master_device;
		sprintf(device->name, "%s Wheel #%s", dev_name, dev_usbpath);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		device->private_data = private_data;
		indigo_attach_device(device);
		devices[slot]=device;
	}

	pthread_mutex_unlock(&device_mutex);
}

static void process_unplug_event() {
	int slot;
	bool removed = false;
	qhy_private_data *private_data = NULL;
	pthread_mutex_lock(&device_mutex);
	while ((slot = find_unplugged_device_slot()) != NOT_FOUND) {
		indigo_device **device = &devices[slot];
		if (*device == NULL) {
			pthread_mutex_unlock(&device_mutex);
			return;
		}
		qhy_close(*device);
		indigo_detach_device(*device);
		if ((*device)->private_data) {
			private_data = (qhy_private_data*)((*device)->private_data);
		}
		free(*device);
		*device = NULL;
		removed = true;
	}

	if (private_data) {
		free(private_data);
		private_data = NULL;
	}

	if (!removed) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "No unplugged device found.");
	}
	pthread_mutex_unlock(&device_mutex);
}

//#ifdef __APPLE__
static void *plug_thread_func(void *sid) {
	#ifdef __APPLE__
	char firmware_base_dir[1024] = "/usr/local/lib/qhy";
	if (getenv("INDIGO_FIRMWARE_BASE") != NULL) {
		strncpy(firmware_base_dir, getenv("INDIGO_FIRMWARE_BASE"), 1024);
	}
	OSXInitQHYCCDFirmware(firmware_base_dir);
	indigo_usleep(3000000);
	#else
	indigo_usleep(1000000);
	#endif /* __APPLE__ */
	process_plug_event();
	pthread_exit(NULL);
	return NULL;
}

static void *unplug_thread_func(void *sid) {
	#ifdef __APPLE__
	indigo_usleep(3000000);
	#else
	indigo_usleep(1000000);
	#endif /* __APPLE__ */
	process_unplug_event();
	pthread_exit(NULL);
	return NULL;
}

static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	struct libusb_device_descriptor descriptor;

	libusb_get_device_descriptor(dev, &descriptor);
	if ((descriptor.idVendor != QHY_VENDOR_ID1) &&
		(descriptor.idVendor != QHY_VENDOR_ID2) &&
		(descriptor.idVendor != QHY_VENDOR_ID3) &&
		(descriptor.idVendor != QHY_VENDOR_ID4) &&
		(descriptor.idVendor != QHY_VENDOR_ID5)) {
		return 0;
	}
	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Hot plug: vid=%x pid=%x", descriptor.idVendor, descriptor.idProduct);
			if (!indigo_async(plug_thread_func, NULL)) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME,"Error creating thread for plug handler");
			}
			break;
		}
		case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Hot unplug: vid=%x pid=%x", descriptor.idVendor, descriptor.idProduct);
			if (!indigo_async(unplug_thread_func, NULL)) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME,"Error creating thread for unplug handler");
			}
			break;
		}
	}
	return 0;
};

static libusb_hotplug_callback_handle callback_handle;

#else

static void add_all_devices() {
#ifdef __APPLE__
	char firmware_base_dir[1024] = "/usr/local/lib/qhy";
	if (getenv("INDIGO_FIRMWARE_BASE") != NULL) {
		strncpy(firmware_base_dir, getenv("INDIGO_FIRMWARE_BASE"), 1024);
	}
	OSXInitQHYCCDFirmware(firmware_base_dir);
#endif /* __APPLE__ */
	int count = ScanQHYCCD();
	int slot = 0;
	for (int i = 0; i < count; i++) {
		if (slot == MAX_DEVICES)
			break;
		char sid[MAX_SID_LEN] = { 0 };
		GetQHYCCDId(i, sid);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME,"%d of %d: %s", i + 1 , count, sid);
		char dev_usbpath[MAX_SID_LEN];
		char dev_name[MAX_SID_LEN];
		GetQHYCCDModel(sid, dev_name);
		qhyccd_handle *handle;
		handle = OpenQHYCCD(sid);
		if (handle == NULL) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Camera %s can not be open.", sid);
			continue;
		}
		int check_st4 = IsQHYCCDControlAvailable(handle, CONTROL_ST4PORT);
		int check_wheel = IsQHYCCDControlAvailable(handle, CONTROL_CFWPORT);
		indigo_get_usb_path(libusb_get_device((libusb_device_handle *)handle), dev_usbpath);
		CloseQHYCCD(handle);
		indigo_device *device = (indigo_device*)malloc(sizeof(indigo_device));
		indigo_device *master_device = device;
		assert(device != NULL);
		memcpy(device, &ccd_template, sizeof(indigo_device));
		device->master_device = master_device;
		sprintf(device->name, "%s #%s", dev_name, dev_usbpath);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		qhy_private_data *private_data = (qhy_private_data*)malloc(sizeof(qhy_private_data));
		assert(private_data);
		memset(private_data, 0, sizeof(qhy_private_data));
		sprintf(private_data->dev_sid, "%s", sid);
		device->private_data = private_data;
		indigo_attach_device(device);
		devices[slot++] = device;
		if (slot == MAX_DEVICES)
			break;
		if (check_st4 == QHYCCD_SUCCESS) {
			device = (indigo_device*)malloc(sizeof(indigo_device));
			assert(device != NULL);
			memcpy(device, &guider_template, sizeof(indigo_device));
			device->master_device = master_device;
			sprintf(device->name, "%s Guider #%s", dev_name, dev_usbpath);
			INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
			private_data->fw_count = FW_COUNT;
			device->private_data = private_data;
			indigo_attach_device(device);
			devices[slot++] = device;
		}
		if (slot == MAX_DEVICES)
			break;
		if (check_wheel == QHYCCD_SUCCESS) {
			device = (indigo_device*)malloc(sizeof(indigo_device));
			assert(device != NULL);
			memcpy(device, &wheel_template, sizeof(indigo_device));
			device->master_device = master_device;
			sprintf(device->name, "%s Wheel #%s", dev_name, dev_usbpath);
			INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
			device->private_data = private_data;
			indigo_attach_device(device);
			devices[slot++] = device;
		}
	}
}

#endif

static void remove_all_devices() {
	for (int i = 0; i < MAX_DEVICES; i++) {
		indigo_device *device = devices[i];
		if (device == NULL)
			continue;
		qhy_close(device);
		indigo_detach_device(device);
	}
	for (int i = 0; i < MAX_DEVICES; i++) {
		indigo_device *device = devices[i];
		if (device == NULL)
			continue;
		if (device->master_device == device)
			free(device->private_data);
		free(device);
		devices[i] = NULL;
	}
}

indigo_result INDIGO_CCD_QHY(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	int rc;
	SET_DRIVER_INFO(info, DRIVER_DESCRIPTION, __FUNCTION__, DRIVER_VERSION, true, last_action);
	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			if (indigo_driver_initialized((char *)CONFLICTING_DRIVER)) {
				INDIGO_DRIVER_LOG(DRIVER_NAME, "Conflicting driver %s is already loaded", CONFLICTING_DRIVER);
				last_action = INDIGO_DRIVER_SHUTDOWN;
				return INDIGO_FAILED;
			}
#ifdef QHY2
			SetQHYCCDAutoDetectCamera(false);
#endif  // new SDK
			SetQHYCCDLogLevel(6);
			rc = InitQHYCCDResource();
			if (rc != QHYCCD_SUCCESS)
				return INDIGO_FAILED;
#ifdef HOTPLUG
			indigo_start_usb_event_handler();
			rc = libusb_hotplug_register_callback(NULL, (libusb_hotplug_event)(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT), LIBUSB_HOTPLUG_ENUMERATE, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_register_callback ->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
			return rc >= 0 ? INDIGO_OK : INDIGO_FAILED;
#else
			INDIGO_ASYNC(add_all_devices, NULL);
			return INDIGO_OK;
#endif

		case INDIGO_DRIVER_SHUTDOWN:
			for (int i = 0; i < MAX_DEVICES; i++)
				VERIFY_NOT_CONNECTED(devices[i]);
			last_action = action;
#ifdef HOTPLUG
			libusb_hotplug_deregister_callback(NULL, callback_handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_deregister_callback");
#endif
			remove_all_devices();
			ReleaseQHYCCDResource();
			break;

		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}

#else

indigo_result INDIGO_CCD_QHY(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, DRIVER_DESCRIPTION, __FUNCTION__, DRIVER_VERSION, true, last_action);

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
