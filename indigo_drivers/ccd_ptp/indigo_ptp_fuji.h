// Copyright (c) 2019-2025 CloudMakers, s. r. o.
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

#ifndef indigo_ptp_fuji_h
#define indigo_ptp_fuji_h

#include <indigo/indigo_driver.h>

/*
typedef enum {
} ptp_operation_fuji_code;

typedef enum {
} ptp_event_fuji__code;
*/

typedef enum {
	ptp_property_fuji_FilmSimulation = 0xd001,
	ptp_property_fuji_DynamicRange = 0xd007,
	ptp_property_fuji_ColorSpace = 0xd00a,
	ptp_property_fuji_CompressionSetting = 0xd018,
	ptp_property_fuji_Zoom = 0xd01b,
	ptp_property_fuji_NoiseReduction = 0xd01c,
	ptp_property_fuji_LockSetting = 0xd136,
	ptp_property_fuji_ControlPriority = 0xd207,
	ptp_property_fuji_AutoFocus = 0xd208,
	ptp_property_fuji_AutoFocusState = 0xd209,
	ptp_property_fuji_CardSave = 0xd20c,
	ptp_property_fuji_GetEvent = 0xd212,
} ptp_property_fuji_code;

typedef struct {
	uint64_t mode;
	uint64_t focus_state;
	uint64_t focus_mode;
	uint64_t shutter_speed;
	bool is_dual_compression;
} fuji_private_data;


extern char *ptp_operation_fuji_code_label(uint16_t code);
extern char *ptp_event_fuji_code_label(uint16_t code);
extern char *ptp_property_fuji_code_name(uint16_t code);
extern char *ptp_property_fuji_code_label(uint16_t code);
extern char *ptp_property_fuji_value_code_label(indigo_device *device, uint16_t property, uint64_t code);

extern bool ptp_fuji_initialise(indigo_device *device);
extern bool ptp_fuji_handle_event(indigo_device *device, ptp_event_code code, uint32_t *params);
extern bool ptp_fuji_fix_property(indigo_device *device, ptp_property *property);
extern bool ptp_fuji_set_property(indigo_device *device, ptp_property *property);
extern bool ptp_fuji_exposure(indigo_device *device);
extern bool ptp_fuji_liveview(indigo_device *device);
extern bool ptp_fuji_af(indigo_device *device);
extern bool ptp_fuji_check_dual_compression(indigo_device *device);

#endif /* indigo_ptp_fuji_h */
