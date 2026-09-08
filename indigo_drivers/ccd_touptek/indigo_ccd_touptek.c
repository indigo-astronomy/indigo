// Copyright (c) 2018-2026 CloudMakers, s. r. o.
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

// Queue refactoring by OpenAI Codex (2026).

// version history
// 2.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>
// 2.0 refactoring by Rumen G. Bogdanovski <rumenastro@gmail.com>
// 3.0 refactoring by Peter Polakovic <peter.polakovic@cloudmakers.eu>

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <stdatomic.h>
#include <stdint.h>
#include <ctype.h>

#include <indigo/indigo_usb_utils.h>
#include <indigo/indigo_driver_xml.h>

#include "indigo_ccd_touptek_vendors.h"

#pragma mark - Common definitions

#define DRIVER_VERSION									0x0300002b
#define PRIVATE_DATA												((DRIVER_PRIVATE_DATA *)device->private_data)

#define ADVANCED_GROUP											"Advanced"

#ifndef MAKEFOURCC
#define MAKEFOURCC(a, b, c, d) ((unsigned)(unsigned char)(a) | ((unsigned)(unsigned char)(b) << 8) | ((unsigned)(unsigned char)(c) << 16) | ((unsigned)(unsigned char)(d) << 24))
#endif

#define ROUND_BIN(dimention, bin) (2 * ((unsigned)(dimention) / (unsigned)(bin) / 2))

#ifdef TOUPTEK
#define TOUPTEK_VID		0x0547
#endif

#pragma mark - Property definitions

#define CCD_MODE_RAW08_ITEM_NAME						"RAW08_%d"
#define CCD_MODE_RAW10_ITEM_NAME						"RAW10_%d"
#define CCD_MODE_RAW12_ITEM_NAME						"RAW12_%d"
#define CCD_MODE_RAW14_ITEM_NAME						"RAW14_%d"
#define CCD_MODE_RAW16_ITEM_NAME						"RAW16_%d"
#define CCD_MODE_RGB08_ITEM_NAME						"RGB08_%d"
#define CCD_MODE_MON08_ITEM_NAME						"MON08_%d"
#define CCD_MODE_MON10_ITEM_NAME						"MON10_%d"
#define CCD_MODE_MON12_ITEM_NAME						"MON12_%d"
#define CCD_MODE_MON14_ITEM_NAME						"MON14_%d"
#define CCD_MODE_MON16_ITEM_NAME						"MON16_%d"

#define X_CCD_ADVANCED_PROPERTY							(PRIVATE_DATA->advanced_property)
#define X_CCD_SPEED_ITEM										(X_CCD_ADVANCED_PROPERTY->items + 0)
#define X_CCD_CONTRAST_ITEM									(X_CCD_ADVANCED_PROPERTY->items + 1)
#define X_CCD_HUE_ITEM											(X_CCD_ADVANCED_PROPERTY->items + 2)
#define X_CCD_SATURATION_ITEM								(X_CCD_ADVANCED_PROPERTY->items + 3)
#define X_CCD_BRIGHTNESS_ITEM								(X_CCD_ADVANCED_PROPERTY->items + 4)
#define X_CCD_GAMMA_ITEM										(X_CCD_ADVANCED_PROPERTY->items + 5)
#define X_CCD_R_GAIN_ITEM										(X_CCD_ADVANCED_PROPERTY->items + 6)
#define X_CCD_G_GAIN_ITEM										(X_CCD_ADVANCED_PROPERTY->items + 7)
#define X_CCD_B_GAIN_ITEM										(X_CCD_ADVANCED_PROPERTY->items + 8)
#define X_CCD_ADVANCED_PROPERTY_NAME				"X_CCD_ADVANCED"
#define X_CCD_SPEED_ITEM_NAME								"SPEED"
#define X_CCD_CONTRAST_ITEM_NAME						"CONTRAST"
#define X_CCD_HUE_ITEM_NAME									"HUE"
#define X_CCD_SATURATION_ITEM_NAME					"SATURATION"
#define X_CCD_BRIGHTNESS_ITEM_NAME					"BRIGHTNESS"
#define X_CCD_GAMMA_ITEM_NAME								"GAMMA"
#define X_CCD_R_GAIN_ITEM_NAME							"R_GAIN"
#define X_CCD_G_GAIN_ITEM_NAME							"G_GAIN"
#define X_CCD_B_GAIN_ITEM_NAME							"B_GAIN"

#define X_CCD_FAN_PROPERTY									(PRIVATE_DATA->fan_property)
#define X_CCD_FAN_SPEED_ITEM								(X_CCD_FAN_PROPERTY->items + 0)
#define X_CCD_FAN_PROPERTY_NAME							"X_CCD_FAN"
#define X_CCD_FAN_SPEED_ITEM_NAME						"FAN_SPEED"

#define X_CCD_HEATER_PROPERTY								(PRIVATE_DATA->heater_property)
#define X_CCD_HEATER_POWER_ITEM							(X_CCD_HEATER_PROPERTY->items + 0)
#define X_CCD_HEATER_PROPERTY_NAME					"X_CCD_HEATER"
#define X_CCD_HEATER_POWER_ITEM_NAME				"POWER"

#define X_CCD_LED_PROPERTY									(PRIVATE_DATA->led_property)
#define X_CCD_LED_ON_ITEM										(X_CCD_LED_PROPERTY->items + 0)
#define X_CCD_LED_OFF_ITEM									(X_CCD_LED_PROPERTY->items + 1)
#define X_CCD_LED_PROPERTY_NAME							"X_CCD_LED"
#define X_CCD_LED_ON_ITEM_NAME							"ON"
#define X_CCD_LED_OFF_ITEM_NAME							"OFF"

#define X_CCD_CONVERSION_GAIN_PROPERTY			(PRIVATE_DATA->conversion_gain_property)
#define X_CCD_CONVERSION_GAIN_LCG_ITEM			(X_CCD_CONVERSION_GAIN_PROPERTY->items + 0)
#define X_CCD_CONVERSION_GAIN_HCG_ITEM			(X_CCD_CONVERSION_GAIN_PROPERTY->items + 1)
#define X_CCD_CONVERSION_GAIN_HDR_ITEM			(X_CCD_CONVERSION_GAIN_PROPERTY->items + 2)
#define X_CCD_CONVERSION_GAIN_PROPERTY_NAME	"X_CCD_CONVERSION_GAIN"
#define X_CCD_CONVERSION_GAIN_LCG_ITEM_NAME	"LCG"
#define X_CCD_CONVERSION_GAIN_HCG_ITEM_NAME	"HCG"
#define X_CCD_CONVERSION_GAIN_HDR_ITEM_NAME	"HDR"

#define X_CCD_BIN_MODE_PROPERTY							(PRIVATE_DATA->bin_mode_property)
#define X_CCD_BIN_MODE_SATURATE_ITEM				(X_CCD_BIN_MODE_PROPERTY->items + 0)
#define X_CCD_BIN_MODE_EXPAND_ITEM					(X_CCD_BIN_MODE_PROPERTY->items + 1)
#define X_CCD_BIN_MODE_AVERAGE_ITEM					(X_CCD_BIN_MODE_PROPERTY->items + 2)
#define X_CCD_BIN_MODE_PROPERTY_NAME				"X_CCD_BIN_MODE"
#define X_CCD_BIN_MODE_SATURATE_ITEM_NAME		"SATURATE"
#define X_CCD_BIN_MODE_EXPAND_ITEM_NAME			"EXPAND"
#define X_CCD_BIN_MODE_AVERAGE_ITEM_NAME		"AVERAGE"

#define X_CALIBRATE_PROPERTY								(PRIVATE_DATA->calibrate_property)
#define X_CALIBRATE_START_ITEM							(X_CALIBRATE_PROPERTY->items + 0)
#define X_CALIBRATE_PROPERTY_NAME						"X_CALIBRATE"
#define X_CALIBRATE_START_ITEM_NAME					"START"

#define X_WHEEL_MODEL_PROPERTY							(PRIVATE_DATA->wheel_model_property)
#define X_WHEEL_MODEL_5_POSITION_ITEM				(X_WHEEL_MODEL_PROPERTY->items + 0)
#define X_WHEEL_MODEL_7_POSITION_ITEM				(X_WHEEL_MODEL_PROPERTY->items + 1)
#define X_WHEEL_MODEL_8_POSITION_ITEM				(X_WHEEL_MODEL_PROPERTY->items + 2)
#define X_WHEEL_MODEL_PROPERTY_NAME					"X_WHEEL_MODEL"
#define X_WHEEL_MODEL_5_POSITION_ITEM_NAME	"5_POSITIONS"
#define X_WHEEL_MODEL_7_POSITION_ITEM_NAME	"7_POSITIONS"
#define X_WHEEL_MODEL_8_POSITION_ITEM_NAME	"8_POSITIONS"

#define X_BEEP_PROPERTY											(PRIVATE_DATA->beep_property)
#define X_BEEP_ON_ITEM											(X_BEEP_PROPERTY->items + 0)
#define X_BEEP_OFF_ITEM											(X_BEEP_PROPERTY->items + 1)
#define X_BEEP_PROPERTY_NAME								"X_AAF_BEEP"
#define X_BEEP_ON_ITEM_NAME									"ON"
#define X_BEEP_OFF_ITEM_NAME								"OFF"

#pragma mark - Private data definition

typedef struct {
	SDK_TYPE(DeviceV2) cam;
	SDK_HANDLE handle;
	int count;
	atomic_bool removing;
	atomic_uintptr_t event_generation; // SDK notifications carry the pull-mode generation
	/* Camera related */
	indigo_device *camera;
	char bayer_pattern[5];
	indigo_device *guider;
	double current_temperature;
	char *buffer;
	unsigned bin_mode;
	int bits;
	int mode;
	int left, top, width, height;
	bool aborting;
	bool video_mode;
	indigo_property *advanced_property;
	indigo_property *fan_property;
	indigo_property *heater_property;
	indigo_property *conversion_gain_property;
	indigo_property *bin_mode_property;
	indigo_property *led_property;
	/* wheel related */
	int current_slot, target_slot;
	indigo_property *calibrate_property;
	indigo_property *wheel_model_property;
	/* focuser related */
	bool has_temperature_sensor;
	int current_position, target_position;
	int max_position;
	int backlash;
	double prev_temp;
	indigo_property *beep_property;
} DRIVER_PRIVATE_DATA;

#pragma mark - Low level code

static indigo_queue *driver_queue;
static _Atomic(indigo_driver_action) last_action = INDIGO_DRIVER_SHUTDOWN;

#ifdef TOUPTEK

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
	{ 0x547, 0xe076, 0x1076, "Meade DSI IV Mono" }, // USB2.0

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
	if (usb_count < 0) {
		return 0;
	}
	for (int i = 0; (i < usb_count) && (oem_count < max_count); i++) {
		libusb_device *dev = list[i];
		struct libusb_device_descriptor desc;
		if (libusb_get_device_descriptor(dev, &desc) < 0) {
			continue;
		}
		for (int j = 0; oem_2_toupcam[j].name != NULL; j++) {
			if (oem_2_toupcam[j].oem_vid == desc.idVendor && oem_2_toupcam[j].oem_pid == desc.idProduct) {
				cams[oem_count].model = Toupcam_get_Model(TOUPTEK_VID, oem_2_toupcam[j].toupcam_pid);
				INDIGO_STRCPYW(cams[oem_count].displayname, INDIGO_CHAR_TO_WCHAR(oem_2_toupcam[j].name));
				INDIGO_SNPRINTFW(cams[oem_count].id, sizeof(cams[oem_count].id) / 2, "tp-%d-%d-%d-%d", libusb_get_bus_number(dev), libusb_get_device_address(dev), TOUPTEK_VID, oem_2_toupcam[j].toupcam_pid);
				oem_count++;
			}
		}
	}
	libusb_free_device_list(list, 1);
	return oem_count;
}

#endif

static void ccd_start_exposure_handler(indigo_device *device);

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
	// we normalize the values to 256 to get nice round scale, however valid values would be from 0-248 as BLACKLEVEL8_MAX is 31 not 32
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

static void get_bayer_pattern(indigo_device *device) {
	unsigned fourcc = 0, bitspp = 0;
	HRESULT result = SDK_CALL(get_RawFormat)(PRIVATE_DATA->handle, &fourcc, &bitspp);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "get_RawFormat(->%x, -> %d) = %d", fourcc, bitspp, result);
	switch (fourcc) {
		case MAKEFOURCC('G', 'B', 'R', 'G'):
			strncpy(PRIVATE_DATA->bayer_pattern, "GBRG", 5);
			break;
		case MAKEFOURCC('R', 'G', 'G', 'B'):
			strncpy(PRIVATE_DATA->bayer_pattern, "RGGB", 5);
			break;
		case MAKEFOURCC('B', 'G', 'G', 'R'):
			strncpy(PRIVATE_DATA->bayer_pattern, "BGGR", 5);
			break;
		case MAKEFOURCC('G', 'R', 'B', 'G'):
			strncpy(PRIVATE_DATA->bayer_pattern, "GRBG", 5);
			break;
		default:
			strncpy(PRIVATE_DATA->bayer_pattern, "", 5);
	}
}

static void finish_exposure(indigo_device *device) {
	CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
}

