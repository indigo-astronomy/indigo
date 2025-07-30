// Copyright (c) 2018 CloudMakers, s. r. o.
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
// 2.0 by Rumen Bogdanovski <rumenastro@gmail.com>

/** INDIGO ToupTek CCD, filter wheel & focuser driver
 \file indigo_ccd_touptek.c
 */

#define DRIVER_VERSION 0x0027

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <ctype.h>
#include <sys/time.h>

#include <indigo/indigo_usb_utils.h>
#include <indigo/indigo_driver_xml.h>

#if defined(ALTAIR)

#define ENTRY_POINT						indigo_ccd_altair
#define CAMERA_NAME_PREFIX		"Altair"
#define DRIVER_LABEL					"AltairAstro Camera"
#define DRIVER_NAME						"indigo_ccd_altair"
#define DRIVER_PRIVATE_DATA		altair_private_data

#define SDK_CALL(x)						Altaircam_##x
#define SDK_DEF(x)						ALTAIRCAM_##x
#define SDK_TYPE(x)						Altaircam##x
#define SDK_HANDLE						HAltaircam

#include <altaircam.h>
#include "../ccd_altair/indigo_ccd_altair.h"

#elif defined(BRESSER)

#define ENTRY_POINT						indigo_ccd_bresser
#define CAMERA_NAME_PREFIX		"Bresser"
#define DRIVER_LABEL					"Bresser Camera"
#define DRIVER_NAME						"indigo_ccd_bresser"
#define DRIVER_PRIVATE_DATA		bresser_private_data

#define SDK_CALL(x)						Bressercam_##x
#define SDK_DEF(x)						BRESSERCAM_##x
#define SDK_TYPE(x)						Bressercam##x
#define SDK_HANDLE						HBressercam

#include <bressercam.h>
#include "../ccd_bresser/indigo_ccd_bresser.h"

#elif defined(OMEGONPRO)

#define ENTRY_POINT						indigo_ccd_omegonpro
#define CAMERA_NAME_PREFIX		"Omegon"
#define DRIVER_LABEL					"OmegonPro Camera"
#define DRIVER_NAME						"indigo_ccd_omegonpro"
#define DRIVER_PRIVATE_DATA		omegonpro_private_data

#define SDK_CALL(x)						Omegonprocam_##x
#define SDK_DEF(x)						OMEGONPROCAM_##x
#define SDK_TYPE(x)						Omegonprocam##x
#define SDK_HANDLE						HOmegonprocam

#include <omegonprocam.h>
#include "../ccd_omegonpro/indigo_ccd_omegonpro.h"

#elif defined(STARSHOOTG)

#define ENTRY_POINT						indigo_ccd_ssg
#define CAMERA_NAME_PREFIX		"Orion"
#define DRIVER_LABEL					"Orion StarShot G Camera"
#define DRIVER_NAME						"indigo_ccd_ssg"
#define DRIVER_PRIVATE_DATA		ssg_private_data

#define SDK_CALL(x)						Starshootg_##x
#define SDK_DEF(x)						STARSHOOTG_##x
#define SDK_TYPE(x)						Starshootg##x
#define SDK_HANDLE						HStarshootg

#include <starshootg.h>
#include "../ccd_ssg/indigo_ccd_ssg.h"

#elif defined(RISING)

#define ENTRY_POINT						indigo_ccd_rising
#define CAMERA_NAME_PREFIX		"RisingCam"
#define DRIVER_LABEL					"RisingCam Camera"
#define DRIVER_NAME						"indigo_ccd_rising"
#define DRIVER_PRIVATE_DATA		rising_private_data

#define SDK_CALL(x)						Nncam_##x
#define SDK_DEF(x)						NNCAM_##x
#define SDK_TYPE(x)						Nncam##x
#define SDK_HANDLE						HNncam

#include <nncam.h>
#include "../ccd_rising/indigo_ccd_rising.h"

#elif defined(MALLIN)

#define ENTRY_POINT						indigo_ccd_mallin
#define CAMERA_NAME_PREFIX		"MallinCam"
#define DRIVER_LABEL					"MallinCam Camera"
#define DRIVER_NAME						"indigo_ccd_mallin"
#define DRIVER_PRIVATE_DATA		mallin_private_data

#define SDK_CALL(x)						Mallincam_##x
#define SDK_DEF(x)						MALLINCAM_##x
#define SDK_TYPE(x)						Mallincam##x
#define SDK_HANDLE						HMallincam

#include <mallincam.h>
#include "../ccd_mallin/indigo_ccd_mallin.h"

#elif defined(OGMA)

#define ENTRY_POINT						indigo_ccd_ogma
#define CAMERA_NAME_PREFIX		"OGMA"
#define DRIVER_LABEL					"OGMA Camera"
#define DRIVER_NAME						"indigo_ccd_ogma"
#define DRIVER_PRIVATE_DATA		ogma_private_data

#define SDK_CALL(x)						Ogmacam_##x
#define SDK_DEF(x)						OGMACAM_##x
#define SDK_TYPE(x)						Ogmacam##x
#define SDK_HANDLE						HOgmacam

#include <ogmacam.h>
#include "../ccd_ogma/indigo_ccd_ogma.h"

#elif defined(SVBONY)

#define ENTRY_POINT						indigo_ccd_svb2
#define CAMERA_NAME_PREFIX		"SvBony"
#define DRIVER_LABEL					"SVBONY (OEM) Camera"
#define DRIVER_NAME						"indigo_ccd_svb2"
#define DRIVER_PRIVATE_DATA		svb2_private_data

#define SDK_CALL(x)						Svbonycam_##x
#define SDK_DEF(x)						SVBONYCAM_##x
#define SDK_TYPE(x)						Svbonycam##x
#define SDK_HANDLE						HSvbonycam

#include <svbonycam.h>
#include "../ccd_svb2/indigo_ccd_svb2.h"

#else

#define TOUPTEK

#define ENTRY_POINT						indigo_ccd_touptek
#define CAMERA_NAME_PREFIX		"Touptek"
#define DRIVER_LABEL					"Touptek Camera"
#define DRIVER_NAME						"indigo_ccd_touptek"
#define DRIVER_PRIVATE_DATA		touptek_private_data

#define SDK_CALL(x)						Toupcam_##x
#define SDK_DEF(x)						TOUPCAM_##x
#define SDK_TYPE(x)						Toupcam##x
#define SDK_HANDLE						HToupCam

#include <toupcam.h>
#include "../ccd_touptek/indigo_ccd_touptek.h"

#endif

#define PRIVATE_DATA        							((DRIVER_PRIVATE_DATA *)device->private_data)

#define X_CCD_ADVANCED_PROPERTY						(PRIVATE_DATA->advanced_property)
#define X_CCD_SPEED_ITEM									(X_CCD_ADVANCED_PROPERTY->items + 0)
#define X_CCD_CONTRAST_ITEM								(X_CCD_ADVANCED_PROPERTY->items + 1)
#define X_CCD_HUE_ITEM										(X_CCD_ADVANCED_PROPERTY->items + 2)
#define X_CCD_SATURATION_ITEM							(X_CCD_ADVANCED_PROPERTY->items + 3)
#define X_CCD_BRIGHTNESS_ITEM							(X_CCD_ADVANCED_PROPERTY->items + 4)
#define X_CCD_GAMMA_ITEM									(X_CCD_ADVANCED_PROPERTY->items + 5)
#define X_CCD_R_GAIN_ITEM									(X_CCD_ADVANCED_PROPERTY->items + 6)
#define X_CCD_G_GAIN_ITEM									(X_CCD_ADVANCED_PROPERTY->items + 7)
#define X_CCD_B_GAIN_ITEM									(X_CCD_ADVANCED_PROPERTY->items + 8)

#define X_CCD_FAN_PROPERTY								(PRIVATE_DATA->fan_property)
#define X_CCD_FAN_SPEED_ITEM							(X_CCD_FAN_PROPERTY->items + 0)

#define X_CCD_HEATER_PROPERTY								(PRIVATE_DATA->heater_property)
#define X_CCD_HEATER_POWER_ITEM								(X_CCD_HEATER_PROPERTY->items + 0)

#define X_CCD_LED_PROPERTY									(PRIVATE_DATA->led_property)
#define X_CCD_LED_ON_ITEM								(X_CCD_LED_PROPERTY->items + 0)
#define X_CCD_LED_OFF_ITEM								(X_CCD_LED_PROPERTY->items + 1)

#define X_CCD_CONVERSION_GAIN_PROPERTY						(PRIVATE_DATA->conversion_gain_property)
#define X_CCD_CONVERSION_GAIN_LCG_ITEM							(X_CCD_CONVERSION_GAIN_PROPERTY->items + 0)
#define X_CCD_CONVERSION_GAIN_HCG_ITEM							(X_CCD_CONVERSION_GAIN_PROPERTY->items + 1)
#define X_CCD_CONVERSION_GAIN_HDR_ITEM							(X_CCD_CONVERSION_GAIN_PROPERTY->items + 2)

#define X_CCD_BIN_MODE_PROPERTY									(PRIVATE_DATA->bin_mode_property)
#define X_CCD_BIN_MODE_SATURATE_ITEM							(X_CCD_BIN_MODE_PROPERTY->items + 0)
#define X_CCD_BIN_MODE_EXPAND_ITEM								(X_CCD_BIN_MODE_PROPERTY->items + 1)
#define X_CCD_BIN_MODE_AVERAGE_ITEM								(X_CCD_BIN_MODE_PROPERTY->items + 2)

#define X_BEEP_PROPERTY               (PRIVATE_DATA->beep_property)
#define X_BEEP_ON_ITEM                (X_BEEP_PROPERTY->items+0)
#define X_BEEP_OFF_ITEM               (X_BEEP_PROPERTY->items+1)
#define X_BEEP_PROPERTY_NAME          "X_AAF_BEEP"
#define X_BEEP_ON_ITEM_NAME           "ON"
#define X_BEEP_OFF_ITEM_NAME          "OFF"

typedef struct {
	SDK_TYPE(DeviceV2) cam;
	SDK_HANDLE handle;
	bool present;
	/* Camera related */
	indigo_device *camera;
	char bayer_pattern[5];
	indigo_device *guider;
	indigo_timer *exposure_watchdog_timer, *temperature_timer, *guider_timer_ra, *guider_timer_dec;
	double current_temperature;
	unsigned char *buffer;
	unsigned bin_mode;
	int bits;
	int mode;
	int left, top, width, height;
	bool aborting;
	pthread_mutex_t mutex;
	indigo_property *advanced_property;
	indigo_property *fan_property;
	indigo_property *heater_property;
	indigo_property *conversion_gain_property;
	indigo_property *bin_mode_property;
	indigo_property *led_property;
	/* wheel related */
	int current_slot, target_slot;
	int count;
	indigo_timer *wheel_timer;
	indigo_property *calibrate_property;
	indigo_property *wheel_model_property;
	/* focuser related */
	int current_position, target_position;
	int max_position;
	int backlash;
	double prev_temp;
	indigo_timer *focuser_timer;
	indigo_property *beep_property;
} DRIVER_PRIVATE_DATA;

#define ADVANCED_GROUP                 "Advanced"

#define X_CALIBRATE_PROPERTY           (PRIVATE_DATA->calibrate_property)
#define X_CALIBRATE_START_ITEM         (X_CALIBRATE_PROPERTY->items+0)
#define X_CALIBRATE_PROPERTY_NAME      "X_CALIBRATE"
#define X_CALIBRATE_START_ITEM_NAME    "START"

#define X_WHEEL_MODEL_PROPERTY           (PRIVATE_DATA->wheel_model_property)
#define X_WHEEL_MODEL_5_POSITION_ITEM    (X_WHEEL_MODEL_PROPERTY->items+0)
#define X_WHEEL_MODEL_7_POSITION_ITEM    (X_WHEEL_MODEL_PROPERTY->items+1)
#define X_WHEEL_MODEL_8_POSITION_ITEM    (X_WHEEL_MODEL_PROPERTY->items+2)
#define X_WHEEL_MODEL_PROPERTY_NAME      "X_WHEEL_MODEL"
#define X_WHEEL_MODEL_5_POSITION_ITEM_NAME "5_POSITIONS"
#define X_WHEEL_MODEL_7_POSITION_ITEM_NAME "7_POSITIONS"
#define X_WHEEL_MODEL_8_POSITION_ITEM_NAME "8_POSITIONS"


// -------------------------------------------------------------------------------- INDIGO CCD device implementation
#ifndef MAKEFOURCC
#define MAKEFOURCC(a, b, c, d) ((unsigned)(unsigned char)(a) | ((unsigned)(unsigned char)(b) << 8) | ((unsigned)(unsigned char)(c) << 16) | ((unsigned)(unsigned char)(d) << 24))
#endif

#define ROUND_BIN(dimention, bin) (2 * ((unsigned)(dimention) / (unsigned)(bin) / 2));

static bool get_blacklevel(indigo_device *device, int *blacklevel, double *scale) {
	int pixel_format;
	int blacklevel_raw;
	int result = SDK_CALL(get_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_PIXEL_FORMAT), &pixel_format);
	if (result < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "get_Option(OPTION_PIXEL_FORMAT, -> %d) = %d", pixel_format, result);
		return false;
	}
	result = SDK_CALL(get_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_BLACKLEVEL), &blacklevel_raw);
	if (result < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "get_Option(OPTION_BLACKLEVEL, -> %d) = %d", blacklevel_raw, result);
		return false;
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "get_Option(OPTION_BLACKLEVEL, -> %d) = %d", blacklevel_raw, result);
	}
	/* we normalize the values to 256 to get nice round scale,
	   however valid values would be from 0-248 as BLACKLEVEL8_MAX is 31 not 32
	*/
	switch (pixel_format) {
		case SDK_DEF(PIXELFORMAT_RAW10):
			*scale = 256.0 / (32 * 4);
			break;
		case SDK_DEF(PIXELFORMAT_RAW12):
		case SDK_DEF(PIXELFORMAT_GMCY12):
			*scale = 256.0 / (32 * 16);
			break;
		case SDK_DEF(PIXELFORMAT_RAW14):
			*scale = 256.0 / (32 * 64);
			break;
		case SDK_DEF(PIXELFORMAT_RAW16):
			*scale = 256.0 / (32 * 256);
			break;
		default:
			*scale = 256.0 / (32);
	}
	*blacklevel = blacklevel_raw;
	return true;
}

static void handle_offset(indigo_device *device) {
	int blacklevel;
	double scale = 1;
	if (get_blacklevel(device, &blacklevel, &scale)) {
		int target_blacklevel = (int)(CCD_OFFSET_ITEM->number.target / scale);
		if (blacklevel != (target_blacklevel)) {
			int result = SDK_CALL(put_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_BLACKLEVEL), target_blacklevel);
			if (result >= 0) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_Option(OPTION_BLACKLEVEL, <- %d) = %d", target_blacklevel, result);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "set blacklevel=%d, scale=%f => offset=%f", target_blacklevel, scale, target_blacklevel * scale);
				CCD_OFFSET_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, CCD_OFFSET_PROPERTY, NULL);
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "put_Option(OPTION_BLACKLEVEL, <- %d) = %d", target_blacklevel, result);
				CCD_OFFSET_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, CCD_OFFSET_PROPERTY, "Can not set camera offset");
			}
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "unchanged blacklevel=%d, scale=%f => offset=%f", target_blacklevel, scale, target_blacklevel * scale);
			CCD_OFFSET_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_OFFSET_PROPERTY, NULL);
		}
	} else {
		CCD_OFFSET_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, CCD_OFFSET_PROPERTY, "Can not set camera offset");
	}
}

static void get_bayer_pattern(indigo_device *device, char *bayer_pattern) {
	unsigned fourcc = 0, bitspp = 0;
	HRESULT result = SDK_CALL(get_RawFormat)(PRIVATE_DATA->handle, &fourcc, &bitspp);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "get_RawFormat(->%x, -> %d) = %d", fourcc, bitspp, result);
	switch (fourcc) {
		case MAKEFOURCC('G', 'B', 'R', 'G'):
			strncpy(PRIVATE_DATA->bayer_pattern,"GBRG", 5);
			break;
		case MAKEFOURCC('R', 'G', 'G', 'B'):
			strncpy(PRIVATE_DATA->bayer_pattern,"RGGB", 5);
			break;
		case MAKEFOURCC('B', 'G', 'G', 'R'):
			strncpy(PRIVATE_DATA->bayer_pattern,"BGGR", 5);
			break;
		case MAKEFOURCC('G', 'R', 'B', 'G'):
			strncpy(PRIVATE_DATA->bayer_pattern,"GRBG", 5);
			break;
		default:
			strncpy(PRIVATE_DATA->bayer_pattern,"", 5);
	}
}

