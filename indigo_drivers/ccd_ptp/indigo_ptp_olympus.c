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
 \file indigo_ptp_olympus.c
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
#include "indigo_ptp_olympus.h"

#define OLYMPUS_PRIVATE_DATA	((olympus_private_data *)(PRIVATE_DATA->vendor_private_data))
#define OLYMPUS_CAPTURE_PRESS 0x03
#define OLYMPUS_CAPTURE_RELEASE 0x06
#define OLYMPUS_CAMERA_CONTROL_MODE_PC 0x0001

char *ptp_operation_olympus_code_label(uint16_t code) {
	switch (code) {
		case ptp_operation_olympus_Capture: return "ptp_operation_olympus_Capture";
		case ptp_operation_olympus_GetDateTime: return "ptp_operation_olympus_GetDateTime";
		case ptp_operation_olympus_GetLiveViewImage: return "ptp_operation_olympus_GetLiveViewImage";
		case ptp_operation_olympus_GetImage: return "ptp_operation_olympus_GetImage";
		case ptp_operation_olympus_ChangedProperties: return "ptp_operation_olympus_ChangedProperties";
		case ptp_operation_olympus_MFDrive: return "ptp_operation_olympus_MFDrive";
		case ptp_operation_olympus_SetProperties: return "ptp_operation_olympus_SetProperties";
	}
	return ptp_operation_code_label(code);
}

char *ptp_event_olympus_code_label(uint16_t code) {
	switch (code) {
		case ptp_event_olympus_ObjectAddedLegacy: return "ptp_event_olympus_ObjectAddedLegacy";
		case ptp_event_olympus_DevicePropChangedLegacy: return "ptp_event_olympus_DevicePropChangedLegacy";
		case ptp_event_olympus_CreateRecView: return "ptp_event_olympus_CreateRecView";
		case ptp_event_olympus_ObjectAdded: return "ptp_event_olympus_ObjectAdded";
		case ptp_event_olympus_CaptureComplete: return "ptp_event_olympus_CaptureComplete";
		case ptp_event_olympus_DevicePropChanged: return "ptp_event_olympus_DevicePropChanged";
	}
	return ptp_event_code_label(code);
}

char *ptp_property_olympus_code_name(uint16_t code) {
	switch (code) {
		case ptp_property_ExposureTime: return DSLR_SHUTTER_PROPERTY_NAME;
		case ptp_property_FNumber: return DSLR_APERTURE_PROPERTY_NAME;
		case ptp_property_ExposureProgramMode: return DSLR_PROGRAM_PROPERTY_NAME;
		case ptp_property_ExposureIndex: return DSLR_ISO_PROPERTY_NAME;
		case ptp_property_WhiteBalance: return DSLR_WHITE_BALANCE_PROPERTY_NAME;
		case ptp_property_CompressionSetting: return DSLR_COMPRESSION_PROPERTY_NAME;
		case ptp_property_FocusMode: return DSLR_FOCUS_MODE_PROPERTY_NAME;
		case ptp_property_FocusMeteringMode: return DSLR_FOCUS_METERING_PROPERTY_NAME;
		case ptp_property_ExposureMeteringMode: return DSLR_EXPOSURE_METERING_PROPERTY_NAME;
		case ptp_property_ExposureBiasCompensation: return DSLR_EXPOSURE_COMPENSATION_PROPERTY_NAME;
		case ptp_property_BatteryLevel: return DSLR_BATTERY_LEVEL_PROPERTY_NAME;
		case ptp_property_StillCaptureMode: return DSLR_CAPTURE_MODE_PROPERTY_NAME;
		// vendor codes confirmed on a real OM-1: the body exposes NO standard
		// exposure properties, all controls live in the 0xD0xx range
		case ptp_property_olympus_Aperture: return DSLR_APERTURE_PROPERTY_NAME;
		case ptp_property_olympus_FocusMode: return DSLR_FOCUS_MODE_PROPERTY_NAME;
		case ptp_property_olympus_ExposureMeteringMode: return DSLR_EXPOSURE_METERING_PROPERTY_NAME;
		case ptp_property_olympus_ISO: return DSLR_ISO_PROPERTY_NAME;
		case ptp_property_olympus_ISOSensitivity: return DSLR_ISO_PROPERTY_NAME;
		// d00c is a coarse still/movie state register, not a mode selector; the
		// real dial lives in d006 (read-only enum 1/2/3/4/8/11 = M/P/A/S/Movie/B,
		// hardware-verified) but that register only answers in PC control mode
		// and is absent from the boot-mode property list, so it cannot be
		// enumerated the normal way - candidate future improvement; the P/A/S/M
		// subset is covered by the injected ExposureProgramMode below
		case ptp_property_olympus_ExposureProgram: return "ADV_CameraState";
		case ptp_property_olympus_ExposureBias: return DSLR_EXPOSURE_COMPENSATION_PROPERTY_NAME;
		case ptp_property_olympus_DriveMode: return DSLR_CAPTURE_MODE_PROPERTY_NAME;
		case ptp_property_olympus_ImageFormat: return DSLR_COMPRESSION_PROPERTY_NAME;
		case ptp_property_olympus_Shutterspeed: return DSLR_SHUTTER_PROPERTY_NAME;
		case ptp_property_olympus_WhiteBalance: return DSLR_WHITE_BALANCE_PROPERTY_NAME;
		// still unidentified or not user-facing, kept in the advanced group
		case ptp_property_olympus_ExposureCompensation: return "ADV_ExposureCompensationLegacy";
		case ptp_property_olympus_ColorTemperature: return "ADV_ColorTemperature";
		case ptp_property_olympus_FaceDetection: return "ADV_FaceDetection";
		case ptp_property_olympus_AspectRatio: return "ADV_AspectRatio";
		case ptp_property_olympus_AFArea: return "ADV_AFArea";
		case ptp_property_olympus_CameraControlMode: return "ADV_CameraControlMode";
		case ptp_property_olympus_LiveViewModeOM: return "ADV_LiveViewModeOM";
		case ptp_property_olympus_CaptureTarget: return "ADV_CaptureTarget";
	}
	return ptp_property_code_name(code);
}