static void stop_video_mode(indigo_device *device) {
	if (!PRIVATE_DATA->video_mode) {
		return;
	}
	PRIVATE_DATA->video_mode = false;
	bool aborting = CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE;
	HRESULT result = SDK_CALL(put_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_TRIGGER), 1);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_Option(OPTION_TRIGGER, 1) -> %08x", result);
	// The SDK has joined its callback. Discard notifications from the stopped stream.
	atomic_store(&PRIVATE_DATA->event_generation, (atomic_load(&PRIVATE_DATA->event_generation) + 1) & (UINTPTR_MAX >> 16));
	if (aborting) {
		indigo_finalize_video_stream(device);
	} else {
		indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
	}
}

static void cleanup_aborted_exposure(indigo_device *device) {
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
		indigo_ccd_abort_exposure_cleanup(device);
	}
}

static void ccd_exposure_watchdog_handler(indigo_device *device) {
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "pull_callback() was not called in time");
	// Flush the frame buffer to unstick the SDK pipeline.  With multiple cameras and short exposures the SDK occasionally stops delivering EVENT_IMAGE, leaving
	// the buffer full so that every subsequent Trigger() is also silently dropped. Flushing clears that condition.  We also reset PRIVATE_DATA->mode so that the
	// next call to setup_exposure() unconditionally re-calls StartPullModeWithCallback(), recovering from any case where the SDK silently
	// dropped the callback registration (observed race in the closed-source SDK when two cameras fire nearly simultaneously).
	HRESULT result = SDK_CALL(put_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_FLUSH), 3);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_Option(OPTION_FLUSH, 3) -> %08x", result);
	PRIVATE_DATA->mode = -1;
	CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Exposure failed, pull callback was not called");
}

static void ccd_event_handler(indigo_device *device, void *data) {
	uintptr_t notification = (uintptr_t)data;
	if ((notification >> 16) != atomic_load(&PRIVATE_DATA->event_generation)) {
		return;
	}
	unsigned event = notification & 0xffff;
	SDK_TYPE(FrameInfoV2) frameInfo = { 0 };
	HRESULT result;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "pull_callback(%04x) called", event);
	indigo_cancel_pending_handler(device, ccd_exposure_watchdog_handler);
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
			result = SDK_CALL(PullImageV2)(PRIVATE_DATA->handle, PRIVATE_DATA->buffer + FITS_HEADER_SIZE, PRIVATE_DATA->bits, &frameInfo);
			if (result >= 0) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "PullImageV2(%d, ->[%d x %d, %x, %d]) -> %08x", PRIVATE_DATA->bits, frameInfo.width, frameInfo.height, frameInfo.flag, frameInfo.seq, result);
				if (PRIVATE_DATA->aborting) {
					// Abort path (single-exposure or streaming): discard this frame, finalize any open video file, and clean up.
					PRIVATE_DATA->aborting = false;
					indigo_finalize_video_stream(device);
					cleanup_aborted_exposure(device);
				} else {
					if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
						indigo_process_image(device, PRIVATE_DATA->buffer, frameInfo.width, frameInfo.height, PRIVATE_DATA->bits > 8 && PRIVATE_DATA->bits <= 16 ? 16 : PRIVATE_DATA->bits, true, true, fits_keywords, false);
						CCD_EXPOSURE_ITEM->number.value = 0;
						finish_exposure(device);
					} else if (CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
						indigo_process_image(device, PRIVATE_DATA->buffer, frameInfo.width, frameInfo.height, PRIVATE_DATA->bits > 8 && PRIVATE_DATA->bits <= 16 ? 16 : PRIVATE_DATA->bits, true, true, fits_keywords, true);
						if (CCD_STREAMING_COUNT_ITEM->number.value > 0) {
							CCD_STREAMING_COUNT_ITEM->number.value--;
						}
						if (CCD_STREAMING_COUNT_ITEM->number.value == 0) {
							indigo_finalize_video_stream(device);
							CCD_STREAMING_PROPERTY->state = INDIGO_OK_STATE;
							/* Stop streaming here, outside the SDK callback. */
							stop_video_mode(device);
						} else {
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
					stop_video_mode(device);
				}
			}
			break;
		}
		case SDK_DEF(EVENT_NOFRAMETIMEOUT):
		case SDK_DEF(EVENT_NOPACKETTIMEOUT):
		case SDK_DEF(EVENT_ERROR): {
			result = SDK_CALL(put_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_FLUSH), 3);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_Option(OPTION_FLUSH, 3) -> %08x", result);
			indigo_ccd_failure_cleanup(device);
			if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
				CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "SDK reported error");
			} else if (CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
				indigo_finalize_video_stream(device);
				CCD_STREAMING_PROPERTY->state = INDIGO_ALERT_STATE;
				stop_video_mode(device);
			}
			break;
		}
	}
}

static void pull_callback(unsigned event, void *callbackCtx) {
	indigo_device *device = callbackCtx;
	uintptr_t notification = (atomic_load(&PRIVATE_DATA->event_generation) << 16) | (event & 0xffff);
	indigo_execute_handler_with_data(device, ccd_event_handler, (void *)notification);
}

static void ccd_temperature_monitor_handler(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	short temperature;
	HRESULT result = SDK_CALL(get_Temperature)(PRIVATE_DATA->handle, &temperature);
	if (result >= 0) {
		PRIVATE_DATA->current_temperature = CCD_TEMPERATURE_ITEM->number.value = temperature / 10.0;
		if (CCD_TEMPERATURE_PROPERTY->perm == INDIGO_RW_PERM && fabs(CCD_TEMPERATURE_ITEM->number.value - CCD_TEMPERATURE_ITEM->number.target) > 1.0) {
			if (!CCD_COOLER_PROPERTY->hidden && CCD_COOLER_OFF_ITEM->sw.value) {
				CCD_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				CCD_TEMPERATURE_PROPERTY->state = INDIGO_BUSY_STATE;
			}
		} else {
			CCD_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
		}
		indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "get_Temperature() -> %08x", result);
	}
	if (!CCD_COOLER_POWER_PROPERTY->hidden) {
		int current_voltage = 0, max_voltage = 0;
		// When cooler is OFF current_voltage is reported as the last measured when power was ON, so we set it to 0 to show correct percentage
		if (CCD_COOLER_ON_ITEM->sw.value) {
			result = SDK_CALL(get_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_TEC_VOLTAGE), &current_voltage);
		} else {
			current_voltage = 0;
		}
		result = SDK_CALL(get_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_TEC_VOLTAGE_MAX), &max_voltage);
		if (result >= 0 && max_voltage > 0) {
			double cooler_power = (double)current_voltage / max_voltage * 100;
			CCD_COOLER_POWER_PROPERTY->state = INDIGO_OK_STATE;
			CCD_COOLER_POWER_ITEM->number.value = round(cooler_power);
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "get_Option(OPTION_TEC_VOLTAGE_MAX) -> %08x", result);
			CCD_COOLER_POWER_PROPERTY->state = INDIGO_ALERT_STATE;
			CCD_COOLER_POWER_ITEM->number.value = 0;
		}
		indigo_update_property(device, CCD_COOLER_POWER_PROPERTY, NULL);
	}
	indigo_execute_handler_in(device, 5, ccd_temperature_monitor_handler);
}

static bool exposure_setup_pending(indigo_device *device) {
	return CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE;
}

static void setup_exposure_frame(indigo_device *device) {
	if (!exposure_setup_pending(device)) {
		return;
	}
	HRESULT result;
	if (PRIVATE_DATA->cam.model->flag & SDK_DEF(FLAG_ROI_HARDWARE)) {
		unsigned left = ROUND_BIN(CCD_FRAME_LEFT_ITEM->number.value, 1);
		unsigned top = ROUND_BIN(CCD_FRAME_TOP_ITEM->number.value, 1);
		unsigned width = ROUND_BIN(CCD_FRAME_WIDTH_ITEM->number.value, 1);
		if (width < 16) {
			width = 16;
		}
		unsigned height = ROUND_BIN(CCD_FRAME_HEIGHT_ITEM->number.value, 1);
		if (height < 16) {
			height = 16;
		}
		unsigned max_width = (unsigned)CCD_INFO_WIDTH_ITEM->number.value;
		unsigned max_height = (unsigned)CCD_INFO_HEIGHT_ITEM->number.value;
		if (left + width > max_width || top + height > max_height) {
			left = top = 0;
			width = max_width;
			height = max_height;
		}
		if (PRIVATE_DATA->left != left || PRIVATE_DATA->top != top || PRIVATE_DATA->width != width || PRIVATE_DATA->height != height) {
			result = SDK_CALL(put_Roi)(PRIVATE_DATA->handle, left, top, width, height);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_Roi(%d, %d, %d, %d) -> %08x", left, top, width, height, result);
			PRIVATE_DATA->left = left;
			PRIVATE_DATA->top = top;
			PRIVATE_DATA->width = width;
			PRIVATE_DATA->height = height;
			indigo_execute_handler_in(device, 0.1, ccd_start_exposure_handler);
			return;
		}
	}
	ccd_start_exposure_handler(device);
}

static void ccd_setup_pull_handler(indigo_device *device) {
	if (!exposure_setup_pending(device)) {
		return;
	}
	HRESULT result = SDK_CALL(StartPullModeWithCallback)(PRIVATE_DATA->handle, pull_callback, device);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "StartPullModeWithCallback() -> %08x", result);
	setup_exposure_frame(device);
}

static void ccd_setup_binning_handler(indigo_device *device) {
	if (!exposure_setup_pending(device)) {
		return;
	}
	unsigned binning = atoi(strchr(CCD_MODE_PROPERTY->items[PRIVATE_DATA->mode].name, '_') + 1);
	HRESULT result = SDK_CALL(put_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_BINNING), PRIVATE_DATA->bin_mode | binning);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_Option(OPTION_BINNING, %x) -> %08x", PRIVATE_DATA->bin_mode | binning, result);
	indigo_execute_handler_in(device, 0.1, ccd_setup_pull_handler);
}

static void setup_exposure(indigo_device *device) {
	HRESULT result;
	for (int i = 0; i < CCD_MODE_PROPERTY->count; i++) {
		indigo_item *item = CCD_MODE_PROPERTY->items + i;
		if (item->sw.value) {
			if (PRIVATE_DATA->mode != i) {
				result = SDK_CALL(Stop)(PRIVATE_DATA->handle);
				// Stop joins the callback; notifications already queued belong to the old pull mode.
				atomic_store(&PRIVATE_DATA->event_generation, (atomic_load(&PRIVATE_DATA->event_generation) + 1) & (UINTPTR_MAX >> 16));
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Stop() -> %08x", result);
				if (strncmp(item->name, "RAW08", 5) == 0 || strncmp(item->name, "MON08", 5) == 0) {
					result = SDK_CALL(put_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_RAW), 1);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_Option(OPTION_RAW, 1) -> %08x", result);
					result = SDK_CALL(put_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_BITDEPTH), 0);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_Option(OPTION_BITDEPTH, 0) -> %08x", result);
					PRIVATE_DATA->bits = 8;
				} else if (strncmp(item->name, "RAW", 3) == 0 || strncmp(item->name, "MON", 3) == 0) {
					result = SDK_CALL(put_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_BITDEPTH), 1);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_Option(OPTION_BITDEPTH, 1) -> %08x", result);
					result = SDK_CALL(put_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_RAW), 1);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_Option(OPTION_RAW, 1) -> %08x", result);
					PRIVATE_DATA->bits = atoi(item->name + 3); // FIXME: should be ignored in RAW mode, but it is not
				} else if (strncmp(item->name, "RGB08", 5) == 0) {
					result = SDK_CALL(put_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_RAW), 0);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_Option(OPTION_RAW, 0) -> %08x", result);
					result = SDK_CALL(put_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_BITDEPTH), 0);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_Option(OPTION_BITDEPTH, 0) -> %08x", result);
					PRIVATE_DATA->bits = 24;
				}
				PRIVATE_DATA->mode = i;
				indigo_execute_handler_in(device, 0.1, ccd_setup_binning_handler);
				return;
			}
			break;
		}
	}
	setup_exposure_frame(device);
}

static void ccd_start_exposure_handler(indigo_device *device) {
	if (!exposure_setup_pending(device)) {
		return;
	}
	HRESULT result = SDK_CALL(put_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_FLUSH), 3);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_Option(OPTION_FLUSH) -> %08x", result);
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		result = SDK_CALL(put_ExpoTime)(PRIVATE_DATA->handle, (unsigned)(CCD_EXPOSURE_ITEM->number.target * 1000000));
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_ExpoTime(%u) -> %08x", (unsigned)(CCD_EXPOSURE_ITEM->number.target * 1000000), result);
		PRIVATE_DATA->aborting = false;
		result = SDK_CALL(Trigger)(PRIVATE_DATA->handle, 1);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Trigger(1) -> %08x", result);
		double whatchdog_timeout = (CCD_EXPOSURE_ITEM->number.target > 50) ? 1.5 * CCD_EXPOSURE_ITEM->number.target : CCD_EXPOSURE_ITEM->number.target + 25;
		indigo_execute_handler_in(device, whatchdog_timeout, ccd_exposure_watchdog_handler);
		indigo_ccd_change_property(device, NULL, CCD_EXPOSURE_PROPERTY);
	} else {
		result = SDK_CALL(put_ExpoTime)(PRIVATE_DATA->handle, (unsigned)(CCD_STREAMING_EXPOSURE_ITEM->number.target * 1000000));
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_ExpoTime(%u) -> %08x", (unsigned)(CCD_STREAMING_EXPOSURE_ITEM->number.target * 1000000), result);
		PRIVATE_DATA->aborting = false;
		PRIVATE_DATA->video_mode = true;
		result = SDK_CALL(put_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_TRIGGER), 0);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_Option(OPTION_TRIGGER, 0) -> %08x", result);
		indigo_ccd_change_property(device, NULL, CCD_STREAMING_PROPERTY);
	}
}