static void fnish_exposure_async(indigo_device *device) {
	CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
}

static void finish_streaming_async(indigo_device *device) {
	CCD_STREAMING_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
}

static void exposure_watchdog_callback(indigo_device *device) {
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "pull_callback() was not called in time");
	CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Exposure failed, pull callback was not called");
}

/* dummy exposure callback - needed for a workaround */
static void pull_callback_dummy(unsigned event, void* callbackCtx) {
	//SDK_TYPE(FrameInfoV2) frameInfo = { 0 };
	HRESULT result;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s(%04x) called", __FUNCTION__, event);

	indigo_device *device = (indigo_device *)callbackCtx;

	switch (event) {
		case SDK_DEF(EVENT_IMAGE): {
			result = SDK_CALL(put_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_FLUSH), 3);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_Option(OPTION_FLUSH) -> %08x", result);
			break;
		}
	}
}

static void pull_callback(unsigned event, void* callbackCtx) {
	SDK_TYPE(FrameInfoV2) frameInfo = { 0 };
	HRESULT result;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "pull_callback(%04x) called", event);

	indigo_device *device = (indigo_device *)callbackCtx;

	indigo_cancel_timer_sync(device, &PRIVATE_DATA->exposure_watchdog_timer);

	indigo_fits_keyword keywords[] = {
		{ INDIGO_FITS_STRING, "BAYERPAT", .string = PRIVATE_DATA->bayer_pattern, "Bayer color pattern" },
		{ 0 }
	};
	indigo_fits_keyword *fits_keywords = NULL;
	if (PRIVATE_DATA->bayer_pattern[0] != '\0' && PRIVATE_DATA->bits != 24 && PRIVATE_DATA->bits != 48) {
		fits_keywords = keywords;
	}

	switch (event) {
		case SDK_DEF(EVENT_IMAGE): {
			pthread_mutex_lock(&PRIVATE_DATA->mutex);
			result = SDK_CALL(PullImageV2)(PRIVATE_DATA->handle, PRIVATE_DATA->buffer + FITS_HEADER_SIZE, PRIVATE_DATA->bits, &frameInfo);
			pthread_mutex_unlock(&PRIVATE_DATA->mutex);
			if (result >= 0) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "PullImageV2(%d, ->[%d x %d, %x, %d]) -> %08x", PRIVATE_DATA->bits, frameInfo.width, frameInfo.height, frameInfo.flag, frameInfo.seq, result);
				if (PRIVATE_DATA->aborting) {
					indigo_finalize_video_stream(device);
				} else {
					if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
						indigo_process_image(device, PRIVATE_DATA->buffer, frameInfo.width, frameInfo.height, PRIVATE_DATA->bits > 8 && PRIVATE_DATA->bits <= 16 ? 16 : PRIVATE_DATA->bits, true, true, fits_keywords, false);
						CCD_EXPOSURE_ITEM->number.value = 0;
						indigo_set_timer(device, 0, fnish_exposure_async, NULL);
					} else if (CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
						indigo_process_image(device, PRIVATE_DATA->buffer, frameInfo.width, frameInfo.height, PRIVATE_DATA->bits > 8 && PRIVATE_DATA->bits <= 16 ? 16 : PRIVATE_DATA->bits, true, true, fits_keywords, true);
						if (--CCD_STREAMING_COUNT_ITEM->number.value == 0) {
							indigo_finalize_video_stream(device);
							indigo_set_timer(device, 0, finish_streaming_async, NULL);
						} else if (CCD_STREAMING_COUNT_ITEM->number.value < -1) {
							CCD_STREAMING_COUNT_ITEM->number.value = -1;
							indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
						}
					}
				}
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "PullImageV2(%d, ->[%d x %d, %x, %d]) -> %08x", PRIVATE_DATA->bits, frameInfo.width, frameInfo.height, frameInfo.flag, frameInfo.seq, result);
				indigo_ccd_failure_cleanup(device);
				if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
					CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
				} else if (CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
					indigo_finalize_video_stream(device);
					CCD_STREAMING_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
				}
			}
			break;
		}
		case SDK_DEF(EVENT_NOFRAMETIMEOUT):
		case SDK_DEF(EVENT_NOPACKETTIMEOUT):
		case SDK_DEF(EVENT_ERROR): {
			indigo_ccd_failure_cleanup(device);
			CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
			break;
		}
	}
}

static void ccd_temperature_callback(indigo_device *device) {
	if (!CONNECTION_CONNECTED_ITEM->sw.value) {
		return;
	}
	short temperature;
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	HRESULT result = SDK_CALL(get_Temperature)(PRIVATE_DATA->handle, &temperature);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	if (result >= 0) {
		PRIVATE_DATA->current_temperature = CCD_TEMPERATURE_ITEM->number.value = temperature / 10.0;
		if (CCD_TEMPERATURE_PROPERTY->perm == INDIGO_RW_PERM && fabs(CCD_TEMPERATURE_ITEM->number.value - CCD_TEMPERATURE_ITEM->number.target) > 1.0) {
			if (!CCD_COOLER_PROPERTY->hidden && CCD_COOLER_OFF_ITEM->sw.value)
				CCD_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
			else
				CCD_TEMPERATURE_PROPERTY->state = INDIGO_BUSY_STATE;
		} else {
			CCD_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
		}
		indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "get_Temperature() -> %08x", result);
	}
	if (!CCD_COOLER_POWER_PROPERTY->hidden) {
		int current_voltage = 0, max_voltage = 0;
		/* When cooler is OFF current_voltage is reported as the last measured when
		   power was ON, so we set it to 0 to show correct percentage
		*/
		if (CCD_COOLER_ON_ITEM->sw.value) {
			result = SDK_CALL(get_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_TEC_VOLTAGE), &current_voltage);
		} else {
			current_voltage = 0;
		}
		result = SDK_CALL(get_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_TEC_VOLTAGE_MAX), &max_voltage);
		if (result >= 0 && max_voltage > 0) {
			double cooler_power = (double)current_voltage/max_voltage * 100;
			CCD_COOLER_POWER_PROPERTY->state = INDIGO_OK_STATE;
			CCD_COOLER_POWER_ITEM->number.value = round(cooler_power);
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "get_Option(OPTION_TEC_VOLTAGE_MAX) -> %08x", result);
			CCD_COOLER_POWER_PROPERTY->state = INDIGO_ALERT_STATE;
			CCD_COOLER_POWER_ITEM->number.value = 0;
		}
		indigo_update_property(device, CCD_COOLER_POWER_PROPERTY, NULL);
	}
	indigo_reschedule_timer(device, 5, &PRIVATE_DATA->temperature_timer);
}

static void setup_exposure(indigo_device *device) {
	HRESULT result;
	unsigned binning = 1;
	for (int i = 0; i < CCD_MODE_PROPERTY->count; i++) {
		indigo_item *item = CCD_MODE_PROPERTY->items + i;
		if (item->sw.value) {
			binning = atoi(strchr(item->name, '_') + 1);
			if (PRIVATE_DATA->mode != i) {
				result = SDK_CALL(Stop)(PRIVATE_DATA->handle);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Stop() -> %08x", result);
				//indigo_usleep(200000);
				if (strncmp(item->name, "RAW08", 5) == 0 || strncmp(item->name, "MON08", 5) == 0) {
					result = SDK_CALL(put_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_RAW), 1);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_Option(OPTION_RAW, 1) -> %08x", result);
					result = SDK_CALL(put_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_BITDEPTH), 0);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_Option(OPTION_BITDEPTH, 0) -> %08x", result);
					PRIVATE_DATA->bits = 8;
					indigo_usleep(100000);
				} else if (strncmp(item->name, "RAW", 3) == 0 || strncmp(item->name, "MON", 3) == 0) {
					result = SDK_CALL(put_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_BITDEPTH), 1);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_Option(OPTION_BITDEPTH, 1) -> %08x", result);
					result = SDK_CALL(put_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_RAW), 1);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_Option(OPTION_RAW, 1) -> %08x", result);
					PRIVATE_DATA->bits = atoi(item->name + 3); // FIXME: should be ignored in RAW mode, but it is not
					indigo_usleep(100000);
				} else if (strncmp(item->name, "RGB08", 5) == 0) {
					result = SDK_CALL(put_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_RAW), 0);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_Option(OPTION_RAW, 0) -> %08x", result);
					result = SDK_CALL(put_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_BITDEPTH), 0);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_Option(OPTION_BITDEPTH, 0) -> %08x", result);
					PRIVATE_DATA->bits = 24;
					indigo_usleep(100000);
				}
				result = SDK_CALL(put_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_BINNING), PRIVATE_DATA->bin_mode | binning);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_Option(OPTION_BINNING, %x) -> %08x", PRIVATE_DATA->bin_mode | binning, result);
				//result = SDK_CALL(put_Speed)(PRIVATE_DATA->handle, 0);
				//INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_Speed(0) -> %08x", result);
				indigo_usleep(100000);
				result = SDK_CALL(StartPullModeWithCallback)(PRIVATE_DATA->handle, pull_callback, device);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "StartPullModeWithCallback() -> %08x", result);
				PRIVATE_DATA->mode = i;
			}
			break;
		}
	}
	if (PRIVATE_DATA->cam.model->flag & SDK_DEF(FLAG_ROI_HARDWARE)) {
		unsigned left = ROUND_BIN(CCD_FRAME_LEFT_ITEM->number.value, 1);
		unsigned top = ROUND_BIN(CCD_FRAME_TOP_ITEM->number.value, 1);
		unsigned width = ROUND_BIN(CCD_FRAME_WIDTH_ITEM->number.value, 1);
		if (width < 16)
			width = 16;
		unsigned height = ROUND_BIN(CCD_FRAME_HEIGHT_ITEM->number.value, 1);
		if (height < 16)
			height = 16;
		int max_width = CCD_INFO_WIDTH_ITEM->number.value;
		int max_height = CCD_INFO_HEIGHT_ITEM->number.value;
		if (left + width > max_width || top + height > max_height) {
			left = top = 0;
			width = max_width;
			height = max_height;
		}
		if (PRIVATE_DATA->left != left || PRIVATE_DATA->top != top || PRIVATE_DATA->width != width || PRIVATE_DATA->height != height) {
			result = SDK_CALL(put_Roi)(PRIVATE_DATA->handle, left, top, width, height);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_Roi(%d, %d, %d, %d) -> %08x", left, top, width, height, result);
			indigo_usleep(100000);
			PRIVATE_DATA->left = left;
			PRIVATE_DATA->top = top;
			PRIVATE_DATA->width = width;
			PRIVATE_DATA->height = height;
		}
	}
	result = SDK_CALL(put_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_FLUSH), 3);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_Option(OPTION_FLUSH) -> %08x", result);
}