char *ptp_property_olympus_code_label(uint16_t code) {
	switch (code) {
		case ptp_property_olympus_Aperture: return "Aperture";
		case ptp_property_olympus_FocusMode: return "Focus mode";
		case ptp_property_olympus_ExposureMeteringMode: return "Exposure metering";
		case ptp_property_olympus_ISO: return "ISO";
		case ptp_property_olympus_ISOSensitivity: return "ISO";
		case ptp_property_olympus_ExposureProgram: return "Camera state";
		case ptp_property_olympus_ExposureBias: return "Exposure compensation";
		case ptp_property_olympus_ExposureCompensation: return "Exposure compensation (legacy)";
		case ptp_property_olympus_DriveMode: return "Drive mode";
		case ptp_property_olympus_ImageFormat: return "Image format";
		case ptp_property_olympus_ColorTemperature: return "Color temperature";
		case ptp_property_olympus_FaceDetection: return "Face detection";
		case ptp_property_olympus_AspectRatio: return "Aspect ratio";
		case ptp_property_olympus_Shutterspeed: return "Shutter speed";
		case ptp_property_olympus_WhiteBalance: return "White balance";
		case ptp_property_olympus_AFArea: return "AF area";
		case ptp_property_olympus_CameraControlMode: return "Camera control mode";
		case ptp_property_olympus_LiveViewModeOM: return "Live view mode";
		case ptp_property_olympus_CaptureTarget: return "Capture target";
	}
	return ptp_property_code_label(code);
}

