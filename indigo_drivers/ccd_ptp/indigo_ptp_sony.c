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
#define SONY_ILCE_7RM4_PRODUCT_ID 0x0ccc
#define SONY_ILCE_7RM4A_PRODUCT_ID 0x0d9f
// SONY ILCE-9M2 with Camera Remote SDK Ver.1.10.00
#define SONY_NEW_API_SUPPORT_PRODUCT_ID_BORDER SONY_ILCE_7RM4_PRODUCT_ID
#define SONY_NEW_API 300
#define SONY_OLD_API 200

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
		case ptp_property_ExposureMeteringMode: return DSLR_EXPOSURE_METERING_PROPERTY_NAME;
		case ptp_property_FlashMode: return DSLR_FLASH_MODE_PROPERTY_NAME;
		case ptp_property_ExposureProgramMode: return DSLR_PROGRAM_PROPERTY_NAME;
		case ptp_property_ExposureBiasCompensation: return DSLR_EXPOSURE_COMPENSATION_PROPERTY_NAME;
		case ptp_property_StillCaptureMode: return DSLR_CAPTURE_MODE_PROPERTY_NAME;
		case ptp_property_FocusMeteringMode: return DSLR_FOCUS_METERING_PROPERTY_NAME;
		case ptp_property_sony_DPCCompensation: return "ADV_DPCCompensation";
		case ptp_property_sony_DRangeOptimize: return "ADV_DRangeOptimize";
		case ptp_property_sony_ImageSize: return CCD_MODE_PROPERTY_NAME;
		case ptp_property_sony_ShutterSpeed: return DSLR_SHUTTER_PROPERTY_NAME;
		case ptp_property_sony_AspectRatio: return DSLR_ASPECT_RATIO_PROPERTY_NAME;
		case ptp_property_sony_Zoom: return "Zoom_Sony";
		//case ptp_property_sony_D217: return "ADV_SONY_D217";
		case ptp_property_sony_BatteryLevel: return DSLR_BATTERY_LEVEL_PROPERTY_NAME;
		case ptp_property_sony_SensorCrop: return "ADV_SensorCrop";
		case ptp_property_sony_PictureEffect: return DSLR_PICTURE_STYLE_PROPERTY_NAME;
		case ptp_property_sony_ISO:	return DSLR_ISO_PROPERTY_NAME;
		//case ptp_property_sony_D21F: return "ADV_SONY_D21F";
		//case ptp_property_sony_D221: return "ADV_SONY_D221";
		case ptp_property_sony_PCRemoteSaveDest: return "ADV_PCRemoteSaveDest";
		case ptp_property_sony_ExposureCompensation: return "ADV_ExposureCompensation";
		case ptp_property_sony_FocusArea: return DSLR_FOCUS_METERING_PROPERTY_NAME;
		case ptp_property_sony_LiveViewDisplay: return "ADV_LiveViewDisplay";
		//case ptp_property_sony_D239: return "ADV_SONY_D239";
		//case ptp_property_sony_D23A: return "ADV_SONY_D23A";
		case ptp_property_sony_PictureProfile: return "ADV_PictureProfile";
		case ptp_property_sony_CreativeStyle: return "ADV_CreativeStyle";
		case ptp_property_sony_MovieFileFormat: return "ADV_MovieFileFormat";
		case ptp_property_sony_MovieRecordSetting: return "ADV_MovieRecordSetting";
		//case ptp_property_sony_D243: return "ADV_SONY_D243";
		//case ptp_property_sony_D244: return "ADV_SONY_D244";
		//case ptp_property_sony_D245: return "ADV_SONY_D245";
		//case ptp_property_sony_D246: return "ADV_SONY_D246";
		case ptp_property_sony_MemoryCameraSettings: return "ADV_MemoryCameraSettings";
		//case ptp_property_sony_D248: return "ADV_SONY_D248";
		case ptp_property_sony_IntervalShoot: return "ADV_IntervalShoot";
		case ptp_property_sony_JPEGQuality: return "ADV_JPEGQuality";
		case ptp_property_sony_CompressionSetting: return DSLR_COMPRESSION_PROPERTY_NAME;
		case ptp_property_sony_FocusMagnifier: return "ADV_FocusMagnifier";
		case ptp_property_sony_AFTrackingSensitivity: return "ADV_AFTrackingSensitivity";
		//case ptp_property_sony_D256: return "ADV_SONY_D256";
		//case ptp_property_sony_D259: return "ADV_SONY_D259";
		// all model has D25A (writable 2 items, [0, 1])
		//case ptp_property_sony_D25A: return "ADV_SONY_D25A";
		//case ptp_property_sony_D25B: return "ADV_SONY_D25B";
		case ptp_property_sony_ZoomSetting: return "ADV_ZoomSetting";
		//case ptp_property_sony_D260: return "ADV_SONY_D260";
		case ptp_property_sony_WirelessFlash: return "ADV_WirelessFlash";
		case ptp_property_sony_RedEyeReduction: return "ADV_RedEyeReduction";
		//case ptp_property_sony_D264: return "ADV_SONY_D264";
		case ptp_property_sony_PCSaveImageSize: return "ADV_PCSaveImageSize";
		case ptp_property_sony_PCSaveImg: return "ADV_PCSaveImg";
		//case ptp_property_sony_D26A: return "ADV_SONY_D26A";
		//case ptp_property_sony_D270: return "ADV_SONY_D270";
		//case ptp_property_sony_D271: return "ADV_SONY_D271";
		//case ptp_property_sony_D272: return "ADV_SONY_D272";
		//case ptp_property_sony_D273: return "ADV_SONY_D273";
		//case ptp_property_sony_D274: return "ADV_SONY_D274";
		//case ptp_property_sony_D275: return "ADV_SONY_D275";
		//case ptp_property_sony_D276: return "ADV_SONY_D276";
		//case ptp_property_sony_D279: return "ADV_SONY_D279";
		//case ptp_property_sony_D27A: return "ADV_SONY_D27A";
		//case ptp_property_sony_D27C: return "ADV_SONY_D27C";
		//case ptp_property_sony_D27D: return "ADV_SONY_D27D";
		//case ptp_property_sony_D27E: return "ADV_SONY_D27E";
		//case ptp_property_sony_D27F: return "ADV_SONY_D27F";
		case ptp_property_sony_HiFrequencyFlicker: return "ADV_HiFrequencyFlicker";
		case ptp_property_sony_TouchFuncInShooting: return "ADV_TouchFuncInShooting";
		case ptp_property_sony_RecFrameRate: return "ADV_RecFrameRate";
		case ptp_property_sony_JPEG_HEIFSwitch: return "ADV_JPEG_HEIFSwitch";
		//case ptp_property_sony_D289: return "ADV_SONY_D289";
		//case ptp_property_sony_D28A: return "ADV_SONY_D28A";
		//case ptp_property_sony_D28B: return "ADV_SONY_D28B";
		//case ptp_property_sony_D28C: return "ADV_SONY_D28C";
		//case ptp_property_sony_D28D: return "ADV_SONY_D28D";
		//case ptp_property_sony_D28E: return "ADV_SONY_D28E";
		//case ptp_property_sony_D28F: return "ADV_SONY_D28F";
		//case ptp_property_sony_D290: return "ADV_SONY_D290";
		case ptp_property_sony_SaveHEIFSize: return "ADV_SaveHEIFSize";
		//case ptp_property_sony_D292: return "ADV_SONY_D292";
		//case ptp_property_sony_D293: return "ADV_SONY_D293";
		//case ptp_property_sony_D294: return "ADV_SONY_D294";
		//case ptp_property_sony_D295: return "ADV_SONY_D295";
		//case ptp_property_sony_D296: return "ADV_SONY_D296";
		// some modern model has D297 (readonly items; [0, 1, 2, 3, 4])
		//case ptp_property_sony_D297: return "ADV_SONY_D297";
		//case ptp_property_sony_D298: return "ADV_SONY_D298";
		// some modern model has D299 (writable 2 items; [0, 1])
		//case ptp_property_sony_D299: return "ADV_SONY_D299";
		case ptp_property_sony_NearFar: return "ADV_NearFar";
		case ptp_property_sony_ZoomState: return "ADV_ZoomState";
		case ptp_property_sony_ZoomRatio: return "ADV_ZoomRatio";
	}
	return ptp_property_code_name(code);
}