static indigo_result ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result ccd_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_ccd_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		// --------------------------------------------------------------------------------
		unsigned long long flags = PRIVATE_DATA->cam.model->flag;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "flags = %0LX", flags);
		char name[128], label[128];
		INFO_PROPERTY->count = 8;
		indigo_copy_value(INFO_DEVICE_MODEL_ITEM->text.value, PRIVATE_DATA->cam.model->name);
		CCD_INFO_PIXEL_WIDTH_ITEM->number.value = PRIVATE_DATA->cam.model->xpixsz;
		CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = PRIVATE_DATA->cam.model->ypixsz;
		CCD_INFO_PIXEL_SIZE_ITEM->number.value = (CCD_INFO_PIXEL_WIDTH_ITEM->number.value + CCD_INFO_PIXEL_HEIGHT_ITEM->number.value) / 2.0;
		CCD_MODE_PROPERTY->perm = INDIGO_RW_PERM;
		CCD_MODE_PROPERTY->count = 0;
		CCD_INFO_WIDTH_ITEM->number.value = 0;
		CCD_INFO_HEIGHT_ITEM->number.value = 0;
		CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.min = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = 8;
		for (int i = 0; i < PRIVATE_DATA->cam.model->preview; i++) {
			int frame_width = PRIVATE_DATA->cam.model->res[i].width;
			int frame_height = PRIVATE_DATA->cam.model->res[i].height;
			if (frame_width > CCD_INFO_WIDTH_ITEM->number.value) {
				CCD_INFO_WIDTH_ITEM->number.value = frame_width;
			}
			if (frame_height > CCD_INFO_HEIGHT_ITEM->number.value) {
				CCD_INFO_HEIGHT_ITEM->number.value = frame_height;
			}
		}
		for (int bin = 1; bin <= 8; bin++) {
			int frame_width = ROUND_BIN(CCD_INFO_WIDTH_ITEM->number.value, bin);
			int frame_height = ROUND_BIN(CCD_INFO_HEIGHT_ITEM->number.value, bin);
			if ((flags & SDK_DEF(FLAG_MONO)) == 0) {
				if (flags & SDK_DEF(FLAG_RAW8)) {
					snprintf(name, sizeof(name), "RAW08_%d", bin);
					snprintf(label, sizeof(label), "RAW 8 %dx%d", frame_width, frame_height);
					indigo_init_switch_item(CCD_MODE_ITEM + CCD_MODE_PROPERTY->count, name, label, false);
					CCD_MODE_PROPERTY->count++;
				}
				if (flags & SDK_DEF(FLAG_RAW10)) {
					snprintf(name, sizeof(name), "RAW10_%d", bin);
					snprintf(label, sizeof(label), "RAW 10 %dx%d", frame_width, frame_height);
					indigo_init_switch_item(CCD_MODE_ITEM + CCD_MODE_PROPERTY->count, name, label, false);
					if (CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max < 10)
						CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = 10;
					CCD_MODE_PROPERTY->count++;
				}
				if (flags & SDK_DEF(FLAG_RAW12)) {
					snprintf(name, sizeof(name), "RAW12_%d", bin);
					snprintf(label, sizeof(label), "RAW 12 %dx%d", frame_width, frame_height);
					indigo_init_switch_item(CCD_MODE_ITEM + CCD_MODE_PROPERTY->count, name, label, false);
					if (CCD_INFO_BITS_PER_PIXEL_ITEM->number.value < 12)
						CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = 12;
					CCD_MODE_PROPERTY->count++;
				}
				if (flags & SDK_DEF(FLAG_RAW14)) {
					snprintf(name, sizeof(name), "RAW14_%d", bin);
					snprintf(label, sizeof(label), "RAW 14 %dx%d", frame_width, frame_height);
					indigo_init_switch_item(CCD_MODE_ITEM + CCD_MODE_PROPERTY->count, name, label, false);
					if (CCD_INFO_BITS_PER_PIXEL_ITEM->number.value < 14)
						CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = 14;
					CCD_MODE_PROPERTY->count++;
				}
				if (flags & SDK_DEF(FLAG_RAW16)) {
					snprintf(name, sizeof(name), "RAW16_%d", bin);
					snprintf(label, sizeof(label), "RAW 16 %dx%d", frame_width, frame_height);
					indigo_init_switch_item(CCD_MODE_ITEM + CCD_MODE_PROPERTY->count, name, label, false);
					if (CCD_INFO_BITS_PER_PIXEL_ITEM->number.value < 16)
						CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = 16;
					CCD_MODE_PROPERTY->count++;
				}
				snprintf(name, sizeof(name), "RGB08_%d", bin);
				snprintf(label, sizeof(label), "RGB 24 %dx%d", frame_width, frame_height);
				indigo_init_switch_item(CCD_MODE_ITEM + CCD_MODE_PROPERTY->count, name, label, false);
				CCD_MODE_PROPERTY->count++;
			} else {
				if (flags & SDK_DEF(FLAG_RAW8)) {
					snprintf(name, sizeof(name), "MON08_%d", bin);
					snprintf(label, sizeof(label), "MON 8 %dx%d", frame_width, frame_height);
					indigo_init_switch_item(CCD_MODE_ITEM + CCD_MODE_PROPERTY->count, name, label, false);
					CCD_MODE_PROPERTY->count++;
				}
				if (flags & SDK_DEF(FLAG_RAW10)) {
					snprintf(name, sizeof(name), "MON10_%d", bin);
					snprintf(label, sizeof(label), "MON 10 %dx%d", frame_width, frame_height);
					indigo_init_switch_item(CCD_MODE_ITEM + CCD_MODE_PROPERTY->count, name, label, false);
					if (CCD_INFO_BITS_PER_PIXEL_ITEM->number.value < 10)
						CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = 10;
					CCD_MODE_PROPERTY->count++;
				}
				if (flags & SDK_DEF(FLAG_RAW12)) {
					snprintf(name, sizeof(name), "MON12_%d", bin);
					snprintf(label, sizeof(label), "MON 12 %dx%d", frame_width, frame_height);
					indigo_init_switch_item(CCD_MODE_ITEM + CCD_MODE_PROPERTY->count, name, label, false);
					if (CCD_INFO_BITS_PER_PIXEL_ITEM->number.value < 12)
						CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = 12;
					CCD_MODE_PROPERTY->count++;
				}
				if (flags & SDK_DEF(FLAG_RAW14)) {
					snprintf(name, sizeof(name), "MON14_%d", bin);
					snprintf(label, sizeof(label), "MON 14 %dx%d", frame_width, frame_height);
					indigo_init_switch_item(CCD_MODE_ITEM + CCD_MODE_PROPERTY->count, name, label, false);
					if (CCD_INFO_BITS_PER_PIXEL_ITEM->number.value < 14)
						CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = 14;
					CCD_MODE_PROPERTY->count++;
				}
				if (flags & SDK_DEF(FLAG_RAW16)) {
					snprintf(name, sizeof(name), "MON16_%d", bin);
					snprintf(label, sizeof(label), "MON 16 %d x %d", frame_width, frame_height);
					indigo_init_switch_item(CCD_MODE_ITEM + CCD_MODE_PROPERTY->count, name, label, false);
					if (CCD_INFO_BITS_PER_PIXEL_ITEM->number.value < 16)
						CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = 16;
					CCD_MODE_PROPERTY->count++;
				}
			}
		}
		CCD_MODE_ITEM->sw.value = true;
		CCD_BIN_PROPERTY->perm = INDIGO_RW_PERM;
		CCD_BIN_HORIZONTAL_ITEM->number.min = CCD_BIN_VERTICAL_ITEM->number.min = 1;
		CCD_BIN_HORIZONTAL_ITEM->number.max = CCD_BIN_VERTICAL_ITEM->number.max = 8;
		CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max = CCD_INFO_WIDTH_ITEM->number.value;
		CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max = CCD_INFO_HEIGHT_ITEM->number.value;
		if ((flags & SDK_DEF(FLAG_ROI_HARDWARE)) == 0) {
			CCD_FRAME_PROPERTY->perm = INDIGO_RO_PERM;
		}
		if (PRIVATE_DATA->cam.model->flag & SDK_DEF(FLAG_GETTEMPERATURE)) {
			CCD_TEMPERATURE_PROPERTY->hidden = false;
			if (PRIVATE_DATA->cam.model->flag & SDK_DEF(FLAG_TEC_ONOFF)) {
				CCD_TEMPERATURE_PROPERTY->perm = INDIGO_RW_PERM;
				CCD_COOLER_PROPERTY->hidden = false;
				CCD_COOLER_POWER_PROPERTY->hidden = false;
				indigo_set_switch(CCD_COOLER_PROPERTY, CCD_COOLER_OFF_ITEM, true);
			} else {
				CCD_TEMPERATURE_PROPERTY->perm = INDIGO_RO_PERM;
			}
		}
		CCD_STREAMING_PROPERTY->hidden = ((flags & SDK_DEF(FLAG_TRIGGER_SINGLE)) != 0);
		CCD_IMAGE_FORMAT_PROPERTY->count = CCD_STREAMING_PROPERTY->hidden ? 5 : 6;
		CCD_GAIN_PROPERTY->hidden = false;

		X_CCD_ADVANCED_PROPERTY = indigo_init_number_property(NULL, device->name, "X_CCD_ADVANCED", CCD_ADVANCED_GROUP, "Advanced Settings", INDIGO_OK_STATE, INDIGO_RW_PERM, 9);
		if (X_CCD_ADVANCED_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(X_CCD_SPEED_ITEM, "SPEED", "Speed level", 0, PRIVATE_DATA->cam.model->maxspeed, 1, 0);
		indigo_init_number_item(X_CCD_CONTRAST_ITEM, "CONTRAST", "Contrast", SDK_DEF(CONTRAST_MIN), SDK_DEF(CONTRAST_MAX), 1, SDK_DEF(CONTRAST_DEF));
		indigo_init_number_item(X_CCD_HUE_ITEM, "HUE", "Hue", SDK_DEF(HUE_MIN), SDK_DEF(HUE_MAX), 1, SDK_DEF(HUE_DEF));
		indigo_init_number_item(X_CCD_SATURATION_ITEM, "SATURATION", "Saturation", SDK_DEF(SATURATION_MIN), SDK_DEF(SATURATION_MAX), 1, SDK_DEF(SATURATION_DEF));
		indigo_init_number_item(X_CCD_BRIGHTNESS_ITEM, "BRIGHTNESS", "Brightness", SDK_DEF(BRIGHTNESS_MIN), SDK_DEF(BRIGHTNESS_MAX), 1, SDK_DEF(BRIGHTNESS_DEF));
		indigo_init_number_item(X_CCD_GAMMA_ITEM, "GAMMA", "Gamma", SDK_DEF(GAMMA_MIN), SDK_DEF(GAMMA_MAX), 1, SDK_DEF(GAMMA_DEF));
		indigo_init_number_item(X_CCD_R_GAIN_ITEM, "R_GAIN", "Red gain", SDK_DEF(WBGAIN_MIN), SDK_DEF(WBGAIN_MAX), 1, SDK_DEF(WBGAIN_DEF));
		indigo_init_number_item(X_CCD_G_GAIN_ITEM, "G_GAIN", "Green gain", SDK_DEF(WBGAIN_MIN), SDK_DEF(WBGAIN_MAX), 1, SDK_DEF(WBGAIN_DEF));
		indigo_init_number_item(X_CCD_B_GAIN_ITEM, "B_GAIN", "Blue gain", SDK_DEF(WBGAIN_MIN), SDK_DEF(WBGAIN_MAX), 1, SDK_DEF(WBGAIN_DEF));
		if ((flags & SDK_DEF(FLAG_MONO))) {
			X_CCD_ADVANCED_PROPERTY->count = 1;  /* only SPEED is valid for mono cams */
		}

		if (flags & SDK_DEF(FLAG_FAN)) {
			X_CCD_FAN_PROPERTY = indigo_init_number_property(NULL, device->name, "X_CCD_FAN", CCD_ADVANCED_GROUP, "Fan control", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
			if (X_CCD_FAN_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_number_item(X_CCD_FAN_SPEED_ITEM, "FAN_SPEED", "Fan speed", 0, 0, 1, 0);
		}
		if (flags & SDK_DEF(FLAG_HEAT)) {
			X_CCD_HEATER_PROPERTY = indigo_init_number_property(NULL, device->name, "X_CCD_HEATER", CCD_ADVANCED_GROUP, "Window heater", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
			if (X_CCD_HEATER_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_number_item(X_CCD_HEATER_POWER_ITEM, "POWER", "Power", 0, 0, 1, 0);
		}
		if (flags & SDK_DEF(FLAG_CG) || flags & SDK_DEF(FLAG_CGHDR)) {
			X_CCD_CONVERSION_GAIN_PROPERTY = indigo_init_switch_property(NULL, device->name, "X_CCD_CONVERSION_GAIN", CCD_ADVANCED_GROUP, "Conversion gain", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
			if (X_CCD_CONVERSION_GAIN_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_switch_item(X_CCD_CONVERSION_GAIN_LCG_ITEM, "LCG", "Low conversion gain", true);
			indigo_init_switch_item(X_CCD_CONVERSION_GAIN_HCG_ITEM, "HCG", "High conversion gain", false);
			indigo_init_switch_item(X_CCD_CONVERSION_GAIN_HDR_ITEM, "HDR", "High dynamic range", false);
			if (flags & SDK_DEF(FLAG_CGHDR)) {
				X_CCD_CONVERSION_GAIN_PROPERTY->count = 3;
			} else if (flags & SDK_DEF(FLAG_CG)) {
				X_CCD_CONVERSION_GAIN_PROPERTY->count = 2;
			}
		}

		X_CCD_BIN_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, "X_CCD_BIN_MODE", CCD_ADVANCED_GROUP, "Binning mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
		if (X_CCD_BIN_MODE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(X_CCD_BIN_MODE_SATURATE_ITEM, "SATURATE", "Sum and saturate", true);
		indigo_init_switch_item(X_CCD_BIN_MODE_EXPAND_ITEM, "EXPAND", "Sum and expand to 16-bits (10, 12 and 14-bit data)", false);
		indigo_init_switch_item(X_CCD_BIN_MODE_AVERAGE_ITEM, "AVERAGE", "Average", false);

		X_CCD_LED_PROPERTY = indigo_init_switch_property(NULL, device->name, "X_CCD_LED", CCD_ADVANCED_GROUP, "Camera LED control", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_CCD_LED_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(X_CCD_LED_ON_ITEM, "ON", "On", true);
		indigo_init_switch_item(X_CCD_LED_OFF_ITEM, "OFF", "Off", false);
		X_CCD_LED_PROPERTY->hidden = true;

		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		// --------------------------------------------------------------------------------
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (X_CCD_ADVANCED_PROPERTY && indigo_property_match(X_CCD_ADVANCED_PROPERTY, property))
			indigo_define_property(device, X_CCD_ADVANCED_PROPERTY, NULL);
		if (X_CCD_FAN_PROPERTY && indigo_property_match(X_CCD_FAN_PROPERTY, property))
			indigo_define_property(device, X_CCD_FAN_PROPERTY, NULL);
		if (X_CCD_HEATER_PROPERTY && indigo_property_match(X_CCD_HEATER_PROPERTY, property))
			indigo_define_property(device, X_CCD_HEATER_PROPERTY, NULL);
		if (X_CCD_CONVERSION_GAIN_PROPERTY && indigo_property_match(X_CCD_CONVERSION_GAIN_PROPERTY, property))
			indigo_define_property(device, X_CCD_CONVERSION_GAIN_PROPERTY, NULL);
		if (X_CCD_BIN_MODE_PROPERTY && indigo_property_match(X_CCD_BIN_MODE_PROPERTY, property))
			indigo_define_property(device, X_CCD_BIN_MODE_PROPERTY, NULL);
		if (X_CCD_LED_PROPERTY && indigo_property_match(X_CCD_LED_PROPERTY, property))
			indigo_define_property(device, X_CCD_LED_PROPERTY, NULL);
	}
	return indigo_ccd_enumerate_properties(device, NULL, NULL);
}

static void ccd_connect_callback(indigo_device *device) {
	HRESULT result;
	indigo_lock_master_device(device);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (PRIVATE_DATA->handle == NULL) {
			if (indigo_try_global_lock(device) != INDIGO_OK) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_try_global_lock(): failed to get lock.");
			} else {
				char id[66];
				sprintf(id, "@%s", PRIVATE_DATA->cam.id);
				PRIVATE_DATA->handle = SDK_CALL(Open)(id);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Open(%s) -> %p", id, PRIVATE_DATA->handle);
			}
		}
		device->gp_bits = 1;
		if (PRIVATE_DATA->handle) {
			PRIVATE_DATA->buffer = (unsigned char *)indigo_alloc_blob_buffer(3 * CCD_INFO_WIDTH_ITEM->number.value * CCD_INFO_HEIGHT_ITEM->number.value + FITS_HEADER_SIZE);
			if (PRIVATE_DATA->cam.model->flag & SDK_DEF(FLAG_GETTEMPERATURE)) {
				if (CCD_TEMPERATURE_PROPERTY->perm == INDIGO_RW_PERM) {
					int value;
					result = SDK_CALL(get_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_TEC), &value);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "get_Option(OPTION_TEC, ->%d) -> %08x", value, result);
					indigo_set_switch(CCD_COOLER_PROPERTY, value ? CCD_COOLER_ON_ITEM : CCD_COOLER_OFF_ITEM, true);
					result = SDK_CALL(get_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_TECTARGET), &value);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "get_Option(OPTION_TECTARGET, ->%d) -> %08x", value, result);
					PRIVATE_DATA->current_temperature = CCD_TEMPERATURE_ITEM->number.target = value / 10.0;
				}
				indigo_set_timer(device, 5.0, ccd_temperature_callback, &PRIVATE_DATA->temperature_timer);
			}
			result = SDK_CALL(put_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_CALLBACK_THREAD), 1);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_Option(OPTION_CALLBACK_THREAD, 1) -> %08x", result);
			result = SDK_CALL(get_SerialNumber)(PRIVATE_DATA->handle, INFO_DEVICE_SERIAL_NUM_ITEM->text.value);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "get_SerialNumber() -> %08x", result);
			result = SDK_CALL(get_HwVersion)(PRIVATE_DATA->handle, INFO_DEVICE_HW_REVISION_ITEM->text.value);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "get_HwVersion() -> %08x", result);
			result = SDK_CALL(get_FwVersion)(PRIVATE_DATA->handle, INFO_DEVICE_FW_REVISION_ITEM->text.value);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "get_FwVersion() -> %08x", result);
			indigo_update_property(device, INFO_PROPERTY, NULL);
			get_bayer_pattern(device, PRIVATE_DATA->bayer_pattern);
			int bitDepth = 0;
			int binning = 1;
			char name[16];
			result = SDK_CALL(get_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_BITDEPTH), &bitDepth);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "get_Option(OPTION_BITDEPTH, ->%d) -> %08x", bitDepth, result);
			result = SDK_CALL(get_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_BINNING), &binning);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "get_Option(OPTION_BINNING, ->%d) -> %08x", binning, result);
			if (PRIVATE_DATA->cam.model->flag & SDK_DEF(FLAG_MONO)) {
				sprintf(name, "MON%02d_%d", bitDepth ? 16 : 8, binning);
				CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.target = bitDepth ? 16 : 8;
			} else {
				int rawMode = 0;
				result = SDK_CALL(get_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_RAW), &rawMode);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "get_Option(OPTION_RAW, ->%d) -> %08x", rawMode, result);
				if (rawMode) {
					sprintf(name, "RAW%02d_%d", bitDepth ? 16 : 8, binning);
					CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.target = bitDepth ? 16 : 8;
				} else {
					sprintf(name, "RGB08_%d", binning);
					CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.target = 8;
				}
			}
			for (int i = 0; i < CCD_MODE_PROPERTY->count; i++) {
				if (strcmp(name, CCD_MODE_PROPERTY->items[i].name) == 0) {
					indigo_set_switch(CCD_MODE_PROPERTY, CCD_MODE_PROPERTY->items + i, true);
				}
			}
			PRIVATE_DATA->mode = PRIVATE_DATA->left = PRIVATE_DATA->top = PRIVATE_DATA->width = PRIVATE_DATA->height = -1;
			result = SDK_CALL(get_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_BINNING), &binning);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "get_Option(OPTION_BINNING, ->%d) -> %08x", binning, result);
			CCD_BIN_HORIZONTAL_ITEM->number.value =
			CCD_BIN_HORIZONTAL_ITEM->number.target =
			CCD_BIN_VERTICAL_ITEM->number.value =
			CCD_BIN_VERTICAL_ITEM->number.target = binning;
			uint32_t min, max, current;
			SDK_CALL(get_ExpTimeRange)(PRIVATE_DATA->handle, &min, &max, &current);
			CCD_EXPOSURE_ITEM->number.min = CCD_STREAMING_EXPOSURE_ITEM->number.min = min / 1000000.0;
			CCD_EXPOSURE_ITEM->number.max = CCD_STREAMING_EXPOSURE_ITEM->number.max = max / 1000000.0;
			min = max = current = 0;
			result = SDK_CALL(put_AutoExpoEnable)(PRIVATE_DATA->handle, false);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_AutoExpoEnable(false) -> %08x", result);
			result = SDK_CALL(get_ExpoAGainRange)(PRIVATE_DATA->handle, (unsigned short *)&min, (unsigned short *)&max, (unsigned short *)&current);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "get_ExpoAGainRange(->%d, ->%d, ->%d) -> %08x", min, max, current, result);
			result = SDK_CALL(get_ExpoAGain)(PRIVATE_DATA->handle, (unsigned short *)&current);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "get_ExpoAGain(->%d) -> %08x", current, result);
			CCD_GAIN_ITEM->number.min = min;
			CCD_GAIN_ITEM->number.max = max;
			CCD_GAIN_ITEM->number.value = current;

			if (PRIVATE_DATA->cam.model->flag & SDK_DEF(FLAG_BLACKLEVEL)) {
				CCD_OFFSET_PROPERTY->hidden = false;
				CCD_OFFSET_ITEM->number.min = SDK_DEF(BLACKLEVEL_MIN);
				/* offset values are normaized to 256 but max is 248 see get_blacklevel() */
				CCD_OFFSET_ITEM->number.max = 248;
				int blacklevel = 0;
				double scale = 8;
				get_blacklevel(device, &blacklevel, &scale);
				CCD_OFFSET_ITEM->number.value = CCD_OFFSET_ITEM->number.target = blacklevel * scale;
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Offset supported: balcklevel=%d, scale=%f", blacklevel, scale);
			}

			if (X_CCD_ADVANCED_PROPERTY) {
				unsigned short current_speed = 1;
				result = SDK_CALL(get_Speed)(PRIVATE_DATA->handle, &current_speed);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "get_Speed(-> %d) -> %08x", current_speed, result);
				X_CCD_SPEED_ITEM->number.value = X_CCD_SPEED_ITEM->number.target = current_speed;
				indigo_define_property(device, X_CCD_ADVANCED_PROPERTY, NULL);
			}
			if (X_CCD_FAN_PROPERTY) {
				X_CCD_FAN_SPEED_ITEM->number.max = SDK_CALL(get_FanMaxSpeed)(PRIVATE_DATA->handle);
				int value = 0;
				SDK_CALL(get_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_FAN), &value);
				X_CCD_FAN_SPEED_ITEM->number.value = (double)value;
				indigo_define_property(device, X_CCD_FAN_PROPERTY, NULL);
			}
			if (X_CCD_HEATER_PROPERTY) {
				int value = 0;
				SDK_CALL(get_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_HEAT_MAX), &value);
				X_CCD_HEATER_POWER_ITEM->number.max = (double)value;
				SDK_CALL(get_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_HEAT), &value);
				X_CCD_HEATER_POWER_ITEM->number.value = (double)value;
				indigo_define_property(device, X_CCD_HEATER_PROPERTY, NULL);
			}
			if (X_CCD_CONVERSION_GAIN_PROPERTY) {
				int value = 0;
				result = SDK_CALL(get_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_CG), &value);
				if (result >= 0) {
					switch (value) {
					case 0:
						indigo_set_switch(X_CCD_CONVERSION_GAIN_PROPERTY, X_CCD_CONVERSION_GAIN_LCG_ITEM, true);
						break;
					case 1:
						indigo_set_switch(X_CCD_CONVERSION_GAIN_PROPERTY, X_CCD_CONVERSION_GAIN_HCG_ITEM, true);
						break;
					case 2:
						indigo_set_switch(X_CCD_CONVERSION_GAIN_PROPERTY, X_CCD_CONVERSION_GAIN_HDR_ITEM, true);
						break;
					}
				}
				indigo_define_property(device, X_CCD_CONVERSION_GAIN_PROPERTY, NULL);
			}

			int led_state = 0;
			result = SDK_CALL(get_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_TAILLIGHT), &led_state);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "get_Option(OPTION_TAILLIGHT, ->%d) -> %08x", led_state, result);
			if (FAILED(result)) {
				X_CCD_LED_PROPERTY->hidden = true;
			} else {
				X_CCD_LED_PROPERTY->hidden = false;
				indigo_set_switch(X_CCD_LED_PROPERTY, led_state ? X_CCD_LED_ON_ITEM : X_CCD_LED_OFF_ITEM, true);
				indigo_define_property(device, X_CCD_LED_PROPERTY, NULL);
			}

			result = SDK_CALL(put_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_TRIGGER), 1);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_Option(OPTION_TRIGGER, 1) -> %08x", result);

			/*
			This is a workaround for a problem with some cameras that fail to get exposure if
			after being plugged StartPullModeWithCallback() and Stop() are called without Trigger()
			in between. For the next calls this does not seem to cause a problem.
			*/
			if (PRIVATE_DATA->cam.model->flag & SDK_DEF(FLAG_USB30)) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "USB 3.0 Camera - exposure cludge executed");
				result = SDK_CALL(StartPullModeWithCallback)(PRIVATE_DATA->handle, pull_callback_dummy, device);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "StartPullModeWithCallback() -> %08x", result);
				result = SDK_CALL(Trigger)(PRIVATE_DATA->handle, 1);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Trigger(1) -> %08x", result);
				indigo_usleep(100000);
				result = SDK_CALL(Stop)(PRIVATE_DATA->handle);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Stop() -> %08x", result);
			}
			result = SDK_CALL(StartPullModeWithCallback)(PRIVATE_DATA->handle, pull_callback, device);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "StartPullModeWithCallback() -> %08x", result);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			device->gp_bits = 0;
		}
	} else {
		result = SDK_CALL(Stop)(PRIVATE_DATA->handle);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Stop() -> %08x", result);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->temperature_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->exposure_watchdog_timer);
		if (PRIVATE_DATA->buffer != NULL) {
			free(PRIVATE_DATA->buffer);
			PRIVATE_DATA->buffer = NULL;
		}
		if (X_CCD_ADVANCED_PROPERTY)
			indigo_delete_property(device, X_CCD_ADVANCED_PROPERTY, NULL);
		if (X_CCD_FAN_PROPERTY)
			indigo_delete_property(device, X_CCD_FAN_PROPERTY, NULL);
		if (X_CCD_HEATER_PROPERTY)
			indigo_delete_property(device, X_CCD_HEATER_PROPERTY, NULL);
		if (X_CCD_CONVERSION_GAIN_PROPERTY)
			indigo_delete_property(device, X_CCD_CONVERSION_GAIN_PROPERTY, NULL);
		if (X_CCD_BIN_MODE_PROPERTY)
			indigo_delete_property(device, X_CCD_BIN_MODE_PROPERTY, NULL);
		if (X_CCD_LED_PROPERTY)
			indigo_delete_property(device, X_CCD_LED_PROPERTY, NULL);
		if (PRIVATE_DATA->guider && PRIVATE_DATA->guider->gp_bits == 0) {
			if (PRIVATE_DATA->handle != NULL) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Closing camera");
				pthread_mutex_lock(&PRIVATE_DATA->mutex);
				SDK_CALL(Close)(PRIVATE_DATA->handle);
				pthread_mutex_unlock(&PRIVATE_DATA->mutex);
			}
			PRIVATE_DATA->handle = NULL;
			indigo_global_unlock(device);
		}
		device->gp_bits = 0;
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_ccd_change_property(device, NULL, CONNECTION_PROPERTY);
	indigo_unlock_master_device(device);
}

