// Copyright (c) 2019 CloudMakers, s. r. o.
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

/** INDIGO PTP Sony implementation
 \file indigo_ptp_sony.h
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdarg.h>
#include <libusb-1.0/libusb.h>

#include "indigo_ptp.h"
#include "indigo_ptp_sony.h"

#define SONY_PRIVATE_DATA	((sony_private_data *)(PRIVATE_DATA->vendor_private_data))

char *ptp_operation_sony_code_label(uint16_t code) {
	switch (code) {
		case ptp_operation_sony_SDIOConnect: return "sonySDIOConnect_Sony";
		case ptp_operation_sony_GetSDIOGetExtDeviceInfo: return "sonyGetSDIOGetExtDeviceInfo_Sony";
		case ptp_operation_sony_GetDevicePropDesc: return "sonyGetDevicePropDesc_Sony";
		case ptp_operation_sony_GetDevicePropertyValue: return "sonyGetDevicePropertyValue_Sony";
		case ptp_operation_sony_SetControlDeviceA: return "sonySetControlDeviceA_Sony";
		case ptp_operation_sony_GetControlDeviceDesc: return "sonyGetControlDeviceDesc_Sony";
		case ptp_operation_sony_SetControlDeviceB: return "sonySetControlDeviceB_Sony";
		case ptp_operation_sony_GetAllDevicePropData: return "sonyGetAllDevicePropData_Sony";
	}
	return ptp_operation_code_label(code);
}

char *ptp_event_sony_code_label(uint16_t code) {
	switch (code) {
		case ptp_event_sony_ObjectAdded: return "ObjectAdded_Sony";
		case ptp_event_sony_ObjectRemoved: return "ObjectRemoved_Sony";
		case ptp_event_sony_PropertyChanged: return "PropertyChanged_Sony";
	}
	return ptp_event_code_label(code);
}

char *ptp_property_sony_code_name(uint16_t code) {
	switch (code) {
		case ptp_property_CompressionSetting: return DSLR_COMPRESSION_PROPERTY_NAME;
		case ptp_property_WhiteBalance: return DSLR_WHITE_BALANCE_PROPERTY_NAME;
		case ptp_property_FNumber: return DSLR_APERTURE_PROPERTY_NAME;
		case ptp_property_FocusMode: return DSLR_FOCUS_MODE_PROPERTY_NAME;
		case ptp_property_FocusMeteringMode: return DSLR_FOCUS_METERING_PROPERTY_NAME;
		case ptp_property_FlashMode: return DSLR_FLASH_MODE_PROPERTY_NAME;
		case ptp_property_ExposureProgramMode: return DSLR_PROGRAM_PROPERTY_NAME;
		case ptp_property_ExposureBiasCompensation: return DSLR_EXPOSURE_COMPENSATION_PROPERTY_NAME;
		case ptp_property_StillCaptureMode: return DSLR_CAPTURE_MODE_PROPERTY_NAME;
		case ptp_property_sony_ShutterSpeed: return DSLR_SHUTTER_PROPERTY_NAME;
		case ptp_property_sony_AspectRatio: return DSLR_ASPECT_RATIO_PROPERTY_NAME;
		case ptp_property_sony_ISO:	return DSLR_ISO_PROPERTY_NAME;
		case ptp_property_sony_ImageSize: return DSLR_IMAGE_SIZE_PROPERTY_NAME;
	}
	return ptp_property_code_name(code);
}

char *ptp_property_sony_code_label(uint16_t code) {
	switch (code) {
		case ptp_property_sony_DPCCompensation: return "DPCCompensation_Sony";
		case ptp_property_sony_DRangeOptimize: return "DRangeOptimize_Sony";
		case ptp_property_sony_ImageSize: return "Image size";
		case ptp_property_sony_ShutterSpeed: return "Shutter speed";
		case ptp_property_sony_ColorTemp: return "ColorTemp_Sony";
		case ptp_property_sony_CCFilter: return "CCFilter_Sony";
		case ptp_property_sony_AspectRatio: return "Aspect ratio";
		case ptp_property_sony_FocusStatus: return "FocusStatus_Sony";
		case ptp_property_sony_ExposeIndex: return "ExposeIndex_Sony";
		case ptp_property_sony_PictureEffect: return "PictureEffect_Sony";
		case ptp_property_sony_ABFilter: return "ABFilter_Sony";
		case ptp_property_sony_ISO: return "ISO";
		case ptp_property_sony_Autofocus: return "Autofocus_Sony";
		case ptp_property_sony_Capture: return "Capture_Sony";
		case ptp_property_sony_Movie: return "Movie_Sony";
		case ptp_property_sony_StillImage: return "StillImage_Sony";
	}
	return ptp_property_code_label(code);
}

char *ptp_property_sony_value_code_label(indigo_device *device, uint16_t property, uint64_t code) {
	switch (property) {
		case ptp_property_StillCaptureMode: {
			switch (code) {
				case 1: return "Single shooting";
				case 2: return "Cont. shooting HI";
				case 0x8012: return "Cont. shooting LO";
				case 32787: return "Cont. shooting";
				case 32788: return "Spd priority cont.";
				case 32773: return "Self-timer 2s";
				case 0x8003: return "Self-timer 5s";
				case 32772: return "Self-time 10s";
				case 32776: return "Self-timer 10s 3x";
				case 32777: return "Self-timer 10s 5x";
				case 33591: return "Bracket 1/3EV 3x cont.";
				case 33655: return "Bracket 2/3EV 3x cont.";
				case 33553: return "Bracket 1EV 3x cont.";
				case 33569: return "Bracket 2EV 3x cont.";
				case 33585: return "Bracket 3EV 3x cont.";
				case 33590: return "Bracket 1/3EV 3x";
				case 33654: return "Bracket 2/3EV 3x";
				case 33552: return "Bracket 1EV 3x";
				case 33568: return "Bracket 2EV 3x";
				case 33584: return "Bracket 3EV 3x";
				case 32792: return "Bracket WB Lo";
				case 32808: return "Bracket WB Hi";
				case 32793: return "Bracket DRO Lo";
				case 32809: return "Bracket DRO Hi";
			}
			break;
		}
		case ptp_property_WhiteBalance: {
			switch (code) {
				case 2: return "Auto";
				case 4: return "Daylight";
				case 32785: return "Shade";
				case 32784: return "Cloudy";
				case 6: return "Incandescent";
				case 32769: return "Flourescent warm white";
				case 32770: return "Flourescent cool white";
				case 32771: return "Flourescent day white";
				case 32772: return "Flourescent daylight";
				case 7: return "Flash";
				case 32786: return "C.Temp/Filter";
				case 32816: return "Underwater";
				case 32800: return "Custom 1";
				case 32801: return "Custom 2";
				case 32802: return "Custom 3";
				case 32803: return "Custom 4";
			}
			break;
		}
		case ptp_property_CompressionSetting: {
			switch (code) {
				case 2: return "Standard";
				case 3: return "Fine";
				case 4: return "Extra Fine";
				case 16: return "RAW";
				case 19: return "RAW + JPEG";
			}
			break;
		}
		case ptp_property_ExposureMeteringMode: {
			switch (code) {
				case 1: return "Multi";
				case 2: return "Center";
				case 4: return "Spot";
			}
			break;
		}
		case ptp_property_FocusMode: {
			switch (code) {
				case 1: return "MF";
				case 2: return "AF-S";
				case 32772: return "AF-C";
				case 32774: return "DMF";
			}
			break;
		}
		case ptp_property_sony_ImageSize: {
			switch (code) {
				case 1: return "Large";
				case 2: return "Medium";
				case 3: return "Small";
			}
			break;
		}
		case ptp_property_sony_AspectRatio: {
			switch (code) {
				case 1: return "3:2";
				case 2: return "16:9";
			}
			break;
		}
		case ptp_property_ExposureProgramMode: {
			switch (code) {
				case 1: return "M";
				case 2: return "P";
				case 3: return "A";
				case 4: return "S";
				case 7: return "Portrait";
				case 32768: return "Intelligent auto";
				case 32769: return "Superior auto";
				case 32785: return "Sport";
				case 32786: return "Sunset";
				case 32787: return "Night scene";
				case 32788: return "Landscape";
				case 32789: return "Macro";
				case 32790: return "Handheld twilight";
				case 32791: return "Night portrait";
				case 32792: return "Anti motion blur";
				case 32848: return "P - movie";
				case 32849: return "A - movie";
				case 32850: return "S - movie";
				case 32851: return "M - movie";
				case 32833: return "Sweep panorama";
			}
			break;
		}
		case ptp_property_FlashMode: {
			switch (code) {
				case 0: return "Undefined";
				case 1: return "Automatic flash";
				case 2: return "Flash off";
				case 3: return "Fill flash";
				case 4: return "Automatic Red-eye Reduction";
				case 5: return "Red-eye fill flash";
				case 6: return "External sync";
				case 0x8032: return "Slow Sync";
				case 0x8003: return "Reer Sync";
			}
			break;
		}
	}
	return ptp_property_value_code_label(device, property, code);
}

uint8_t *ptp_sony_decode_property(uint8_t *source, indigo_device *device) {
	uint16_t code;
	source = ptp_decode_uint16(source, &code);
	ptp_property dummy, *target = &dummy;
	for (int i = 0; PRIVATE_DATA->info_properties_supported[i]; i++) {
		if (PRIVATE_DATA->info_properties_supported[i] == code) {
			target = PRIVATE_DATA->properties + i;
			break;
		}
	}
	target->code = code;
	source = ptp_decode_uint16(source, &target->type);
	source = ptp_decode_uint8(source, &target->writable);
	source += sizeof(uint8_t);
	switch (target->type) {
		case ptp_uint8_type: {
			uint8_t value;
			source = ptp_decode_uint8(source + sizeof(uint8_t), &value);
			target->value.number.value = value;
			break;
		}
		case ptp_int8_type: {
			int8_t value;
			source = ptp_decode_uint8(source + sizeof(uint8_t), (uint8_t *)&value);
			target->value.number.value = value;
			break;
		}
		case ptp_uint16_type: {
			uint16_t value;
			source = ptp_decode_uint16(source + sizeof(uint16_t), &value);
			target->value.number.value = value;
			break;
		}
		case ptp_int16_type: {
			int16_t value;
			source = ptp_decode_uint16(source + sizeof(uint16_t), (uint16_t *)&value);
			target->value.number.value = value;
			break;
		}
		case ptp_uint32_type: {
			uint32_t value;
			source = ptp_decode_uint32(source + sizeof(uint32_t), &value);
			target->value.number.value = value;
			break;
		}
		case ptp_int32_type: {
			int32_t value;
			source = ptp_decode_uint32(source + sizeof(uint32_t), (uint32_t *)&value);
			target->value.number.value = value;
			break;
		}
		case ptp_uint64_type:
		case ptp_int64_type: {
			int64_t value;
			source = ptp_decode_uint64(source + 2 * sizeof(uint32_t), (uint64_t *)&value);
			target->value.number.value = value;
			break;
		}
		case ptp_uint128_type:
		case ptp_int128_type: {
			source = ptp_decode_uint128(source + 4 * sizeof(uint32_t), target->value.text.value);
			break;
		}
		case ptp_str_type: {
			source += *source *2 + 1;
			source = ptp_decode_string(source, target->value.text.value);
			break;
		}
		default:
			assert(false);
	}
	source = ptp_decode_uint8(source, &target->form);
	switch (target->form) {
		case ptp_none_form:
			if (target->type <= ptp_uint64_type) {
				target->value.number.min = LONG_MIN;
				target->value.number.max = LONG_MAX;
				target->value.number.step = 0;
			}
			break;
		case ptp_range_form:
			switch (target->type) {
				case ptp_uint8_type: {
					uint8_t min, max, step;
					source = ptp_decode_uint8(source, &min);
					source = ptp_decode_uint8(source, &max);
					source = ptp_decode_uint8(source, &step);
					target->value.number.min = min;
					target->value.number.max = max;
					target->value.number.step = step;
					break;
				}
				case ptp_int8_type: {
					int8_t min, max, step;
					source = ptp_decode_uint8(source, (uint8_t *)&min);
					source = ptp_decode_uint8(source, (uint8_t *)&max);
					source = ptp_decode_uint8(source, (uint8_t *)&step);
					target->value.number.min = min;
					target->value.number.max = max;
					target->value.number.step = step;
					break;
				}
				case ptp_uint16_type: {
					uint16_t min, max, step;
					source = ptp_decode_uint16(source, &min);
					source = ptp_decode_uint16(source, &max);
					source = ptp_decode_uint16(source, &step);
					target->value.number.min = min;
					target->value.number.max = max;
					target->value.number.step = step;
					break;
				}
				case ptp_int16_type: {
					int16_t min, max, step;
					source = ptp_decode_uint16(source, (uint16_t *)&min);
					source = ptp_decode_uint16(source, (uint16_t *)&max);
					source = ptp_decode_uint16(source, (uint16_t *)&step);
					target->value.number.min = min;
					target->value.number.max = max;
					target->value.number.step = step;
					break;
				}
				case ptp_uint32_type: {
					uint32_t min, max, step;
					source = ptp_decode_uint32(source, &min);
					source = ptp_decode_uint32(source, &max);
					source = ptp_decode_uint32(source, &step);
					target->value.number.min = min;
					target->value.number.max = max;
					target->value.number.step = step;
					break;
				}
				case ptp_int32_type: {
					int32_t min, max, step;
					source = ptp_decode_uint32(source, (uint32_t *)&min);
					source = ptp_decode_uint32(source, (uint32_t *)&max);
					source = ptp_decode_uint32(source, (uint32_t *)&step);
					target->value.number.min = min;
					target->value.number.max = max;
					target->value.number.step = step;
					break;
				}
				case ptp_uint64_type: {
					uint64_t min, max, step;
					source = ptp_decode_uint64(source, &min);
					source = ptp_decode_uint64(source, &max);
					source = ptp_decode_uint64(source, &step);
					target->value.number.min = min;
					target->value.number.max = max;
					target->value.number.step = step;
					break;
				}
				case ptp_int64_type: {
					int64_t min, max, step;
					source = ptp_decode_uint64(source, (uint64_t *)&min);
					source = ptp_decode_uint64(source, (uint64_t *)&max);
					source = ptp_decode_uint64(source, (uint64_t *)&step);
					target->value.number.min = min;
					target->value.number.max = max;
					target->value.number.step = step;
					break;
				}
				case ptp_uint128_type:
				case ptp_int128_type:
					source += 3 * 4 * sizeof(uint32_t);
					break;
				default:
					assert(false);
			}
			break;
		case ptp_enum_form: {
			uint16_t count;
			source = ptp_decode_uint16(source, &count);
			target->count = count;
			for (int i = 0; i < target->count; i++) {
				switch (target->type) {
					case ptp_uint8_type: {
						uint8_t value;
						source = ptp_decode_uint8(source, &value);
						target->value.sw.values[i] = value;
						break;
					}
					case ptp_int8_type: {
						int8_t value;
						source = ptp_decode_uint8(source, (uint8_t *)&value);
						target->value.sw.values[i] = value;
						break;
					}
					case ptp_uint16_type: {
						uint16_t value;
						source = ptp_decode_uint16(source, &value);
						target->value.sw.values[i] = value;
						break;
					}
					case ptp_int16_type: {
						int16_t value;
						source = ptp_decode_uint16(source, (uint16_t *)&value);
						target->value.sw.values[i] = value;
						break;
					}
					case ptp_uint32_type: {
						uint32_t value;
						source = ptp_decode_uint32(source, &value);
						target->value.sw.values[i] = value;
						break;
					}
					case ptp_int32_type: {
						int32_t value;
						source = ptp_decode_uint32(source, (uint32_t *)&value);
						target->value.sw.values[i] = value;
						break;
					}
					case ptp_str_type: {
						if (i < 16)
							source = ptp_decode_string(source, target->value.sw_str.values[i]);
						break;
					}
					default:
						assert(false);
				}
			}
			break;
		}
	}
	switch (code) {
		case ptp_property_ExposureBiasCompensation: {
			target->count = 3;
			target->type = ptp_str_type;
			sprintf(target->value.sw_str.value, "%+.1f", target->value.number.value / 1000.0);
			strcpy(target->value.sw_str.values[0], "-");
			strcpy(target->value.sw_str.values[1], target->value.sw_str.value);
			strcpy(target->value.sw_str.values[2], "+");
			break;
		}
		case ptp_property_FNumber: {
			target->count = 3;
			target->type = ptp_str_type;
			sprintf(target->value.sw_str.value, "f/%.1f", target->value.number.value / 100.0);
			strcpy(target->value.sw_str.values[0], "-");
			strcpy(target->value.sw_str.values[1], target->value.sw_str.value);
			strcpy(target->value.sw_str.values[2], "+");
			break;
		}
		case ptp_property_sony_ISO: {
			target->count = 3;
			target->type = ptp_str_type;
			sprintf(target->value.sw_str.value, "%d", (int)target->value.number.value);
			strcpy(target->value.sw_str.values[0], "-");
			strcpy(target->value.sw_str.values[1], target->value.sw_str.value);
			strcpy(target->value.sw_str.values[2], "+");
			break;
		}
		case ptp_property_sony_ShutterSpeed: {
			target->count = 3;
			target->type = ptp_str_type;
			if (target->value.number.value == 0) {
				strcpy(target->value.sw_str.value, "Bulb");
			} else {
				int a = (int)target->value.number.value >> 16;
				int b = (int)target->value.number.value & 0xFFFF;
				if (b == 10)
					sprintf(target->value.sw_str.value, "%g\"", (double)a / b);
				else
					sprintf(target->value.sw_str.value, "1/%d", b);
			}
			strcpy(target->value.sw_str.values[0], "-");
			strcpy(target->value.sw_str.values[1], target->value.sw_str.value);
			strcpy(target->value.sw_str.values[2], "+");
			break;
		}
	}
	ptp_update_property(device, target);
	return source;
}

static void ptp_check_event(indigo_device *device) {
	ptp_get_event(device);
	indigo_reschedule_timer(device, 1, &PRIVATE_DATA->event_checker);
}

bool ptp_sony_initialise(indigo_device *device) {
	PRIVATE_DATA->vendor_private_data = malloc(sizeof(sony_private_data));
	memset(SONY_PRIVATE_DATA, 0, sizeof(sony_private_data));
	if (!ptp_initialise(device))
		return false;
	INDIGO_LOG(indigo_log("%s[%d, %s]: device ext_info", DRIVER_NAME, __LINE__, __FUNCTION__));

	if (ptp_operation_supported(device, ptp_operation_sony_GetSDIOGetExtDeviceInfo)) {
		void *buffer = NULL;
		uint32_t size;
		ptp_transaction_3_0(device, ptp_operation_sony_SDIOConnect, 1, 0, 0);
		ptp_transaction_3_0(device, ptp_operation_sony_SDIOConnect, 2, 0, 0);
		if (ptp_transaction_1_0_i(device, ptp_operation_sony_GetSDIOGetExtDeviceInfo, 0xC8, &buffer, &size)) {
			uint32_t count = size / 2;
			uint16_t operations[PTP_MAX_ELEMENTS] = { 0 }, *last_operation = operations;
			uint16_t events[PTP_MAX_ELEMENTS] = { 0 }, *last_event = events;
			uint16_t properties[PTP_MAX_ELEMENTS] = { 0 }, *last_property = properties;
			for (int i = 0; i < count; i++) {
				uint16_t code = ((uint16_t *)buffer)[i];
				if ((code & 0x7000) == 0x1000) {
					*last_operation++ = code;
				} else if ((code & 0x7000) == 0x4000) {
					*last_event++ = code;
				} else if ((code & 0x7000) == 0x5000) {
					*last_property++ = code;
				}
			}
			ptp_append_uint16_16_array(PRIVATE_DATA->info_operations_supported, operations);
			ptp_append_uint16_16_array(PRIVATE_DATA->info_events_supported, events);
			ptp_append_uint16_16_array(PRIVATE_DATA->info_properties_supported, properties);
			free(buffer);
			buffer = NULL;
			INDIGO_LOG(indigo_log("%s[%d, %s]: device ext_info", DRIVER_NAME, __LINE__, __FUNCTION__));
			INDIGO_LOG(indigo_log("operations:"));
			for (uint16_t *operation = operations; *operation; operation++) {
				INDIGO_LOG(indigo_log("  %04x %s", *operation, ptp_operation_sony_code_label(*operation)));
			}
			INDIGO_LOG(indigo_log("events:"));
			for (uint16_t *event = events; *event; event++) {
				INDIGO_LOG(indigo_log("  %04x %s", *event, ptp_event_sony_code_label(*event)));
			}
			INDIGO_LOG(indigo_log("properties:"));
			for (uint16_t *property = properties; *property; property++) {
				INDIGO_LOG(indigo_log("  %04x %s", *property, ptp_property_sony_code_label(*property)));
			}
			ptp_transaction_3_0(device, ptp_operation_sony_SDIOConnect, 3, 0, 0);
			if (ptp_transaction_0_0_i(device, ptp_operation_sony_GetAllDevicePropData, &buffer, &size)) {
				uint8_t *source = buffer;
				source = ptp_decode_uint32(source, &count);
				source += sizeof(uint32_t);
				for (int i = 0; i < count; i++) {
					source = ptp_sony_decode_property(source, device);
				}
				free(buffer);
				buffer = NULL;
			}
		}
		if (buffer)
			free(buffer);
	}
	PRIVATE_DATA->event_checker = indigo_set_timer(device, 0.5, ptp_check_event);
	return true;
}

bool ptp_sony_handle_event(indigo_device *device, ptp_event_code code, uint32_t *params) {
	switch ((int)code) {
		case ptp_event_sony_PropertyChanged: {
			void *buffer = NULL;
			uint32_t size, count;
			if (ptp_transaction_0_0_i(device, ptp_operation_sony_GetAllDevicePropData, &buffer, &size)) {
				uint8_t *source = buffer;
				source = ptp_decode_uint32(source, &count);
				source += sizeof(uint32_t);
				for (int i = 0; i < count; i++) {
					source = ptp_sony_decode_property(source, device);
				}
			}
			free(buffer);
			buffer = NULL;
			return true;
		}
	}
	return ptp_handle_event(device, code, params);
}

bool ptp_sony_set_property(indigo_device *device, ptp_property *property) {
	assert(0);
}

bool ptp_sony_exposure(indigo_device *device) {
	assert(0);
}

bool ptp_sony_liveview(indigo_device *device) {
	assert(0);
}

bool ptp_sony_af(indigo_device *device) {
	assert(0);
}

bool ptp_sony_check_compression_has_raw(indigo_device *device) {
	assert(0);
}