static void guider_guide_ra_finalizer(indigo_device *device) {
	GUIDER_GUIDE_EAST_ITEM->number.value = 0;
	GUIDER_GUIDE_WEST_ITEM->number.value = 0;
	GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
}

static void guider_guide_dec_finalizer(indigo_device *device) {
	GUIDER_GUIDE_NORTH_ITEM->number.value = 0;
	GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
	GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
}

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
	WHEEL_SLOT_OFFSET_PROPERTY->count = positions;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "get_Option(OPTION_FILTERWHEEL_SLOT) -> %08x, %d", result, positions);
}

static void wheel_move_finalizer(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	HRESULT result = SDK_CALL(get_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_FILTERWHEEL_POSITION), &PRIVATE_DATA->current_slot);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "get_Option(OPTION_FILTERWHEEL_POSITION) -> %08x, %d", result, PRIVATE_DATA->current_slot);
	PRIVATE_DATA->current_slot++;
	WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->current_slot;
	if (PRIVATE_DATA->current_slot == PRIVATE_DATA->target_slot) {
		WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
	} else if (PRIVATE_DATA->current_slot == 0) { //still moving
		indigo_execute_handler_in(device, 0.5, wheel_move_finalizer);
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Set filter %d failed", (int)WHEEL_SLOT_ITEM->number.target);
		WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
}

static void wheel_calibrate_finalizer(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	int pos = 0;
	HRESULT result = SDK_CALL(get_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_FILTERWHEEL_POSITION), &pos);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "get_Option(OPTION_FILTERWHEEL_POSITION) -> %08x, %d", result, pos);
	if (pos == -1) {
		indigo_execute_handler_in(device, 1, wheel_calibrate_finalizer);
		return;
	}
	WHEEL_SLOT_ITEM->number.value =
	WHEEL_SLOT_ITEM->number.target =
	PRIVATE_DATA->current_slot =
	PRIVATE_DATA->target_slot = pos + 1;
	WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
	X_CALIBRATE_START_ITEM->sw.value = false;
	X_CALIBRATE_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, X_CALIBRATE_PROPERTY, "Calibration finished");
}

static void wheel_calibrate_handler(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	HRESULT result = SDK_CALL(put_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_FILTERWHEEL_POSITION), -1); // -1 means calibrate
	if (SUCCEEDED(result)) {
		indigo_execute_handler_in(device, 1, wheel_calibrate_finalizer);
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "put_Option(OPTION_FILTERWHEEL_POSITION, -1) -> %08x", result);
		WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
		X_CALIBRATE_START_ITEM->sw.value = false;
		X_CALIBRATE_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_CALIBRATE_PROPERTY, "Calibration failed");
	}
}

static void wheel_connection_finalizer(indigo_device *device) {
	if (!CONNECTION_CONNECTED_ITEM->sw.value || PRIVATE_DATA->handle == NULL) {
		return;
	}
	int value = 0;
	HRESULT result = SDK_CALL(get_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_FILTERWHEEL_POSITION), &value);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "get_Option(OPTION_FILTERWHEEL_POSITION) -> %08x, %d", result, value + 1);
	if (value == -1) {
		indigo_execute_handler_in(device, 1, wheel_connection_finalizer);
	} else {
		WHEEL_SLOT_ITEM->number.value = WHEEL_SLOT_ITEM->number.target = value + 1;
		indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_wheel_change_property(device, NULL, CONNECTION_PROPERTY);
	}
}

static void focuser_move_finalizer(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	int is_moving = 0;
	HRESULT res;
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
	FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
	if ((!is_moving) || (PRIVATE_DATA->current_position == PRIVATE_DATA->target_position)) {
		FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
		FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		indigo_execute_handler_in(device, 0.5, focuser_move_finalizer);
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
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Compensation: temp_difference = %.2f, Compensation = %d, steps/degC = %.0f, threshold = %.2f", temp_difference, compensation, FOCUSER_COMPENSATION_ITEM->number.value, FOCUSER_COMPENSATION_THRESHOLD_ITEM->number.value);
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Not compensating (not needed): temp_difference = %.2f, threshold = %.2f", temp_difference, FOCUSER_COMPENSATION_THRESHOLD_ITEM->number.value);
		return;
	}
	PRIVATE_DATA->target_position = PRIVATE_DATA->current_position + compensation;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Compensation: PRIVATE_DATA->current_position = %d, PRIVATE_DATA->target_position = %d", PRIVATE_DATA->current_position, PRIVATE_DATA->target_position);
	HRESULT res = (SDK_CALL(AAF)(PRIVATE_DATA->handle, SDK_DEF(AAF_GETPOSITION), 0, &PRIVATE_DATA->current_position));
	if (FAILED(res)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "AAF(AAF_GETPOSITION) -> %08x (value = %d) (failed)", res, PRIVATE_DATA->current_position);
	}
	// Make sure we do not attempt to go beyond the limits
	if (FOCUSER_POSITION_ITEM->number.max < PRIVATE_DATA->target_position) {
		PRIVATE_DATA->target_position = (int)FOCUSER_POSITION_ITEM->number.max;
	} else if (FOCUSER_POSITION_ITEM->number.min > PRIVATE_DATA->target_position) {
		PRIVATE_DATA->target_position = (int)FOCUSER_POSITION_ITEM->number.min;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Compensating: Corrected PRIVATE_DATA->target_position = %d", PRIVATE_DATA->target_position);
	res = (SDK_CALL(AAF)(PRIVATE_DATA->handle, SDK_DEF(AAF_SETPOSITION), PRIVATE_DATA->target_position, NULL));
	if (FAILED(res)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "AAF(AAF_SETPOSITION) -> %08x (value = %d) (failed)", res, PRIVATE_DATA->target_position);
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	PRIVATE_DATA->prev_temp = new_temp;
	FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
	FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	indigo_cancel_pending_handler(device, focuser_move_finalizer);
	indigo_execute_handler_in(device, 0.5, focuser_move_finalizer);
}

static void focuser_temperature_handler(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	int temp10 = -2732;
	HRESULT res;
	FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
	res = (SDK_CALL(AAF)(PRIVATE_DATA->handle, SDK_DEF(AAF_GETAMBIENTTEMP), 0, &temp10));
	if (FAILED(res)) {
		if (PRIVATE_DATA->has_temperature_sensor) {
			INDIGO_DRIVER_LOG(DRIVER_NAME, "The temperature sensor is not connected (using internal sensor).");
			indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, "The temperature sensor is not connected (using internal sensor).");
		}
		PRIVATE_DATA->has_temperature_sensor = false;
	} else {
		if (!PRIVATE_DATA->has_temperature_sensor) {
			INDIGO_DRIVER_LOG(DRIVER_NAME, "The temperature sensor connected.");
			indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, "The temperature sensor connected.");
		}
		PRIVATE_DATA->has_temperature_sensor = true;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "AAF(AAF_GETAMBIENTTEMP) -> %08x (value = %d)", res, temp10);
	if (!PRIVATE_DATA->has_temperature_sensor) {
		res = (SDK_CALL(AAF)(PRIVATE_DATA->handle, SDK_DEF(AAF_GETTEMP), 0, &temp10));
		if (FAILED(res)) {
			temp10 = -2732;
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "AAF(AAF_GETTEMP) -> %08x (value = %d) (failed)", res, temp10);
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "AAF(AAF_GETTEMP) -> %08x (value = %d)", res, temp10);
		}
	}
	FOCUSER_TEMPERATURE_ITEM->number.value = (double)temp10 / 10.0;
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
	indigo_execute_handler_in(device, 2, focuser_temperature_handler);
}

#pragma mark - High level code (ccd)

static void ccd_connection_handler(indigo_device *device) {
	HRESULT result;
	indigo_cancel_pending_handlers(device);
	indigo_lock_master_device(device);
	// Cancelled requests must not leave properties BUSY in the next session.
	indigo_property *properties[] = {
		CCD_MODE_PROPERTY, CCD_BIN_PROPERTY, CCD_FRAME_PROPERTY, CCD_EXPOSURE_PROPERTY,
		CCD_STREAMING_PROPERTY, CCD_ABORT_EXPOSURE_PROPERTY, CCD_COOLER_PROPERTY, CCD_TEMPERATURE_PROPERTY,
		CCD_GAIN_PROPERTY, CCD_OFFSET_PROPERTY, X_CCD_ADVANCED_PROPERTY, X_CCD_FAN_PROPERTY,
		X_CCD_HEATER_PROPERTY, X_CCD_CONVERSION_GAIN_PROPERTY, X_CCD_LED_PROPERTY, X_CCD_BIN_MODE_PROPERTY,
		CONFIG_PROPERTY
	};
	for (unsigned i = 0; i < sizeof(properties) / sizeof(properties[0]); i++) {
		if (properties[i] && properties[i]->state == INDIGO_BUSY_STATE) {
			properties[i]->state = INDIGO_ALERT_STATE;
		}
	}
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (PRIVATE_DATA->count++ == 0) {
			if (indigo_try_global_lock(device) != INDIGO_OK) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_try_global_lock(): failed to get lock.");
			} else {
				char id[66];
				sprintf(id, "@%s", INDIGO_WCHAR_TO_CHAR(PRIVATE_DATA->cam.id));
				PRIVATE_DATA->handle = SDK_CALL(Open)(INDIGO_CHAR_TO_WCHAR(id));
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Open(%s) -> %p", id, PRIVATE_DATA->handle);
				if (PRIVATE_DATA->handle == NULL) {
					indigo_global_unlock(device);
				}
			}
		}
		if (PRIVATE_DATA->handle) {
			PRIVATE_DATA->buffer = (char *)indigo_alloc_blob_buffer(3 * (int)CCD_INFO_WIDTH_ITEM->number.value * (int)CCD_INFO_HEIGHT_ITEM->number.value + FITS_HEADER_SIZE);
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
				indigo_execute_handler_in(device, 5.0, ccd_temperature_monitor_handler);
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
			get_bayer_pattern(device);
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
			unsigned min, max, current;
			SDK_CALL(get_ExpTimeRange)(PRIVATE_DATA->handle, &min, &max, &current);
			CCD_EXPOSURE_ITEM->number.min = CCD_STREAMING_EXPOSURE_ITEM->number.min = min / 1000000.0;
			CCD_EXPOSURE_ITEM->number.max = CCD_STREAMING_EXPOSURE_ITEM->number.max = max / 1000000.0;
			unsigned short gain_min = 0, gain_max = 0, gain_current = 0;
			result = SDK_CALL(put_AutoExpoEnable)(PRIVATE_DATA->handle, false);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_AutoExpoEnable(false) -> %08x", result);
			result = SDK_CALL(get_ExpoAGainRange)(PRIVATE_DATA->handle, &gain_min, &gain_max, &gain_current);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "get_ExpoAGainRange(->%d, ->%d, ->%d) -> %08x", gain_min, gain_max, gain_current, result);
			result = SDK_CALL(get_ExpoAGain)(PRIVATE_DATA->handle, &gain_current);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "get_ExpoAGain(->%d) -> %08x", gain_current, result);
			CCD_GAIN_ITEM->number.min = gain_min;
			CCD_GAIN_ITEM->number.max = gain_max;
			CCD_GAIN_ITEM->number.value = gain_current;
			if (PRIVATE_DATA->cam.model->flag & SDK_DEF(FLAG_BLACKLEVEL)) {
				CCD_OFFSET_PROPERTY->hidden = false;
				CCD_OFFSET_ITEM->number.min = SDK_DEF(BLACKLEVEL_MIN);
				// offset values are normaized to 256 but max is 248 see get_blacklevel()
				CCD_OFFSET_ITEM->number.max = 248;
				int blacklevel = 0;
				double scale = 8;
				get_blacklevel(device, &blacklevel, &scale);
				CCD_OFFSET_ITEM->number.value = CCD_OFFSET_ITEM->number.target = blacklevel * scale;
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Offset supported: blacklevel=%d, scale=%f", blacklevel, scale);
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
			result = SDK_CALL(StartPullModeWithCallback)(PRIVATE_DATA->handle, pull_callback, device);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "StartPullModeWithCallback() -> %08x", result);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			PRIVATE_DATA->count--;
		}
	} else {
		if (PRIVATE_DATA->handle) {
			result = SDK_CALL(Stop)(PRIVATE_DATA->handle);
			atomic_store(&PRIVATE_DATA->event_generation, (atomic_load(&PRIVATE_DATA->event_generation) + 1) & (UINTPTR_MAX >> 16));
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Stop() -> %08x", result);
		}
		// Stop has joined the SDK callback; drain its last queued notifications before freeing the buffer.
		indigo_unlock_master_device(device);
		indigo_cancel_pending_handlers(device);
		indigo_lock_master_device(device);
		if (PRIVATE_DATA->buffer != NULL) {
			free(PRIVATE_DATA->buffer);
			PRIVATE_DATA->buffer = NULL;
		}
		if (X_CCD_ADVANCED_PROPERTY) {
			indigo_delete_property(device, X_CCD_ADVANCED_PROPERTY, NULL);
		}
		if (X_CCD_FAN_PROPERTY) {
			indigo_delete_property(device, X_CCD_FAN_PROPERTY, NULL);
		}
		if (X_CCD_HEATER_PROPERTY) {
			indigo_delete_property(device, X_CCD_HEATER_PROPERTY, NULL);
		}
		if (X_CCD_CONVERSION_GAIN_PROPERTY) {
			indigo_delete_property(device, X_CCD_CONVERSION_GAIN_PROPERTY, NULL);
		}
		if (X_CCD_BIN_MODE_PROPERTY) {
			indigo_delete_property(device, X_CCD_BIN_MODE_PROPERTY, NULL);
		}
		if (X_CCD_LED_PROPERTY) {
			indigo_delete_property(device, X_CCD_LED_PROPERTY, NULL);
		}
		if (--PRIVATE_DATA->count == 0) {
			if (PRIVATE_DATA->handle != NULL) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Closing camera");
				SDK_CALL(Close)(PRIVATE_DATA->handle);
			}
			PRIVATE_DATA->handle = NULL;
			indigo_global_unlock(device);
		}
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_ccd_change_property(device, NULL, CONNECTION_PROPERTY);
	indigo_unlock_master_device(device);
}