static indigo_result ccd_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	HRESULT result;
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION -> CCD_INFO, CCD_COOLER, CCD_TEMPERATURE
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, ccd_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_MODE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_MODE
		indigo_property_copy_values(CCD_MODE_PROPERTY, property, false);
		PRIVATE_DATA->mode = PRIVATE_DATA->left = PRIVATE_DATA->top = PRIVATE_DATA->width = PRIVATE_DATA->height = -1;
		CCD_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
		for (int i = 0; i < CCD_MODE_PROPERTY->count; i++) {
			indigo_item *item = &CCD_MODE_PROPERTY->items[i];
			if (item->sw.value) {
				char *underscore = strchr(item->name, '_');
				unsigned binning = atoi(underscore + 1);
				CCD_FRAME_BITS_PER_PIXEL_ITEM->number.target = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = atoi(item->name + 3);
				CCD_FRAME_PROPERTY->state = INDIGO_OK_STATE;
				CCD_BIN_HORIZONTAL_ITEM->number.target = CCD_BIN_HORIZONTAL_ITEM->number.value = binning;
				CCD_BIN_VERTICAL_ITEM->number.target = CCD_BIN_VERTICAL_ITEM->number.value = binning;
				CCD_BIN_PROPERTY->state = INDIGO_OK_STATE;
				CCD_MODE_PROPERTY->state = INDIGO_OK_STATE;
				break;
			}
		}
		indigo_update_property(device, CCD_FRAME_PROPERTY, NULL);
		indigo_update_property(device, CCD_BIN_PROPERTY, NULL);
		indigo_update_property(device, CCD_MODE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_BIN_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_BIN
		int prev_h_bin = (int)CCD_BIN_HORIZONTAL_ITEM->number.value;
		int prev_v_bin = (int)CCD_BIN_VERTICAL_ITEM->number.value;
		indigo_property_copy_values(CCD_BIN_PROPERTY, property, false);
		PRIVATE_DATA->mode = PRIVATE_DATA->left = PRIVATE_DATA->top = PRIVATE_DATA->width = PRIVATE_DATA->height = -1;
		CCD_BIN_PROPERTY->state = INDIGO_OK_STATE;
		int horizontal_bin = (int)CCD_BIN_HORIZONTAL_ITEM->number.value;
		int vertical_bin = (int)CCD_BIN_VERTICAL_ITEM->number.value;
		/* Touptek (& family) cameras work with binx = biny for we force it here */
		if (prev_h_bin != horizontal_bin) {
			vertical_bin =
			CCD_BIN_HORIZONTAL_ITEM->number.target =
			CCD_BIN_HORIZONTAL_ITEM->number.value =
			CCD_BIN_VERTICAL_ITEM->number.target =
			CCD_BIN_VERTICAL_ITEM->number.value = horizontal_bin;
		} else if (prev_v_bin != vertical_bin) {
			horizontal_bin =
			CCD_BIN_HORIZONTAL_ITEM->number.target =
			CCD_BIN_HORIZONTAL_ITEM->number.value =
			CCD_BIN_VERTICAL_ITEM->number.target =
			CCD_BIN_VERTICAL_ITEM->number.value = vertical_bin;
		}
		char *selected_name = CCD_MODE_PROPERTY->items[0].name;
		for (int k = 0; k < CCD_MODE_PROPERTY->count; k++) {
			indigo_item *item = &CCD_MODE_PROPERTY->items[k];
			if (item->sw.value) {
				selected_name = item->name;
				break;
			}
		}
		for (int k = 0; k < CCD_MODE_PROPERTY->count; k++) {
			indigo_item *item = &CCD_MODE_PROPERTY->items[k];
			char *underscore = strchr(item->name, '_');
			unsigned bin = atoi(underscore + 1);
			if (bin == horizontal_bin && !strncmp(item->name, selected_name, 5)) {
				indigo_set_switch(CCD_MODE_PROPERTY, item, true);
				CCD_MODE_PROPERTY->state = INDIGO_OK_STATE;
				CCD_BIN_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, CCD_MODE_PROPERTY, NULL);
				indigo_update_property(device, CCD_BIN_PROPERTY, NULL);
				return INDIGO_OK;
			}
		}
		CCD_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
		CCD_BIN_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, CCD_MODE_PROPERTY, NULL);
		indigo_update_property(device, CCD_BIN_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_FRAME_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_FRAME
		indigo_property_copy_values(CCD_FRAME_PROPERTY, property, false);
		char name[INDIGO_NAME_SIZE];
		PRIVATE_DATA->mode = PRIVATE_DATA->left = PRIVATE_DATA->top = PRIVATE_DATA->width = PRIVATE_DATA->height = -1;
		for (int j = 0; j < CCD_MODE_PROPERTY->count; j++) {
			indigo_item *item = &CCD_MODE_PROPERTY->items[j];
			if (item->sw.value) {
				strcpy(name, item->name);
				sprintf(name + 3, "%02d", (int)(CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value));
				name[5] = '_';
				for (int k = 0; k < CCD_MODE_PROPERTY->count; k++) {
					item = &CCD_MODE_PROPERTY->items[k];
					if (!strcmp(name, item->name)) {
						indigo_set_switch(CCD_MODE_PROPERTY, item, true);
						CCD_MODE_PROPERTY->state = INDIGO_OK_STATE;
						indigo_update_property(device, CCD_MODE_PROPERTY, NULL);
						return indigo_ccd_change_property(device, client, property);
					}
				}
			}
		}
		CCD_FRAME_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, CCD_FRAME_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_EXPOSURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_EXPOSURE
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE)
			return INDIGO_OK;
		indigo_property_copy_values(CCD_EXPOSURE_PROPERTY, property, false);
		indigo_use_shortest_exposure_if_bias(device);
		if (CCD_UPLOAD_MODE_LOCAL_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
			CCD_IMAGE_FILE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
		}
		if (CCD_UPLOAD_MODE_CLIENT_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
			CCD_IMAGE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
		}
		CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		pthread_mutex_lock(&PRIVATE_DATA->mutex);
		setup_exposure(device);

		result = SDK_CALL(put_ExpoTime)(PRIVATE_DATA->handle, (unsigned)(CCD_EXPOSURE_ITEM->number.target * 1000000));
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_ExpoTime(%u) -> %08x", (unsigned)(CCD_EXPOSURE_ITEM->number.target * 1000000), result);
		PRIVATE_DATA->aborting = false;
		result = SDK_CALL(Trigger)(PRIVATE_DATA->handle, 1);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Trigger(1) -> %08x", result);
		double whatchdog_timeout = (CCD_EXPOSURE_ITEM->number.target > 50) ? 1.5 * CCD_EXPOSURE_ITEM->number.target : CCD_EXPOSURE_ITEM->number.target + 25;
		indigo_set_timer(device, whatchdog_timeout, exposure_watchdog_callback, &PRIVATE_DATA->exposure_watchdog_timer);
		pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	} else if (indigo_property_match_changeable(CCD_STREAMING_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_STREAMING
		if (CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE)
			return INDIGO_OK;
		indigo_property_copy_values(CCD_STREAMING_PROPERTY, property, false);
		indigo_use_shortest_exposure_if_bias(device);
		if (CCD_UPLOAD_MODE_LOCAL_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
			CCD_IMAGE_FILE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
		}
		if (CCD_UPLOAD_MODE_CLIENT_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
			CCD_IMAGE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
		}
		CCD_STREAMING_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
		pthread_mutex_lock(&PRIVATE_DATA->mutex);
		setup_exposure(device);
		result = SDK_CALL(put_ExpoTime)(PRIVATE_DATA->handle, (unsigned)(CCD_STREAMING_EXPOSURE_ITEM->number.target * 1000000));
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_ExpoTime(%u) -> %08x", (unsigned)(CCD_STREAMING_EXPOSURE_ITEM->number.target * 1000000), result);
		PRIVATE_DATA->aborting = false;
		result = SDK_CALL(Trigger)(PRIVATE_DATA->handle, (int)CCD_STREAMING_COUNT_ITEM->number.value);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Trigger(%d) -> %08x", (int)CCD_STREAMING_COUNT_ITEM->number.value);
		pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	} else if (indigo_property_match_changeable(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_ABORT_EXPOSURE
		indigo_property_copy_values(CCD_ABORT_EXPOSURE_PROPERTY, property, false);
		if (CCD_ABORT_EXPOSURE_ITEM->sw.value) {
			CCD_ABORT_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_ABORT_EXPOSURE_PROPERTY, NULL);
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->exposure_watchdog_timer);
			PRIVATE_DATA->aborting = true;
			pthread_mutex_lock(&PRIVATE_DATA->mutex);
			result = SDK_CALL(Trigger)(PRIVATE_DATA->handle, 0);
			pthread_mutex_unlock(&PRIVATE_DATA->mutex);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Trigger(0) -> %08x", result);
		}
	} else if (indigo_property_match_changeable(CCD_COOLER_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_COOLER
		indigo_property_copy_values(CCD_COOLER_PROPERTY, property, false);
		result = SDK_CALL(put_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_TEC), CCD_COOLER_ON_ITEM->sw.value ? 1 : 0);
		if (result >= 0) {
			CCD_COOLER_PROPERTY->state = INDIGO_OK_STATE;
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_Option(OPTION_TEC) -> %08x", result);
		} else {
			CCD_COOLER_PROPERTY->state = INDIGO_ALERT_STATE;
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "put_Option(OPTION_TEC) -> %08x", result);
		}
		indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_TEMPERATURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_TEMPERATURE
		indigo_property_copy_values(CCD_TEMPERATURE_PROPERTY, property, false);
		result = SDK_CALL(put_Temperature)(PRIVATE_DATA->handle, (short)(CCD_TEMPERATURE_ITEM->number.target * 10));
		if (result >= 0) {
			CCD_TEMPERATURE_PROPERTY->state = INDIGO_BUSY_STATE;
			CCD_TEMPERATURE_ITEM->number.value = PRIVATE_DATA->current_temperature;
			if (!CCD_COOLER_PROPERTY->hidden && CCD_COOLER_OFF_ITEM->sw.value) {
				result = SDK_CALL(put_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_TEC), 1);
				if (result >= 0) {
					indigo_set_switch(CCD_COOLER_PROPERTY, CCD_COOLER_ON_ITEM, true);
					CCD_COOLER_PROPERTY->state = INDIGO_OK_STATE;
				} else {
					CCD_COOLER_PROPERTY->state = INDIGO_ALERT_STATE;
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "put_Option(OPTION_TEC, 1) -> %08x", result);
				}
				indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
			}
		} else {
			CCD_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "put_Temperature() -> %08x", result);
		}
		indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_GAIN_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_GAIN
		indigo_property_copy_values(CCD_GAIN_PROPERTY, property, false);
		result = SDK_CALL(put_ExpoAGain)(PRIVATE_DATA->handle, (int)CCD_GAIN_ITEM->number.value);
		if (result < 0) {
			CCD_GAIN_PROPERTY->state = INDIGO_ALERT_STATE;
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "put_ExpoAGain(%d) -> %08x", (int)CCD_GAIN_ITEM->number.value, result);
			indigo_update_property(device, CCD_GAIN_PROPERTY, "Analog gain setting is not supported");
		} else {
			CCD_GAIN_PROPERTY->state = INDIGO_OK_STATE;
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_ExpoAGain(%d) -> %08x", (int)CCD_GAIN_ITEM->number.value, result);
			indigo_update_property(device, CCD_GAIN_PROPERTY, NULL);
		}
	} else if (indigo_property_match_changeable(CCD_OFFSET_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_OFFSET
		indigo_property_copy_values(CCD_OFFSET_PROPERTY, property, false);
		handle_offset(device);
		return INDIGO_OK;
	} else if (X_CCD_ADVANCED_PROPERTY && indigo_property_match_defined(X_CCD_ADVANCED_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_CCD_ADVANCED
		indigo_property_copy_values(X_CCD_ADVANCED_PROPERTY, property, false);
		X_CCD_ADVANCED_PROPERTY->state = INDIGO_OK_STATE;
		if (X_CCD_ADVANCED_PROPERTY->count != 1) {
			result = SDK_CALL(put_Contrast)(PRIVATE_DATA->handle, (int)X_CCD_CONTRAST_ITEM->number.value);
			if (result < 0) {
				X_CCD_ADVANCED_PROPERTY->state = INDIGO_ALERT_STATE;
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "put_Contrast(%d) -> %08x", (int)X_CCD_CONTRAST_ITEM->number.value, result);
			} else {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_Contrast(%d) -> %08x", (int)X_CCD_CONTRAST_ITEM->number.value, result);
			}
			result = SDK_CALL(put_Hue)(PRIVATE_DATA->handle, (int)X_CCD_HUE_ITEM->number.value);
			if (result < 0) {
				X_CCD_ADVANCED_PROPERTY->state = INDIGO_ALERT_STATE;
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "put_Hue(%d) -> %08x", (int)X_CCD_HUE_ITEM->number.value, result);
			} else {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_Hue(%d) -> %08x", (int)X_CCD_HUE_ITEM->number.value, result);
			}
			result = SDK_CALL(put_Saturation)(PRIVATE_DATA->handle, (int)X_CCD_SATURATION_ITEM->number.value);
			if (result < 0) {
				X_CCD_ADVANCED_PROPERTY->state = INDIGO_ALERT_STATE;
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "put_Saturation(%d) -> %08x", (int)X_CCD_SATURATION_ITEM->number.value, result);
			} else {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_Saturation(%d) -> %08x", (int)X_CCD_SATURATION_ITEM->number.value, result);
			}
			result = SDK_CALL(put_Brightness)(PRIVATE_DATA->handle, (int)X_CCD_BRIGHTNESS_ITEM->number.value);
			if (result < 0) {
				X_CCD_ADVANCED_PROPERTY->state = INDIGO_ALERT_STATE;
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "put_Brightness(%d) -> %08x", (int)X_CCD_BRIGHTNESS_ITEM->number.value, result);
			} else {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_Brightness(%d) -> %08x", (int)X_CCD_BRIGHTNESS_ITEM->number.value, result);
			}
			result = SDK_CALL(put_Gamma)(PRIVATE_DATA->handle, (int)X_CCD_GAMMA_ITEM->number.value);
			if (result < 0) {
				X_CCD_ADVANCED_PROPERTY->state = INDIGO_ALERT_STATE;
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "put_Gamma(%d) -> %08x", (int)X_CCD_GAMMA_ITEM->number.value, result);
			} else {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_Gamma(%d) -> %08x", (int)X_CCD_GAMMA_ITEM->number.value, result);
			}
			int gain[3] = { (int)X_CCD_R_GAIN_ITEM->number.value, (int)X_CCD_G_GAIN_ITEM->number.value, (int)X_CCD_B_GAIN_ITEM->number.value };
			result = SDK_CALL(put_WhiteBalanceGain)(PRIVATE_DATA->handle, gain);
			if (result < 0) {
				X_CCD_ADVANCED_PROPERTY->state = INDIGO_ALERT_STATE;
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "put_WhiteBalanceGain(%d, %d, %d) -> %08x", gain[0], gain[1], gain[2], result);
			} else {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_WhiteBalanceGain(%d, %d, %d) -> %08x", gain[0], gain[1], gain[2], result);
			}
		}
		result = SDK_CALL(put_Speed)(PRIVATE_DATA->handle, (int)X_CCD_SPEED_ITEM->number.value);
		if (result < 0) {
			X_CCD_ADVANCED_PROPERTY->state = INDIGO_ALERT_STATE;
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "put_Speed(%d) -> %08x", (int)X_CCD_SPEED_ITEM->number.value, result);
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_Speed(%d) -> %08x", (int)X_CCD_SPEED_ITEM->number.value, result);
		}
		indigo_update_property(device, X_CCD_ADVANCED_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (X_CCD_FAN_PROPERTY && indigo_property_match_defined(X_CCD_FAN_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_CCD_FAN
		indigo_property_copy_values(X_CCD_FAN_PROPERTY, property, false);
		X_CCD_FAN_PROPERTY->state = INDIGO_OK_STATE;
		result = SDK_CALL(put_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_FAN), (int)X_CCD_FAN_SPEED_ITEM->number.value);
		if (result < 0) {
			X_CCD_FAN_PROPERTY->state = INDIGO_ALERT_STATE;
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "put_Option(OPTION_FAN, %d) -> %08x", (int)X_CCD_FAN_SPEED_ITEM->number.value, result);
			indigo_update_property(device, X_CCD_FAN_PROPERTY, "Fan speed setting is not supported");
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_Option(OPTION_FAN, %d) -> %08x", (int)X_CCD_FAN_SPEED_ITEM->number.value, result);
			indigo_update_property(device, X_CCD_FAN_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (X_CCD_HEATER_PROPERTY && indigo_property_match_defined(X_CCD_HEATER_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_CCD_HEATER
		indigo_property_copy_values(X_CCD_HEATER_PROPERTY, property, false);
		X_CCD_HEATER_PROPERTY->state = INDIGO_OK_STATE;
		result = SDK_CALL(put_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_HEAT), (int)X_CCD_HEATER_POWER_ITEM->number.value);
		if (result < 0) {
			X_CCD_HEATER_PROPERTY->state = INDIGO_ALERT_STATE;
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "put_Option(OPTION_HEATER, %d) -> %08x", (int)X_CCD_HEATER_POWER_ITEM->number.value, result);
			indigo_update_property(device, X_CCD_HEATER_PROPERTY, "Window heater is not supported");
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_Option(OPTION_HEATER, %d) -> %08x", (int)X_CCD_HEATER_POWER_ITEM->number.value, result);
			indigo_update_property(device, X_CCD_HEATER_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (X_CCD_CONVERSION_GAIN_PROPERTY && indigo_property_match_defined(X_CCD_CONVERSION_GAIN_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_CCD_CONVERSION_GAIN
		indigo_property_copy_values(X_CCD_CONVERSION_GAIN_PROPERTY, property, false);
		X_CCD_CONVERSION_GAIN_PROPERTY->state = INDIGO_OK_STATE;
		int value = 0;
		if (X_CCD_CONVERSION_GAIN_LCG_ITEM->sw.value) {
			value = 0;
		} else if (X_CCD_CONVERSION_GAIN_HCG_ITEM->sw.value) {
			value = 1;
		} else if (X_CCD_CONVERSION_GAIN_HDR_ITEM->sw.value) {
			value = 2;
		}
		result = SDK_CALL(put_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_CG), value);
		if (result < 0) {
			X_CCD_CONVERSION_GAIN_PROPERTY->state = INDIGO_ALERT_STATE;
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "put_Option(OPTION_CG, %d) -> %08x", value, result);
			indigo_update_property(device, X_CCD_CONVERSION_GAIN_PROPERTY, "Requested conversion gain is not supported");
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_Option(OPTION_CG, %d) -> %08x", value, result);
			indigo_update_property(device, X_CCD_CONVERSION_GAIN_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_defined(X_CCD_LED_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_CCD_LED
		indigo_property_copy_values(X_CCD_LED_PROPERTY, property, false);
		X_CCD_LED_PROPERTY->state = INDIGO_OK_STATE;
		result = SDK_CALL(put_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_TAILLIGHT), X_CCD_LED_ON_ITEM->sw.value);
		if (result < 0) {
			X_CCD_LED_PROPERTY->state = INDIGO_ALERT_STATE;
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "put_Option(OPTION_TAILLIGHT, %d) -> %08x", X_CCD_LED_ON_ITEM->sw.value, result);
			indigo_update_property(device, X_CCD_LED_PROPERTY, "LED light setting failed");
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_Option(OPTION_TAILLIGHT, %d) -> %08x", X_CCD_LED_ON_ITEM->sw.value, result);
			indigo_update_property(device, X_CCD_LED_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_defined(X_CCD_BIN_MODE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_CCD_BIN_MODE
		indigo_property_copy_values(X_CCD_BIN_MODE_PROPERTY, property, false);
		PRIVATE_DATA->mode = -1;
		if (X_CCD_BIN_MODE_SATURATE_ITEM->sw.value) {
			PRIVATE_DATA->bin_mode = 0x00;
		} else if (X_CCD_BIN_MODE_EXPAND_ITEM->sw.value) {
			PRIVATE_DATA->bin_mode = 0x40;
		} else if (X_CCD_BIN_MODE_AVERAGE_ITEM->sw.value) {
			PRIVATE_DATA->bin_mode = 0x80;
		}
		X_CCD_BIN_MODE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, X_CCD_BIN_MODE_PROPERTY, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- CONFIG
	} else if (indigo_property_match_changeable(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, X_CCD_ADVANCED_PROPERTY);
			indigo_save_property(device, NULL, X_CCD_CONVERSION_GAIN_PROPERTY);
			indigo_save_property(device, NULL, X_CCD_BIN_MODE_PROPERTY);
			indigo_save_property(device, NULL, X_CCD_LED_PROPERTY);
		}
		// --------------------------------------------------------------------------------
	}
	return indigo_ccd_change_property(device, client, property);
}

static indigo_result ccd_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		ccd_connect_callback(device);
	}
	if (X_CCD_ADVANCED_PROPERTY) {
		indigo_release_property(X_CCD_ADVANCED_PROPERTY);
	}
	if (X_CCD_FAN_PROPERTY) {
		indigo_release_property(X_CCD_FAN_PROPERTY);
	}
	if (X_CCD_HEATER_PROPERTY) {
		indigo_release_property(X_CCD_HEATER_PROPERTY);
	}
	if (X_CCD_CONVERSION_GAIN_PROPERTY) {
		indigo_release_property(X_CCD_CONVERSION_GAIN_PROPERTY);
	}
	if (X_CCD_BIN_MODE_PROPERTY) {
		indigo_release_property(X_CCD_BIN_MODE_PROPERTY);
	}
	if (X_CCD_LED_PROPERTY) {
		indigo_release_property(X_CCD_LED_PROPERTY);
	}
	if (device == device->master_device) {
		indigo_global_unlock(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_ccd_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO guider device implementation

static indigo_result guider_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_guider_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		INFO_PROPERTY->count = 8;
		indigo_copy_value(INFO_DEVICE_MODEL_ITEM->text.value, PRIVATE_DATA->cam.model->name);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void guider_connect_callback(indigo_device *device) {
	indigo_lock_master_device(device);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (PRIVATE_DATA->handle == NULL) {
			if (indigo_try_global_lock(device) != INDIGO_OK) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_try_global_lock(): failed to get lock.");
			} else {
				char id[66];
				sprintf(id, "@%s", PRIVATE_DATA->cam.id);
				PRIVATE_DATA->handle = SDK_CALL(Open)(id);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Open(%s) -> %p", id, PRIVATE_DATA->handle);
			}
		}
		device->gp_bits = 1;
		if (PRIVATE_DATA->handle) {
			HRESULT result = SDK_CALL(put_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_CALLBACK_THREAD), 1);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Tuopcam_put_Option(OPTION_CALLBACK_THREAD, 1) -> %08x", result);
			result = SDK_CALL(get_SerialNumber)(PRIVATE_DATA->handle, INFO_DEVICE_SERIAL_NUM_ITEM->text.value);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "get_SerialNumber() -> %08x", result);
			result = SDK_CALL(get_HwVersion)(PRIVATE_DATA->handle, INFO_DEVICE_HW_REVISION_ITEM->text.value);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "get_HwVersion() -> %08x", result);
			result = SDK_CALL(get_FwVersion)(PRIVATE_DATA->handle, INFO_DEVICE_FW_REVISION_ITEM->text.value);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "get_FwVersion() -> %08x", result);
			indigo_update_property(device, INFO_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			device->gp_bits = 0;
		}
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->guider_timer_ra);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->guider_timer_dec);
		if (PRIVATE_DATA->camera && PRIVATE_DATA->camera->gp_bits == 0) {
			if (PRIVATE_DATA->handle != NULL) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Closing camera");
				pthread_mutex_lock(&PRIVATE_DATA->mutex);
				SDK_CALL(Close)(PRIVATE_DATA->handle);
				pthread_mutex_unlock(&PRIVATE_DATA->mutex);
				indigo_global_unlock(device);
			}
			PRIVATE_DATA->handle = NULL;
		}
		device->gp_bits = 0;
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_guider_change_property(device, NULL, CONNECTION_PROPERTY);
	indigo_unlock_master_device(device);
}

