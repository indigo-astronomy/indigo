// Copyright (c) 2026 CloudMakers, s. r. o.
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

/** INDIGO PTP Olympus/OM System implementation
 \file indigo_ptp_olympus.h
 */

#ifndef indigo_ptp_olympus_h
#define indigo_ptp_olympus_h

#include <indigo/indigo_driver.h>

// Olympus OMD vendor extension (also used by OM Digital Solutions bodies),
// codes as documented by libgphoto2 camlibs/ptp2/ptp.h

// the OM-1 answers every request within milliseconds or not at all, so the
// stock 10s PTP_TIMEOUT only delays wedge detection and recovery (libusb only)
#define OLYMPUS_PTP_TIMEOUT	3000

typedef enum {
	ptp_operation_olympus_Capture = 0x9481,
	ptp_operation_olympus_GetDateTime = 0x9482,
	ptp_operation_olympus_GetLiveViewImage = 0x9484,
	ptp_operation_olympus_GetImage = 0x9485,
	ptp_operation_olympus_ChangedProperties = 0x9486,
	ptp_operation_olympus_MFDrive = 0x9487,
	ptp_operation_olympus_SetProperties = 0x9489
} ptp_operation_olympus_code;

typedef enum {
	ptp_event_olympus_ObjectAddedLegacy = 0xC002,
	ptp_event_olympus_DevicePropChangedLegacy = 0xC008,
	ptp_event_olympus_CreateRecView = 0xC101,
	ptp_event_olympus_ObjectAdded = 0xC102,
	ptp_event_olympus_CaptureComplete = 0xC103,
	ptp_event_olympus_DevicePropChanged = 0xC108
} ptp_event_olympus_code;

typedef enum {
	ptp_property_olympus_Aperture = 0xD002,
	ptp_property_olympus_FocusMode = 0xD003,
	ptp_property_olympus_ExposureMeteringMode = 0xD004,
	ptp_property_olympus_ISO = 0xD007,
	ptp_property_olympus_ExposureCompensation = 0xD008,
	ptp_property_olympus_DriveMode = 0xD009,
	ptp_property_olympus_ExposureProgram = 0xD00C,
	ptp_property_olympus_ImageFormat = 0xD00D,
	ptp_property_olympus_ColorTemperature = 0xD00E,
	ptp_property_olympus_ExposureBias = 0xD00F,
	ptp_property_olympus_FaceDetection = 0xD01A,
	ptp_property_olympus_AspectRatio = 0xD01B,
	ptp_property_olympus_Shutterspeed = 0xD01C,
	ptp_property_olympus_WhiteBalance = 0xD01E,
	ptp_property_olympus_AFArea = 0xD051,
	ptp_property_olympus_CameraControlMode = 0xD052,
	ptp_property_olympus_LiveViewModeOM = 0xD06D,
	ptp_property_olympus_CaptureTarget = 0xD0DC,
	ptp_property_olympus_ISOSensitivity = 0xD1C0
} ptp_property_olympus_code;

typedef struct {
	bool is_dual_compression;
	uint32_t last_changed_checksum;
	int forced_refresh_countdown;
	int last_usb_error;
	char last_object_name[256];
	uint32_t last_object_size;
} olympus_private_data;


extern char *ptp_operation_olympus_code_label(uint16_t code);
extern char *ptp_event_olympus_code_label(uint16_t code);
extern char *ptp_property_olympus_code_name(uint16_t code);
extern char *ptp_property_olympus_code_label(uint16_t code);
extern char *ptp_property_olympus_value_code_label(indigo_device *device, uint16_t property, uint64_t code);

extern bool ptp_olympus_initialise(indigo_device *device);
extern bool ptp_olympus_handle_event(indigo_device *device, ptp_event_code code, uint32_t *params);
extern bool ptp_olympus_fix_property(indigo_device *device, ptp_property *property);
extern bool ptp_olympus_set_property(indigo_device *device, ptp_property *property);
extern bool ptp_olympus_exposure(indigo_device *device);
extern bool ptp_olympus_liveview(indigo_device *device);
extern bool ptp_olympus_focus(indigo_device *device, int steps);
extern bool ptp_olympus_check_dual_compression(indigo_device *device);

#endif /* indigo_ptp_olympus_h */
