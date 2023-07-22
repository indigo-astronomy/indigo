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
	ptp_property_sony_BatteryLevel = 0xD218,
	ptp_property_sony_SensorCrop = 0xD219,
	ptp_property_sony_PictureEffect = 0xD21B,
	ptp_property_sony_ABFilter = 0xD21C,
	ptp_property_sony_ISO = 0xD21E,
	ptp_property_sony_PCRemoteSaveDest = 0xD222,
	ptp_property_sony_ExposureCompensation = 0xD224,
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