static void guider_timer_callback_ra(indigo_device *device) {
	if (!CONNECTION_CONNECTED_ITEM->sw.value) {
		return;
	}
	GUIDER_GUIDE_EAST_ITEM->number.value = 0;
	GUIDER_GUIDE_WEST_ITEM->number.value = 0;
	GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
}

static void guider_timer_callback_dec(indigo_device *device) {
	if (!CONNECTION_CONNECTED_ITEM->sw.value) {
		return;
	}
	GUIDER_GUIDE_NORTH_ITEM->number.value = 0;
	GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
	GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
}

static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, guider_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_DEC
		indigo_property_copy_values(GUIDER_GUIDE_DEC_PROPERTY, property, false);
		HRESULT result = 0;
		int pulse_length = 0;
		indigo_cancel_timer(device, &PRIVATE_DATA->guider_timer_dec);
		if (GUIDER_GUIDE_NORTH_ITEM->number.value > 0) {
			pulse_length = (int)GUIDER_GUIDE_NORTH_ITEM->number.value;
			result = SDK_CALL(ST4PlusGuide)(PRIVATE_DATA->handle, 0, pulse_length);
		} else if (GUIDER_GUIDE_SOUTH_ITEM->number.value > 0) {
			pulse_length = (int)GUIDER_GUIDE_SOUTH_ITEM->number.value;
			result = SDK_CALL(ST4PlusGuide)(PRIVATE_DATA->handle, 1, pulse_length);
		}
		GUIDER_GUIDE_DEC_PROPERTY->state = SUCCEEDED(result) ? INDIGO_BUSY_STATE : INDIGO_ALERT_STATE;
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		if (GUIDER_GUIDE_DEC_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_set_timer(device, pulse_length / 1000.0, guider_timer_callback_dec, &PRIVATE_DATA->guider_timer_dec);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_RA
		indigo_property_copy_values(GUIDER_GUIDE_RA_PROPERTY, property, false);
		HRESULT result = 0;
		int pulse_length = 0;
		indigo_cancel_timer(device, &PRIVATE_DATA->guider_timer_ra);
		if (GUIDER_GUIDE_EAST_ITEM->number.value > 0) {
			pulse_length = (int)GUIDER_GUIDE_EAST_ITEM->number.value;
			result = SDK_CALL(ST4PlusGuide)(PRIVATE_DATA->handle, 2, pulse_length);
		} else if (GUIDER_GUIDE_WEST_ITEM->number.value > 0) {
			pulse_length = (int)GUIDER_GUIDE_WEST_ITEM->number.value;
			result = SDK_CALL(ST4PlusGuide)(PRIVATE_DATA->handle, 3, pulse_length);
		}
		GUIDER_GUIDE_RA_PROPERTY->state = SUCCEEDED(result) ? INDIGO_BUSY_STATE : INDIGO_ALERT_STATE;
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
		if (GUIDER_GUIDE_RA_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_set_timer(device, pulse_length / 1000.0, guider_timer_callback_ra, &PRIVATE_DATA->guider_timer_ra);
		}
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
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_guider_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO Wheel device implementation
static void set_wheel_positions(indigo_device *device) {
	int positions = 7;
	if (X_WHEEL_MODEL_5_POSITION_ITEM->sw.value) {
		positions = 5;
	} else if (X_WHEEL_MODEL_7_POSITION_ITEM->sw.value) {
		positions = 7;
	} else if (X_WHEEL_MODEL_8_POSITION_ITEM->sw.value) {
		positions = 8;
	}
	HRESULT result = SDK_CALL(put_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_FILTERWHEEL_SLOT), positions);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_Option(OPTION_FILTERWHEEL_SLOT) -> %08x", result);
	positions = 7;
	result = SDK_CALL(get_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_FILTERWHEEL_SLOT), &positions);
	WHEEL_SLOT_ITEM->number.max =
	WHEEL_SLOT_NAME_PROPERTY->count =
	WHEEL_SLOT_OFFSET_PROPERTY->count =
	PRIVATE_DATA->count = positions;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "get_Option(OPTION_FILTERWHEEL_SLOT) -> %08x, %d", result, positions);
}

