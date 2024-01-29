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

#ifndef indigo_ptp_sony_h
#define indigo_ptp_sony_h

#include <indigo/indigo_driver.h>

typedef enum {
	ptp_operation_sony_SDIOConnect = 0x9201,
	ptp_operation_sony_GetSDIOGetExtDeviceInfo = 0x9202,
	ptp_operation_sony_GetDevicePropDesc = 0x9203,
	ptp_operation_sony_GetDevicePropertyValue = 0x9204,
	ptp_operation_sony_SetControlDeviceA = 0x9205,
	ptp_operation_sony_GetControlDeviceDesc = 0x9206,
	ptp_operation_sony_SetControlDeviceB = 0x9207,
	ptp_operation_sony_GetAllDevicePropData = 0x9209,
} ptp_operation_sony_code;

typedef enum {
	ptp_event_sony_ObjectAdded = 0xC201,
	ptp_event_sony_ObjectRemoved = 0xC202,
	ptp_event_sony_PropertyChanged = 0xC203,
} ptp_event_sony_code;

typedef enum {
	ptp_property_sony_DPCCompensation = 0xD200,
	ptp_property_sony_DRangeOptimize = 0xD201,
	ptp_property_sony_ImageSize = 0xD203,
	ptp_property_sony_ShutterSpeed = 0xD20D,
	ptp_property_sony_ColorTemp = 0xD20F,
	ptp_property_sony_CCFilter = 0xD210,
	ptp_property_sony_AspectRatio = 0xD211,
	ptp_property_sony_FocusStatus = 0xD213,
	ptp_property_sony_Zoom = 0xD214,
	ptp_property_sony_ObjectInMemory = 0xD215,
	ptp_property_sony_ExposeIndex = 0xD216,
	ptp_property_sony_D217 = 0xD217,
	ptp_property_sony_BatteryLevel = 0xD218,
	ptp_property_sony_SensorCrop = 0xD219,
	ptp_property_sony_PictureEffect = 0xD21B,
	ptp_property_sony_ABFilter = 0xD21C,
	ptp_property_sony_ISO = 0xD21E,
	ptp_property_sony_D21F = 0xD21F,
	ptp_property_sony_D221 = 0xD221,
	ptp_property_sony_PCRemoteSaveDest = 0xD222,
	ptp_property_sony_ExposureCompensation = 0xD224,
	ptp_property_sony_FocusArea = 0xD22C,
	ptp_property_sony_LiveViewDisplay = 0xD231,
	ptp_property_sony_D239 = 0xD239,
	ptp_property_sony_D23A = 0xD23a,
	ptp_property_sony_PictureProfile = 0xD23F,
	ptp_property_sony_CreativeStyle = 0xD240,
	ptp_property_sony_MovieFileFormat = 0xD241,
	ptp_property_sony_MovieRecordSetting = 0xD242,
	ptp_property_sony_D243 = 0xD243,
	ptp_property_sony_D244 = 0xD244,
	ptp_property_sony_D245 = 0xD245,
	ptp_property_sony_D246 = 0xD246,
	ptp_property_sony_MemoryCameraSettings = 0xD247,
	ptp_property_sony_D248 = 0xD248,
	ptp_property_sony_IntervalShoot = 0xD24F,
	ptp_property_sony_JPEGQuality = 0xD252,
	ptp_property_sony_CompressionSetting = 0xD253,
	ptp_property_sony_FocusMagnifier = 0xD254,
	ptp_property_sony_AFTrackingSensitivity = 0xD255,
	ptp_property_sony_D256 = 0xD256,
	ptp_property_sony_D259 = 0xD259,
	ptp_property_sony_D25A = 0xD25A,
	ptp_property_sony_D25B = 0xD25B,
	ptp_property_sony_ZoomSetting = 0xD25F,
	ptp_property_sony_D260 = 0xD260,
	ptp_property_sony_WirelessFlash = 0xD262,
	ptp_property_sony_RedEyeReduction = 0xD263,
	ptp_property_sony_D264 = 0xD264,
	ptp_property_sony_PCSaveImageSize = 0xD268,
	ptp_property_sony_PCSaveImg = 0xD269,
	ptp_property_sony_D26A = 0xD26A,
	ptp_property_sony_D270 = 0xD270,
	ptp_property_sony_D271 = 0xD271,
	ptp_property_sony_D272 = 0xD272,
	ptp_property_sony_D273 = 0xD273,
	ptp_property_sony_D274 = 0xD274,
	ptp_property_sony_D275 = 0xD275,
	ptp_property_sony_D276 = 0xD276,
	ptp_property_sony_D279 = 0xD279,
	ptp_property_sony_D27A = 0xD27A,
	ptp_property_sony_D27C = 0xD27C,
	ptp_property_sony_D27D = 0xD27D,
	ptp_property_sony_D27E = 0xD27E,
	ptp_property_sony_D27F = 0xD27F,
	ptp_property_sony_HiFrequencyFlicker = 0xD281,
	ptp_property_sony_TouchFuncInShooting = 0xD283,
	ptp_property_sony_RecFrameRate = 0xD286,
	ptp_property_sony_JPEG_HEIFSwitch = 0xD287,
	ptp_property_sony_D289 = 0xD289,
	ptp_property_sony_D28A = 0xD28A,
	ptp_property_sony_D28B = 0xD28B,
	ptp_property_sony_D28C = 0xD28C,
	ptp_property_sony_D28D = 0xD28D,
	ptp_property_sony_D28E = 0xD28E,
	ptp_property_sony_D28F = 0xD28F,
	ptp_property_sony_SaveHEIFSize = 0xD291,
	ptp_property_sony_D293 = 0xD293,
	ptp_property_sony_D294 = 0xD294,
	ptp_property_sony_D295 = 0xD295,
	ptp_property_sony_D296 = 0xD296,
	ptp_property_sony_D297 = 0xD297,
	ptp_property_sony_D298 = 0xD298,
	ptp_property_sony_D299 = 0xD299,
	ptp_property_sony_Autofocus = 0xD2C1,
	ptp_property_sony_Capture = 0xD2C2,
	ptp_property_sony_Movie = 0xD2C8,
	ptp_property_sony_StillImage = 0xD2C7,
	ptp_property_sony_NearFar = 0xD2D1,
	ptp_property_sony_ZoomState = 0xD22D,
	ptp_property_sony_ZoomRatio = 0xD22F,
} ptp_property_sony_code;