static void ccd_mode_handler(indigo_device *device) {
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
}

static void ccd_bin_handler(indigo_device *device) {
	int prev_h_bin = (int)CCD_BIN_HORIZONTAL_ITEM->number.value;
	int prev_v_bin = (int)CCD_BIN_VERTICAL_ITEM->number.value;
	CCD_BIN_HORIZONTAL_ITEM->number.value = CCD_BIN_HORIZONTAL_ITEM->number.target;
	CCD_BIN_VERTICAL_ITEM->number.value = CCD_BIN_VERTICAL_ITEM->number.target;
	PRIVATE_DATA->mode = PRIVATE_DATA->left = PRIVATE_DATA->top = PRIVATE_DATA->width = PRIVATE_DATA->height = -1;
	CCD_BIN_PROPERTY->state = INDIGO_OK_STATE;
	int horizontal_bin = (int)CCD_BIN_HORIZONTAL_ITEM->number.value;
	int vertical_bin = (int)CCD_BIN_VERTICAL_ITEM->number.value;
	/* Touptek (& family) cameras work with binx = biny for we force it here */
	if (prev_h_bin != horizontal_bin) {
		vertical_bin = (int)(CCD_BIN_HORIZONTAL_ITEM->number.target = CCD_BIN_HORIZONTAL_ITEM->number.value = CCD_BIN_VERTICAL_ITEM->number.target = CCD_BIN_VERTICAL_ITEM->number.value = horizontal_bin);
	} else if (prev_v_bin != vertical_bin) {
		horizontal_bin = (int)(CCD_BIN_HORIZONTAL_ITEM->number.target = CCD_BIN_HORIZONTAL_ITEM->number.value = CCD_BIN_VERTICAL_ITEM->number.target = CCD_BIN_VERTICAL_ITEM->number.value = vertical_bin);
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
			return;
		}
	}
	CCD_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
	CCD_BIN_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, CCD_MODE_PROPERTY, NULL);
	indigo_update_property(device, CCD_BIN_PROPERTY, NULL);
}

static void ccd_frame_handler(indigo_device *device) {
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
					indigo_ccd_change_property(device, NULL, CCD_FRAME_PROPERTY);
					return;
				}
			}
		}
	}
	CCD_FRAME_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, CCD_FRAME_PROPERTY, NULL);
}

static void ccd_exposure_handler(indigo_device *device) {
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
	setup_exposure(device);
}

static void ccd_streaming_handler(indigo_device *device) {
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
	setup_exposure(device);
}

static void ccd_abort_exposure_handler(indigo_device *device) {
	HRESULT result;
	if (CCD_ABORT_EXPOSURE_ITEM->sw.value) {
		CCD_ABORT_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CCD_ABORT_EXPOSURE_PROPERTY, NULL);
		indigo_cancel_pending_handler(device, ccd_exposure_watchdog_handler);
		indigo_cancel_pending_handler(device, ccd_setup_binning_handler);
		indigo_cancel_pending_handler(device, ccd_setup_pull_handler);
		indigo_cancel_pending_handler(device, ccd_start_exposure_handler);
		PRIVATE_DATA->mode = -1;
		if (CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
			stop_video_mode(device);
		} else {
			// Single-exposure abort: cancel the pending trigger. The callback will fire with the discarded frame and clean up via aborting flag.
			PRIVATE_DATA->aborting = true;
			result = SDK_CALL(Trigger)(PRIVATE_DATA->handle, 0);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Trigger(0) -> %08x", result);
		}
	}
	indigo_ccd_change_property(device, NULL, CCD_ABORT_EXPOSURE_PROPERTY);
}

static void ccd_cooler_handler(indigo_device *device) {
	HRESULT result;
	result = SDK_CALL(put_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_TEC), CCD_COOLER_ON_ITEM->sw.value ? 1 : 0);
	if (result >= 0) {
		CCD_COOLER_PROPERTY->state = INDIGO_OK_STATE;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_Option(OPTION_TEC) -> %08x", result);
	} else {
		CCD_COOLER_PROPERTY->state = INDIGO_ALERT_STATE;
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "put_Option(OPTION_TEC) -> %08x", result);
	}
	indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
}

static void ccd_temperature_handler(indigo_device *device) {
	HRESULT result;
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
}

static void ccd_gain_handler(indigo_device *device) {
	HRESULT result;
	result = SDK_CALL(put_ExpoAGain)(PRIVATE_DATA->handle, (unsigned short)CCD_GAIN_ITEM->number.value);
	if (result < 0) {
		CCD_GAIN_PROPERTY->state = INDIGO_ALERT_STATE;
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "put_ExpoAGain(%d) -> %08x", (unsigned short)CCD_GAIN_ITEM->number.value, result);
		indigo_update_property(device, CCD_GAIN_PROPERTY, "Analog gain setting is not supported");
	} else {
		CCD_GAIN_PROPERTY->state = INDIGO_OK_STATE;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_ExpoAGain(%d) -> %08x", (unsigned short)CCD_GAIN_ITEM->number.value, result);
		indigo_update_property(device, CCD_GAIN_PROPERTY, NULL);
	}
	indigo_ccd_change_property(device, NULL, CCD_GAIN_PROPERTY);
}

static void ccd_offset_handler(indigo_device *device) {
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

static void ccd_x_advanced_handler(indigo_device *device) {
	HRESULT result;
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
	result = SDK_CALL(put_Speed)(PRIVATE_DATA->handle, (unsigned short)X_CCD_SPEED_ITEM->number.value);
	if (result < 0) {
		X_CCD_ADVANCED_PROPERTY->state = INDIGO_ALERT_STATE;
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "put_Speed(%d) -> %08x", (unsigned short)X_CCD_SPEED_ITEM->number.value, result);
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_Speed(%d) -> %08x", (unsigned short)X_CCD_SPEED_ITEM->number.value, result);
	}
	indigo_update_property(device, X_CCD_ADVANCED_PROPERTY, NULL);
}

static void ccd_x_fan_handler(indigo_device *device) {
	HRESULT result;
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
}

static void ccd_x_heater_handler(indigo_device *device) {
	HRESULT result;
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
}

static void ccd_x_conversion_gain_handler(indigo_device *device) {
	HRESULT result;
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
}

static void ccd_x_led_handler(indigo_device *device) {
	HRESULT result;
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
}

static void ccd_x_bin_mode_handler(indigo_device *device) {
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
}

static void ccd_config_handler(indigo_device *device) {
	if (CONFIG_SAVE_ITEM->sw.value) {
		indigo_save_property(device, NULL, X_CCD_ADVANCED_PROPERTY);
		indigo_save_property(device, NULL, X_CCD_CONVERSION_GAIN_PROPERTY);
		indigo_save_property(device, NULL, X_CCD_BIN_MODE_PROPERTY);
		indigo_save_property(device, NULL, X_CCD_LED_PROPERTY);
	}
	indigo_property *property = indigo_copy_property(NULL, CONFIG_PROPERTY);
	indigo_ccd_change_property(device, NULL, property);
	indigo_release_property(property);
	if (CONFIG_PROPERTY->state == INDIGO_BUSY_STATE) {
		CONFIG_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, CONFIG_PROPERTY, NULL);
	}
}

#pragma mark - Device API (ccd)