static void wheel_timer_callback(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	HRESULT result = SDK_CALL(get_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_FILTERWHEEL_POSITION), &PRIVATE_DATA->current_slot);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "get_Option(OPTION_FILTERWHEEL_POSITION) -> %08x, %d", result, PRIVATE_DATA->current_slot);
	PRIVATE_DATA->current_slot++;
	WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->current_slot;
	if (PRIVATE_DATA->current_slot == PRIVATE_DATA->target_slot) {
		WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
	} else if (PRIVATE_DATA->current_slot == 0) { //still moving
		indigo_reschedule_timer(device, 0.5, &(PRIVATE_DATA->wheel_timer));
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Set filter %d failed", (int)WHEEL_SLOT_ITEM->number.target);
		WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
}

static void calibrate_callback(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	HRESULT result = SDK_CALL(put_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_FILTERWHEEL_POSITION), -1); // -1 means callibrate
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	if (SUCCEEDED(result)) {
		int pos = 0;
		do {
			indigo_usleep(ONE_SECOND_DELAY);
			pthread_mutex_lock(&PRIVATE_DATA->mutex);
			HRESULT result = SDK_CALL(get_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_FILTERWHEEL_POSITION), &pos);
			pthread_mutex_unlock(&PRIVATE_DATA->mutex);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "get_Option(OPTION_FILTERWHEEL_POSITION) -> %08x, %d", result, pos);
		} while (pos == -1);
		WHEEL_SLOT_ITEM->number.value =
		WHEEL_SLOT_ITEM->number.target =
		PRIVATE_DATA->current_slot =
		PRIVATE_DATA->target_slot = ++pos;
		WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
		X_CALIBRATE_START_ITEM->sw.value=false;
		X_CALIBRATE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, X_CALIBRATE_PROPERTY, "Calibration finished");
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "put_Option(OPTION_FILTERWHEEL_POSITION, -1) -> %08x", result);
		WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
		X_CALIBRATE_START_ITEM->sw.value=false;
		X_CALIBRATE_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_CALIBRATE_PROPERTY, "Calibration failed");
	}
}

static indigo_result wheel_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result wheel_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);

	if (indigo_wheel_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		INFO_PROPERTY->count = 7;
		// --------------------------------------------------------------------------------- X_CALIBRATE
		X_CALIBRATE_PROPERTY = indigo_init_switch_property(NULL, device->name, X_CALIBRATE_PROPERTY_NAME, ADVANCED_GROUP, "Calibrate filter wheel", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
		if (X_CALIBRATE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(X_CALIBRATE_START_ITEM, X_CALIBRATE_START_ITEM_NAME, "Start", false);
		// --------------------------------------------------------------------------------- X_WHEEL_MODEL
		X_WHEEL_MODEL_PROPERTY = indigo_init_switch_property(NULL, device->name, X_WHEEL_MODEL_PROPERTY_NAME, MAIN_GROUP, "Device Model", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
		if (X_WHEEL_MODEL_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(X_WHEEL_MODEL_5_POSITION_ITEM, X_WHEEL_MODEL_5_POSITION_ITEM_NAME, "5 positions Filter wheel", false);
		indigo_init_switch_item(X_WHEEL_MODEL_7_POSITION_ITEM, X_WHEEL_MODEL_7_POSITION_ITEM_NAME, "7 positions Filter wheel", true);
		indigo_init_switch_item(X_WHEEL_MODEL_8_POSITION_ITEM, X_WHEEL_MODEL_8_POSITION_ITEM_NAME, "8 positions Filter wheel", false);
		// --------------------------------------------------------------------------
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return wheel_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result wheel_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	indigo_define_matching_property(X_WHEEL_MODEL_PROPERTY);
	if (IS_CONNECTED) {
		indigo_define_matching_property(X_CALIBRATE_PROPERTY);
	}
	return indigo_wheel_enumerate_properties(device, client, property);
}

static void wheel_connect_callback(indigo_device *device) {
	indigo_lock_master_device(device);
	CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (PRIVATE_DATA->handle == NULL) {
			if (indigo_try_global_lock(device) != INDIGO_OK) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_try_global_lock(): failed to get lock.");
			} else {
				char id[66];
				sprintf(id, "@%s", PRIVATE_DATA->cam.id);
				PRIVATE_DATA->handle = SDK_CALL(Open)(id);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Open(%s) -> %p", id, PRIVATE_DATA->handle);
			}
		}
		device->gp_bits = 1;
		if (PRIVATE_DATA->handle) {
			HRESULT result = SDK_CALL(get_HwVersion)(PRIVATE_DATA->handle, INFO_DEVICE_HW_REVISION_ITEM->text.value);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "get_HwVersion() -> %08x", result);
			result = SDK_CALL(get_FwVersion)(PRIVATE_DATA->handle, INFO_DEVICE_FW_REVISION_ITEM->text.value);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "get_FwVersion() -> %08x", result);
			indigo_update_property(device, INFO_PROPERTY, NULL);
			indigo_define_property(device, X_CALIBRATE_PROPERTY, NULL);

			set_wheel_positions(device);

			/* This is a hack! We need to reset to some position because sometimes after reconnect
			   the the state remains "moving" forever although it is not moving. However it tries
			   to set slot 1 at every connect, so this hack does not change anything.
			*/
			int slot = 0 + (1 << 8);  // slot 1 using closest approach
			pthread_mutex_lock(&PRIVATE_DATA->mutex);
			SDK_CALL(put_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_FILTERWHEEL_POSITION), slot);
			pthread_mutex_unlock(&PRIVATE_DATA->mutex);
			int value = 0;
			do {
				indigo_usleep(ONE_SECOND_DELAY);
				result = SDK_CALL(get_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_FILTERWHEEL_POSITION), &value);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "get_Option(OPTION_FILTERWHEEL_POSITION) -> %08x, %d", result, value + 1);
			} while (value == -1);
			WHEEL_SLOT_ITEM->number.value = WHEEL_SLOT_ITEM->number.target = ++value;
			indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);

			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			device->gp_bits = 0;
		}
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->wheel_timer);
		indigo_delete_property(device, X_CALIBRATE_PROPERTY, NULL);
		if (PRIVATE_DATA->camera && PRIVATE_DATA->camera->gp_bits != 0) {
			if (PRIVATE_DATA->handle != NULL) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Closing wheel");
				pthread_mutex_lock(&PRIVATE_DATA->mutex);
				SDK_CALL(Close)(PRIVATE_DATA->handle);
				pthread_mutex_unlock(&PRIVATE_DATA->mutex);
				indigo_global_unlock(device);
			}
			PRIVATE_DATA->handle = NULL;
		}
		device->gp_bits = 0;
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_wheel_change_property(device, NULL, CONNECTION_PROPERTY);
	indigo_unlock_master_device(device);
}

static indigo_result wheel_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, wheel_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(WHEEL_SLOT_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- WHEEL_SLOT
		indigo_property_copy_values(WHEEL_SLOT_PROPERTY, property, false);
		if (WHEEL_SLOT_ITEM->number.value < 1 || WHEEL_SLOT_ITEM->number.value > WHEEL_SLOT_ITEM->number.max) {
			WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
		} else if (WHEEL_SLOT_ITEM->number.value == PRIVATE_DATA->current_slot) {
			WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			WHEEL_SLOT_PROPERTY->state = INDIGO_BUSY_STATE;
			PRIVATE_DATA->target_slot = WHEEL_SLOT_ITEM->number.value;
			WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->current_slot;
			int slot = ((int)WHEEL_SLOT_ITEM->number.target-1) + (1<< 8);
			pthread_mutex_lock(&PRIVATE_DATA->mutex);
			HRESULT result = SDK_CALL(put_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_FILTERWHEEL_POSITION), slot);
			pthread_mutex_unlock(&PRIVATE_DATA->mutex);
			if (FAILED(result)) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "put_Option(OPTION_FILTERWHEEL_POSITION, %d) -> %08x", slot, result);
				SDK_CALL(get_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_FILTERWHEEL_POSITION), &PRIVATE_DATA->current_slot);
				WHEEL_SLOT_ITEM->number.value = ++PRIVATE_DATA->current_slot;
				WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
			} else {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_Option(OPTION_FILTERWHEEL_POSITION, %d) -> %08x", slot, result);
				indigo_set_timer(device, 0.5, wheel_timer_callback, &PRIVATE_DATA->wheel_timer);
			}
		}
		indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_CALIBRATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_CALIBRATE
		indigo_property_copy_values(X_CALIBRATE_PROPERTY, property, false);
		if (X_CALIBRATE_START_ITEM->sw.value) {
			X_CALIBRATE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, X_CALIBRATE_PROPERTY, "Calibration started");
			WHEEL_SLOT_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
			indigo_set_timer(device, 0.5, calibrate_callback, &PRIVATE_DATA->wheel_timer);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_WHEEL_MODEL_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_WHEEL_MODEL
		indigo_property_copy_values(X_WHEEL_MODEL_PROPERTY, property, false);

		set_wheel_positions(device);

		indigo_delete_property(device, WHEEL_SLOT_PROPERTY, NULL);
		indigo_delete_property(device, WHEEL_SLOT_NAME_PROPERTY, NULL);
		indigo_delete_property(device, WHEEL_SLOT_OFFSET_PROPERTY, NULL);
		indigo_define_property(device, WHEEL_SLOT_PROPERTY, NULL);
		indigo_define_property(device, WHEEL_SLOT_NAME_PROPERTY, NULL);
		indigo_define_property(device, WHEEL_SLOT_OFFSET_PROPERTY, NULL);

		indigo_update_property(device, X_WHEEL_MODEL_PROPERTY, NULL);
	} else if (indigo_property_match_changeable(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG_PROPERTY
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, X_WHEEL_MODEL_PROPERTY);
		}
	}
	// --------------------------------------------------------------------------------
	return indigo_wheel_change_property(device, client, property);
}

static indigo_result wheel_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		wheel_connect_callback(device);
	}
	indigo_release_property(X_CALIBRATE_PROPERTY);
	indigo_release_property(X_WHEEL_MODEL_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_wheel_detach(device);
}


// -------------------------------------------------------------------------------- INDIGO focuser device implementation
static void focuser_timer_callback(indigo_device *device) {
	int is_moving = 0;
	HRESULT res;

	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	res = (SDK_CALL(AAF)(PRIVATE_DATA->handle, SDK_DEF(AAF_ISMOVING), 0, &is_moving));
	if (FAILED(res)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "AAF(AAF_ISMOVING) -> %08x (value = %d) (failed)", res, is_moving);
		FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "AAF(AAF_ISMOVING) -> %08x (value = %d)", res, is_moving);
	}

	res = (SDK_CALL(AAF)(PRIVATE_DATA->handle, SDK_DEF(AAF_GETPOSITION), 0, &PRIVATE_DATA->current_position));
	if (FAILED(res)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "AAF(AAF_GETPOSITION) -> %08x (value = %d) (failed)", res, PRIVATE_DATA->current_position);
		FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "AAF(AAF_GETPOSITION) -> %08x (value = %d)", res, PRIVATE_DATA->current_position);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);

	FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;

	if ((!is_moving) || (PRIVATE_DATA->current_position == PRIVATE_DATA->target_position)) {
		FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
		FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		indigo_reschedule_timer(device, 0.5, &(PRIVATE_DATA->focuser_timer));
	}
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
}


static void compensate_focus(indigo_device *device, double new_temp) {
	int compensation;
	double temp_difference = new_temp - PRIVATE_DATA->prev_temp;

	// we do not have previous temperature reading 
	if (PRIVATE_DATA->prev_temp < -270) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Not compensating: PRIVATE_DATA->prev_temp = %f", PRIVATE_DATA->prev_temp);
		PRIVATE_DATA->prev_temp = new_temp;
		return;
	}

	// we do not have current temperature reading or focuser is moving
	if ((new_temp < -270) || (FOCUSER_POSITION_PROPERTY->state != INDIGO_OK_STATE)) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Not compensating: new_temp = %f, FOCUSER_POSITION_PROPERTY->state = %d", new_temp, FOCUSER_POSITION_PROPERTY->state);
		return;
	}

	// temperature difference if more than 1 degree so compensation needed
	if ((fabs(temp_difference) >= FOCUSER_COMPENSATION_THRESHOLD_ITEM->number.value) && (fabs(temp_difference) < 100)) {
		compensation = (int)(temp_difference * FOCUSER_COMPENSATION_ITEM->number.value);
		INDIGO_DRIVER_DEBUG(
			DRIVER_NAME,
			"Compensation: temp_difference = %.2f, Compensation = %d, steps/degC = %.0f, threshold = %.2f",
			temp_difference,
			compensation,
			FOCUSER_COMPENSATION_ITEM->number.value,
			FOCUSER_COMPENSATION_THRESHOLD_ITEM->number.value
		);
	} else {
		INDIGO_DRIVER_DEBUG(
			DRIVER_NAME,
			"Not compensating (not needed): temp_difference = %.2f, threshold = %.2f",
			temp_difference,
			FOCUSER_COMPENSATION_THRESHOLD_ITEM->number.value
		);
		return;
	}

	PRIVATE_DATA->target_position = PRIVATE_DATA->current_position + compensation;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Compensation: PRIVATE_DATA->current_position = %d, PRIVATE_DATA->target_position = %d", PRIVATE_DATA->current_position, PRIVATE_DATA->target_position);

	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	HRESULT res = (SDK_CALL(AAF)(PRIVATE_DATA->handle, SDK_DEF(AAF_GETPOSITION), 0, &PRIVATE_DATA->current_position));
	if (FAILED(res)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "AAF(AAF_GETPOSITION) -> %08x (value = %d) (failed)", res, PRIVATE_DATA->current_position);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);

	// Make sure we do not attempt to go beyond the limits
	if (FOCUSER_POSITION_ITEM->number.max < PRIVATE_DATA->target_position) {
		PRIVATE_DATA->target_position = FOCUSER_POSITION_ITEM->number.max;
	} else if (FOCUSER_POSITION_ITEM->number.min > PRIVATE_DATA->target_position) {
		PRIVATE_DATA->target_position = FOCUSER_POSITION_ITEM->number.min;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Compensating: Corrected PRIVATE_DATA->target_position = %d", PRIVATE_DATA->target_position);

	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	res = (SDK_CALL(AAF)(PRIVATE_DATA->handle, SDK_DEF(AAF_SETPOSITION), PRIVATE_DATA->target_position, NULL));
	if (FAILED(res)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "AAF(AAF_SETPOSITION) -> %08x (value = %d) (failed)", res, PRIVATE_DATA->target_position);
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);

	PRIVATE_DATA->prev_temp = new_temp;
	FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
	FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	indigo_set_timer(device, 0.5, focuser_timer_callback, &PRIVATE_DATA->focuser_timer);
}


