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

#include <indigo/indigo_ccd_driver.h>

#include "indigo_ptp.h"
#include "indigo_ptp_sony.h"

#define SONY_PRIVATE_DATA	((sony_private_data *)(PRIVATE_DATA->vendor_private_data))
#define SONY_ITERATE_DOWN	0xFFFF0000FFFFFFFFULL
#define SONY_ITERATE_UP		0xFFFF000000000000ULL
#define SONY_ISO_AUTO     0xFFFFFFULL

char *ptp_operation_sony_code_label(uint16_t code) {
	switch (code) {
		case ptp_operation_sony_SDIOConnect: return "SDIOConnect_Sony";
		case ptp_operation_sony_GetSDIOGetExtDeviceInfo: return "GetSDIOGetExtDeviceInfo_Sony";
		case ptp_operation_sony_GetDevicePropDesc: return "GetDevicePropDesc_Sony";
		case ptp_operation_sony_GetDevicePropertyValue: return "GetDevicePropertyValue_Sony";
		case ptp_operation_sony_SetControlDeviceA: return "SetControlDeviceA_Sony";
		case ptp_operation_sony_GetControlDeviceDesc: return "GetControlDeviceDesc_Sony";
		case ptp_operation_sony_SetControlDeviceB: return "SetControlDeviceB_Sony";
		case ptp_operation_sony_GetAllDevicePropData: return "GetAllDevicePropData_Sony";
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
		case ptp_property_sony_ImageSize: return CCD_MODE_PROPERTY_NAME;
		case ptp_property_sony_PictureEffect: return DSLR_PICTURE_STYLE_PROPERTY_NAME;
		case ptp_property_sony_BatteryLevel: return DSLR_BATTERY_LEVEL_PROPERTY_NAME;
		case ptp_property_sony_PCRemoteSaveDest: return "ADV_PCRemoteSaveDest";
		case ptp_property_sony_Zoom: return "Zoom_Sony";
		case ptp_property_sony_ZoomState: return "ADV_ZoomState";
		case ptp_property_sony_ZoomRatio: return "ADV_ZoomRatio";
		case ptp_property_sony_NearFar: return "NearFar";
		case ptp_property_sony_ExposureCompensation: return "ADV_ExposureCompensation";
		case ptp_property_sony_SensorCrop: return "ADV_SensorCrop";
	}
	return ptp_property_code_name(code);
}

char *ptp_property_sony_code_label(uint16_t code) {
	switch (code) {
		case ptp_property_sony_DPCCompensation: return "DPC Compensation";
		case ptp_property_sony_DRangeOptimize: return "D Range Optimize";
		case ptp_property_sony_ImageSize: return "Image size";
		case ptp_property_sony_ShutterSpeed: return "Shutter speed";
		case ptp_property_sony_ColorTemp: return "Color Temperature";
		case ptp_property_sony_CCFilter: return "CC Filter";
		case ptp_property_sony_AspectRatio: return "Aspect ratio";
		case ptp_property_sony_FocusStatus: return "Focus Status";
		case ptp_property_sony_Zoom: return "Zoom";
		case ptp_property_sony_ExposeIndex: return "Expose Index";
		case ptp_property_sony_BatteryLevel: return "Battery Level";
		case ptp_property_sony_SensorCrop: return "Sensor Crop";
		case ptp_property_sony_PictureEffect: return "Picture effect";
		case ptp_property_sony_ABFilter: return "AB Filter";
		case ptp_property_sony_ISO: return "ISO";
		case ptp_property_sony_Autofocus: return "Autofocus";
		case ptp_property_sony_Capture: return "Capture";
		case ptp_property_sony_Movie: return "Movie";
		case ptp_property_sony_StillImage: return "Still Image";
		case ptp_property_sony_PCRemoteSaveDest: return "PC Remote Save Destination";
		case ptp_property_sony_ExposureCompensation: return "Exposure compensation";
		case ptp_property_sony_NearFar: return "Near/Far Manual Focus Adjustment";
		case ptp_property_sony_ZoomState: return "Zoom State";
		case ptp_property_sony_ZoomRatio: return "Zoom Ratio";
	}
	return ptp_property_code_label(code);
}