char *ptp_property_sony_code_label(uint16_t code) {
	switch (code) {
		case ptp_property_sony_DPCCompensation: return "Flash Compensation";
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
		case ptp_property_sony_PCRemoteSaveDest: return "PC Remote Save Destination";
		case ptp_property_sony_ExposureCompensation: return "Exposure compensation";
		case ptp_property_sony_FocusArea: return "FocusArea";
		case ptp_property_sony_LiveViewDisplay: return "Live View Display";
		case ptp_property_sony_PictureProfile: return "Picture Profile";
		case ptp_property_sony_CreativeStyle: return "Creative Style";
		case ptp_property_sony_MovieFileFormat: return "File Format (movie)";
		case ptp_property_sony_MovieRecordSetting: return "Record Setting (movie)";
		case ptp_property_sony_MemoryCameraSettings: return "Memory (camera settings)";
		case ptp_property_sony_IntervalShoot: return "Interval Shoot";
		case ptp_property_sony_JPEGQuality: return "JPEG Quality";
		case ptp_property_sony_CompressionSetting: return "CompressionSetting";
		case ptp_property_sony_FocusMagnifier: return "Focus Magnifier";
		case ptp_property_sony_AFTrackingSensitivity: return "AF Tracking Sens. (still image)";
		case ptp_property_sony_ZoomSetting: return "Zoom Setting";
		case ptp_property_sony_WirelessFlash: return "Wireless Flash";
		case ptp_property_sony_RedEyeReduction: return "Red Eye Reduction";
		case ptp_property_sony_PCSaveImageSize: return "PC Save Image Size";
		case ptp_property_sony_PCSaveImg: return "RAW+J PC Save Img";
		case ptp_property_sony_HiFrequencyFlicker: return "Hi Frequency flicker";
		case ptp_property_sony_TouchFuncInShooting: return "Touch Func. in Shooting";
		case ptp_property_sony_RecFrameRate: return "Rec Frame Rate";
		case ptp_property_sony_JPEG_HEIFSwitch: return "JPEG/HEIF Switch";
		case ptp_property_sony_Autofocus: return "Autofocus";
		case ptp_property_sony_Capture: return "Capture";
		case ptp_property_sony_Movie: return "Movie";
		case ptp_property_sony_StillImage: return "Still Image";
		case ptp_property_sony_SaveHEIFSize: return "Save HEIF Size";
		case ptp_property_sony_NearFar: return "Near/Far Manual Focus Adjustment";
		case ptp_property_sony_ZoomState: return "Zoom State";
		case ptp_property_sony_ZoomRatio: return "Zoom Ratio";
	}
	return ptp_property_code_label(code);
}