static indigo_result ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result ccd_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_ccd_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		// --------------------------------------------------------------------------------
		unsigned long long flags = PRIVATE_DATA->cam.model->flag;
		char name[128], label[128];
		INFO_PROPERTY->count = 8;
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, INDIGO_WCHAR_TO_CHAR(PRIVATE_DATA->cam.model->name));
		CCD_INFO_PIXEL_WIDTH_ITEM->number.value = PRIVATE_DATA->cam.model->xpixsz;
		CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = PRIVATE_DATA->cam.model->ypixsz;
		CCD_INFO_PIXEL_SIZE_ITEM->number.value = (CCD_INFO_PIXEL_WIDTH_ITEM->number.value + CCD_INFO_PIXEL_HEIGHT_ITEM->number.value) / 2.0;
		CCD_MODE_PROPERTY->perm = INDIGO_RW_PERM;
		CCD_MODE_PROPERTY->count = 0;
		CCD_INFO_WIDTH_ITEM->number.value = 0;
		CCD_INFO_HEIGHT_ITEM->number.value = 0;
		CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.min = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = 8;
		for (unsigned i = 0; i < PRIVATE_DATA->cam.model->preview; i++) {
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
					snprintf(name, sizeof(name), CCD_MODE_RAW08_ITEM_NAME, bin);
					snprintf(label, sizeof(label), "RAW 8 %dx%d", frame_width, frame_height);
					indigo_init_switch_item(CCD_MODE_ITEM + CCD_MODE_PROPERTY->count, name, label, false);
					CCD_MODE_PROPERTY->count++;
				}
				if (flags & SDK_DEF(FLAG_RAW10)) {
					snprintf(name, sizeof(name), CCD_MODE_RAW10_ITEM_NAME, bin);
					snprintf(label, sizeof(label), "RAW 10 %dx%d", frame_width, frame_height);
					indigo_init_switch_item(CCD_MODE_ITEM + CCD_MODE_PROPERTY->count, name, label, false);
					if (CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max < 10) {
						CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = 10;
					}
					CCD_MODE_PROPERTY->count++;
				}
				if (flags & SDK_DEF(FLAG_RAW12)) {
					snprintf(name, sizeof(name), CCD_MODE_RAW12_ITEM_NAME, bin);
					snprintf(label, sizeof(label), "RAW 12 %dx%d", frame_width, frame_height);
					indigo_init_switch_item(CCD_MODE_ITEM + CCD_MODE_PROPERTY->count, name, label, false);
					if (CCD_INFO_BITS_PER_PIXEL_ITEM->number.value < 12) {
						CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = 12;
					}
					CCD_MODE_PROPERTY->count++;
				}
				if (flags & SDK_DEF(FLAG_RAW14)) {
					snprintf(name, sizeof(name), CCD_MODE_RAW14_ITEM_NAME, bin);
					snprintf(label, sizeof(label), "RAW 14 %dx%d", frame_width, frame_height);
					indigo_init_switch_item(CCD_MODE_ITEM + CCD_MODE_PROPERTY->count, name, label, false);
					if (CCD_INFO_BITS_PER_PIXEL_ITEM->number.value < 14) {
						CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = 14;
					}
					CCD_MODE_PROPERTY->count++;
				}
				if (flags & SDK_DEF(FLAG_RAW16)) {
					snprintf(name, sizeof(name), CCD_MODE_RAW16_ITEM_NAME, bin);
					snprintf(label, sizeof(label), "RAW 16 %dx%d", frame_width, frame_height);
					indigo_init_switch_item(CCD_MODE_ITEM + CCD_MODE_PROPERTY->count, name, label, false);
					if (CCD_INFO_BITS_PER_PIXEL_ITEM->number.value < 16) {
						CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = 16;
					}
					CCD_MODE_PROPERTY->count++;
				}
				snprintf(name, sizeof(name), CCD_MODE_RGB08_ITEM_NAME, bin);
				snprintf(label, sizeof(label), "RGB 24 %dx%d", frame_width, frame_height);
				indigo_init_switch_item(CCD_MODE_ITEM + CCD_MODE_PROPERTY->count, name, label, false);
				CCD_MODE_PROPERTY->count++;
			} else {
				if (flags & SDK_DEF(FLAG_RAW8)) {
					snprintf(name, sizeof(name), CCD_MODE_MON08_ITEM_NAME, bin);
					snprintf(label, sizeof(label), "MON 8 %dx%d", frame_width, frame_height);
					indigo_init_switch_item(CCD_MODE_ITEM + CCD_MODE_PROPERTY->count, name, label, false);
					CCD_MODE_PROPERTY->count++;
				}
				if (flags & SDK_DEF(FLAG_RAW10)) {
					snprintf(name, sizeof(name), CCD_MODE_MON10_ITEM_NAME, bin);
					snprintf(label, sizeof(label), "MON 10 %dx%d", frame_width, frame_height);
					indigo_init_switch_item(CCD_MODE_ITEM + CCD_MODE_PROPERTY->count, name, label, false);
					if (CCD_INFO_BITS_PER_PIXEL_ITEM->number.value < 10) {
						CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = 10;
					}
					CCD_MODE_PROPERTY->count++;
				}
				if (flags & SDK_DEF(FLAG_RAW12)) {
					snprintf(name, sizeof(name), CCD_MODE_MON12_ITEM_NAME, bin);
					snprintf(label, sizeof(label), "MON 12 %dx%d", frame_width, frame_height);
					indigo_init_switch_item(CCD_MODE_ITEM + CCD_MODE_PROPERTY->count, name, label, false);
					if (CCD_INFO_BITS_PER_PIXEL_ITEM->number.value < 12) {
						CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = 12;
					}
					CCD_MODE_PROPERTY->count++;
				}
				if (flags & SDK_DEF(FLAG_RAW14)) {
					snprintf(name, sizeof(name), CCD_MODE_MON14_ITEM_NAME, bin);
					snprintf(label, sizeof(label), "MON 14 %dx%d", frame_width, frame_height);
					indigo_init_switch_item(CCD_MODE_ITEM + CCD_MODE_PROPERTY->count, name, label, false);
					if (CCD_INFO_BITS_PER_PIXEL_ITEM->number.value < 14) {
						CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = 14;
					}
					CCD_MODE_PROPERTY->count++;
				}
				if (flags & SDK_DEF(FLAG_RAW16)) {
					snprintf(name, sizeof(name), CCD_MODE_MON16_ITEM_NAME, bin);
					snprintf(label, sizeof(label), "MON 16 %d x %d", frame_width, frame_height);
					indigo_init_switch_item(CCD_MODE_ITEM + CCD_MODE_PROPERTY->count, name, label, false);
					if (CCD_INFO_BITS_PER_PIXEL_ITEM->number.value < 16) {
						CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = 16;
					}
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
		CCD_STREAMING_PROPERTY->hidden = false;
		CCD_IMAGE_FORMAT_PROPERTY->count = 7;
		CCD_GAIN_PROPERTY->hidden = false;
		X_CCD_ADVANCED_PROPERTY = indigo_init_number_property(NULL, device->name, X_CCD_ADVANCED_PROPERTY_NAME, CCD_ADVANCED_GROUP, "Advanced Settings", INDIGO_OK_STATE, INDIGO_RW_PERM, 9);
		if (X_CCD_ADVANCED_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_CCD_SPEED_ITEM, X_CCD_SPEED_ITEM_NAME, "Speed level", 0, PRIVATE_DATA->cam.model->maxspeed, 1, 0);
		indigo_init_number_item(X_CCD_CONTRAST_ITEM, X_CCD_CONTRAST_ITEM_NAME, "Contrast", SDK_DEF(CONTRAST_MIN), SDK_DEF(CONTRAST_MAX), 1, SDK_DEF(CONTRAST_DEF));
		indigo_init_number_item(X_CCD_HUE_ITEM, X_CCD_HUE_ITEM_NAME, "Hue", SDK_DEF(HUE_MIN), SDK_DEF(HUE_MAX), 1, SDK_DEF(HUE_DEF));
		indigo_init_number_item(X_CCD_SATURATION_ITEM, X_CCD_SATURATION_ITEM_NAME, "Saturation", SDK_DEF(SATURATION_MIN), SDK_DEF(SATURATION_MAX), 1, SDK_DEF(SATURATION_DEF));
		indigo_init_number_item(X_CCD_BRIGHTNESS_ITEM, X_CCD_BRIGHTNESS_ITEM_NAME, "Brightness", SDK_DEF(BRIGHTNESS_MIN), SDK_DEF(BRIGHTNESS_MAX), 1, SDK_DEF(BRIGHTNESS_DEF));
		indigo_init_number_item(X_CCD_GAMMA_ITEM, X_CCD_GAMMA_ITEM_NAME, "Gamma", SDK_DEF(GAMMA_MIN), SDK_DEF(GAMMA_MAX), 1, SDK_DEF(GAMMA_DEF));
		indigo_init_number_item(X_CCD_R_GAIN_ITEM, X_CCD_R_GAIN_ITEM_NAME, "Red gain", SDK_DEF(WBGAIN_MIN), SDK_DEF(WBGAIN_MAX), 1, SDK_DEF(WBGAIN_DEF));
		indigo_init_number_item(X_CCD_G_GAIN_ITEM, X_CCD_G_GAIN_ITEM_NAME, "Green gain", SDK_DEF(WBGAIN_MIN), SDK_DEF(WBGAIN_MAX), 1, SDK_DEF(WBGAIN_DEF));
		indigo_init_number_item(X_CCD_B_GAIN_ITEM, X_CCD_B_GAIN_ITEM_NAME, "Blue gain", SDK_DEF(WBGAIN_MIN), SDK_DEF(WBGAIN_MAX), 1, SDK_DEF(WBGAIN_DEF));
		if ((flags & SDK_DEF(FLAG_MONO))) {
			X_CCD_ADVANCED_PROPERTY->count = 1;  // only SPEED is valid for mono cams
		}
		if (flags & SDK_DEF(FLAG_FAN)) {
			X_CCD_FAN_PROPERTY = indigo_init_number_property(NULL, device->name, X_CCD_FAN_PROPERTY_NAME, CCD_ADVANCED_GROUP, "Fan control", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
			if (X_CCD_FAN_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			indigo_init_number_item(X_CCD_FAN_SPEED_ITEM, X_CCD_FAN_SPEED_ITEM_NAME, "Fan speed", 0, 0, 1, 0);
		}
		if (flags & SDK_DEF(FLAG_HEAT)) {
			X_CCD_HEATER_PROPERTY = indigo_init_number_property(NULL, device->name, X_CCD_HEATER_PROPERTY_NAME, CCD_ADVANCED_GROUP, "Window heater", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
			if (X_CCD_HEATER_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			indigo_init_number_item(X_CCD_HEATER_POWER_ITEM, X_CCD_HEATER_POWER_ITEM_NAME, "Power", 0, 0, 1, 0);
		}
		if (flags & SDK_DEF(FLAG_CG) || flags & SDK_DEF(FLAG_CGHDR)) {
			X_CCD_CONVERSION_GAIN_PROPERTY = indigo_init_switch_property(NULL, device->name, X_CCD_CONVERSION_GAIN_PROPERTY_NAME, CCD_ADVANCED_GROUP, "Conversion gain", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
			if (X_CCD_CONVERSION_GAIN_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			indigo_init_switch_item(X_CCD_CONVERSION_GAIN_LCG_ITEM, X_CCD_CONVERSION_GAIN_LCG_ITEM_NAME, "Low conversion gain", true);
			indigo_init_switch_item(X_CCD_CONVERSION_GAIN_HCG_ITEM, X_CCD_CONVERSION_GAIN_HCG_ITEM_NAME, "High conversion gain", false);
			indigo_init_switch_item(X_CCD_CONVERSION_GAIN_HDR_ITEM, X_CCD_CONVERSION_GAIN_HDR_ITEM_NAME, "High dynamic range", false);
			if (flags & SDK_DEF(FLAG_CGHDR)) {
				X_CCD_CONVERSION_GAIN_PROPERTY->count = 3;
			} else if (flags & SDK_DEF(FLAG_CG)) {
				X_CCD_CONVERSION_GAIN_PROPERTY->count = 2;
			}
		}
		X_CCD_BIN_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, X_CCD_BIN_MODE_PROPERTY_NAME, CCD_ADVANCED_GROUP, "Binning mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
		if (X_CCD_BIN_MODE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_CCD_BIN_MODE_SATURATE_ITEM, X_CCD_BIN_MODE_SATURATE_ITEM_NAME, "Sum and saturate", true);
		indigo_init_switch_item(X_CCD_BIN_MODE_EXPAND_ITEM, X_CCD_BIN_MODE_EXPAND_ITEM_NAME, "Sum and expand to 16-bits (10, 12 and 14-bit data)", false);
		indigo_init_switch_item(X_CCD_BIN_MODE_AVERAGE_ITEM, X_CCD_BIN_MODE_AVERAGE_ITEM_NAME, "Average", false);
		X_CCD_LED_PROPERTY = indigo_init_switch_property(NULL, device->name, X_CCD_LED_PROPERTY_NAME, CCD_ADVANCED_GROUP, "Camera LED control", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_CCD_LED_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_CCD_LED_ON_ITEM, X_CCD_LED_ON_ITEM_NAME, "On", true);
		indigo_init_switch_item(X_CCD_LED_OFF_ITEM, X_CCD_LED_OFF_ITEM_NAME, "Off", false);
		X_CCD_LED_PROPERTY->hidden = true;
		// --------------------------------------------------------------------------------
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (X_CCD_ADVANCED_PROPERTY && indigo_property_match(X_CCD_ADVANCED_PROPERTY, property)) {
			indigo_define_property(device, X_CCD_ADVANCED_PROPERTY, NULL);
		}
		if (X_CCD_FAN_PROPERTY && indigo_property_match(X_CCD_FAN_PROPERTY, property)) {
			indigo_define_property(device, X_CCD_FAN_PROPERTY, NULL);
		}
		if (X_CCD_HEATER_PROPERTY && indigo_property_match(X_CCD_HEATER_PROPERTY, property)) {
			indigo_define_property(device, X_CCD_HEATER_PROPERTY, NULL);
		}
		if (X_CCD_CONVERSION_GAIN_PROPERTY && indigo_property_match(X_CCD_CONVERSION_GAIN_PROPERTY, property)) {
			indigo_define_property(device, X_CCD_CONVERSION_GAIN_PROPERTY, NULL);
		}
		if (X_CCD_BIN_MODE_PROPERTY && indigo_property_match(X_CCD_BIN_MODE_PROPERTY, property)) {
			indigo_define_property(device, X_CCD_BIN_MODE_PROPERTY, NULL);
		}
		if (X_CCD_LED_PROPERTY && indigo_property_match(X_CCD_LED_PROPERTY, property)) {
			indigo_define_property(device, X_CCD_LED_PROPERTY, NULL);
		}
	}
	return indigo_ccd_enumerate_properties(device, client, property);
}

static indigo_result ccd_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (last_action != INDIGO_DRIVER_INIT || PRIVATE_DATA->removing) {
		return INDIGO_OK;
	}
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION -> CCD_INFO, CCD_COOLER, CCD_TEMPERATURE
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			indigo_queue_add(driver_queue, device, INDIGO_TASK_PRIORITY_NORMAL, 0, ccd_connection_handler, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CONFIG_PROPERTY, ccd_config_handler);
		return INDIGO_OK;
	} else if (!IS_CONNECTED || CONNECTION_PROPERTY->state != INDIGO_OK_STATE) {
		return indigo_ccd_change_property(device, client, property);
	} else if (indigo_property_match_changeable(CCD_MODE_PROPERTY, property)) {
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE || CCD_MODE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_BIN_PROPERTY->state == INDIGO_BUSY_STATE || CCD_FRAME_PROPERTY->state == INDIGO_BUSY_STATE) {
			return INDIGO_OK;
		}
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_MODE_PROPERTY, ccd_mode_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_BIN_PROPERTY, property)) {
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE || CCD_MODE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_BIN_PROPERTY->state == INDIGO_BUSY_STATE || CCD_FRAME_PROPERTY->state == INDIGO_BUSY_STATE) {
			return INDIGO_OK;
		}
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(CCD_BIN_PROPERTY, ccd_bin_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_FRAME_PROPERTY, property)) {
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE || CCD_MODE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_BIN_PROPERTY->state == INDIGO_BUSY_STATE || CCD_FRAME_PROPERTY->state == INDIGO_BUSY_STATE) {
			return INDIGO_OK;
		}
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_FRAME_PROPERTY, ccd_frame_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_EXPOSURE_PROPERTY, property)) {
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
			return INDIGO_OK;
		}
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_EXPOSURE_PROPERTY, ccd_exposure_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_STREAMING_PROPERTY, property)) {
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
			return INDIGO_OK;
		}
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_STREAMING_PROPERTY, ccd_streaming_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_ABORT_EXPOSURE_PROPERTY, ccd_abort_exposure_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_COOLER_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_COOLER_PROPERTY, ccd_cooler_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_TEMPERATURE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE_ANYTIME(CCD_TEMPERATURE_PROPERTY, ccd_temperature_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_GAIN_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_GAIN_PROPERTY, ccd_gain_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_OFFSET_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_OFFSET_PROPERTY, ccd_offset_handler);
		return INDIGO_OK;
	} else if (X_CCD_ADVANCED_PROPERTY && indigo_property_match_defined(X_CCD_ADVANCED_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_CCD_ADVANCED_PROPERTY, ccd_x_advanced_handler);
		return INDIGO_OK;
	} else if (X_CCD_FAN_PROPERTY && indigo_property_match_defined(X_CCD_FAN_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_CCD_FAN_PROPERTY, ccd_x_fan_handler);
		return INDIGO_OK;
	} else if (X_CCD_HEATER_PROPERTY && indigo_property_match_defined(X_CCD_HEATER_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_CCD_HEATER_PROPERTY, ccd_x_heater_handler);
		return INDIGO_OK;
	} else if (X_CCD_CONVERSION_GAIN_PROPERTY && indigo_property_match_defined(X_CCD_CONVERSION_GAIN_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_CCD_CONVERSION_GAIN_PROPERTY, ccd_x_conversion_gain_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_defined(X_CCD_LED_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_CCD_LED_PROPERTY, ccd_x_led_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_defined(X_CCD_BIN_MODE_PROPERTY, property)) {
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
			return INDIGO_OK;
		}
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_CCD_BIN_MODE_PROPERTY, ccd_x_bin_mode_handler);
		return INDIGO_OK;
	}
	return indigo_ccd_change_property(device, client, property);
}