char *ptp_property_olympus_value_code_label(indigo_device *device, uint16_t property, uint64_t code) {
	static char label[PTP_MAX_CHARS];
	switch (property) {
		case ptp_property_ExposureProgramMode:
			// hidden standard property, values confirmed on a real OM-1 (2 = dial P)
			switch (code) {
				case 1: return "M";
				case 2: return "P";
				case 3: return "A";
				case 4: return "S";
			}
			break;
		case ptp_property_olympus_Aperture: {
			// confirmed on OM-1: f-number * 10 (0x0a = f/1.0, 0x28 = f/4.0)
			sprintf(label, "f/%.1f", (int)code / 10.0);
			return label;
		}
		case ptp_property_olympus_Shutterspeed: {
			// confirmed on OM-1 by a live sub-mode sweep: the B dial position
			// reports FC/FD/FA depending on the selected sub-mode; FB is the
			// E-M1-generation Time sentinel (libgphoto2 config.c), kept for
			// older bodies matched by the wildcard table entries
			switch ((uint32_t)code) {
				case 0xFFFFFFFC: return "Bulb";
				case 0xFFFFFFFD: return "Live Time";
				case 0xFFFFFFFB: return "Time";
				case 0xFFFFFFFA: return "Live Comp";
			}
			// confirmed on OM-1: numerator << 16 | denominator, in seconds
			int numerator = (int)(code >> 16);
			int denominator = (int)(code & 0xFFFF);
			if (denominator == 0) {
				break;
			}
			if (numerator == 1) {
				sprintf(label, "1/%d", denominator);
			} else if (numerator % denominator == 0) {
				sprintf(label, "%d\"", numerator / denominator);
			} else {
				sprintf(label, "%g\"", (double)numerator / denominator);
			}
			return label;
		}
		case ptp_property_olympus_ISOSensitivity: {
			if (code == 0xFFFFFFFF) {
				return "Auto";
			}
			sprintf(label, "%d", (int)code);
			return label;
		}
		case ptp_property_olympus_ExposureBias: {
			// confirmed on OM-1: signed EV * 1000 (300 = +0.3 EV, 62536 = -3.0 EV)
			sprintf(label, "%+.1f", (int16_t)code / 1000.0);
			return label;
		}
		case ptp_property_olympus_ExposureProgram:
			// confirmed by a full dial sweep on a real OM-1: this is NOT the P/A/S/M
			// selector (all native stills modes report 0x8802), it encodes the
			// capture family only
			switch (code) {
				case 0x8100: return "Still (electronic shutter)";
				case 0x8801: return "Still (custom mode)";
				case 0x8802: return "Still";
				case 0x8804: return "Movie";
			}
			break;
		case ptp_property_olympus_DriveMode:
			// confirmed on OM-1 by stepping through the drive menu: 0x20 flag =
			// silent (electronic shutter), 0x40 flag = Pro Capture
			switch (code) {
				case 0x01: return "Single";
				case 0x21: return "Silent Single";
				case 0x07: return "Sequential";
				case 0x27: return "Silent Sequential";
				case 0x28: return "SH1";
				case 0x29: return "SH2";
				case 0x43: return "Pro Capture";
				case 0x48: return "Pro Capture SH1";
				case 0x49: return "Pro Capture SH2";
				case 0x04: return "Self-timer 12s";
				case 0x05: return "Self-timer 2s";
				case 0x24: return "Silent Self-timer 2s";
				case 0x06: return "Custom Self-timer";
			}
			break;
		case ptp_property_olympus_ImageFormat:
			switch (code) {
				case 0x020: return "RAW";
				case 0x101: return "JPEG SF";
				case 0x102: return "JPEG F";
				case 0x103: return "JPEG N";
				case 0x104: return "JPEG B";
				case 0x121: return "RAW + JPEG SF";
				case 0x122: return "RAW + JPEG F";
				case 0x123: return "RAW + JPEG N";
				case 0x124: return "RAW + JPEG B";
			}
			break;
		case ptp_property_olympus_FocusMode:
			switch (code) {
				case 1: return "MF";
				case 2: return "S-AF";
				case 0x8002: return "C-AF";
				case 0x8004: return "S-AF + MF";
				case 0x8007: return "Starry Sky AF";
			}
			break;
		case ptp_property_olympus_ExposureMeteringMode:
			switch (code) {
				case 2: return "Center weighted";
				case 4: return "Spot";
				case 0x8001: return "ESP";
				case 0x8011: return "Spot highlight";
				case 0x8012: return "Spot shadow";
			}
			break;
		case ptp_property_olympus_WhiteBalance:
			switch (code) {
				case 1: return "Auto";
				case 2: return "Sunny";
				case 3: return "Shade";
				case 4: return "Cloudy";
				case 5: return "Incandescent";
				case 6: return "Fluorescent";
				case 7: return "Underwater";
				case 8: return "Flash";
				case 9: return "Custom 1";
				case 10: return "Custom 2";
				case 11: return "Custom 3";
				case 12: return "Custom 4";
				case 13: return "Color temperature";
			}
			break;
		case ptp_property_olympus_CaptureTarget:
			switch (code) {
				case 1: return "RAM";
				case 2: return "Card";
				case 3: return "RAM + Card";
			}
			break;
		case ptp_property_olympus_CameraControlMode:
			switch (code) {
				case 1: return "PC control";
				case 2: return "Camera";
			}
			break;
	}
	return ptp_property_value_code_label(device, property, code);
}

bool ptp_olympus_fix_property(indigo_device *device, ptp_property *property) {
	switch (property->code) {
		case ptp_property_olympus_ImageFormat: {
			OLYMPUS_PRIVATE_DATA->is_dual_compression = property->value.sw.value >= 0x121 && property->value.sw.value <= 0x124;
			return true;
		}
		case ptp_property_ExposureProgramMode: {
			// the descriptor claims the property is settable but the camera answers
			// DevicePropNotSupported to writes, the physical dial is the only authority
			property->writable = false;
			return true;
		}
	}
	return false;
}