char *ptp_property_sony_value_code_label(indigo_device *device, uint16_t property, uint64_t code) {
	static char label[PTP_MAX_CHARS];
	switch (property) {
		case ptp_property_StillCaptureMode: {
			switch (code) {
				case 1: return "Single shooting";
				case 2: return "Cont. shooting HI";
				case 0x8012: return "Cont. shooting LO";
				case 32787: return "Cont. shooting";
				case 0x8015: return "Cont. shooting MI";
				case 32788: return "Spd priority cont.";
				case 32773: return "Self-timer 2s";
				case 0x8003: return "Self-timer 5s";
				case 32772: return "Self-time 10s";
				case 32776: return "Self-timer 10s 3x";
				case 32777: return "Self-timer 10s 5x";
				case 0x800c: return "Self-timer 5s 3x";
				case 0x800d: return "Self-timer 5s 5x";
				case 0x800e: return "Self-timer 2s 3x";
				case 0x800f: return "Self-timer 2s 5x";
				case 0x8010: return "Continuous Hi+ Speed";
				case 33591: return "Bracket 1/3EV 3x cont.";
				case 0x8537: return "Bracket 1/3EV 5x cont.";
				case 0x8937: return "Bracket 1/3EV 9x cont.";
				case 0x8357: return "Bracket 1/2EV 3x cont.";
				case 0x8557: return "Bracket 1/2EV 5x cont.";
				case 0x8957: return "Bracket 1/2EV 9x cont.";
				case 33655: return "Bracket 2/3EV 3x cont.";
				case 0x8577: return "Bracket 2/3EV 5x cont.";
				case 0x8977: return "Bracket 2/3EV 9x cont.";
				case 33553: return "Bracket 1EV 3x cont.";
				case 0x8511: return "Bracket 1EV 5x cont.";
				case 0x8911: return "Bracket 1EV 9x cont.";
				case 33569: return "Bracket 2EV 3x cont.";
				case 0x8521: return "Bracket 2EV 5x cont.";
				case 33585: return "Bracket 3EV 3x cont.";
				case 0x8531: return "Bracket 3EV 5x cont.";
				case 33590: return "Bracket 1/3EV 3x";
				case 0x8536: return "Bracket 1/3EV 5x";
				case 0x8936: return "Bracket 1/3EV 9x";
				case 0x8356: return "Bracket 1/2EV 3x";
				case 0x8556: return "Bracket 1/2EV 5x";
				case 0x8956: return "Bracket 1/2EV 9x";
				case 33654: return "Bracket 2/3EV 3x";
				case 0x8576: return "Bracket 2/3EV 5x";
				case 0x8976: return "Bracket 2/3EV 9x";
				case 33552: return "Bracket 1EV 3x";
				case 0x8510: return "Bracket 1EV 5x";
				case 0x8910: return "Bracket 1EV 9x";
				case 33568: return "Bracket 2EV 3x";
				case 0x8520: return "Bracket 2EV 5x";
				case 33584: return "Bracket 3EV 3x";
				case 0x8530: return "Bracket 3EV 5x";
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
				case 18: return "RAW + JPEG (Standard)";
				case 19: return "RAW + JPEG";
				case 20: return "RAW + JPEG (Extra Fine)";
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
				case 32773: return "AF-A";
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
				case 32848: return "Movie P";
				case 32849: return "Movie A";
				case 32850: return "Movie S";
				case 32851: return "Movie M";
				case 32833: return "Sweep panorama";
				case 32857: return "Q&S auto";
				case 32858: return "Q&S A";
				case 32859: return "Q&S S";
				case 32860: return "Q&S M";
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
		case ptp_property_sony_PictureEffect: {
			switch (code) {
				case 32768: return "Off";
				case 32769: return "Toy camera - normal";
				case 32770: return "Toy camera - cool";
				case 32771: return "Toy camera - warm";
				case 32772: return "Toy camera - green";
				case 32773: return "Toy camera - magenta";
				case 32784: return "Pop Color";
				case 32800: return "Posterisation B/W";
				case 32801: return "Posterisation Color";
				case 32816: return "Retro";
				case 32832: return "Soft high key";
				case 32848: return "Partial color - red";
				case 32849: return "Partial color - green";
				case 32850: return "Partial color - blue";
				case 32851: return "Partial color - yellow";
				case 32864: return "High contrast mono";
				case 32880: return "Soft focus - low";
				case 32881: return "Soft focus - mid";
				case 32882: return "Soft focus - high";
				case 32896: return "HDR painting - low";
				case 32897: return "HDR painting - mid";
				case 32898: return "HDR painting - high";
				case 32912: return "Rich tone mono";
				case 32928: return "Miniature - auto";
				case 32929: return "Miniature - top";
				case 32930: return "Miniature - middle horizontal";
				case 32931: return "Miniature - bottom";
				case 32932: return "Miniature - right";
				case 32933: return "Miniature - middle vertical";
				case 32934: return "Miniature - left";
				case 32944: return "Watercolor";
				case 32960: return "Illustration - low";
				case 32961: return "Illustration - mid";
				case 32962: return "Illustration - high";
			}
			break;
		}
		case ptp_property_ExposureBiasCompensation: {
			switch (code) {
				case SONY_ITERATE_DOWN: return "-";
				case SONY_ITERATE_UP: return "+";
			}
			sprintf(label, "%+.1f", (int)code / 1000.0);
			return label;
		}
		case ptp_property_FNumber: {
			switch (code) {
				case SONY_ITERATE_DOWN: return "-";
				case SONY_ITERATE_UP: return "+";
			}
			sprintf(label, "f/%.1f", (int)code / 100.0);
			return label;
		}
		case ptp_property_sony_ISO: {
			switch (code) {
				case SONY_ITERATE_DOWN: return "-";
				case SONY_ITERATE_UP: return "+";
				case SONY_ISO_AUTO: return "Auto";
			}
			sprintf(label, "%d", (int)code);
			return label;
		}
		case ptp_property_sony_ShutterSpeed: {
			if (code == SONY_ITERATE_DOWN) {
				return "-";
			} else if (code == SONY_ITERATE_UP) {
				return "+";
			} else if (code == 0) {
				return "Bulb";
			} else {
				int a = (int)code >> 16;
				int b = (int)code & 0xFFFF;
				if (b == 10)
					sprintf(label, "%g\"", (double)a / b);
				else
					sprintf(label, "1/%d", b);
			}
			return label;
		}
		case ptp_property_sony_PCRemoteSaveDest: {
			switch (code) {
				case 0x01: return "PC";
				case 0x10: return "Camera";
				case 0x11: return "PC+Camera";
			}
			break;
		}
		case ptp_property_sony_ZoomState: {
			switch (code) {
				case 0: return "Normal";
				case 1: return "x1.0";
				case 2: return "Zooming";
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
	uint64_t mode = SONY_PRIVATE_DATA->mode;
	switch (code) {
		case ptp_property_sony_ImageSize:
			target->count = 3;
			break;
		case ptp_property_ExposureBiasCompensation:
			target->count = 3;
			target->value.sw.values[0] = SONY_ITERATE_DOWN;
			target->value.sw.values[1] = target->value.number.value;
			target->value.sw.values[2] = SONY_ITERATE_UP;
			target->writable = mode == 2 || mode == 3 || mode == 4 || mode == 32848 || mode == 32849 || mode == 32850 || mode == 32851;
			break;
		case ptp_property_sony_ISO:
			target->count = 3;
			target->value.sw.values[0] = SONY_ITERATE_DOWN;
			target->value.sw.values[1] = target->value.number.value;
			target->value.sw.values[2] = SONY_ITERATE_UP;
			target->writable = mode == 1 || mode == 2 || mode == 3 || mode == 4 || mode == 32848 || mode == 32849 || mode == 32850 || mode == 32851;
			break;
		case ptp_property_sony_ShutterSpeed:
			target->count = 3;
			target->value.sw.values[0] = SONY_ITERATE_DOWN;
			target->value.sw.values[1] = target->value.number.value;
			target->value.sw.values[2] = SONY_ITERATE_UP;
			target->writable = mode == 1 || mode == 4 || mode == 32850 || mode == 32851;
			SONY_PRIVATE_DATA->shutter_speed = target->value.number.value;
			break;
		case ptp_property_FNumber:
			target->count = 3;
			target->value.sw.values[0] = SONY_ITERATE_DOWN;
			target->value.sw.values[1] = target->value.number.value;
			target->value.sw.values[2] = SONY_ITERATE_UP;
			target->writable = mode == 1 ||  mode == 3 ||mode == 32849 || mode == 32851;
			break;
		case ptp_property_FocusMode:
			target->writable = mode == 1 || mode == 2 || mode == 3 || mode == 4 || mode == 32848 || mode == 32849 || mode == 32850 || mode == 32851;
			SONY_PRIVATE_DATA->focus_mode = target->value.number.value;
			break;
		case ptp_property_ExposureMeteringMode:
			target->writable = true;
			break;
		case ptp_property_ExposureProgramMode:
			if (target->count == 0 || target->count == 1) {
				target->count = 1;
				target->value.sw.values[0] = target->value.sw.value;
			}
			if (SONY_PRIVATE_DATA->mode != target->value.sw.value) {
				SONY_PRIVATE_DATA->mode = target->value.sw.value;
				ptp_sony_handle_event(device, (ptp_event_code)ptp_event_sony_PropertyChanged, NULL);
			}
			break;
		case ptp_property_sony_FocusStatus:
			SONY_PRIVATE_DATA->focus_state = target->value.number.value;
			break;
		case ptp_property_CompressionSetting:
			SONY_PRIVATE_DATA->is_dual_compression = target-> value.number.value == 18 || target-> value.number.value == 19 || target-> value.number.value == 20;
			break;
	}
	ptp_update_property(device, target);
	return source;
}

static void ptp_check_event(indigo_device *device) {
#ifdef USE_ICA_TRANSPORT
	ptp_get_event(device);
#else
	ptp_container event;
	int length = 0;
	memset(&event, 0, sizeof(event));
	int rc = libusb_bulk_transfer(PRIVATE_DATA->handle, PRIVATE_DATA->ep_int, (unsigned char *)&event, sizeof(event), &length, 1000);
	if (rc >= 0) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer() -> %s, %d", rc < 0 ? libusb_error_name(rc) : "OK", length);
		PTP_DUMP_CONTAINER(&event);
		PRIVATE_DATA->handle_event(device, event.code, event.payload.params);
	}
#endif
	if (IS_CONNECTED) {
		indigo_reschedule_timer(device, 0, &PRIVATE_DATA->event_checker);
	}
}

bool ptp_sony_initialise(indigo_device *device) {
	DSLR_MIRROR_LOCKUP_PROPERTY->hidden = true;
	DSLR_DELETE_IMAGE_PROPERTY->hidden = true;
	indigo_set_switch(DSLR_DELETE_IMAGE_PROPERTY, DSLR_DELETE_IMAGE_OFF_ITEM, true);
	PRIVATE_DATA->vendor_private_data = indigo_safe_malloc(sizeof(sony_private_data));
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
			// SONY a7II and a7S need to wait to boot
			indigo_usleep(ONE_SECOND_DELAY);
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
	indigo_set_timer(device, 0.5, ptp_check_event, &PRIVATE_DATA->event_checker);
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
		case ptp_event_sony_ObjectAdded: {
			void *buffer = NULL;
			if (ptp_transaction_1_0_i(device, ptp_operation_GetObjectInfo, params[0], &buffer, NULL)) {
				uint32_t size;
				char filename[PTP_MAX_CHARS];
				uint8_t *source = buffer;
				source = ptp_decode_uint32(source + 8, &size);
				source = ptp_decode_string(source + 40 , filename);
				free(buffer);
				buffer = NULL;
				INDIGO_DRIVER_LOG(DRIVER_NAME, "ptp_event_ObjectAdded: handle = %08x, size = %u, name = '%s'", params[0], size, filename);
				if (size && ptp_transaction_1_0_i(device, ptp_operation_GetObject, params[0], &buffer, NULL)) {
					const char *ext = strchr(filename, '.');
					if (PRIVATE_DATA->check_dual_compression(device) && ptp_check_jpeg_ext(ext)) {
						if (CCD_PREVIEW_ENABLED_ITEM->sw.value) {
							indigo_process_dslr_preview_image(device, buffer, size);
						}
						ptp_sony_handle_event(device, code, params);
					} else {
						indigo_process_dslr_image(device, buffer, size, ext, false);
						if (PRIVATE_DATA->image_buffer)
							free(PRIVATE_DATA->image_buffer);
						PRIVATE_DATA->image_buffer = buffer;
						buffer = NULL;
					}
				}
			}
			if (buffer)
				free(buffer);
			return true;
		}
	}
	return ptp_handle_event(device, code, params);
}

bool ptp_sony_set_property(indigo_device *device, ptp_property *property) {
  switch (property->code) {
		case ptp_property_ExposureBiasCompensation:
		case ptp_property_sony_ISO:
		case ptp_property_sony_ShutterSpeed:
    case ptp_property_FNumber: {
			int16_t value = 0;
			if (property->property->items[0].sw.value) {
				value = -1;
			} else if (property->property->items[2].sw.value) {
				value = 1;
			}
			if (property->code == ptp_property_sony_ShutterSpeed)
				value = -value;
			indigo_set_switch(property->property, property->property->items + 1, true);
			return ptp_transaction_0_1_o(device, ptp_operation_sony_SetControlDeviceB, property->code, &value, sizeof(uint16_t));
		}
    default: {
			int64_t value = 0;
			switch (property->property->type) {
				case INDIGO_SWITCH_VECTOR: {
					for (int i = 0; i < property->property->count; i++) {
						indigo_item *item = property->property->items + i;
						if (item->sw.value) {
							value = strtol(item->name, NULL, 16);
							break;
						}
					}
					break;
				}
				case INDIGO_NUMBER_VECTOR: {
					property->value.number.value = property->property->items->number.target;
					value = (int64_t)property->value.number.value;
					break;
				}
				default:
					assert(0);
			}
			switch (property->type) {
				case ptp_int8_type:
				case ptp_uint8_type:
					return ptp_transaction_0_1_o(device, ptp_operation_sony_SetControlDeviceA, property->code, &value, sizeof(uint8_t));
				case ptp_int16_type:
				case ptp_uint16_type:
					return ptp_transaction_0_1_o(device, ptp_operation_sony_SetControlDeviceA, property->code, &value, sizeof(uint16_t));
				case ptp_int32_type:
				case ptp_uint32_type:
					return ptp_transaction_0_1_o(device, ptp_operation_sony_SetControlDeviceA, property->code, &value, sizeof(uint32_t));
				case ptp_int64_type:
				case ptp_uint64_type:
					return ptp_transaction_0_1_o(device, ptp_operation_sony_SetControlDeviceA, property->code, &value, sizeof(uint64_t));
			}
		}
  }
	return false;
}

bool ptp_sony_exposure(indigo_device *device) {
	if ((PRIVATE_DATA->model.product == 0x0ccc || PRIVATE_DATA->model.product == 0x0d9f) && !SONY_PRIVATE_DATA->did_capture) {
		// A7R4/A7R4A needs 3s delay before first capture
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "3s delay...");
		for (int i = 0; i < 30; i++) {
			if (PRIVATE_DATA->abort_capture)
				return false;
			indigo_usleep(100000);
		}
		SONY_PRIVATE_DATA->did_capture = true;
		SONY_PRIVATE_DATA->did_liveview = false;
	}
	int16_t value = 2;
	SONY_PRIVATE_DATA->focus_state = 1;
	if (ptp_transaction_0_1_o(device, ptp_operation_sony_SetControlDeviceB, ptp_property_sony_Autofocus, &value, sizeof(uint16_t))) {
		if (SONY_PRIVATE_DATA->focus_mode != 1) {
			for (int i = 0; i < 50 && SONY_PRIVATE_DATA->focus_state == 1; i++) {
				usleep(100000);
			}
			if (SONY_PRIVATE_DATA->focus_state == 3) {
				value = 1;
				ptp_transaction_0_1_o(device, ptp_operation_sony_SetControlDeviceB, ptp_property_sony_Autofocus, &value, sizeof(uint16_t));
				return false;
			}
		} else {
			usleep(1000000);
		}
	}
	if (SONY_PRIVATE_DATA->shutter_speed == 0) {
		value = 1;
		ptp_transaction_0_1_o(device, ptp_operation_sony_SetControlDeviceB, ptp_property_sony_Autofocus, &value, sizeof(uint16_t));
	}
	value = 2;
	if (ptp_transaction_0_1_o(device, ptp_operation_sony_SetControlDeviceB, ptp_property_sony_Capture, &value, sizeof(uint16_t))) {
		value = 1;
		if (SONY_PRIVATE_DATA->shutter_speed == 0) {
			ptp_blob_exposure_timer(device);
			ptp_transaction_0_1_o(device, ptp_operation_sony_SetControlDeviceB, ptp_property_sony_Capture, &value, sizeof(uint16_t));
		} else {
			ptp_transaction_0_1_o(device, ptp_operation_sony_SetControlDeviceB, ptp_property_sony_Capture, &value, sizeof(uint16_t));
			ptp_transaction_0_1_o(device, ptp_operation_sony_SetControlDeviceB, ptp_property_sony_Autofocus, &value, sizeof(uint16_t));
		}
		if (CCD_IMAGE_PROPERTY->state == INDIGO_BUSY_STATE && CCD_PREVIEW_ENABLED_ITEM->sw.value && ptp_sony_check_dual_compression(device)) {
			CCD_PREVIEW_IMAGE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_PREVIEW_IMAGE_PROPERTY, NULL);
		}
		bool complete_detected = false;
		while (true) {
			if (PRIVATE_DATA->abort_capture || (CCD_IMAGE_PROPERTY->state != INDIGO_BUSY_STATE && CCD_PREVIEW_IMAGE_PROPERTY->state != INDIGO_BUSY_STATE && CCD_IMAGE_FILE_PROPERTY->state != INDIGO_BUSY_STATE))
				break;
			// SONY a9 does not notify ObjectAdded
			ptp_property *property = ptp_property_supported(device, ptp_property_sony_ObjectInMemory);
			if (!complete_detected && property && property->value.number.value > 0x8000) {
				// CaptureCompleted
				complete_detected = property->value.number.value == 0x8001;
				uint32_t dummy[1] = { 0xffffc001 };
				ptp_sony_handle_event(device, (ptp_event_code)ptp_event_sony_ObjectAdded, dummy);
			}
			indigo_usleep(100000);
		}
		if (PRIVATE_DATA->abort_capture) {
			if (CCD_IMAGE_PROPERTY->state != INDIGO_OK_STATE) {
				CCD_IMAGE_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
			}
			if (CCD_PREVIEW_IMAGE_PROPERTY->state != INDIGO_OK_STATE) {
				CCD_PREVIEW_IMAGE_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, CCD_PREVIEW_IMAGE_PROPERTY, NULL);
			}
			if (CCD_IMAGE_FILE_PROPERTY->state != INDIGO_OK_STATE) {
				CCD_IMAGE_FILE_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
			}
		}
		return !PRIVATE_DATA->abort_capture;
	}
	return false;
}