static indigo_result ccd_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_cancel_pending_handlers(device);
	if (PRIVATE_DATA->count > 0) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		ccd_connection_handler(device);
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

#pragma mark - High level code (guider)

static void guider_connection_handler(indigo_device *device) {
	indigo_cancel_pending_handlers(device);
	indigo_lock_master_device(device);
	// Cancelled requests must not leave properties BUSY in the next session.
	indigo_property *properties[] = {
		GUIDER_GUIDE_DEC_PROPERTY, GUIDER_GUIDE_RA_PROPERTY
	};
	for (unsigned i = 0; i < sizeof(properties) / sizeof(properties[0]); i++) {
		if (properties[i] && properties[i]->state == INDIGO_BUSY_STATE) {
			properties[i]->state = INDIGO_ALERT_STATE;
		}
	}
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (PRIVATE_DATA->count++ == 0) {
			if (indigo_try_global_lock(device) != INDIGO_OK) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_try_global_lock(): failed to get lock.");
			} else {
				char id[66];
				sprintf(id, "@%s", INDIGO_WCHAR_TO_CHAR(PRIVATE_DATA->cam.id));
				PRIVATE_DATA->handle = SDK_CALL(Open)(INDIGO_CHAR_TO_WCHAR(id));
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Open(%s) -> %p", id, PRIVATE_DATA->handle);
				if (PRIVATE_DATA->handle == NULL) {
					indigo_global_unlock(device);
				}
			}
		}
		if (PRIVATE_DATA->handle) {
			HRESULT result = SDK_CALL(put_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_CALLBACK_THREAD), 1);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_Option(OPTION_CALLBACK_THREAD, 1) -> %08x", result);
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
			PRIVATE_DATA->count--;
		}
	} else {
		if (--PRIVATE_DATA->count == 0) {
			if (PRIVATE_DATA->handle != NULL) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Closing camera");
				SDK_CALL(Close)(PRIVATE_DATA->handle);
				indigo_global_unlock(device);
			}
			PRIVATE_DATA->handle = NULL;
		}
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_guider_change_property(device, NULL, CONNECTION_PROPERTY);
	indigo_unlock_master_device(device);
}

static void guider_guide_dec_handler(indigo_device *device) {
	indigo_cancel_pending_handler(device, guider_guide_dec_handler);
	HRESULT result = 0;
	unsigned pulse_length = 0;
	indigo_cancel_pending_handler(device, guider_guide_dec_finalizer);
	if (GUIDER_GUIDE_NORTH_ITEM->number.value > 0) {
		pulse_length = (unsigned)GUIDER_GUIDE_NORTH_ITEM->number.value;
		result = SDK_CALL(ST4PlusGuide)(PRIVATE_DATA->handle, 0, pulse_length);
	} else if (GUIDER_GUIDE_SOUTH_ITEM->number.value > 0) {
		pulse_length = (unsigned)GUIDER_GUIDE_SOUTH_ITEM->number.value;
		result = SDK_CALL(ST4PlusGuide)(PRIVATE_DATA->handle, 1, pulse_length);
	}
	GUIDER_GUIDE_DEC_PROPERTY->state = SUCCEEDED(result) ? INDIGO_BUSY_STATE : INDIGO_ALERT_STATE;
	indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
	if (GUIDER_GUIDE_DEC_PROPERTY->state == INDIGO_BUSY_STATE) {
		indigo_execute_handler_in(device, pulse_length / 1000.0, guider_guide_dec_finalizer);
	}
}

static void guider_guide_ra_handler(indigo_device *device) {
	indigo_cancel_pending_handler(device, guider_guide_ra_handler);
	HRESULT result = 0;
	unsigned pulse_length = 0;
	indigo_cancel_pending_handler(device, guider_guide_ra_finalizer);
	if (GUIDER_GUIDE_EAST_ITEM->number.value > 0) {
		pulse_length = (unsigned)GUIDER_GUIDE_EAST_ITEM->number.value;
		result = SDK_CALL(ST4PlusGuide)(PRIVATE_DATA->handle, 2, pulse_length);
	} else if (GUIDER_GUIDE_WEST_ITEM->number.value > 0) {
		pulse_length = (unsigned)GUIDER_GUIDE_WEST_ITEM->number.value;
		result = SDK_CALL(ST4PlusGuide)(PRIVATE_DATA->handle, 3, pulse_length);
	}
	GUIDER_GUIDE_RA_PROPERTY->state = SUCCEEDED(result) ? INDIGO_BUSY_STATE : INDIGO_ALERT_STATE;
	indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
	if (GUIDER_GUIDE_RA_PROPERTY->state == INDIGO_BUSY_STATE) {
		indigo_execute_handler_in(device, pulse_length / 1000.0, guider_guide_ra_finalizer);
	}
}

#pragma mark - Device API (guider)

static indigo_result guider_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_guider_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		INFO_PROPERTY->count = 8;
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, INDIGO_WCHAR_TO_CHAR(PRIVATE_DATA->cam.model->name));
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (last_action != INDIGO_DRIVER_INIT || PRIVATE_DATA->removing) {
		return INDIGO_OK;
	}
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			indigo_queue_add(driver_queue, device, INDIGO_TASK_PRIORITY_NORMAL, 0, guider_connection_handler, NULL);
		}
		return INDIGO_OK;
	} else if (!IS_CONNECTED || CONNECTION_PROPERTY->state != INDIGO_OK_STATE) {
		return indigo_guider_change_property(device, client, property);
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE_ANYTIME(GUIDER_GUIDE_DEC_PROPERTY, guider_guide_dec_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_RA_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE_ANYTIME(GUIDER_GUIDE_RA_PROPERTY, guider_guide_ra_handler);
		return INDIGO_OK;
	}
	return indigo_guider_change_property(device, client, property);
}

static indigo_result guider_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_cancel_pending_handlers(device);
	// A queued disconnect still owns a reference; a queued connect does not.
	if (CONNECTION_CONNECTED_ITEM->sw.value != (CONNECTION_PROPERTY->state == INDIGO_BUSY_STATE)) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		guider_connection_handler(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_guider_detach(device);
}

#pragma mark - High level code (wheel)

static void wheel_connection_handler(indigo_device *device) {
	indigo_cancel_pending_handlers(device);
	indigo_lock_master_device(device);
	// Cancelled requests must not leave properties BUSY in the next session.
	indigo_property *properties[] = {
		WHEEL_SLOT_PROPERTY, X_CALIBRATE_PROPERTY, X_WHEEL_MODEL_PROPERTY, CONFIG_PROPERTY
	};
	for (unsigned i = 0; i < sizeof(properties) / sizeof(properties[0]); i++) {
		if (properties[i] && properties[i]->state == INDIGO_BUSY_STATE) {
			properties[i]->state = INDIGO_ALERT_STATE;
		}
	}
	CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (PRIVATE_DATA->count++ == 0) {
			if (indigo_try_global_lock(device) != INDIGO_OK) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_try_global_lock(): failed to get lock.");
			} else {
				char id[66];
				sprintf(id, "@%s", INDIGO_WCHAR_TO_CHAR(PRIVATE_DATA->cam.id));
				PRIVATE_DATA->handle = SDK_CALL(Open)(INDIGO_CHAR_TO_WCHAR(id));
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Open(%s) -> %p", id, PRIVATE_DATA->handle);
				if (PRIVATE_DATA->handle == NULL) {
					indigo_global_unlock(device);
				}
			}
		}
		if (PRIVATE_DATA->handle) {
			HRESULT result = SDK_CALL(get_HwVersion)(PRIVATE_DATA->handle, INFO_DEVICE_HW_REVISION_ITEM->text.value);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "get_HwVersion() -> %08x", result);
			result = SDK_CALL(get_FwVersion)(PRIVATE_DATA->handle, INFO_DEVICE_FW_REVISION_ITEM->text.value);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "get_FwVersion() -> %08x", result);
			indigo_update_property(device, INFO_PROPERTY, NULL);
			indigo_define_property(device, X_CALIBRATE_PROPERTY, NULL);
			set_wheel_positions(device);
			// This is a hack! We need to reset to some position because sometimes after reconnect
			// the the state remains "moving" forever although it is not moving. However it tries
			// to set slot 1 at every connect, so this hack does not change anything.
			int slot = 0 + (1 << 8);  // slot 1 using closest approach
			SDK_CALL(put_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_FILTERWHEEL_POSITION), slot);
			CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_execute_handler_in(device, 1, wheel_connection_finalizer);
			indigo_unlock_master_device(device);
			return;
		} else {
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			PRIVATE_DATA->count--;
		}
	} else {
		indigo_delete_property(device, X_CALIBRATE_PROPERTY, NULL);
		if (--PRIVATE_DATA->count == 0) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Closing wheel");
			SDK_CALL(Close)(PRIVATE_DATA->handle);
			indigo_global_unlock(device);
			PRIVATE_DATA->handle = NULL;
		}
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_wheel_change_property(device, NULL, CONNECTION_PROPERTY);
	indigo_unlock_master_device(device);
}

static void wheel_slot_handler(indigo_device *device) {
	if (WHEEL_SLOT_ITEM->number.value < 1 || WHEEL_SLOT_ITEM->number.value > WHEEL_SLOT_ITEM->number.max) {
		WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
	} else if (WHEEL_SLOT_ITEM->number.value == PRIVATE_DATA->current_slot) {
		WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		WHEEL_SLOT_PROPERTY->state = INDIGO_BUSY_STATE;
		PRIVATE_DATA->target_slot = (int)WHEEL_SLOT_ITEM->number.value;
		WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->current_slot;
		int slot = ((int)WHEEL_SLOT_ITEM->number.target - 1) + (1 << 8);
		HRESULT result = SDK_CALL(put_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_FILTERWHEEL_POSITION), slot);
		if (FAILED(result)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "put_Option(OPTION_FILTERWHEEL_POSITION, %d) -> %08x", slot, result);
			SDK_CALL(get_Option)(PRIVATE_DATA->handle, SDK_DEF(OPTION_FILTERWHEEL_POSITION), &PRIVATE_DATA->current_slot);
			WHEEL_SLOT_ITEM->number.value = ++PRIVATE_DATA->current_slot;
			WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "put_Option(OPTION_FILTERWHEEL_POSITION, %d) -> %08x", slot, result);
			indigo_cancel_pending_handler(device, wheel_move_finalizer);
			indigo_execute_handler_in(device, 0.5, wheel_move_finalizer);
		}
	}
	indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
}

static void wheel_x_calibrate_handler(indigo_device *device) {
	if (X_CALIBRATE_START_ITEM->sw.value) {
		X_CALIBRATE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_CALIBRATE_PROPERTY, "Calibration started");
		WHEEL_SLOT_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
		indigo_execute_handler_in(device, 0.5, wheel_calibrate_handler);
	} else {
		X_CALIBRATE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, X_CALIBRATE_PROPERTY, NULL);
	}
}

static void wheel_x_wheel_model_handler(indigo_device *device) {
	if (IS_CONNECTED && CONNECTION_PROPERTY->state == INDIGO_OK_STATE) {
		set_wheel_positions(device);
		indigo_delete_property(device, WHEEL_SLOT_PROPERTY, NULL);
		indigo_delete_property(device, WHEEL_SLOT_NAME_PROPERTY, NULL);
		indigo_delete_property(device, WHEEL_SLOT_OFFSET_PROPERTY, NULL);
		indigo_define_property(device, WHEEL_SLOT_PROPERTY, NULL);
		indigo_define_property(device, WHEEL_SLOT_NAME_PROPERTY, NULL);
		indigo_define_property(device, WHEEL_SLOT_OFFSET_PROPERTY, NULL);
	}
	X_WHEEL_MODEL_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, X_WHEEL_MODEL_PROPERTY, NULL);
	indigo_wheel_change_property(device, NULL, X_WHEEL_MODEL_PROPERTY);
}

static void wheel_config_handler(indigo_device *device) {
	if (CONFIG_SAVE_ITEM->sw.value) {
		indigo_save_property(device, NULL, X_WHEEL_MODEL_PROPERTY);
	}
	indigo_property *property = indigo_copy_property(NULL, CONFIG_PROPERTY);
	indigo_wheel_change_property(device, NULL, property);
	indigo_release_property(property);
	if (CONFIG_PROPERTY->state == INDIGO_BUSY_STATE) {
		CONFIG_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, CONFIG_PROPERTY, NULL);
	}
}