bool ptp_olympus_handle_event(indigo_device *device, ptp_event_code code, uint32_t *params) {
	switch ((int)code) {
		case ptp_event_olympus_ObjectAddedLegacy:
		case ptp_event_olympus_ObjectAdded: {
			// OM-1: param1 is the handle of the newly stored image, the camera sends
			// one event per storage slot the image was saved to - download the first
			// copy only and skip (optionally delete) the second one
			INDIGO_DRIVER_LOG(DRIVER_NAME, "%s: param1 = %08x", ptp_event_olympus_code_label(code), params[0]);
			if (params[0] != 0) {
				void *buffer = NULL;
				if (ptp_transaction_1_0_i(device, ptp_operation_GetObjectInfo, params[0], &buffer, NULL) && buffer) {
					uint32_t size;
					char filename[PTP_MAX_CHARS];
					uint8_t *source = buffer;
					source = ptp_decode_uint32(source + 8, &size);
					ptp_decode_string(source + 40, filename);
					free(buffer);
					if (size == OLYMPUS_PRIVATE_DATA->last_object_size && !strcmp(filename, OLYMPUS_PRIVATE_DATA->last_object_name)) {
						INDIGO_DRIVER_LOG(DRIVER_NAME, "duplicate copy of '%s' from second storage slot skipped", filename);
						if (DSLR_DELETE_IMAGE_ON_ITEM->sw.value) {
							ptp_transaction_1_0(device, ptp_operation_DeleteObject, params[0]);
						}
						return true;
					}
					strncpy(OLYMPUS_PRIVATE_DATA->last_object_name, filename, sizeof(OLYMPUS_PRIVATE_DATA->last_object_name));
					OLYMPUS_PRIVATE_DATA->last_object_name[sizeof(OLYMPUS_PRIVATE_DATA->last_object_name) - 1] = '\0';
					OLYMPUS_PRIVATE_DATA->last_object_size = size;
				}
				return ptp_handle_event(device, ptp_event_ObjectAdded, params);
			}
			return true;
		}
		case ptp_event_olympus_CaptureComplete:
			// OM-1: param1 is NOT an object handle, the image arrives via 0xC102,
			// downloading here would fetch (and possibly delete) an unrelated object;
			// note the OM-1 Mark II may differ - libgphoto2 downloads C103's param1
			// as an object handle there (their PR #1123) and that body is also
			// reported to tolerate d052 writes worse than this generation (their
			// issue #1161) - first suspects if a Mark II misbehaves with this driver
			INDIGO_DRIVER_LOG(DRIVER_NAME, "%s: param1 = %08x", ptp_event_olympus_code_label(code), params[0]);
			return true;
		case ptp_event_olympus_CreateRecView:
			return true;
		case ptp_event_olympus_DevicePropChangedLegacy:
		case ptp_event_olympus_DevicePropChanged:
#ifndef USE_ICA_TRANSPORT
			// do NOT refresh per event on the raw transport: a dial change floods
			// C108 events while the body is unresponsive and every triggered
			// GetDevicePropDesc burns a 10s timeout and wedges the pipe; the
			// ChangedProperties checksum poll below refreshes all mapped
			// properties within ~2s and has wedge recovery
			return true;
#else
			// param1 is the changed vendor property code
			return ptp_handle_event(device, ptp_event_DevicePropChanged, params);
#endif
	}
	return ptp_handle_event(device, code, params);
}

#ifndef USE_ICA_TRANSPORT
static bool ptp_olympus_device_reset(indigo_device *device) {
	// PIMA 15740 class-specific Device Reset request, returns the camera's PTP
	// stack to the idle state when a transaction is stuck (the equivalent of
	// libgphoto2's ptp_usb_control_device_reset_request)
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	int rc = libusb_control_transfer(PRIVATE_DATA->handle, LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE, 0x66, 0, PRIVATE_DATA->iface, NULL, 0, PRIVATE_DATA->transaction_timeout);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_control_transfer(DEVICE_RESET) -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
	OLYMPUS_PRIVATE_DATA->last_usb_error = rc < 0 ? rc : 0;
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	return rc >= 0;
}

static bool ptp_olympus_recover(indigo_device *device) {
	// the OM-1 swallows requests fired while it is transitioning (mode switch,
	// dial change) and the abandoned transaction wedges the bulk pipe; recover
	// with a class Device Reset plus a fresh session and poll GetDeviceInfo
	// until the camera answers again
	INDIGO_DRIVER_LOG(DRIVER_NAME, "recovering wedged PTP pipe with device reset");
	for (int attempt = 0; attempt < 3; attempt++) {
		if (!ptp_olympus_device_reset(device) && OLYMPUS_PRIVATE_DATA->last_usb_error == LIBUSB_ERROR_NO_DEVICE) {
			// the camera is physically gone - retrying cannot succeed
			return false;
		}
		indigo_usleep(1000000);
		// flush the interrupt queue before reopening - the camera keeps
		// queueing events while the bulk pipe is dead and holds off bulk
		// responses until the backlog is drained
		for (int i = 0; i < 16; i++) {
			ptp_container event;
			int length = 0;
			memset(&event, 0, sizeof(event));
			if (libusb_bulk_transfer(PRIVATE_DATA->handle, PRIVATE_DATA->ep_int, (unsigned char *)&event, sizeof(event), &length, 100) < 0 || length == 0) {
				break;
			}
			PTP_DUMP_CONTAINER(&event);
		}
		PRIVATE_DATA->transaction_id = 0;
		if (!ptp_transaction_1_1(device, ptp_operation_OpenSession, 1, &PRIVATE_DATA->session_id)) {
			INDIGO_DRIVER_LOG(DRIVER_NAME, "reopen session failed (%04x)", PRIVATE_DATA->last_error);
			continue;
		}
		indigo_usleep(1000000);
		void *buffer = NULL;
		uint32_t size = 0;
		bool responsive = ptp_transaction_0_0_i(device, ptp_operation_GetDeviceInfo, &buffer, &size);
		if (buffer) {
			free(buffer);
		}
		if (responsive) {
			INDIGO_DRIVER_LOG(DRIVER_NAME, "camera responsive again (attempt %d)", attempt + 1);
			return true;
		}
	}
	return false;
}
#endif