static void temperature_timer_callback(indigo_device *device) {
	int temp10 = -2732;
	static bool has_sensor = true;
	HRESULT res;

	FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;

	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	res = (SDK_CALL(AAF)(PRIVATE_DATA->handle, SDK_DEF(AAF_GETAMBIENTTEMP), 0, &temp10));
	if (FAILED(res)) {
		if (has_sensor) {
			INDIGO_DRIVER_LOG(DRIVER_NAME, "The temperature sensor is not connected (using internal sensor).");
			indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, "The temperature sensor is not connected (using internal sensor).");
		}
		has_sensor = false;
	} else {
		if (!has_sensor) {
			INDIGO_DRIVER_LOG(DRIVER_NAME, "The temperature sensor connected.");
			indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, "The temperature sensor connected.");
		}
		has_sensor = true;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "AAF(AAF_GETAMBIENTTEMP) -> %08x (value = %d)", res, temp10);

	if (!has_sensor) {
		res = (SDK_CALL(AAF)(PRIVATE_DATA->handle, SDK_DEF(AAF_GETTEMP), 0, &temp10));
		if (FAILED(res)) {
			temp10 = -2732;
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "AAF(AAF_GETTEMP) -> %08x (value = %d) (failed)", res, temp10);
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "AAF(AAF_GETTEMP) -> %08x (value = %d)", res, temp10);
		}
	}

	FOCUSER_TEMPERATURE_ITEM->number.value = (double)temp10/10.0;
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);

	if (FOCUSER_TEMPERATURE_ITEM->number.value < -270.0) {
		FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_IDLE_STATE;
	}
	indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);

	if (FOCUSER_MODE_AUTOMATIC_ITEM->sw.value) {
		compensate_focus(device, FOCUSER_TEMPERATURE_ITEM->number.value);
	} else {
		// reset temp so that the compensation starts when auto mode is selected
		PRIVATE_DATA->prev_temp = -273;
	}
	indigo_reschedule_timer(device, 2, &(PRIVATE_DATA->temperature_timer));
}


static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		indigo_define_matching_property(X_BEEP_PROPERTY);
	}
	return indigo_focuser_enumerate_properties(device, NULL, NULL);
}


static indigo_result focuser_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);

		FOCUSER_LIMITS_PROPERTY->hidden = false;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.min = 0;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.max = 65000;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.min = 0;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.step = 100;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value = 0;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.max = 0;
		//INDIGO_DRIVER_DEBUG(DRIVER_NAME, "\'%s\' MaxStep = %d",device->name ,PRIVATE_DATA->info.MaxStep);

		FOCUSER_SPEED_PROPERTY->hidden = true;

		FOCUSER_BACKLASH_PROPERTY->hidden = false;
		FOCUSER_BACKLASH_ITEM->number.min = 0;
		FOCUSER_BACKLASH_ITEM->number.max = 10000;
		FOCUSER_BACKLASH_ITEM->number.step = 1;

		// TESTED: focuser does not go beyond 65000
		FOCUSER_POSITION_ITEM->number.min = 0;
		FOCUSER_POSITION_ITEM->number.step = 1;
		FOCUSER_POSITION_ITEM->number.max = 65000;

		FOCUSER_STEPS_ITEM->number.min = 0;
		FOCUSER_STEPS_ITEM->number.step = 1;
		FOCUSER_STEPS_ITEM->number.max = 65000;

		FOCUSER_ON_POSITION_SET_PROPERTY->hidden = false;
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = false;

		// -------------------------------------------------------------------------- FOCUSER_COMPENSATION
		FOCUSER_COMPENSATION_PROPERTY->hidden = false;
		FOCUSER_COMPENSATION_ITEM->number.min = -10000;
		FOCUSER_COMPENSATION_ITEM->number.max = 10000;
		FOCUSER_COMPENSATION_PROPERTY->count = 2;
		// -------------------------------------------------------------------------- FOCUSER_MODE
		FOCUSER_MODE_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------- BEEP_PROPERTY
		X_BEEP_PROPERTY = indigo_init_switch_property(NULL, device->name, X_BEEP_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "Buzzer", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_BEEP_PROPERTY == NULL)
			return INDIGO_FAILED;

		indigo_init_switch_item(X_BEEP_ON_ITEM, X_BEEP_ON_ITEM_NAME, "On", false);
		indigo_init_switch_item(X_BEEP_OFF_ITEM, X_BEEP_OFF_ITEM_NAME, "Off", true);
		// --------------------------------------------------------------------------
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void focuser_connect_callback(indigo_device *device) {
indigo_lock_master_device(device);
	CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (PRIVATE_DATA->handle == NULL) {
			if (indigo_try_global_lock(device) != INDIGO_OK) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_try_global_lock(): failed to get lock.");
			} else {
				char id[66];
				sprintf(id, "@%s", PRIVATE_DATA->cam.id);
				PRIVATE_DATA->handle = SDK_CALL(Open)(id);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Open(%s) -> %p", id, PRIVATE_DATA->handle);
			}
		}
		device->gp_bits = 1;
		if (PRIVATE_DATA->handle) {
			HRESULT result = SDK_CALL(get_HwVersion)(PRIVATE_DATA->handle, INFO_DEVICE_HW_REVISION_ITEM->text.value);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "get_HwVersion() -> %08x", result);
			result = SDK_CALL(get_FwVersion)(PRIVATE_DATA->handle, INFO_DEVICE_FW_REVISION_ITEM->text.value);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "get_FwVersion() -> %08x", result);
			indigo_update_property(device, INFO_PROPERTY, NULL);
			indigo_define_property(device, X_CALIBRATE_PROPERTY, NULL);

			pthread_mutex_lock(&PRIVATE_DATA->mutex);
			int value = 0;
			HRESULT res = (SDK_CALL(AAF)(PRIVATE_DATA->handle, SDK_DEF(AAF_RANGEMAX), 0, &value));

			res = (SDK_CALL(AAF)(PRIVATE_DATA->handle, SDK_DEF(AAF_GETBACKLASH), 0, &value));
			if (FAILED(res)) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "AAF(AAF_GETBACKLASH) -> %08x (value = %d) (failed)", res, value);
			} else {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "AAF(AAF_GETBACKLASH) -> %08x (value = %d)", res, value);
				FOCUSER_BACKLASH_ITEM->number.value = (double)value;
				PRIVATE_DATA->backlash = value;
			}

			res = (SDK_CALL(AAF)(PRIVATE_DATA->handle, SDK_DEF(AAF_GETPOSITION), 0, &value));
			if (FAILED(res)) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "AAF(AAF_GETPOSITION) -> %08x (value = %d) (failed)", res, value);
			} else {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "AAF(AAF_GETPOSITION) -> %08x (value = %d)", res, value);
				FOCUSER_POSITION_ITEM->number.value = (double)value;
				PRIVATE_DATA->current_position = PRIVATE_DATA->target_position = value;
			}

			res = (SDK_CALL(AAF)(PRIVATE_DATA->handle, SDK_DEF(AAF_GETDIRECTION), 0, &value));
			if (FAILED(res)) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "AAF(AAF_GETDIRECTION) -> %08x (value = %d) (failed)", res, value);
			} else {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "AAF(AAF_GETDIRECTION) -> %08x (value = %d)", res, value);
				FOCUSER_REVERSE_MOTION_DISABLED_ITEM->sw.value = (value > 0);
				FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value = !FOCUSER_REVERSE_MOTION_DISABLED_ITEM->sw.value;
			}

			res = (SDK_CALL(AAF)(PRIVATE_DATA->handle, SDK_DEF(AAF_GETMAXSTEP), 0, &PRIVATE_DATA->max_position));
			if (FAILED(res)) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "AAF(AAF_GETMAXSTEP) -> %08x (value = %d) (failed)", res, PRIVATE_DATA->max_position);
			} else {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "AAF(AAF_GETMAXSTEP) -> %08x (value = %d)", res, PRIVATE_DATA->max_position);
				FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target = (double)PRIVATE_DATA->max_position;
			}

			res = (SDK_CALL(AAF)(PRIVATE_DATA->handle, SDK_DEF(AAF_GETBUZZER), 0, &value));
			if (FAILED(res)) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "AAF(AAF_GETBUZZER) -> %08x (value = %d) (failed)", res, value);
			} else {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "AAF(AAF_GETBUZZER) -> %08x (value = %d)", res, value);
				X_BEEP_ON_ITEM->sw.value = (value > 0);
			}
			X_BEEP_OFF_ITEM->sw.value = !X_BEEP_ON_ITEM->sw.value;

			pthread_mutex_unlock(&PRIVATE_DATA->mutex);

			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;

			indigo_define_property(device, X_BEEP_PROPERTY, NULL);

			PRIVATE_DATA->prev_temp = -273;  /* we do not have previous temperature reading */ 
			indigo_set_timer(device, 0.5, focuser_timer_callback, &PRIVATE_DATA->focuser_timer);
			indigo_set_timer(device, 0.1, temperature_timer_callback, &PRIVATE_DATA->temperature_timer);
		} else {
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			device->gp_bits = 0;
		}
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->focuser_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->temperature_timer);
		indigo_delete_property(device, X_BEEP_PROPERTY, NULL);
		if (PRIVATE_DATA->camera && PRIVATE_DATA->camera->gp_bits != 0) {
			if (PRIVATE_DATA->handle != NULL) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Closing focuser");
				pthread_mutex_lock(&PRIVATE_DATA->mutex);
				SDK_CALL(Close)(PRIVATE_DATA->handle);
				pthread_mutex_unlock(&PRIVATE_DATA->mutex);
				indigo_global_unlock(device);
			}
			PRIVATE_DATA->handle = NULL;
		}
		device->gp_bits = 0;
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
	indigo_unlock_master_device(device);
}

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	HRESULT res;
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_REVERSE_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_REVERSE_MOTION
		indigo_property_copy_values(FOCUSER_REVERSE_MOTION_PROPERTY, property, false);
		FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		pthread_mutex_lock(&PRIVATE_DATA->mutex);

		int value = FOCUSER_REVERSE_MOTION_DISABLED_ITEM->sw.value ? 1 : 0;
		res = (SDK_CALL(AAF)(PRIVATE_DATA->handle, SDK_DEF(AAF_SETDIRECTION), value, NULL));
		if (FAILED(res)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "AAF(AAF_SETDIRECTION) -> %08x (value = %d) (failed)", res, value);
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "AAF(AAF_SETDIRECTION) -> %08x (value = %d)", res, value);
			FOCUSER_REVERSE_MOTION_DISABLED_ITEM->sw.value = (value > 0);
			FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value = !FOCUSER_REVERSE_MOTION_DISABLED_ITEM->sw.value;
		}

		pthread_mutex_unlock(&PRIVATE_DATA->mutex);
		indigo_update_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_POSITION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		indigo_property_copy_values(FOCUSER_POSITION_PROPERTY, property, false);
		if (FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE) {
			return INDIGO_OK;
		}
		if (FOCUSER_POSITION_ITEM->number.target < 0 || FOCUSER_POSITION_ITEM->number.target > FOCUSER_POSITION_ITEM->number.max) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		} else if (FOCUSER_POSITION_ITEM->number.target == PRIVATE_DATA->current_position) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		} else {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			PRIVATE_DATA->target_position = FOCUSER_POSITION_ITEM->number.target;
			FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);

			if (FOCUSER_ON_POSITION_SET_GOTO_ITEM->sw.value) { /* GOTO POSITION */
				pthread_mutex_lock(&PRIVATE_DATA->mutex);
				res = (SDK_CALL(AAF)(PRIVATE_DATA->handle, SDK_DEF(AAF_SETPOSITION), PRIVATE_DATA->target_position, NULL));
				if (FAILED(res)) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "AAF(AAF_SETPOSITION) -> %08x (value = %d) (failed)", res, PRIVATE_DATA->target_position);
				} else {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "AAF(AAF_SETPOSITION) -> %08x (value = %d)", res, PRIVATE_DATA->target_position);
				}
				pthread_mutex_unlock(&PRIVATE_DATA->mutex);
				indigo_set_timer(device, 0.5, focuser_timer_callback, &PRIVATE_DATA->focuser_timer);
			} else { /* SYNC POSITION */
				FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
				FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
				pthread_mutex_lock(&PRIVATE_DATA->mutex);
				res = (SDK_CALL(AAF)(PRIVATE_DATA->handle, SDK_DEF(AAF_SETZERO), PRIVATE_DATA->target_position, NULL));
				if (FAILED(res)) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "AAF(AAF_SETZERO) -> %08x (value = %d) (failed)", res, PRIVATE_DATA->target_position);
				} else {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "AAF(AAF_SETZERO) -> %08x (value = %d)", res, PRIVATE_DATA->target_position);
				}

				res = (SDK_CALL(AAF)(PRIVATE_DATA->handle, SDK_DEF(AAF_GETPOSITION), 0, &PRIVATE_DATA->current_position));
				if (FAILED(res)) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "AAF(AAF_GETPOSITION) -> %08x (value = %d) (failed)", res, PRIVATE_DATA->current_position);
					FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
					FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
				} else {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "AAF(AAF_GETPOSITION) -> %08x (value = %d)", res, PRIVATE_DATA->current_position);
					FOCUSER_POSITION_ITEM->number.value = (double)PRIVATE_DATA->current_position;
					FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
					FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
				}
				pthread_mutex_unlock(&PRIVATE_DATA->mutex);
				indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
				indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			}
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_LIMITS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_LIMITS
		indigo_property_copy_values(FOCUSER_LIMITS_PROPERTY, property, false);
		FOCUSER_LIMITS_PROPERTY->state = INDIGO_OK_STATE;
		int max_position = PRIVATE_DATA->max_position;
		PRIVATE_DATA->max_position = (int)FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target;

		pthread_mutex_lock(&PRIVATE_DATA->mutex);
		res = (SDK_CALL(AAF)(PRIVATE_DATA->handle, SDK_DEF(AAF_SETMAXSTEP), PRIVATE_DATA->max_position, NULL));
		if (FAILED(res)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "AAF(AAF_SETMAXSTEP) -> %08x (value = %d) (failed)", res, PRIVATE_DATA->max_position);
			FOCUSER_LIMITS_PROPERTY->state = INDIGO_ALERT_STATE;
			PRIVATE_DATA->max_position = max_position;
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "AAF(AAF_SETMAXSTEP) -> %08x (value = %d)", res, PRIVATE_DATA->max_position);
		}
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = (double)PRIVATE_DATA->max_position;
		pthread_mutex_unlock(&PRIVATE_DATA->mutex);

		indigo_update_property(device, FOCUSER_LIMITS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_BACKLASH_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_BACKLASH
		indigo_property_copy_values(FOCUSER_BACKLASH_PROPERTY, property, false);
		FOCUSER_BACKLASH_PROPERTY->state = INDIGO_OK_STATE;
		pthread_mutex_lock(&PRIVATE_DATA->mutex);
		int backlash = PRIVATE_DATA->backlash;
		PRIVATE_DATA->backlash = (int)FOCUSER_BACKLASH_ITEM->number.target;

		res = (SDK_CALL(AAF)(PRIVATE_DATA->handle, SDK_DEF(AAF_SETBACKLASH), PRIVATE_DATA->backlash, NULL));
		if (FAILED(res)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "AAF(AAF_SETBACKLASH) -> %08x (value = %d) (failed)", res, PRIVATE_DATA->backlash);
			FOCUSER_BACKLASH_PROPERTY->state = INDIGO_ALERT_STATE;
			PRIVATE_DATA->backlash = backlash;
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "AAF(AAF_SETBACKLASH) -> %08x (value = %d)", res, PRIVATE_DATA->backlash);
		}

		FOCUSER_BACKLASH_ITEM->number.value = (double)PRIVATE_DATA->backlash;
		pthread_mutex_unlock(&PRIVATE_DATA->mutex);
		indigo_update_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		indigo_property_copy_values(FOCUSER_STEPS_PROPERTY, property, false);
		if (FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE) {
			return INDIGO_OK;
		}
		if (FOCUSER_STEPS_ITEM->number.value < 0 || FOCUSER_STEPS_ITEM->number.value > FOCUSER_STEPS_ITEM->number.max) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		} else {
			FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			pthread_mutex_lock(&PRIVATE_DATA->mutex);

			res = (SDK_CALL(AAF)(PRIVATE_DATA->handle, SDK_DEF(AAF_GETPOSITION), 0, &PRIVATE_DATA->current_position));
			if (FAILED(res)) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "AAF(AAF_GETPOSITION) -> %08x (value = %d) (failed)", res, PRIVATE_DATA->current_position);
			} else {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "AAF(AAF_GETPOSITION) -> %08x (value = %d)", res, PRIVATE_DATA->current_position);
				FOCUSER_POSITION_ITEM->number.value = (double)PRIVATE_DATA->current_position;
			}

			if (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value) {
				PRIVATE_DATA->target_position = PRIVATE_DATA->current_position - FOCUSER_STEPS_ITEM->number.value;
			} else {
				PRIVATE_DATA->target_position = PRIVATE_DATA->current_position + FOCUSER_STEPS_ITEM->number.value;
			}

			/* Make sure we do not attempt to go beyond the limits */
			if (FOCUSER_POSITION_ITEM->number.max < PRIVATE_DATA->target_position) {
				PRIVATE_DATA->target_position = FOCUSER_POSITION_ITEM->number.max;
			} else if (FOCUSER_POSITION_ITEM->number.min > PRIVATE_DATA->target_position) {
				PRIVATE_DATA->target_position = FOCUSER_POSITION_ITEM->number.min;
			}

			FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;

			res = (SDK_CALL(AAF)(PRIVATE_DATA->handle, SDK_DEF(AAF_SETPOSITION), PRIVATE_DATA->target_position, NULL));
			if (FAILED(res)) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "AAF(AAF_SETPOSITION) -> %08x (value = %d) (failed)", res, PRIVATE_DATA->target_position);
				FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			} else {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "AAF(AAF_SETPOSITION) -> %08x (value = %d)", res, PRIVATE_DATA->target_position);
			}

			pthread_mutex_unlock(&PRIVATE_DATA->mutex);
			indigo_set_timer(device, 0.5, focuser_timer_callback, &PRIVATE_DATA->focuser_timer);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_ABORT_MOTION
		indigo_property_copy_values(FOCUSER_ABORT_MOTION_PROPERTY, property, false);
		FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
		FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_cancel_timer(device, &PRIVATE_DATA->focuser_timer);

		pthread_mutex_lock(&PRIVATE_DATA->mutex);
		res = (SDK_CALL(AAF)(PRIVATE_DATA->handle, SDK_DEF(AAF_HALT), 1, NULL));
		if (FAILED(res)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "AAF(AAF_HALT) -> %08x (value = %d) (failed)", res, 1);
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "AAF(AAF_HALT) -> %08x (value = %d)", res, 1);
		}
		res = (SDK_CALL(AAF)(PRIVATE_DATA->handle, SDK_DEF(AAF_GETPOSITION), 0, &PRIVATE_DATA->current_position));
		if (FAILED(res)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "AAF(AAF_GETPOSITION) -> %08x (value = %d) (failed)", res, PRIVATE_DATA->current_position);
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "AAF(AAF_GETPOSITION) -> %08x (value = %d)", res, PRIVATE_DATA->current_position);
		}
		pthread_mutex_unlock(&PRIVATE_DATA->mutex);

		FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
		FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_COMPENSATION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_COMPENSATION_PROPERTY
		indigo_property_copy_values(FOCUSER_COMPENSATION_PROPERTY, property, false);
		FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_COMPENSATION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_BEEP_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- EAF_BEEP_PROPERTY
		indigo_property_copy_values(X_BEEP_PROPERTY, property, false);
		X_BEEP_PROPERTY->state = INDIGO_OK_STATE;
		pthread_mutex_lock(&PRIVATE_DATA->mutex);
		res = (SDK_CALL(AAF)(PRIVATE_DATA->handle, SDK_DEF(AAF_SETBUZZER), X_BEEP_ON_ITEM->sw.value, NULL));
		if (FAILED(res)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "AAF(AAF_SETBUZZER) -> %08x (value = %d) (failed)", res, X_BEEP_ON_ITEM->sw.value);
			X_BEEP_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "AAF(AAF_SETBUZZER) -> %08x (value = %d)", res, X_BEEP_ON_ITEM->sw.value);
		}
		pthread_mutex_unlock(&PRIVATE_DATA->mutex);
		indigo_update_property(device, X_BEEP_PROPERTY, NULL);
		return INDIGO_OK;
		// ------------------------------------------------------------------------------- FOCUSER_MODE
	} else if (indigo_property_match_changeable(FOCUSER_MODE_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_MODE_PROPERTY, property, false);
		if (FOCUSER_MODE_MANUAL_ITEM->sw.value) {
			indigo_define_property(device, FOCUSER_ON_POSITION_SET_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_SPEED_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_DIRECTION_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			FOCUSER_POSITION_PROPERTY->perm = INDIGO_RW_PERM;
			indigo_define_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		} else {
			indigo_delete_property(device, FOCUSER_ON_POSITION_SET_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_SPEED_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_DIRECTION_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			FOCUSER_POSITION_PROPERTY->perm = INDIGO_RO_PERM;
			indigo_define_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		}
		FOCUSER_MODE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_MODE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			//indigo_save_property(device, NULL, EAF_BEEP_PROPERTY);
		}
		// --------------------------------------------------------------------------------
	}
	return indigo_focuser_change_property(device, client, property);
}