#pragma mark - Device API (wheel)

static indigo_result wheel_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result wheel_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_wheel_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		INFO_PROPERTY->count = 7;
		// --------------------------------------------------------------------------------- X_CALIBRATE
		X_CALIBRATE_PROPERTY = indigo_init_switch_property(NULL, device->name, X_CALIBRATE_PROPERTY_NAME, ADVANCED_GROUP, "Calibrate filter wheel", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
		if (X_CALIBRATE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_CALIBRATE_START_ITEM, X_CALIBRATE_START_ITEM_NAME, "Start", false);
		// --------------------------------------------------------------------------------- X_WHEEL_MODEL
		X_WHEEL_MODEL_PROPERTY = indigo_init_switch_property(NULL, device->name, X_WHEEL_MODEL_PROPERTY_NAME, MAIN_GROUP, "Device Model", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
		if (X_WHEEL_MODEL_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_WHEEL_MODEL_5_POSITION_ITEM, X_WHEEL_MODEL_5_POSITION_ITEM_NAME, "5 positions Filter wheel", false);
		indigo_init_switch_item(X_WHEEL_MODEL_7_POSITION_ITEM, X_WHEEL_MODEL_7_POSITION_ITEM_NAME, "7 positions Filter wheel", true);
		indigo_init_switch_item(X_WHEEL_MODEL_8_POSITION_ITEM, X_WHEEL_MODEL_8_POSITION_ITEM_NAME, "8 positions Filter wheel", false);
		// --------------------------------------------------------------------------
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return wheel_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result wheel_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	INDIGO_DEFINE_MATCHING_PROPERTY(X_WHEEL_MODEL_PROPERTY);
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_CALIBRATE_PROPERTY);
	}
	return indigo_wheel_enumerate_properties(device, client, property);
}

static indigo_result wheel_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (last_action != INDIGO_DRIVER_INIT || PRIVATE_DATA->removing) {
		return INDIGO_OK;
	}
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			indigo_queue_add(driver_queue, device, INDIGO_TASK_PRIORITY_NORMAL, 0, wheel_connection_handler, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CONFIG_PROPERTY, wheel_config_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_WHEEL_MODEL_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_WHEEL_MODEL_PROPERTY, wheel_x_wheel_model_handler);
		return INDIGO_OK;
	} else if (!IS_CONNECTED || CONNECTION_PROPERTY->state != INDIGO_OK_STATE) {
		return indigo_wheel_change_property(device, client, property);
	} else if (indigo_property_match_changeable(WHEEL_SLOT_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(WHEEL_SLOT_PROPERTY, wheel_slot_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_CALIBRATE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_CALIBRATE_PROPERTY, wheel_x_calibrate_handler);
		return INDIGO_OK;
	}
	// --------------------------------------------------------------------------------
	return indigo_wheel_change_property(device, client, property);
}

static indigo_result wheel_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_cancel_pending_handlers(device);
	if (PRIVATE_DATA->count > 0) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		wheel_connection_handler(device);
	}
	indigo_release_property(X_CALIBRATE_PROPERTY);
	indigo_release_property(X_WHEEL_MODEL_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_wheel_detach(device);
}

#pragma mark - High level code (focuser)

static void focuser_connection_handler(indigo_device *device) {
	indigo_cancel_pending_handlers(device);
	indigo_lock_master_device(device);
	// Cancelled requests must not leave properties BUSY in the next session.
	indigo_property *properties[] = {
		FOCUSER_REVERSE_MOTION_PROPERTY, FOCUSER_POSITION_PROPERTY, FOCUSER_LIMITS_PROPERTY, FOCUSER_BACKLASH_PROPERTY,
		FOCUSER_STEPS_PROPERTY, FOCUSER_ABORT_MOTION_PROPERTY, FOCUSER_COMPENSATION_PROPERTY, X_BEEP_PROPERTY,
		FOCUSER_MODE_PROPERTY, CONFIG_PROPERTY
	};
	for (unsigned i = 0; i < sizeof(properties) / sizeof(properties[0]); i++) {
		if (properties[i] && properties[i]->state == INDIGO_BUSY_STATE) {
			properties[i]->state = INDIGO_ALERT_STATE;
		}
	}
	CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (PRIVATE_DATA->count++ == 0) {
			if (indigo_try_global_lock(device) != INDIGO_OK) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_try_global_lock(): failed to get lock.");
			} else {
				char id[66];
				sprintf(id, "@%s", INDIGO_WCHAR_TO_CHAR(PRIVATE_DATA->cam.id));
				PRIVATE_DATA->handle = SDK_CALL(Open)(INDIGO_CHAR_TO_WCHAR(id));
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Open(%s) -> %p", id, PRIVATE_DATA->handle);
				if (PRIVATE_DATA->handle == NULL) {
					indigo_global_unlock(device);
				}
			}
		}
		if (PRIVATE_DATA->handle) {
			HRESULT result = SDK_CALL(get_HwVersion)(PRIVATE_DATA->handle, INFO_DEVICE_HW_REVISION_ITEM->text.value);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "get_HwVersion() -> %08x", result);
			result = SDK_CALL(get_FwVersion)(PRIVATE_DATA->handle, INFO_DEVICE_FW_REVISION_ITEM->text.value);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "get_FwVersion() -> %08x", result);
			indigo_update_property(device, INFO_PROPERTY, NULL);
			int value = 0;
			HRESULT res = (SDK_CALL(AAF)(PRIVATE_DATA->handle, SDK_DEF(AAF_RANGEMAX), 0, &value));
			if (FAILED(res)) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "AAF(AAF_RANGEMAX) -> %08x (value = %d) (failed)", res, value);
			} else {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "AAF(AAF_RANGEMAX) -> %08x (value = %d)", res, value);
			}
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
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_define_property(device, X_BEEP_PROPERTY, NULL);
			PRIVATE_DATA->prev_temp = -273;  /* we do not have previous temperature reading */
			indigo_cancel_pending_handler(device, focuser_move_finalizer);
			indigo_execute_handler_in(device, 0.5, focuser_move_finalizer);
			indigo_execute_handler_in(device, 0.1, focuser_temperature_handler);
		} else {
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			PRIVATE_DATA->count--;
		}
	} else {
		indigo_delete_property(device, X_BEEP_PROPERTY, NULL);
		if (--PRIVATE_DATA->count == 0) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Closing focuser");
			SDK_CALL(Close)(PRIVATE_DATA->handle);
			indigo_global_unlock(device);
			PRIVATE_DATA->handle = NULL;
		}
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
	indigo_unlock_master_device(device);
}

static void focuser_reverse_motion_handler(indigo_device *device) {
	HRESULT res;
	FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	int value = FOCUSER_REVERSE_MOTION_DISABLED_ITEM->sw.value ? 1 : 0;
	res = (SDK_CALL(AAF)(PRIVATE_DATA->handle, SDK_DEF(AAF_SETDIRECTION), value, NULL));
	if (FAILED(res)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "AAF(AAF_SETDIRECTION) -> %08x (value = %d) (failed)", res, value);
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "AAF(AAF_SETDIRECTION) -> %08x (value = %d)", res, value);
		FOCUSER_REVERSE_MOTION_DISABLED_ITEM->sw.value = (value > 0);
		FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value = !FOCUSER_REVERSE_MOTION_DISABLED_ITEM->sw.value;
	}
	indigo_update_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
}

static void focuser_position_handler(indigo_device *device) {
	HRESULT res;
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
		PRIVATE_DATA->target_position = (int)FOCUSER_POSITION_ITEM->number.target;
		FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		if (FOCUSER_ON_POSITION_SET_GOTO_ITEM->sw.value) { /* GOTO POSITION */
			res = (SDK_CALL(AAF)(PRIVATE_DATA->handle, SDK_DEF(AAF_SETPOSITION), PRIVATE_DATA->target_position, NULL));
			if (FAILED(res)) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "AAF(AAF_SETPOSITION) -> %08x (value = %d) (failed)", res, PRIVATE_DATA->target_position);
			} else {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "AAF(AAF_SETPOSITION) -> %08x (value = %d)", res, PRIVATE_DATA->target_position);
			}
			indigo_cancel_pending_handler(device, focuser_move_finalizer);
			indigo_execute_handler_in(device, 0.5, focuser_move_finalizer);
		} else { /* SYNC POSITION */
			FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
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
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		}
	}
}

static void focuser_limits_handler(indigo_device *device) {
	HRESULT res;
	FOCUSER_LIMITS_PROPERTY->state = INDIGO_OK_STATE;
	int max_position = PRIVATE_DATA->max_position;
	PRIVATE_DATA->max_position = (int)FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target;
	res = (SDK_CALL(AAF)(PRIVATE_DATA->handle, SDK_DEF(AAF_SETMAXSTEP), PRIVATE_DATA->max_position, NULL));
	if (FAILED(res)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "AAF(AAF_SETMAXSTEP) -> %08x (value = %d) (failed)", res, PRIVATE_DATA->max_position);
		FOCUSER_LIMITS_PROPERTY->state = INDIGO_ALERT_STATE;
		PRIVATE_DATA->max_position = max_position;
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "AAF(AAF_SETMAXSTEP) -> %08x (value = %d)", res, PRIVATE_DATA->max_position);
	}
	FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = (double)PRIVATE_DATA->max_position;
	indigo_update_property(device, FOCUSER_LIMITS_PROPERTY, NULL);
}

static void focuser_backlash_handler(indigo_device *device) {
	HRESULT res;
	FOCUSER_BACKLASH_PROPERTY->state = INDIGO_OK_STATE;
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
	indigo_update_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
}

static void focuser_steps_handler(indigo_device *device) {
	HRESULT res;
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
		res = (SDK_CALL(AAF)(PRIVATE_DATA->handle, SDK_DEF(AAF_GETPOSITION), 0, &PRIVATE_DATA->current_position));
		if (FAILED(res)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "AAF(AAF_GETPOSITION) -> %08x (value = %d) (failed)", res, PRIVATE_DATA->current_position);
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "AAF(AAF_GETPOSITION) -> %08x (value = %d)", res, PRIVATE_DATA->current_position);
			FOCUSER_POSITION_ITEM->number.value = (double)PRIVATE_DATA->current_position;
		}
		if (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value) {
			PRIVATE_DATA->target_position = PRIVATE_DATA->current_position - (int)FOCUSER_STEPS_ITEM->number.value;
		} else {
			PRIVATE_DATA->target_position = PRIVATE_DATA->current_position + (int)FOCUSER_STEPS_ITEM->number.value;
		}
		/* Make sure we do not attempt to go beyond the limits */
		if (FOCUSER_POSITION_ITEM->number.max < PRIVATE_DATA->target_position) {
			PRIVATE_DATA->target_position = (int)FOCUSER_POSITION_ITEM->number.max;
		} else if (FOCUSER_POSITION_ITEM->number.min > PRIVATE_DATA->target_position) {
			PRIVATE_DATA->target_position = (int)FOCUSER_POSITION_ITEM->number.min;
		}
		FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
		res = (SDK_CALL(AAF)(PRIVATE_DATA->handle, SDK_DEF(AAF_SETPOSITION), PRIVATE_DATA->target_position, NULL));
		if (FAILED(res)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "AAF(AAF_SETPOSITION) -> %08x (value = %d) (failed)", res, PRIVATE_DATA->target_position);
			FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "AAF(AAF_SETPOSITION) -> %08x (value = %d)", res, PRIVATE_DATA->target_position);
		}
		indigo_cancel_pending_handler(device, focuser_move_finalizer);
		indigo_execute_handler_in(device, 0.5, focuser_move_finalizer);
	}
}

static void focuser_abort_motion_handler(indigo_device *device) {
	HRESULT res;
	FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
	FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
	FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	indigo_cancel_pending_handler(device, focuser_move_finalizer);
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
	FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
	FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
}

static void focuser_compensation_handler(indigo_device *device) {
	FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, FOCUSER_COMPENSATION_PROPERTY, NULL);
}

static void focuser_x_beep_handler(indigo_device *device) {
	HRESULT res;
	X_BEEP_PROPERTY->state = INDIGO_OK_STATE;
	res = (SDK_CALL(AAF)(PRIVATE_DATA->handle, SDK_DEF(AAF_SETBUZZER), X_BEEP_ON_ITEM->sw.value, NULL));
	if (FAILED(res)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "AAF(AAF_SETBUZZER) -> %08x (value = %d) (failed)", res, X_BEEP_ON_ITEM->sw.value);
		X_BEEP_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "AAF(AAF_SETBUZZER) -> %08x (value = %d)", res, X_BEEP_ON_ITEM->sw.value);
	}
	indigo_update_property(device, X_BEEP_PROPERTY, NULL);
}

static void focuser_mode_handler(indigo_device *device) {
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
}