static void ptp_olympus_check_event(indigo_device *device) {
	bool transition = false;
#ifdef USE_ICA_TRANSPORT
	ptp_get_event(device);
#else
	// drain the whole event queue every tick - a capture bursts several C108
	// state events ahead of the C102 image notification, and reading one event
	// per second leaves the image stuck in the queue until the exposure wait
	// expires
	for (int i = 0; i < 16; i++) {
		ptp_container event;
		int length = 0;
		memset(&event, 0, sizeof(event));
		int rc = libusb_bulk_transfer(PRIVATE_DATA->handle, PRIVATE_DATA->ep_int, (unsigned char *)&event, sizeof(event), &length, 100);
		if (rc < 0 || length == 0) {
			break;
		}
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer() -> OK, %d", length);
		PTP_DUMP_CONTAINER(&event);
		if (event.code == ptp_event_olympus_DevicePropChanged || event.code == ptp_event_olympus_DevicePropChangedLegacy) {
			// the camera is mid-transition (dial move, mode switch) - property
			// descriptor requests fired now are silently swallowed and wedge
			// the pipe, so postpone the refresh below to the first quiet tick;
			// the stale checksum guarantees it happens
			transition = true;
		}
		ptp_olympus_handle_event(device, event.code, event.payload.params);
	}
#endif
	if (!transition && ptp_operation_supported(device, ptp_operation_olympus_ChangedProperties)) {
		if (--OLYMPUS_PRIVATE_DATA->forced_refresh_countdown <= 0) {
			// some dial moves (still <-> movie after the first round trip) emit
			// no events and leave the ChangedProperties dump byte-identical, so
			// the checksum gate alone would keep the controls stale forever -
			// periodically force a live descriptor refresh to bound that
			OLYMPUS_PRIVATE_DATA->forced_refresh_countdown = 10;
			OLYMPUS_PRIVATE_DATA->last_changed_checksum = 0;
		}
		void *buffer = NULL;
		uint32_t size = 0;
		bool ok = ptp_transaction_0_0_i(device, ptp_operation_olympus_ChangedProperties, &buffer, &size);
		if (ok && buffer && size >= sizeof(uint32_t)) {
			uint32_t count = 0;
			uint8_t *source = ptp_decode_uint32(buffer, &count);
			// the payload is a count-prefixed dump of all vendor property descriptors
			// (~11KB on the OM-1), the OM-1 does not send c108 for every camera-side
			// change (e.g. dial moves between P/A/S/M) and value changes may keep the
			// size, so detect changes by checksum and refresh the mapped controls
			uint32_t checksum = 2166136261u;
			for (uint32_t i = 0; i < size; i++) {
				checksum = (checksum ^ ((uint8_t *)buffer)[i]) * 16777619u;
			}
			if (count > 0 && checksum != OLYMPUS_PRIVATE_DATA->last_changed_checksum) {
				OLYMPUS_PRIVATE_DATA->last_changed_checksum = checksum;
				static uint16_t core_properties[] = {
					ptp_property_ExposureProgramMode,
					ptp_property_olympus_Aperture,
					ptp_property_olympus_FocusMode,
					ptp_property_olympus_ExposureMeteringMode,
					ptp_property_olympus_DriveMode,
					ptp_property_olympus_ImageFormat,
					ptp_property_olympus_ExposureBias,
					ptp_property_olympus_Shutterspeed,
					ptp_property_olympus_WhiteBalance,
					ptp_property_olympus_ISOSensitivity,
					0
				};
				for (int i = 0; core_properties[i]; i++) {
					ptp_property *property = ptp_property_supported(device, core_properties[i]);
					if (property) {
						if (ptp_refresh_property(device, property)) {
							ptp_olympus_fix_property(device, property);
							ptp_update_property(device, property);
						} else {
							// the camera went unresponsive mid-refresh (dial change) -
							// stop asking immediately instead of burning a timeout per
							// property, re-refresh after the recovery below
							ok = false;
							OLYMPUS_PRIVATE_DATA->last_changed_checksum = 0;
							break;
						}
					}
				}
				if (ok && size == sizeof(uint32_t) + count * sizeof(uint16_t) && count <= PTP_MAX_ELEMENTS) {
					for (uint32_t i = 0; i < count; i++) {
						uint16_t property_code = 0;
						source = ptp_decode_uint16(source, &property_code);
						ptp_property *property = ptp_property_supported(device, property_code);
						if (property) {
							if (ptp_refresh_property(device, property)) {
								ptp_update_property(device, property);
							} else {
								ok = false;
								OLYMPUS_PRIVATE_DATA->last_changed_checksum = 0;
								break;
							}
						}
					}
				}
			}
		}
		if (buffer) {
			free(buffer);
		}
#ifndef USE_ICA_TRANSPORT
		if (!ok && IS_CONNECTED && !PRIVATE_DATA->abort_capture) {
			// a refresh fired into a camera-side transition (dial change) wedges
			// the pipe - recover instead of leaving the connection dead
			ptp_olympus_recover(device);
		}
#endif
	}
	if (IS_CONNECTED) {
		// 2Hz halves the dial-change latency (event tick + quiet-tick refresh)
		// and bounds silently changed modes at 5s via the forced refresh
		indigo_reschedule_timer(device, 0.5, &PRIVATE_DATA->event_checker);
	}
}