char *ptp_property_sony_value_code_label(indigo_device *device, uint16_t property, uint64_t code) {
	static char label[PTP_MAX_CHARS];
	switch (property) {
		case ptp_property_CompressionSetting: {
			switch (code & 0xFF) {
				case 0x02: return "Standard";
				case 0x03: return "Fine";
				case 0x04: return "Extra Fine";
				case 0x10: return "RAW";
				case 0x12: return "RAW + JPEG (Standard)";
				case 0x13: return "RAW + JPEG";
				case 0x14: return "RAW + JPEG (Extra Fine)";
			}
			break;
		}
		case ptp_property_WhiteBalance: {
			switch (code & 0xFFFF) {
				case 0x0002: return "Auto";
				case 0x0004: return "Daylight";
				case 0x8011: return "Shade";
				case 0x8010: return "Cloudy";
				case 0x0006: return "Incandescent";
				case 0x8001: return "Flourescent warm white";
				case 0x8002: return "Flourescent cool white";
				case 0x8003: return "Flourescent day white";
				case 0x8004: return "Flourescent daylight";
				case 0x0007: return "Flash";
				case 0x8012: return "C.Temp/Filter";
				case 0x8030: return "Underwater";
				case 0x8020: return "Custom 1";
				case 0x8021: return "Custom 2";
				case 0x8022: return "Custom 3";
				case 0x8023: return "Custom 4";
			}
			break;
		}
		case ptp_property_FNumber: {
			switch (code) {
				case SONY_ITERATE_DOWN: return "-";
				case SONY_ITERATE_UP: return "+";
			}
			sprintf(label, "f/%.1f", (int)code / 100.0);
			return label;
		}
		case ptp_property_FocusMode: {
			switch (code & 0xFFFF) {
				case 0x0001: return "MF";
				case 0x0002: return "AF-S";
				case 0x8004: return "AF-C";
				case 0x8005: return "AF-A";
				case 0x8006: return "DMF";
			}
			break;
		}
		case ptp_property_ExposureMeteringMode: {
			switch (code & 0xFF) {
				case 0x01: return "Multi";
				case 0x02: return "Center";
				case 0x03: return "Entire Screen Avg.";
				case 0x04: return "Spot";
				case 0x05: return "Spot (Large)";
				case 0x06: return "Highlight";
			}
			break;
		}
		case ptp_property_FlashMode: {
			switch (code & 0xFFFF) {
				case 0x0000: return "Undefined";
				case 0x0001: return "Automatic flash";
				case 0x0002: return "Flash off";
				case 0x0003: return "Fill flash";
				case 0x0004: return "Automatic Red-eye Reduction";
				case 0x0005: return "Red-eye fill flash";
				case 0x0006: return "External sync";
				case 0x8032: return "Slow Sync";
				case 0x8001: return "Slow Sync";
				case 0x8003: return "Reer Sync";
			}
			break;
		}
		case ptp_property_ExposureProgramMode: {
			switch (code & 0xFFFF) {
				case 0x0001: return "M";
				case 0x0002: return "P";
				case 0x0003: return "A";
				case 0x0004: return "S";
				case 0x0007: return "Portrait";
				case 0x8000: return "Intelligent auto";
				case 0x8001: return "Superior auto";
				case 0x8011: return "Sport";
				case 0x8012: return "Sunset";
				case 0x8013: return "Night scene";
				case 0x8014: return "Landscape";
				case 0x8015: return "Macro";
				case 0x8016: return "Handheld twilight";
				case 0x8017: return "Night portrait";
				case 0x8018: return "Anti motion blur";
				case 0x8050: return "Movie P";
				case 0x8051: return "Movie A";
				case 0x8052: return "Movie S";
				case 0x8053: return "Movie M";
				case 0x8041: return "Sweep panorama";
				case 0x8059: return "Q&S auto";
				case 0x805A: return "Q&S A";
				case 0x805B: return "Q&S S";
				case 0x805C: return "Q&S M";
			}
			break;
		}
		case ptp_property_ExposureBiasCompensation: {
			sprintf(label, "%+.1f", (int)code / 1000.0);
			return label;
		}
		case ptp_property_StillCaptureMode: {
			switch (code & 0xFFFF) {
				case 0x0001: return "Single shooting";
				case 0x0002: return "Cont. shooting Hi";
				case 0x8012: return "Cont. shooting Lo";
				case 0x8013: return "Cont. shooting";
				case 0x8015: return "Cont. shooting Mi";
				case 0x8010: return "Cont. shooting Hi+";
				case 0x8014: return "Spd priority cont.";
				case 0x8005: return "Self-timer 2s";
				case 0x8003: return "Self-timer 5s";
				case 0x8004: return "Self-time 10s";
				case 0x8008: return "Self-timer 10s 3x";
				case 0x8009: return "Self-timer 10s 5x";
				case 0x800c: return "Self-timer 5s 3x";
				case 0x800d: return "Self-timer 5s 5x";
				case 0x800e: return "Self-timer 2s 3x";
				case 0x800f: return "Self-timer 2s 5x";
				case 0xc237: return "Bracket 1/3EV 2x(+) cont.";
				case 0xc23f: return "Bracket 1/3EV 2x(-) cont.";
				case 0x8337: return "Bracket 1/3EV 3x cont.";
				case 0x8537: return "Bracket 1/3EV 5x cont.";
				case 0x8737: return "Bracket 1/3EV 7x cont.";
				case 0x8937: return "Bracket 1/3EV 9x cont.";
				case 0xc257: return "Bracket 1/2EV 2x(+) cont.";
				case 0xc25f: return "Bracket 1/2EV 2x(-) cont.";
				case 0x8357: return "Bracket 1/2EV 3x cont.";
				case 0x8557: return "Bracket 1/2EV 5x cont.";
				case 0x8757: return "Bracket 1/2EV 7x cont.";
				case 0x8957: return "Bracket 1/2EV 9x cont.";
				case 0xc277: return "Bracket 2/3EV 2x(+) cont.";
				case 0xc27f: return "Bracket 2/3EV 2x(-) cont.";
				case 0x8377: return "Bracket 2/3EV 3x cont.";
				case 0x8577: return "Bracket 2/3EV 5x cont.";
				case 0x8777: return "Bracket 2/3EV 7x cont.";
				case 0x8977: return "Bracket 2/3EV 9x cont.";
				case 0xc211: return "Bracket 1EV 2x(+) cont.";
				case 0xc219: return "Bracket 1EV 2x(-) cont.";
				case 0x8311: return "Bracket 1EV 3x cont.";
				case 0x8511: return "Bracket 1EV 5x cont.";
				case 0x8711: return "Bracket 1EV 7x cont.";
				case 0x8911: return "Bracket 1EV 9x cont.";
				case 0xc241: return "Bracket 1.3EV 2x(+) cont.";
				case 0xc249: return "Bracket 1.3EV 2x(-) cont.";
				case 0x8341: return "Bracket 1.3EV 3x cont.";
				case 0x8541: return "Bracket 1.3EV 5x cont.";
				case 0x8741: return "Bracket 1.3EV 7x cont.";
				case 0xc261: return "Bracket 1.5EV 2x(+) cont.";
				case 0xc269: return "Bracket 1.5EV 2x(-) cont.";
				case 0x8361: return "Bracket 1.5EV 3x cont.";
				case 0x8561: return "Bracket 1.5EV 5x cont.";
				case 0x8761: return "Bracket 1.5EV 7x cont.";
				case 0xc281: return "Bracket 1.7EV 2x(+) cont.";
				case 0xc289: return "Bracket 1.7EV 2x(-) cont.";
				case 0x8381: return "Bracket 1.7EV 3x cont.";
				case 0x8581: return "Bracket 1.7EV 5x cont.";
				case 0x8781: return "Bracket 1.7EV 7x cont.";
				case 0xc221: return "Bracket 2EV 2x(+) cont.";
				case 0xc229: return "Bracket 2EV 2x(-) cont.";
				case 0x8321: return "Bracket 2EV 3x cont.";
				case 0x8521: return "Bracket 2EV 5x cont.";
				case 0x8721: return "Bracket 2EV 7x cont.";
				case 0xc251: return "Bracket 2.3EV 2x(+) cont.";
				case 0xc259: return "Bracket 2.3EV 2x(-) cont.";
				case 0x8351: return "Bracket 2.3EV 3x cont.";
				case 0x8551: return "Bracket 2.3EV 5x cont.";
				case 0xc271: return "Bracket 2.5EV 2x(+) cont.";
				case 0xc279: return "Bracket 2.5EV 2x(-) cont.";
				case 0x8371: return "Bracket 2.5EV 3x cont.";
				case 0x8571: return "Bracket 2.5EV 5x cont.";
				case 0xc291: return "Bracket 2.7EV 2x(+) cont.";
				case 0xc299: return "Bracket 2.7EV 2x(-) cont.";
				case 0x8391: return "Bracket 2.7EV 3x cont.";
				case 0x8591: return "Bracket 2.7EV 5x cont.";
				case 0xc231: return "Bracket 3EV 2x(+) cont.";
				case 0xc239: return "Bracket 3EV 2x(-) cont.";
				case 0x8331: return "Bracket 3EV 3x cont.";
				case 0x8531: return "Bracket 3EV 5x cont.";
				case 0xc236: return "Bracket 1/3EV 2x(+)";
				case 0xc23e: return "Bracket 1/3EV 2x(-)";
				case 0x8336: return "Bracket 1/3EV 3x";
				case 0x8536: return "Bracket 1/3EV 5x";
				case 0x8736: return "Bracket 1/3EV 7x";
				case 0x8936: return "Bracket 1/3EV 9x";
				case 0xc256: return "Bracket 1/3EV 2x(+)";
				case 0xc25e: return "Bracket 1/3EV 2x(-)";
				case 0x8356: return "Bracket 1/2EV 3x";
				case 0x8556: return "Bracket 1/2EV 5x";
				case 0x8756: return "Bracket 1/2EV 7x";
				case 0x8956: return "Bracket 1/2EV 9x";
				case 0xc276: return "Bracket 2/3EV 2x(+)";
				case 0xc27e: return "Bracket 2/3EV 2x(-)";
				case 0x8376: return "Bracket 2/3EV 3x";
				case 0x8576: return "Bracket 2/3EV 5x";
				case 0x8776: return "Bracket 2/3EV 7x";
				case 0x8976: return "Bracket 2/3EV 9x";
				case 0xc210: return "Bracket 1EV 2x(+)";
				case 0xc218: return "Bracket 1EV 2x(-)";
				case 0x8310: return "Bracket 1EV 3x";
				case 0x8510: return "Bracket 1EV 5x";
				case 0x8710: return "Bracket 1EV 7x";
				case 0x8910: return "Bracket 1EV 9x";
				case 0xc240: return "Bracket 1.3EV 2x(+)";
				case 0xc248: return "Bracket 1.3EV 2x(-)";
				case 0x8340: return "Bracket 1.3EV 3x";
				case 0x8540: return "Bracket 1.3EV 5x";
				case 0x8740: return "Bracket 1.3EV 7x";
				case 0xc260: return "Bracket 1.5EV 2x(+)";
				case 0xc268: return "Bracket 1.5EV 2x(-)";
				case 0x8360: return "Bracket 1.5EV 3x";
				case 0x8560: return "Bracket 1.5EV 5x";
				case 0x8760: return "Bracket 1.5EV 7x";
				case 0xc280: return "Bracket 1.7EV 2x(+)";
				case 0xc288: return "Bracket 1.7EV 2x(-)";
				case 0x8380: return "Bracket 1.7EV 3x";
				case 0x8580: return "Bracket 1.7EV 5x";
				case 0x8780: return "Bracket 1.7EV 7x";
				case 0xc220: return "Bracket 2EV 2x(+)";
				case 0xc228: return "Bracket 2EV 2x(-)";
				case 0x8320: return "Bracket 2EV 3x";
				case 0x8520: return "Bracket 2EV 5x";
				case 0x8720: return "Bracket 2EV 7x";
				case 0xc250: return "Bracket 2.3EV 2x(+)";
				case 0xc258: return "Bracket 2.3EV 2x(-)";
				case 0x8350: return "Bracket 2.3EV 3x";
				case 0x8550: return "Bracket 2.3EV 5x";
				case 0xc270: return "Bracket 2.5EV 2x(+)";
				case 0xc278: return "Bracket 2.5EV 2x(-)";
				case 0x8370: return "Bracket 2.5EV 3x";
				case 0x8570: return "Bracket 2.5EV 5x";
				case 0xc290: return "Bracket 2.7EV 2x(+)";
				case 0xc298: return "Bracket 2.7EV 2x(-)";
				case 0x8390: return "Bracket 2.7EV 3x";
				case 0x8590: return "Bracket 2.7EV 5x";
				case 0xc230: return "Bracket 3EV 2x(+)";
				case 0xc238: return "Bracket 3EV 2x(-)";
				case 0x8330: return "Bracket 3EV 3x";
				case 0x8530: return "Bracket 3EV 5x";
				case 0x8018: return "Bracket WB Lo";
				case 0x8028: return "Bracket WB Hi";
				case 0x8019: return "Bracket DRO Lo";
				case 0x8029: return "Bracket DRO Hi";
				case 0x8040: return "Focus Bracket";
			}
			break;
		}
		case ptp_property_sony_DPCCompensation: {
			switch (code) {
				case 3000: return "+3.0";
				case 2700: return "+2.7";
				case 2300: return "+2.3";
				case 2000: return "+2.0";
				case 1700: return "+1.7";
				case 1300: return "+1.3";
				case 1000: return "+1.0";
				case 700: return "+0.7";
				case 300: return "+0.3";
				case 0: return "0.0";
				case -300: return "-0.3";
				case -700: return "-0.7";
				case -1000: return "-1.0";
				case -1300: return "-1.3";
				case -1700: return "-1.7";
				case -2000: return "-2.0";
				case -2300: return "-2.3";
				case -2700: return "-2.7";
				case -3000: return "-3.0";
			}
			break;
		}
		case ptp_property_sony_DRangeOptimize: {
			switch (code & 0xFF) {
				case 0x01: return "Off";
				case 0x1f: return "DRO:Auto";
				case 0x11: return "DRO:Lv1";
				case 0x12: return "DRO:Lv2";
				case 0x13: return "DRO:Lv3";
				case 0x14: return "DRO:Lv4";
				case 0x15: return "DRO:Lv5";
				case 0x20: return "HDR:Auto";
				case 0x21: return "HDR:1.0EV";
				case 0x22: return "HDR:2.0EV";
				case 0x23: return "HDR:3.0EV";
				case 0x24: return "HDR:4.0EV";
				case 0x25: return "HDR:5.0EV";
				case 0x26: return "HDR:6.0EV";
			}
			break;
		}
		case ptp_property_sony_ImageSize: {
			switch (code & 0xFF) {
				case 0x01: return "Large";
				case 0x02: return "Medium";
				case 0x03: return "Small";
			}
			break;
		}
		case ptp_property_sony_ShutterSpeed: {
			switch (code) {
				case SONY_ITERATE_DOWN: return "-";
				case SONY_ITERATE_UP: return "+";
				case 0: return "Bulb";
			}
			int a = (int)code >> 16;
			int b = (int)code & 0xFFFF;
			if (b == 10)
				sprintf(label, "%g\"", (double)a / b);
			else
				sprintf(label, "1/%d", b);
			return label;
		}
		case ptp_property_sony_AspectRatio: {
			switch (code & 0xFF) {
				case 0x01: return "3:2";
				case 0x02: return "16:9";
				case 0x03: return "4:3";
				case 0x04: return "1:1";
			}
			break;
		}
		case ptp_property_sony_SensorCrop: {
			switch (code & 0xFF) {
				case 0x01: return "OFF";
				case 0x02: return "ON";
			}
			break;
		}
		case ptp_property_sony_PictureEffect: {
			switch (code & 0xFFFF) {
				case 0x8000: return "Off";
				case 0x8001: return "Toy camera - normal";
				case 0x8002: return "Toy camera - cool";
				case 0x8003: return "Toy camera - warm";
				case 0x8004: return "Toy camera - green";
				case 0x8005: return "Toy camera - magenta";
				case 0x8010: return "Pop Color";
				case 0x8020: return "Posterisation B/W";
				case 0x8021: return "Posterisation Color";
				case 0x8030: return "Retro";
				case 0x8040: return "Soft high key";
				case 0x8050: return "Partial color - red";
				case 0x8051: return "Partial color - green";
				case 0x8052: return "Partial color - blue";
				case 0x8053: return "Partial color - yellow";
				case 0x8060: return "High contrast mono";
				case 0x8070: return "Soft focus - low";
				case 0x8071: return "Soft focus - mid";
				case 0x8072: return "Soft focus - high";
				case 0x8080: return "HDR painting - low";
				case 0x8081: return "HDR painting - mid";
				case 0x8082: return "HDR painting - high";
				case 0x8090: return "Rich tone mono";
				case 0x80A0: return "Miniature - auto";
				case 0x80A1: return "Miniature - top";
				case 0x80A2: return "Miniature - middle horizontal";
				case 0x80A3: return "Miniature - bottom";
				case 0x80A4: return "Miniature - right";
				case 0x80A5: return "Miniature - middle vertical";
				case 0x80A6: return "Miniature - left";
				case 0x80B0: return "Watercolor";
				case 0x80C0: return "Illustration - low";
				case 0x80C1: return "Illustration - mid";
				case 0x80C2: return "Illustration - high";
			}
			break;
		}
		case ptp_property_sony_ISO: {
			switch (code) {
				case SONY_ITERATE_DOWN: return "-";
				case SONY_ITERATE_UP: return "+";
				case SONY_ISO_AUTO: return "Auto";
			}
			sprintf(label, "%d", (int)(code & 0xffffff));
			return label;
		}
		case ptp_property_sony_PCRemoteSaveDest: {
			switch (code & 0xFF) {
				case 0x01: return "PC";
				case 0x10: return "Camera";
				case 0x11: return "PC+Camera";
			}
			break;
		}
		case ptp_property_sony_FocusArea: {
			switch (code & 0xFFFF) {
				case 0x0001: return "Wide";
				case 0x0002: return "Zone";
				case 0x0003: return "Center";
				case 0x0101: return "Flexible Spot (S)";
				case 0x0102: return "Flexible Spot (M)";
				case 0x0103: return "Flexible Spot (L)";
				case 0x0104: return "Expand Flexible Spot";
				case 0x0201: return "Tracking: Wide";
				case 0x0202: return "Tracking: Zone";
				case 0x0203: return "Tracking: Center";
				case 0x0204: return "Tracking: Flexible Spot (S)";
				case 0x0205: return "Tracking: Flexible Spot (M)";
				case 0x0206: return "Tracking: Flexible Spot (L)";
				case 0x0207: return "Tracking: Expand Flexible Spot";
			}
			break;
		}
		case ptp_property_sony_LiveViewDisplay: {
			switch (code) {
				case 0x01: return "Setting Effect ON";
				case 0x02: return "Setting Effect OFF";
			}
			break;
		}
		case ptp_property_sony_PictureProfile: {
			switch (code & 0xFF) {
				case 0x00: return "None";
				case 0x01: return "PP1";
				case 0x02: return "PP2";
				case 0x03: return "PP3";
				case 0x04: return "PP4";
				case 0x05: return "PP5";
				case 0x06: return "PP6";
				case 0x07: return "PP7";
				case 0x08: return "PP8";
				case 0x09: return "PP9";
				case 0x0A: return "PP10";
				case 0x0B: return "PP11";
			}
			break;
		}
		case ptp_property_sony_CreativeStyle: {
			switch (code & 0xFF) {
				case 0x01: return "Standard";
				case 0x02: return "Vivid";
				case 0x03: return "Portrait";
				case 0x04: return "Landscape";
				case 0x05: return "Sunset";
				case 0x06: return "Black/White";
				case 0x07: return "Light";
				case 0x08: return "Neutral";
				case 0x09: return "Clear";
				case 0x0A: return "Deep";
				case 0x0B: return "Night Scene";
				case 0x0C: return "Autumn leaves";
				case 0x0D: return "Sepia";
				case 0x0E: return "Custom 1";
				case 0x0F: return "Custom 2";
				case 0x10: return "Custom 3";
				case 0x11: return "Custom 4";
				case 0x12: return "Custom 5";
				case 0x13: return "Custom 6";
			}
			break;
		}
		case ptp_property_sony_MovieFileFormat: {
			switch (code & 0xFF) {
				case 0x03: return "AVCHD";
				case 0x08: return "XAVC S 4K";
				case 0x09: return "XAVC S HD";
				case 0x0A: return "XAVC HS 8K";
				case 0x0B: return "XAVC HS 4K";
				case 0x0C: return "XAVC S 4K";
				case 0x0D: return "XAVC S HD";
				case 0x0E: return "XAVC S-I 4K";
				case 0x0F: return "XAVC S-I HD";
			}
			break;
		}
		case ptp_property_sony_MovieRecordSetting: {
			switch (code & 0xFF) {
				case 0x01: return "60p 50M";
				case 0x02: return "30p 50M";
				case 0x03: return "24p 50M";
				case 0x06: return "60i 24M(FX)";
				case 0x08: return "60i 17M(FH)";
				case 0x18: return "60p 25M";
				case 0x1a: return "30p 16M";
				case 0x1c: return "120p 100M";
				case 0x1e: return "120p 60M";
				case 0x20: return "30p 100M";
				case 0x22: return "24p 100M";
				case 0x23: return "30p 60M";
				case 0x25: return "24p 60M";
				case 0x38: return "100M 4:2:0 8bit";
				case 0x3c: return "60M 4:2:0 8bit";
				case 0x3d: return "50M 4:2:2 10bit";
				case 0x3f: return "50M 4:2:0 8bit";
				case 0x42: return "25M 4:2:0 8bit";
				case 0x43: return "50M 4:2:0 8bit";
			}
			break;
		}
		case ptp_property_sony_MemoryCameraSettings: {
			switch (code & 0xFF) {
				case 0x00: return "Not selected";
				case 0x01: return "1";
				case 0x02: return "2";
				case 0x03: return "3";
				case 0x11: return "M1";
				case 0x12: return "M2";
				case 0x13: return "M3";
				case 0x14: return "M4";
			}
			break;
		}
		case ptp_property_sony_IntervalShoot: {
			switch (code & 0xFF) {
				case 0x01: return "Off";
				case 0x02: return "On";
			}
			break;
		}
		case ptp_property_sony_JPEGQuality: {
			switch (code & 0xFF) {
				case 0x01: return "X.FINE";
				case 0x02: return "FINE";
				case 0x03: return "STD";
				case 0x04: return "LIGHT";
			}
			break;
		}
		case ptp_property_sony_CompressionSetting: {
			switch (code & 0xFF) {
				case 0x01: return "RAW";
				case 0x02: return "RAW+JPEG";
				case 0x03: return "JPEG";
				case 0x04: return "RAW+HEIF";
				case 0x05: return "HEIF";
			}
			break;
		}
		case ptp_property_sony_FocusMagnifier: {
			switch (code & 0xFFFFFFFFFF) {
				case 0x00ffffffff: return "None";
				case 0x0affffffff: return "x1.0";
				case 0x15ffffffff: return "x2.1";
				case 0x29ffffffff: return "x4.1";
				case 0x2affffffff: return "x4.2";
				case 0x2fffffffff: return "x4.7";
				case 0x37ffffffff: return "x5.5";
				case 0x3bffffffff: return "x5.9";
				case 0x3dffffffff: return "x6.1";
				case 0x55ffffffff: return "x8.5";
				case 0x5dffffffff: return "x9.3";
				case 0x5effffffff: return "x9.4";
				case 0x6effffffff: return "x11.0";
				case 0x75ffffffff: return "x11.7";
				case 0x79ffffffff: return "x12.1";
			}
			break;
		}
		case ptp_property_sony_AFTrackingSensitivity: {
			switch (code & 0xFF) {
				case 0x01: return "1(Locked on)";
				case 0x02: return "2";
				case 0x03: return "3(Standard)";
				case 0x04: return "4";
				case 0x05: return "5(Responsive)";
			}
			break;
		}
		case ptp_property_sony_TouchFuncInShooting: {
			switch (code & 0xFF) {
				case 0x01: return "Off";
				case 0x02: return "Touch Shutter";
				case 0x03: return "Touch Focus";
				case 0x04: return "Touch Tracking";
				case 0x05: return "Touch AE";
				case 0x06: return "Touch Shutter+AE On";
				case 0x07: return "Touch Shutter+AE Off";
				case 0x08: return "Touch Focus+AE On";
				case 0x09: return "Touch Focus+AE Off";
				case 0x0A: return "Touch Tracking+AE On";
				case 0x0B: return "Touch Tracking+AE Off";
			}
			break;
		}
		case ptp_property_sony_ZoomSetting: {
			switch (code & 0xFF) {
				case 0x01: return "Optical zoom only";
				case 0x03: return "ClearImage Zoom";
				case 0x04: return "Digital Zoom";
			}
			break;
		}
		case ptp_property_sony_WirelessFlash: {
			switch (code & 0xFF) {
				case 0x00: return "Off";
				case 0x01: return "On";
			}
			break;
		}
		case ptp_property_sony_RedEyeReduction: {
			switch (code & 0xFF) {
				case 0x00: return "Off";
				case 0x01: return "On";
			}
			break;
		}
		case ptp_property_sony_PCSaveImageSize: {
			switch (code & 0xFF) {
				case 0x01: return "Original";
				case 0x02: return "2M";
			}
			break;
		}
		case ptp_property_sony_PCSaveImg: {
			switch (code & 0xFF) {
				case 0x01: return "RAW+JPEG";
				case 0x02: return "JPEG";
				case 0x03: return "RAW";
				case 0x04: return "RAW+HEIF";
				case 0x05: return "HEIF";
			}
			break;
		}
		case ptp_property_sony_HiFrequencyFlicker: {
			switch (code & 0xFF) {
				case 0x00: return "Off";
				case 0x01: return "On";
			}
			break;
		}
		case ptp_property_sony_RecFrameRate: {
			switch (code & 0xFF) {
				case 0x01: return "120p";
				case 0x03: return "60p";
				case 0x05: return "30p";
				case 0x07: return "24p";
			}
			break;
		}
		case ptp_property_sony_JPEG_HEIFSwitch: {
			switch (code & 0xFF) {
				case 0x01: return "JPEG";
				case 0x02: return "HEIF(4:2:2)";
				case 0x03: return "HEIF(4:2:0)";
			}
			break;
		}
		case ptp_property_sony_SaveHEIFSize: {
			switch (code & 0xFF) {
				case 0x01: return "Large Size";
				case 0x02: return "Small Size";
			}
			break;
		}
		case ptp_property_sony_ZoomState: {
			switch (code & 0xFF) {
				case 0x00: return "Normal";
				case 0x01: return "x1.0";
				case 0x02: return "Zooming";
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
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Bad type: 0x%x", target->type);
			return NULL;
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
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Bad type: 0x%x", target->type);
					return NULL;
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
					case ptp_uint64_type: {
						uint64_t value;
						source = ptp_decode_uint64(source, &value);
						target->value.sw.values[i] = value;
						break;
					}
					case ptp_int64_type: {
						int64_t value;
						source = ptp_decode_uint64(source, (uint64_t *)&value);
						target->value.sw.values[i] = value;
						break;
					}
					case ptp_str_type: {
						if (i < 16)
							source = ptp_decode_string(source, target->value.sw_str.values[i]);
						break;
					}
					default:
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Bad type: 0x%x", target->type);
						return NULL;
				}
			}
			break;
		}
	}
	if (SONY_PRIVATE_DATA->api_version == SONY_NEW_API) {
		// Second enum part (some item cannot set caused by current device state)
		// e.g. ILCE-7RM4's ptp_property_FocusMode (0x500a)
		//   1st enum:
		//     count=1
		//     values:
		//     - 0x0001
		//   2nd enum:
		//     count=5
		//     - 0x0002
		//     - 0x8005
		//     - 0x8004
		//     - 0x8006
		//     - 0x0001
		switch (target->form) {
			case ptp_none_form:
			case ptp_range_form:
				break;
			case ptp_enum_form: {
				uint16_t count;
				source = ptp_decode_uint16(source, &count);
				if (count != target->count) {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Different count: %d != %d @ 0x%x", count, target->count, code);
				}
				switch (target->type) {
					case ptp_uint8_type:
					case ptp_int8_type:
						source += sizeof(uint8_t) * count;
						break;
					case ptp_uint16_type:
					case ptp_int16_type:
						source += sizeof(uint16_t) * count;
						break;
					case ptp_uint32_type:
					case ptp_int32_type:
						source += sizeof(uint32_t) * count;
						break;
					case ptp_uint64_type:
					case ptp_int64_type:
						source += sizeof(uint64_t) * count;
						break;
					case ptp_str_type: {
						char tmp[PTP_MAX_CHARS];
						for (int i = 0; i < count; i++) {
							if (i < 16)
								source = ptp_decode_string(source, tmp);
							}
						}
						break;
					default:
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Bad type: 0x%x", target->type);
						return NULL;
					}
				}
				break;
			}
	}
	uint64_t mode = SONY_PRIVATE_DATA->mode;
	if (SONY_PRIVATE_DATA->api_version != SONY_NEW_API) {
		// before ILCE-7RM4
		switch (code) {
			case ptp_property_sony_ImageSize:
				target->count = 3;
				break;
			case ptp_property_sony_ISO:
				target->count = 3;
				target->value.sw.values[0] = SONY_ITERATE_DOWN;
				target->value.sw.values[1] = target->value.number.value;
				target->value.sw.values[2] = SONY_ITERATE_UP;
				target->writable = mode == 1 || mode == 2 || mode == 3 || mode == 4 || mode == 0x8050 || mode == 0x8051 || mode == 0x8052 || mode == 0x8053;
				break;
			case ptp_property_sony_ShutterSpeed:
				target->count = 3;
				target->value.sw.values[0] = SONY_ITERATE_DOWN;
				target->value.sw.values[1] = target->value.number.value;
				target->value.sw.values[2] = SONY_ITERATE_UP;
				target->writable = mode == 1 || mode == 4 || mode == 0x8052 || mode == 0x8053;
				SONY_PRIVATE_DATA->shutter_speed = target->value.number.value;
				break;
			case ptp_property_FNumber:
				target->count = 3;
				target->value.sw.values[0] = SONY_ITERATE_DOWN;
				target->value.sw.values[1] = target->value.number.value;
				target->value.sw.values[2] = SONY_ITERATE_UP;
				target->writable = mode == 1 ||  mode == 3 || mode == 0x8051 || mode == 0x8053;
				break;
		}
	} else {
		switch (code) {
			case ptp_property_sony_ISO:
				if (target->count == 0) {
					target->count = 1;
					target->value.sw.values[1] = target->value.number.value;
					target->writable = false;
				} else {
					target->writable = mode == 1 || mode == 2 || mode == 3 || mode == 4 || mode == 0x8050 || mode == 0x8051 || mode == 0x8052 || mode == 0x8053;
				}
				break;
			case ptp_property_sony_ShutterSpeed:
				if (target->count == 0) {
					target->count = 1;
					target->value.sw.values[1] = target->value.number.value;
					target->writable = false;
				} else {
					target->writable = mode == 1 || mode == 4 || mode == 0x8052 || mode == 0x8053;
				}
				SONY_PRIVATE_DATA->shutter_speed = target->value.number.value;
				break;
			case ptp_property_FNumber:
				if (target->count == 0) {
					target->count = 1;
					target->value.sw.values[1] = target->value.number.value;
					target->writable = false;
				} else {
					target->writable = mode == 1 ||  mode == 3 || mode == 0x8051 || mode == 0x8053;
				}
				break;
		}
	}
	switch (code) {
		case ptp_property_ExposureBiasCompensation: // never really writable and sometimes not a list
			target->count = 1;
			target->value.sw.values[0] = target->value.number.value;
			target->writable = false;
			break;
		case ptp_property_FocusMode:
			target->writable = mode == 1 || mode == 2 || mode == 3 || mode == 4 || mode == 0x8050 || mode == 0x8051 || mode == 0x8052 || mode == 0x8053;
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
			int mode = target->value.sw.value & 0xFFFF;
			if (SONY_PRIVATE_DATA->mode != mode) {
				SONY_PRIVATE_DATA->mode = mode;
				ptp_sony_handle_event(device, (ptp_event_code)ptp_event_sony_PropertyChanged, NULL);
			}
			break;
		case ptp_property_sony_FocusStatus:
			SONY_PRIVATE_DATA->focus_state = target->value.number.value;
			break;
		case ptp_property_CompressionSetting:
			SONY_PRIVATE_DATA->is_dual_compression = target-> value.number.value == 18 || target-> value.number.value == 19 || target-> value.number.value == 20;
			break;
		case ptp_property_sony_CompressionSetting:
			SONY_PRIVATE_DATA->is_dual_compression = target->value.number.value == 2;
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
		ptp_sony_handle_event(device, event.code, event.payload.params);
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
		if (PRIVATE_DATA->model.product >= SONY_NEW_API_SUPPORT_PRODUCT_ID_BORDER) {
			SONY_PRIVATE_DATA->api_version = SONY_NEW_API;
			indigo_log("0x%04x >= 0x%04x new API used", PRIVATE_DATA->model.product, SONY_NEW_API_SUPPORT_PRODUCT_ID_BORDER);
		} else {
			SONY_PRIVATE_DATA->api_version = SONY_OLD_API;
			indigo_log("0x%04x < 0x%04x old API used", PRIVATE_DATA->model.product, SONY_NEW_API_SUPPORT_PRODUCT_ID_BORDER);
		}
		if (PRIVATE_DATA->model.product == SONY_ILCE_7RM4_PRODUCT_ID || PRIVATE_DATA->model.product == SONY_ILCE_7RM4A_PRODUCT_ID)
			SONY_PRIVATE_DATA->needs_pre_capture_delay = true;
		else
			SONY_PRIVATE_DATA->needs_pre_capture_delay = false;
		if (ptp_transaction_1_0_i(device, ptp_operation_sony_GetSDIOGetExtDeviceInfo, SONY_PRIVATE_DATA->api_version, &buffer, &size)) {
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
					if (source == NULL) {
						break;
					}
				}
				free(buffer);
				buffer = NULL;
			}
		}
		if (buffer) {
			free(buffer);
		}
	}
	indigo_set_timer(device, 0.5, ptp_check_event, &PRIVATE_DATA->event_checker);
	struct timespec now;
	clock_gettime(CLOCK_REALTIME, &now);
	SONY_PRIVATE_DATA->connection_time = now.tv_sec;
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
					if (source == NULL) {
						break;
					}
				}
			}
			free(buffer);
			buffer = NULL;
			return true;
		}
		case ptp_event_sony_ObjectAdded: {
			void *buffer = NULL;
			if (ptp_transaction_1_0_i(device, ptp_operation_GetObjectInfo, params[0], &buffer, NULL) && buffer) {
				uint32_t size;
				char filename[PTP_MAX_CHARS];
				memset(&filename, 0, PTP_MAX_CHARS);
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
						if (PRIVATE_DATA->image_buffer) {
							free(PRIVATE_DATA->image_buffer);
						}
						PRIVATE_DATA->image_buffer = buffer;
						buffer = NULL;
					}
				}
			}
			if (buffer) {
				free(buffer);
			}
			return true;
		}
	}
	return ptp_handle_event(device, code, params);
}