static void focuser_config_handler(indigo_device *device) {
	if (CONFIG_SAVE_ITEM->sw.value) {
		//indigo_save_property(device, NULL, EAF_BEEP_PROPERTY);
	}
	indigo_property *property = indigo_copy_property(NULL, CONFIG_PROPERTY);
	indigo_focuser_change_property(device, NULL, property);
	indigo_release_property(property);
	if (CONFIG_PROPERTY->state == INDIGO_BUSY_STATE) {
		CONFIG_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, CONFIG_PROPERTY, NULL);
	}
}

#pragma mark - Device API (focuser)

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result focuser_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
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
		if (X_BEEP_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_BEEP_ON_ITEM, X_BEEP_ON_ITEM_NAME, "On", false);
		indigo_init_switch_item(X_BEEP_OFF_ITEM, X_BEEP_OFF_ITEM_NAME, "Off", true);
		// --------------------------------------------------------------------------
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_BEEP_PROPERTY);
	}
	return indigo_focuser_enumerate_properties(device, client, property);
}

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (last_action != INDIGO_DRIVER_INIT || PRIVATE_DATA->removing) {
		return INDIGO_OK;
	}
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			indigo_queue_add(driver_queue, device, INDIGO_TASK_PRIORITY_NORMAL, 0, focuser_connection_handler, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CONFIG_PROPERTY, focuser_config_handler);
		return INDIGO_OK;
	} else if (!IS_CONNECTED || CONNECTION_PROPERTY->state != INDIGO_OK_STATE) {
		return indigo_focuser_change_property(device, client, property);
	} else if (indigo_property_match_changeable(FOCUSER_REVERSE_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_REVERSE_MOTION_PROPERTY, focuser_reverse_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_POSITION_PROPERTY, property)) {
		if (FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE) {
			return INDIGO_OK;
		}
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_POSITION_PROPERTY, focuser_position_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_LIMITS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_LIMITS_PROPERTY, focuser_limits_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_BACKLASH_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_BACKLASH_PROPERTY, focuser_backlash_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_STEPS_PROPERTY, focuser_steps_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_ABORT_MOTION_PROPERTY, focuser_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_COMPENSATION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_COMPENSATION_PROPERTY, focuser_compensation_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_BEEP_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_BEEP_PROPERTY, focuser_x_beep_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_MODE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_MODE_PROPERTY, focuser_mode_handler);
		return INDIGO_OK;
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_cancel_pending_handlers(device);
	if (PRIVATE_DATA->count > 0) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_connection_handler(device);
	}
	indigo_release_property(X_BEEP_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

#pragma mark - Device templates

static indigo_device ccd_template = INDIGO_DEVICE_INITIALIZER("", ccd_attach, ccd_enumerate_properties, ccd_change_property, NULL, ccd_detach);
static indigo_device guider_template = INDIGO_DEVICE_INITIALIZER("", guider_attach, indigo_guider_enumerate_properties, guider_change_property, NULL, guider_detach);
static indigo_device wheel_template = INDIGO_DEVICE_INITIALIZER("", wheel_attach, wheel_enumerate_properties, wheel_change_property, NULL, wheel_detach);
static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER("", focuser_attach, focuser_enumerate_properties, focuser_change_property, NULL, focuser_detach);

#pragma mark - Hot-plug code

static indigo_device *devices[SDK_DEF(MAX)];

static void process_unplug_event_handler(indigo_device *removed_device);

static void process_plug_event_handler(indigo_device *unused) {
	process_unplug_event_handler(NULL);
	SDK_TYPE(DeviceV2) cams[SDK_DEF(MAX)];
	int count = SDK_CALL(EnumV2)(cams);
#ifdef TOUPTEK
	count += OEMCamEnum(&cams[count], TOUPCAM_MAX - count);
#endif
	indigo_device *templates[] = { &ccd_template, &wheel_template, &focuser_template };
	unsigned long long flags[] = {
		SDK_DEF(FLAG_CMOS) | SDK_DEF(FLAG_CCD_PROGRESSIVE) | SDK_DEF(FLAG_CCD_INTERLACED),
		SDK_DEF(FLAG_FILTERWHEEL), SDK_DEF(FLAG_AUTOFOCUSER)
	};
	for (int j = 0; j < count; j++) {
		SDK_TYPE(DeviceV2) *cam = &cams[j];
		if (cam->model == NULL) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "No SDK model for %s", INDIGO_WCHAR_TO_CHAR(cam->id));
			continue;
		}
		for (unsigned type = 0; type < sizeof(templates) / sizeof(templates[0]); type++) {
			if (!(cam->model->flag & flags[type])) {
				continue;
			}
			indigo_device *device_template = templates[type];
			bool found = false;
			int slot = -1;
			for (int i = 0; i < SDK_DEF(MAX); i++) {
				indigo_device *device = devices[i];
				if (device) {
					if (device->attach == device_template->attach && !strcmp(INDIGO_WCHAR_TO_CHAR(PRIVATE_DATA->cam.id), INDIGO_WCHAR_TO_CHAR(cam->id))) {
						found = true;
						break;
					}
				} else if (slot < 0) {
					slot = i;
				}
			}
			if (found) {
				continue;
			}
			if (slot < 0) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "No free device slot for %s", INDIGO_WCHAR_TO_CHAR(cam->id));
				continue;
			}
			char name[INDIGO_NAME_SIZE];
			char guider_name[INDIGO_NAME_SIZE];
			snprintf(name, sizeof(name), "%s %s", CAMERA_NAME_PREFIX, INDIGO_WCHAR_TO_CHAR(cam->displayname));
			snprintf(guider_name, sizeof(guider_name), "%s %s (guider)", CAMERA_NAME_PREFIX, INDIGO_WCHAR_TO_CHAR(cam->displayname));
#ifdef INDIGO_MACOS
			if (device_template == &ccd_template) {
				char camera_id[16] = { 0 };
				SDK_HANDLE handle = SDK_CALL(Open)(cam->id);
				if (handle != NULL) {
					char serial[33] = { 0 };
					SDK_CALL(get_SerialNumber)(handle, serial);
					SDK_CALL(Close)(handle);
					size_t serial_length = strlen(serial);
					strcpy(camera_id, serial + (serial_length > 6 ? serial_length - 6 : 0));
				} else {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Can not get serial number of Camera %s #%s", cam->displayname, cam->id);
				}
				snprintf(name, sizeof(name), "%s %s #%s", CAMERA_NAME_PREFIX, cam->displayname, camera_id);
				snprintf(guider_name, sizeof(guider_name), "%s %s (guider) #%s", CAMERA_NAME_PREFIX, cam->displayname, camera_id);
			}
#endif
			DRIVER_PRIVATE_DATA *private_data = indigo_safe_malloc(sizeof(DRIVER_PRIVATE_DATA));
			private_data->cam = *cam;
			private_data->has_temperature_sensor = device_template == &focuser_template;
			indigo_device *device = indigo_safe_malloc_copy(sizeof(indigo_device), device_template);
			device->private_data = private_data;
			device->master_device = device;
			private_data->camera = device;
			snprintf(device->name, INDIGO_NAME_SIZE, "%s", name);
#ifdef INDIGO_MACOS
			if (device_template != &ccd_template) {
				indigo_make_name_unique(device->name, NULL);
			}
#else
			indigo_make_name_unique(device->name, NULL);
#endif
			if (indigo_attach_device(device) != INDIGO_OK || device->last_result != INDIGO_OK) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to attach %s", device->name);
				process_unplug_event_handler(device);
				continue;
			}
			if (device_template == &ccd_template && (cam->model->flag & SDK_DEF(FLAG_ST4))) {
				indigo_device *guider = indigo_safe_malloc_copy(sizeof(indigo_device), &guider_template);
				guider->private_data = private_data;
				guider->master_device = device;
				private_data->guider = guider;
				snprintf(guider->name, INDIGO_NAME_SIZE, "%s", guider_name);
#ifndef INDIGO_MACOS
				indigo_make_name_unique(guider->name, NULL);
#endif
				if (indigo_attach_device(guider) != INDIGO_OK || guider->last_result != INDIGO_OK) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to attach %s", guider->name);
					process_unplug_event_handler(device);
					continue;
				}
			}
			devices[slot] = device;
		}
	}
}

static void process_unplug_event_handler(indigo_device *removed_device) {
	SDK_TYPE(DeviceV2) cams[SDK_DEF(MAX)];
	int count = 0;
	if (removed_device == NULL) {
		count = SDK_CALL(EnumV2)(cams);
#ifdef TOUPTEK
		count += OEMCamEnum(&cams[count], TOUPCAM_MAX - count);
#endif
	}
	for (int i = 0; i < SDK_DEF(MAX); i++) {
		indigo_device *device = removed_device ? removed_device : devices[i];
		if (device == NULL) {
			continue;
		}
		bool present = false;
		for (int j = 0; j < count; j++) {
			if (!strcmp(INDIGO_WCHAR_TO_CHAR(PRIVATE_DATA->cam.id), INDIGO_WCHAR_TO_CHAR(cams[j].id))) {
				present = true;
				break;
			}
		}
		if (!present) {
			if (removed_device == NULL) {
				devices[i] = NULL;
			}
			PRIVATE_DATA->removing = true;
			indigo_queue_remove(driver_queue, device, NULL);
			indigo_device *guider = PRIVATE_DATA->guider;
			if (guider) {
				indigo_queue_remove(driver_queue, guider, NULL);
				indigo_detach_device(guider);
				free(guider);
				PRIVATE_DATA->guider = NULL;
			}
			indigo_detach_device(device);
			free(device->private_data);
			free(device);
		}
		if (removed_device) {
			break;
		}
	}
}

static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED:
			indigo_queue_add(driver_queue, NULL, INDIGO_TASK_PRIORITY_NORMAL, 0.5, process_plug_event_handler, NULL);
			break;
		case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT:
			indigo_queue_add(driver_queue, NULL, INDIGO_TASK_PRIORITY_NORMAL, 0.5, process_unplug_event_handler, NULL);
			break;
		default:
			break;
	}
	return 0;
}

static libusb_hotplug_callback_handle callback_handle;

#pragma mark - Main code

static bool devices_disconnected(void) {
	for (int i = 0; i < SDK_DEF(MAX); i++) {
		indigo_device *device = devices[i];
		if (device) {
			if (!IS_DISCONNECTED || PRIVATE_DATA->count > 0 || CONNECTION_PROPERTY->state == INDIGO_BUSY_STATE) {
				return false;
			}
			device = PRIVATE_DATA->guider;
			if (device && (!IS_DISCONNECTED || PRIVATE_DATA->count > 0 || CONNECTION_PROPERTY->state == INDIGO_BUSY_STATE)) {
				return false;
			}
		}
	}
	return true;
}

indigo_result ENTRY_POINT(indigo_driver_action action, indigo_driver_info *info) {
	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, true, last_action);
	if (action == last_action) {
		return INDIGO_OK;
	}
	switch (action) {
		case INDIGO_DRIVER_INIT: {
			driver_queue = indigo_queue_create(NULL);
			if (driver_queue == NULL) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to create driver queue");
				return INDIGO_FAILED;
			}
			indigo_queue_set_name(driver_queue, "Queue " DRIVER_LABEL);
			// The enqueue API has no failure return. Preserve the old unbounded asynchronous submission behavior so connection/cleanup tasks cannot be dropped.
			indigo_queue_set_max_pending_tasks(driver_queue, 0);
			for (int i = 0; i < SDK_DEF(MAX); i++) {
				devices[i] = NULL;
			}
			last_action = INDIGO_DRIVER_INIT;
			INDIGO_DRIVER_LOG(DRIVER_NAME, "SDK version %s", INDIGO_WCHAR_TO_CHAR(SDK_CALL(Version)()));
			indigo_start_usb_event_handler();
			int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_register_callback ->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
			if (rc < 0) {
				last_action = INDIGO_DRIVER_SHUTDOWN;
				indigo_queue_remove(driver_queue, NULL, NULL);
				for (int i = 0; i < SDK_DEF(MAX); i++) {
					if (devices[i]) {
						process_unplug_event_handler(devices[i]);
						devices[i] = NULL;
					}
				}
				indigo_queue_delete(&driver_queue);
				return INDIGO_FAILED;
			}
			return INDIGO_OK;
		}
		case INDIGO_DRIVER_SHUTDOWN: {
			last_action = INDIGO_DRIVER_SHUTDOWN;
			libusb_hotplug_deregister_callback(NULL, callback_handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_deregister_callback");
			// Stop discovery before inspecting the device table. An accepted connection remains BUSY/connected, so shutdown must leave it running.
			indigo_queue_remove(driver_queue, NULL, process_plug_event_handler);
			indigo_queue_remove(driver_queue, NULL, process_unplug_event_handler);
			if (!devices_disconnected()) {
				last_action = INDIGO_DRIVER_INIT;
				int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
				if (rc < 0) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to restore hot-plug callback: %s", libusb_error_name(rc));
					return INDIGO_FAILED;
				}
				hotplug_callback(NULL, NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
				return INDIGO_BUSY;
			}
			indigo_queue_remove(driver_queue, NULL, NULL);
			for (int i = 0; i < SDK_DEF(MAX); i++) {
				if (devices[i]) {
					process_unplug_event_handler(devices[i]);
					devices[i] = NULL;
				}
			}
			indigo_queue_delete(&driver_queue);
			break;
		}
		case INDIGO_DRIVER_INFO:
			break;
	}
	return INDIGO_OK;
}