bool ptp_olympus_initialise(indigo_device *device) {
	DSLR_MIRROR_LOCKUP_PROPERTY->hidden = true;
	PRIVATE_DATA->vendor_private_data = indigo_safe_malloc(sizeof(olympus_private_data));
#ifndef USE_ICA_TRANSPORT
	// normally preset at attach time; kept here for any attach path that
	// forgets - a request fired into a camera-side transition is silently
	// dropped and waiting the stock 10s per read just delays the recovery
	PRIVATE_DATA->transaction_timeout = OLYMPUS_PTP_TIMEOUT;
#endif
	// mirror the OM Capture / libgphoto2 camera_init preamble: the OM-1 acts on the
	// CameraControlMode write but never sends its response container unless
	// GetDeviceInfo and a storage/object enumeration happen first; on macOS the ICA
	// stack performs that negotiation before the driver runs, on raw libusb the
	// driver has to do it itself or the transaction stream wedges
	void *buffer = NULL;
	uint32_t size = 0;
	bool pc_mode_active = false;
	if (ptp_transaction_0_0_i(device, ptp_operation_GetDeviceInfo, &buffer, &size)) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "pre-init GetDeviceInfo: %d bytes", size);
	}
#ifndef USE_ICA_TRANSPORT
	else if (!ptp_olympus_recover(device)) {
		// a session left over from a dead server run can make the camera ignore
		// requests; the device reset in the recovery closes it
		return false;
	}
#endif
	if (buffer) {
		free(buffer);
		buffer = NULL;
	}
#ifndef USE_ICA_TRANSPORT
	// a previous server run also leaves the camera in PC control mode - d052
	// reverts to 2 only on physical unplug - and there the filesystem preamble
	// is pointless and the mode switch is already done; reading d052 at the
	// entry of a leftover session has wedged the pipe (1015 answers normally
	// in an established mode-1 session, the wedge is entry-state specific), so
	// detect the mode by the ChangedProperties dump size instead, ~450 bytes
	// in boot mode 2 vs ~10KB in mode 1
	if (ptp_transaction_0_0_i(device, ptp_operation_olympus_ChangedProperties, &buffer, &size)) {
		pc_mode_active = size > 4096;
	}
	if (buffer) {
		free(buffer);
		buffer = NULL;
	}
	if (pc_mode_active) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "PC control mode already active, taking the reconnect fast path");
	}