bool ptp_sony_liveview(indigo_device *device) {
	void *buffer = NULL;
	uint32_t size;
	int retry_count = 0;
	if ((PRIVATE_DATA->model.product == 0x0ccc || PRIVATE_DATA->model.product == 0x0d9f) && !SONY_PRIVATE_DATA->did_liveview) {
		// A7R4/A7R4A needs 3s delay before first capture
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "3s delay...");
		for (int i = 0; i < 30; i++) {
			if (PRIVATE_DATA->abort_capture)
				return false;
			indigo_usleep(100000);
		}
		SONY_PRIVATE_DATA->did_liveview = true;
		SONY_PRIVATE_DATA->did_capture = false;
	}
	while (!PRIVATE_DATA->abort_capture && CCD_STREAMING_COUNT_ITEM->number.value != 0) {
		if (ptp_transaction_1_0_i(device, ptp_operation_GetObject, 0xffffc002, &buffer, &size)) {
			uint8_t *start = (uint8_t *)buffer;
			while (size > 0) {
				if (start[0] == 0xFF && start[1] == 0xD8 && start[2] == 0xFF && start[3] == 0xDB) {
					uint8_t *end = start + 2;
					size -= 2;
					while (size > 0) {
						if (end[0] == 0xFF && end[1] == 0xD9) {
							if (CCD_UPLOAD_MODE_LOCAL_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
								CCD_IMAGE_FILE_PROPERTY->state = INDIGO_BUSY_STATE;
								indigo_update_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
							}
							if (CCD_UPLOAD_MODE_CLIENT_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
								CCD_IMAGE_PROPERTY->state = INDIGO_BUSY_STATE;
								indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
							}
							indigo_process_dslr_image(device, start, (int)(end - start), ".jpeg", true);
							if (PRIVATE_DATA->image_buffer)
								free(PRIVATE_DATA->image_buffer);
							PRIVATE_DATA->image_buffer = buffer;
							buffer = NULL;
							CCD_STREAMING_COUNT_ITEM->number.value--;
							if (CCD_STREAMING_COUNT_ITEM->number.value < 0)
								CCD_STREAMING_COUNT_ITEM->number.value = -1;
							indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
							retry_count = 0;
							break;
						}
						end++;
						size--;
					}
					break;
				}
				start++;
				size--;
			}
		} else if (PRIVATE_DATA->last_error == ptp_response_AccessDenied) {
			if (retry_count++ > 100) {
				indigo_finalize_dslr_video_stream(device);
				return false;
			}
		}
		if (buffer)
			free(buffer);
		buffer = NULL;
		indigo_usleep(100000);
	}
	indigo_finalize_dslr_video_stream(device);
	return !PRIVATE_DATA->abort_capture;
}