static indigo_result focuser_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_connect_callback(device);
	}
	indigo_release_property(X_BEEP_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}


// -------------------------------------------------------------------------------- hot-plug support

static indigo_device *devices[SDK_DEF(MAX)];
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

#ifdef TOUPTEK

#define TOUPTEK_VID	0x0547

struct oem_2_toupcam {
	int oem_vid;
	int oem_pid;
	int toupcam_pid;
	char *name;
} oem_2_toupcam[] = {
	{ 0x547, 0xe077, 0x11ea, "Meade DSI IV Color" }, // USB3.0 + DDR
	{ 0x547, 0xe078, 0x11eb, "Meade DSI IV Color" }, // USB2.0 + DDR
	{ 0x547, 0xe079, 0x11f6, "Meade DSI IV Mono" }, // USB3.0 + DDR
	{ 0x547, 0xe07a, 0x11f7, "Meade DSI IV Mono" }, // USB2.0 + DDR

	{ 0x547, 0xe06b, 0x106b, "Meade DSI IV Color" }, // USB3.0
	{ 0x547, 0xe075, 0x1075, "Meade DSI IV Color" }, // USB2.0
	{ 0x547, 0xe06d, 0x106d, "Meade DSI IV Mono" }, // USB3.0
	{ 0x547, 0xe076, 0x1076, "Meade DSI IV Mono" },  // USB2.0
	
	{ 0x547, 0xe00b, 0x11ca, "Meade LPI-GC Adv" }, // USB3.0
	{ 0x547, 0xe00c, 0x11cb, "Meade LPI-GC Adv" }, // USB2.0
	{ 0x547, 0xe00d, 0x11cc, "Meade LPI-GM Adv" }, // USB3.0
	{ 0x547, 0xe00e, 0x11cd, "Meade LPI-GM Adv" }, // USB2.0
	
	{ 0x547, 0xe007, 0x115a, "Meade LPI-GC Adv" }, // USB3.0 + temperature sensor
	{ 0x547, 0xe008, 0x115b, "Meade LPI-GC Adv" }, // USB2.0 + temperature sensor
	{ 0x547, 0xe009, 0x115c, "Meade LPI-GM Adv" }, // USB3.0 + temperature sensor
	{ 0x547, 0xe00a, 0x115d, "Meade LPI-GM Adv" }, // USB2.0 + temperature sensor

	{ 0x549, 0xe003, 0x1003, "Meade LPI-GC" },
	{ 0x549, 0xe004, 0x1004, "Meade LPI-GM" },

	{ 0, 0, 0, NULL }
};

int OEMCamEnum(ToupcamDeviceV2 *cams, int max_count) {
	int oem_count = 0;
	libusb_device **list;
	int usb_count = (int)libusb_get_device_list(NULL, &list);
	for (int i = 0; (i < usb_count) && (oem_count < max_count); i++) {
		libusb_device *dev = list[i];
		struct libusb_device_descriptor desc;
		libusb_get_device_descriptor(dev, &desc);
		for (int j = 0; oem_2_toupcam[j].name != NULL; j++) {
			if (oem_2_toupcam[j].oem_vid == desc.idVendor && oem_2_toupcam[j].oem_pid == desc.idProduct) {
				cams[oem_count].model = Toupcam_get_Model(TOUPTEK_VID, oem_2_toupcam[j].toupcam_pid);
				strcpy(cams[oem_count].displayname, oem_2_toupcam[j].name);
				sprintf(cams[oem_count].id, "tp-%d-%d-%d-%d", libusb_get_bus_number(dev), libusb_get_device_address(dev),TOUPTEK_VID, oem_2_toupcam[j].toupcam_pid);
				oem_count++;
			}
		}
	}
	libusb_free_device_list(list, 1);
	return oem_count;
}

#endif

static void process_plug_event(indigo_device *unusued) {
	pthread_mutex_lock(&mutex);
	for (int i = 0; i < SDK_DEF(MAX); i++) {
		indigo_device *device = devices[i];
		if (device)
			PRIVATE_DATA->present = false;
	}
	SDK_TYPE(DeviceV2) cams[SDK_DEF(MAX)];
	int count = SDK_CALL(EnumV2)(cams);

#ifdef TOUPTEK
	count += OEMCamEnum(&cams[count], TOUPCAM_MAX - count);
#endif

	for (int j = 0; j < count; j++) {
		SDK_TYPE(DeviceV2) cam = cams[j];
		bool found = false;
		for (int i = 0; i < SDK_DEF(MAX); i++) {
			indigo_device *device = devices[i];
			if (device && !strncmp(PRIVATE_DATA->cam.id, cam.id, sizeof(cam.id))) {
				found = true;
				PRIVATE_DATA->present = true;
				break;
			}
		}
		if (!found) {
			// Device is camera
			if (cam.model->flag & SDK_DEF(FLAG_CMOS) || cam.model->flag & SDK_DEF(FLAG_CCD_PROGRESSIVE) || cam.model->flag & SDK_DEF(FLAG_CCD_INTERLACED)) {
				static indigo_device ccd_template = INDIGO_DEVICE_INITIALIZER(
					"",
					ccd_attach,
					ccd_enumerate_properties,
					ccd_change_property,
					NULL,
					ccd_detach
				);

#ifdef INDIGO_MACOS
				char camera_id[16] = {0};
				SDK_HANDLE handle = SDK_CALL(Open)(cam.id);
				if (handle != NULL) {
					char serial[33] = {0};
					SDK_CALL(get_SerialNumber)(handle, serial);
					SDK_CALL(Close)(handle);
					strcpy(camera_id, serial + strlen(serial) - 6);
				} else {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Can not get serial number of Camera %s #%s", cam.displayname, cam.id);
				}
#endif

				DRIVER_PRIVATE_DATA *private_data = indigo_safe_malloc(sizeof(DRIVER_PRIVATE_DATA));
				private_data->cam = cam;
				private_data->present = true;
				indigo_device *camera = indigo_safe_malloc_copy(sizeof(indigo_device), &ccd_template);
#ifdef INDIGO_MACOS
				snprintf(camera->name, INDIGO_NAME_SIZE, "%s %s #%s", CAMERA_NAME_PREFIX, cam.displayname, camera_id);
#else
				snprintf(camera->name, INDIGO_NAME_SIZE, "%s %s", CAMERA_NAME_PREFIX, cam.displayname);
				indigo_make_name_unique(camera->name, NULL);
#endif
				camera->private_data = private_data;
				camera->master_device = camera;
				private_data->camera = camera;
				for (int i = 0; i < SDK_DEF(MAX); i++) {
					if (devices[i] == NULL) {
						indigo_attach_device(devices[i] = camera);
						break;
					}
				}
				if (cam.model->flag & SDK_DEF(FLAG_ST4)) {
					static indigo_device guider_template = INDIGO_DEVICE_INITIALIZER(
						"",
						guider_attach,
						indigo_guider_enumerate_properties,
						guider_change_property,
						NULL,
						guider_detach
						);
					indigo_device *guider = indigo_safe_malloc_copy(sizeof(indigo_device), &guider_template);
#ifdef INDIGO_MACOS
					snprintf(guider->name, INDIGO_NAME_SIZE, "%s %s (guider) #%s", CAMERA_NAME_PREFIX, cam.displayname, camera_id);
#else
					snprintf(guider->name, INDIGO_NAME_SIZE, "%s %s (guider)", CAMERA_NAME_PREFIX, cam.displayname);
					indigo_make_name_unique(guider->name, NULL);
#endif
					guider->private_data = private_data;
					guider->master_device = camera;
					private_data->guider = guider;
					indigo_attach_device(guider);
				}
			}
			// Device is filter wheel
			if (cam.model->flag & SDK_DEF(FLAG_FILTERWHEEL)) {
				static indigo_device wheel_template = INDIGO_DEVICE_INITIALIZER(
					"",
					wheel_attach,
					wheel_enumerate_properties,
					wheel_change_property,
					NULL,
					wheel_detach
				);

				DRIVER_PRIVATE_DATA *private_data = indigo_safe_malloc(sizeof(DRIVER_PRIVATE_DATA));
				private_data->cam = cam;
				private_data->present = true;
				indigo_device *camera = indigo_safe_malloc_copy(sizeof(indigo_device), &wheel_template);
				snprintf(camera->name, INDIGO_NAME_SIZE, "%s %s", CAMERA_NAME_PREFIX, cam.displayname);
				indigo_make_name_unique(camera->name, NULL);
				camera->private_data = private_data;
				camera->master_device = camera;
				private_data->camera = camera;
				for (int i = 0; i < SDK_DEF(MAX); i++) {
					if (devices[i] == NULL) {
						indigo_attach_device(devices[i] = camera);
						break;
					}
				}
			}
			// Device is focuser
			if (cam.model->flag & SDK_DEF(FLAG_AUTOFOCUSER)) {
				static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(
					"",
					focuser_attach,
					focuser_enumerate_properties,
					focuser_change_property,
					NULL,
					focuser_detach
				);

				DRIVER_PRIVATE_DATA *private_data = indigo_safe_malloc(sizeof(DRIVER_PRIVATE_DATA));
				private_data->cam = cam;
				private_data->present = true;
				indigo_device *camera = indigo_safe_malloc_copy(sizeof(indigo_device), &focuser_template);
				snprintf(camera->name, INDIGO_NAME_SIZE, "%s %s", CAMERA_NAME_PREFIX, cam.displayname);
				indigo_make_name_unique(camera->name, NULL);
				camera->private_data = private_data;
				camera->master_device = camera;
				private_data->camera = camera;
				for (int i = 0; i < SDK_DEF(MAX); i++) {
					if (devices[i] == NULL) {
						indigo_attach_device(devices[i] = camera);
						break;
					}
				}
			}
		}
	}
	for (int i = 0; i < SDK_DEF(MAX); i++) {
		indigo_device *device = devices[i];
		if (device && !PRIVATE_DATA->present) {
			indigo_device *guider = PRIVATE_DATA->guider;
			if (guider) {
				indigo_detach_device(guider);
				free(guider);
			}
			indigo_detach_device(device);
			if (device->private_data) {
				free(device->private_data);
			}
			free(device);
			devices[i] = NULL;
		}
	}
	pthread_mutex_unlock(&mutex);
}

static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	indigo_set_timer(NULL, 0.5, process_plug_event, NULL);
	return 0;
};

static void remove_all_devices() {
	for (int i = 0; i < SDK_DEF(MAX); i++) {
		indigo_device *device = devices[i];
		if (device) {
			indigo_device *guider = PRIVATE_DATA->guider;
			if (guider) {
				indigo_detach_device(guider);
				free(guider);
			}
			indigo_detach_device(device);
			if (device->private_data) {
				free(device->private_data);
			}
			free(device);
			devices[i] = NULL;
		}
	}
}

static libusb_hotplug_callback_handle callback_handle;

indigo_result ENTRY_POINT(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch(action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			for (int i = 0; i < SDK_DEF(MAX); i++)
				devices[i] = NULL;
			INDIGO_DRIVER_LOG(DRIVER_NAME, "SDK version %s", SDK_CALL(Version)());
//	dump cameras supported by SDK
//			for (int i = 0; i < 0xFFFF; i++) {
//				SDK_TYPE(ModelV2) *model = SDK_CALL(get_Model)(0x0547, i);
//				if (model) {
//					printf("%04x %s\n", i, model->name);
//				}
//			}
			indigo_start_usb_event_handler();
			int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_register_callback ->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
			return rc >= 0 ? INDIGO_OK : INDIGO_FAILED;
		}
		case INDIGO_DRIVER_SHUTDOWN:
			for (int i = 0; i < SDK_DEF(MAX); i++) {
				VERIFY_NOT_CONNECTED(devices[i]);
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