#endif
	// the OM Capture application reads the storage ids, each storage info and the
	// root folder objects before switching to PC control mode - without that
	// "filesystem initialisation" the OM-1 drops the response container of the
	// mode switch (libgphoto2 camera_init notes the same requirement)
	if (!pc_mode_active) {
		uint32_t storage_ids[8];
		uint32_t storage_count = 0;
		if (ptp_transaction_0_0_i(device, ptp_operation_GetStorageIDs, &buffer, &size)) {
			uint32_t count = 0;
			if (buffer && size >= sizeof(uint32_t)) {
				uint8_t *source = ptp_decode_uint32(buffer, &count);
				for (uint32_t i = 0; i < count && storage_count < 8 && sizeof(uint32_t) * (i + 2) <= size; i++) {
					source = ptp_decode_uint32(source, storage_ids + storage_count);
					storage_count++;
				}
			}
			INDIGO_DRIVER_LOG(DRIVER_NAME, "ptp_operation_GetStorageIDs: %d storage(s)", count);
		}
		if (buffer) {
			free(buffer);
			buffer = NULL;
		}
		for (uint32_t i = 0; i < storage_count; i++) {
			if (ptp_transaction_1_0_i(device, ptp_operation_GetStorageInfo, storage_ids[i], &buffer, &size)) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "pre-init GetStorageInfo(%08x): %d bytes", storage_ids[i], size);
			}
			if (buffer) {
				free(buffer);
				buffer = NULL;
			}
			if (ptp_transaction_3_0_i(device, ptp_operation_GetObjectHandles, storage_ids[i], 0, 0xFFFFFFFF, &buffer, &size)) {
				uint32_t count = 0;
				uint8_t *source = NULL;
				if (buffer && size >= sizeof(uint32_t)) {
					source = ptp_decode_uint32(buffer, &count);
				}
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "pre-init GetObjectHandles(%08x): %d object(s)", storage_ids[i], count);
				if (count > 16) {
					count = 16;
				}
				for (uint32_t j = 0; j < count && sizeof(uint32_t) * (j + 2) <= size; j++) {
					uint32_t handle = 0;
					source = ptp_decode_uint32(source, &handle);
					void *info = NULL;
					if (ptp_transaction_1_0_i(device, ptp_operation_GetObjectInfo, handle, &info, NULL)) {
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "pre-init GetObjectInfo(%08x)", handle);
					}
					if (info) {
						free(info);
					}
				}
			}
			if (buffer) {
				free(buffer);
				buffer = NULL;
			}
		}
		// read the current control mode before writing it (libgphoto2 ptp_olympus_init_pc_mode)
		if (ptp_transaction_1_0_i(device, ptp_operation_GetDevicePropValue, ptp_property_olympus_CameraControlMode, &buffer, &size)) {
			uint16_t mode = 0;
			if (buffer && size >= sizeof(uint16_t)) {
				ptp_decode_uint16(buffer, &mode);
			}
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "CameraControlMode was %04x", mode);
		}
		if (buffer) {
			free(buffer);
			buffer = NULL;
		}
	}
	// the mode switch itself happens AFTER ptp_initialise below: remote capture
	// (9481) is accepted but ignored in the boot mode 2, so PC control mode is
	// required to fire the shutter, but the property enumeration is known-good in
	// mode 2 on the raw transport, so enumerate first and switch last
	if (!ptp_initialise(device)) {
		return false;
	}
	// the OM-1 keeps the real P/A/S/M exposure mode in the standard but
	// unadvertised ExposureProgramMode property, inject it Fuji-style (only
	// when it is genuinely unadvertised - a wildcard-matched body that lists
	// it would otherwise get a duplicate DSLR_PROGRAM)
	if (!ptp_property_supported(device, ptp_property_ExposureProgramMode) && ptp_transaction_1_0_i(device, ptp_operation_GetDevicePropDesc, ptp_property_ExposureProgramMode, &buffer, &size)) {
		int last = 0;
		for (last = 0; PRIVATE_DATA->info_properties_supported[last]; last++) {
		}
		PRIVATE_DATA->info_properties_supported[last] = ptp_property_ExposureProgramMode;
		ptp_decode_property(buffer, size, device, PRIVATE_DATA->properties + last);
		// the descriptor claims the property is settable but the camera answers
		// DevicePropNotSupported to writes, the physical dial is the only authority
		PRIVATE_DATA->properties[last].writable = false;
	}
	if (buffer) {
		free(buffer);
		buffer = NULL;
	}
	// switch to PC control mode (libgphoto2 ptp_olympus_init_pc_mode) - the
	// shutter only fires remotely in mode 1
	if (!pc_mode_active) {
		uint16_t value = OLYMPUS_CAMERA_CONTROL_MODE_PC;
#ifndef USE_ICA_TRANSPORT
		// a genuine 2->1 write is acted on but its response container is never
		// sent, so waiting the full transaction timeout for it is pure dead
		// time - the C108 confirm arrives within ~200ms
		int saved_timeout = PRIVATE_DATA->transaction_timeout;
		PRIVATE_DATA->transaction_timeout = 1000;
#endif
		bool switched = ptp_transaction_0_1_o(device, ptp_operation_SetDevicePropValue, ptp_property_olympus_CameraControlMode, &value, sizeof(uint16_t));
#ifndef USE_ICA_TRANSPORT
		PRIVATE_DATA->transaction_timeout = saved_timeout;
#endif
		if (switched) {
			INDIGO_DRIVER_LOG(DRIVER_NAME, "CameraControlMode set to PC control");
		} else {
			INDIGO_DRIVER_LOG(DRIVER_NAME, "CameraControlMode set failed (%04x)", PRIVATE_DATA->last_error);
		}
#ifdef USE_ICA_TRANSPORT
		indigo_usleep(100000);
		ptp_get_event(device);
#else
		if (!switched) {
			// a genuine mode change makes the OM-1 drop the response container
			// and confirm via a C108 event ~1.5-3s after the write; recovering
			// before that confirm lands inside the transition window where even
			// the reset/OpenSession get swallowed (costing a full failed
			// attempt), so wait for the confirm - or give up after 3.5s - first
			bool confirmed = false;
			for (int i = 0; i < 35 && !confirmed; i++) {
				ptp_container event;
				int length = 0;
				memset(&event, 0, sizeof(event));
				if (libusb_bulk_transfer(PRIVATE_DATA->handle, PRIVATE_DATA->ep_int, (unsigned char *)&event, sizeof(event), &length, 100) < 0 || length == 0) {
					continue;
				}
				PTP_DUMP_CONTAINER(&event);
				if (event.code == ptp_event_olympus_DevicePropChanged || event.code == ptp_event_olympus_DevicePropChangedLegacy) {
					confirmed = true;
				}
				ptp_olympus_handle_event(device, event.code, event.payload.params);
			}
			// do NOT query GetDevicePropValue here - a 1015 fired into the
			// transition window is swallowed and wedges the pipe (in a settled
			// mode-1 session 1015 answers normally); the C108 wait plus the
			// recovery already confirm the switch
			ptp_olympus_recover(device);
		}
#endif
	}
	indigo_set_timer(device, 0.5, ptp_olympus_check_event, &PRIVATE_DATA->event_checker);
	return true;
}