bool ptp_sony_af(indigo_device *device) {
	if (SONY_PRIVATE_DATA->focus_mode != 1) {
		int16_t value = 2;
		SONY_PRIVATE_DATA->focus_state = 1;
		if (ptp_transaction_0_1_o(device, ptp_operation_sony_SetControlDeviceB, ptp_property_sony_Autofocus, &value, sizeof(uint16_t))) {
			for (int i = 0; i < 50 && SONY_PRIVATE_DATA->focus_state == 1; i++) {
				usleep(100000);
			}
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "focus_state %d", (int)(SONY_PRIVATE_DATA->focus_state));
			value = 1;
			ptp_transaction_0_1_o(device, ptp_operation_sony_SetControlDeviceB, ptp_property_sony_Autofocus, &value, sizeof(uint16_t));
			return SONY_PRIVATE_DATA->focus_state;
		}
	}
	return false;
}

bool ptp_sony_focus(indigo_device *device, int steps) {
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	if (steps == 0) {
		pthread_mutex_lock(&mutex);
		SONY_PRIVATE_DATA->steps = 0;
		pthread_mutex_unlock(&mutex);
		return true;
	} else {
		pthread_mutex_lock(&mutex);
		SONY_PRIVATE_DATA->steps = steps;
		pthread_mutex_unlock(&mutex);		
		while (true) {
			pthread_mutex_lock(&mutex);
			if (SONY_PRIVATE_DATA->steps == 0) {
				pthread_mutex_unlock(&mutex);
				return true;
			}
			int16_t value = SONY_PRIVATE_DATA->steps <= -7 ? -7 : (SONY_PRIVATE_DATA->steps >= 7 ? 7 : SONY_PRIVATE_DATA->steps);
			SONY_PRIVATE_DATA->steps -= value;
			pthread_mutex_unlock(&mutex);
			if (!ptp_transaction_0_1_o(device, ptp_operation_sony_SetControlDeviceB, ptp_property_sony_NearFar, &value, sizeof(uint16_t))) {
				return false;
			}
			indigo_usleep(50000);
		}
	}
	return true;
}

bool ptp_sony_check_dual_compression(indigo_device *device) {
	return SONY_PRIVATE_DATA->is_dual_compression;
}