bool ptp_sony_set_property(indigo_device *device, ptp_property *property) {
	if (SONY_PRIVATE_DATA->api_version != SONY_NEW_API) {
		switch (property->code) {
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
		}
	}
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
	return false;
}

bool ptp_sony_exposure(indigo_device *device) {
	if (SONY_PRIVATE_DATA->needs_pre_capture_delay) {
		// A7R4/A7R4A needs 3s delay before first capture
		struct timespec now;
		clock_gettime(CLOCK_REALTIME, &now);
		if (now.tv_sec - SONY_PRIVATE_DATA->connection_time < 3) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "enforce 3s delay...");
			while (true) {
				indigo_usleep(100000);
				clock_gettime(CLOCK_REALTIME, &now);
				if (now.tv_sec - SONY_PRIVATE_DATA->connection_time > 3) {
					break;
				}
				if (PRIVATE_DATA->abort_capture)
					return false;
			}
		}
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
	if (SONY_PRIVATE_DATA->api_version != SONY_NEW_API && SONY_PRIVATE_DATA->shutter_speed == 0) {
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
		if (SONY_PRIVATE_DATA->api_version == SONY_NEW_API) {
			// readout memory buffer
			while (true) {
				// NOTE: DO NOT ABORT HERE
				// The image remains in the memory buffer as long as the object is not read out.
				ptp_property *property = ptp_property_supported(device, ptp_property_sony_ObjectInMemory);
				if (property) {
					if (property->value.number.value > 0x8000) {
						// CaptureCompleted
						uint32_t dummy[1] = { 0xffffc001 };
						ptp_sony_handle_event(device, (ptp_event_code)ptp_event_sony_ObjectAdded, dummy);
						break;
					}
					indigo_usleep(100000);
				}
			}
		} else {
			bool complete_detected = false;
			while (true) {
				if (PRIVATE_DATA->abort_capture || (CCD_IMAGE_PROPERTY->state != INDIGO_BUSY_STATE && CCD_PREVIEW_IMAGE_PROPERTY->state != INDIGO_BUSY_STATE && CCD_IMAGE_FILE_PROPERTY->state != INDIGO_BUSY_STATE)) {
					break;
				}
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
	if (SONY_PRIVATE_DATA->needs_pre_capture_delay) {
		// A7R4/A7R4A needs 3s delay before first capture
		struct timespec now;
		clock_gettime(CLOCK_REALTIME, &now);
		if (now.tv_sec - SONY_PRIVATE_DATA->connection_time < 3) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "enforce 3s delay...");
			while (true) {
				indigo_usleep(100000);
				clock_gettime(CLOCK_REALTIME, &now);
				if (now.tv_sec - SONY_PRIVATE_DATA->connection_time > 3) {
					break;
				}
				if (PRIVATE_DATA->abort_capture)
					return false;
			}
		}
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
							if (PRIVATE_DATA->image_buffer) {
								free(PRIVATE_DATA->image_buffer);
							}
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
		if (buffer) {
			free(buffer);
		}
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
			int16_t value = SONY_PRIVATE_DATA->steps <= -1 ? -1 : 1;
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
