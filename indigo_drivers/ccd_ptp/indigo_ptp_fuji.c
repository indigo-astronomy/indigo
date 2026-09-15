// Copyright (c) 2019-2026 CloudMakers, s. r. o.
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
// 2.0 by Tsunoda Koji <tsunoda@astroarts.co.jp>

/** INDIGO PTP Fuji implementation
 \file indigo_ptp_fuji.h
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdarg.h>

#include <indigo/indigo_ccd_driver.h>
#include <indigo/indigo_usb_utils.h>

#include "indigo_ptp.h"
#include "indigo_ptp_fuji.h"

#define FUJI_PRIVATE_DATA	((fuji_private_data *)(PRIVATE_DATA->vendor_private_data))
#define FUJI_CONTROL_PRIORITY_CAMERA 0x0001
#define FUJI_CONTROL_PRIORITY_PC 0x0002
#define FUJI_CARD_SAVE_ON_RAW_JPEG 0x0001
#define FUJI_CARD_SAVE_OFF 0x0004
#define FUJI_LIVEVIEW_HANDLE 0x80000001

char *ptp_operation_fuji_code_label(uint16_t code) {
	return ptp_operation_code_label(code);
}

char *ptp_event_fuji_code_label(uint16_t code) {
	return ptp_event_code_label(code);
}

char *ptp_property_fuji_code_name(uint16_t code) {
	switch (code) {
		case ptp_property_ImageSize: return CCD_MODE_PROPERTY_NAME;
		case ptp_property_ExposureTime: return DSLR_SHUTTER_PROPERTY_NAME;
		case ptp_property_FNumber: return DSLR_APERTURE_PROPERTY_NAME;
		case ptp_property_ExposureProgramMode: return DSLR_PROGRAM_PROPERTY_NAME;
		case ptp_property_ExposureIndex: return DSLR_ISO_PROPERTY_NAME;
		case ptp_property_WhiteBalance: return DSLR_WHITE_BALANCE_PROPERTY_NAME;
		case ptp_property_FocusMode: return DSLR_FOCUS_MODE_PROPERTY_NAME;
		case ptp_property_FocusMeteringMode: return DSLR_FOCUS_METERING_PROPERTY_NAME;
		case ptp_property_fuji_FilmSimulation: return "ADV_FilmSimulation";
		case ptp_property_fuji_DynamicRange: return "ADV_DynamicRange";
		case ptp_property_fuji_ColorSpace: return "ADV_ColorSpace";
		case ptp_property_fuji_CompressionSetting: return DSLR_COMPRESSION_PROPERTY_NAME;
		case ptp_property_fuji_Zoom: return "ADV_Zoom";
		case ptp_property_fuji_NoiseReduction: return "ADV_NoiseReduction";
		case ptp_property_fuji_LockSetting: return "ADV_LockSetting";
		case ptp_property_fuji_ControlPriority: return "DSLR_ControlPriority";
		case ptp_property_fuji_AutoFocus: return "ADV_AutoFocus";
		case ptp_property_fuji_AutoFocusState: return "ADV_AutoFocusState";
		case ptp_property_fuji_CardSave: return "DSLR_CardSave";
	}
	return ptp_property_code_name(code);
}

char *ptp_property_fuji_code_label(uint16_t code) {
	switch (code) {
		case ptp_property_fuji_FilmSimulation: return "Film Simulation";
		case ptp_property_fuji_DynamicRange: return "Dynamic Range";
		case ptp_property_fuji_ColorSpace: return "Color Space";
		case ptp_property_fuji_CompressionSetting: return "Compression";
		case ptp_property_fuji_Zoom: return "Zoom";
		case ptp_property_fuji_NoiseReduction: return "Noise Reduction";
		case ptp_property_fuji_LockSetting: return "Lock Setting";
		case ptp_property_fuji_ControlPriority: return "Control Priority";
		case ptp_property_fuji_AutoFocus: return "AutoFocus";
		case ptp_property_fuji_AutoFocusState: return "AutoFocus State";
		case ptp_property_fuji_CardSave: return "Card Save";
	}
	return ptp_property_code_label(code);
}

char *ptp_property_fuji_value_code_label(indigo_device *device, uint16_t property, uint64_t code) {
	switch (property) {
		case ptp_property_ExposureProgramMode: {
			switch (code) {
				case 1: return "Manual";
				case 3: return "Aperture priority";
				case 4: return "Shutter priority";
				case 6: return "Program";
			}
			break;
		}
		case ptp_property_ExposureTime: {
			switch (code) {
				case 0x0000007a: return "1/8000s";
				case 0x00000099: return "1/6400s";
				case 0x000000c1: return "1/5000s";
				case 0x000000f4: return "1/4000s";
				case 0x00000133: return "1/3200s";
				case 0x00000183: return "1/2500s";
				case 0x000001e8: return "1/2000s";
				case 0x00000267: return "1/1600s";
				case 0x00000307: return "1/1250s";
				case 0x000003d0: return "1/1000s";
				case 0x000004ce: return "1/800s";
				case 0x0000060e: return "1/640s";
				case 0x000007a1: return "1/500s";
				case 0x0000099c: return "1/400s";
				case 0x00000c1c: return "1/320s";
				case 0x00000f42: return "1/250s";
				case 0x00001339: return "1/200s";
				case 0x00001838: return "1/160s";
				case 0x00001e84: return "1/125s";
				case 0x00002673: return "1/100s";
				case 0x00003071: return "1/80s";
				case 0x00003d09: return "1/60s";
				case 0x00004ce6: return "1/50s";
				case 0x000060e3: return "1/40s";
				case 0x00007a12: return "1/30s";
				case 0x000099cc: return "1/25s";
				case 0x0000c1c6: return "1/20s";
				case 0x0000f424: return "1/15s";
				case 0x00013399: return "1/13s";
				case 0x0001838c: return "1/10s";
				case 0x0001e848: return "1/8s";
				case 0x00026732: return "1/6s";
				case 0x00030719: return "1/5s";
				case 0x0003d090: return "1/4s";
				case 0x0004ce64: return "1/3s";
				case 0x00060e32: return "1/2.5s";
				case 0x0007a120: return "1/2s";
				case 0x00099cc8: return "1/1.6s";
				case 0x000c1c64: return "1/1.3s";
				case 0x000f4240: return "1s";
				case 0x00133991: return "1.3s";
				case 0x00159445: return "1.5s";
				case 0x001838c9: return "1.5s";
				case 0x001e8480: return "2s";
				case 0x00267322: return "2.5s";
				case 0x00307192: return "3s";
				case 0x003d0900: return "4s";
				case 0x004ce644: return "5s";
				case 0x0060e324: return "6.5s";
				case 0x007a1200: return "8s";
				case 0x0099cc88: return "10s";
				case 0x00c1c648: return "13s";
				case 0x00f42400: return "15s";
				case 0x01339910: return "20s";
				case 0x01838c90: return "25s";
				case 0x01e84800: return "30s";
				case 0x02673221: return "40s";
				case 0x03071921: return "50s";
				case 0x03d09000: return "60s";
				case 0x03d0901e: return "2m";
				case 0x03d0903c: return "4m";
				case 0x03d0905a: return "8m";
				case 0x03d09078: return "15m";
				case 0x03d09096: return "30m";
				case 0x03d090b4: return "60m";
				case 0xffffffff: return "Bulb";
				case 0xfffffffe: return "Time";
				case 0xfffffff0: return "Auto";
			}
			break;
		}
		case ptp_property_ExposureIndex:
			switch (code) {
				case -1: return "AUTO1";
				case -2: return "AUTO2";
				case -3: return "AUTO3";
			}
			break;
		case ptp_property_WhiteBalance:
			switch (code) {
				case 0x0008: return "Underwater";
				case 0x8001: return "Fluorescent Light-1";
				case 0x8002: return "Fluorescent Light-2";
				case 0x8003: return "Fluorescent Light-3";
				case 0x8006: return "Shade";
				case 0x8007: return "Color Temperature";
				case 0x8008: return "Custom1";
				case 0x8009: return "Custom2";
				case 0x800a: return "Custom3";
			}
			break;
		case ptp_property_FocusMode:
			switch (code) {
				case 0x0001: return "MF";
				case 0x8001: return "AF-S";
				case 0x8002: return "AF-C";
			}
			break;
		case ptp_property_FocusMeteringMode:
			switch (code) {
				case 2: return "Multi Point";
				case 0x8001: return "Area";
				case 0x8002: return "Zone";
				case 0x8003: return "Wide/Tracking";
			}
			break;
		case ptp_property_fuji_FilmSimulation:
			switch (code) {
				case 0x01: return "PROVIA/STANDARD";
				case 0x02: return "Velvia/VIVID";
				case 0x03: return "ASTIA/SOFT";
				case 0x04: return "PRO Neg. Hi";
				case 0x05: return "PRO Neg. Std";
				case 0x06: return "MONOCHROME";
				case 0x07: return "MONOCHROME+Ye FILTER";
				case 0x08: return "MONOCHROME+R FILTER";
				case 0x09: return "MONOCHROME+G FILTER";
				case 0x0a: return "SEPIA";
				case 0x0b: return "CLASSIC CHROME";
				case 0x0c: return "ACROS";
				case 0x0d: return "ACROS+Ye FILTER";
				case 0x0e: return "ACROS+R FILTER";
				case 0x0f: return "ACROS+G FILTER";
			}
			break;
		case ptp_property_fuji_DynamicRange:
			switch (code) {
				case 0xffff: return "AUTO";
				case 0x064: return "100%";
				case 0x0c8: return "200%";
				case 0x190: return "400%";
			}
			break;
		case ptp_property_fuji_ColorSpace:
			switch (code) {
				case 0x01: return "sRGB";
				case 0x02: return "Adobe RGB";
			}
			break;
		case ptp_property_fuji_CompressionSetting:
			switch (code) {
				case 1: return "RAW";
				case 2: return "FINE";
				case 3: return "NORMAL";
				case 4: return "FINE+RAW";
				case 5: return "NORMAL+RAW";
				case 6: return "SUPER FINE";
				case 7: return "SUPER FINE+RAW";
			}
			break;
		case ptp_property_fuji_Zoom:
			switch (code) {
				case 1: return "1x";
				case 2: return "2.5x";
				case 4: return "4x";
				case 5: return "8x";
				case 6: return "16.7x";
				//case 7: return "";
				//case 8: return "";
				//case 9: return "";
				//case 10: return "";
			}
			break;
		case ptp_property_fuji_NoiseReduction:
			switch (code) {
				case 0x0000: return "+2";
				case 0x1000: return "+1";
				case 0x2000: return "0";
				case 0x3000: return "-1";
				case 0x4000: return "-2";
				case 0x5000: return "+4";
				case 0x6000: return "+3";
				case 0x7000: return "-3";
				case 0x8000: return "-4";
			}
			break;
		case ptp_property_fuji_LockSetting:
			switch (code) {
				case 1: return "Unlock";
				case 2: return "All Function";
				case 3: return "Selected Function";
			}
			break;
		case ptp_property_fuji_ControlPriority:
			switch (code) {
				case 1: return "Camera";
				case 2: return "PC";
			}
			break;
		case ptp_property_fuji_CardSave:
			switch (code) {
				case 1: return "On RAW+JPEG";
				case 2: return "On RAW";
				case 3: return "On JPEG";
				case 4: return "Off";
			}
			break;
	}
	return ptp_property_value_code_label(device, property, code);
}

static uint16_t FUJI_CHECK_PROPERTIES[] = {
	ptp_property_ImageSize,
	ptp_property_WhiteBalance,
	ptp_property_FNumber,
	ptp_property_ExposureTime,
	ptp_property_ExposureIndex,
	ptp_property_ExposureBiasCompensation,
	ptp_property_fuji_FilmSimulation,
	ptp_property_fuji_DynamicRange,
	ptp_property_fuji_ColorSpace,
	ptp_property_fuji_CompressionSetting,
	ptp_property_fuji_Zoom,
	ptp_property_fuji_NoiseReduction,
	ptp_property_fuji_LockSetting,
	ptp_property_fuji_ControlPriority,
	0,
};

static void ptp_fuji_get_event(indigo_device *device) {
	void *buffer = NULL;
	uint32_t size = 0;
	for (int i = 0; FUJI_CHECK_PROPERTIES[i]; i++) {
		uint16_t code = FUJI_CHECK_PROPERTIES[i];
		ptp_property *prop = ptp_property_supported(device, FUJI_CHECK_PROPERTIES[i]);
		if (! prop) {
			continue;
		}
		if (ptp_transaction_1_0_i(device, ptp_operation_GetDevicePropValue, code, &buffer, &size)) {
			if (code == 0xd212) {
				continue;
			}
			ptp_decode_property_value(buffer, device, prop);
			ptp_fuji_fix_property(device, prop);
			ptp_update_property(device, prop);
		}
		if (buffer) {
			free(buffer);
			buffer = NULL;
		}
	}
	// check event?
	if (ptp_transaction_1_0_i(device, ptp_operation_GetDevicePropValue, ptp_property_fuji_GetEvent, &buffer, &size)) {
//		if (size == 8 && (
//				// X-T1
//				memcmp(buffer, "\x01\x00\x0e\xd2\x10\x00\x00\x00", 8) == 0 ||
//				// X-T2
//				memcmp(buffer, "\x01\x00\x0e\xd2\x12\x00\x00\x00", 8) == 0 ||
//				// X-T3, X-T4
//				memcmp(buffer, "\x01\x00\x0e\xd2\x1f\x00\x00\x00", 8) == 0 ||
//				// GFX 50S
//				memcmp(buffer, "\x01\x00\x0e\xd2\x05\x00\x00\x00", 8) == 0 ||
//				// X-S10
//				memcmp(buffer, "\x01\x00\x0e\xd2\x0f\x00\x00\x00", 8) == 0)) {
		if (size == 8 && memcmp(buffer, "\x01\x00\x0e\xd2", 4) == 0) {
			free(buffer);
			buffer = NULL;
			// capture ready
			if (ptp_transaction_3_0_i(device, ptp_operation_GetObjectHandles, 0xffffffff, 0, 0, &buffer, &size)) {
				uint32_t count = 0;
				uint8_t *source = buffer;
				void *image_buffer = NULL;
				source = ptp_decode_uint32(source, &count);
				for (uint32_t i = 0; i < count; ++i) {
					uint32_t handle = 0;
					source = ptp_decode_uint32(source, &handle);
					if (ptp_transaction_1_0_i(device, ptp_operation_GetObjectInfo, handle, &image_buffer, NULL)) {
						uint32_t image_size;
						char filename[PTP_MAX_CHARS];
						uint8_t *source = image_buffer;
						source = ptp_decode_uint32(source + 8, &image_size);
						source = ptp_decode_string(source + 40 , filename);
						free(image_buffer);
						image_buffer = NULL;
						INDIGO_DRIVER_LOG(DRIVER_NAME, "ptp_event_ObjectAdded: handle = %08x, size = %u, name = '%s'", handle, image_size, filename);
						if (size && ptp_transaction_1_0_i(device, ptp_operation_GetObject, handle, &image_buffer, NULL)) {
							const char *ext = strchr(filename, '.');
							if (PRIVATE_DATA->check_dual_compression(device) && ptp_check_jpeg_ext(ext)) {
								if (CCD_PREVIEW_ENABLED_ITEM->sw.value) {
									indigo_process_dslr_preview_image(device, image_buffer, image_size);
								}
							} else {
								indigo_process_dslr_image(device, image_buffer, image_size, ext, false);
							}
							if (PRIVATE_DATA->image_buffer) {
								free(PRIVATE_DATA->image_buffer);
							}
							PRIVATE_DATA->image_buffer = image_buffer;
							PRIVATE_DATA->image_added = true;
							image_buffer = NULL;
							if (ptp_transaction_1_0(device, ptp_operation_DeleteObject, handle)) {
								INDIGO_DRIVER_LOG(DRIVER_NAME, "ptp_operation_DeleteObject succeed.");
							}
						}
					}
				}
			}
		} else if (size == 8) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "unknown signature: %02x %02x %02x %02x %02x %02x %02x %02x", ((char *)buffer)[0] & 0xFF, ((char *)buffer)[1] & 0xFF, ((char *)buffer)[2] & 0xFF, ((char *)buffer)[3] & 0xFF, ((char *)buffer)[4] & 0xFF, ((char *)buffer)[5] & 0xFF, ((char *)buffer)[6] & 0xFF, ((char *)buffer)[7] & 0xFF);
		}
	}
	if (buffer) {
		free(buffer);
		buffer = NULL;
	}
}

static void ptp_fuji_check_event(indigo_device *device) {
	ptp_fuji_get_event(device);
	if (IS_CONNECTED) {
		indigo_execute_handler_in(device, 1, ptp_fuji_check_event);
	}
}

bool ptp_fuji_initialise(indigo_device *device) {
	PRIVATE_DATA->vendor_private_data = indigo_safe_malloc(sizeof(fuji_private_data));
	if (!ptp_initialise(device))
		return false;
	uint16_t value = 1;
	if (ptp_transaction_0_1_o(device, ptp_operation_SetDevicePropValue, 0xd38c, &value, sizeof(uint16_t))) {
		INDIGO_LOG(indigo_log("D38C=1"));
	}
	void *buffer = NULL;
	uint32_t size = 0;
	const int hidden_properties[] = {
		ptp_property_ImageSize,
		ptp_property_WhiteBalance,
		ptp_property_FNumber,
		ptp_property_ExposureTime,
		ptp_property_ExposureIndex,
		ptp_property_ExposureBiasCompensation,
		ptp_property_fuji_FilmSimulation,
		ptp_property_fuji_DynamicRange,
		ptp_property_fuji_ColorSpace,
		ptp_property_fuji_CompressionSetting,
		ptp_property_fuji_Zoom,
		ptp_property_fuji_NoiseReduction,
		ptp_property_fuji_LockSetting,
		ptp_property_fuji_ControlPriority,
		ptp_property_fuji_AutoFocus,
		ptp_property_fuji_AutoFocusState,
		ptp_property_fuji_CardSave
	};
	for (int i = 0; i < sizeof(hidden_properties)/sizeof(int); ++i) {
		if (!ptp_property_supported(device, hidden_properties[i])) {
			// Fujifilm hidden property special case
			if (ptp_transaction_1_0_i(device, ptp_operation_GetDevicePropDesc, hidden_properties[i], &buffer, &size)) {
				// Not listed but property defined
				// detect last
				int last = -1;
				for (last = 0; PRIVATE_DATA->info_properties_supported[last]; last++) {
				}
				// overwrite last
				if (last != -1) {
					PRIVATE_DATA->info_properties_supported[last] = hidden_properties[i];
					ptp_decode_property(buffer, size, device, PRIVATE_DATA->properties + last);
				}
			}
			if (buffer) {
				free(buffer);
				buffer = NULL;
			}
		}
	}
	if (!ptp_property_supported(device, ptp_property_FNumber)) {
		// Fujifilm ExposureTime special case
		if (ptp_transaction_1_0_i(device, ptp_operation_GetDevicePropDesc, ptp_property_FNumber, &buffer, &size)) {
			// ExposureTime is hidden property
			// detect last
			int last = -1;
			for (last = 0; PRIVATE_DATA->info_properties_supported[last]; last++) {
			}
			// overwrite last
			if (last != -1) {
				PRIVATE_DATA->info_properties_supported[last] = ptp_property_FNumber;
				ptp_decode_property(buffer, size, device, PRIVATE_DATA->properties + last);
			}
		}
		if (buffer) {
			free(buffer);
			buffer = NULL;
		}
	}
	ptp_fuji_get_event(device);
	indigo_execute_handler_in(device, 0.5, ptp_fuji_check_event);
	return true;
}

bool ptp_fuji_fix_property(indigo_device *device, ptp_property *property) {
	switch (property->code) {
		case ptp_property_fuji_AutoFocus: {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Fuji d208 descriptor type=0x%04x form=%u count=%d", property->type, property->form, property->count);
			FUJI_PRIVATE_DATA->action_enum_known = property->form == ptp_enum_form && property->type == ptp_uint16_type && property->count >= 0;
			if (FUJI_PRIVATE_DATA->action_enum_known) {
				bool start = false, stop = false;
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Fuji d208 declared action count=%d", property->count);
				for (int i = 0; i < property->count; i++) {
					unsigned action = property->value.sw.values[i];
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Fuji d208 declared action[%d]=0x%04x", i, action);
					start |= action == 0x0500;
					stop |= action == 0x000c;
				}
				FUJI_PRIVATE_DATA->bulb_supported = start && stop;
			}
			return true;
		}
		case ptp_property_fuji_CompressionSetting: {
			FUJI_PRIVATE_DATA->is_dual_compression = property->value.sw.value == 4 || property->value.sw.value == 5 || property->value.sw.value == 7;
			return true;
		}
		case ptp_property_ExposureTime: {
			if (property->count == 0 || property->count == 1) { // A
				property->count = 1;
				property->value.sw.values[0] = property->value.sw.value;
				property->writable = false;
			} else {
				// enumeration succeed
				// search bulb property
				bool has_bulb = false;
				for (int i=0; i < property->count; ++i) {
					if (property->value.sw.values[i] == 0xffffffff) {
						has_bulb = true;
						break;
					}
				}
				if (!has_bulb) {
					// some model (e.g. X-T4) does not enumerate Bulb
					property->value.sw.values[property->count] = 0xffffffff;
					property->count += 1;
				}
				property->writable = true;
			}
			return true;
		}
		case ptp_property_FNumber: {
			if (property->count == 0 || property->count == 1) { // A
				property->count = 1;
				property->value.sw.values[0] = property->value.sw.value;
				property->writable = false;
			} else {
				// detect no lens
				if (property->value.sw.value == 0) {
					property->count = 0;
					property->writable = false;
				} else {
					property->writable = true;
				}
			}
			return true;
		}
	}
	return false;
}

static bool ptp_set_switch_property(indigo_device *device, ptp_property *property, uint16_t value) {
	bool result = true;
	if (property->value.sw.value != value) {
		for (int i=0; i < property->property->count; i++) {
			property->property->items[i].sw.value = property->value.sw.values[i] == value;
		}
		property->value.sw.value = value;
		result = ptp_set_property(device, property);
		indigo_update_property(device, property->property, NULL);
	}
	return result;
}

bool ptp_fuji_set_control_priority(indigo_device *device, bool pc) {
	ptp_property *property = ptp_property_supported(device, ptp_property_fuji_ControlPriority);
	if (!property) {
		return false;
	}
	const uint16_t target = pc ? FUJI_CONTROL_PRIORITY_PC : FUJI_CONTROL_PRIORITY_CAMERA;
	return ptp_set_switch_property(device, property, target);
}

bool ptp_fuji_set_property(indigo_device *device, ptp_property *property) {
	if (property->code != ptp_property_fuji_ControlPriority && !ptp_fuji_set_control_priority(device, true)) {
		return false;
	}
	if (!ptp_set_property(device, property)) {
		return false;
	}
	if (property->code == ptp_property_ExposureProgramMode) {
		ptp_refresh_property(device, ptp_property_supported(device, ptp_property_ExposureTime));
		ptp_refresh_property(device, ptp_property_supported(device, ptp_property_FNumber));
	}
	return true;
}

static bool fuji_capture_stop(indigo_device *device) {
	if (!PRIVATE_DATA->shutter_open && !FUJI_PRIVATE_DATA->focus_held) {
		return true;
	}
	uint16_t value = PRIVATE_DATA->shutter_open ? 0x000c : 0x0004;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Fuji release d208=0x%04x", value);
	bool result = ptp_transaction_0_1_o(device, ptp_operation_SetDevicePropValue, ptp_property_fuji_AutoFocus, &value, sizeof(value)) && ptp_transaction_2_0(device, ptp_operation_InitiateCapture, 0, 0);
	if (result) {
		PRIVATE_DATA->shutter_open = false;
		FUJI_PRIVATE_DATA->focus_held = false;
	}
	return result;
}

bool ptp_fuji_exposure(indigo_device *device) {
	if (PRIVATE_DATA->capture_phase == 0) {
		PRIVATE_DATA->capture_stop = fuji_capture_stop;
		if (!fuji_capture_stop(device)) {
			return false;
		}
		ptp_property *property = ptp_property_supported(device, ptp_property_ExposureTime);
		if (!property || !ptp_operation_supported(device, ptp_operation_InitiateCapture)) {
			return false;
		}
		// PC priority and queued AF polling can change the camera's shutter readback.
		FUJI_PRIVATE_DATA->shutter_speed = property->value.sw.value;
		if (FUJI_PRIVATE_DATA->shutter_speed == 0xffffffff && FUJI_PRIVATE_DATA->action_enum_known && !FUJI_PRIVATE_DATA->bulb_supported) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Camera does not advertise USB BULB start and stop");
			indigo_send_message(device, ALERT_PROPERTY, "Camera does not advertise USB BULB start and stop");
			return false;
		}
		if (PRIVATE_DATA->abort_capture || !ptp_fuji_set_control_priority(device, true)) {
			return false;
		}
		ptp_property *card_save = ptp_property_supported(device, ptp_property_fuji_CardSave);
		if (card_save && !ptp_set_switch_property(device, card_save, DSLR_DELETE_IMAGE_ON_ITEM->sw.value || PRIVATE_DATA->model.product == 703 ? FUJI_CARD_SAVE_OFF : FUJI_CARD_SAVE_ON_RAW_JPEG)) {
			return false;
		}
		uint16_t value = 0x0200;
		if (!ptp_transaction_0_1_o(device, ptp_operation_SetDevicePropValue, ptp_property_fuji_AutoFocus, &value, sizeof(value))) {
			return false;
		}
		FUJI_PRIVATE_DATA->focus_held = true;
		if (!ptp_transaction_2_0(device, ptp_operation_InitiateCapture, 0, 0)) {
			return false;
		}
		PRIVATE_DATA->capture_phase = 3;
		PRIVATE_DATA->phase_deadline = indigo_monotonic_time() + 10;
		return true;
	}
	if (PRIVATE_DATA->capture_phase == 3) {
		if (PRIVATE_DATA->abort_capture || indigo_monotonic_time() >= PRIVATE_DATA->phase_deadline) {
			return false;
		}
		void *buffer = NULL;
		uint32_t size = 0;
		bool result = ptp_transaction_1_0_i(device, ptp_operation_GetDevicePropValue, ptp_property_fuji_AutoFocusState, &buffer, &size);
		uint16_t state = 1;
		if (result && buffer && size >= sizeof(state)) {
			ptp_decode_uint16(buffer, &state);
		} else {
			result = false;
		}
		free(buffer);
		if (!result) {
			return false;
		}
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Fuji autofocus d209=0x%04x", state);
		if (state == 1) {
			return true;
		}
		if (state != 2) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Fuji autofocus did not become capture-ready (0x%04x)", state);
			return false;
		}
		bool bulb = FUJI_PRIVATE_DATA->shutter_speed == 0xffffffff;
		uint16_t value = bulb ? 0x0500 : 0x0304;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Fuji capture d208=0x%04x, requested shutter=0x%08llx", value, (unsigned long long)FUJI_PRIVATE_DATA->shutter_speed);
		if (!ptp_transaction_0_1_o(device, ptp_operation_SetDevicePropValue, ptp_property_fuji_AutoFocus, &value, sizeof(value)) || !ptp_transaction_2_0(device, ptp_operation_InitiateCapture, 0, 0)) {
			return false;
		}
		FUJI_PRIVATE_DATA->focus_held = false;
		ptp_capture_start(device, bulb, 0);
		return true;
	}
	if (PRIVATE_DATA->capture_phase == 1) {
		if (!PRIVATE_DATA->abort_capture && indigo_monotonic_time() < PRIVATE_DATA->shutter_deadline) {
			return true;
		}
		if (!fuji_capture_stop(device)) {
			return false;
		}
		PRIVATE_DATA->capture_phase = 2;
	}
	return ptp_capture_wait(device);
}

static bool fuji_liveview_stop(indigo_device *device) {
	if (!PRIVATE_DATA->stream_started) {
		return true;
	}
	bool result = ptp_transaction_1_0(device, ptp_operation_TerminateOpenCapture, FUJI_LIVEVIEW_HANDLE);
	PRIVATE_DATA->stream_started = !result;
	return result;
}

bool ptp_fuji_liveview(indigo_device *device) {
	PRIVATE_DATA->liveview_stop = fuji_liveview_stop;
	if (!PRIVATE_DATA->stream_started) {
		if (!ptp_fuji_set_control_priority(device, true)) {
			return false;
		}
		if (!ptp_transaction_2_0(device, ptp_operation_InitiateOpenCapture, 0, 0)) {
			return PRIVATE_DATA->last_error == ptp_response_DeviceBusy && ++PRIVATE_DATA->stream_retries <= 100;
		}
		PRIVATE_DATA->stream_started = true;
		PRIVATE_DATA->stream_retries = 0;
		return true;
	}
	void *buffer = NULL;
	uint32_t size = 0;
	if (!ptp_transaction_3_0(device, ptp_operation_GetNumObjects, 0xffffffff, 0, 0) || !ptp_transaction_3_0_i(device, ptp_operation_GetObjectHandles, 0xffffffff, 0, 0, &buffer, &size)) {
		free(buffer);
		return PRIVATE_DATA->last_error == ptp_response_DeviceBusy && ++PRIVATE_DATA->stream_retries <= 100;
	}
	uint32_t count = 0, handle = 0;
	if (!buffer || size < sizeof(count)) {
		free(buffer);
		return false;
	}
	ptp_decode_uint32(buffer, &count);
	if (count != 1) {
		free(buffer);
		return ++PRIVATE_DATA->stream_retries <= 100;
	}
	if (size < 2 * sizeof(uint32_t)) {
		free(buffer);
		return false;
	}
	ptp_decode_uint32((uint8_t *)buffer + sizeof(count), &handle);
	free(buffer);
	buffer = NULL;
	bool result = ptp_transaction_1_0_i(device, ptp_operation_GetObjectInfo, handle, &buffer, &size);
	free(buffer);
	buffer = NULL;
	if (!result || !ptp_transaction_1_0_i(device, ptp_operation_GetObject, handle, &buffer, &size) || !buffer || size == 0) {
		free(buffer);
		return false;
	}
	ptp_stream_frame(device, buffer, buffer, size);
	return ptp_transaction_1_0(device, ptp_operation_DeleteObject, handle);
}

bool ptp_fuji_af(indigo_device *device) {
	return false;
}

bool ptp_fuji_check_dual_compression(indigo_device *device) {
	return FUJI_PRIVATE_DATA->is_dual_compression;
}