typedef struct {
	uint64_t mode;
	uint64_t focus_state;
	uint64_t focus_mode;
	uint64_t shutter_speed;
	bool is_dual_compression;
	bool did_capture;
	bool did_liveview;
	int steps;
	uint32_t api_version;
	bool needs_pre_capture_delay;
} sony_private_data;


extern char *ptp_operation_sony_code_label(uint16_t code);
extern char *ptp_event_sony_code_label(uint16_t code);
extern char *ptp_property_sony_code_name(uint16_t code);
extern char *ptp_property_sony_code_label(uint16_t code);
extern char *ptp_property_sony_value_code_label(indigo_device *device, uint16_t property, uint64_t code);

extern bool ptp_sony_initialise(indigo_device *device);
extern bool ptp_sony_handle_event(indigo_device *device, ptp_event_code code, uint32_t *params);
extern bool ptp_sony_set_property(indigo_device *device, ptp_property *property);
extern bool ptp_sony_exposure(indigo_device *device);
extern bool ptp_sony_liveview(indigo_device *device);
extern bool ptp_sony_af(indigo_device *device);
extern bool ptp_sony_focus(indigo_device *device, int steps);
extern bool ptp_sony_check_dual_compression(indigo_device *device);

#endif /* indigo_ptp_sony_h */