bool ptp_olympus_exposure(indigo_device *device) {
	// in the B dial position the shutter property reports one of the
	// bulb/live-time/live-comp sentinels (0xFFFFFFFx) instead of a real fraction:
	// hold the shutter with 0x03, time the exposure on the host, release with 0x06
	ptp_property *shutter = ptp_property_supported(device, ptp_property_olympus_Shutterspeed);
	bool is_bulb = shutter && (shutter->value.sw.value & 0xFFFFFF00) == 0xFFFFFF00;
	PRIVATE_DATA->image_added = false;
	bool result = ptp_transaction_1_0(device, ptp_operation_olympus_Capture, OLYMPUS_CAPTURE_PRESS);
	if (result) {
		if (is_bulb) {
			ptp_blob_exposure_timer(device);
		}
		// the release must be sent even after an abort, it ends the exposure
		result = ptp_transaction_1_0(device, ptp_operation_olympus_Capture, OLYMPUS_CAPTURE_RELEASE) && result;
	} else {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "ptp_operation_olympus_Capture failed (%04x)", PRIVATE_DATA->last_error);
	}
	if (result) {
		if (CCD_IMAGE_PROPERTY->state == INDIGO_BUSY_STATE && CCD_PREVIEW_ENABLED_ITEM->sw.value && ptp_olympus_check_dual_compression(device)) {
			CCD_PREVIEW_IMAGE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_PREVIEW_IMAGE_PROPERTY, NULL);
		}
		// the image arrives asynchronously via the event pipe, wait for the exposure
		// plus a 60s margin for processing and download
		int timeout = 600 + 10 * (int)CCD_EXPOSURE_ITEM->number.target;
		for (int i = 0; i < timeout && !PRIVATE_DATA->abort_capture && !PRIVATE_DATA->image_added; i++) {
			indigo_usleep(100000);
		}
		result = PRIVATE_DATA->image_added;
	}
	if (!result || PRIVATE_DATA->abort_capture) {
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
	return result && !PRIVATE_DATA->abort_capture;
}

bool ptp_olympus_liveview(indigo_device *device) {
	void *buffer = NULL;
	uint32_t size = 0;
	int retry_count = 0;
	uint32_t mode = 0x04000300;
	// enable the live view stream (libgphoto2 uses the same LiveViewModeOM value)
	if (!ptp_transaction_0_1_o(device, ptp_operation_SetDevicePropValue, ptp_property_olympus_LiveViewModeOM, &mode, sizeof(uint32_t))) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "failed to enable live view (%04x)", PRIVATE_DATA->last_error);
		return false;
	}
	while (!PRIVATE_DATA->abort_capture && CCD_STREAMING_COUNT_ITEM->number.value != 0) {
		if (ptp_transaction_1_0_i(device, ptp_operation_olympus_GetLiveViewImage, 1, &buffer, &size) && size > 1024) {
			if (CCD_UPLOAD_MODE_LOCAL_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
				CCD_IMAGE_FILE_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
			}
			if (CCD_UPLOAD_MODE_CLIENT_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
				CCD_IMAGE_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
			}
			indigo_process_dslr_image(device, buffer, size, ".jpeg", true);
			if (PRIVATE_DATA->image_buffer) {
				free(PRIVATE_DATA->image_buffer);
			}
			PRIVATE_DATA->image_buffer = buffer;
			buffer = NULL;
			CCD_STREAMING_COUNT_ITEM->number.value--;
			if (CCD_STREAMING_COUNT_ITEM->number.value < 0) {
				CCD_STREAMING_COUNT_ITEM->number.value = -1;
			}
			indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
			retry_count = 0;
		} else {
			// DeviceBusy or an undersized placeholder frame while live view spins up
			if (buffer) {
				free(buffer);
				buffer = NULL;
			}
			if (retry_count++ > 100) {
				INDIGO_DRIVER_LOG(DRIVER_NAME, "live view failed to start (%04x)", PRIVATE_DATA->last_error);
				mode = 0;
				ptp_transaction_0_1_o(device, ptp_operation_SetDevicePropValue, ptp_property_olympus_LiveViewModeOM, &mode, sizeof(uint32_t));
				indigo_finalize_dslr_video_stream(device);
				return false;
			}
		}
		indigo_usleep(50000);
	}
	mode = 0;
	ptp_transaction_0_1_o(device, ptp_operation_SetDevicePropValue, ptp_property_olympus_LiveViewModeOM, &mode, sizeof(uint32_t));
	indigo_finalize_dslr_video_stream(device);
	return !PRIVATE_DATA->abort_capture;
}

bool ptp_olympus_focus(indigo_device *device, int steps) {
	if (steps == 0) {
		return true;
	}
	// MFDrive: param1 = direction (0x01 near, 0x02 far), param2 = step size in
	// lens-specific units (libgphoto2 presets: 0x03 small, 0x0e medium, 0x3c large)
	uint32_t direction = steps > 0 ? 0x02 : 0x01;
	uint32_t step_size = steps > 0 ? steps : -steps;
	return ptp_transaction_2_0(device, ptp_operation_olympus_MFDrive, direction, step_size);
}

bool ptp_olympus_set_property(indigo_device *device, ptp_property *property) {
	if (!ptp_set_property(device, property)) {
		return false;
	}
	// the OM-1 acknowledges PC-initiated changes but reports them via events only
	// much later (with the next camera-side interaction), re-read immediately so
	// clients see the actual camera state
	if (ptp_refresh_property(device, property)) {
		ptp_update_property(device, property);
	}
	return true;
}

bool ptp_olympus_check_dual_compression(indigo_device *device) {
	return OLYMPUS_PRIVATE_DATA->is_dual_compression;
}
