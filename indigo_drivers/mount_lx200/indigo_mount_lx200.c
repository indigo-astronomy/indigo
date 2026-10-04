// Copyright (c) 2016-2026 CloudMakers, s. r. o.
// All rights reserved.

// You may use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

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

// This file generated from indigo_mount_lx200.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

//+ include

#include <ctype.h>
#include <time.h>

#include <indigo/indigo_base64.h>

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_mount_driver.h>
#include <indigo/indigo_align.h>
#include <indigo/indigo_guider_driver.h>
#include <indigo/indigo_focuser_driver.h>
#include <indigo/indigo_aux_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_mount_lx200.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x0300004C
#define DRIVER_NAME          "indigo_mount_lx200"
#define DRIVER_LABEL         "LX200 Mount"
#define MOUNT_DEVICE_NAME    "Mount LX200"
#define GUIDER_DEVICE_NAME   "Mount LX200 (guider)"
#define FOCUSER_DEVICE_NAME  "Mount LX200 (focuser)"
#define AUX_DEVICE_NAME      "Mount LX200 (aux)"
#define PRIVATE_DATA         ((lx200_private_data *)device->private_data)

//+ define

#define NYX_BASE64_THRESHOLD_VERSION "1.32.0"

// How long a Gemini may take to acknowledge a park with the "in progress" status before
// a 0 on :h?# stops meaning "the command has not arrived yet" and starts meaning "the
// park failed". See meade_update_gemini_state().
#define GEMINI_PARK_ACK_TIMEOUT 5.0

#ifndef MAX
#define MAX(a,               b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b)             ((a) < (b) ? (a) : (b))
#endif

// The longest an Onstep focuser move may take before the driver stops waiting for the
// controller to report the focuser standing still again
#define ONSTEP_FOCUS_TIMEOUT 120.0

// The tracking frequency Onstep reports through :GT# for the king rate, and how far a
// reading may be from it and still be that rate. Sidereal is 60.164 Hz on the same
// firmware, so the two are 0.028 Hz apart.
#define ONSTEP_KING_FREQUENCY 60.136
#define ONSTEP_RATE_TOLERANCE 0.01

// Onstep has eight auxiliary device slots (1-indexed) which can have user defined purposes
#define ONSTEP_AUX_DEVICE_COUNT 8
#define AUX_GROUP            "Powerbox"
#define ONSTEP_AUX_HEATER_OUTLET_MAPPING (PRIVATE_DATA->onstep_aux_heater_outlet_slot_mapping)
#define ONSTEP_AUX_POWER_OUTLET_MAPPING (PRIVATE_DATA->onstep_aux_power_outlet_slot_mapping)

#define IS_PARKED            (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PROPERTY->count == 2 && MOUNT_PARK_PARKED_ITEM->sw.value)

#define NYX_TEMPLATE_INDEX   0
#define ZWO_TEMPLATE_INDEX   1

typedef enum {
	ONSTEP_AUX_NONE = 0, 		// Auxiliary slot is disabled
	ONSTEP_AUX_SWITCH = 1,	// Auxiliary slot is a on/off switch -> power outlet in indigo
	ONSTEP_AUX_ANALOG = 2		// Auxiliary slot is an analog / pwm output -> heater outlet in indigo
	//TODO implement Momentary Switch, Dew Heater and Intervalometer
} onstep_aux_device_purpose;

//- define

#pragma mark - Property definitions

#define MOUNT_TYPE_PROPERTY             (PRIVATE_DATA->mount_type_property)
#define MOUNT_TYPE_DETECT_ITEM          (MOUNT_TYPE_PROPERTY->items + 0)
#define MOUNT_TYPE_MEADE_ITEM           (MOUNT_TYPE_PROPERTY->items + 1)
#define MOUNT_TYPE_10MICRONS_ITEM       (MOUNT_TYPE_PROPERTY->items + 2)
#define MOUNT_TYPE_GEMINI_ITEM          (MOUNT_TYPE_PROPERTY->items + 3)
#define MOUNT_TYPE_STARGO_ITEM          (MOUNT_TYPE_PROPERTY->items + 4)
#define MOUNT_TYPE_STARGO2_ITEM         (MOUNT_TYPE_PROPERTY->items + 5)
#define MOUNT_TYPE_AP_ITEM              (MOUNT_TYPE_PROPERTY->items + 6)
#define MOUNT_TYPE_ON_STEP_ITEM         (MOUNT_TYPE_PROPERTY->items + 7)
#define MOUNT_TYPE_AGOTINO_ITEM         (MOUNT_TYPE_PROPERTY->items + 8)
#define MOUNT_TYPE_ZWO_ITEM             (MOUNT_TYPE_PROPERTY->items + 9)
#define MOUNT_TYPE_NYX_ITEM             (MOUNT_TYPE_PROPERTY->items + 10)
#define MOUNT_TYPE_OAT_ITEM             (MOUNT_TYPE_PROPERTY->items + 11)
#define MOUNT_TYPE_TEEN_ASTRO_ITEM      (MOUNT_TYPE_PROPERTY->items + 12)
#define MOUNT_TYPE_ESP32GO_ITEM         (MOUNT_TYPE_PROPERTY->items + 13)
#define MOUNT_TYPE_CLASSIC_ITEM         (MOUNT_TYPE_PROPERTY->items + 14)
#define MOUNT_TYPE_GENERIC_ITEM         (MOUNT_TYPE_PROPERTY->items + 15)

#define MOUNT_TYPE_PROPERTY_NAME        "X_MOUNT_TYPE"
#define MOUNT_TYPE_DETECT_ITEM_NAME     "DETECT"
#define MOUNT_TYPE_MEADE_ITEM_NAME      "MEADE"
#define MOUNT_TYPE_10MICRONS_ITEM_NAME  "10MIC"
#define MOUNT_TYPE_GEMINI_ITEM_NAME     "GEMINI"
#define MOUNT_TYPE_STARGO_ITEM_NAME     "STARGO"
#define MOUNT_TYPE_STARGO2_ITEM_NAME    "STARGO2"
#define MOUNT_TYPE_AP_ITEM_NAME         "AP"
#define MOUNT_TYPE_ON_STEP_ITEM_NAME    "ONSTEP"
#define MOUNT_TYPE_AGOTINO_ITEM_NAME    "AGOTINO"
#define MOUNT_TYPE_ZWO_ITEM_NAME        "ZWO_AM"
#define MOUNT_TYPE_NYX_ITEM_NAME        "NYX"
#define MOUNT_TYPE_OAT_ITEM_NAME        "OAT"
#define MOUNT_TYPE_TEEN_ASTRO_ITEM_NAME "TEEN_ASTRO"
#define MOUNT_TYPE_ESP32GO_ITEM_NAME    "ESP32GO"
#define MOUNT_TYPE_CLASSIC_ITEM_NAME    "CLASSIC"
#define MOUNT_TYPE_GENERIC_ITEM_NAME    "GENERIC"

#define MOUNT_MODE_PROPERTY            (PRIVATE_DATA->alignment_mode_property)
#define EQUATORIAL_ITEM                (MOUNT_MODE_PROPERTY->items + 0)
#define ALTAZ_MODE_ITEM                (MOUNT_MODE_PROPERTY->items + 1)

#define MOUNT_MODE_PROPERTY_NAME       "X_MOUNT_MODE"
#define EQUATORIAL_ITEM_NAME           "EQUATORIAL"
#define ALTAZ_MODE_ITEM_NAME           "ALTAZ"

#define GEMINI_STARTUP_PROPERTY               (PRIVATE_DATA->gemini_startup_property)
#define GEMINI_STARTUP_COLD_ITEM              (GEMINI_STARTUP_PROPERTY->items + 0)
#define GEMINI_STARTUP_WARM_ITEM              (GEMINI_STARTUP_PROPERTY->items + 1)
#define GEMINI_STARTUP_WARM_RESTART_ITEM      (GEMINI_STARTUP_PROPERTY->items + 2)

#define GEMINI_STARTUP_PROPERTY_NAME          "X_GEMINI_STARTUP"
#define GEMINI_STARTUP_COLD_ITEM_NAME         "COLD"
#define GEMINI_STARTUP_WARM_ITEM_NAME         "WARM"
#define GEMINI_STARTUP_WARM_RESTART_ITEM_NAME "WARM_RESTART"

#define GEMINI_PARK_POSITION_PROPERTY          (PRIVATE_DATA->gemini_park_position_property)
#define GEMINI_PARK_POSITION_STARTUP_ITEM      (GEMINI_PARK_POSITION_PROPERTY->items + 0)
#define GEMINI_PARK_POSITION_HOME_ITEM         (GEMINI_PARK_POSITION_PROPERTY->items + 1)
#define GEMINI_PARK_POSITION_ZENITH_ITEM       (GEMINI_PARK_POSITION_PROPERTY->items + 2)

#define GEMINI_PARK_POSITION_PROPERTY_NAME     "X_GEMINI_PARK_POSITION"
#define GEMINI_PARK_POSITION_STARTUP_ITEM_NAME "STARTUP"
#define GEMINI_PARK_POSITION_HOME_ITEM_NAME    "HOME"
#define GEMINI_PARK_POSITION_ZENITH_ITEM_NAME  "ZENITH"

#define AP_SYNC_MODE_PROPERTY          (PRIVATE_DATA->ap_sync_mode_property)
#define AP_SYNC_MODE_RCAL_ITEM         (AP_SYNC_MODE_PROPERTY->items + 0)
#define AP_SYNC_MODE_SYNC_ITEM         (AP_SYNC_MODE_PROPERTY->items + 1)

#define AP_SYNC_MODE_PROPERTY_NAME     "X_AP_SYNC_MODE"
#define AP_SYNC_MODE_RCAL_ITEM_NAME    "RCAL"
#define AP_SYNC_MODE_SYNC_ITEM_NAME    "SYNC"

#define AP_PARK_POSITION_PROPERTY          (PRIVATE_DATA->ap_park_position_property)
#define AP_PARK_POSITION_CURRENT_ITEM      (AP_PARK_POSITION_PROPERTY->items + 0)
#define AP_PARK_POSITION_PARK1_ITEM        (AP_PARK_POSITION_PROPERTY->items + 1)
#define AP_PARK_POSITION_PARK2_ITEM        (AP_PARK_POSITION_PROPERTY->items + 2)
#define AP_PARK_POSITION_PARK3_ITEM        (AP_PARK_POSITION_PROPERTY->items + 3)
#define AP_PARK_POSITION_PARK4_ITEM        (AP_PARK_POSITION_PROPERTY->items + 4)
#define AP_PARK_POSITION_PARK5_ITEM        (AP_PARK_POSITION_PROPERTY->items + 5)

#define AP_PARK_POSITION_PROPERTY_NAME     "X_AP_PARK_POSITION"
#define AP_PARK_POSITION_CURRENT_ITEM_NAME "CURRENT"
#define AP_PARK_POSITION_PARK1_ITEM_NAME   "PARK1"
#define AP_PARK_POSITION_PARK2_ITEM_NAME   "PARK2"
#define AP_PARK_POSITION_PARK3_ITEM_NAME   "PARK3"
#define AP_PARK_POSITION_PARK4_ITEM_NAME   "PARK4"
#define AP_PARK_POSITION_PARK5_ITEM_NAME   "PARK5"

#define ZWO_BUZZER_PROPERTY            (PRIVATE_DATA->zwo_buzzer_property)
#define ZWO_BUZZER_OFF_ITEM            (ZWO_BUZZER_PROPERTY->items + 0)
#define ZWO_BUZZER_LOW_ITEM            (ZWO_BUZZER_PROPERTY->items + 1)
#define ZWO_BUZZER_HIGH_ITEM           (ZWO_BUZZER_PROPERTY->items + 2)

#define ZWO_BUZZER_PROPERTY_NAME       "X_ZWO_BUZZER"
#define ZWO_BUZZER_OFF_ITEM_NAME       "OFF"
#define ZWO_BUZZER_LOW_ITEM_NAME       "LOW"
#define ZWO_BUZZER_HIGH_ITEM_NAME      "HIGH"

#define ZWO_MERIDIAN_PROPERTY               (PRIVATE_DATA->zwo_meridian_property)
#define ZWO_MERIDIAN_AUTO_FLIP_ITEM         (ZWO_MERIDIAN_PROPERTY->items + 0)
#define ZWO_MERIDIAN_TRACK_PASSED_ITEM      (ZWO_MERIDIAN_PROPERTY->items + 1)

#define ZWO_MERIDIAN_PROPERTY_NAME          "X_ZWO_MERIDIAN"
#define ZWO_MERIDIAN_AUTO_FLIP_ITEM_NAME    "AUTO_FLIP_AT_LIMIT"
#define ZWO_MERIDIAN_TRACK_PASSED_ITEM_NAME "TRACK_PASSED_MERIDIAN"

#define ZWO_MERIDIAN_LIMIT_PROPERTY      (PRIVATE_DATA->zwo_meridian_limit_property)
#define ZWO_MERIDIAN_LIMIT_ITEM          (ZWO_MERIDIAN_LIMIT_PROPERTY->items + 0)

#define ZWO_MERIDIAN_LIMIT_PROPERTY_NAME "X_ZWO_MERIDIAN_LIMIT"
#define ZWO_MERIDIAN_LIMIT_ITEM_NAME     "LIMIT"

#define ZWO_MAX_SLEW_SPEED_PROPERTY       (PRIVATE_DATA->zwo_max_slew_speed_property)
#define ZWO_MAX_SLEW_SPEED_LOW_ITEM       (ZWO_MAX_SLEW_SPEED_PROPERTY->items + 0)
#define ZWO_MAX_SLEW_SPEED_HIGH_ITEM      (ZWO_MAX_SLEW_SPEED_PROPERTY->items + 1)

#define ZWO_MAX_SLEW_SPEED_PROPERTY_NAME  "X_ZWO_MAX_SLEW_SPEED"
#define ZWO_MAX_SLEW_SPEED_LOW_ITEM_NAME  "LOW"
#define ZWO_MAX_SLEW_SPEED_HIGH_ITEM_NAME "HIGH"

#define NYX_WIFI_AP_PROPERTY           (PRIVATE_DATA->nyx_wifi_ap_property)
#define NYX_WIFI_AP_SSID_ITEM          (NYX_WIFI_AP_PROPERTY->items + 0)
#define NYX_WIFI_AP_PASSWORD_ITEM      (NYX_WIFI_AP_PROPERTY->items + 1)

#define NYX_WIFI_AP_PROPERTY_NAME      "X_NYX_WIFI_AP"
#define NYX_WIFI_AP_SSID_ITEM_NAME     "AP_SSID"
#define NYX_WIFI_AP_PASSWORD_ITEM_NAME "AP_PASSWORD"

#define NYX_WIFI_CL_PROPERTY           (PRIVATE_DATA->nyx_wifi_cl_property)
#define NYX_WIFI_CL_SSID_ITEM          (NYX_WIFI_CL_PROPERTY->items + 0)
#define NYX_WIFI_CL_PASSWORD_ITEM      (NYX_WIFI_CL_PROPERTY->items + 1)

#define NYX_WIFI_CL_PROPERTY_NAME      "X_NYX_WIFI_CL"
#define NYX_WIFI_CL_SSID_ITEM_NAME     "CL_SSID"
#define NYX_WIFI_CL_PASSWORD_ITEM_NAME "CL_PASSWORD"

#define NYX_WIFI_RESET_PROPERTY        (PRIVATE_DATA->nyx_wifi_reset_property)
#define NYX_WIFI_RESET_ITEM            (NYX_WIFI_RESET_PROPERTY->items + 0)

#define NYX_WIFI_RESET_PROPERTY_NAME   "X_NYX_WIFI_RESET"
#define NYX_WIFI_RESET_ITEM_NAME       "RESET"

#define NYX_LEVELER_PROPERTY           (PRIVATE_DATA->nyx_leveler_property)
#define NYX_LEVELER_PITCH_ITEM         (NYX_LEVELER_PROPERTY->items + 0)
#define NYX_LEVELER_ROLL_ITEM          (NYX_LEVELER_PROPERTY->items + 1)
#define NYX_LEVELER_COMPASS_ITEM       (NYX_LEVELER_PROPERTY->items + 2)

#define NYX_LEVELER_PROPERTY_NAME      "X_NYX_LEVELER"
#define NYX_LEVELER_PITCH_ITEM_NAME    "PITCH"
#define NYX_LEVELER_ROLL_ITEM_NAME     "ROLL"
#define NYX_LEVELER_COMPASS_ITEM_NAME  "COMPASS"

#define ONSTEP_PREFERRED_PIER_SIDE_PROPERTY       (PRIVATE_DATA->onstep_preferred_pier_side_property)
#define ONSTEP_PREFERRED_PIER_SIDE_EAST_ITEM      (ONSTEP_PREFERRED_PIER_SIDE_PROPERTY->items + 0)
#define ONSTEP_PREFERRED_PIER_SIDE_WEST_ITEM      (ONSTEP_PREFERRED_PIER_SIDE_PROPERTY->items + 1)
#define ONSTEP_PREFERRED_PIER_SIDE_BEST_ITEM      (ONSTEP_PREFERRED_PIER_SIDE_PROPERTY->items + 2)
#define ONSTEP_PREFERRED_PIER_SIDE_AUTO_ITEM      (ONSTEP_PREFERRED_PIER_SIDE_PROPERTY->items + 3)

#define ONSTEP_PREFERRED_PIER_SIDE_PROPERTY_NAME  "X_ONSTEP_PREFERRED_PIER_SIDE"
#define ONSTEP_PREFERRED_PIER_SIDE_EAST_ITEM_NAME "EAST"
#define ONSTEP_PREFERRED_PIER_SIDE_WEST_ITEM_NAME "WEST"
#define ONSTEP_PREFERRED_PIER_SIDE_BEST_ITEM_NAME "BEST"
#define ONSTEP_PREFERRED_PIER_SIDE_AUTO_ITEM_NAME "AUTO"

#define ONSTEP_AUTO_MERIDIAN_FLIP_PROPERTY           (PRIVATE_DATA->onstep_auto_meridian_flip_property)
#define ONSTEP_AUTO_MERIDIAN_FLIP_ENABLED_ITEM       (ONSTEP_AUTO_MERIDIAN_FLIP_PROPERTY->items + 0)
#define ONSTEP_AUTO_MERIDIAN_FLIP_DISABLED_ITEM      (ONSTEP_AUTO_MERIDIAN_FLIP_PROPERTY->items + 1)

#define ONSTEP_AUTO_MERIDIAN_FLIP_PROPERTY_NAME      "X_ONSTEP_AUTOMATIC_MERIDIAN_FLIP"
#define ONSTEP_AUTO_MERIDIAN_FLIP_ENABLED_ITEM_NAME  "ENABLED"
#define ONSTEP_AUTO_MERIDIAN_FLIP_DISABLED_ITEM_NAME "DISABLED"

#define ONSTEP_MERIDIAN_LIMITS_PROPERTY       (PRIVATE_DATA->onstep_meridian_limits_property)
#define ONSTEP_MERIDIAN_LIMITS_EAST_ITEM      (ONSTEP_MERIDIAN_LIMITS_PROPERTY->items + 0)
#define ONSTEP_MERIDIAN_LIMITS_WEST_ITEM      (ONSTEP_MERIDIAN_LIMITS_PROPERTY->items + 1)

#define ONSTEP_MERIDIAN_LIMITS_PROPERTY_NAME  "X_ONSTEP_MERIDIAN_LIMITS"
#define ONSTEP_MERIDIAN_LIMITS_EAST_ITEM_NAME "EAST"
#define ONSTEP_MERIDIAN_LIMITS_WEST_ITEM_NAME "WEST"

#define ONSTEP_ALTITUDE_LIMITS_PROPERTY           (PRIVATE_DATA->onstep_altitude_limits_property)
#define ONSTEP_ALTITUDE_LIMITS_HORIZON_ITEM       (ONSTEP_ALTITUDE_LIMITS_PROPERTY->items + 0)
#define ONSTEP_ALTITUDE_LIMITS_OVERHEAD_ITEM      (ONSTEP_ALTITUDE_LIMITS_PROPERTY->items + 1)

#define ONSTEP_ALTITUDE_LIMITS_PROPERTY_NAME      "X_ALTITUDE_LIMITS"
#define ONSTEP_ALTITUDE_LIMITS_HORIZON_ITEM_NAME  "HORIZON"
#define ONSTEP_ALTITUDE_LIMITS_OVERHEAD_ITEM_NAME "OVERHEAD"

#define AUX_WEATHER_PROPERTY           (PRIVATE_DATA->weather_property)
#define AUX_WEATHER_TEMPERATURE_ITEM   (AUX_WEATHER_PROPERTY->items + 0)
#define AUX_WEATHER_PRESSURE_ITEM      (AUX_WEATHER_PROPERTY->items + 1)

#define AUX_INFO_PROPERTY              (PRIVATE_DATA->aux_info_property)
#define AUX_INFO_VOLTAGE_ITEM          (AUX_INFO_PROPERTY->items + 0)

#define AUX_HEATER_OUTLET_PROPERTY     (PRIVATE_DATA->heater_outlet_property)
#define AUX_HEATER_OUTLET_1_ITEM       (AUX_HEATER_OUTLET_PROPERTY->items + 0)
#define AUX_HEATER_OUTLET_2_ITEM       (AUX_HEATER_OUTLET_PROPERTY->items + 1)
#define AUX_HEATER_OUTLET_3_ITEM       (AUX_HEATER_OUTLET_PROPERTY->items + 2)
#define AUX_HEATER_OUTLET_4_ITEM       (AUX_HEATER_OUTLET_PROPERTY->items + 3)
#define AUX_HEATER_OUTLET_5_ITEM       (AUX_HEATER_OUTLET_PROPERTY->items + 4)
#define AUX_HEATER_OUTLET_6_ITEM       (AUX_HEATER_OUTLET_PROPERTY->items + 5)
#define AUX_HEATER_OUTLET_7_ITEM       (AUX_HEATER_OUTLET_PROPERTY->items + 6)
#define AUX_HEATER_OUTLET_8_ITEM       (AUX_HEATER_OUTLET_PROPERTY->items + 7)

#define AUX_POWER_OUTLET_PROPERTY      (PRIVATE_DATA->power_outlet_property)
#define AUX_POWER_OUTLET_1_ITEM        (AUX_POWER_OUTLET_PROPERTY->items + 0)
#define AUX_POWER_OUTLET_2_ITEM        (AUX_POWER_OUTLET_PROPERTY->items + 1)
#define AUX_POWER_OUTLET_3_ITEM        (AUX_POWER_OUTLET_PROPERTY->items + 2)
#define AUX_POWER_OUTLET_4_ITEM        (AUX_POWER_OUTLET_PROPERTY->items + 3)
#define AUX_POWER_OUTLET_5_ITEM        (AUX_POWER_OUTLET_PROPERTY->items + 4)
#define AUX_POWER_OUTLET_6_ITEM        (AUX_POWER_OUTLET_PROPERTY->items + 5)
#define AUX_POWER_OUTLET_7_ITEM        (AUX_POWER_OUTLET_PROPERTY->items + 6)
#define AUX_POWER_OUTLET_8_ITEM        (AUX_POWER_OUTLET_PROPERTY->items + 7)

#pragma mark - Private data definition

typedef struct {
	int count;
	indigo_uni_handle *handle;
	indigo_property *mount_type_property;
	indigo_property *alignment_mode_property;
	indigo_property *gemini_startup_property;
	indigo_property *gemini_park_position_property;
	indigo_property *ap_sync_mode_property;
	indigo_property *ap_park_position_property;
	indigo_property *zwo_buzzer_property;
	indigo_property *zwo_meridian_property;
	indigo_property *zwo_meridian_limit_property;
	indigo_property *zwo_max_slew_speed_property;
	indigo_property *nyx_wifi_ap_property;
	indigo_property *nyx_wifi_cl_property;
	indigo_property *nyx_wifi_reset_property;
	indigo_property *nyx_leveler_property;
	indigo_property *onstep_preferred_pier_side_property;
	indigo_property *onstep_auto_meridian_flip_property;
	indigo_property *onstep_meridian_limits_property;
	indigo_property *onstep_altitude_limits_property;
	indigo_property *weather_property;
	indigo_property *aux_info_property;
	indigo_property *heater_outlet_property;
	indigo_property *power_outlet_property;
	//+ data
	char lastMotionNS, lastMotionWE, lastSlewRate, lastTrackRate;
	char classicGuideNS, classicGuideWE;
	bool classicGoto;
	double classicGuideDeadlineNS, classicGuideDeadlineWE;
	indigo_device *classicGuider;
	double lastRA, lastDec;
	bool coordinate_read_failed;
	char product[64];
	bool slewing, tracking, parked, parking, homed, homing;
	// An OpenAstroTracker cannot be asked whether it is parked, so the driver remembers it.
	// See meade_update_oat_state().
	bool oat_parked, oat_park_expected;
	// A Gemini answers :Gv# with ! while an axis is stalled, and :h?# with 0 both for a park
	// it never received and for one that failed. See meade_update_gemini_state().
	bool stalled, gemini_park_expected, gemini_park_failed;
	// Astro-Physics GTO servo controller: generation (2 to 6, from :V#), the :GOS# status, coordinates with
	// tenths/hundredths of a second, King rate, firmware park positions, timed pulses over 999 ms, a firmware
	// park in progress and the last fault reported in :GOS#.
	int ap_controller;
	bool ap_use_gos, ap_high_precision, ap_king, ap_firmware_parks, ap_long_pulses, ap_parking;
	char ap_fault;
	// Meade: the alignment the mount reported at connect (A, P or G), firmware that does not
	// answer :GW#, Autostar firmware without :Mg that is guided with :RG# and :M?#/:Q?#, the
	// Autostar II models that take a guide rate with :Rg, the LXD600 that has neither long
	// format nor park, and a mount that stopped answering after :hP#.
	char meade_alignment;
	bool meade_no_gw, meade_host_timed_guiding, meade_rg_guide_rate, meade_lxd600, meade_park_silent;
	// Gemini: the software level (4, 5, 6 from :GV#), the last :Gv# velocity, and on Level 4,
	// which cuts a :Mg pulse to 255 encoder ticks, the longest pulse that stays below it and
	// the part of each axis' pulse still to be sent.
	int gemini_level;
	// StarGO: when the sidereal time was last given to the controller.
	double stargo_lst_synced;
	// ZWO AM: the firmware version from :GV# as 0xMMmmpp, and the last :GAT# error code.
	int zwo_firmware, zwo_tracking_error;
	char gemini_velocity;
	int gemini_pulse_chunk, gemini_remaining_ns, gemini_remaining_we;
	char gemini_direction_ns, gemini_direction_we;
	double gemini_park_deadline;
	bool goto_issued;
	// The guider device, which shows the guiding speed of a Gemini as GUIDER_RATE.
	indigo_device *guider_device;
	bool park_allowed, unpark_allowed, home_allowed;
	double timeout;
	char response[128];
	bool use_dst_commands;
	long time_difference;
	int utc_offset;
	bool focus_aborted;
	int onstep_aux_power_outlet_slot_mapping[ONSTEP_AUX_DEVICE_COUNT];	// maps power outlet property item index to onstep aux slot
	int onstep_aux_heater_outlet_slot_mapping[ONSTEP_AUX_DEVICE_COUNT];	// maps heater outlet property item index to onstep aux slot
	//- data
} lx200_private_data;

#pragma mark - Low level code

//+ code

// Compare two version strings in the format "major.minor.patch"
// Returns: -1 if version1 < version2, 0 if equal, 1 if version1 > version2

static int compare_versions(const char *version1, const char *version2) {
	if (!version1 || !version2) {
		return 0;
	}
	char *v1_copy = strdup(version1);
	char *v2_copy = strdup(version2);
	if (!v1_copy || !v2_copy) {
		indigo_safe_free(v1_copy);
		indigo_safe_free(v2_copy);
		return 0;
	}
	int v1_major = 0, v1_minor = 0, v1_patch = 0;
	char *token = strtok(v1_copy, ".");
	if (token) {
		v1_major = atoi(token);
	}
	token = strtok(NULL, ".");
	if (token) {
		v1_minor = atoi(token);
	}
	token = strtok(NULL, ".");
	if (token) {
		v1_patch = atoi(token);
	}
	indigo_safe_free(v1_copy);
	token = strtok(v2_copy, ".");
	int v2_major = 0, v2_minor = 0, v2_patch = 0;
	if (token) {
		v2_major = atoi(token);
	}
	token = strtok(NULL, ".");
	if (token) {
		v2_minor = atoi(token);
	}
	token = strtok(NULL, ".");
	if (token) {
		v2_patch = atoi(token);
	}
	indigo_safe_free(v2_copy);
	if (v1_major != v2_major) {
		return (v1_major > v2_major) ? 1 : -1;
	}
	if (v1_minor != v2_minor) {
		return (v1_minor > v2_minor) ? 1 : -1;
	}
	if (v1_patch != v2_patch) {
		return (v1_patch > v2_patch) ? 1 : -1;
	}
	return 0;
}

static char *meade_error_string(indigo_device *device, unsigned int code) {
	if (MOUNT_TYPE_ZWO_ITEM->sw.value) {
		const char *error_string[] = {
			NULL,
			"Parmeters out of range",
			"Format error",
			"Mount not initialized",
			"Mount is Moving",
			"Target is below horizon",
			"Target is below the altitude limit",
			"Time and location is not set",
			"Unknown error"
		};
		if (code > 8) return NULL;
		return (char *)error_string[code];
	} else if (MOUNT_TYPE_ON_STEP_ITEM->sw.value || MOUNT_TYPE_NYX_ITEM->sw.value) {
		const char *error_string[] = {
			NULL,
			"Below the horizon limit",
			"Above overhead limit",
			"Controller in standby",
			"Mount is parked",
			"Slew in progress",
			"Outside limits",
			"Hardware fault",
			"Already in motion",
			"Unspecified error"
		};
		if (code > 9) return NULL;
		return (char *)error_string[code];
	} else if (MOUNT_TYPE_TEEN_ASTRO_ITEM->sw.value) {
		const char *error_string[] = {
			NULL,
			"Below the horizon limit",
			"No object selected",
			"Same side",
			"Mount is parked",
			"Slew in progress",
			"Outside limits",
			"Guide in progress",
			"Above overhead limit",
			"Hardware fault",
			"Unspecified error"
		};
		if (code > 9) return NULL;
		return (char *)error_string[code];
	}
	return NULL;
}

static void str_replace(char *string, char c0, char c1) {
	char *cp = strchr(string, c0);
	if (cp) {
		*cp = c1;
	}
}

static bool meade_validate_handle(indigo_device *device);

static bool meade_no_reply_command(indigo_device *device, char *command, ...) {
	if (!meade_validate_handle(device)) {
		return false;
	}
	long result = indigo_uni_discard(PRIVATE_DATA->handle);
	if (result >= 0) {
		va_list args;
		va_start(args, command);
		result = indigo_uni_vprintf(PRIVATE_DATA->handle, command, args);
		va_end(args);
	}
	if (result >= 0) {
		indigo_usleep(50000);
	}
	return result >= 0;
}

static bool meade_simple_reply_command(indigo_device *device, char *command, ...) {
	if (!meade_validate_handle(device)) {
		return false;
	}
	long result = indigo_uni_discard(PRIVATE_DATA->handle);
	if (result >= 0) {
		va_list args;
		va_start(args, command);
		result = indigo_uni_vprintf(PRIVATE_DATA->handle, command, args);
		va_end(args);
	}
	if (result >= 0) {
		result = indigo_uni_read_section(PRIVATE_DATA->handle, PRIVATE_DATA->response, 1, "", "", INDIGO_DELAY(PRIVATE_DATA->timeout));
		if (!(MOUNT_TYPE_ON_STEP_ITEM->sw.value || MOUNT_TYPE_ZWO_ITEM->sw.value || MOUNT_TYPE_STARGO2_ITEM->sw.value || MOUNT_TYPE_NYX_ITEM->sw.value || MOUNT_TYPE_TEEN_ASTRO_ITEM->sw.value)) {
			// :SCMM/DD/YY# returns two delimiters PRIVATE_DATA->response:
			// "1Updating Planetary Data#                                #"
			// readout progress part
			if (result && !strncmp(command, ":SC", 3) && (MOUNT_TYPE_AP_ITEM->sw.value || *PRIVATE_DATA->response == '1')) {
				char progress[128];
				indigo_uni_read_section(PRIVATE_DATA->handle, progress, sizeof(progress) - 1, "#", "#", INDIGO_DELAY(0.1));
				indigo_uni_read_section(PRIVATE_DATA->handle, progress, sizeof(progress) - 1, "#", "#", INDIGO_DELAY(0.1));
			}
		}
	}
	if (result >= 0) {
		indigo_usleep(50000);
	}
	return result >= 0;
}

static bool meade_command(indigo_device *device, char *command, ...) {
	if (!meade_validate_handle(device)) {
		return false;
	}
	long result = indigo_uni_discard(PRIVATE_DATA->handle);
	if (result >= 0) {
		va_list args;
		va_start(args, command);
		result = indigo_uni_vprintf(PRIVATE_DATA->handle, command, args);
		va_end(args);
	}
	if (result >= 0) {
		result = indigo_uni_read_section2(PRIVATE_DATA->handle, PRIVATE_DATA->response, sizeof(PRIVATE_DATA->response) - 1, "#", "#", INDIGO_DELAY(PRIVATE_DATA->timeout), INDIGO_DELAY(0.1));
	}
	if (result >= 0) {
		indigo_usleep(50000);
	}
	return result >= 0;
}

// Like meade_command(), but the # is counted, so the result tells a bare # (1) from no
// reply at all (0) and an I/O error (-1). The # is not part of the response.
static long meade_counted_command(indigo_device *device, char *command) {
	if (!meade_validate_handle(device)) {
		return -1;
	}
	long result = indigo_uni_discard(PRIVATE_DATA->handle);
	if (result >= 0) {
		result = indigo_uni_printf(PRIVATE_DATA->handle, "%s", command);
	}
	if (result >= 0) {
		result = indigo_uni_read_section2(PRIVATE_DATA->handle, PRIVATE_DATA->response, sizeof(PRIVATE_DATA->response) - 1, "#", "", INDIGO_DELAY(PRIVATE_DATA->timeout), INDIGO_DELAY(0.1));
	}
	if (result > 0 && PRIVATE_DATA->response[result - 1] == '#') {
		PRIVATE_DATA->response[result - 1] = 0;
	}
	if (result >= 0) {
		indigo_usleep(50000);
	}
	return result;
}

static bool gemini_set(indigo_device *device, int command, char *parameter) {
	char buffer[128];
	char *end = buffer + sprintf(buffer, ">%d:%s", command, parameter);
	uint8_t checksum = buffer[0];
	for (size_t i = 1; i < strlen(buffer); i++)
		checksum = checksum ^ buffer[i];
	checksum = checksum % 128 + 64;
	*end++ = checksum;
	*end++ = '#';
	*end++ = 0;
	return meade_no_reply_command(device, "%s", buffer);
}

// A native get <id:<checksum># answers <value><checksum>#, the checksum being the XOR of the
// value characters, modulo 128, plus 64. An undefined id answers a bare #. Gemini Level 5
// command description, Gemini Native Commands.
static bool gemini_get(indigo_device *device, int command, char *value, size_t size) {
	char buffer[32];
	int length = snprintf(buffer, sizeof(buffer), "<%d:", command);
	uint8_t checksum = 0;
	for (int i = 0; i < length; i++) {
		checksum ^= (uint8_t)buffer[i];
	}
	snprintf(buffer + length, sizeof(buffer) - length, "%c#", checksum % 128 + 64);
	if (!meade_command(device, "%s", buffer)) {
		return false;
	}
	size_t reply_length = strlen(PRIVATE_DATA->response);
	if (reply_length < 2 || reply_length > size) {
		return false;
	}
	checksum = 0;
	for (size_t i = 0; i < reply_length - 1; i++) {
		checksum ^= (uint8_t)PRIVATE_DATA->response[i];
	}
	if ((uint8_t)PRIVATE_DATA->response[reply_length - 1] != checksum % 128 + 64) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Gemini native %d reply %s has a wrong checksum", command, PRIVATE_DATA->response);
		return false;
	}
	memcpy(value, PRIVATE_DATA->response, reply_length - 1);
	value[reply_length - 1] = 0;
	return true;
}

static void keep_alive_callback(indigo_device *device) {
	if (!IS_CONNECTED) { // Ping mount if master device (mount) is not connected
		meade_command(device, ":GR#");
		indigo_execute_handler_in(device, 5, keep_alive_callback);
	}
}

// A Gemini that was just switched on answers no :GR# until its startup is completed. The ACK
// byte answers B while the startup message is shown, b while it waits for the startup mode,
// S during a cold start and G or A once it is ready; the mode is chosen with bC#, bW# or bR#.
// Answers true when the controller was starting up and is ready now. Gemini Level 5 command
// description, 0x06.
static bool gemini_startup(indigo_device *device) {
	bool starting = false;
	double deadline = indigo_monotonic_time() + 120;
	while (indigo_monotonic_time() < deadline) {
		if (!meade_simple_reply_command(device, "\006")) {
			return false;
		}
		switch (*PRIVATE_DATA->response) {
			case 'B':
			case 'S':
				starting = true;
				break;
			case 'b':
				starting = true;
				INDIGO_DRIVER_LOG(DRIVER_NAME, "Gemini waits for the startup mode, selecting %s", GEMINI_STARTUP_WARM_ITEM->sw.value ? "warm start" : GEMINI_STARTUP_WARM_RESTART_ITEM->sw.value ? "warm restart" : "cold start");
				if (!meade_no_reply_command(device, GEMINI_STARTUP_WARM_ITEM->sw.value ? "bW#" : GEMINI_STARTUP_WARM_RESTART_ITEM->sw.value ? "bR#" : "bC#")) {
					return false;
				}
				break;
			case 'G':
			case 'A':
				return starting;
			default:
				return false;
		}
		indigo_usleep(500000);
	}
	return false;
}

static bool lx200_open(indigo_device *device) {
	char *name = DEVICE_PORT_ITEM->text.value;
	if (!indigo_uni_is_url(name, "lx200")) {
		if (device->matched_pattern_index == NYX_TEMPLATE_INDEX) {
			indigo_set_switch(MOUNT_TYPE_PROPERTY, MOUNT_TYPE_NYX_ITEM, true);
		} else if (device->matched_pattern_index == ZWO_TEMPLATE_INDEX) {
			indigo_set_switch(MOUNT_TYPE_PROPERTY, MOUNT_TYPE_ZWO_ITEM, true);
		}
		if (MOUNT_TYPE_NYX_ITEM->sw.value) {
			indigo_set_text_item_value(DEVICE_BAUDRATE_ITEM, "115200-8N1");
		} else if (MOUNT_TYPE_OAT_ITEM->sw.value) {
			indigo_set_text_item_value(DEVICE_BAUDRATE_ITEM, "19200-8N1");
		}
		PRIVATE_DATA->timeout = 1;
		for (int i = 0; i < 3; i++) {
			PRIVATE_DATA->handle = indigo_uni_open_serial_with_config(name, indigo_get_text_item_value(DEVICE_BAUDRATE_ITEM), INDIGO_LOG_DEBUG);
			if (PRIVATE_DATA->handle != NULL) {
				bool answered = (meade_command(device, ":GR#") && strlen(PRIVATE_DATA->response) >= 6) || (meade_command(device, ":GR#") && strlen(PRIVATE_DATA->response) >= 6);
				if (!answered && (MOUNT_TYPE_DETECT_ITEM->sw.value || MOUNT_TYPE_GEMINI_ITEM->sw.value) && gemini_startup(device)) {
					answered = meade_command(device, ":GR#") && strlen(PRIVATE_DATA->response) >= 6;
				}
				if (answered) {
					PRIVATE_DATA->timeout = 3;
					break;
				} else {
					indigo_uni_close(&PRIVATE_DATA->handle);
					if (!strcmp(indigo_get_text_item_value(DEVICE_BAUDRATE_ITEM), "9600-8N1")) {
						indigo_set_text_item_value(DEVICE_BAUDRATE_ITEM, "19200-8N1");
					} else if (!strcmp(indigo_get_text_item_value(DEVICE_BAUDRATE_ITEM), "19200-8N1")) {
						indigo_set_text_item_value(DEVICE_BAUDRATE_ITEM, "115200-8N1");
					} else {
						indigo_set_text_item_value(DEVICE_BAUDRATE_ITEM, "9600-8N1");
					}
				}
			}
		}
		indigo_update_property(device, DEVICE_BAUDRATE_PROPERTY, NULL);
	} else {
		if (MOUNT_TYPE_NYX_ITEM->sw.value || MOUNT_TYPE_ON_STEP_ITEM->sw.value) {
			PRIVATE_DATA->handle = indigo_uni_open_url(name, 9999, INDIGO_TCP_HANDLE, INDIGO_LOG_DEBUG);
		} else {
			PRIVATE_DATA->handle = indigo_uni_open_url(name, 4030, INDIGO_TCP_HANDLE, INDIGO_LOG_DEBUG);
		}
	}
	if (PRIVATE_DATA->handle != NULL) {
		if (PRIVATE_DATA->handle->type == INDIGO_TCP_HANDLE) {
			indigo_uni_set_socket_nodelay_option(PRIVATE_DATA->handle);
			indigo_execute_handler(device, keep_alive_callback);
		}
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to %s", name);
		indigo_uni_discard(PRIVATE_DATA->handle);
		PRIVATE_DATA->timeout = 3;
		if (MOUNT_TYPE_CLASSIC_ITEM->sw.value) {
			// A fresh transport cannot inherit ownership from a failed old guide stop.
			if (!meade_no_reply_command(device, ":Q#")) {
				indigo_uni_close(&PRIVATE_DATA->handle);
				return false;
			}
			PRIVATE_DATA->classicGuideNS = PRIVATE_DATA->classicGuideWE = 0;
			PRIVATE_DATA->lastMotionNS = PRIVATE_DATA->lastMotionWE = 0;
			PRIVATE_DATA->classicGoto = false;
		}
		return true;
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", name);
		return false;
	}
}

static void lx200_close(indigo_device *device) {
	if (PRIVATE_DATA->handle != NULL) {
		indigo_uni_close(&PRIVATE_DATA->handle);
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected from %s", DEVICE_PORT_ITEM->text.value);
	}
}

static bool meade_validate_handle(indigo_device *device) {
	if (PRIVATE_DATA->handle == NULL) {
		return false;
	}
	if (!indigo_uni_is_valid(PRIVATE_DATA->handle)) {
		lx200_close(device);
		indigo_execute_handler(device->master_device, indigo_disconnect_slave_devices);
		return false;
	}
	return true;
}

// ---------------------------------------------------------------------  low level mount commands

static bool meade_set_utc(indigo_device *device, time_t secs, int utc_offset) {
	PRIVATE_DATA->time_difference = time(NULL) - secs;
	time_t seconds = secs + utc_offset * 3600;
	struct tm tm;
	indigo_gmtime(&seconds, &tm);
	// A Gemini keeps its real time clock at UTC and refuses a date or a local time it cannot
	// place on a timeline: "The time difference has to be set before setting the calendar
	// date (SC) and local time (SL)". Gemini Level 5 command description, :SG#. Every other
	// profile keeps the order it had.
	bool offset_first = MOUNT_TYPE_GEMINI_ITEM->sw.value;
	if (offset_first && (!meade_simple_reply_command(device, ":SG%+03d#", -utc_offset) || *PRIVATE_DATA->response != '1')) {
		return false;
	}
	if (!meade_simple_reply_command(device, ":SC%02d/%02d/%02d#", tm.tm_mon + 1, tm.tm_mday, tm.tm_year % 100) || *PRIVATE_DATA->response != '1') {
		return false;
	}
	if (PRIVATE_DATA->use_dst_commands) {
		meade_no_reply_command(device, ":SH%d#", indigo_get_dst_state());
	}
	if (!offset_first && (!meade_simple_reply_command(device, ":SG%+03d#", -utc_offset) || *PRIVATE_DATA->response != '1')) {
		return false;
	}
	if (!meade_simple_reply_command(device, ":SL%02d:%02d:%02d#", tm.tm_hour, tm.tm_min, tm.tm_sec) || *PRIVATE_DATA->response != '1') {
		return false;
	}
	return true;
}

static bool meade_get_utc(indigo_device *device, time_t *secs, int *utc_offset) {
	if (MOUNT_TYPE_MEADE_ITEM->sw.value || MOUNT_TYPE_GEMINI_ITEM->sw.value || MOUNT_TYPE_10MICRONS_ITEM->sw.value || MOUNT_TYPE_AP_ITEM->sw.value || MOUNT_TYPE_ZWO_ITEM->sw.value || MOUNT_TYPE_NYX_ITEM->sw.value || MOUNT_TYPE_OAT_ITEM->sw.value || MOUNT_TYPE_ON_STEP_ITEM->sw.value || MOUNT_TYPE_TEEN_ASTRO_ITEM->sw.value || MOUNT_TYPE_GENERIC_ITEM->sw.value || MOUNT_TYPE_CLASSIC_ITEM->sw.value) {
		struct tm tm;
		memset(&tm, 0, sizeof(tm));
		char separator[2];
		if (meade_command(device, ":GC#") && sscanf(PRIVATE_DATA->response, "%d%c%d%c%d", &tm.tm_mon, separator, &tm.tm_mday, separator, &tm.tm_year) == 5) {
			bool time_read = meade_command(device, ":GL#") && sscanf(PRIVATE_DATA->response, "%d%c%d%c%d", &tm.tm_hour, separator, &tm.tm_min, separator, &tm.tm_sec) == 5;
			if (!time_read && MOUNT_TYPE_GEMINI_ITEM->sw.value && strchr(PRIVATE_DATA->response, ':') == NULL) {
				// A Gemini in Double Precision mode, which another client may have left it in
				// with :u#, answers :GL# with decimal hours. Gemini Level 5 command
				// description, :GL#.
				char *end;
				double hours = strtod(PRIVATE_DATA->response, &end);
				if (end != PRIVATE_DATA->response && *end == 0 && hours >= 0 && hours < 24) {
					long seconds = lround(hours * 3600) % 86400;
					tm.tm_hour = (int)(seconds / 3600);
					tm.tm_min = (int)(seconds / 60 % 60);
					tm.tm_sec = (int)(seconds % 60);
					time_read = true;
				}
			}
			if (time_read) {
				tm.tm_year += 100; // TODO: To be fixed in year 2100 :)
				tm.tm_mon -= 1;
				if (meade_command(device, ":GG#")) {
					if (MOUNT_TYPE_AP_ITEM->sw.value && PRIVATE_DATA->response[0] == ':') {
						if (PRIVATE_DATA->response[1] == 'A') {
							switch (PRIVATE_DATA->response[2]) {
								case '1':
									strcpy(PRIVATE_DATA->response, "-05");
									break;
								case '2':
									strcpy(PRIVATE_DATA->response, "-04");
									break;
								case '3':
									strcpy(PRIVATE_DATA->response, "-03");
									break;
								case '4':
									strcpy(PRIVATE_DATA->response, "-02");
									break;
								case '5':
									strcpy(PRIVATE_DATA->response, "-01");
									break;
							}
						} else if (PRIVATE_DATA->response[1] == '@') {
							switch (PRIVATE_DATA->response[2]) {
								case '4':
									strcpy(PRIVATE_DATA->response, "-12");
									break;
								case '5':
									strcpy(PRIVATE_DATA->response, "-11");
									break;
								case '6':
									strcpy(PRIVATE_DATA->response, "-10");
									break;
								case '7':
									strcpy(PRIVATE_DATA->response, "-09");
									break;
								case '8':
									strcpy(PRIVATE_DATA->response, "-08");
									break;
								case '9':
									strcpy(PRIVATE_DATA->response, "-07");
									break;
							}
						} else if (PRIVATE_DATA->response[1] == '0') {
							strcpy(PRIVATE_DATA->response, "-06");
						}
					}
					// A Gemini on Level 5 answers :GG# with the extended {+-}hh:mm:ss as
					// well as with the plain {+-}hh, and a timezone at thirty minutes loses
					// its half hour to a conversion that reads only the hours. The clock is
					// computed from the whole value; MOUNT_UTC_OFFSET keeps the whole hours
					// the driver carries, which is unchanged for every reply that has no
					// minutes in it. Gemini Level 5 command description, :GG#.
					const char *offset_digits = PRIVATE_DATA->response;
					int offset_sign = *offset_digits == '-' ? -1 : 1;
					if (*offset_digits == '+' || *offset_digits == '-') {
						offset_digits++;
					}
					int offset_hours = 0, offset_minutes = 0, offset_seconds = 0;
					sscanf(offset_digits, "%d:%d:%d", &offset_hours, &offset_minutes, &offset_seconds);
					int offset = offset_sign * (offset_hours * 3600 + offset_minutes * 60 + offset_seconds);
					*utc_offset = -(offset / 3600);
					*secs = indigo_timegm(&tm) + offset;
					PRIVATE_DATA->time_difference = time(NULL) - *secs;
					return true;
				}
			}
		}
		// The clock could not be read, which is not the same as a clock that is wrong.
		return false;
	} else {
		*secs = time(NULL);
		PRIVATE_DATA->time_difference = 0;
	}
	return true;
}

// Answers false for a controller that has no site query, so the caller keeps the site it
// already holds instead of replacing it with the zeroes of an answer that never came. A site
// of 0, 0 is a legal position off the coast of Africa, so the driver cannot publish it for a
// mount that was never asked.
static bool meade_get_site(indigo_device *device, double *latitude, double *longitude) {
	if (MOUNT_TYPE_STARGO2_ITEM->sw.value || MOUNT_TYPE_AGOTINO_ITEM->sw.value) {
		return false;
	}
	if (meade_command(device, ":Gt#")) {
		if (MOUNT_TYPE_STARGO_ITEM->sw.value) {
			str_replace(PRIVATE_DATA->response, 't', '*');
		}
		// An ESP32Go marks the degrees with 0xE1 here too. indigo_stod() stops at it and
		// returns the whole degrees, so the arcminutes of the site are silently lost.
		if (MOUNT_TYPE_ESP32GO_ITEM->sw.value) {
			str_replace(PRIVATE_DATA->response, (char)0xE1, '*');
		}
		*latitude = indigo_stod(PRIVATE_DATA->response);
	}
	if (meade_command(device, ":Gg#")) {
		if (MOUNT_TYPE_STARGO_ITEM->sw.value) {
			str_replace(PRIVATE_DATA->response, 'g', '*');
		}
		if (MOUNT_TYPE_ESP32GO_ITEM->sw.value) {
			str_replace(PRIVATE_DATA->response, (char)0xE1, '*');
		}
		if (MOUNT_TYPE_STARGO_ITEM->sw.value) {
			// A StarGO keeps the longitude signed and positive to the east.
			*longitude = fmod(indigo_stod(PRIVATE_DATA->response) + 360, 360);
		} else {
			// LX200 protocol returns negative longitude for the east, INDIGO publishes it east
			// positive in 0 .. 360, where a site on the prime meridian is 0 and never 360.
			*longitude = fmod(360 - fmod(indigo_stod(PRIVATE_DATA->response) + 360, 360), 360);
		}
	}
	return true;
}

static void stargo_sync_lst(indigo_device *device, double longitude);

static bool meade_set_site(indigo_device *device, double latitude, double longitude, double elevation) {
	char sexagesimal[128];
	bool result = true;
	if (MOUNT_TYPE_AGOTINO_ITEM->sw.value) {
		// The aGotino has no site command. The site is still the one the framework computes
		// the local sidereal time and the horizontal coordinates from, so the driver keeps
		// what the client set instead of refusing it.
		return true;
	}
	if (MOUNT_TYPE_STARGO_ITEM->sw.value) {
		meade_simple_reply_command(device, ":St%s#", indigo_dtos_r(latitude, "%+03d*%02d:%02d", sexagesimal, sizeof(sexagesimal)));
		result = true; // ignore result for Avalon StarGO
	} else {
		result = meade_simple_reply_command(device, ":St%s#", indigo_dtos_r(latitude, "%+03d*%02d", sexagesimal, sizeof(sexagesimal))) && *PRIVATE_DATA->response == '1';
	}
	if (MOUNT_TYPE_STARGO_ITEM->sw.value) {
		// A StarGO takes the longitude signed and positive to the east, -180 .. +180, and
		// computes the sidereal time from it.
		double east = fmod(longitude + 360, 360);
		if (east > 180) {
			east -= 360;
		}
		meade_simple_reply_command(device, ":Sg%s#", indigo_dtos_r(east, "%+04d*%02d:%02d", sexagesimal, sizeof(sexagesimal)));
		stargo_sync_lst(device, longitude);
		return true; // the StarGO does not confirm the site
	}
	// LX200 protocol expects negative longitude for the east
	longitude = fmod(360 - fmod(longitude + 360, 360), 360);
	if (MOUNT_TYPE_OAT_ITEM->sw.value) {
		// An OpenAstroTracker answers an unsigned longitude with 0 and keeps the site it
		// had; it accepts the same value written as :SgsDDD*MM#. Firmware v1.13.20.
		result = meade_simple_reply_command(device, ":Sg%s#", indigo_dtos_r(longitude, "%+04d*%02d", sexagesimal, sizeof(sexagesimal))) && *PRIVATE_DATA->response == '1';
	} else {
		result = meade_simple_reply_command(device, ":Sg%s#", indigo_dtos_r(longitude, "%03d*%02d", sexagesimal, sizeof(sexagesimal))) && *PRIVATE_DATA->response == '1';
	}
	if (MOUNT_TYPE_NYX_ITEM->sw.value) {
		result = meade_simple_reply_command(device, ":Sv%.1f#", elevation) && *PRIVATE_DATA->response == '1';
	}
	return result;
}

static bool meade_parse_coordinate(const char *reply, bool right_ascension, double *value) {
	if (strpbrk(reply, "eExX") != NULL) {
		return false;
	}
	const char *cursor = reply;
	if (!right_ascension && (*cursor == '+' || *cursor == '-')) {
		cursor++;
	}
	if (!isdigit((unsigned char)*cursor)) {
		return false;
	}
	char *end;
	long degrees = strtol(cursor, &end, 10);
	if (degrees > (right_ascension ? 23 : 90) || (right_ascension ? *end != ':' : *end != '*' && *end != ':' && (unsigned char)*end != 0xDF)) {
		return false;
	}
	cursor = end + 1;
	if (!isdigit((unsigned char)*cursor)) {
		return false;
	}
	double minutes = strtod(cursor, &end);
	if (!isfinite(minutes) || minutes < 0 || minutes >= 60) {
		return false;
	}
	double seconds = 0;
	if (*end == ':') {
		cursor = end + 1;
		if (!isdigit((unsigned char)*cursor)) {
			return false;
		}
		seconds = strtod(cursor, &end);
		if (!isfinite(seconds) || seconds < 0 || seconds >= 60) {
			return false;
		}
	}
	if (*end != 0 || (!right_ascension && degrees == 90 && (minutes != 0 || seconds != 0))) {
		return false;
	}
	*value = indigo_stod(reply);
	return isfinite(*value);
}

// A Gemini put into Double Precision with :u# answers every coordinate as a signed decimal
// value with six digits after the point and no sexagesimal separator at all. This driver
// never selects that mode, and deliberately does not send a command to leave it: :U# is a
// toggle and :u# changes what every other client on the same mount sees. Another client can
// have selected it, though, and then the sexagesimal parser rejects everything the mount
// says. Gemini Level 5 command description, :u#.
static bool meade_parse_double_precision(const char *reply, double *value) {
	if (strpbrk(reply, ":*eExX") != NULL || strchr(reply, (char)0xDF) != NULL || strchr(reply, '.') == NULL) {
		return false;
	}
	const char *cursor = reply;
	if (*cursor == '+' || *cursor == '-') {
		cursor++;
	}
	if (!isdigit((unsigned char)*cursor)) {
		return false;
	}
	char *end;
	double parsed = strtod(reply, &end);
	if (*end != 0 || !isfinite(parsed)) {
		return false;
	}
	*value = parsed;
	return true;
}

// The sexagesimal parser, with the Gemini decimal format tried first for that profile alone.
static bool meade_parse_reply_coordinate(indigo_device *device, const char *reply, bool right_ascension, double *value) {
	if (MOUNT_TYPE_GEMINI_ITEM->sw.value && meade_parse_double_precision(reply, value)) {
		return true;
	}
	return meade_parse_coordinate(reply, right_ascension, value);
}

static bool meade_get_coordinates(indigo_device *device, double *ra, double *dec) {
	if (MOUNT_TYPE_NYX_ITEM->sw.value) {
		if (meade_command(device, ":GRH#")) {
			if (!meade_parse_reply_coordinate(device, PRIVATE_DATA->response, true, ra)) {
				return false;
			}
			if (meade_command(device, ":GDH#")) {
				if (!meade_parse_reply_coordinate(device, PRIVATE_DATA->response, false, dec)) {
					return false;
				}
				return true;
			}
		}
	} else if (meade_command(device, ":GR#")) {
		if (!meade_parse_reply_coordinate(device, PRIVATE_DATA->response, true, ra)) {
			return false;
		}
		if (strlen(PRIVATE_DATA->response) < 8) {
			// An LXD600 has no long format, :P# would only toggle it on every poll.
			if ((MOUNT_TYPE_MEADE_ITEM->sw.value && !PRIVATE_DATA->meade_lxd600) || MOUNT_TYPE_OAT_ITEM->sw.value) {
				meade_command(device, ":P#");
				meade_command(device, ":GR#");
			} else if (MOUNT_TYPE_10MICRONS_ITEM->sw.value) {
				meade_no_reply_command(device, ":U1#");
				meade_command(device, ":GR#");
			} else if (MOUNT_TYPE_GEMINI_ITEM->sw.value || MOUNT_TYPE_AP_ITEM->sw.value || MOUNT_TYPE_ON_STEP_ITEM->sw.value || MOUNT_TYPE_TEEN_ASTRO_ITEM->sw.value) {
				meade_no_reply_command(device, ":U#");
				meade_command(device, ":GR#");
			}
		}
		if (!meade_parse_reply_coordinate(device, PRIVATE_DATA->response, true, ra)) {
			return false;
		}
		if (meade_command(device, ":GD#")) {
			if (MOUNT_TYPE_AGOTINO_ITEM->sw.value) {
				PRIVATE_DATA->response[3] = '*';
			}
			if (MOUNT_TYPE_OAT_ITEM->sw.value) {
				// An OpenAstroTracker separates the arcminutes from the arcseconds with the
				// arcminute mark rather than a colon: +45*00'00. Without this the reply is
				// rejected and the driver never reads a declination from the mount at all.
				str_replace(PRIVATE_DATA->response, '\'', ':');
			}
			if (MOUNT_TYPE_ESP32GO_ITEM->sw.value) {
				// An ESP32Go marks the degrees with 0xE1, which is neither the * nor the
				// 0xDF the protocol allows, so without this the reply is rejected and the
				// driver never reads a declination from the mount at all. misc.cpp of the
				// firmware prints every angle with sprintf(..., 225, ...).
				str_replace(PRIVATE_DATA->response, (char)0xE1, '*');
			}
			if (!meade_parse_reply_coordinate(device, PRIVATE_DATA->response, false, dec)) {
				return false;
			}
			return true;
		}
	}
	return false;
}

static bool meade_set_tracking(indigo_device *device, bool on);

// Sends the target of a goto or a sync with :Sr# and :Sd#. indigo_dtos_r() carries a field that
// rounds up to 60 into the next one, so a right ascension just below 24 h comes out as 24:00:00,
// which no controller takes; it is 00:00:00.
static bool meade_set_target(indigo_device *device, double ra, double dec) {
	char sexagesimal[128];
	// A GTOCP4 from P01-04 and every GTOCP5/6 take the right ascension to a hundredth and the declination to a
	// tenth of a second.
	bool ap_precision = MOUNT_TYPE_AP_ITEM->sw.value && PRIVATE_DATA->ap_high_precision;
	indigo_dtos_r(ra, ap_precision ? "%02d:%02d:%05.2f" : "%02d:%02d:%02.0f", sexagesimal, sizeof(sexagesimal));
	if (!strncmp(sexagesimal, "24", 2)) {
		sexagesimal[0] = sexagesimal[1] = '0';
	}
	if (!meade_simple_reply_command(device, ":Sr%s#", sexagesimal) || *PRIVATE_DATA->response != '1') {
		return false;
	}
	return meade_simple_reply_command(device, ":Sd%s#", indigo_dtos_r(dec, ap_precision ? "%+03d*%02d:%04.1f" : "%+03d*%02d:%02.0f", sexagesimal, sizeof(sexagesimal))) && *PRIVATE_DATA->response == '1';
}

static bool meade_slew(indigo_device *device, double ra, double dec) {
	if (MOUNT_TYPE_NYX_ITEM->sw.value) {
		if (MOUNT_TRACKING_OFF_ITEM->sw.value) {
			meade_set_tracking(device, true);
		}
	}
	if (!meade_set_target(device, ra, dec)) {
		return false;
	}
	if (!meade_simple_reply_command(device, ":MS#") || *PRIVATE_DATA->response != '0') {
		if (MOUNT_TYPE_ZWO_ITEM->sw.value && *PRIVATE_DATA->response == 'e') {
			int error_code = 0;
			sscanf(PRIVATE_DATA->response, "e%d", &error_code);
			char *message = meade_error_string(device, error_code);
			if (message) {
				indigo_send_message(device, ALERT_PROPERTY, "%s", message);
			}
		}
		// OnStep answers :MS# with the same 0 .. 9 code table as the OnStep derived NYX, so
		// a client is told why the controller refused the slew instead of only that it did.
		if (MOUNT_TYPE_ON_STEP_ITEM->sw.value || MOUNT_TYPE_NYX_ITEM->sw.value || MOUNT_TYPE_TEEN_ASTRO_ITEM->sw.value) {
			int error_code = atoi(PRIVATE_DATA->response);
			char *message = meade_error_string(device, error_code);
			if (message) {
				indigo_send_message(device, ALERT_PROPERTY, "%s", message);
			}
		}
		// A Gemini follows the code 1 to 7 with its reason, for example "6Outside Limits.#"
		// (Gemini Level 5 command description, :MS#), and so do the classic LX200, the
		// Autostar, the 10micron and the Astro-Physics GTO, for example "1Object Below
		// Horizon#", padded with spaces on a 10micron.
		bool reason_follows = MOUNT_TYPE_GEMINI_ITEM->sw.value || MOUNT_TYPE_MEADE_ITEM->sw.value || MOUNT_TYPE_CLASSIC_ITEM->sw.value || MOUNT_TYPE_10MICRONS_ITEM->sw.value || MOUNT_TYPE_AP_ITEM->sw.value;
		if (reason_follows && *PRIVATE_DATA->response >= '1' && *PRIVATE_DATA->response <= '7') {
			char reason[64];
			long length = indigo_uni_read_section2(PRIVATE_DATA->handle, reason, sizeof(reason) - 1, "#", "#", INDIGO_DELAY(0.5), INDIGO_DELAY(0.1));
			size_t size = length > 0 ? strlen(reason) : 0;
			while (size > 0 && reason[size - 1] == ' ') {
				reason[--size] = 0;
			}
			if (size > 0) {
				indigo_send_message(device, ALERT_PROPERTY, "Slew refused: %s", reason);
			}
		}
		return false;
	}
	return true;
}

static bool meade_sync(indigo_device *device, double ra, double dec) {
	if (!meade_set_target(device, ra, dec)) {
		return false;
	}
	// A GTO servo controller recalibrates with :CMR#, which keeps the side of the pier it already knows. :CM#
	// redefines it, and a sync from the wrong side makes the following slews run into the pier.
	if (MOUNT_TYPE_MEADE_ITEM->sw.value) {
		// An older Autostar synchronises, but sends the reply to :CM# only after the next
		// command arrives, so nothing at all comes back in time. The ACK pushes the reply
		// out; any answer means the mount took the sync. A bare # is an empty reply.
		long received = meade_counted_command(device, ":CM#");
		if (received < 0 || (received == 0 && !meade_simple_reply_command(device, "\006"))) {
			return false;
		}
	} else if (!meade_command(device, MOUNT_TYPE_AP_ITEM->sw.value && AP_SYNC_MODE_RCAL_ITEM->sw.value ? ":CMR#" : ":CM#")) {
		return false;
	}
	if (*PRIVATE_DATA->response == 0) {
		return false;
	}
	if (MOUNT_TYPE_GEMINI_ITEM->sw.value && !strncmp(PRIVATE_DATA->response, "No object!", 10)) {
		// A Gemini that has not been aligned, or that has no object selected, refuses the
		// synchronisation with this string and keeps the position it had. A successful one
		// answers with the name of the object instead, so only the content of an ordinary
		// string tells the two apart and an empty-reply check reports the refusal as a
		// sync that happened. Gemini Level 5 command description, Synchronize.
		indigo_send_message(device, ALERT_PROPERTY, "Sync refused, the mount is not aligned or no object is selected");
		return false;
	}
	if (MOUNT_TYPE_ZWO_ITEM->sw.value && *PRIVATE_DATA->response == 'e') {
		int error_code = 0;
		sscanf(PRIVATE_DATA->response, "e%d", &error_code);
		char *message = meade_error_string(device, error_code);
		if (message) {
			indigo_send_message(device, ALERT_PROPERTY, "%s", message);
		}
		return false;
	}
	if (MOUNT_TYPE_NYX_ITEM->sw.value && *PRIVATE_DATA->response == 'E') {
		int error_code = 0;
		sscanf(PRIVATE_DATA->response, "E%d", &error_code);
		char *message = meade_error_string(device, error_code);
		if (message) {
			indigo_send_message(device, ALERT_PROPERTY, "%s", message);
		}
		return false;
	}
	return true;
}

static bool meade_pec(indigo_device *device, bool on) {
	if (MOUNT_TYPE_ON_STEP_ITEM->sw.value) {
		return meade_no_reply_command(device, on ? ":$QZ+#" : ":$QZ-#");
	}
	return false;
}

static void gemini_read_guiding(indigo_device *device);

static bool meade_set_guide_rate(indigo_device *device, int ra, int dec) {
	if (MOUNT_TYPE_STARGO_ITEM->sw.value) {
		if (meade_no_reply_command(device, ":X20%02d#", ra)) {
			return meade_no_reply_command(device, ":X21%02d#", dec);
		}
	} else if (MOUNT_TYPE_ZWO_ITEM->sw.value) {
		// asi mount has one guide rate for ra and dec
		if (ra < 10) {
			ra = 10;
		}
		if (ra > 90) {
			ra = 90;
		}
		double rate = ra / 100.0;
		return (meade_no_reply_command(device, ":Rg%.1lf#", rate));
	} else if (MOUNT_TYPE_GEMINI_ITEM->sw.value) {
		// One guiding speed for both axes, 0.2 to 0.8 times the sidereal rate. Native 150.
		char speed[16];
		snprintf(speed, sizeof(speed), "%.1f", fmin(80, fmax(20, ra)) / 100.0);
		if (!gemini_set(device, 150, speed)) {
			return false;
		}
		// Level 4 pulses are cut to a length that depends on the guiding speed.
		if (PRIVATE_DATA->gemini_level == 4) {
			gemini_read_guiding(device);
		}
		return true;
	} else if (MOUNT_TYPE_MEADE_ITEM->sw.value && PRIVATE_DATA->meade_rg_guide_rate) {
		// An Autostar II takes one guide rate for both axes as :RgSS.S# in arc seconds per
		// second, at most the sidereal rate, and answers nothing.
		return meade_no_reply_command(device, ":Rg%04.1f#", ra * 15.0417 / 100.0);
	}
	return false;
}

static bool meade_get_guide_rate(indigo_device *device, int *ra, int *dec) {
	if (MOUNT_TYPE_ZWO_ITEM->sw.value) {
		bool res = meade_command(device, ":Ggr#");
		if (!res) {
			return false;
		}
		double rate = 0;
		int parsed = sscanf(PRIVATE_DATA->response, "%lf", &rate);
		if (parsed != 1) {
			return false;
		}
		*ra = *dec = (int)(rate * 100);
		return true;
	}
	return false;
}

static bool meade_set_tracking(indigo_device *device, bool on) {
	if (on) { // TBD
		if (MOUNT_TYPE_GEMINI_ITEM->sw.value) {
			return gemini_set(device, 192, "");
		} else if (MOUNT_TYPE_STARGO_ITEM->sw.value) {
			return meade_no_reply_command(device, ":X122#");
		} else if (MOUNT_TYPE_AP_ITEM->sw.value) {
			// The King correction is switched on with :RT8# and off with :RT3#, :RT2# then selects the sidereal rate.
			if (MOUNT_TRACK_RATE_KING_ITEM->sw.value && PRIVATE_DATA->ap_king) {
				return meade_no_reply_command(device, ":RT8#") && meade_no_reply_command(device, ":RT2#");
			} else if (MOUNT_TRACK_RATE_SIDEREAL_ITEM->sw.value) {
				return (!PRIVATE_DATA->ap_king || meade_no_reply_command(device, ":RT3#")) && meade_no_reply_command(device, ":RT2#");
			} else if (MOUNT_TRACK_RATE_SOLAR_ITEM->sw.value) {
				return meade_no_reply_command(device, ":RT1#");
			} else if (MOUNT_TRACK_RATE_LUNAR_ITEM->sw.value) {
				return meade_no_reply_command(device, ":RT0#");
			}
		} else if (MOUNT_TYPE_ZWO_ITEM->sw.value || MOUNT_TYPE_TEEN_ASTRO_ITEM->sw.value) {
			return meade_command(device, ":Te#") && *PRIVATE_DATA->response == '1';
		} else if (MOUNT_TYPE_NYX_ITEM->sw.value || MOUNT_TYPE_ON_STEP_ITEM->sw.value) {
			if (MOUNT_TRACK_RATE_SIDEREAL_ITEM->sw.value) {
				return meade_command(device, ":TQ#:Te#") && *PRIVATE_DATA->response == '1';
			} else if (MOUNT_TRACK_RATE_SOLAR_ITEM->sw.value) {
				return meade_command(device, ":TS#:Te#") && *PRIVATE_DATA->response == '1';
			} else if (MOUNT_TRACK_RATE_LUNAR_ITEM->sw.value) {
				return meade_command(device, ":TL#:Te#") && *PRIVATE_DATA->response == '1';
			} else if (MOUNT_TRACK_RATE_KING_ITEM->sw.value) {
				return meade_command(device, ":TK#:Te#") && *PRIVATE_DATA->response == '1';
			}
		} else if (MOUNT_TYPE_OAT_ITEM->sw.value) {
			return meade_command(device, ":MT1#") && *PRIVATE_DATA->response == '1';
		} else if (MOUNT_TYPE_MEADE_ITEM->sw.value) {
			// :AA# and :AP# set the alignment and start tracking in it, so the alignment the
			// mount reported at connect is the one restored. A mount whose alignment is not
			// known is not switched into one: :AP# would make an alt-az mount polar.
			if (PRIVATE_DATA->meade_alignment == 'A') {
				return meade_no_reply_command(device, ":AA#");
			} else if (PRIVATE_DATA->meade_alignment == 'P' || PRIVATE_DATA->meade_alignment == 'G') {
				return meade_no_reply_command(device, ":AP#");
			}
			return false;
		} else {
			if (meade_command(device, ":GW#") && *PRIVATE_DATA->response == 'A') {
				return meade_no_reply_command(device, ":AA#");
			} else {
				return meade_no_reply_command(device, ":AP#");
			}
		}
	} else {
		if (MOUNT_TYPE_GEMINI_ITEM->sw.value) {
			return gemini_set(device, 191, "");
		} else if (MOUNT_TYPE_STARGO_ITEM->sw.value) {
			return meade_no_reply_command(device, ":X120#");
		} else if (MOUNT_TYPE_AP_ITEM->sw.value) {
			return meade_no_reply_command(device, ":RT9#");
		} else if (MOUNT_TYPE_ON_STEP_ITEM->sw.value || MOUNT_TYPE_ZWO_ITEM->sw.value || MOUNT_TYPE_NYX_ITEM->sw.value || MOUNT_TYPE_TEEN_ASTRO_ITEM->sw.value) {
			return meade_no_reply_command(device, ":Td#");
		} else if (MOUNT_TYPE_OAT_ITEM->sw.value) {
			return meade_command(device, ":MT0#") && *PRIVATE_DATA->response == '1';
		} else {
			return meade_no_reply_command(device, ":AL#");
		}
	}
	return false;
}

static bool meade_set_tracking_rate(indigo_device *device) {
	if (MOUNT_TYPE_CLASSIC_ITEM->sw.value) {
		char rate = MOUNT_TRACK_RATE_SIDEREAL_ITEM->sw.value ? 'q' : MOUNT_TRACK_RATE_SOLAR_ITEM->sw.value ? 's' : 'l';
		if (PRIVATE_DATA->lastTrackRate == rate) {
			return true;
		}
		// Classic manual FREQ table: quartz sidereal, 60.0 Hz solar, 57.9 Hz lunar.
		if (rate == 'q') {
			if (!meade_no_reply_command(device, ":TQ#")) {
				return false;
			}
		} else if (!meade_simple_reply_command(device, ":ST%.1f#", rate == 's' ? 60.0 : 57.9) || *PRIVATE_DATA->response != '1' || !meade_no_reply_command(device, ":TM#")) {
			return false;
		}
		PRIVATE_DATA->lastTrackRate = rate;
		return true;
	}
	if (MOUNT_TYPE_AP_ITEM->sw.value && MOUNT_TRACKING_OFF_ITEM->sw.value) {
		// Every :RTn# of a GTO servo controller but :RT9# starts tracking, so the rate is applied when tracking is
		// switched on.
		PRIVATE_DATA->lastTrackRate = 0;
		return true;
	}
	if (MOUNT_TRACK_RATE_SIDEREAL_ITEM->sw.value && PRIVATE_DATA->lastTrackRate != 'q') {
		PRIVATE_DATA->lastTrackRate = 'q';
		if (MOUNT_TYPE_GEMINI_ITEM->sw.value) {
			return gemini_set(device, 131, "");
		} else if (MOUNT_TYPE_AP_ITEM->sw.value) {
			return (!PRIVATE_DATA->ap_king || meade_no_reply_command(device, ":RT3#")) && meade_no_reply_command(device, ":RT2#");
		} else if (MOUNT_TYPE_OAT_ITEM->sw.value) {
			return meade_no_reply_command(device, ":XSS1.000#");
		} else if (!MOUNT_TYPE_AGOTINO_ITEM->sw.value) {
			return meade_no_reply_command(device, ":TQ#");
		}
	} else if (MOUNT_TRACK_RATE_SOLAR_ITEM->sw.value && PRIVATE_DATA->lastTrackRate != 's') {
		PRIVATE_DATA->lastTrackRate = 's';
		if (MOUNT_TYPE_GEMINI_ITEM->sw.value) {
			return gemini_set(device, 134, "");
		} else if (MOUNT_TYPE_10MICRONS_ITEM->sw.value) {
			return meade_no_reply_command(device, ":TSOLAR#");
		} else if (MOUNT_TYPE_AP_ITEM->sw.value) {
			return meade_no_reply_command(device, ":RT1#");
		} else if (MOUNT_TYPE_OAT_ITEM->sw.value) {
			return meade_no_reply_command(device, ":XSS0.997#");
		} else if (!MOUNT_TYPE_AGOTINO_ITEM->sw.value) {
			return meade_no_reply_command(device, ":TS#");
		}
	} else if (MOUNT_TRACK_RATE_LUNAR_ITEM->sw.value && PRIVATE_DATA->lastTrackRate != 'l') {
		PRIVATE_DATA->lastTrackRate = 'l';
		if (MOUNT_TYPE_GEMINI_ITEM->sw.value) {
			return gemini_set(device, 133, "");
		} else if (MOUNT_TYPE_AP_ITEM->sw.value) {
			return meade_no_reply_command(device, ":RT0#");
		} else if (MOUNT_TYPE_OAT_ITEM->sw.value) {
			return meade_no_reply_command(device, ":XSS0.965#");
		} else if (!MOUNT_TYPE_AGOTINO_ITEM->sw.value) {
			return meade_no_reply_command(device, ":TL#");
		}
	} else if (MOUNT_TRACK_RATE_KING_ITEM->sw.value && PRIVATE_DATA->lastTrackRate != 'k') {
		PRIVATE_DATA->lastTrackRate = 'k';
		if (MOUNT_TYPE_GEMINI_ITEM->sw.value) {
			return gemini_set(device, 132, "");
		} else if (MOUNT_TYPE_NYX_ITEM->sw.value || MOUNT_TYPE_ON_STEP_ITEM->sw.value || MOUNT_TYPE_ESP32GO_ITEM->sw.value) {
			return meade_no_reply_command(device, ":TK#");
		} else if (MOUNT_TYPE_AP_ITEM->sw.value && PRIVATE_DATA->ap_king) {
			return meade_no_reply_command(device, ":RT8#") && meade_no_reply_command(device, ":RT2#");
		}
	}
	return true;
}

static bool meade_get_tracking_rate(indigo_device *device) {
	if (MOUNT_TYPE_CLASSIC_ITEM->sw.value) {
		if (!meade_command(device, ":GT#")) {
			return false;
		}
		char *end;
		double frequency = strtod(PRIVATE_DATA->response, &end);
		if (end == PRIVATE_DATA->response || *end || !isfinite(frequency) || frequency < 56.4 || frequency > 60.2) {
			return false;
		}
		indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, frequency < 59 ? MOUNT_TRACK_RATE_LUNAR_ITEM : frequency <= 60.0 ? MOUNT_TRACK_RATE_SOLAR_ITEM : MOUNT_TRACK_RATE_SIDEREAL_ITEM, true);
		MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_OK_STATE;
		return true;
	}
	if (MOUNT_TYPE_GEMINI_ITEM->sw.value) {
		// Native 130 answers the rate id: 131 sidereal, 132 King, 133 lunar, 134 solar; closed
		// loop (136) and comet (137) are reported as sidereal. The cache follows what the mount
		// reports, so a rate changed on the hand controller is sent again when selected.
		char rate[16];
		PRIVATE_DATA->lastTrackRate = 0;
		if (!gemini_get(device, 130, rate, sizeof(rate))) {
			return false;
		}
		switch (atoi(rate)) {
			case 132:
				indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_KING_ITEM, true);
				PRIVATE_DATA->lastTrackRate = 'k';
				break;
			case 133:
				indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_LUNAR_ITEM, true);
				PRIVATE_DATA->lastTrackRate = 'l';
				break;
			case 134:
				indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SOLAR_ITEM, true);
				PRIVATE_DATA->lastTrackRate = 's';
				break;
			case 131:
				PRIVATE_DATA->lastTrackRate = 'q';
				// fall through
			case 136:
			case 137:
				indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SIDEREAL_ITEM, true);
				break;
			default:
				return false;
		}
		return true;
	}
	// Onstep and the NYX have it in the :GU# response. The NYX answers :GT# with 0 while
	// tracking is disabled, which is not a tracking rate and must not be decoded as one.
	if (MOUNT_TYPE_MEADE_ITEM->sw.value || MOUNT_TYPE_10MICRONS_ITEM->sw.value || MOUNT_TYPE_TEEN_ASTRO_ITEM->sw.value) {
		if (MOUNT_TYPE_MEADE_ITEM->sw.value) {
			// An Autostar answers with one decimal: 60.1 on the sidereal rate, 60.0 on the
			// solar and 57.9 on the lunar one. It has no king rate, and the property shows
			// three items, so 60.1 is sidereal and not the king rate 60.136.
			if (!meade_command(device, ":GT#") || *PRIVATE_DATA->response == 0) {
				return false;
			}
			double rate = atof(PRIVATE_DATA->response);
			indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, rate < 59 ? MOUNT_TRACK_RATE_LUNAR_ITEM : rate < 60.05 ? MOUNT_TRACK_RATE_SOLAR_ITEM : MOUNT_TRACK_RATE_SIDEREAL_ITEM, true);
			return true;
		}
		if (meade_command(device, ":GT#")) {
			double rate = atof(PRIVATE_DATA->response);
			if (rate <= 57.9) {
				indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_LUNAR_ITEM, true);
			} else if (rate <= 60.0) {
				indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SOLAR_ITEM, true);
			} else if (rate <= 60.14) {
				indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_KING_ITEM, true);
			} else {
				indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SIDEREAL_ITEM, true);
			}
			return true;
		}
	} else if (MOUNT_TYPE_ESP32GO_ITEM->sw.value) {
		// The last character of the ESP32Go status word is the tracking rate index its
		// set_track_speed() keeps, 1 to 4. :GT# answers the tracking frequency in hertz,
		// which this firmware leaves at 50.0 whatever rate is selected.
		if (meade_command(device, ":GU#") && strlen(PRIVATE_DATA->response) >= 5) {
			switch (PRIVATE_DATA->response[4]) {
				case '2':
					indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SOLAR_ITEM, true);
					break;
				case '3':
					indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_LUNAR_ITEM, true);
					break;
				case '4':
					indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_KING_ITEM, true);
					break;
				default:
					indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SIDEREAL_ITEM, true);
					break;
			}
			return true;
		}
	} else if (MOUNT_TYPE_ZWO_ITEM->sw.value) {
		if (meade_command(device, ":GT#")) {
			if (strchr(PRIVATE_DATA->response, '0')) {
				indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SIDEREAL_ITEM, true);
			} else if (strchr(PRIVATE_DATA->response, '1')) {
				indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_LUNAR_ITEM, true);
			} else if (strchr(PRIVATE_DATA->response, '2')) {
				indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SOLAR_ITEM, true);
			}
			return true;
		}
	}
	return false;
}

static bool meade_set_slew_rate(indigo_device *device) {
	// A StarGO knows the rates only as :RG#, :RC#, :RM# and :RS# without an argument, which the
	// last branch sends.
	if (MOUNT_TYPE_AP_ITEM->sw.value) {
		// :RS# only sets the GOTO speed of a GTO servo controller, the N-S-E-W rate is the centering rate,
		// :RC1# 64x, :RC2# 600x and :RC3# 1200x.
		if (MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 'g') {
			PRIVATE_DATA->lastSlewRate = 'g';
			return meade_no_reply_command(device, ":RG#");
		} else if (MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 'c') {
			PRIVATE_DATA->lastSlewRate = 'c';
			return meade_no_reply_command(device, ":RC1#");
		} else if (MOUNT_SLEW_RATE_FIND_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 'm') {
			PRIVATE_DATA->lastSlewRate = 'm';
			return meade_no_reply_command(device, ":RC2#");
		} else if (MOUNT_SLEW_RATE_MAX_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 's') {
			PRIVATE_DATA->lastSlewRate = 's';
			return meade_no_reply_command(device, ":RC3#");
		}
	} else if (MOUNT_TYPE_ZWO_ITEM->sw.value || MOUNT_TYPE_NYX_ITEM->sw.value || MOUNT_TYPE_ON_STEP_ITEM->sw.value) {
		if (MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 'g') {
			PRIVATE_DATA->lastSlewRate = 'g';
			return meade_no_reply_command(device, ":R1#");
		} else if (MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 'c') {
			PRIVATE_DATA->lastSlewRate = 'c';
			return meade_no_reply_command(device, ":R4#");
		} else if (MOUNT_SLEW_RATE_FIND_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 'm') {
			PRIVATE_DATA->lastSlewRate = 'm';
			return meade_no_reply_command(device, ":R7#");
		} else if (MOUNT_SLEW_RATE_MAX_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 's') {
			PRIVATE_DATA->lastSlewRate = 's';
			return meade_no_reply_command(device, ":R9#");
		}
	} else {
		if (MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 'g') {
			PRIVATE_DATA->lastSlewRate = 'g';
			return meade_no_reply_command(device, ":RG#");
		} else if (MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 'c') {
			PRIVATE_DATA->lastSlewRate = 'c';
			return meade_no_reply_command(device, ":RC#");
		} else if (MOUNT_SLEW_RATE_FIND_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 'm') {
			PRIVATE_DATA->lastSlewRate = 'm';
			return meade_no_reply_command(device, ":RM#");
		} else if (MOUNT_SLEW_RATE_MAX_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 's') {
			PRIVATE_DATA->lastSlewRate = 's';
			return meade_no_reply_command(device, ":RS#");
		}
	}
	return true;
}

static bool meade_motion_dec(indigo_device *device) {
	bool stopped = true;
	if (PRIVATE_DATA->lastMotionNS == 'n') {
		stopped = meade_no_reply_command(device, ":Qn#");
	} else if (PRIVATE_DATA->lastMotionNS == 's') {
		stopped = meade_no_reply_command(device, ":Qs#");
	}
	if (stopped) {
		if (MOUNT_MOTION_NORTH_ITEM->sw.value) {
			PRIVATE_DATA->lastMotionNS = 'n';
			return meade_no_reply_command(device, ":Mn#");
		} else if (MOUNT_MOTION_SOUTH_ITEM->sw.value) {
			PRIVATE_DATA->lastMotionNS = 's';
			return meade_no_reply_command(device, ":Ms#");
		} else {
			PRIVATE_DATA->lastMotionNS = 0;
		}
	}
	return stopped;
}

static bool meade_motion_ra(indigo_device *device) {
	bool stopped = true;
	if (PRIVATE_DATA->lastMotionWE == 'w') {
		stopped = meade_no_reply_command(device, ":Qw#");
	} else if (PRIVATE_DATA->lastMotionWE == 'e') {
		stopped = meade_no_reply_command(device, ":Qe#");
	}
	if (stopped) {
		if (MOUNT_MOTION_WEST_ITEM->sw.value) {
			PRIVATE_DATA->lastMotionWE = 'w';
			return meade_no_reply_command(device, ":Mw#");
		} else if (MOUNT_MOTION_EAST_ITEM->sw.value) {
			PRIVATE_DATA->lastMotionWE = 'e';
			return meade_no_reply_command(device, ":Me#");
		} else {
			PRIVATE_DATA->lastMotionWE = 0;
		}
	}
	return stopped;
}

static bool meade_park(indigo_device *device) {
	// OnStep and the OnStep derived NYX answer :hP# with 0 or 1 and refuse the park in states
	// the controller cannot leave on its own, for example the standby a :hF# reset puts it in.
	// The reply has to be read, or a refused park is published busy and never completes.
	if (MOUNT_TYPE_ON_STEP_ITEM->sw.value || MOUNT_TYPE_NYX_ITEM->sw.value) {
		return meade_simple_reply_command(device, ":hP#") && *PRIVATE_DATA->response == '1';
	}
	if (MOUNT_TYPE_OAT_ITEM->sw.value) {
		// The status that follows is what confirms the park, see meade_update_oat_state().
		PRIVATE_DATA->oat_park_expected = true;
		return meade_no_reply_command(device, ":hP#");
	}
	if (MOUNT_TYPE_MEADE_ITEM->sw.value || MOUNT_TYPE_TEEN_ASTRO_ITEM->sw.value) {
		return meade_no_reply_command(device, ":hP#");
	}
	if (MOUNT_TYPE_AP_ITEM->sw.value && PRIVATE_DATA->ap_firmware_parks && !AP_PARK_POSITION_CURRENT_ITEM->sw.value) {
		// A controller with firmware park positions slews to Park n itself after $Kn#; the tracking is stopped first.
		int position = 1;
		for (int i = 1; i <= 5; i++) {
			if (AP_PARK_POSITION_PROPERTY->items[i].sw.value) {
				position = i;
			}
		}
		if (!meade_no_reply_command(device, ":Q#") || !meade_simple_reply_command(device, ":RD0#") || !meade_no_reply_command(device, ":RT9#") || !meade_no_reply_command(device, "$K%d#", position)) {
			return false;
		}
		PRIVATE_DATA->ap_parking = true;
		return true;
	}
	if (MOUNT_TYPE_AP_ITEM->sw.value || MOUNT_TYPE_10MICRONS_ITEM->sw.value) {
		return meade_no_reply_command(device, ":KA#");
	}
	if (MOUNT_TYPE_GEMINI_ITEM->sw.value) {
		// :h?# answers 0 both for a park the controller never received and for one that
		// failed, so the driver has to remember that it asked and give the controller a
		// moment to acknowledge before it reads the 0 as a failure. See
		// meade_update_gemini_state(). Gemini Level 5 command description, :h?#.
		// :hC# parks at the startup (counterweight down) position, :hP# at the home position
		// and :hZ# at the zenith, which only Level 5 and later know.
		if (GEMINI_PARK_POSITION_ZENITH_ITEM->sw.value && PRIVATE_DATA->gemini_level > 0 && PRIVATE_DATA->gemini_level < 5) {
			indigo_send_message(device, ALERT_PROPERTY, "Gemini Level %d cannot park at the zenith", PRIVATE_DATA->gemini_level);
			return false;
		}
		PRIVATE_DATA->gemini_park_expected = true;
		PRIVATE_DATA->gemini_park_failed = false;
		PRIVATE_DATA->gemini_park_deadline = indigo_monotonic_time() + GEMINI_PARK_ACK_TIMEOUT;
		return meade_no_reply_command(device, GEMINI_PARK_POSITION_HOME_ITEM->sw.value ? ":hP#" : GEMINI_PARK_POSITION_ZENITH_ITEM->sw.value ? ":hZ#" : ":hC#");
	}
	if (MOUNT_TYPE_STARGO_ITEM->sw.value) {
		return meade_command(device, ":X362#") && strcmp(PRIVATE_DATA->response, "pB") == 0;
	}
	return false;
}

// Puts MOUNT_PARK back on the state the driver knows the mount is in, for every path that
// refuses or fails a request after the framework has already copied the requested value.
static void meade_restore_park_switch(indigo_device *device) {
	if (MOUNT_PARK_PROPERTY->count == 2) {
		indigo_set_switch(MOUNT_PARK_PROPERTY, PRIVATE_DATA->parked ? MOUNT_PARK_PARKED_ITEM : MOUNT_PARK_UNPARKED_ITEM, true);
	} else {
		MOUNT_PARK_PARKED_ITEM->sw.value = false;
	}
}

static bool meade_unpark(indigo_device *device) {
	if (MOUNT_TYPE_OAT_ITEM->sw.value) {
		PRIVATE_DATA->oat_parked = PRIVATE_DATA->oat_park_expected = false;
		return meade_no_reply_command(device, ":hU#");
	}
	if (MOUNT_TYPE_GEMINI_ITEM->sw.value) {
		PRIVATE_DATA->gemini_park_expected = PRIVATE_DATA->gemini_park_failed = false;
		return meade_no_reply_command(device, ":hW#");
	}
	if (MOUNT_TYPE_10MICRONS_ITEM->sw.value || MOUNT_TYPE_AP_ITEM->sw.value) {
		return meade_no_reply_command(device, ":PO#");
	}
	if (MOUNT_TYPE_STARGO_ITEM->sw.value) {
		return meade_command(device, ":X370#") && strcmp(PRIVATE_DATA->response, "p0") == 0;
	}
	// OnStep and the NYX answer :hR# with 0 or 1; TeenAstro has no documented reply.
	if (MOUNT_TYPE_ON_STEP_ITEM->sw.value || MOUNT_TYPE_NYX_ITEM->sw.value) {
		return meade_simple_reply_command(device, ":hR#") && *PRIVATE_DATA->response == '1';
	}
	if (MOUNT_TYPE_TEEN_ASTRO_ITEM->sw.value) {
		return meade_no_reply_command(device, ":hR#");
	}
	return false;
}

static bool meade_park_set(indigo_device *device) {
	if (MOUNT_TYPE_ON_STEP_ITEM->sw.value || MOUNT_TYPE_NYX_ITEM->sw.value || MOUNT_TYPE_TEEN_ASTRO_ITEM->sw.value) {
		return meade_simple_reply_command(device, ":hQ#") && *PRIVATE_DATA->response == '1';
	}
	return false;
}

static bool meade_home(indigo_device *device) {
	if (MOUNT_TYPE_10MICRONS_ITEM->sw.value || MOUNT_TYPE_OAT_ITEM->sw.value) {
		return meade_no_reply_command(device, ":hF#");
	}
	if (MOUNT_TYPE_ON_STEP_ITEM->sw.value || MOUNT_TYPE_ZWO_ITEM->sw.value || MOUNT_TYPE_NYX_ITEM->sw.value || MOUNT_TYPE_TEEN_ASTRO_ITEM->sw.value) {
		return meade_no_reply_command(device, ":hC#");
	}
	if (MOUNT_TYPE_STARGO_ITEM->sw.value) {
		return meade_command(device, ":X361#") && strcmp(PRIVATE_DATA->response, "pA") == 0;
	}
	if (MOUNT_TYPE_ESP32GO_ITEM->sw.value) {
		return meade_no_reply_command(device, ":hP#");
	}
	return false;
}

static bool meade_home_set(indigo_device *device) {
	if (MOUNT_TYPE_ON_STEP_ITEM->sw.value || MOUNT_TYPE_NYX_ITEM->sw.value) {
		return meade_no_reply_command(device, ":hF#");
	}
	if (MOUNT_TYPE_TEEN_ASTRO_ITEM->sw.value) {
		return meade_no_reply_command(device, ":hB#");
	}
	if (MOUNT_TYPE_ESP32GO_ITEM->sw.value) {
		return meade_no_reply_command(device, ":hS#");
	}
	return false;
}

static void guider_guide_dec_finalizer(indigo_device *device);
static void guider_guide_ra_finalizer(indigo_device *device);
static void guider_guide_dec_handler(indigo_device *device);
static void guider_guide_ra_handler(indigo_device *device);

static bool meade_classic_guide_command(indigo_device *device, char command, char direction) {
	// Classic manual guide commands have no reply or specified post-write delay.
	// The ordinary helper's 50 ms pause would lengthen every host-timed pulse.
	return meade_validate_handle(device) && indigo_uni_discard(PRIVATE_DATA->handle) >= 0 && indigo_uni_printf(PRIVATE_DATA->handle, ":%c%c#", command, direction) >= 0;
}

static bool meade_classic_guide_stop(indigo_device *device, char *direction) {
	if (*direction) {
		if (!meade_classic_guide_command(device, 'Q', *direction)) {
			return false;
		}
		*direction = 0;
	}
	return true;
}

// The classic LX200 and Autostar firmware before 31Ee have no :Mg pulse; they are guided by
// :RG# and a slow motion the driver starts and stops itself.
static bool meade_host_timed_guiding(indigo_device *device) {
	return MOUNT_TYPE_CLASSIC_ITEM->sw.value || (MOUNT_TYPE_MEADE_ITEM->sw.value && PRIVATE_DATA->meade_host_timed_guiding);
}

static void meade_classic_cancel_guides(indigo_device *device) {
	if (meade_host_timed_guiding(device) && PRIVATE_DATA->classicGuider) {
		device = PRIVATE_DATA->classicGuider;
		indigo_cancel_pending_handler(device, guider_guide_dec_handler);
		indigo_cancel_pending_handler(device, guider_guide_ra_handler);
		indigo_cancel_pending_handler(device, guider_guide_dec_finalizer);
		indigo_cancel_pending_handler(device, guider_guide_ra_finalizer);
		guider_guide_dec_finalizer(device);
		guider_guide_ra_finalizer(device);
	}
}

static bool meade_classic_guide_start(indigo_device *device, char *active, double *deadline, char direction, int duration) {
	// RG changes the shared manual rate, so manual motion and goto cannot overlap a pulse.
	if (PRIVATE_DATA->lastMotionNS || PRIVATE_DATA->lastMotionWE || PRIVATE_DATA->classicGoto) {
		return false;
	}
	if (!meade_classic_guide_stop(device, active) || !meade_classic_guide_command(device, 'R', 'G')) {
		return false;
	}
	PRIVATE_DATA->lastSlewRate = 'g';
	if (!meade_classic_guide_command(device, 'M', direction)) {
		return false;
	}
	*active = direction;
	*deadline = indigo_monotonic_time() + duration / 1000.0;
	return true;
}

static bool meade_stop(indigo_device *device) {
	return meade_no_reply_command(device, ":Q#");
}

static void gemini_guide_dec_continue(indigo_device *device);
static void gemini_guide_ra_continue(indigo_device *device);

// The next part of a Level 4 pulse that is longer than gemini_pulse_chunk, sent when the
// previous part ends. See gemini_read_guiding().
static void gemini_guide_continue(indigo_device *device, char direction, int *remaining, indigo_timer_callback next) {
	if (*remaining <= 0) {
		return;
	}
	int part = *remaining < PRIVATE_DATA->gemini_pulse_chunk ? *remaining : PRIVATE_DATA->gemini_pulse_chunk;
	*remaining -= part;
	if (!meade_no_reply_command(device, ":Mg%c%04d#", direction, part)) {
		*remaining = 0;
		return;
	}
	if (*remaining > 0) {
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_URGENT, part / 1000.0, next);
	}
}

static void gemini_guide_dec_continue(indigo_device *device) {
	gemini_guide_continue(device, PRIVATE_DATA->gemini_direction_ns, &PRIVATE_DATA->gemini_remaining_ns, gemini_guide_dec_continue);
}

static void gemini_guide_ra_continue(indigo_device *device) {
	gemini_guide_continue(device, PRIVATE_DATA->gemini_direction_we, &PRIVATE_DATA->gemini_remaining_we, gemini_guide_ra_continue);
}

// A Gemini pulse. Level 4 cuts :Mg to 255 motor encoder ticks modulo 256, so a longer pulse
// is sent in parts that stay below that, each one when the previous ends; the pulse as a
// whole still ends at the requested time. A new pulse replaces the rest of a running one.
static bool gemini_guide(indigo_device *device, char direction, int duration, char *active, int *remaining, indigo_timer_callback next) {
	indigo_cancel_pending_handler(device, next);
	*remaining = 0;
	*active = direction;
	if (PRIVATE_DATA->gemini_level == 4 && PRIVATE_DATA->gemini_pulse_chunk > 0 && duration > PRIVATE_DATA->gemini_pulse_chunk) {
		*remaining = duration;
		gemini_guide_continue(device, direction, remaining, next);
		return true;
	}
	return meade_no_reply_command(device, ":Mg%c%04d#", direction, duration);
}

static bool meade_guide_dec(indigo_device *device, int north, int south) {
	if (PRIVATE_DATA->meade_park_silent) {
		return false;
	}
	if (MOUNT_TYPE_GEMINI_ITEM->sw.value) {
		return (north > 0 || south > 0) && gemini_guide(device, north > 0 ? 'n' : 's', north > 0 ? north : south, &PRIVATE_DATA->gemini_direction_ns, &PRIVATE_DATA->gemini_remaining_ns, gemini_guide_dec_continue);
	}
	if (meade_host_timed_guiding(device)) {
		return meade_classic_guide_start(device, &PRIVATE_DATA->classicGuideNS, &PRIVATE_DATA->classicGuideDeadlineNS, north > 0 ? 'n' : 's', north > 0 ? north : south);
	}
	if (MOUNT_TYPE_AP_ITEM->sw.value) {
		// A GTOCP4 or later times pulses up to 99999 ms, earlier boxes take three digits.
		int limit = PRIVATE_DATA->ap_long_pulses ? 99999 : 999;
		if (north > 0) {
			return meade_no_reply_command(device, ":Mn%03d#", north > limit ? limit : north);
		} else if (south > 0) {
			return meade_no_reply_command(device, ":Ms%03d#", south > limit ? limit : south);
		}
	} else {
		if (north > 0) {
			return meade_no_reply_command(device, ":Mgn%04d#", north);
		} else if (south > 0) {
			return meade_no_reply_command(device, ":Mgs%04d#", south);
		}
	}
	return false;
}

static bool meade_guide_ra(indigo_device *device, int west, int east) {
	if (PRIVATE_DATA->meade_park_silent) {
		return false;
	}
	if (MOUNT_TYPE_GEMINI_ITEM->sw.value) {
		return (west > 0 || east > 0) && gemini_guide(device, west > 0 ? 'w' : 'e', west > 0 ? west : east, &PRIVATE_DATA->gemini_direction_we, &PRIVATE_DATA->gemini_remaining_we, gemini_guide_ra_continue);
	}
	if (meade_host_timed_guiding(device)) {
		return meade_classic_guide_start(device, &PRIVATE_DATA->classicGuideWE, &PRIVATE_DATA->classicGuideDeadlineWE, west > 0 ? 'w' : 'e', west > 0 ? west : east);
	}
	if (MOUNT_TYPE_AP_ITEM->sw.value) {
		int limit = PRIVATE_DATA->ap_long_pulses ? 99999 : 999;
		if (west > 0) {
			return meade_no_reply_command(device, ":Mw%03d#", west > limit ? limit : west);
		} else if (east > 0) {
			return meade_no_reply_command(device, ":Me%03d#", east > limit ? limit : east);
		}
	} else {
		if (west > 0) {
			return meade_no_reply_command(device, ":Mgw%04d#", west);
		} else if (east > 0) {
			return meade_no_reply_command(device, ":Mge%04d#", east);
		}
	}
	return false;
}

static bool meade_focus_abort(indigo_device *device) {
	if (MOUNT_TYPE_MEADE_ITEM->sw.value || MOUNT_TYPE_AP_ITEM->sw.value || MOUNT_TYPE_ON_STEP_ITEM->sw.value || MOUNT_TYPE_OAT_ITEM->sw.value) {
		if (meade_no_reply_command(device, ":FQ#")) {
			return true;
		}
	}
	return false;
}

static bool meade_focus_rel(indigo_device *device, bool slow, int steps) {
	if (steps == 0) {
		return true;
	}
	PRIVATE_DATA->focus_aborted = false;
	if (MOUNT_TYPE_MEADE_ITEM->sw.value || MOUNT_TYPE_AP_ITEM->sw.value || MOUNT_TYPE_ON_STEP_ITEM->sw.value || MOUNT_TYPE_OAT_ITEM->sw.value) {
		if (!meade_no_reply_command(device, slow ? ":FS#" : ":FF#"))
			return false;
	}
	if (MOUNT_TYPE_MEADE_ITEM->sw.value || MOUNT_TYPE_AP_ITEM->sw.value || MOUNT_TYPE_OAT_ITEM->sw.value) {
		if (!meade_no_reply_command(device, steps > 0 ? ":F+#" : ":F-#"))
			return false;
		if (steps < 0) {
			steps = - steps;
		}
		for (int i = 0; i < steps; i++) {
			if (PRIVATE_DATA->focus_aborted) {
				return meade_focus_abort(device);
			}
			indigo_usleep(1000);
		}
		if (!meade_no_reply_command(device, ":FQ#"))
			return false;
		return true;
	} else if (MOUNT_TYPE_ON_STEP_ITEM->sw.value) {
		if (!meade_no_reply_command(device, ":FR%+d#", steps))
			return false;
		// A controller that never reports the focuser standing still again must fail the
		// move instead of holding the device queue for the rest of the session.
		double deadline = indigo_monotonic_time() + ONSTEP_FOCUS_TIMEOUT;
		while (true) {
			if (PRIVATE_DATA->focus_aborted) {
				return meade_focus_abort(device);
			}
			indigo_usleep(100000);
			if (!meade_command(device, ":FT#"))
				return false;
			if (*PRIVATE_DATA->response == 'S') {
				break;
			}
			if (indigo_monotonic_time() > deadline) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Onstep focuser did not finish the move within %g s, last status '%s'", ONSTEP_FOCUS_TIMEOUT, PRIVATE_DATA->response);
				meade_focus_abort(device);
				return false;
			}
		}
		return true;
	}
	return false;
}

static bool meade_detect_generic_mount(indigo_device *device) {
	// These commands are found in the classic LX200 Instruction Manual, and the compatible mounts refer to that manual.
	if (!meade_command(device, ":GR#") || strlen(PRIVATE_DATA->response) == 0) {
		INDIGO_LOG(indigo_log(":GR# failed."));
		return false;
	}
	PRIVATE_DATA->response[0] = 0;
	if (!meade_command(device, ":GD#") || strlen(PRIVATE_DATA->response) == 0) {
		INDIGO_LOG(indigo_log(":GD# failed."));
		return false;
	}
	PRIVATE_DATA->response[0] = 0;
	if (!meade_command(device, ":GC#") || strlen(PRIVATE_DATA->response) == 0) {
		INDIGO_LOG(indigo_log(":GC# failed."));
		return false;
	}
	PRIVATE_DATA->response[0] = 0;
	if (!meade_command(device, ":GL#") || strlen(PRIVATE_DATA->response) == 0) {
		INDIGO_LOG(indigo_log(":GL# failed."));
		return false;
	}
	PRIVATE_DATA->response[0] = 0;
	if (!meade_command(device, ":GG#") || strlen(PRIVATE_DATA->response) == 0) {
		INDIGO_LOG(indigo_log(":GG# failed."));
		return false;
	}
	PRIVATE_DATA->response[0] = 0;
	if (!meade_command(device, ":GS#") || strlen(PRIVATE_DATA->response) == 0) {
		INDIGO_LOG(indigo_log(":GS# failed."));
		return false;
	}
	PRIVATE_DATA->response[0] = 0;
	if (!meade_command(device, ":Gg#") || strlen(PRIVATE_DATA->response) == 0) {
		INDIGO_LOG(indigo_log(":Gg# failed."));
		return false;
	}
	PRIVATE_DATA->response[0] = 0;
	if (!meade_command(device, ":Gt#") || strlen(PRIVATE_DATA->response) == 0) {
		INDIGO_LOG(indigo_log(":Gt# failed."));
		return false;
	}
	return true;
}

// A GTO servo controller leaves :GVP# unanswered. A GTOCP4 or later identifies itself with its :V# version, e.g.
// VCP4-P02-15, and answers at once, so the probe waits only briefly and costs a mount that answers neither little.
static bool meade_detect_ap_mount(indigo_device *device) {
	double timeout = PRIVATE_DATA->timeout;
	PRIVATE_DATA->timeout = 0.5;
	bool result = meade_command(device, ":V#") && !strncmp(PRIVATE_DATA->response, "VCP", 3);
	PRIVATE_DATA->timeout = timeout;
	if (result) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Version: %s", PRIVATE_DATA->response);
	}
	return result;
}

// The :GVP# product of a Meade controller: LX2001 for the LX200GPS, LX800, Autostar and its
// successor Audiostar, RCX400, and an LXD600 that answers both :GVP# and :GVN# with 6.12S.
static bool meade_is_meade_product(const char *product) {
	return !strncmp(product, "LX", 2) || !strncmp(product, "Autostar", 8) || !strncmp(product, "Audiostar", 9) || !strncmp(product, "RCX", 3) || !strcmp(product, "6.12S");
}

// What the product and the firmware version of a Meade controller decide: Autostar firmware
// before 31Ee has no :Mg pulse, the Autostar II models (LX200GPS, LX800, RCX400) take a guide
// rate with :Rg, and an LXD600 has neither long format nor park. The guider reads this as well,
// because it can be connected without the mount.
static void meade_read_meade_firmware(indigo_device *device) {
	if (*PRIVATE_DATA->product == 0 && meade_command(device, ":GVP#")) {
		snprintf(PRIVATE_DATA->product, sizeof(PRIVATE_DATA->product), "%s", PRIVATE_DATA->response);
	}
	char firmware[64] = "";
	if (meade_command(device, ":GVN#")) {
		snprintf(firmware, sizeof(firmware), "%s", PRIVATE_DATA->response);
	}
	PRIVATE_DATA->meade_lxd600 = !strcmp(PRIVATE_DATA->product, "6.12S");
	PRIVATE_DATA->meade_rg_guide_rate = !strncmp(PRIVATE_DATA->product, "LX2001", 6) || !strncmp(PRIVATE_DATA->product, "LX800", 5) || !strncmp(PRIVATE_DATA->product, "RCX", 3);
	PRIVATE_DATA->meade_host_timed_guiding = false;
	if (!strncmp(PRIVATE_DATA->product, "Autostar", 8) && isdigit((unsigned char)firmware[0]) && isdigit((unsigned char)firmware[1])) {
		int major = (firmware[0] - '0') * 10 + firmware[1] - '0';
		PRIVATE_DATA->meade_host_timed_guiding = major < 31 || (major == 31 && strcasecmp(firmware + 2, "Ee") < 0);
	}
	INDIGO_DRIVER_LOG(DRIVER_NAME, "Meade %s %s: %s, %s", PRIVATE_DATA->product, firmware, PRIVATE_DATA->meade_host_timed_guiding ? "host timed guiding" : ":Mg guiding", PRIVATE_DATA->meade_rg_guide_rate ? ":Rg guide rate" : "no guide rate");
}

static bool meade_detect_mount(indigo_device *device) {
	bool result = true;
	if (meade_command(device, ":GVP#")) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Product: %s", PRIVATE_DATA->response);
		strncpy(PRIVATE_DATA->product, PRIVATE_DATA->response, sizeof(PRIVATE_DATA->product) - 1);
		PRIVATE_DATA->product[sizeof(PRIVATE_DATA->product) - 1] = 0;
		MOUNT_TYPE_PROPERTY->state = INDIGO_OK_STATE;
		if (meade_is_meade_product(PRIVATE_DATA->product)) {
			indigo_set_switch(MOUNT_TYPE_PROPERTY, MOUNT_TYPE_MEADE_ITEM, true);
		} else if (!strncmp(PRIVATE_DATA->product, "10micron", 8)) {
			indigo_set_switch(MOUNT_TYPE_PROPERTY, MOUNT_TYPE_10MICRONS_ITEM, true);
		} else if (!strncmp(PRIVATE_DATA->product, "Losmandy", 8)) {
			indigo_set_switch(MOUNT_TYPE_PROPERTY, MOUNT_TYPE_GEMINI_ITEM, true);
		} else if (!strncmp(PRIVATE_DATA->product, "Avalon", 6)) {
			indigo_set_switch(MOUNT_TYPE_PROPERTY, MOUNT_TYPE_STARGO_ITEM, true);
		} else if (!strncmp(PRIVATE_DATA->product, "On-Step", 7)) {
			indigo_set_switch(MOUNT_TYPE_PROPERTY, MOUNT_TYPE_ON_STEP_ITEM, true);
		} else if (!strncmp(PRIVATE_DATA->product, "TeenAstro", 9)) {
			indigo_set_switch(MOUNT_TYPE_PROPERTY, MOUNT_TYPE_TEEN_ASTRO_ITEM, true);
		} else if (!strncmp(PRIVATE_DATA->product, "AM", 2) && isdigit(PRIVATE_DATA->product[2])) {
			indigo_set_switch(MOUNT_TYPE_PROPERTY, MOUNT_TYPE_ZWO_ITEM, true);
		} else if (!strncmp(PRIVATE_DATA->product, "NYX", 3)) {
			indigo_set_switch(MOUNT_TYPE_PROPERTY, MOUNT_TYPE_NYX_ITEM, true);
		} else if (!strncmp(PRIVATE_DATA->product, "OpenAstroTracker", 16)) {
			indigo_set_switch(MOUNT_TYPE_PROPERTY, MOUNT_TYPE_OAT_ITEM, true);
		} else if (!strncmp(PRIVATE_DATA->product, "aGotino", 7)) {
			indigo_set_switch(MOUNT_TYPE_PROPERTY, MOUNT_TYPE_AGOTINO_ITEM, true);
		} else if (!strncasecmp(PRIVATE_DATA->product, "esp32go", 7)) {
			indigo_set_switch(MOUNT_TYPE_PROPERTY, MOUNT_TYPE_ESP32GO_ITEM, true);
		} else if (*PRIVATE_DATA->product == 0 && meade_detect_ap_mount(device)) {
			indigo_set_switch(MOUNT_TYPE_PROPERTY, MOUNT_TYPE_AP_ITEM, true);
		} else {
			// The classic LX200 and some of the LX200-compatible mounts doesn't implement ":GVP#"
			if (meade_detect_generic_mount(device)) {
				indigo_set_switch(MOUNT_TYPE_PROPERTY, MOUNT_TYPE_GENERIC_ITEM, true);
			} else {
				MOUNT_TYPE_PROPERTY->state = INDIGO_ALERT_STATE;
				result = false;
			}
		}
	} else {
		MOUNT_TYPE_PROPERTY->state = INDIGO_ALERT_STATE;
		result = false;
	}
	indigo_update_property(device, MOUNT_TYPE_PROPERTY, NULL);
	return result;
}

// ---------------------------------------------------------------------  mount specific init & state update

static void meade_init_meade_mount(indigo_device *device) {
	MOUNT_MODE_PROPERTY->hidden = false;
	MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
	MOUNT_UTC_TIME_PROPERTY->hidden = false;
	MOUNT_PARK_PROPERTY->count = 1;
	MOUNT_PARK_PROPERTY->rule = INDIGO_AT_MOST_ONE_RULE;
	MOUNT_PARK_PARKED_ITEM->sw.value = false;
	strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "Meade");
	meade_read_meade_firmware(device);
	MOUNT_GUIDE_RATE_PROPERTY->hidden = !PRIVATE_DATA->meade_rg_guide_rate;
	MOUNT_PARK_PROPERTY->hidden = PRIVATE_DATA->meade_lxd600;
	if (meade_command(device, ":GVF#")) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Version: %s", PRIVATE_DATA->response);
		char *sep = strchr(PRIVATE_DATA->response, '|');
		if (sep != NULL) {
			*sep = 0;
		}
		INDIGO_COPY_VALUE(MOUNT_INFO_MODEL_ITEM->text.value, PRIVATE_DATA->response);
	}
	if (PRIVATE_DATA->meade_lxd600) {
		strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "LXD600");
	}
	if (meade_command(device, ":GVN#")) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Firmware: %s", PRIVATE_DATA->response);
		INDIGO_COPY_VALUE(MOUNT_INFO_FIRMWARE_ITEM->text.value, PRIVATE_DATA->response);
	}
	// Older firmware does not answer :GW# at all, and the ACK byte then gives the alignment:
	// A alt-az, P polar, G German polar or L land, which is an alt-az mount that does not
	// track. Polling :GW# on such a mount would wait out the timeout on every poll.
	PRIVATE_DATA->meade_alignment = 0;
	PRIVATE_DATA->meade_no_gw = !meade_command(device, ":GW#") || *PRIVATE_DATA->response == 0;
	if (!PRIVATE_DATA->meade_no_gw) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Status: %s", PRIVATE_DATA->response);
		PRIVATE_DATA->meade_alignment = *PRIVATE_DATA->response;
	} else if (meade_simple_reply_command(device, "\006") && *PRIVATE_DATA->response) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Alignment: %c", *PRIVATE_DATA->response);
		PRIVATE_DATA->meade_alignment = *PRIVATE_DATA->response == 'L' ? 'A' : *PRIVATE_DATA->response;
	}
	if (PRIVATE_DATA->meade_alignment == 'P' || PRIVATE_DATA->meade_alignment == 'G') {
		indigo_set_switch(MOUNT_MODE_PROPERTY, EQUATORIAL_ITEM, true);
	} else {
		indigo_set_switch(MOUNT_MODE_PROPERTY, ALTAZ_MODE_ITEM, true);
	}
	if (meade_command(device, ":GH#")) {
		PRIVATE_DATA->use_dst_commands = *PRIVATE_DATA->response != 0;
	}
}

static void meade_update_meade_state(indigo_device *device) {
	if (PRIVATE_DATA->meade_park_silent) {
		PRIVATE_DATA->parked = true;
		return;
	}
	// :D# answers a bare # when nothing moves. An Autostar sent to its park position stops
	// answering anything until it is switched off, so a park followed by silence is a mount
	// that has parked, and nothing is sent to it from then on.
	long received = meade_counted_command(device, ":D#");
	if (received >= 0) {
		PRIVATE_DATA->slewing = *PRIVATE_DATA->response;
		if (MOUNT_PARK_PROPERTY->state == INDIGO_BUSY_STATE) {
			PRIVATE_DATA->parking = PRIVATE_DATA->slewing;
			PRIVATE_DATA->parked = !PRIVATE_DATA->slewing;
			if (received == 0) {
				PRIVATE_DATA->meade_park_silent = true;
				indigo_send_message(device, OK_PROPERTY, "The mount stopped answering after the park, switch it off before the next session");
			}
		}
	}
	if (PRIVATE_DATA->meade_park_silent) {
		return;
	}
	if (PRIVATE_DATA->meade_no_gw) {
		if (meade_simple_reply_command(device, "\006") && *PRIVATE_DATA->response) {
			PRIVATE_DATA->tracking = *PRIVATE_DATA->response != 'L';
		}
	} else if (meade_command(device, ":GW#")) {
		PRIVATE_DATA->tracking = PRIVATE_DATA->response[1] == 'T';
	}
}

static void meade_init_10microns_mount(indigo_device *device) {
	MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
	MOUNT_UTC_TIME_PROPERTY->hidden = false;
	MOUNT_HOME_PROPERTY->hidden = false;
	MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
	MOUNT_INFO_PROPERTY->count = 1;
	strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "10Micron");
	indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
	indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
	meade_no_reply_command(device, ":EMUAP#");
	meade_no_reply_command(device, ":U1#");
}

static void meade_update_10microns_state(indigo_device *device) {
	if (meade_command(device, ":Gstat#")) {
		switch (atoi(PRIVATE_DATA->response)) {
			case 0:
				PRIVATE_DATA->tracking = true;
				break;
			case 2:
				PRIVATE_DATA->parking = true;
				break;
			case 4:
				PRIVATE_DATA->homing = true;
				break;
			case 5:
				PRIVATE_DATA->parked = true;
				break;
			case 6:
				PRIVATE_DATA->slewing = true;
				break;
			case 7:
				if (MOUNT_HOME_PROPERTY->state == INDIGO_BUSY_STATE) {
					PRIVATE_DATA->homed = true;
				}
				break;
		}
	}
}

// The software level from :GV# (<l><vv>, for example 512) and, on Level 4, which cuts a :Mg
// pulse to 255 motor encoder ticks modulo 256, the longest pulse that stays below that limit.
// It is computed for the fastest guiding motion, westwards at (1 + guide speed) times the
// sidereal rate, from the RA worm ratio <21, the encoder ticks per worm turn <27 and the guide
// speed <150. The guider reads this as well, because it can be connected without the mount.
// A Gemini has one guiding speed, which the mount shows as MOUNT_GUIDE_RATE and the guider as
// GUIDER_RATE, so a change made through one device is published by the other one as well.
static void gemini_show_mount_guide_rate(indigo_device *device, double rate) {
	if (IS_CONNECTED && !MOUNT_GUIDE_RATE_PROPERTY->hidden) {
		MOUNT_GUIDE_RATE_RA_ITEM->number.value = MOUNT_GUIDE_RATE_RA_ITEM->number.target = rate;
		MOUNT_GUIDE_RATE_DEC_ITEM->number.value = MOUNT_GUIDE_RATE_DEC_ITEM->number.target = rate;
		indigo_update_property(device, MOUNT_GUIDE_RATE_PROPERTY, NULL);
	}
}

static void gemini_show_guider_rate(indigo_device *device, double rate) {
	if (device != NULL && IS_CONNECTED && !GUIDER_RATE_PROPERTY->hidden) {
		GUIDER_RATE_ITEM->number.value = GUIDER_RATE_ITEM->number.target = rate;
		indigo_update_property(device, GUIDER_RATE_PROPERTY, NULL);
	}
}

static void gemini_read_guiding(indigo_device *device) {
	PRIVATE_DATA->gemini_level = 0;
	PRIVATE_DATA->gemini_pulse_chunk = 0;
	if (meade_command(device, ":GV#") && isdigit((unsigned char)*PRIVATE_DATA->response)) {
		PRIVATE_DATA->gemini_level = *PRIVATE_DATA->response - '0';
	}
	char worm[32], steps[32], speed[32];
	if (PRIVATE_DATA->gemini_level == 4 && gemini_get(device, 21, worm, sizeof(worm)) && gemini_get(device, 27, steps, sizeof(steps)) && gemini_get(device, 150, speed, sizeof(speed))) {
		double ticks_per_turn = fabs(atof(worm)) * atof(steps);
		double guide_speed = indigo_atod(speed);
		if (ticks_per_turn > 0 && guide_speed > 0) {
			double ticks_per_second = (1 + guide_speed) * 15.041 * ticks_per_turn / 1296000.0;
			PRIVATE_DATA->gemini_pulse_chunk = (int)fmax(100, floor(255 / ticks_per_second * 1000));
		}
	}
	INDIGO_DRIVER_LOG(DRIVER_NAME, "Gemini level %d, longest pulse %d ms", PRIVATE_DATA->gemini_level, PRIVATE_DATA->gemini_pulse_chunk);
}

static void meade_init_gemini_mount(indigo_device *device) {
	MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
	MOUNT_UTC_TIME_PROPERTY->hidden = false;
	MOUNT_SIDE_OF_PIER_PROPERTY->hidden = false;
	GEMINI_PARK_POSITION_PROPERTY->hidden = false;
	MOUNT_INFO_PROPERTY->count = 3;
	MOUNT_TRACK_RATE_PROPERTY->count = 4;
	strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "Losmandy");
	strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "Gemini");
	if (meade_command(device, ":GVN#") && *PRIVATE_DATA->response) {
		INDIGO_COPY_VALUE(MOUNT_INFO_FIRMWARE_ITEM->text.value, PRIVATE_DATA->response);
	}
	meade_no_reply_command(device, ":p0#");
	gemini_read_guiding(device);
	// One guiding speed for both axes, 0.2 to 0.8 times the sidereal rate. Native 150.
	char speed[32];
	MOUNT_GUIDE_RATE_PROPERTY->hidden = !gemini_get(device, 150, speed, sizeof(speed));
	if (!MOUNT_GUIDE_RATE_PROPERTY->hidden) {
		MOUNT_GUIDE_RATE_RA_ITEM->number.min = MOUNT_GUIDE_RATE_DEC_ITEM->number.min = 20;
		MOUNT_GUIDE_RATE_RA_ITEM->number.max = MOUNT_GUIDE_RATE_DEC_ITEM->number.max = 80;
		MOUNT_GUIDE_RATE_RA_ITEM->number.step = MOUNT_GUIDE_RATE_DEC_ITEM->number.step = 10;
		MOUNT_GUIDE_RATE_RA_ITEM->number.value = MOUNT_GUIDE_RATE_RA_ITEM->number.target = MOUNT_GUIDE_RATE_DEC_ITEM->number.value = MOUNT_GUIDE_RATE_DEC_ITEM->number.target = round(indigo_atod(speed) * 100);
	}
}

static void meade_update_gemini_state(indigo_device *device) {
	PRIVATE_DATA->gemini_velocity = 0;
	if (meade_command(device, ":Gv#")) {
		bool was_stalled = PRIVATE_DATA->stalled;
		PRIVATE_DATA->gemini_velocity = PRIVATE_DATA->response[0];
		switch (PRIVATE_DATA->response[0]) {
			case 'S':
			case 'C':
				// The velocity is the faster of the two axes, so a slew or a centering
				// motion hides the tracking that goes on underneath and resumes after it.
				PRIVATE_DATA->slewing = true;
				PRIVATE_DATA->tracking = true;
				PRIVATE_DATA->stalled = false;
				break;
			case 'T':
			case 'G':
				PRIVATE_DATA->tracking = true;
				PRIVATE_DATA->stalled = false;
				break;
			case 'N':
				PRIVATE_DATA->stalled = false;
				break;
			case '!':
				// An axis stalled. The controller still answers every query and still
				// reports a position, so nothing else says the motion it was asked for is
				// not happening; without this a goto that stalled is published as one that
				// finished. Gemini Level 5 command description, :Gv#.
				PRIVATE_DATA->stalled = true;
				break;
		}
		if (PRIVATE_DATA->stalled && !was_stalled) {
			indigo_send_message(device, ALERT_PROPERTY, "Mount reports a stalled axis");
		} else if (was_stalled && !PRIVATE_DATA->stalled) {
			// The first velocity the controller reports again ends the fault.
			indigo_send_message(device, OK_PROPERTY, "Mount is moving again");
		}
	}
	if (meade_command(device, ":h?#")) {
		switch (PRIVATE_DATA->response[0]) {
			case '1':
				// :h?# keeps answering 1 after :hW# woke the mount up, so only a mount that
				// also stands still is parked; as soon as it moves it is not.
				if (PRIVATE_DATA->gemini_velocity == 'N') {
					PRIVATE_DATA->parked = true;
					PRIVATE_DATA->gemini_park_expected = false;
				}
				break;
			case '2':
				PRIVATE_DATA->parking = true;
				// The controller has taken the command, so a 0 from here on is a failure
				// rather than a park that has not arrived yet.
				PRIVATE_DATA->gemini_park_deadline = 0;
				break;
			case '0':
				// "No Prk command received or Park operation failed". Only what the driver
				// asked for tells the two apart: while a park it issued is still inside its
				// acknowledgement window this is the first, and once the controller has
				// reported the operation in progress, or the window has passed, the second.
				if (PRIVATE_DATA->gemini_park_expected && (PRIVATE_DATA->gemini_park_deadline == 0 || indigo_monotonic_time() > PRIVATE_DATA->gemini_park_deadline)) {
					PRIVATE_DATA->gemini_park_expected = false;
					PRIVATE_DATA->gemini_park_failed = true;
				}
				break;
		}
	}
	if (PRIVATE_DATA->gemini_park_failed) {
		// The generic state machine below mirrors the mount into the switch and the light
		// but leaves the property state alone, so the failure survives to the client.
		PRIVATE_DATA->gemini_park_failed = false;
		MOUNT_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_send_message(device, ALERT_PROPERTY, "Park failed");
	}
	if (meade_command(device, ":Gm#")) {
		if (PRIVATE_DATA->response[0] == 'W' && !MOUNT_SIDE_OF_PIER_WEST_ITEM->sw.value) {
			indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, MOUNT_SIDE_OF_PIER_WEST_ITEM, true);
		} else if (PRIVATE_DATA->response[0] == 'E' && !MOUNT_SIDE_OF_PIER_EAST_ITEM->sw.value) {
			indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, MOUNT_SIDE_OF_PIER_EAST_ITEM, true);
		}
	}
}

// A StarGO has no calendar and no clock command; it keeps the local sidereal time, which
// :X32HHMMSS# sets, and the longitude it computes the sidereal time on from.
static void stargo_sync_lst(indigo_device *device, double longitude) {
	time_t utc = time(NULL);
	double lst = indigo_lst(&utc, longitude);
	long seconds = ((long)llround(lst * 3600.0) % 86400L + 86400L) % 86400L;
	meade_no_reply_command(device, ":X32%02ld%02ld%02ld#", seconds / 3600, seconds / 60 % 60, seconds % 60);
	PRIVATE_DATA->stargo_lst_synced = indigo_monotonic_time();
}

static void meade_init_stargo_mount(indigo_device *device) {
	MOUNT_HOME_PROPERTY->hidden = false;
	MOUNT_INFO_PROPERTY->count = 2;
	strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "Avalon");
	strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "Avalon StarGO");
	indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
	indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
	meade_simple_reply_command(device, ":TTSFh#");
	if (meade_command(device, ":X22#")) {
		int ra, dec;
		if (sscanf(PRIVATE_DATA->response, "%db%d#", &ra, &dec) == 2) {
			MOUNT_GUIDE_RATE_RA_ITEM->number.value = MOUNT_GUIDE_RATE_RA_ITEM->number.target = ra;
			MOUNT_GUIDE_RATE_DEC_ITEM->number.value = MOUNT_GUIDE_RATE_DEC_ITEM->number.target = dec;
			MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_OK_STATE;
		}
	}
	// :TTSFd# would set the "force meridian flip" flag, which is the mount configuration's
	// business and is left alone. The sidereal time is the only time the StarGO keeps, and
	// it gets it from the client.
	stargo_sync_lst(device, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value);
}

static void meade_update_stargo_state(indigo_device *device) {
	// The sidereal time drifts unless it is synchronised now and then, every 22 s while the
	// mount does not slew.
	if (indigo_monotonic_time() - PRIVATE_DATA->stargo_lst_synced > 22 && !PRIVATE_DATA->goto_issued) {
		stargo_sync_lst(device, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value);
	}
	if (meade_command(device, ":X34#")) {
		// The motion digit of each axis is 0 stopped, 1 tracking and above 1 moving, through
		// the acceleration, the slew and the deceleration.
		PRIVATE_DATA->slewing = (PRIVATE_DATA->response[1] > '1' || PRIVATE_DATA->response[2] > '1');
		// Each StarGO motor has its own status digit; RA tracking also continues during DEC motion (m15).
		PRIVATE_DATA->tracking = PRIVATE_DATA->response[1] == '1';
	}
	if (meade_command(device, ":X38#")) {
		switch (PRIVATE_DATA->response[1]) {
			case '2':
				PRIVATE_DATA->parked = true;
				break;
			case 'B':
				PRIVATE_DATA->parking = true;
				break;
		}
	}
	if (MOUNT_HOME_PROPERTY->state == INDIGO_BUSY_STATE) {
		PRIVATE_DATA->homing = PRIVATE_DATA->slewing;
		PRIVATE_DATA->homed = !PRIVATE_DATA->slewing && !PRIVATE_DATA->tracking;
	}
}

static void meade_init_stargo2_mount(indigo_device *device) {
	MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
	MOUNT_TRACKING_PROPERTY->hidden = true;
	MOUNT_PARK_PROPERTY->hidden = true;
	MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
	MOUNT_INFO_PROPERTY->count = 2;
	strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "Avalon");
	strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "Avalon StarGO2");
}

static void meade_update_generic_state(indigo_device *device);

// Reads what the servo controller can do from its :V# version. A guider connected without the mount needs it
// for the pulse length.
static void meade_read_ap_version(indigo_device *device) {
	PRIVATE_DATA->ap_controller = 2;
	PRIVATE_DATA->ap_use_gos = PRIVATE_DATA->ap_high_precision = PRIVATE_DATA->ap_king = PRIVATE_DATA->ap_long_pulses = false;
	// The servo controller version: VCPn-Pxx-yy on a GTOCPn from the GTOCP4 on, a chip revision letter on earlier
	// boxes, S and later on a GTOCP3.
	if (meade_command(device, ":V#") && *PRIVATE_DATA->response) {
		char version[INDIGO_VALUE_SIZE];
		INDIGO_COPY_VALUE(version, PRIVATE_DATA->response);
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Version: %s", version);
		INDIGO_COPY_VALUE(MOUNT_INFO_FIRMWARE_ITEM->text.value, version);
		if (!strncmp(version, "VCP", 3) && version[3] >= '4' && version[3] <= '9') {
			PRIVATE_DATA->ap_controller = version[3] - '0';
			snprintf(MOUNT_INFO_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE, "GTOCP%c", version[3]);
			const char *revision = strlen(version) > 5 ? version + 5 : "";
			bool early = !strncmp(revision, "P01-00", 6) || !strncmp(revision, "P01-01", 6) || !strncmp(revision, "P01-02", 6) || !strncmp(revision, "P01-03", 6);
			PRIVATE_DATA->ap_high_precision = PRIVATE_DATA->ap_controller > 4 || !early;
			PRIVATE_DATA->ap_king = PRIVATE_DATA->ap_controller > 4 || strncmp(revision, "P02-08", 6) >= 0;
			PRIVATE_DATA->ap_long_pulses = true;
		} else if (strcmp(version, "S") >= 0) {
			PRIVATE_DATA->ap_controller = 3;
			strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "GTOCP3");
		}
		PRIVATE_DATA->ap_use_gos = PRIVATE_DATA->ap_controller >= 3;
	}
}

static void meade_init_ap_mount(indigo_device *device) {
	MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
	MOUNT_UTC_TIME_PROPERTY->hidden = false;
	MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
	MOUNT_SIDE_OF_PIER_PROPERTY->hidden = false;
	AP_SYNC_MODE_PROPERTY->hidden = false;
	MOUNT_INFO_PROPERTY->count = 3;
	strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "AstroPhysics");
	indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
	meade_no_reply_command(device, "#");
	meade_no_reply_command(device, ":U#");
	meade_no_reply_command(device, ":Br 00:00:00#");
	PRIVATE_DATA->ap_firmware_parks = PRIVATE_DATA->ap_parking = false;
	PRIVATE_DATA->ap_fault = '0';
	meade_read_ap_version(device);
	// Firmware park positions: always on a GTOCP5/6, flagged by bit 7 of :G_E# on a GTOCP3/4 or by a :G_S# of 160
	// on a GTOCP4.
	if (PRIVATE_DATA->ap_controller >= 5) {
		PRIVATE_DATA->ap_firmware_parks = true;
	} else if (PRIVATE_DATA->ap_controller >= 3) {
		if (meade_command(device, ":G_E#") && isdigit((unsigned char)*PRIVATE_DATA->response) && (atoi(PRIVATE_DATA->response) & 0x80)) {
			PRIVATE_DATA->ap_firmware_parks = true;
		} else if (PRIVATE_DATA->ap_controller == 4 && meade_command(device, ":G_S#") && atoi(PRIVATE_DATA->response) == 160) {
			PRIVATE_DATA->ap_firmware_parks = true;
		}
	}
	AP_PARK_POSITION_PROPERTY->hidden = !PRIVATE_DATA->ap_firmware_parks;
	if (PRIVATE_DATA->ap_king) {
		MOUNT_TRACK_RATE_PROPERTY->count = 4;
	}
	INDIGO_DRIVER_LOG(DRIVER_NAME, "GTOCP%d, status %s, high precision %s, King rate %s, firmware parks %s, long pulses %s", PRIVATE_DATA->ap_controller, PRIVATE_DATA->ap_use_gos ? "yes" : "no", PRIVATE_DATA->ap_high_precision ? "yes" : "no", PRIVATE_DATA->ap_king ? "yes" : "no", PRIVATE_DATA->ap_firmware_parks ? "yes" : "no", PRIVATE_DATA->ap_long_pulses ? "yes" : "no");
}

// Decodes the :GOS# status of a GTOCP3 from revision S on and of every later controller: 1 park (P parked),
// 2 tracking (0 lunar, 1 solar, 2 sidereal, T King, C/c custom, 9 stopped), 4 slewing (S), 11 fault.
static bool meade_update_ap_status(indigo_device *device) {
	if (!meade_command(device, ":GOS#") || strlen(PRIVATE_DATA->response) < 11) {
		return false;
	}
	char status[INDIGO_VALUE_SIZE];
	INDIGO_COPY_VALUE(status, PRIVATE_DATA->response);
	PRIVATE_DATA->parked = status[0] == 'P';
	if (PRIVATE_DATA->parked) {
		PRIVATE_DATA->ap_parking = false;
	}
	PRIVATE_DATA->parking = PRIVATE_DATA->ap_parking;
	PRIVATE_DATA->tracking = !PRIVATE_DATA->parked && status[1] != '9';
	PRIVATE_DATA->slewing = status[3] == 'S';
	if (PRIVATE_DATA->tracking && MOUNT_TRACK_RATE_PROPERTY->state != INDIGO_BUSY_STATE) {
		indigo_item *rate = status[1] == '0' ? MOUNT_TRACK_RATE_LUNAR_ITEM : status[1] == '1' ? MOUNT_TRACK_RATE_SOLAR_ITEM : status[1] == '2' ? MOUNT_TRACK_RATE_SIDEREAL_ITEM : status[1] == 'T' && PRIVATE_DATA->ap_king ? MOUNT_TRACK_RATE_KING_ITEM : NULL;
		if (rate != NULL && !rate->sw.value) {
			indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, rate, true);
			indigo_update_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
		}
	}
	char fault = status[10];
	// A stalled motor or a servo fault means the axis is not where a slew would have taken it.
	PRIVATE_DATA->stalled = fault == '1' || fault == 'Z' || fault == '4' || fault == 'X';
	if (fault != PRIVATE_DATA->ap_fault) {
		PRIVATE_DATA->ap_fault = fault;
		const char *message = NULL;
		switch (fault) {
			case '0': message = NULL; break;
			case '1': case 'Z': message = "Motor stall"; break;
			case '2': case 'Y': message = "Low power supply voltage"; break;
			case '4': case 'X': message = "Servo fault"; break;
			case 'N': message = "CCW internal declination limit or absolute encoder limit"; break;
			case 'S': message = "CW internal declination limit or absolute encoder limit"; break;
			case 'E': message = "East internal right ascension limit or absolute encoder limit"; break;
			case 'W': message = "West internal right ascension limit or absolute encoder limit"; break;
			case 'z': message = "Kill function has been issued"; break;
			default: message = "Unknown fault"; break;
		}
		if (message != NULL) {
			indigo_send_message(device, ALERT_PROPERTY, "Mount reports: %s", message);
		}
	}
	return true;
}

static void meade_update_ap_state(indigo_device *device) {
	if (!PRIVATE_DATA->ap_use_gos || !meade_update_ap_status(device)) {
		// Without a status the motion is guessed from the coordinates and tracking is what was last requested.
		meade_update_generic_state(device);
		PRIVATE_DATA->tracking = MOUNT_TRACKING_ON_ITEM->sw.value;
	}
	if (meade_command(device, ":pS#")) {
		if (!strcmp(PRIVATE_DATA->response, "West") && !MOUNT_SIDE_OF_PIER_WEST_ITEM->sw.value) {
			indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, MOUNT_SIDE_OF_PIER_WEST_ITEM, true);
		} else if (!strcmp(PRIVATE_DATA->response, "East") && !MOUNT_SIDE_OF_PIER_EAST_ITEM->sw.value) {
			indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, MOUNT_SIDE_OF_PIER_EAST_ITEM, true);
		}
	}
}

static void meade_init_onstep_mount(indigo_device *device) {
	MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
	MOUNT_UTC_TIME_PROPERTY->hidden = false;
	MOUNT_PARK_SET_PROPERTY->hidden = false;
	MOUNT_PARK_SET_PROPERTY->count = 1;
	MOUNT_HOME_PROPERTY->hidden = false;
	MOUNT_HOME_PROPERTY->count = 2;
	MOUNT_HOME_PROPERTY->rule = INDIGO_ONE_OF_MANY_RULE;
	MOUNT_HOME_SET_PROPERTY->hidden = false;
	MOUNT_HOME_SET_PROPERTY->count = 1;
	MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
	MOUNT_TRACK_RATE_PROPERTY->count = 4;
	MOUNT_SIDE_OF_PIER_PROPERTY->hidden = false;
	MOUNT_PEC_PROPERTY->hidden = false;
	strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "On-Step");
	if (meade_command(device, ":GVP#")) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Model: %s", PRIVATE_DATA->response);
		INDIGO_COPY_VALUE(MOUNT_INFO_MODEL_ITEM->text.value, PRIVATE_DATA->response);
	}
	if (meade_command(device, ":GVN#")) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Firmware: %s", PRIVATE_DATA->response);
		INDIGO_COPY_VALUE(MOUNT_INFO_FIRMWARE_ITEM->text.value, PRIVATE_DATA->response);
	}
	if (meade_command(device, ":$QZ?#")) {
		indigo_set_switch(MOUNT_PEC_PROPERTY, PRIVATE_DATA->response[0] == 'P' ? MOUNT_PEC_ENABLED_ITEM : MOUNT_PEC_DISABLED_ITEM, true);
	}
	if (meade_command(device, ":GX96#")) {
		if (PRIVATE_DATA->response[0] == 'E') {
			indigo_set_switch(ONSTEP_PREFERRED_PIER_SIDE_PROPERTY, ONSTEP_PREFERRED_PIER_SIDE_EAST_ITEM, true);
		} else if (PRIVATE_DATA->response[0] == 'W') {
			indigo_set_switch(ONSTEP_PREFERRED_PIER_SIDE_PROPERTY, ONSTEP_PREFERRED_PIER_SIDE_WEST_ITEM, true);
		} else if (PRIVATE_DATA->response[0] == 'B') {
			indigo_set_switch(ONSTEP_PREFERRED_PIER_SIDE_PROPERTY, ONSTEP_PREFERRED_PIER_SIDE_BEST_ITEM, true);
		} else {
			indigo_set_switch(ONSTEP_PREFERRED_PIER_SIDE_PROPERTY, ONSTEP_PREFERRED_PIER_SIDE_AUTO_ITEM, true);
		}
		ONSTEP_PREFERRED_PIER_SIDE_PROPERTY->hidden = false;
	}
	if (meade_command(device, ":GX95#")) {
		indigo_set_switch(ONSTEP_AUTO_MERIDIAN_FLIP_PROPERTY, PRIVATE_DATA->response[0] == '1' ? ONSTEP_AUTO_MERIDIAN_FLIP_ENABLED_ITEM : ONSTEP_AUTO_MERIDIAN_FLIP_DISABLED_ITEM, true);
		ONSTEP_AUTO_MERIDIAN_FLIP_PROPERTY->hidden = false;
	}
	if (meade_command(device, ":GXE9#")) {
		ONSTEP_MERIDIAN_LIMITS_EAST_ITEM->number.value = ONSTEP_MERIDIAN_LIMITS_EAST_ITEM->number.target = atof(PRIVATE_DATA->response) / 4.0;
		if (meade_command(device, ":GXEA#")) {
			ONSTEP_MERIDIAN_LIMITS_WEST_ITEM->number.value = ONSTEP_MERIDIAN_LIMITS_WEST_ITEM->number.target = atof(PRIVATE_DATA->response) / 4.0;
			ONSTEP_MERIDIAN_LIMITS_PROPERTY->hidden = false;
		}
	}
	if (meade_command(device, ":Gh#")) {
		ONSTEP_ALTITUDE_LIMITS_HORIZON_ITEM->number.value = ONSTEP_ALTITUDE_LIMITS_HORIZON_ITEM->number.target = atof(PRIVATE_DATA->response);
		if (meade_command(device, ":Go#")) {
			ONSTEP_ALTITUDE_LIMITS_OVERHEAD_ITEM->number.value = ONSTEP_ALTITUDE_LIMITS_OVERHEAD_ITEM->number.target = atof(PRIVATE_DATA->response);
			ONSTEP_ALTITUDE_LIMITS_PROPERTY->hidden = false;
		}
	}
}

static void meade_update_onstep_state(indigo_device *device) {
	if (meade_command(device, ":GU#")) {
		if (strchr(PRIVATE_DATA->response, 'N') == NULL) {
			PRIVATE_DATA->slewing = true;
			if (MOUNT_HOME_PROPERTY->state == INDIGO_BUSY_STATE) {
				PRIVATE_DATA->homing = true;
			}
		}
		if (strchr(PRIVATE_DATA->response, 'n') == NULL) {
			PRIVATE_DATA->tracking = true;
		}
		if (strchr(PRIVATE_DATA->response, 'P')) {
			PRIVATE_DATA->parked = true;
		} else if (strchr(PRIVATE_DATA->response, 'I')) {
			PRIVATE_DATA->parking = true;
		}
		if (strchr(PRIVATE_DATA->response, 'h')) {
			PRIVATE_DATA->homing = true;
		} else if (strchr(PRIVATE_DATA->response, 'H')) {
			PRIVATE_DATA->homed = true;
		}
		if (strchr(PRIVATE_DATA->response, 'o')) {
			MOUNT_SIDE_OF_PIER_WEST_ITEM->sw.value = MOUNT_SIDE_OF_PIER_EAST_ITEM->sw.value = false;
			MOUNT_SIDE_OF_PIER_PROPERTY->state = INDIGO_IDLE_STATE;
		} else if (strchr(PRIVATE_DATA->response, 'W')) {
			indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, MOUNT_SIDE_OF_PIER_WEST_ITEM, true);
			MOUNT_SIDE_OF_PIER_PROPERTY->state = INDIGO_OK_STATE;
		} else if (strchr(PRIVATE_DATA->response, 'T')) {
			indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, MOUNT_SIDE_OF_PIER_EAST_ITEM, true);
			MOUNT_SIDE_OF_PIER_PROPERTY->state = INDIGO_OK_STATE;
		}
		// Update tracking rate from :GU# status characters: ( = Lunar, O = Solar, k = King, else = Sidereal
		indigo_item *rate_item = NULL;
		bool rate_from_status = true;
		if (MOUNT_TRACK_RATE_PROPERTY->state != INDIGO_BUSY_STATE) {
			if (strchr(PRIVATE_DATA->response, '(')) {
				rate_item = MOUNT_TRACK_RATE_LUNAR_ITEM;
			} else if (strchr(PRIVATE_DATA->response, 'O')) {
				rate_item = MOUNT_TRACK_RATE_SOLAR_ITEM;
			} else if (strchr(PRIVATE_DATA->response, 'k')) {
				rate_item = MOUNT_TRACK_RATE_KING_ITEM;
			} else {
				// OnStepX 10.28x never puts the documented k in its status, so a status with
				// no rate character at all is sidereal or king. The two are told apart by the
				// tracking frequency, which is settled below, after the status string has
				// been read for everything else it carries.
				rate_from_status = false;
			}
		}
		// Update auto meridian flip from :GU# status character: a = enabled
		bool flip_enabled = strchr(PRIVATE_DATA->response, 'a') != NULL;
		if (!rate_from_status && meade_command(device, ":GT#")) {
			double frequency = atof(PRIVATE_DATA->response);
			if (fabs(frequency - ONSTEP_KING_FREQUENCY) < ONSTEP_RATE_TOLERANCE) {
				rate_item = MOUNT_TRACK_RATE_KING_ITEM;
			} else if (frequency > 0) {
				rate_item = MOUNT_TRACK_RATE_SIDEREAL_ITEM;
			}
			// A disabled tracking answers 0, which says nothing about the configured rate,
			// so the rate the driver already holds is kept.
		}
		// A request copied during the :GT# round trip owns the property, its handler reads the target.
		if (rate_item != NULL && !rate_item->sw.value && MOUNT_TRACK_RATE_PROPERTY->state != INDIGO_BUSY_STATE) {
			indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, rate_item, true);
			indigo_update_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
		}
		if (!ONSTEP_AUTO_MERIDIAN_FLIP_PROPERTY->hidden && ONSTEP_AUTO_MERIDIAN_FLIP_PROPERTY->state != INDIGO_BUSY_STATE) {
			indigo_item *flip_item = flip_enabled ? ONSTEP_AUTO_MERIDIAN_FLIP_ENABLED_ITEM : ONSTEP_AUTO_MERIDIAN_FLIP_DISABLED_ITEM;
			if (!flip_item->sw.value) {
				indigo_set_switch(ONSTEP_AUTO_MERIDIAN_FLIP_PROPERTY, flip_item, true);
				indigo_update_property(device, ONSTEP_AUTO_MERIDIAN_FLIP_PROPERTY, NULL);
			}
		}
	}
}

static void meade_init_agotino_mount(indigo_device *device) {
	MOUNT_TRACKING_PROPERTY->hidden = true;
	MOUNT_PARK_PROPERTY->hidden = true;
	MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
	MOUNT_TRACK_RATE_PROPERTY->hidden = true;
	MOUNT_SLEW_RATE_PROPERTY->hidden = true;
	MOUNT_MOTION_RA_PROPERTY->hidden = true;
	MOUNT_MOTION_DEC_PROPERTY->hidden = true;
	strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "aGotino");
	// The firmware has no model query, so the model is the product name :GVP# already gave
	// the autodetection. Leaving it on the "Unknown" the generic initialization writes tells
	// a client less than the driver knows.
	INDIGO_COPY_VALUE(MOUNT_INFO_MODEL_ITEM->text.value, PRIVATE_DATA->product);
	if (meade_command(device, ":GVN#")) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Firmware: %s", PRIVATE_DATA->response);
		INDIGO_COPY_VALUE(MOUNT_INFO_FIRMWARE_ITEM->text.value, PRIVATE_DATA->response);
	}
}

static void meade_update_agotino_state(indigo_device *device) {
	// The aGotino answers :D# with the classic distance bar, an empty reply while it is idle
	// and one DEL character while a slew is running. Its :GR# and :GD# keep reporting the
	// position the slew started from until the slew ends, so the generic "the coordinates
	// moved" heuristic would publish a goto as finished the moment it was issued and the
	// next command would reach a controller still busy inside its slew loop.
	if (meade_command(device, ":D#")) {
		PRIVATE_DATA->slewing = *PRIVATE_DATA->response;
		if (!PRIVATE_DATA->slewing && PRIVATE_DATA->goto_issued) {
			// GR/GD precede D in the poll; the slew may have ended between those reads.
			double ra = 0, dec = 0;
			PRIVATE_DATA->coordinate_read_failed = !meade_get_coordinates(device, &ra, &dec);
			if (PRIVATE_DATA->coordinate_read_failed) {
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			} else {
				indigo_eq_to_j2k(MOUNT_EPOCH_ITEM->number.value, &ra, &dec);
				MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = ra;
				MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = dec;
			}
		}
	}
}

// :GTa# answers ft±nn#: f automatic flip at the limit, t tracking past the meridian (0 or 1)
// and the limit in degrees past the meridian, negative before it.
static bool zwo_get_meridian(indigo_device *device, bool *flip, bool *track, int *limit) {
	if (!meade_command(device, ":GTa#") || strlen(PRIVATE_DATA->response) != 5 || (PRIVATE_DATA->response[2] != '+' && PRIVATE_DATA->response[2] != '-')) {
		return false;
	}
	*flip = PRIVATE_DATA->response[0] != '0';
	*track = PRIVATE_DATA->response[1] != '0';
	*limit = atoi(PRIVATE_DATA->response + 2);
	return true;
}

static bool zwo_set_meridian(indigo_device *device, bool flip, bool track, int limit) {
	if (limit < -15 || limit > 15) {
		return false;
	}
	return meade_simple_reply_command(device, ":STa%c%c%+03d#", flip ? '1' : '0', track ? '1' : '0', limit) && *PRIVATE_DATA->response == '1';
}

// The error codes of the ZWO protocol, as :GAT# reports them. Firmware 1.1.1 and later.
static const char *zwo_tracking_error(int code) {
	static const char *messages[] = { "", "Parameter out of range", "Format error", "Homing, slewing or goto in progress", "Mount is moving", "Target is below the horizon", "Target is below the altitude limit", "Time and site are not set", "Meridian reached, tracking stopped", "Sync point is on the other side of the meridian", "Altitude inverted", "Sync near the pole refused", "Sync too far from the current position" };
	return code > 0 && code < 13 ? messages[code] : "";
}

static void meade_init_zwo_mount(indigo_device *device) {
	MOUNT_MODE_PROPERTY->hidden = false;
	MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
	MOUNT_UTC_TIME_PROPERTY->hidden = false;
	MOUNT_PARK_PROPERTY->hidden = true;
	MOUNT_HOME_PROPERTY->hidden = false;
	MOUNT_HOME_PROPERTY->count = 2;
	MOUNT_HOME_PROPERTY->rule = INDIGO_ONE_OF_MANY_RULE;
	MOUNT_SIDE_OF_PIER_PROPERTY->hidden = false;
	ZWO_BUZZER_PROPERTY->hidden = false;
	strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "ZWO");
	PRIVATE_DATA->zwo_firmware = 0;
	PRIVATE_DATA->zwo_tracking_error = 0;
	if (meade_command(device, ":GV#")) {
		strcpy(MOUNT_INFO_MODEL_ITEM->text.value, PRIVATE_DATA->product);
		strcpy(MOUNT_INFO_FIRMWARE_ITEM->text.value, PRIVATE_DATA->response);
		int major = 0, minor = 0, patch = 0;
		if (sscanf(PRIVATE_DATA->response, "%d.%d.%d", &major, &minor, &patch) == 3) {
			PRIVATE_DATA->zwo_firmware = (major << 16) | (minor << 8) | patch;
		}
	}
	// Firmware 1.2.4 and later keeps the meridian behaviour in :GTa# (flip at the limit, track
	// past the meridian, the limit in degrees) and clears the multi-star calibration with :NSC#.
	if (PRIVATE_DATA->zwo_firmware >= 0x010204) {
		bool flip, track;
		int limit;
		if (zwo_get_meridian(device, &flip, &track, &limit)) {
			ZWO_MERIDIAN_AUTO_FLIP_ITEM->sw.value = flip;
			ZWO_MERIDIAN_TRACK_PASSED_ITEM->sw.value = track;
			ZWO_MERIDIAN_LIMIT_ITEM->number.value = ZWO_MERIDIAN_LIMIT_ITEM->number.target = limit;
			ZWO_MERIDIAN_PROPERTY->hidden = ZWO_MERIDIAN_LIMIT_PROPERTY->hidden = false;
		}
		MOUNT_ALIGNMENT_RESET_PROPERTY->hidden = false;
	}
	// The highest slew speed, 720 or 1440 times sidereal.
	if (meade_command(device, ":GRl#")) {
		int speed = atoi(PRIVATE_DATA->response);
		if (speed == 720 || speed == 1440) {
			indigo_set_switch(ZWO_MAX_SLEW_SPEED_PROPERTY, speed == 720 ? ZWO_MAX_SLEW_SPEED_LOW_ITEM : ZWO_MAX_SLEW_SPEED_HIGH_ITEM, true);
			ZWO_MAX_SLEW_SPEED_PROPERTY->hidden = false;
		}
	}
	MOUNT_GUIDE_RATE_DEC_ITEM->number.min = MOUNT_GUIDE_RATE_RA_ITEM->number.min = 10;
	MOUNT_GUIDE_RATE_DEC_ITEM->number.max = MOUNT_GUIDE_RATE_RA_ITEM->number.max = 90;
	int ra_rate, dec_rate;
	if (meade_get_guide_rate(device, &ra_rate, &dec_rate)) {
		MOUNT_GUIDE_RATE_RA_ITEM->number.target = MOUNT_GUIDE_RATE_RA_ITEM->number.value = (double)ra_rate;
		MOUNT_GUIDE_RATE_DEC_ITEM->number.target = MOUNT_GUIDE_RATE_DEC_ITEM->number.value = (double)dec_rate;
	}
	if (meade_command(device, ":GU#")) {
		if (strchr(PRIVATE_DATA->response, 'G')) {
			indigo_set_switch(MOUNT_MODE_PROPERTY, EQUATORIAL_ITEM, true);
		} else if (strchr(PRIVATE_DATA->response, 'Z')) {
			indigo_set_switch(MOUNT_MODE_PROPERTY, ALTAZ_MODE_ITEM, true);
		}
	}
	if (meade_command(device, ":GBu#")) {
		if (strchr(PRIVATE_DATA->response, '0')) {
			indigo_set_switch(ZWO_BUZZER_PROPERTY, ZWO_BUZZER_OFF_ITEM, true);
		} else if (strchr(PRIVATE_DATA->response, '1')) {
			indigo_set_switch(ZWO_BUZZER_PROPERTY, ZWO_BUZZER_LOW_ITEM, true);
		} else if (strchr(PRIVATE_DATA->response, '2')) {
			indigo_set_switch(ZWO_BUZZER_PROPERTY, ZWO_BUZZER_HIGH_ITEM, true);
		}
	}
}

static void meade_update_zwo_state(indigo_device *device) {
	// The tracking status carries the reason tracking stopped, for example at the meridian.
	if (PRIVATE_DATA->zwo_firmware >= 0x010101 && meade_command(device, ":GAT#")) {
		int code = *PRIVATE_DATA->response == 'e' ? atoi(PRIVATE_DATA->response + 1) : 0;
		if (code != PRIVATE_DATA->zwo_tracking_error && code > 0 && *zwo_tracking_error(code)) {
			indigo_send_message(device, ALERT_PROPERTY, "%s", zwo_tracking_error(code));
		} else if (PRIVATE_DATA->zwo_tracking_error == 8 && code == 0) {
			indigo_send_message(device, OK_PROPERTY, "Tracking can be started again");
		}
		PRIVATE_DATA->zwo_tracking_error = code;
	}
	if (meade_command(device, ":GU#")) {
		if (strchr(PRIVATE_DATA->response, 'N') == NULL) {
			PRIVATE_DATA->slewing = true;
			if (MOUNT_PARK_PROPERTY->state == INDIGO_BUSY_STATE) {
				PRIVATE_DATA->parking = true;
			}
			if (MOUNT_HOME_PROPERTY->state == INDIGO_BUSY_STATE) {
				PRIVATE_DATA->homing = true;
			}
		} else {
			if (MOUNT_PARK_PROPERTY->state == INDIGO_BUSY_STATE) {
				PRIVATE_DATA->parked = true;
			}
			if (MOUNT_HOME_PROPERTY->state == INDIGO_BUSY_STATE) {
				PRIVATE_DATA->homed = true;
			}
		}
		if (strchr(PRIVATE_DATA->response, 'n') == NULL) {
			PRIVATE_DATA->tracking = true;
		}
		if (strchr(PRIVATE_DATA->response, 'h')) {
			PRIVATE_DATA->homing = true;
		} else if (strchr(PRIVATE_DATA->response, 'H')) {
			PRIVATE_DATA->homed = true;
		}
	}
	if (meade_command(device, ":Gm#")) {
		if (strchr(PRIVATE_DATA->response, 'N')) {
			MOUNT_SIDE_OF_PIER_WEST_ITEM->sw.value = MOUNT_SIDE_OF_PIER_EAST_ITEM->sw.value = false;
			MOUNT_SIDE_OF_PIER_PROPERTY->state = INDIGO_IDLE_STATE;
		} else 		if (PRIVATE_DATA->response[0] == 'W' && !MOUNT_SIDE_OF_PIER_WEST_ITEM->sw.value) {
			indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, MOUNT_SIDE_OF_PIER_WEST_ITEM, true);
			MOUNT_SIDE_OF_PIER_PROPERTY->state = INDIGO_OK_STATE;
		} else if (PRIVATE_DATA->response[0] == 'E' && !MOUNT_SIDE_OF_PIER_EAST_ITEM->sw.value) {
			indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, MOUNT_SIDE_OF_PIER_EAST_ITEM, true);
			MOUNT_SIDE_OF_PIER_PROPERTY->state = INDIGO_OK_STATE;
		}
	}
}

static void meade_init_nyx_mount(indigo_device *device) {
	MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
	MOUNT_UTC_TIME_PROPERTY->hidden = false;
	MOUNT_PARK_SET_PROPERTY->hidden = false;
	MOUNT_PARK_SET_PROPERTY->count = 1;
	MOUNT_HOME_PROPERTY->hidden = false;
	MOUNT_HOME_PROPERTY->count = 2;
	MOUNT_HOME_PROPERTY->rule = INDIGO_ONE_OF_MANY_RULE;
	MOUNT_HOME_SET_PROPERTY->hidden = false;
	MOUNT_HOME_SET_PROPERTY->count = 1;
	MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
	MOUNT_TRACK_RATE_PROPERTY->count = 4;
	MOUNT_SIDE_OF_PIER_PROPERTY->hidden = false;
	MOUNT_PEC_PROPERTY->hidden = true;
	NYX_WIFI_AP_PROPERTY->hidden = false;
	NYX_WIFI_CL_PROPERTY->hidden = false;
	NYX_WIFI_RESET_PROPERTY->hidden = false;
	NYX_LEVELER_PROPERTY->hidden = false;
	strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "PegasusAstro");
	if (meade_command(device, ":GVN#")) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Firmware: %s", PRIVATE_DATA->response);
		INDIGO_COPY_VALUE(MOUNT_INFO_FIRMWARE_ITEM->text.value, PRIVATE_DATA->response);
	}
	if (meade_command(device, ":GVP#")) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Model: %s", PRIVATE_DATA->response);
		INDIGO_COPY_VALUE(MOUNT_INFO_MODEL_ITEM->text.value, PRIVATE_DATA->response);
	}
	if (!meade_simple_reply_command(device, ":SXEM,1#") || *PRIVATE_DATA->response != '1') {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Can't set EQ mode");
	}
	if (!meade_simple_reply_command(device, ":SX91,U#") || *PRIVATE_DATA->response != '1') {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Can't unlock brake");
	}
	char *separator = NULL;
	*NYX_WIFI_AP_SSID_ITEM->text.value = 0;
	*NYX_WIFI_AP_PASSWORD_ITEM->text.value = 0;
	if (meade_command(device, ":WL>#") && (separator = strchr(PRIVATE_DATA->response, ':'))) {
		*separator++ = 0;
		strncpy(NYX_WIFI_AP_SSID_ITEM->text.value, PRIVATE_DATA->response, INDIGO_VALUE_SIZE);
		strncpy(NYX_WIFI_AP_PASSWORD_ITEM->text.value, separator, INDIGO_VALUE_SIZE);
	}
	*NYX_WIFI_CL_SSID_ITEM->text.value = 0;
	*NYX_WIFI_CL_PASSWORD_ITEM->text.value = 0;
	if (meade_command(device, ":WLD#") && *PRIVATE_DATA->response != ':' && (separator = strchr(PRIVATE_DATA->response, ':'))) {
		*separator++ = 0;
		strncpy(NYX_WIFI_CL_SSID_ITEM->text.value, PRIVATE_DATA->response, INDIGO_VALUE_SIZE);
		indigo_send_message(device, OK_PROPERTY, "Mount is connected to network '%s' with IP address %s", PRIVATE_DATA->response, separator);
	}
	if (meade_command(device, ":GX9D#") && (separator = strchr(PRIVATE_DATA->response, ':'))) {
		*separator++ = 0;
		NYX_LEVELER_PITCH_ITEM->number.value = atof(PRIVATE_DATA->response);
		NYX_LEVELER_ROLL_ITEM->number.value = atof(separator);
	}
	if (meade_command(device, ":GX9E#")) {
		NYX_LEVELER_COMPASS_ITEM->number.value = atof(PRIVATE_DATA->response);
	}
	meade_no_reply_command(device, ":RE00.03#:RA00.03#");
}

static void meade_update_nyx_state(indigo_device *device) {
	if (meade_command(device, ":GU#")) {
		// 'N' is "no goto" and 'n' is "not tracking". They are independent: a goto with
		// tracking enabled carries neither, so reading them as alternatives publishes
		// tracking off for the whole slew.
		if (strchr(PRIVATE_DATA->response, 'N') == NULL) {
			PRIVATE_DATA->slewing = true;
		}
		if (strchr(PRIVATE_DATA->response, 'n') == NULL) {
			PRIVATE_DATA->tracking = true;
		}
		// The tracking rate the controller is really using: ( = lunar, O = solar, k = king,
		// nothing = sidereal. Pier side none is the lowercase 'o', so it cannot be confused
		// with the solar rate.
		if (MOUNT_TRACK_RATE_PROPERTY->state != INDIGO_BUSY_STATE) {
			indigo_item *rate_item;
			if (strchr(PRIVATE_DATA->response, '(')) {
				rate_item = MOUNT_TRACK_RATE_LUNAR_ITEM;
			} else if (strchr(PRIVATE_DATA->response, 'O')) {
				rate_item = MOUNT_TRACK_RATE_SOLAR_ITEM;
			} else if (strchr(PRIVATE_DATA->response, 'k')) {
				rate_item = MOUNT_TRACK_RATE_KING_ITEM;
			} else {
				rate_item = MOUNT_TRACK_RATE_SIDEREAL_ITEM;
			}
			if (!rate_item->sw.value) {
				indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, rate_item, true);
				indigo_update_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
			}
		}
		if (strchr(PRIVATE_DATA->response, 'I')) {
			PRIVATE_DATA->parking = true;
		} else if (strchr(PRIVATE_DATA->response, 'P')) {
			PRIVATE_DATA->parked = true;
		}
		if (strchr(PRIVATE_DATA->response, 'h')) {
			PRIVATE_DATA->homing = true;
		} else if (strchr(PRIVATE_DATA->response, 'H')) {
			PRIVATE_DATA->homed = true;
		}
		if (strchr(PRIVATE_DATA->response, 'o')) {
			MOUNT_SIDE_OF_PIER_WEST_ITEM->sw.value = MOUNT_SIDE_OF_PIER_EAST_ITEM->sw.value = false;
			MOUNT_SIDE_OF_PIER_PROPERTY->state = INDIGO_IDLE_STATE;
		} else if (strchr(PRIVATE_DATA->response, 'W') && !MOUNT_SIDE_OF_PIER_WEST_ITEM->sw.value) {
			indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, MOUNT_SIDE_OF_PIER_WEST_ITEM, true);
			MOUNT_SIDE_OF_PIER_PROPERTY->state = INDIGO_OK_STATE;
		} else if (strchr(PRIVATE_DATA->response, 'T') && !MOUNT_SIDE_OF_PIER_EAST_ITEM->sw.value) {
			indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, MOUNT_SIDE_OF_PIER_EAST_ITEM, true);
			MOUNT_SIDE_OF_PIER_PROPERTY->state = INDIGO_OK_STATE;
		}
	}
	if (PRIVATE_DATA->parked) {
		char *colon;
		if (meade_command(device, ":GX9D#") && (colon = strchr(PRIVATE_DATA->response, ':'))) {
			*colon++ = 0;
			NYX_LEVELER_PITCH_ITEM->number.value = atof(PRIVATE_DATA->response);
			NYX_LEVELER_ROLL_ITEM->number.value = atof(colon);
		}
		if (meade_command(device, ":GX9E#")) {
			NYX_LEVELER_COMPASS_ITEM->number.value = atof(PRIVATE_DATA->response);
		}
	}
}

static void meade_init_oat_mount(indigo_device *device) {
	MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
	MOUNT_UTC_TIME_PROPERTY->hidden = false;
	// The controller implements both :hP# and :hU#, so park is a two-state switch. With the
	// single parked item it used to publish, the unpark branch of the change handler could
	// never be reached and a parked OpenAstroTracker could not be released at all.
	MOUNT_PARK_PROPERTY->count = 2;
	MOUNT_PARK_PROPERTY->rule = INDIGO_ONE_OF_MANY_RULE;
	MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
	strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "OpenAstroTech");
	// There is no model query, so the model is the product name :GVP# already gave the
	// autodetection.
	INDIGO_COPY_VALUE(MOUNT_INFO_MODEL_ITEM->text.value, PRIVATE_DATA->product);
	// A session starts with the mount released. The firmware cannot be asked whether it is
	// parked and reports every idle mount as parked, so the alternative guess would lock a
	// client out of a mount that is only standing still: it could not track, move or slew
	// until it unparked a mount that was never parked. Being wrong the other way costs
	// nothing, because :MS#, :Mn# and :MT1# work whatever the firmware calls its state.
	PRIVATE_DATA->oat_parked = PRIVATE_DATA->oat_park_expected = false;
	if (meade_command(device, ":GVN#")) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Firmware: %s", PRIVATE_DATA->response);
		INDIGO_COPY_VALUE(MOUNT_INFO_FIRMWARE_ITEM->text.value, PRIVATE_DATA->response);
	}
}

// The :GX# status of an OpenAstroTracker has no idle state: everything that is not slewing,
// guiding, parking or tracking is reported as "Parked", so a mount that merely stopped
// tracking claims to be parked wherever it stands. Reading that as the park state locks a
// client out of its own mount - tracking is refused because the mount is parked, and the
// mount is parked because tracking is off - and it was observed on firmware v1.13.20 at
// declination +75, nowhere near the park position. The park state is therefore the one the
// driver established itself with :hP# and :hU#, the way the Astro-Physics branch keeps its
// own, and a "Parked" status only confirms a park that was asked for.
static void meade_update_oat_state(indigo_device *device) {
	if (meade_command(device, ":GX#")) {
		// "SlewToTarget" is a goto; "FreeSlew" and "ManualSlew" are the axis moving under
		// :Mn# and friends, which is not a goto and must not publish one.
		bool moving = strstr(PRIVATE_DATA->response, "Slew") != NULL;
		if (!strncmp(PRIVATE_DATA->response, "Slew", 4)) {
			PRIVATE_DATA->slewing = true;
		} else if (!strncmp(PRIVATE_DATA->response, "Tracking", 8)) {
			PRIVATE_DATA->tracking = true;
		} else if (!strncmp(PRIVATE_DATA->response, "Parking", 7)) {
			PRIVATE_DATA->parking = true;
		} else if (!strncmp(PRIVATE_DATA->response, "Homing", 6)) {
			PRIVATE_DATA->homing = true;
		} else if (!strncmp(PRIVATE_DATA->response, "Parked", 6) && PRIVATE_DATA->oat_park_expected) {
			PRIVATE_DATA->oat_parked = true;
			PRIVATE_DATA->oat_park_expected = false;
		}
		if (moving) {
			// The firmware stops the tracking motor for the duration of every slew and
			// starts it again, with a compensation for the time it stood still, when the
			// slew ends. :GX# therefore reports no tracking while the mount is on its way,
			// although the setting the client made did not change. Publishing that would
			// turn MOUNT_TRACKING off and on again around every goto and every manual move.
			PRIVATE_DATA->tracking = MOUNT_TRACKING_ON_ITEM->sw.value;
		}
		if (PRIVATE_DATA->tracking || moving) {
			PRIVATE_DATA->oat_parked = PRIVATE_DATA->oat_park_expected = false;
		}
		PRIVATE_DATA->parked = PRIVATE_DATA->oat_parked;
	}
	if (MOUNT_HOME_PROPERTY->state == INDIGO_BUSY_STATE && !PRIVATE_DATA->slewing) {
		PRIVATE_DATA->homed = true;
	}
}

static void meade_init_teenastro_mount(indigo_device *device) {
	MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
	MOUNT_UTC_TIME_PROPERTY->hidden = false;
	MOUNT_PARK_SET_PROPERTY->hidden = false;
	MOUNT_PARK_SET_PROPERTY->count = 1;
	MOUNT_HOME_PROPERTY->hidden = false;
	MOUNT_HOME_SET_PROPERTY->hidden = false;
	MOUNT_HOME_SET_PROPERTY->count = 1;
	MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
	MOUNT_TRACK_RATE_PROPERTY->count = 3;
	MOUNT_SIDE_OF_PIER_PROPERTY->hidden = false;
	MOUNT_PEC_PROPERTY->hidden = false;
	strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "TeenAstro");
	if (meade_command(device, ":GVN#")) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Firmware: %s", PRIVATE_DATA->response);
		INDIGO_COPY_VALUE(MOUNT_INFO_FIRMWARE_ITEM->text.value, PRIVATE_DATA->response);
	}
}

static void meade_update_teenastro_state(indigo_device *device) {
	if (meade_command(device, ":GXI#")) {
		if (PRIVATE_DATA->response[0] == '1') {
			PRIVATE_DATA->tracking = true;
		} else if (PRIVATE_DATA->response[0] == '2' || PRIVATE_DATA->response[0] == '3') {
			PRIVATE_DATA->slewing = true;
			if (MOUNT_HOME_PROPERTY->state == INDIGO_BUSY_STATE) {
				PRIVATE_DATA->homing = true;
			}
		}
		if (PRIVATE_DATA->response[2] == 'P') {
			PRIVATE_DATA->parked = true;
		} else if (PRIVATE_DATA->response[2] == 'I') {
			PRIVATE_DATA->parking = true;
		}
		if (PRIVATE_DATA->response[3] == 'H') {
			PRIVATE_DATA->homed = true;
		}
		if (PRIVATE_DATA->response[13] == 'W' && !MOUNT_SIDE_OF_PIER_WEST_ITEM->sw.value) {
			indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, MOUNT_SIDE_OF_PIER_WEST_ITEM, true);
		} else if (PRIVATE_DATA->response[13] == 'E' && !MOUNT_SIDE_OF_PIER_EAST_ITEM->sw.value) {
			indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, MOUNT_SIDE_OF_PIER_EAST_ITEM, true);
		}
	}
}

static void meade_init_esp32go_mount(indigo_device *device) {
	MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
	MOUNT_UTC_TIME_PROPERTY->hidden = false;
	// Nothing in the LX200 grammar of this firmware stops the tracking motor. :AL# sets
	// telescope->track to 0 and the target speed to 0, which the next pass of the tracking
	// loop overwrites, and :AP# only sets track back to 1 without touching the rate the
	// motor runs at. mount_track_off() exists but is reachable from the infrared remote and
	// the hand pad alone. A switch that cannot turn tracking off is worse than none.
	MOUNT_TRACKING_PROPERTY->hidden = true;
	// :hP# goes to the home position and marks the mount parked, and no command releases it
	// again: only a goto or a sync clears the flag, as a side effect. The one command the
	// web interface calls park is :cRR#, which saves the position and restarts the
	// controller. MOUNT_HOME owns that movement instead, and MOUNT_PARK stays hidden so a
	// client cannot park a mount it would have no way to unpark.
	MOUNT_PARK_PROPERTY->hidden = true;
	MOUNT_HOME_PROPERTY->hidden = false;
	MOUNT_HOME_SET_PROPERTY->hidden = false;
	MOUNT_HOME_SET_PROPERTY->count = 1;
	// :TQ#, :TS#, :TL# and :TK# all reach set_track_speed(), so all four rates are real.
	MOUNT_TRACK_RATE_PROPERTY->count = 4;
	MOUNT_SIDE_OF_PIER_PROPERTY->hidden = false;
	// The guide rate is a configuration value of the controller with no LX200 command to
	// write it, so the property would accept a value the mount never sees.
	MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
	strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "ESP32Go");
	// There is no model query, so the model is the product name :GVP# already gave the
	// autodetection.
	INDIGO_COPY_VALUE(MOUNT_INFO_MODEL_ITEM->text.value, PRIVATE_DATA->product);
	if (meade_command(device, ":GVN#")) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Firmware: %s", PRIVATE_DATA->response);
		INDIGO_COPY_VALUE(MOUNT_INFO_FIRMWARE_ITEM->text.value, PRIVATE_DATA->response);
	}
}

// :GU# answers %c%c%c%c%d of tracking, parked, slewing, pier side and the tracking rate
// index, so one transaction carries the whole state the polling callback needs.
//
// The second character is the flag mount_goto_home() sets when it starts the slew :hP#
// asks for, and that any goto or sync clears again. It is therefore the mount standing on
// its home position rather than a park state a client could act on, and it is published
// through MOUNT_HOME: the home slew has to keep MOUNT_HOME busy while it runs, so "at home"
// is the flag together with an axis that has stopped.
static void meade_update_esp32go_state(indigo_device *device) {
	if (meade_command(device, ":GU#") && strlen(PRIVATE_DATA->response) >= 5) {
		PRIVATE_DATA->tracking = PRIVATE_DATA->response[0] == 'T';
		PRIVATE_DATA->slewing = PRIVATE_DATA->response[2] == 'S';
		PRIVATE_DATA->homed = PRIVATE_DATA->response[1] == 'P' && !PRIVATE_DATA->slewing;
		PRIVATE_DATA->homing = PRIVATE_DATA->response[1] == 'P' && PRIVATE_DATA->slewing;
		if (PRIVATE_DATA->response[3] == 'W' && !MOUNT_SIDE_OF_PIER_WEST_ITEM->sw.value) {
			indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, MOUNT_SIDE_OF_PIER_WEST_ITEM, true);
		} else if (PRIVATE_DATA->response[3] == 'E' && !MOUNT_SIDE_OF_PIER_EAST_ITEM->sw.value) {
			indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, MOUNT_SIDE_OF_PIER_EAST_ITEM, true);
		}
	}
}

static void meade_init_generic_mount(indigo_device *device) {
	MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
	MOUNT_UTC_TIME_PROPERTY->hidden = false;
	MOUNT_TRACKING_PROPERTY->hidden = true;
	MOUNT_PARK_PROPERTY->hidden = true;
	MOUNT_INFO_PROPERTY->count = 1;
	strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "Generic");
}

static void meade_update_generic_state(indigo_device *device) {
	// After Track or Slew
	// NOTE: Distance bar `:D#` is not working (e.g. classic LX200).
	if (fabs(MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value - PRIVATE_DATA->lastRA) > 2.0/60.0 || fabs(MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value - PRIVATE_DATA->lastDec) > 2.0/60.0) {
		PRIVATE_DATA->slewing = true;
	}
}

// ---------------------------------------------------------------------  generic init & state update

static void meade_init_mount(indigo_device *device) {
	MOUNT_MODE_PROPERTY->hidden = true;
	MOUNT_SET_HOST_TIME_PROPERTY->hidden = true;
	MOUNT_UTC_TIME_PROPERTY->hidden = true;
	MOUNT_TRACKING_PROPERTY->hidden = false;
	MOUNT_PARK_PROPERTY->hidden = false;
	MOUNT_PARK_PROPERTY->count = 2;
	MOUNT_PARK_PROPERTY->rule = INDIGO_ONE_OF_MANY_RULE;
	MOUNT_PARK_PARKED_ITEM->sw.value = true;
	MOUNT_PARK_UNPARKED_ITEM->sw.value = false;
	MOUNT_PARK_SET_PROPERTY->hidden = true;
	MOUNT_HOME_PROPERTY->hidden = true;
	MOUNT_HOME_PROPERTY->count = 1;
	MOUNT_HOME_PROPERTY->rule = INDIGO_AT_MOST_ONE_RULE;
	MOUNT_HOME_SET_PROPERTY->hidden = true;
	MOUNT_HOME_SET_PROPERTY->count = 2;
	MOUNT_GUIDE_RATE_PROPERTY->hidden = false;
	MOUNT_TRACK_RATE_PROPERTY->hidden = false;
	MOUNT_TRACK_RATE_PROPERTY->count = 3;
	MOUNT_SLEW_RATE_PROPERTY->hidden = false;
	MOUNT_SIDE_OF_PIER_PROPERTY->hidden = true;
	MOUNT_PEC_PROPERTY->hidden = true;
	MOUNT_MOTION_RA_PROPERTY->hidden = false;
	MOUNT_MOTION_DEC_PROPERTY->hidden = false;
	MOUNT_INFO_PROPERTY->count = 3;
	strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "Unknown");
	strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "Unknown");
	strcpy(MOUNT_INFO_FIRMWARE_ITEM->text.value, "Unknown");
	ZWO_BUZZER_PROPERTY->hidden = true;
	NYX_WIFI_AP_PROPERTY->hidden = true;
	NYX_WIFI_CL_PROPERTY->hidden = true;
	NYX_WIFI_RESET_PROPERTY->hidden = true;
	NYX_LEVELER_PROPERTY->hidden = true;
	ONSTEP_PREFERRED_PIER_SIDE_PROPERTY->hidden = true;
	ONSTEP_AUTO_MERIDIAN_FLIP_PROPERTY->hidden = true;
	ONSTEP_MERIDIAN_LIMITS_PROPERTY->hidden = true;
	ONSTEP_ALTITUDE_LIMITS_PROPERTY->hidden = true;
	AP_SYNC_MODE_PROPERTY->hidden = true;
	AP_PARK_POSITION_PROPERTY->hidden = true;
	PRIVATE_DATA->use_dst_commands = false;
	PRIVATE_DATA->slewing = PRIVATE_DATA->tracking = PRIVATE_DATA->parking = PRIVATE_DATA->parked = PRIVATE_DATA->homing = PRIVATE_DATA->homed = false;
	PRIVATE_DATA->goto_issued = false;
	PRIVATE_DATA->meade_park_silent = PRIVATE_DATA->meade_no_gw = PRIVATE_DATA->meade_lxd600 = false;
	// A park or a stall from the previous session says nothing about this one.
	PRIVATE_DATA->gemini_park_expected = PRIVATE_DATA->gemini_park_failed = PRIVATE_DATA->stalled = false;
	PRIVATE_DATA->gemini_velocity = 0;
	PRIVATE_DATA->gemini_remaining_ns = PRIVATE_DATA->gemini_remaining_we = 0;
	GEMINI_PARK_POSITION_PROPERTY->hidden = true;
	ZWO_MERIDIAN_PROPERTY->hidden = ZWO_MERIDIAN_LIMIT_PROPERTY->hidden = ZWO_MAX_SLEW_SPEED_PROPERTY->hidden = true;
	// Only the ZWO AM clears its own calibration, otherwise the reset is about the host side points.
	MOUNT_ALIGNMENT_RESET_PROPERTY->hidden = MOUNT_ALIGNMENT_MODE_CONTROLLER_ITEM->sw.value;
	if (MOUNT_TYPE_MEADE_ITEM->sw.value) {
		meade_init_meade_mount(device);
		meade_update_meade_state(device);
	} else if (MOUNT_TYPE_10MICRONS_ITEM->sw.value) {
		meade_init_10microns_mount(device);
		meade_update_10microns_state(device);
	} else if (MOUNT_TYPE_GEMINI_ITEM->sw.value) {
		meade_init_gemini_mount(device);
		meade_update_gemini_state(device);
	} else if (MOUNT_TYPE_STARGO_ITEM->sw.value) {
		meade_init_stargo_mount(device);
		meade_update_stargo_state(device);
	} else if (MOUNT_TYPE_STARGO2_ITEM->sw.value) {
		meade_init_stargo2_mount(device);
	} else if (MOUNT_TYPE_AP_ITEM->sw.value) {
		meade_init_ap_mount(device);
	} else if (MOUNT_TYPE_ON_STEP_ITEM->sw.value) {
		meade_init_onstep_mount(device);
		meade_update_onstep_state(device);
	} else if (MOUNT_TYPE_AGOTINO_ITEM->sw.value) {
		meade_init_agotino_mount(device);
		meade_update_agotino_state(device);
	} else if (MOUNT_TYPE_ZWO_ITEM->sw.value) {
		meade_init_zwo_mount(device);
		meade_update_zwo_state(device);
	} else if (MOUNT_TYPE_NYX_ITEM->sw.value) {
		meade_init_nyx_mount(device);
		meade_update_nyx_state(device);
	} else if (MOUNT_TYPE_OAT_ITEM->sw.value) {
		meade_init_oat_mount(device);
		meade_update_oat_state(device);
	} else if (MOUNT_TYPE_TEEN_ASTRO_ITEM->sw.value) {
		meade_init_teenastro_mount(device);
		meade_update_teenastro_state(device);
	} else if (MOUNT_TYPE_ESP32GO_ITEM->sw.value) {
		meade_init_esp32go_mount(device);
		meade_update_esp32go_state(device);
	} else {
		meade_init_generic_mount(device);
		if (MOUNT_TYPE_CLASSIC_ITEM->sw.value) {
			MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
			MOUNT_INFO_PROPERTY->count = 2;
			strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "Meade");
			strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "LX200 Classic");
		}
		meade_update_generic_state(device);
	}
	if (!meade_get_tracking_rate(device) && MOUNT_TYPE_CLASSIC_ITEM->sw.value) {
		MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	if (PRIVATE_DATA->parking) {
		indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_PARKED_ITEM, true);
		MOUNT_PARK_PROPERTY->state = MOUNT_STATE_PARK_ITEM->light.value = INDIGO_BUSY_STATE;
	} else if (PRIVATE_DATA->parked) {
		indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_PARKED_ITEM, true);
		MOUNT_PARK_PROPERTY->state = MOUNT_STATE_PARK_ITEM->light.value = INDIGO_OK_STATE;
	} else {
		indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
		MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
		MOUNT_STATE_PARK_ITEM->light.value = INDIGO_IDLE_STATE;
	}
	if (PRIVATE_DATA->homing) {
		indigo_set_switch(MOUNT_HOME_PROPERTY, MOUNT_HOME_ITEM, true);
		MOUNT_HOME_PROPERTY->state = MOUNT_STATE_HOME_ITEM->light.value = INDIGO_BUSY_STATE;
	} else if (PRIVATE_DATA->homed) {
		indigo_set_switch(MOUNT_HOME_PROPERTY, MOUNT_HOME_ITEM, true);
		MOUNT_HOME_PROPERTY->state = MOUNT_STATE_HOME_ITEM->light.value = INDIGO_OK_STATE;
	} else {
		if (MOUNT_HOME_PROPERTY->count == 2) {
			indigo_set_switch(MOUNT_HOME_PROPERTY, MOUNT_AWAY_ITEM, true);
		} else {
			indigo_set_switch(MOUNT_HOME_PROPERTY, MOUNT_HOME_ITEM, false);
		}
		MOUNT_HOME_PROPERTY->state = INDIGO_OK_STATE;
		MOUNT_STATE_HOME_ITEM->light.value = INDIGO_IDLE_STATE;
	}
	if (PRIVATE_DATA->tracking) {
		indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
		MOUNT_STATE_TRACKING_ITEM->light.value = INDIGO_OK_STATE;
	} else {
		indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
		MOUNT_STATE_TRACKING_ITEM->light.value = INDIGO_IDLE_STATE;
	}
	time_t secs = 0;
	bool clock_read = meade_get_utc(device, &secs, &PRIVATE_DATA->utc_offset);
	time_t now = time(NULL);
	// A Gemini whose clock could not be read is left alone: the clock and the site it has are
	// not replaced because of a reply this driver failed to parse.
	if ((clock_read || !MOUNT_TYPE_GEMINI_ITEM->sw.value) && labs(secs - now) > 24 * 60 * 60) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Mount is not initialized, initializing...");
		meade_set_utc(device, now, indigo_get_utc_offset());
		// The clock is what this check is about. The site is written with it only when the
		// driver has one of its own, which on a first connect it has not: the geographic
		// property then still holds the 0, 0 a profile that was never configured starts
		// from, and writing that replaces the site the mount knows with nothing. An
		// OpenAstroTracker that came up with its 2021 clock lost its configured latitude
		// exactly that way.
		if (MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value != 0 || MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value != 0) {
			meade_set_site(device, MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value, MOUNT_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM->number.value);
		}
	}
	{
		// Whatever the branch above did, the published site is the one the mount reports,
		// so a write is read back and a controller the driver did not write to is not
		// published with a site it does not have.
		double latitude = 0, longitude = 0;
		if (meade_get_site(device, &latitude, &longitude)) {
			MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.target = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = latitude;
			MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.target = MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = longitude;
		}
	}
}

static void meade_update_mount_state(indigo_device *device) {
	double ra = 0, dec = 0;
	if (PRIVATE_DATA->meade_park_silent) {
		// A parked Meade that stopped answering is not asked anything, the last position read stands.
		PRIVATE_DATA->coordinate_read_failed = false;
	} else if (meade_get_coordinates(device, &ra, &dec)) {
		indigo_eq_to_j2k(MOUNT_EPOCH_ITEM->number.value, &ra, &dec);
		MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = ra;
		MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = dec;
		if (PRIVATE_DATA->coordinate_read_failed) {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		}
		PRIVATE_DATA->coordinate_read_failed = false;
	} else {
		PRIVATE_DATA->coordinate_read_failed = true;
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	bool ap_parked = MOUNT_TYPE_AP_ITEM->sw.value && PRIVATE_DATA->parked;
	PRIVATE_DATA->slewing = PRIVATE_DATA->tracking = PRIVATE_DATA->parking = PRIVATE_DATA->parked = PRIVATE_DATA->homing = PRIVATE_DATA->homed = false;
	PRIVATE_DATA->parked = ap_parked;
	if (MOUNT_TYPE_MEADE_ITEM->sw.value) {
		meade_update_meade_state(device);
	} else if (MOUNT_TYPE_10MICRONS_ITEM->sw.value) {
		meade_update_10microns_state(device);
	} else if (MOUNT_TYPE_GEMINI_ITEM->sw.value) {
		meade_update_gemini_state(device);
	} else if (MOUNT_TYPE_STARGO_ITEM->sw.value) {
		meade_update_stargo_state(device);
	} else if (MOUNT_TYPE_ON_STEP_ITEM->sw.value) {
		meade_update_onstep_state(device);
	} else if (MOUNT_TYPE_AGOTINO_ITEM->sw.value) {
		meade_update_agotino_state(device);
	} else if (MOUNT_TYPE_ZWO_ITEM->sw.value) {
		meade_update_zwo_state(device);
	} else if (MOUNT_TYPE_NYX_ITEM->sw.value) {
		meade_update_nyx_state(device);
		if (PRIVATE_DATA->parked) {
			indigo_update_property(device, NYX_LEVELER_PROPERTY, NULL);
		}
	} else if (MOUNT_TYPE_OAT_ITEM->sw.value) {
		meade_update_oat_state(device);
	} else if (MOUNT_TYPE_TEEN_ASTRO_ITEM->sw.value) {
		meade_update_teenastro_state(device);
	} else if (MOUNT_TYPE_ESP32GO_ITEM->sw.value) {
		meade_update_esp32go_state(device);
	} else if (MOUNT_TYPE_AP_ITEM->sw.value) {
		meade_update_ap_state(device);
	} else {
		meade_update_generic_state(device);
	}
	if (meade_host_timed_guiding(device) && !PRIVATE_DATA->slewing && !PRIVATE_DATA->coordinate_read_failed) {
		PRIVATE_DATA->classicGoto = false;
	}
	PRIVATE_DATA->lastRA = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value;
	PRIVATE_DATA->lastDec = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value;
	indigo_debug("*** slewing=%d, tracking=%d, parked=%d, parking=%d, homed=%d, homing=%d", PRIVATE_DATA->slewing, PRIVATE_DATA->tracking, PRIVATE_DATA->parked, PRIVATE_DATA->parking, PRIVATE_DATA->homed, PRIVATE_DATA->homing);
	if (PRIVATE_DATA->coordinate_read_failed) {
		MOUNT_STATE_SLEW_ITEM->light.value = INDIGO_ALERT_STATE;
	} else if (PRIVATE_DATA->stalled) {
		// The position is readable and the axis is not moving, which is what the branch
		// below reads as an arrival. A stalled mount has not arrived anywhere.
		MOUNT_STATE_SLEW_ITEM->light.value = INDIGO_ALERT_STATE;
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		PRIVATE_DATA->goto_issued = false;
	} else if (PRIVATE_DATA->slewing) {
		// a running slew (driver or hand controller initiated) completes when it stops
		PRIVATE_DATA->goto_issued = true;
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = MOUNT_STATE_SLEW_ITEM->light.value = INDIGO_BUSY_STATE;
	} else if (MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE && !PRIVATE_DATA->goto_issued) {
		// a GOTO was accepted but its slew command has not been sent yet
	} else {
		// The coordinates were read, so the position is valid again whatever left the
		// property in ALERT. Only a failed readback keeps it there, through the branch
		// above; a refused GOTO must not poison the property for the rest of the session.
		PRIVATE_DATA->goto_issued = false;
		MOUNT_STATE_SLEW_ITEM->light.value = INDIGO_IDLE_STATE;
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
	}
	if (MOUNT_TRACKING_PROPERTY->state != INDIGO_BUSY_STATE) { // to avoid race never change tracking state if BUSY
		if (PRIVATE_DATA->tracking && !MOUNT_TRACKING_ON_ITEM->sw.value) {
			indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
			MOUNT_TRACKING_PROPERTY->state = MOUNT_STATE_TRACKING_ITEM->light.value = INDIGO_OK_STATE;
		} else if (!PRIVATE_DATA->tracking && !MOUNT_TRACKING_OFF_ITEM->sw.value) {
			indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
			MOUNT_STATE_TRACKING_ITEM->light.value = INDIGO_IDLE_STATE;
			MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
		}
	}
	if (MOUNT_PARK_PROPERTY->state == INDIGO_BUSY_STATE) { // to avoid race never change parking state if BUSY with these exceptions
		if ((MOUNT_PARK_PROPERTY->count == 1 || MOUNT_PARK_PARKED_ITEM->sw.value) && PRIVATE_DATA->parked) {
			MOUNT_PARK_PROPERTY->state = MOUNT_STATE_PARK_ITEM->light.value = INDIGO_OK_STATE;
		} else if (MOUNT_PARK_PROPERTY->count == 2 && MOUNT_PARK_UNPARKED_ITEM->sw.value && !PRIVATE_DATA->parked) {
			MOUNT_STATE_PARK_ITEM->light.value = INDIGO_IDLE_STATE;
			MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
		}
	} else { // otherwise mirror state reported by mount
		if (PRIVATE_DATA->parking) {
			MOUNT_STATE_PARK_ITEM->light.value = INDIGO_BUSY_STATE;
		} else if (PRIVATE_DATA->parked) {
			indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_PARKED_ITEM, true);
			MOUNT_STATE_PARK_ITEM->light.value = INDIGO_OK_STATE;
		} else {
			indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
			MOUNT_STATE_PARK_ITEM->light.value = INDIGO_IDLE_STATE;
		}
	}
	if (MOUNT_HOME_PROPERTY->state == INDIGO_BUSY_STATE) { // to avoid race never change home state if BUSY with this exception
		if (PRIVATE_DATA->homed) {
			// A momentary single item MOUNT_HOME was cleared when the request was accepted,
			// so the completion has to put it back. Without this the driver publishes the
			// home slew as finished while the property itself still says the mount is away,
			// and only the next polling cycle corrects it. A two item MOUNT_HOME already
			// holds the item the request set, so this changes nothing for one.
			indigo_set_switch(MOUNT_HOME_PROPERTY, MOUNT_HOME_ITEM, true);
			MOUNT_HOME_PROPERTY->state = MOUNT_STATE_HOME_ITEM->light.value = INDIGO_OK_STATE;
		}
	} else { // otherwise mirror state reported by mount
		if (MOUNT_HOME_ITEM->sw.value && !PRIVATE_DATA->homed) {
			if (MOUNT_HOME_PROPERTY->count == 2) {
				indigo_set_switch(MOUNT_HOME_PROPERTY, MOUNT_AWAY_ITEM, true);
			} else {
				indigo_set_switch(MOUNT_HOME_PROPERTY, MOUNT_HOME_ITEM, false);
			}
			MOUNT_STATE_HOME_ITEM->light.value = INDIGO_IDLE_STATE;
		} else if (!MOUNT_HOME_ITEM->sw.value && PRIVATE_DATA->homed) {
			indigo_set_switch(MOUNT_HOME_PROPERTY, MOUNT_HOME_ITEM, true);
			MOUNT_STATE_HOME_ITEM->light.value = INDIGO_OK_STATE;
		} else if (PRIVATE_DATA->homing) {
			MOUNT_STATE_HOME_ITEM->light.value = INDIGO_BUSY_STATE;
		}
	}
	if (MOUNT_UTC_TIME_PROPERTY->state != INDIGO_BUSY_STATE) { // to avoid race never overwrite the requested time while BUSY
		sprintf(MOUNT_UTC_OFFSET_ITEM->text.value, "%d", PRIVATE_DATA->utc_offset);
		indigo_timetoisogm(time(NULL) - PRIVATE_DATA->time_difference, MOUNT_UTC_ITEM->text.value, INDIGO_VALUE_SIZE);
		MOUNT_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
	indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
	indigo_update_property(device, MOUNT_HOME_PROPERTY, NULL);
	indigo_update_property(device, MOUNT_SIDE_OF_PIER_PROPERTY, NULL);
	indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
	indigo_update_property(device, MOUNT_STATE_PROPERTY, NULL);
	indigo_update_coordinates(device, NULL);
}

//- code

//+ guider.code

static void guider_guide_dec_finalizer(indigo_device *device) {
	indigo_cancel_pending_handler(device, gemini_guide_dec_continue);
	PRIVATE_DATA->gemini_remaining_ns = 0;
	bool stopped = !meade_host_timed_guiding(device) || meade_classic_guide_stop(device, &PRIVATE_DATA->classicGuideNS);
	// Only the values, the target of a pulse requested while this one ends is read by its handler.
	GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
	INDIGO_UPDATE_PROPERTY_STATE(GUIDER_GUIDE_DEC_PROPERTY, stopped ? INDIGO_OK_STATE : INDIGO_ALERT_STATE, NULL);
}

static void guider_guide_ra_finalizer(indigo_device *device) {
	indigo_cancel_pending_handler(device, gemini_guide_ra_continue);
	PRIVATE_DATA->gemini_remaining_we = 0;
	bool stopped = !meade_host_timed_guiding(device) || meade_classic_guide_stop(device, &PRIVATE_DATA->classicGuideWE);
	// Only the values, the target of a pulse requested while this one ends is read by its handler.
	GUIDER_GUIDE_WEST_ITEM->number.value = GUIDER_GUIDE_EAST_ITEM->number.value = 0;
	INDIGO_UPDATE_PROPERTY_STATE(GUIDER_GUIDE_RA_PROPERTY, stopped ? INDIGO_OK_STATE : INDIGO_ALERT_STATE, NULL);
}

//- guider.code

//+ aux.code

static void nyx_aux_update(indigo_device *device) {
	bool updateWeather = false;
	bool updateInfo = false;
	if (meade_command(device, ":GX9A#")) {
		double temperature = atof(PRIVATE_DATA->response);
		if (AUX_WEATHER_TEMPERATURE_ITEM->number.value != temperature) {
			AUX_WEATHER_TEMPERATURE_ITEM->number.value = temperature;
			updateWeather = true;
		}
	}
	if (meade_command(device, ":GX9B#")) {
		double pressure = atof(PRIVATE_DATA->response);
		if (AUX_WEATHER_PRESSURE_ITEM->number.value != pressure) {
			AUX_WEATHER_PRESSURE_ITEM->number.value = pressure;
			updateWeather = true;
		}
	}
	if (meade_command(device, ":GX9V#")) {
		double voltage = atof(PRIVATE_DATA->response);
		if (AUX_INFO_VOLTAGE_ITEM->number.value != voltage) {
			AUX_INFO_VOLTAGE_ITEM->number.value = voltage;
			updateInfo = true;
		}
	}
	if (updateWeather) {
		INDIGO_UPDATE_PROPERTY_STATE(AUX_WEATHER_PROPERTY, INDIGO_OK_STATE, NULL);
	}
	if (updateInfo) {
		INDIGO_UPDATE_PROPERTY_STATE(AUX_INFO_PROPERTY, INDIGO_OK_STATE, NULL);
	}
}

static void onstep_aux_update(indigo_device *device) {
	if (AUX_HEATER_OUTLET_PROPERTY->state != INDIGO_BUSY_STATE) {
		bool do_update = false;
		for (int i = 0; i < AUX_HEATER_OUTLET_PROPERTY->count; i++) {
			int onstep_slot = ONSTEP_AUX_HEATER_OUTLET_MAPPING[i];
			// responds with a number between 0 for fully off and 255 for fully on
			if (!meade_command(device, ":GXX%d#", onstep_slot)) {
				continue;
			}
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "received PRIVATE_DATA->response %s for slot %d", PRIVATE_DATA->response, onstep_slot);
			indigo_item *item = AUX_HEATER_OUTLET_PROPERTY->items + i;
			// convert to percent
			int new_value = (int)(atoi(PRIVATE_DATA->response) / 2.56 + 0.5);
			if (new_value != (int) item->number.value) {
				item->number.value = new_value;
				do_update = true;
			}
		}
		if (do_update) {
			INDIGO_UPDATE_PROPERTY_STATE(AUX_HEATER_OUTLET_PROPERTY, INDIGO_OK_STATE, NULL);
		}
	}
	if (AUX_POWER_OUTLET_PROPERTY->state != INDIGO_BUSY_STATE) {
		bool do_update = false;
		for (int i = 0; i < AUX_POWER_OUTLET_PROPERTY->count; i++) {
			int onstep_slot = ONSTEP_AUX_POWER_OUTLET_MAPPING[i];
			// the PRIVATE_DATA->response is 0 when disabled and 1 when the switch is enabled
			if (!meade_command(device, ":GXX%d#", onstep_slot) || AUX_POWER_OUTLET_PROPERTY->state == INDIGO_BUSY_STATE) {
				continue;
			}
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "received PRIVATE_DATA->response %s for slot %d", PRIVATE_DATA->response, onstep_slot);
			indigo_item *item = AUX_POWER_OUTLET_PROPERTY->items + i;
			bool active = PRIVATE_DATA->response[0] - '0';
			if (active != item->sw.value) {
				item->sw.value = active;
				do_update = true;
			}
		}
		if (do_update && AUX_POWER_OUTLET_PROPERTY->state != INDIGO_BUSY_STATE) {
			INDIGO_UPDATE_PROPERTY_STATE(AUX_POWER_OUTLET_PROPERTY, INDIGO_OK_STATE, NULL);
		}
	}
}

static bool onstep_aux_discover(indigo_device *device) {
	// A controller without auxiliary features has no outlet of either kind, so the
	// counts are cleared before the first way out of this function.
	AUX_HEATER_OUTLET_PROPERTY->count = 0;
	AUX_POWER_OUTLET_PROPERTY->count = 0;
	// first we request Onstep to list active aux slots
	if (!meade_command(device, ":GXY0#")) {
		return false;
	}
	// Onstep responds with a string like "11000000" to indicate that the first and second aux device is enabled
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Onstep active device string: %s", PRIVATE_DATA->response);
	// A build without auxiliary features answers "0", and anything that is not the
	// documented bitmap would be read past its terminator as a map of stale slots.
	if (strlen(PRIVATE_DATA->response) != ONSTEP_AUX_DEVICE_COUNT || strspn(PRIVATE_DATA->response, "01") != ONSTEP_AUX_DEVICE_COUNT) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Onstep reports no auxiliary feature slots ('%s')", PRIVATE_DATA->response);
		return false;
	}
	char active_slots[ONSTEP_AUX_DEVICE_COUNT];
	memcpy(active_slots, PRIVATE_DATA->response, sizeof(active_slots));
	// in the first pass over the active devices we count how many auxiliary devices of each purpose we have
	for (int i = 0; i < ONSTEP_AUX_DEVICE_COUNT; i++) {
		if (active_slots[i] != '1') {
			continue;
		}
		// Now we get the name and purpose of each active aux device, it is one-indexed
		meade_command(device, ":GXY%d#", i + 1);
		// Onstep responds with a string like "my switch,2" where the part before "," is the name and after the purpose as int
		char *comma = strchr(PRIVATE_DATA->response, ',');
		if (comma == NULL) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Onstep AUX Device at slot %d invalid PRIVATE_DATA->response", i + 1);
			continue;
		}
		*comma++ = '\0';
		char *name = PRIVATE_DATA->response;
		onstep_aux_device_purpose purpose = *comma - '0';
		if (purpose == ONSTEP_AUX_ANALOG) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Onstep AUX Heater Outlet at slot %d with name %s", i + 1, name);
			strcpy(AUX_HEATER_OUTLET_PROPERTY->items[AUX_HEATER_OUTLET_PROPERTY->count].label, name);
			ONSTEP_AUX_HEATER_OUTLET_MAPPING[AUX_HEATER_OUTLET_PROPERTY->count++] = i + 1;
		} else if (purpose == ONSTEP_AUX_SWITCH) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Onstep AUX Power Outlet Device at slot %d with name %s and purpose switch", i + 1, name);
			strcpy(AUX_POWER_OUTLET_PROPERTY->items[AUX_POWER_OUTLET_PROPERTY->count].label, name);
			ONSTEP_AUX_POWER_OUTLET_MAPPING[AUX_POWER_OUTLET_PROPERTY->count++] = i + 1;
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Onstep AUX Device at index %d not recognized", i + 1);
		}
	}
	return true;
}

//- aux.code

#pragma mark - High level code (mount)

static void mount_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ mount.on_timer
	if (PRIVATE_DATA->handle != NULL) {
		meade_update_mount_state(device);
		indigo_execute_handler_in(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE ? 0.5 : 1, mount_timer_callback);
	}
	//- mount.on_timer
}

static void mount_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = lx200_open(device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ mount.on_connect
			if (MOUNT_TYPE_DETECT_ITEM->sw.value && !meade_detect_mount(device)) {
				connection_result = false;
				indigo_send_message(device, ALERT_PROPERTY, "Autodetection failed!");
			}
			if (connection_result) {
				meade_init_mount(device);
				MOUNT_TYPE_PROPERTY->perm = INDIGO_RO_PERM;
				indigo_delete_property(device, MOUNT_TYPE_PROPERTY, NULL);
				indigo_define_property(device, MOUNT_TYPE_PROPERTY, NULL);
			}
			//- mount.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, MOUNT_MODE_PROPERTY, NULL);
			indigo_define_property(device, GEMINI_PARK_POSITION_PROPERTY, NULL);
			indigo_define_property(device, AP_SYNC_MODE_PROPERTY, NULL);
			indigo_define_property(device, AP_PARK_POSITION_PROPERTY, NULL);
			indigo_define_property(device, ZWO_BUZZER_PROPERTY, NULL);
			indigo_define_property(device, ZWO_MERIDIAN_PROPERTY, NULL);
			indigo_define_property(device, ZWO_MERIDIAN_LIMIT_PROPERTY, NULL);
			indigo_define_property(device, ZWO_MAX_SLEW_SPEED_PROPERTY, NULL);
			indigo_define_property(device, NYX_WIFI_AP_PROPERTY, NULL);
			indigo_define_property(device, NYX_WIFI_CL_PROPERTY, NULL);
			indigo_define_property(device, NYX_WIFI_RESET_PROPERTY, NULL);
			indigo_define_property(device, NYX_LEVELER_PROPERTY, NULL);
			indigo_define_property(device, ONSTEP_PREFERRED_PIER_SIDE_PROPERTY, NULL);
			indigo_define_property(device, ONSTEP_AUTO_MERIDIAN_FLIP_PROPERTY, NULL);
			indigo_define_property(device, ONSTEP_MERIDIAN_LIMITS_PROPERTY, NULL);
			indigo_define_property(device, ONSTEP_ALTITUDE_LIMITS_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", MOUNT_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", MOUNT_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				lx200_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ mount.on_disconnect
		meade_classic_cancel_guides(device);
		meade_stop(device);
		// The stop ended every motion, so the next session starts without one: no direction
		// left selected and no axis the next manual motion would stop first.
		PRIVATE_DATA->lastMotionNS = PRIVATE_DATA->lastMotionWE = 0;
		PRIVATE_DATA->classicGoto = PRIVATE_DATA->goto_issued = false;
		MOUNT_MOTION_NORTH_ITEM->sw.value = MOUNT_MOTION_SOUTH_ITEM->sw.value = false;
		MOUNT_MOTION_WEST_ITEM->sw.value = MOUNT_MOTION_EAST_ITEM->sw.value = false;
		MOUNT_TYPE_PROPERTY->perm = INDIGO_RW_PERM;
		indigo_delete_property(device, MOUNT_TYPE_PROPERTY, NULL);
		indigo_define_property(device, MOUNT_TYPE_PROPERTY, NULL);
		//- mount.on_disconnect
		// Cancelled change handlers must not leave properties BUSY: a new session starts in a clean state.
		indigo_property *cancelled_properties[] = {
			MOUNT_MODE_PROPERTY,
			GEMINI_PARK_POSITION_PROPERTY,
			AP_SYNC_MODE_PROPERTY,
			AP_PARK_POSITION_PROPERTY,
			ZWO_BUZZER_PROPERTY,
			ZWO_MERIDIAN_PROPERTY,
			ZWO_MERIDIAN_LIMIT_PROPERTY,
			ZWO_MAX_SLEW_SPEED_PROPERTY,
			NYX_WIFI_AP_PROPERTY,
			NYX_WIFI_CL_PROPERTY,
			NYX_WIFI_RESET_PROPERTY,
			NYX_LEVELER_PROPERTY,
			ONSTEP_PREFERRED_PIER_SIDE_PROPERTY,
			ONSTEP_AUTO_MERIDIAN_FLIP_PROPERTY,
			ONSTEP_MERIDIAN_LIMITS_PROPERTY,
			ONSTEP_ALTITUDE_LIMITS_PROPERTY,
			MOUNT_STATE_PROPERTY,
			MOUNT_PARK_PROPERTY,
			MOUNT_PARK_SET_PROPERTY,
			MOUNT_HOME_PROPERTY,
			MOUNT_HOME_SET_PROPERTY,
			MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY,
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY,
			MOUNT_ABORT_MOTION_PROPERTY,
			MOUNT_MOTION_DEC_PROPERTY,
			MOUNT_MOTION_RA_PROPERTY,
			MOUNT_SET_HOST_TIME_PROPERTY,
			MOUNT_UTC_TIME_PROPERTY,
			MOUNT_TRACKING_PROPERTY,
			MOUNT_TRACK_RATE_PROPERTY,
			MOUNT_PEC_PROPERTY,
			MOUNT_ALIGNMENT_RESET_PROPERTY,
			MOUNT_GUIDE_RATE_PROPERTY,
		};
		for (unsigned i = 0; i < sizeof(cancelled_properties) / sizeof(cancelled_properties[0]); i++) {
			if (cancelled_properties[i] != NULL && cancelled_properties[i]->state == INDIGO_BUSY_STATE) {
				cancelled_properties[i]->state = INDIGO_OK_STATE;
			}
		}
		if (MOUNT_TYPE_PROPERTY != NULL && MOUNT_TYPE_PROPERTY->state == INDIGO_BUSY_STATE) {
			INDIGO_UPDATE_PROPERTY_STATE(MOUNT_TYPE_PROPERTY, INDIGO_OK_STATE, NULL);
		}
		if (GEMINI_STARTUP_PROPERTY != NULL && GEMINI_STARTUP_PROPERTY->state == INDIGO_BUSY_STATE) {
			INDIGO_UPDATE_PROPERTY_STATE(GEMINI_STARTUP_PROPERTY, INDIGO_OK_STATE, NULL);
		}
		indigo_delete_property(device, MOUNT_MODE_PROPERTY, NULL);
		indigo_delete_property(device, GEMINI_PARK_POSITION_PROPERTY, NULL);
		indigo_delete_property(device, AP_SYNC_MODE_PROPERTY, NULL);
		indigo_delete_property(device, AP_PARK_POSITION_PROPERTY, NULL);
		indigo_delete_property(device, ZWO_BUZZER_PROPERTY, NULL);
		indigo_delete_property(device, ZWO_MERIDIAN_PROPERTY, NULL);
		indigo_delete_property(device, ZWO_MERIDIAN_LIMIT_PROPERTY, NULL);
		indigo_delete_property(device, ZWO_MAX_SLEW_SPEED_PROPERTY, NULL);
		indigo_delete_property(device, NYX_WIFI_AP_PROPERTY, NULL);
		indigo_delete_property(device, NYX_WIFI_CL_PROPERTY, NULL);
		indigo_delete_property(device, NYX_WIFI_RESET_PROPERTY, NULL);
		indigo_delete_property(device, NYX_LEVELER_PROPERTY, NULL);
		indigo_delete_property(device, ONSTEP_PREFERRED_PIER_SIDE_PROPERTY, NULL);
		indigo_delete_property(device, ONSTEP_AUTO_MERIDIAN_FLIP_PROPERTY, NULL);
		indigo_delete_property(device, ONSTEP_MERIDIAN_LIMITS_PROPERTY, NULL);
		indigo_delete_property(device, ONSTEP_ALTITUDE_LIMITS_PROPERTY, NULL);
		if (--PRIVATE_DATA->count == 0) {
			lx200_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_mount_change_property(device, NULL, CONNECTION_PROPERTY);
	if (IS_CONNECTED) {
		indigo_execute_handler(device, mount_timer_callback);
	}
}

static void mount_type_handler(indigo_device *device) {
	MOUNT_TYPE_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_TYPE.on_change
	if (MOUNT_TYPE_STARGO2_ITEM->sw.value) {
		strcpy(DEVICE_PORT_ITEM->text.value, "lx200://StarGo2.local:9624");
		INDIGO_UPDATE_PROPERTY_STATE(DEVICE_PORT_PROPERTY, INDIGO_OK_STATE, NULL);
	}
	//- mount.MOUNT_TYPE.on_change
	indigo_update_property(device, MOUNT_TYPE_PROPERTY, NULL);
}

static void mount_zwo_buzzer_handler(indigo_device *device) {
	ZWO_BUZZER_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.ZWO_BUZZER.on_change
	bool result = false;
	if (ZWO_BUZZER_OFF_ITEM->sw.value) {
		result = meade_no_reply_command(device, ":SBu0#");
	} else if (ZWO_BUZZER_LOW_ITEM->sw.value) {
		result = meade_no_reply_command(device, ":SBu1#");
	} else if (ZWO_BUZZER_HIGH_ITEM->sw.value) {
		result = meade_no_reply_command(device, ":SBu2#");
	}
	if (!result) {
		ZWO_BUZZER_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- mount.ZWO_BUZZER.on_change
	indigo_update_property(device, ZWO_BUZZER_PROPERTY, NULL);
}

static void mount_zwo_meridian_handler(indigo_device *device) {
	ZWO_MERIDIAN_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.ZWO_MERIDIAN.on_change
	if (!zwo_set_meridian(device, ZWO_MERIDIAN_AUTO_FLIP_ITEM->sw.value, ZWO_MERIDIAN_TRACK_PASSED_ITEM->sw.value, (int)ZWO_MERIDIAN_LIMIT_ITEM->number.value)) {
		ZWO_MERIDIAN_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	bool flip, track;
	int limit;
	if (zwo_get_meridian(device, &flip, &track, &limit)) {
		ZWO_MERIDIAN_AUTO_FLIP_ITEM->sw.value = flip;
		ZWO_MERIDIAN_TRACK_PASSED_ITEM->sw.value = track;
	}
	//- mount.ZWO_MERIDIAN.on_change
	indigo_update_property(device, ZWO_MERIDIAN_PROPERTY, NULL);
}

static void mount_zwo_meridian_limit_handler(indigo_device *device) {
	ZWO_MERIDIAN_LIMIT_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.ZWO_MERIDIAN_LIMIT.on_change
	if (!zwo_set_meridian(device, ZWO_MERIDIAN_AUTO_FLIP_ITEM->sw.value, ZWO_MERIDIAN_TRACK_PASSED_ITEM->sw.value, (int)ZWO_MERIDIAN_LIMIT_ITEM->number.value)) {
		ZWO_MERIDIAN_LIMIT_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	bool flip, track;
	int limit;
	if (zwo_get_meridian(device, &flip, &track, &limit)) {
		ZWO_MERIDIAN_LIMIT_ITEM->number.value = ZWO_MERIDIAN_LIMIT_ITEM->number.target = limit;
	}
	//- mount.ZWO_MERIDIAN_LIMIT.on_change
	indigo_update_property(device, ZWO_MERIDIAN_LIMIT_PROPERTY, NULL);
}

static void mount_zwo_max_slew_speed_handler(indigo_device *device) {
	ZWO_MAX_SLEW_SPEED_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.ZWO_MAX_SLEW_SPEED.on_change
	if (!meade_simple_reply_command(device, ZWO_MAX_SLEW_SPEED_LOW_ITEM->sw.value ? ":SRl720#" : ":SRl1440#") || *PRIVATE_DATA->response != '1') {
		ZWO_MAX_SLEW_SPEED_PROPERTY->state = INDIGO_ALERT_STATE;
		// A refused speed leaves the mount on the one it had, which the switch shows.
		if (meade_command(device, ":GRl#")) {
			int speed = atoi(PRIVATE_DATA->response);
			if (speed == 720 || speed == 1440) {
				indigo_set_switch(ZWO_MAX_SLEW_SPEED_PROPERTY, speed == 720 ? ZWO_MAX_SLEW_SPEED_LOW_ITEM : ZWO_MAX_SLEW_SPEED_HIGH_ITEM, true);
			}
		}
	}
	//- mount.ZWO_MAX_SLEW_SPEED.on_change
	indigo_update_property(device, ZWO_MAX_SLEW_SPEED_PROPERTY, NULL);
}

static void mount_nyx_wifi_ap_handler(indigo_device *device) {
	//+ mount.NYX_WIFI_AP.on_change
	NYX_WIFI_AP_PROPERTY->state = INDIGO_ALERT_STATE;
	NYX_WIFI_AP_SSID_ITEM->text.value[25] = 0;
	NYX_WIFI_AP_PASSWORD_ITEM->text.value[30] = 0;
	if (meade_simple_reply_command(device, ":WA%s#", NYX_WIFI_AP_SSID_ITEM->text.value) && *PRIVATE_DATA->response == '1') {
		if (meade_simple_reply_command(device, ":WB%s#", NYX_WIFI_AP_PASSWORD_ITEM->text.value) && *PRIVATE_DATA->response == '1') {
			if (meade_simple_reply_command(device, ":WLC#") && *PRIVATE_DATA->response == '1') {
				indigo_send_message(device, OK_PROPERTY, "Created access point with SSID %s", NYX_WIFI_AP_SSID_ITEM->text.value);
				NYX_WIFI_AP_PROPERTY->state = INDIGO_OK_STATE;
			}
		}
	}
	//- mount.NYX_WIFI_AP.on_change
	indigo_update_property(device, NYX_WIFI_AP_PROPERTY, NULL);
}

static void mount_nyx_wifi_cl_handler(indigo_device *device) {
	NYX_WIFI_CL_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.NYX_WIFI_CL.on_change
	char ssid[345] = "";
	char password[345] = "";
	bool encode = false;
	NYX_WIFI_CL_SSID_ITEM->text.value[25] = 0;
	NYX_WIFI_CL_PASSWORD_ITEM->text.value[30] = 0;
	if (compare_versions(MOUNT_INFO_FIRMWARE_ITEM->text.value, NYX_BASE64_THRESHOLD_VERSION) >= 0) {
		base64_encode((unsigned char *)ssid, (unsigned char *)NYX_WIFI_CL_SSID_ITEM->text.value, (long)strlen(NYX_WIFI_CL_SSID_ITEM->text.value));
		base64_encode((unsigned char *)password, (unsigned char*)NYX_WIFI_CL_PASSWORD_ITEM->text.value, (long)strlen(NYX_WIFI_CL_PASSWORD_ITEM->text.value));
		encode = true;
	}
	if (meade_simple_reply_command(device, ":WS%s#", encode ? ssid : NYX_WIFI_CL_SSID_ITEM->text.value) && *PRIVATE_DATA->response == '1') {
		if (meade_simple_reply_command(device, ":WP%s#", encode ? password : NYX_WIFI_CL_PASSWORD_ITEM->text.value) && *PRIVATE_DATA->response == '1') {
			if (meade_no_reply_command(device, ":WLC#")) {
				indigo_send_message(device, IDLE_PROPERTY, "WiFi reset!");
				INDIGO_UPDATE_PROPERTY_STATE(NYX_WIFI_CL_PROPERTY, INDIGO_OK_STATE, NULL);
				if (PRIVATE_DATA->handle && PRIVATE_DATA->handle->type == INDIGO_TCP_HANDLE) {
					indigo_execute_handler(device->master_device, indigo_disconnect_slave_devices);
				}
				return;
			}
		}
	}
	NYX_WIFI_CL_PROPERTY->state = INDIGO_ALERT_STATE;
	//- mount.NYX_WIFI_CL.on_change
	indigo_update_property(device, NYX_WIFI_CL_PROPERTY, NULL);
}

static void mount_nyx_wifi_reset_handler(indigo_device *device) {
	NYX_WIFI_RESET_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.NYX_WIFI_RESET.on_change
	if (meade_no_reply_command(device, ":WLZ#")) {
		indigo_send_message(device, IDLE_PROPERTY, "WiFi reset!");
		INDIGO_UPDATE_PROPERTY_STATE(NYX_WIFI_RESET_PROPERTY, INDIGO_OK_STATE, NULL);
		if (PRIVATE_DATA->handle && PRIVATE_DATA->handle->type == INDIGO_TCP_HANDLE) {
			indigo_execute_handler(device->master_device, indigo_disconnect_slave_devices);
		}
		return;
	}
	NYX_WIFI_RESET_PROPERTY->state = INDIGO_ALERT_STATE;
	//- mount.NYX_WIFI_RESET.on_change
	indigo_update_property(device, NYX_WIFI_RESET_PROPERTY, NULL);
}

static void mount_onstep_preferred_pier_side_handler(indigo_device *device) {
	ONSTEP_PREFERRED_PIER_SIDE_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.ONSTEP_PREFERRED_PIER_SIDE.on_change
	char cmd[16];
	if (ONSTEP_PREFERRED_PIER_SIDE_EAST_ITEM->sw.value) {
		strncpy(cmd, ":SX96,E#", sizeof(cmd));
	} else if (ONSTEP_PREFERRED_PIER_SIDE_WEST_ITEM->sw.value) {
		strncpy(cmd, ":SX96,W#", sizeof(cmd));
	} else if (ONSTEP_PREFERRED_PIER_SIDE_BEST_ITEM->sw.value) {
		strncpy(cmd, ":SX96,B#", sizeof(cmd));
	} else {
		strncpy(cmd, ":SX96,A#", sizeof(cmd));
	}
	if (!(meade_simple_reply_command(device, cmd) && *PRIVATE_DATA->response == '1')) {
		ONSTEP_PREFERRED_PIER_SIDE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- mount.ONSTEP_PREFERRED_PIER_SIDE.on_change
	indigo_update_property(device, ONSTEP_PREFERRED_PIER_SIDE_PROPERTY, NULL);
}

static void mount_onstep_auto_meridian_flip_handler(indigo_device *device) {
	ONSTEP_AUTO_MERIDIAN_FLIP_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.ONSTEP_AUTO_MERIDIAN_FLIP.on_change
	// The OnStep status poll mirrors the setting the mount reports and may overwrite the value between the copy
	// of the request and this handler, the targets keep the request. After a refusal the next poll shows the
	// setting the mount reports.
	char cmd[16];
	strncpy(cmd, indigo_get_switch_target(ONSTEP_AUTO_MERIDIAN_FLIP_PROPERTY, ONSTEP_AUTO_MERIDIAN_FLIP_ENABLED_ITEM_NAME) ? ":SX95,1#" : ":SX95,0#", sizeof(cmd));
	if (meade_simple_reply_command(device, cmd) && *PRIVATE_DATA->response == '1') {
		indigo_apply_switch_targets(ONSTEP_AUTO_MERIDIAN_FLIP_PROPERTY);
	} else {
		ONSTEP_AUTO_MERIDIAN_FLIP_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- mount.ONSTEP_AUTO_MERIDIAN_FLIP.on_change
	indigo_update_property(device, ONSTEP_AUTO_MERIDIAN_FLIP_PROPERTY, NULL);
}

static void mount_onstep_meridian_limits_handler(indigo_device *device) {
	ONSTEP_MERIDIAN_LIMITS_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.ONSTEP_MERIDIAN_LIMITS.on_change
	char command[64];
	snprintf(command, sizeof(command), ":SXE9,%d#", (int)round(ONSTEP_MERIDIAN_LIMITS_EAST_ITEM->number.target * 4.0));
	bool ok = meade_simple_reply_command(device, command) && *PRIVATE_DATA->response == '1';
	if (ok) {
		snprintf(command, sizeof(command), ":SXEA,%d#", (int)round(ONSTEP_MERIDIAN_LIMITS_WEST_ITEM->number.target * 4.0));
		ok = meade_simple_reply_command(device, command) && *PRIVATE_DATA->response == '1';
	}
	if (ok) {
		ONSTEP_MERIDIAN_LIMITS_EAST_ITEM->number.value = ONSTEP_MERIDIAN_LIMITS_EAST_ITEM->number.target;
		ONSTEP_MERIDIAN_LIMITS_WEST_ITEM->number.value = ONSTEP_MERIDIAN_LIMITS_WEST_ITEM->number.target;
	} else {
		ONSTEP_MERIDIAN_LIMITS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- mount.ONSTEP_MERIDIAN_LIMITS.on_change
	indigo_update_property(device, ONSTEP_MERIDIAN_LIMITS_PROPERTY, NULL);
}

static void mount_onstep_altitude_limits_handler(indigo_device *device) {
	ONSTEP_ALTITUDE_LIMITS_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.ONSTEP_ALTITUDE_LIMITS.on_change
	char command[64];
	snprintf(command, sizeof(command), ":Sh%+d#", (int)round(ONSTEP_ALTITUDE_LIMITS_HORIZON_ITEM->number.target));
	bool ok = meade_simple_reply_command(device, command) && *PRIVATE_DATA->response == '1';
	if (ok) {
		snprintf(command, sizeof(command), ":So%d#", (int)round(ONSTEP_ALTITUDE_LIMITS_OVERHEAD_ITEM->number.target));
		ok = meade_simple_reply_command(device, command) && *PRIVATE_DATA->response == '1';
	}
	if (ok) {
		ONSTEP_ALTITUDE_LIMITS_HORIZON_ITEM->number.value = ONSTEP_ALTITUDE_LIMITS_HORIZON_ITEM->number.target;
		ONSTEP_ALTITUDE_LIMITS_OVERHEAD_ITEM->number.value = ONSTEP_ALTITUDE_LIMITS_OVERHEAD_ITEM->number.target;
	} else {
		ONSTEP_ALTITUDE_LIMITS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- mount.ONSTEP_ALTITUDE_LIMITS.on_change
	indigo_update_property(device, ONSTEP_ALTITUDE_LIMITS_PROPERTY, NULL);
}

static void mount_park_handler(indigo_device *device) {
	MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_PARK.on_change
	// The status poll mirrors the park state the mount reports and may overwrite the value between the copy of
	// the request and this handler, the targets keep the request. Every refusal or failure below shows the
	// state the mount is in.
	bool park = indigo_get_switch_target(MOUNT_PARK_PROPERTY, MOUNT_PARK_PARKED_ITEM_NAME);
	bool unpark = indigo_get_switch_target(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM_NAME);
	if (!((PRIVATE_DATA->park_allowed && park) || (PRIVATE_DATA->unpark_allowed && unpark))) {
		// A refused request may not leave the rejected value in the property: the
		// parked item is what the generated guards of the motion and tracking
		// properties read, so a client would be locked out until the next poll.
		meade_restore_park_switch(device);
		indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
		return;
	}
	indigo_apply_switch_targets(MOUNT_PARK_PROPERTY);
	if (MOUNT_PARK_PARKED_ITEM->sw.value) {
		if (MOUNT_PARK_PROPERTY->count == 1) {
			MOUNT_PARK_PARKED_ITEM->sw.value = false;
		}
		if (meade_park(device)) {
			if (MOUNT_TYPE_AP_ITEM->sw.value) {
				if (PRIVATE_DATA->ap_parking) {
					// A firmware park position is reached through a slew, :GOS# tells when it is parked.
					MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
				} else {
					PRIVATE_DATA->parked = true;
				}
			}
			if (MOUNT_TYPE_MEADE_ITEM->sw.value || MOUNT_TYPE_10MICRONS_ITEM->sw.value || MOUNT_TYPE_GEMINI_ITEM->sw.value || MOUNT_TYPE_STARGO_ITEM->sw.value || MOUNT_TYPE_ON_STEP_ITEM->sw.value || MOUNT_TYPE_NYX_ITEM->sw.value || MOUNT_TYPE_OAT_ITEM->sw.value || MOUNT_TYPE_TEEN_ASTRO_ITEM->sw.value) {
				MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
			}
		} else {
			meade_restore_park_switch(device);
			MOUNT_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	} else if (MOUNT_PARK_UNPARKED_ITEM->sw.value) {
		if (meade_unpark(device)) {
			if (MOUNT_TYPE_10MICRONS_ITEM->sw.value || MOUNT_TYPE_STARGO_ITEM->sw.value || MOUNT_TYPE_ON_STEP_ITEM->sw.value || MOUNT_TYPE_OAT_ITEM->sw.value) {
				MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
			} else {
				// The unpark is finished the moment the mount accepts it, so the
				// cached state has to follow at once. Waiting for the next status
				// poll leaves a window in which the admission guard below refuses
				// the next request against a mount it still believes is parked.
				PRIVATE_DATA->parked = false;
			}
		} else {
			meade_restore_park_switch(device);
			MOUNT_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	// The light says whether the mount is parked, so a successful unpark turns it off
	// instead of mirroring the OK state of the request.
	MOUNT_STATE_PARK_ITEM->light.value = MOUNT_PARK_UNPARKED_ITEM->sw.value && MOUNT_PARK_PROPERTY->state == INDIGO_OK_STATE ? INDIGO_IDLE_STATE : MOUNT_PARK_PROPERTY->state;
	indigo_update_property(device, MOUNT_STATE_PROPERTY, NULL);
	//- mount.MOUNT_PARK.on_change
	indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
}

static void mount_park_set_handler(indigo_device *device) {
	MOUNT_PARK_SET_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_PARK_SET.on_change
	if (MOUNT_PARK_SET_CURRENT_ITEM->sw.value) {
		MOUNT_PARK_SET_CURRENT_ITEM->sw.value = false;
		if (meade_park_set(device)) {
			indigo_send_message(device, OK_PROPERTY, "Current position set as park position");
		} else {
			MOUNT_PARK_SET_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_send_message(device, ALERT_PROPERTY, "Setting park position failed");
		}
	}
	//- mount.MOUNT_PARK_SET.on_change
	indigo_update_property(device, MOUNT_PARK_SET_PROPERTY, NULL);
}

static void mount_home_handler(indigo_device *device) {
	MOUNT_HOME_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_HOME.on_change
	// The status poll mirrors the home state the mount reports and may clear the item between the copy of the
	// request and this handler, the target keeps the request.
	if (!(PRIVATE_DATA->home_allowed && indigo_get_switch_target(MOUNT_HOME_PROPERTY, MOUNT_HOME_ITEM_NAME))) {
		indigo_update_property(device, MOUNT_HOME_PROPERTY, NULL);
		return;
	}
	indigo_apply_switch_targets(MOUNT_HOME_PROPERTY);
	MOUNT_HOME_PROPERTY->state = INDIGO_BUSY_STATE;
	if (MOUNT_HOME_PROPERTY->count == 1) {
		indigo_set_switch(MOUNT_HOME_PROPERTY, MOUNT_HOME_ITEM, false);
	}
	if (!meade_home(device)) {
		MOUNT_HOME_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	MOUNT_STATE_HOME_ITEM->light.value = MOUNT_HOME_PROPERTY->state;
	indigo_update_property(device, MOUNT_STATE_PROPERTY, NULL);
	//- mount.MOUNT_HOME.on_change
	indigo_update_property(device, MOUNT_HOME_PROPERTY, NULL);
}

static void mount_home_set_handler(indigo_device *device) {
	MOUNT_HOME_SET_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_HOME_SET.on_change
	if (MOUNT_HOME_SET_CURRENT_ITEM->sw.value) {
		MOUNT_HOME_SET_CURRENT_ITEM->sw.value = false;
		if (meade_home_set(device)) {
			indigo_send_message(device, OK_PROPERTY, "Current position set as home");
		} else {
			MOUNT_HOME_SET_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_send_message(device, ALERT_PROPERTY, "Setting home position failed");
		}
	}
	//- mount.MOUNT_HOME_SET.on_change
	indigo_update_property(device, MOUNT_HOME_SET_PROPERTY, NULL);
}

static void mount_geographic_coordinates_handler(indigo_device *device) {
	MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_GEOGRAPHIC_COORDINATES.on_change
	if (!meade_set_site(device, MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value, MOUNT_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM->number.value)) {
		MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- mount.MOUNT_GEOGRAPHIC_COORDINATES.on_change
	indigo_update_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
}

static void mount_equatorial_coordinates_handler(indigo_device *device) {
	if (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {
		indigo_send_message(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, "Mount is parked!");
		INDIGO_UPDATE_PROPERTY_STATE(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, INDIGO_ALERT_STATE, NULL);
		return;
	}
	MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_EQUATORIAL_COORDINATES.on_change
	if (meade_host_timed_guiding(device) && (PRIVATE_DATA->classicGuideNS || PRIVATE_DATA->classicGuideWE)) {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, "Classic guiding is active");
		return;
	}
	MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
	double ra = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target;
	double dec = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target;
	indigo_j2k_to_eq(MOUNT_EPOCH_ITEM->number.value, &ra, &dec);
	if (MOUNT_ON_COORDINATES_SET_TRACK_ITEM->sw.value) {
		if (meade_set_tracking_rate(device) && meade_slew(device, ra, dec)) {
			indigo_usleep(500000); // wait for the mount to start slewing to get correct state in the position timer
			PRIVATE_DATA->goto_issued = true;
			PRIVATE_DATA->classicGoto = meade_host_timed_guiding(device);
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
			// The slew light goes on with the coordinates that go BUSY, not a poll later.
			MOUNT_STATE_SLEW_ITEM->light.value = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_STATE_PROPERTY, NULL);
		} else {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_send_message(device, ALERT_PROPERTY, "Slew failed");
		}
	} else if (MOUNT_ON_COORDINATES_SET_SYNC_ITEM->sw.value) {
		if (meade_sync(device, ra, dec)) {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_send_message(device, ALERT_PROPERTY, "Sync failed");
		}
	}
	//- mount.MOUNT_EQUATORIAL_COORDINATES.on_change
	indigo_update_coordinates(device, NULL);
}

static void mount_abort_motion_handler(indigo_device *device) {
	MOUNT_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_ABORT_MOTION.on_change
	if (MOUNT_ABORT_MOTION_ITEM->sw.value) {
		MOUNT_ABORT_MOTION_ITEM->sw.value = false;
		meade_classic_cancel_guides(device);
		if (meade_stop(device)) {
			// Abort can overtake a queued reference-position request; cancel its start
			// as well as settling an already running park/home operation. A park or a
			// home the stop interrupted is over, and nothing the driver expected from it
			// may latch on a later poll.
			indigo_cancel_pending_handler(device, mount_park_handler);
			indigo_cancel_pending_handler(device, mount_home_handler);
			PRIVATE_DATA->parking = PRIVATE_DATA->homing = false;
			PRIVATE_DATA->gemini_park_expected = PRIVATE_DATA->oat_park_expected = PRIVATE_DATA->ap_parking = false;
			if (MOUNT_PARK_PROPERTY->state == INDIGO_BUSY_STATE) {
				meade_restore_park_switch(device);
				MOUNT_STATE_PARK_ITEM->light.value = PRIVATE_DATA->parked ? INDIGO_OK_STATE : INDIGO_IDLE_STATE;
				INDIGO_UPDATE_PROPERTY_STATE(MOUNT_PARK_PROPERTY, INDIGO_OK_STATE, NULL);
			}
			if (MOUNT_HOME_PROPERTY->state == INDIGO_BUSY_STATE) {
				indigo_set_switch(MOUNT_HOME_PROPERTY, MOUNT_HOME_ITEM, false);
				MOUNT_STATE_HOME_ITEM->light.value = INDIGO_IDLE_STATE;
				INDIGO_UPDATE_PROPERTY_STATE(MOUNT_HOME_PROPERTY, INDIGO_OK_STATE, NULL);
			}
			// An aborted goto did not reach its target, so it ends as a failed request.
			// The values are the last position read, and the next poll publishes the
			// position the mount stopped at as valid again.
			bool goto_aborted = MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE;
			if (goto_aborted) {
				PRIVATE_DATA->goto_issued = false;
				MOUNT_STATE_SLEW_ITEM->light.value = INDIGO_IDLE_STATE;
			}
			indigo_update_property(device, MOUNT_STATE_PROPERTY, NULL);
			if (goto_aborted) {
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_coordinates(device, "Goto aborted");
			}
			PRIVATE_DATA->classicGoto = false;
			PRIVATE_DATA->lastMotionNS = PRIVATE_DATA->lastMotionWE = 0;
			MOUNT_MOTION_NORTH_ITEM->sw.value = false;
			MOUNT_MOTION_SOUTH_ITEM->sw.value = false;
			INDIGO_UPDATE_PROPERTY_STATE(MOUNT_MOTION_DEC_PROPERTY, INDIGO_OK_STATE, NULL);
			MOUNT_MOTION_WEST_ITEM->sw.value = false;
			MOUNT_MOTION_EAST_ITEM->sw.value = false;
			INDIGO_UPDATE_PROPERTY_STATE(MOUNT_MOTION_RA_PROPERTY, INDIGO_OK_STATE, NULL);
		} else {
			MOUNT_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	//- mount.MOUNT_ABORT_MOTION.on_change
	indigo_update_property(device, MOUNT_ABORT_MOTION_PROPERTY, NULL);
	indigo_mount_commit_motion_client(device, MOUNT_ABORT_MOTION_PROPERTY);
}

static void mount_motion_dec_handler(indigo_device *device) {
	if (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {
		indigo_send_message(device, MOUNT_MOTION_DEC_PROPERTY, "Mount is parked!");
		INDIGO_UPDATE_PROPERTY_STATE(MOUNT_MOTION_DEC_PROPERTY, INDIGO_ALERT_STATE, NULL);
		indigo_mount_commit_motion_client(device, MOUNT_MOTION_DEC_PROPERTY);
		return;
	}
	MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_MOTION_DEC.on_change
	if (meade_host_timed_guiding(device) && (PRIVATE_DATA->classicGuideNS || PRIVATE_DATA->classicGuideWE)) {
		MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, "Classic guiding is active");
		indigo_mount_commit_motion_client(device, MOUNT_MOTION_DEC_PROPERTY);
		return;
	}
	if (meade_set_slew_rate(device) && meade_motion_dec(device)) {
		if (PRIVATE_DATA->lastMotionNS) {
			MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
		}
	} else {
		MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- mount.MOUNT_MOTION_DEC.on_change
	indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
	indigo_mount_commit_motion_client(device, MOUNT_MOTION_DEC_PROPERTY);
}

static void mount_motion_ra_handler(indigo_device *device) {
	if (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {
		indigo_send_message(device, MOUNT_MOTION_RA_PROPERTY, "Mount is parked!");
		INDIGO_UPDATE_PROPERTY_STATE(MOUNT_MOTION_RA_PROPERTY, INDIGO_ALERT_STATE, NULL);
		indigo_mount_commit_motion_client(device, MOUNT_MOTION_RA_PROPERTY);
		return;
	}
	MOUNT_MOTION_RA_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_MOTION_RA.on_change
	if (meade_host_timed_guiding(device) && (PRIVATE_DATA->classicGuideNS || PRIVATE_DATA->classicGuideWE)) {
		MOUNT_MOTION_RA_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, "Classic guiding is active");
		indigo_mount_commit_motion_client(device, MOUNT_MOTION_RA_PROPERTY);
		return;
	}
	if (meade_set_slew_rate(device) && meade_motion_ra(device)) {
		if (PRIVATE_DATA->lastMotionWE) {
			MOUNT_MOTION_RA_PROPERTY->state = INDIGO_BUSY_STATE;
		}
	} else {
		MOUNT_MOTION_RA_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- mount.MOUNT_MOTION_RA.on_change
	indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
	indigo_mount_commit_motion_client(device, MOUNT_MOTION_RA_PROPERTY);
}

static void mount_set_host_time_handler(indigo_device *device) {
	MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_SET_HOST_TIME.on_change
	if (MOUNT_SET_HOST_TIME_ITEM->sw.value) {
		MOUNT_SET_HOST_TIME_ITEM->sw.value = false;
		time_t secs = time(NULL);
		if (meade_set_utc(device, secs, indigo_get_utc_offset())) {
			// A pending MOUNT_UTC_TIME request owns its items, its handler sets the requested time next.
			if (MOUNT_UTC_TIME_PROPERTY->state != INDIGO_BUSY_STATE) {
				indigo_timetoisogm(secs, MOUNT_UTC_ITEM->text.value, INDIGO_VALUE_SIZE);
				INDIGO_UPDATE_PROPERTY_STATE(MOUNT_UTC_TIME_PROPERTY, INDIGO_OK_STATE, NULL);
			}
		} else {
			MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	//- mount.MOUNT_SET_HOST_TIME.on_change
	indigo_update_property(device, MOUNT_SET_HOST_TIME_PROPERTY, NULL);
}

static void mount_utc_time_handler(indigo_device *device) {
	MOUNT_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_UTC_TIME.on_change
	int offset = 0;
	time_t secs = indigo_mount_get_utc_target(device, &offset);
	if (secs == -1) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Wrong date/time format!");
		MOUNT_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
	} else if (meade_set_utc(device, secs, offset)) {
		PRIVATE_DATA->utc_offset = offset;
		indigo_timetoisogm(secs, MOUNT_UTC_ITEM->text.value, INDIGO_VALUE_SIZE);
		snprintf(MOUNT_UTC_OFFSET_ITEM->text.value, INDIGO_VALUE_SIZE, "%d", offset);
	} else {
		MOUNT_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- mount.MOUNT_UTC_TIME.on_change
	indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
}

static void mount_tracking_handler(indigo_device *device) {
	if (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {
		indigo_send_message(device, MOUNT_TRACKING_PROPERTY, "Mount is parked!");
		INDIGO_UPDATE_PROPERTY_STATE(MOUNT_TRACKING_PROPERTY, INDIGO_ALERT_STATE, NULL);
		return;
	}
	MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_TRACKING.on_change
	// The status poll mirrors the tracking the mount reports and may overwrite the value between the copy of the
	// request and this handler, the target keeps the request. On failure the switch shows the tracking the mount
	// last reported.
	bool on = indigo_get_switch_target(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM_NAME);
	if (meade_set_tracking(device, on)) {
		indigo_apply_switch_targets(MOUNT_TRACKING_PROPERTY);
		MOUNT_STATE_TRACKING_ITEM->light.value = on ? INDIGO_OK_STATE : INDIGO_IDLE_STATE;
	} else {
		indigo_set_switch(MOUNT_TRACKING_PROPERTY, PRIVATE_DATA->tracking ? MOUNT_TRACKING_ON_ITEM : MOUNT_TRACKING_OFF_ITEM, true);
		MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
		MOUNT_STATE_TRACKING_ITEM->light.value = INDIGO_ALERT_STATE;
	}
	// MOUNT_PARK and MOUNT_HOME publish the light they changed; so does this one,
	// instead of leaving a client with the previous value until the next poll.
	indigo_update_property(device, MOUNT_STATE_PROPERTY, NULL);
	//- mount.MOUNT_TRACKING.on_change
	indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
}

static void mount_track_rate_handler(indigo_device *device) {
	MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_TRACK_RATE.on_change
	// The OnStep and NYX status polls mirror the rate the mount reports and may overwrite the value between the
	// copy of the request and this handler, the targets keep the request. meade_set_tracking_rate() sends the
	// selected item, as it does before a slew, so the request is selected first; a refused rate is shown with
	// ALERT as before.
	indigo_apply_switch_targets(MOUNT_TRACK_RATE_PROPERTY);
	if (!meade_set_tracking_rate(device)) {
		MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- mount.MOUNT_TRACK_RATE.on_change
	indigo_update_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
}

static void mount_pec_handler(indigo_device *device) {
	MOUNT_PEC_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_PEC.on_change
	if (!meade_pec(device, MOUNT_PEC_ENABLED_ITEM->sw.value)) {
		MOUNT_PEC_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- mount.MOUNT_PEC.on_change
	indigo_update_property(device, MOUNT_PEC_PROPERTY, NULL);
}

static void mount_alignment_reset_handler(indigo_device *device) {
	MOUNT_ALIGNMENT_RESET_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_ALIGNMENT_RESET.on_change
	if (MOUNT_ALIGNMENT_RESET_ITEM->sw.value) {
		if (MOUNT_TYPE_ZWO_ITEM->sw.value && PRIVATE_DATA->zwo_firmware >= 0x010204) {
			// The multi-star calibration of the controller.
			if (!meade_simple_reply_command(device, ":NSC#") || *PRIVATE_DATA->response != '1') {
				MOUNT_ALIGNMENT_RESET_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		}
		MOUNT_CONTEXT->alignment_point_count = 0;
		indigo_mount_update_alignment_points(device);
	}
	MOUNT_ALIGNMENT_RESET_ITEM->sw.value = false;
	//- mount.MOUNT_ALIGNMENT_RESET.on_change
	indigo_update_property(device, MOUNT_ALIGNMENT_RESET_PROPERTY, NULL);
}

static void mount_guide_rate_handler(indigo_device *device) {
	MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_GUIDE_RATE.on_change
	if (MOUNT_TYPE_ZWO_ITEM->sw.value || MOUNT_TYPE_MEADE_ITEM->sw.value || MOUNT_TYPE_GEMINI_ITEM->sw.value) {
		MOUNT_GUIDE_RATE_DEC_ITEM->number.value = MOUNT_GUIDE_RATE_DEC_ITEM->number.target = MOUNT_GUIDE_RATE_RA_ITEM->number.value = MOUNT_GUIDE_RATE_RA_ITEM->number.target;
	}
	if (!meade_set_guide_rate(device, (int)MOUNT_GUIDE_RATE_RA_ITEM->number.target, (int)MOUNT_GUIDE_RATE_DEC_ITEM->number.target)) {
		MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
	} else if (MOUNT_TYPE_GEMINI_ITEM->sw.value) {
		gemini_show_guider_rate(PRIVATE_DATA->guider_device, MOUNT_GUIDE_RATE_RA_ITEM->number.target);
	}
	//- mount.MOUNT_GUIDE_RATE.on_change
	indigo_update_property(device, MOUNT_GUIDE_RATE_PROPERTY, NULL);
}

#pragma mark - Device API (mount)

static indigo_result mount_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result mount_attach(indigo_device *device) {
	if (indigo_mount_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		ADDITIONAL_INSTANCES_PROPERTY->hidden = device->base_device != NULL;
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		DEVICE_BAUDRATE_PROPERTY->hidden = false;
		//+ mount.on_attach
		MOUNT_ON_COORDINATES_SET_PROPERTY->count = 2;
		//- mount.on_attach
		MOUNT_TYPE_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_TYPE_PROPERTY_NAME, MAIN_GROUP, "Mount type", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 16);
		if (MOUNT_TYPE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(MOUNT_TYPE_DETECT_ITEM, MOUNT_TYPE_DETECT_ITEM_NAME, "Autodetect", true);
		indigo_init_switch_item(MOUNT_TYPE_MEADE_ITEM, MOUNT_TYPE_MEADE_ITEM_NAME, "Meade", false);
		indigo_init_switch_item(MOUNT_TYPE_10MICRONS_ITEM, MOUNT_TYPE_10MICRONS_ITEM_NAME, "10Microns", false);
		indigo_init_switch_item(MOUNT_TYPE_GEMINI_ITEM, MOUNT_TYPE_GEMINI_ITEM_NAME, "Losmandy Gemini", false);
		indigo_init_switch_item(MOUNT_TYPE_STARGO_ITEM, MOUNT_TYPE_STARGO_ITEM_NAME, "Avalon StarGO", false);
		indigo_init_switch_item(MOUNT_TYPE_STARGO2_ITEM, MOUNT_TYPE_STARGO2_ITEM_NAME, "Avalon StarGO2", false);
		indigo_init_switch_item(MOUNT_TYPE_AP_ITEM, MOUNT_TYPE_AP_ITEM_NAME, "Astro-Physics GTO", false);
		indigo_init_switch_item(MOUNT_TYPE_ON_STEP_ITEM, MOUNT_TYPE_ON_STEP_ITEM_NAME, "OnStep", false);
		indigo_init_switch_item(MOUNT_TYPE_AGOTINO_ITEM, MOUNT_TYPE_AGOTINO_ITEM_NAME, "aGotino", false);
		indigo_init_switch_item(MOUNT_TYPE_ZWO_ITEM, MOUNT_TYPE_ZWO_ITEM_NAME, "ZWO AM", false);
		indigo_init_switch_item(MOUNT_TYPE_NYX_ITEM, MOUNT_TYPE_NYX_ITEM_NAME, "Pegasus NYX", false);
		indigo_init_switch_item(MOUNT_TYPE_OAT_ITEM, MOUNT_TYPE_OAT_ITEM_NAME, "OpenAstroTech", false);
		indigo_init_switch_item(MOUNT_TYPE_TEEN_ASTRO_ITEM, MOUNT_TYPE_TEEN_ASTRO_ITEM_NAME, "Teen Astro", false);
		indigo_init_switch_item(MOUNT_TYPE_ESP32GO_ITEM, MOUNT_TYPE_ESP32GO_ITEM_NAME, "ESP32Go", false);
		indigo_init_switch_item(MOUNT_TYPE_CLASSIC_ITEM, MOUNT_TYPE_CLASSIC_ITEM_NAME, "Meade LX200 Classic", false);
		indigo_init_switch_item(MOUNT_TYPE_GENERIC_ITEM, MOUNT_TYPE_GENERIC_ITEM_NAME, "Generic", false);
		MOUNT_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_MODE_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Mount mode", INDIGO_OK_STATE, INDIGO_RO_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (MOUNT_MODE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(EQUATORIAL_ITEM, EQUATORIAL_ITEM_NAME, "Equatorial mode", false);
		indigo_init_switch_item(ALTAZ_MODE_ITEM, ALTAZ_MODE_ITEM_NAME, "Alt/Az mode", false);
		MOUNT_MODE_PROPERTY->hidden = true;
		GEMINI_STARTUP_PROPERTY = indigo_init_switch_property(NULL, device->name, GEMINI_STARTUP_PROPERTY_NAME, MAIN_GROUP, "Gemini startup mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
		if (GEMINI_STARTUP_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(GEMINI_STARTUP_COLD_ITEM, GEMINI_STARTUP_COLD_ITEM_NAME, "Cold start", true);
		indigo_init_switch_item(GEMINI_STARTUP_WARM_ITEM, GEMINI_STARTUP_WARM_ITEM_NAME, "Warm start", false);
		indigo_init_switch_item(GEMINI_STARTUP_WARM_RESTART_ITEM, GEMINI_STARTUP_WARM_RESTART_ITEM_NAME, "Warm restart", false);
		GEMINI_PARK_POSITION_PROPERTY = indigo_init_switch_property(NULL, device->name, GEMINI_PARK_POSITION_PROPERTY_NAME, MOUNT_ADVANCED_GROUP, "Park position", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
		if (GEMINI_PARK_POSITION_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(GEMINI_PARK_POSITION_STARTUP_ITEM, GEMINI_PARK_POSITION_STARTUP_ITEM_NAME, "Startup position (CWD)", true);
		indigo_init_switch_item(GEMINI_PARK_POSITION_HOME_ITEM, GEMINI_PARK_POSITION_HOME_ITEM_NAME, "Home position", false);
		indigo_init_switch_item(GEMINI_PARK_POSITION_ZENITH_ITEM, GEMINI_PARK_POSITION_ZENITH_ITEM_NAME, "Zenith", false);
		GEMINI_PARK_POSITION_PROPERTY->hidden = true;
		AP_SYNC_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, AP_SYNC_MODE_PROPERTY_NAME, MOUNT_ADVANCED_GROUP, "Sync mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (AP_SYNC_MODE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AP_SYNC_MODE_RCAL_ITEM, AP_SYNC_MODE_RCAL_ITEM_NAME, "Recalibrate (keeps side of pier)", true);
		indigo_init_switch_item(AP_SYNC_MODE_SYNC_ITEM, AP_SYNC_MODE_SYNC_ITEM_NAME, "Sync (redefines side of pier)", false);
		AP_SYNC_MODE_PROPERTY->hidden = true;
		AP_PARK_POSITION_PROPERTY = indigo_init_switch_property(NULL, device->name, AP_PARK_POSITION_PROPERTY_NAME, MOUNT_ADVANCED_GROUP, "Park position", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 6);
		if (AP_PARK_POSITION_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AP_PARK_POSITION_CURRENT_ITEM, AP_PARK_POSITION_CURRENT_ITEM_NAME, "Current position", true);
		indigo_init_switch_item(AP_PARK_POSITION_PARK1_ITEM, AP_PARK_POSITION_PARK1_ITEM_NAME, "Park 1", false);
		indigo_init_switch_item(AP_PARK_POSITION_PARK2_ITEM, AP_PARK_POSITION_PARK2_ITEM_NAME, "Park 2", false);
		indigo_init_switch_item(AP_PARK_POSITION_PARK3_ITEM, AP_PARK_POSITION_PARK3_ITEM_NAME, "Park 3", false);
		indigo_init_switch_item(AP_PARK_POSITION_PARK4_ITEM, AP_PARK_POSITION_PARK4_ITEM_NAME, "Park 4", false);
		indigo_init_switch_item(AP_PARK_POSITION_PARK5_ITEM, AP_PARK_POSITION_PARK5_ITEM_NAME, "Park 5", false);
		AP_PARK_POSITION_PROPERTY->hidden = true;
		ZWO_BUZZER_PROPERTY = indigo_init_switch_property(NULL, device->name, ZWO_BUZZER_PROPERTY_NAME, MOUNT_ADVANCED_GROUP, "Buzzer volume", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
		if (ZWO_BUZZER_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(ZWO_BUZZER_OFF_ITEM, ZWO_BUZZER_OFF_ITEM_NAME, "Off", false);
		indigo_init_switch_item(ZWO_BUZZER_LOW_ITEM, ZWO_BUZZER_LOW_ITEM_NAME, "Low", false);
		indigo_init_switch_item(ZWO_BUZZER_HIGH_ITEM, ZWO_BUZZER_HIGH_ITEM_NAME, "High", false);
		ZWO_BUZZER_PROPERTY->hidden = true;
		ZWO_MERIDIAN_PROPERTY = indigo_init_switch_property(NULL, device->name, ZWO_MERIDIAN_PROPERTY_NAME, MOUNT_ADVANCED_GROUP, "Action at meridian", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 2);
		if (ZWO_MERIDIAN_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(ZWO_MERIDIAN_AUTO_FLIP_ITEM, ZWO_MERIDIAN_AUTO_FLIP_ITEM_NAME, "Flip automatically at the limit", false);
		indigo_init_switch_item(ZWO_MERIDIAN_TRACK_PASSED_ITEM, ZWO_MERIDIAN_TRACK_PASSED_ITEM_NAME, "Track past the meridian up to the limit", false);
		ZWO_MERIDIAN_PROPERTY->hidden = true;
		ZWO_MERIDIAN_LIMIT_PROPERTY = indigo_init_number_property(NULL, device->name, ZWO_MERIDIAN_LIMIT_PROPERTY_NAME, MOUNT_ADVANCED_GROUP, "Meridian limit", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (ZWO_MERIDIAN_LIMIT_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(ZWO_MERIDIAN_LIMIT_ITEM, ZWO_MERIDIAN_LIMIT_ITEM_NAME, "Limit past the meridian (°, negative before it)", -15, 15, 1, 0);
		ZWO_MERIDIAN_LIMIT_PROPERTY->hidden = true;
		ZWO_MAX_SLEW_SPEED_PROPERTY = indigo_init_switch_property(NULL, device->name, ZWO_MAX_SLEW_SPEED_PROPERTY_NAME, MOUNT_ADVANCED_GROUP, "Max slew speed", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (ZWO_MAX_SLEW_SPEED_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(ZWO_MAX_SLEW_SPEED_LOW_ITEM, ZWO_MAX_SLEW_SPEED_LOW_ITEM_NAME, "720x sidereal", false);
		indigo_init_switch_item(ZWO_MAX_SLEW_SPEED_HIGH_ITEM, ZWO_MAX_SLEW_SPEED_HIGH_ITEM_NAME, "1440x sidereal", false);
		ZWO_MAX_SLEW_SPEED_PROPERTY->hidden = true;
		NYX_WIFI_AP_PROPERTY = indigo_init_text_property(NULL, device->name, NYX_WIFI_AP_PROPERTY_NAME, MOUNT_ADVANCED_GROUP, "AP WiFi settings", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (NYX_WIFI_AP_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(NYX_WIFI_AP_SSID_ITEM, NYX_WIFI_AP_SSID_ITEM_NAME, "SSID", "");
		indigo_init_text_item(NYX_WIFI_AP_PASSWORD_ITEM, NYX_WIFI_AP_PASSWORD_ITEM_NAME, "Password", "");
		NYX_WIFI_AP_PROPERTY->hidden = true;
		NYX_WIFI_CL_PROPERTY = indigo_init_text_property(NULL, device->name, NYX_WIFI_CL_PROPERTY_NAME, MOUNT_ADVANCED_GROUP, "Client WiFi settings", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (NYX_WIFI_CL_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(NYX_WIFI_CL_SSID_ITEM, NYX_WIFI_CL_SSID_ITEM_NAME, "SSID", "");
		indigo_init_text_item(NYX_WIFI_CL_PASSWORD_ITEM, NYX_WIFI_CL_PASSWORD_ITEM_NAME, "Password", "");
		NYX_WIFI_CL_PROPERTY->hidden = true;
		NYX_WIFI_RESET_PROPERTY = indigo_init_switch_property(NULL, device->name, NYX_WIFI_RESET_PROPERTY_NAME, MOUNT_ADVANCED_GROUP, "Reset WiFi settings", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
		if (NYX_WIFI_RESET_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(NYX_WIFI_RESET_ITEM, NYX_WIFI_RESET_ITEM_NAME, "Reset", false);
		NYX_WIFI_RESET_PROPERTY->hidden = true;
		NYX_LEVELER_PROPERTY = indigo_init_number_property(NULL, device->name, NYX_LEVELER_PROPERTY_NAME, MOUNT_ADVANCED_GROUP, "Leveler", INDIGO_OK_STATE, INDIGO_RO_PERM, 3);
		if (NYX_LEVELER_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(NYX_LEVELER_PITCH_ITEM, NYX_LEVELER_PITCH_ITEM_NAME, "Pitch [°]", -180, 180, 0, 0);
		indigo_init_number_item(NYX_LEVELER_ROLL_ITEM, NYX_LEVELER_ROLL_ITEM_NAME, "Roll [°]", -180, 180, 0, 0);
		indigo_init_number_item(NYX_LEVELER_COMPASS_ITEM, NYX_LEVELER_COMPASS_ITEM_NAME, "Compass [°]", 0, 360, 0, 0);
		NYX_LEVELER_PROPERTY->hidden = true;
		ONSTEP_PREFERRED_PIER_SIDE_PROPERTY = indigo_init_switch_property(NULL, device->name, ONSTEP_PREFERRED_PIER_SIDE_PROPERTY_NAME, MOUNT_ADVANCED_GROUP, "Meridian flip preferred pier side", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 4);
		if (ONSTEP_PREFERRED_PIER_SIDE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(ONSTEP_PREFERRED_PIER_SIDE_EAST_ITEM, ONSTEP_PREFERRED_PIER_SIDE_EAST_ITEM_NAME, "East", false);
		indigo_init_switch_item(ONSTEP_PREFERRED_PIER_SIDE_WEST_ITEM, ONSTEP_PREFERRED_PIER_SIDE_WEST_ITEM_NAME, "West", false);
		indigo_init_switch_item(ONSTEP_PREFERRED_PIER_SIDE_BEST_ITEM, ONSTEP_PREFERRED_PIER_SIDE_BEST_ITEM_NAME, "Best", false);
		indigo_init_switch_item(ONSTEP_PREFERRED_PIER_SIDE_AUTO_ITEM, ONSTEP_PREFERRED_PIER_SIDE_AUTO_ITEM_NAME, "Auto", true);
		ONSTEP_PREFERRED_PIER_SIDE_PROPERTY->hidden = true;
		ONSTEP_AUTO_MERIDIAN_FLIP_PROPERTY = indigo_init_switch_property(NULL, device->name, ONSTEP_AUTO_MERIDIAN_FLIP_PROPERTY_NAME, MOUNT_ADVANCED_GROUP, "Automatic meridian flip at limit", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (ONSTEP_AUTO_MERIDIAN_FLIP_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(ONSTEP_AUTO_MERIDIAN_FLIP_ENABLED_ITEM, ONSTEP_AUTO_MERIDIAN_FLIP_ENABLED_ITEM_NAME, "Enabled", false);
		indigo_init_switch_item(ONSTEP_AUTO_MERIDIAN_FLIP_DISABLED_ITEM, ONSTEP_AUTO_MERIDIAN_FLIP_DISABLED_ITEM_NAME, "Disabled", true);
		ONSTEP_AUTO_MERIDIAN_FLIP_PROPERTY->hidden = true;
		ONSTEP_MERIDIAN_LIMITS_PROPERTY = indigo_init_number_property(NULL, device->name, ONSTEP_MERIDIAN_LIMITS_PROPERTY_NAME, MOUNT_ADVANCED_GROUP, "Meridian limits", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (ONSTEP_MERIDIAN_LIMITS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(ONSTEP_MERIDIAN_LIMITS_EAST_ITEM, ONSTEP_MERIDIAN_LIMITS_EAST_ITEM_NAME, "Limit past meridian, East of pier [°]", -270, 270, 0.25, 0);
		indigo_init_number_item(ONSTEP_MERIDIAN_LIMITS_WEST_ITEM, ONSTEP_MERIDIAN_LIMITS_WEST_ITEM_NAME, "Limit past meridian, West of pier [°]", -270, 270, 0.25, 0);
		ONSTEP_MERIDIAN_LIMITS_PROPERTY->hidden = true;
		ONSTEP_ALTITUDE_LIMITS_PROPERTY = indigo_init_number_property(NULL, device->name, ONSTEP_ALTITUDE_LIMITS_PROPERTY_NAME, MOUNT_ADVANCED_GROUP, "Altitude limits", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (ONSTEP_ALTITUDE_LIMITS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(ONSTEP_ALTITUDE_LIMITS_HORIZON_ITEM, ONSTEP_ALTITUDE_LIMITS_HORIZON_ITEM_NAME, "Horizon limit, min altitude [°]", -30, 30, 1, 0);
		indigo_init_number_item(ONSTEP_ALTITUDE_LIMITS_OVERHEAD_ITEM, ONSTEP_ALTITUDE_LIMITS_OVERHEAD_ITEM_NAME, "Overhead limit, max altitude [°]", 60, 90, 1, 90);
		ONSTEP_ALTITUDE_LIMITS_PROPERTY->hidden = true;
		MOUNT_STATE_PROPERTY->hidden = false;
		MOUNT_PARK_PROPERTY->hidden = false;
		MOUNT_PARK_SET_PROPERTY->hidden = true;
		MOUNT_HOME_PROPERTY->hidden = true;
		MOUNT_HOME_SET_PROPERTY->hidden = true;
		MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->hidden = false;
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->hidden = false;
		MOUNT_ABORT_MOTION_PROPERTY->hidden = false;
		MOUNT_MOTION_DEC_PROPERTY->hidden = false;
		MOUNT_MOTION_RA_PROPERTY->hidden = false;
		MOUNT_SET_HOST_TIME_PROPERTY->hidden = true;
		MOUNT_UTC_TIME_PROPERTY->hidden = true;
		MOUNT_TRACKING_PROPERTY->hidden = false;
		MOUNT_TRACK_RATE_PROPERTY->hidden = false;
		MOUNT_PEC_PROPERTY->hidden = true;
		MOUNT_ALIGNMENT_RESET_PROPERTY->hidden = true;
		MOUNT_GUIDE_RATE_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return mount_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result mount_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(MOUNT_MODE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(GEMINI_PARK_POSITION_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AP_SYNC_MODE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AP_PARK_POSITION_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(ZWO_BUZZER_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(ZWO_MERIDIAN_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(ZWO_MERIDIAN_LIMIT_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(ZWO_MAX_SLEW_SPEED_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(NYX_WIFI_AP_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(NYX_WIFI_CL_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(NYX_WIFI_RESET_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(NYX_LEVELER_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(ONSTEP_PREFERRED_PIER_SIDE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(ONSTEP_AUTO_MERIDIAN_FLIP_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(ONSTEP_MERIDIAN_LIMITS_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(ONSTEP_ALTITUDE_LIMITS_PROPERTY);
	}
	INDIGO_DEFINE_MATCHING_PROPERTY(MOUNT_TYPE_PROPERTY);
	INDIGO_DEFINE_MATCHING_PROPERTY(GEMINI_STARTUP_PROPERTY);
	return indigo_mount_enumerate_properties(device, client, property);
}

static indigo_result mount_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_CONNECT(mount_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_TYPE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_SYNC_CHANGE(MOUNT_TYPE_PROPERTY, mount_type_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GEMINI_STARTUP_PROPERTY, property)) {
		indigo_property_copy_values(GEMINI_STARTUP_PROPERTY, property, false);
		GEMINI_STARTUP_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GEMINI_STARTUP_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GEMINI_PARK_POSITION_PROPERTY, property)) {
		indigo_property_copy_values(GEMINI_PARK_POSITION_PROPERTY, property, false);
		GEMINI_PARK_POSITION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GEMINI_PARK_POSITION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AP_SYNC_MODE_PROPERTY, property)) {
		indigo_property_copy_values(AP_SYNC_MODE_PROPERTY, property, false);
		AP_SYNC_MODE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AP_SYNC_MODE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AP_PARK_POSITION_PROPERTY, property)) {
		indigo_property_copy_values(AP_PARK_POSITION_PROPERTY, property, false);
		AP_PARK_POSITION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AP_PARK_POSITION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ZWO_BUZZER_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(ZWO_BUZZER_PROPERTY, mount_zwo_buzzer_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ZWO_MERIDIAN_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(ZWO_MERIDIAN_PROPERTY, mount_zwo_meridian_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ZWO_MERIDIAN_LIMIT_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(ZWO_MERIDIAN_LIMIT_PROPERTY, mount_zwo_meridian_limit_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ZWO_MAX_SLEW_SPEED_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(ZWO_MAX_SLEW_SPEED_PROPERTY, mount_zwo_max_slew_speed_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(NYX_WIFI_AP_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(NYX_WIFI_AP_PROPERTY, mount_nyx_wifi_ap_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(NYX_WIFI_CL_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(NYX_WIFI_CL_PROPERTY, mount_nyx_wifi_cl_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(NYX_WIFI_RESET_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(NYX_WIFI_RESET_PROPERTY, mount_nyx_wifi_reset_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ONSTEP_PREFERRED_PIER_SIDE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(ONSTEP_PREFERRED_PIER_SIDE_PROPERTY, mount_onstep_preferred_pier_side_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ONSTEP_AUTO_MERIDIAN_FLIP_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(ONSTEP_AUTO_MERIDIAN_FLIP_PROPERTY, mount_onstep_auto_meridian_flip_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ONSTEP_MERIDIAN_LIMITS_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(ONSTEP_MERIDIAN_LIMITS_PROPERTY, mount_onstep_meridian_limits_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ONSTEP_ALTITUDE_LIMITS_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(ONSTEP_ALTITUDE_LIMITS_PROPERTY, mount_onstep_altitude_limits_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_PARK_PROPERTY, property)) {
		//+ mount.MOUNT_PARK.on_change_request
		PRIVATE_DATA->park_allowed = !PRIVATE_DATA->parked && !PRIVATE_DATA->parking && !PRIVATE_DATA->homing;
		PRIVATE_DATA->unpark_allowed = PRIVATE_DATA->parked && !PRIVATE_DATA->parking && !PRIVATE_DATA->homing;
		//- mount.MOUNT_PARK.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_PARK_PROPERTY, mount_park_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_PARK_SET_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_PARK_SET_PROPERTY, mount_park_set_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_HOME_PROPERTY, property)) {
		//+ mount.MOUNT_HOME.on_change_request
		PRIVATE_DATA->home_allowed = !PRIVATE_DATA->parked && !PRIVATE_DATA->parking && !PRIVATE_DATA->homing && !PRIVATE_DATA->homed;
		//- mount.MOUNT_HOME.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_HOME_PROPERTY, mount_home_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_HOME_SET_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_HOME_SET_PROPERTY, mount_home_set_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, mount_geographic_coordinates_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, "Mount is parked!");
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, mount_equatorial_coordinates_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(MOUNT_ABORT_MOTION_PROPERTY, mount_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_MOTION_DEC_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value, MOUNT_MOTION_DEC_PROPERTY, "Mount is parked!");
		indigo_mount_record_motion_client(device, client, property);
		INDIGO_COPY_VALUES_PROCESS_CHANGE_ANYTIME(MOUNT_MOTION_DEC_PROPERTY, mount_motion_dec_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_MOTION_RA_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value, MOUNT_MOTION_RA_PROPERTY, "Mount is parked!");
		indigo_mount_record_motion_client(device, client, property);
		INDIGO_COPY_VALUES_PROCESS_CHANGE_ANYTIME(MOUNT_MOTION_RA_PROPERTY, mount_motion_ra_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_SET_HOST_TIME_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_SET_HOST_TIME_PROPERTY, mount_set_host_time_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_UTC_TIME_PROPERTY, property)) {
		//+ mount.MOUNT_UTC_TIME.on_change_request
		// The status poll refreshes the items from the mount clock and MOUNT_SET_HOST_TIME from the host clock, so
		// the handler sends the requested time recorded here instead of the copied items.
		indigo_mount_set_utc_target(device, property);
		//- mount.MOUNT_UTC_TIME.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_UTC_TIME_PROPERTY, mount_utc_time_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_TRACKING_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value, MOUNT_TRACKING_PROPERTY, "Mount is parked!");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_TRACKING_PROPERTY, mount_tracking_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_TRACK_RATE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_TRACK_RATE_PROPERTY, mount_track_rate_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_PEC_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(IS_PARKED, MOUNT_PEC_PROPERTY, "Mount is parked!");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_PEC_PROPERTY, mount_pec_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_ALIGNMENT_RESET_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE, MOUNT_ALIGNMENT_RESET_PROPERTY, "Alignment data can't be reset while the mount is slewing");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_ALIGNMENT_RESET_PROPERTY, mount_alignment_reset_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_GUIDE_RATE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_GUIDE_RATE_PROPERTY, mount_guide_rate_handler);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, MOUNT_TYPE_PROPERTY);
			indigo_save_property(device, NULL, GEMINI_STARTUP_PROPERTY);
			indigo_save_property(device, NULL, GEMINI_PARK_POSITION_PROPERTY);
			indigo_save_property(device, NULL, AP_SYNC_MODE_PROPERTY);
			indigo_save_property(device, NULL, AP_PARK_POSITION_PROPERTY);
		}
	}
	return indigo_mount_change_property(device, client, property);
}

static indigo_result mount_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		mount_connection_handler(device);
	}
	indigo_release_property(MOUNT_TYPE_PROPERTY);
	indigo_release_property(MOUNT_MODE_PROPERTY);
	indigo_release_property(GEMINI_STARTUP_PROPERTY);
	indigo_release_property(GEMINI_PARK_POSITION_PROPERTY);
	indigo_release_property(AP_SYNC_MODE_PROPERTY);
	indigo_release_property(AP_PARK_POSITION_PROPERTY);
	indigo_release_property(ZWO_BUZZER_PROPERTY);
	indigo_release_property(ZWO_MERIDIAN_PROPERTY);
	indigo_release_property(ZWO_MERIDIAN_LIMIT_PROPERTY);
	indigo_release_property(ZWO_MAX_SLEW_SPEED_PROPERTY);
	indigo_release_property(NYX_WIFI_AP_PROPERTY);
	indigo_release_property(NYX_WIFI_CL_PROPERTY);
	indigo_release_property(NYX_WIFI_RESET_PROPERTY);
	indigo_release_property(NYX_LEVELER_PROPERTY);
	indigo_release_property(ONSTEP_PREFERRED_PIER_SIDE_PROPERTY);
	indigo_release_property(ONSTEP_AUTO_MERIDIAN_FLIP_PROPERTY);
	indigo_release_property(ONSTEP_MERIDIAN_LIMITS_PROPERTY);
	indigo_release_property(ONSTEP_ALTITUDE_LIMITS_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_mount_detach(device);
}

#pragma mark - High level code (guider)

static void guider_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = lx200_open(device->master_device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ guider.on_connect
			if (MOUNT_TYPE_DETECT_ITEM->sw.value && !meade_detect_mount(device->master_device)) {
				connection_result = false;
				indigo_send_message(device, ALERT_PROPERTY, "Autodetection failed!");
			}
			if (connection_result && MOUNT_TYPE_MEADE_ITEM->sw.value) {
				meade_read_meade_firmware(device->master_device);
			}
			// A Gemini has one guiding speed for both axes, 0.2 to 0.8 times the sidereal rate, which
			// the guider shows as GUIDER_RATE and the mount as MOUNT_GUIDE_RATE. Native 150.
			GUIDER_RATE_PROPERTY->hidden = true;
			if (connection_result && MOUNT_TYPE_GEMINI_ITEM->sw.value) {
				gemini_read_guiding(device->master_device);
				char speed[32];
				if (gemini_get(device, 150, speed, sizeof(speed))) {
					GUIDER_RATE_PROPERTY->count = 1;
					GUIDER_RATE_ITEM->number.min = 20;
					GUIDER_RATE_ITEM->number.max = 80;
					GUIDER_RATE_ITEM->number.step = 10;
					GUIDER_RATE_ITEM->number.value = GUIDER_RATE_ITEM->number.target = round(indigo_atod(speed) * 100);
					GUIDER_RATE_PROPERTY->hidden = false;
				}
			}
			if (connection_result && meade_host_timed_guiding(device)) {
				PRIVATE_DATA->classicGuider = device;
			}
			if (connection_result && MOUNT_TYPE_AP_ITEM->sw.value) {
				meade_read_ap_version(device->master_device);
			}
			if (connection_result && MOUNT_TYPE_AGOTINO_ITEM->sw.value) {
				// The aGotino firmware has no pulse guiding command. It reads the leading :Mg of
				// one as a slow motion request, finds no direction in the g and ignores it, so a
				// guider device that came up anyway would report every pulse as completed while
				// the mount stood still.
				connection_result = false;
			}
			//- guider.on_connect
		}
		if (connection_result) {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", GUIDER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", GUIDER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				lx200_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ guider.on_disconnect
		meade_classic_cancel_guides(device);
		PRIVATE_DATA->classicGuider = NULL;
		//- guider.on_disconnect
		// Cancelled change handlers must not leave properties BUSY: a new session starts in a clean state.
		indigo_property *cancelled_properties[] = {
			GUIDER_RATE_PROPERTY,
			GUIDER_GUIDE_DEC_PROPERTY,
			GUIDER_GUIDE_RA_PROPERTY,
		};
		for (unsigned i = 0; i < sizeof(cancelled_properties) / sizeof(cancelled_properties[0]); i++) {
			if (cancelled_properties[i] != NULL && cancelled_properties[i]->state == INDIGO_BUSY_STATE) {
				cancelled_properties[i]->state = INDIGO_OK_STATE;
			}
		}
		if (--PRIVATE_DATA->count == 0) {
			lx200_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_guider_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void guider_rate_handler(indigo_device *device) {
	GUIDER_RATE_PROPERTY->state = INDIGO_OK_STATE;
	//+ guider.GUIDER_RATE.on_change
	if (!meade_set_guide_rate(device, (int)GUIDER_RATE_ITEM->number.value, (int)GUIDER_RATE_ITEM->number.value)) {
		GUIDER_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
	} else if (MOUNT_TYPE_GEMINI_ITEM->sw.value) {
		gemini_show_mount_guide_rate(device->master_device, GUIDER_RATE_ITEM->number.value);
	}
	//- guider.GUIDER_RATE.on_change
	indigo_update_property(device, GUIDER_RATE_PROPERTY, NULL);
}

static void guider_guide_dec_handler(indigo_device *device) {
	//+ guider.GUIDER_GUIDE_DEC.on_change
	// A new request replaces the running pulse, so the finaliser of the superseded
	// one must not end the new pulse on the old deadline.
	indigo_cancel_pending_handler(device, guider_guide_dec_finalizer);
	// The finalizer of the previous pulse may have zeroed the values after the request was copied, the targets keep it.
	GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_NORTH_ITEM->number.target;
	GUIDER_GUIDE_SOUTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.target;
	int north = (int)GUIDER_GUIDE_NORTH_ITEM->number.value;
	int south = (int)GUIDER_GUIDE_SOUTH_ITEM->number.value;
	if ((north > 0 || south > 0) && !meade_guide_dec(device, north, south)) {
		GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
		INDIGO_UPDATE_PROPERTY_STATE(GUIDER_GUIDE_DEC_PROPERTY, INDIGO_ALERT_STATE, "Guide command failed");
		return;
	}
	if (north > 0) {
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_URGENT, meade_host_timed_guiding(device) ? fmax(0, PRIVATE_DATA->classicGuideDeadlineNS - indigo_monotonic_time()) : ((double)north) / 1000.0, guider_guide_dec_finalizer);
	} else if (south > 0) {
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_URGENT, meade_host_timed_guiding(device) ? fmax(0, PRIVATE_DATA->classicGuideDeadlineNS - indigo_monotonic_time()) : ((double)south) / 1000.0, guider_guide_dec_finalizer);
	} else {
		guider_guide_dec_finalizer(device);
	}
	//- guider.GUIDER_GUIDE_DEC.on_change
}

static void guider_guide_ra_handler(indigo_device *device) {
	//+ guider.GUIDER_GUIDE_RA.on_change
	// A new request replaces the running pulse, so the finaliser of the superseded
	// one must not end the new pulse on the old deadline.
	indigo_cancel_pending_handler(device, guider_guide_ra_finalizer);
	// The finalizer of the previous pulse may have zeroed the values after the request was copied, the targets keep it.
	GUIDER_GUIDE_WEST_ITEM->number.value = GUIDER_GUIDE_WEST_ITEM->number.target;
	GUIDER_GUIDE_EAST_ITEM->number.value = GUIDER_GUIDE_EAST_ITEM->number.target;
	int west = (int)GUIDER_GUIDE_WEST_ITEM->number.value;
	int east = (int)GUIDER_GUIDE_EAST_ITEM->number.value;
	if ((west > 0 || east > 0) && !meade_guide_ra(device, west, east)) {
		GUIDER_GUIDE_WEST_ITEM->number.value = GUIDER_GUIDE_EAST_ITEM->number.value = 0;
		INDIGO_UPDATE_PROPERTY_STATE(GUIDER_GUIDE_RA_PROPERTY, INDIGO_ALERT_STATE, "Guide command failed");
		return;
	}
	if (west > 0) {
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_URGENT, meade_host_timed_guiding(device) ? fmax(0, PRIVATE_DATA->classicGuideDeadlineWE - indigo_monotonic_time()) : ((double)west) / 1000.0, guider_guide_ra_finalizer);
	} else if (east > 0) {
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_URGENT, meade_host_timed_guiding(device) ? fmax(0, PRIVATE_DATA->classicGuideDeadlineWE - indigo_monotonic_time()) : ((double)east) / 1000.0, guider_guide_ra_finalizer);
	} else {
		guider_guide_ra_finalizer(device);
	}
	//- guider.GUIDER_GUIDE_RA.on_change
}

#pragma mark - Device API (guider)

static indigo_result guider_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result guider_attach(indigo_device *device) {
	if (indigo_guider_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		//+ guider.on_attach
		GUIDER_GUIDE_NORTH_ITEM->number.max = GUIDER_GUIDE_SOUTH_ITEM->number.max = GUIDER_GUIDE_EAST_ITEM->number.max = GUIDER_GUIDE_WEST_ITEM->number.max = 3000;
		PRIVATE_DATA->guider_device = device;
		//- guider.on_attach
		GUIDER_RATE_PROPERTY->hidden = false;
		GUIDER_GUIDE_DEC_PROPERTY->hidden = false;
		GUIDER_GUIDE_RA_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result guider_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	return indigo_guider_enumerate_properties(device, client, property);
}

static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_CONNECT(guider_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_RATE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(GUIDER_RATE_PROPERTY, guider_rate_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		//+ guider.GUIDER_GUIDE_DEC.on_change_request
		GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_NORTH_ITEM->number.target = 0;
		GUIDER_GUIDE_SOUTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.target = 0;
		//- guider.GUIDER_GUIDE_DEC.on_change_request
		INDIGO_COPY_VALUES_PROCESS_PRIORITY_CHANGE_ANYTIME(GUIDER_GUIDE_DEC_PROPERTY, guider_guide_dec_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_RA_PROPERTY, property)) {
		//+ guider.GUIDER_GUIDE_RA.on_change_request
		GUIDER_GUIDE_EAST_ITEM->number.value = GUIDER_GUIDE_EAST_ITEM->number.target = 0;
		GUIDER_GUIDE_WEST_ITEM->number.value = GUIDER_GUIDE_WEST_ITEM->number.target = 0;
		//- guider.GUIDER_GUIDE_RA.on_change_request
		INDIGO_COPY_VALUES_PROCESS_PRIORITY_CHANGE_ANYTIME(GUIDER_GUIDE_RA_PROPERTY, guider_guide_ra_handler);
		return INDIGO_OK;
	}
	return indigo_guider_change_property(device, client, property);
}

static indigo_result guider_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		guider_connection_handler(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_guider_detach(device);
}

#pragma mark - High level code (focuser)

static void focuser_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = lx200_open(device->master_device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ focuser.on_connect
			if (MOUNT_TYPE_DETECT_ITEM->sw.value && !meade_detect_mount(device->master_device)) {
				connection_result = false;
				indigo_send_message(device, ALERT_PROPERTY, "Autodetection failed!");
			}
			if (connection_result) {
				// Onstep answers :Fa# with 1 only in a build that has a focuser. Without one it
				// answers 0 to every focuser command, including the :FT# a move waits on, so a
				// focuser device that came up anyway could never finish its first move.
				if (MOUNT_TYPE_ON_STEP_ITEM->sw.value && !(meade_simple_reply_command(device, ":Fa#") && *PRIVATE_DATA->response == '1')) {
					connection_result = false;
				} else if (MOUNT_TYPE_MEADE_ITEM->sw.value || MOUNT_TYPE_AP_ITEM->sw.value || MOUNT_TYPE_ON_STEP_ITEM->sw.value || MOUNT_TYPE_OAT_ITEM->sw.value) {
					FOCUSER_SPEED_ITEM->number.min = 1;
					FOCUSER_SPEED_ITEM->number.value = 1;
					FOCUSER_SPEED_ITEM->number.target = 1;
					FOCUSER_SPEED_ITEM->number.max = 2;
					FOCUSER_SPEED_PROPERTY->state = INDIGO_OK_STATE;
				} else {
					connection_result = false;
				}
			}
			//- focuser.on_connect
		}
		if (connection_result) {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", FOCUSER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", FOCUSER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				lx200_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		// Cancelled change handlers must not leave properties BUSY: a new session starts in a clean state.
		indigo_property *cancelled_properties[] = {
			FOCUSER_SPEED_PROPERTY,
			FOCUSER_STEPS_PROPERTY,
			FOCUSER_ABORT_MOTION_PROPERTY,
		};
		for (unsigned i = 0; i < sizeof(cancelled_properties) / sizeof(cancelled_properties[0]); i++) {
			if (cancelled_properties[i] != NULL && cancelled_properties[i]->state == INDIGO_BUSY_STATE) {
				cancelled_properties[i]->state = INDIGO_OK_STATE;
			}
		}
		if (--PRIVATE_DATA->count == 0) {
			lx200_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void focuser_steps_handler(indigo_device *device) {
	FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_STEPS.on_change
	int steps = (int)(FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM->sw.value ^ FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value ? -FOCUSER_STEPS_ITEM->number.value : FOCUSER_STEPS_ITEM->number.value);
	if (!meade_focus_rel(device, FOCUSER_SPEED_ITEM->number.value == FOCUSER_SPEED_ITEM->number.min, steps)) {
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.FOCUSER_STEPS.on_change
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
}

static void focuser_abort_motion_handler(indigo_device *device) {
	FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_ABORT_MOTION.on_change
	if (FOCUSER_ABORT_MOTION_ITEM->sw.value) {
		FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
		PRIVATE_DATA->focus_aborted = true;
	}
	//- focuser.FOCUSER_ABORT_MOTION.on_change
	indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
}

#pragma mark - Device API (focuser)

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result focuser_attach(indigo_device *device) {
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		//+ focuser.on_attach
		FOCUSER_POSITION_PROPERTY->hidden = true;
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = false;
		//- focuser.on_attach
		FOCUSER_SPEED_PROPERTY->hidden = false;
		FOCUSER_STEPS_PROPERTY->hidden = false;
		FOCUSER_ABORT_MOTION_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	return indigo_focuser_enumerate_properties(device, client, property);
}

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_CONNECT(focuser_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_SPEED_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_SPEED_PROPERTY, property, false);
		FOCUSER_SPEED_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_STEPS_PROPERTY, focuser_steps_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_SYNC_CHANGE(FOCUSER_ABORT_MOTION_PROPERTY, focuser_abort_motion_handler);
		return INDIGO_OK;
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_connection_handler(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

#pragma mark - High level code (aux)

static void aux_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ aux.on_timer
	if (MOUNT_TYPE_NYX_ITEM->sw.value) {
		nyx_aux_update(device);
		indigo_execute_handler_in(device, 10, aux_timer_callback);
	} else if (MOUNT_TYPE_ON_STEP_ITEM->sw.value) {
		onstep_aux_update(device);
		indigo_execute_handler_in(device, 2, aux_timer_callback);
	}
	//- aux.on_timer
}

static void aux_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = lx200_open(device->master_device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ aux.on_connect
			if (MOUNT_TYPE_DETECT_ITEM->sw.value && !meade_detect_mount(device->master_device)) {
				connection_result = false;
				indigo_send_message(device, ALERT_PROPERTY, "Autodetection failed!");
			}
			if (connection_result) {
				AUX_WEATHER_PROPERTY->hidden = true;
				AUX_INFO_PROPERTY->hidden = true;
				AUX_HEATER_OUTLET_PROPERTY->hidden = true;
				AUX_POWER_OUTLET_PROPERTY->hidden = true;
				if (MOUNT_TYPE_NYX_ITEM->sw.value) {
					AUX_WEATHER_PROPERTY->hidden = false;
					AUX_INFO_PROPERTY->hidden = false;
				} else if (MOUNT_TYPE_ON_STEP_ITEM->sw.value) {
					// A build without auxiliary features has no outlet to offer, and an outlet
					// property with no item is not something a client can do anything with.
					onstep_aux_discover(device);
					AUX_HEATER_OUTLET_PROPERTY->hidden = AUX_HEATER_OUTLET_PROPERTY->count == 0;
					AUX_POWER_OUTLET_PROPERTY->hidden = AUX_POWER_OUTLET_PROPERTY->count == 0;
				} else {
					connection_result = false;
				}
			}
			//- aux.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, AUX_WEATHER_PROPERTY, NULL);
			indigo_define_property(device, AUX_INFO_PROPERTY, NULL);
			indigo_define_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
			indigo_define_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", AUX_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", AUX_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				lx200_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		// Cancelled change handlers must not leave properties BUSY: a new session starts in a clean state.
		indigo_property *cancelled_properties[] = {
			AUX_WEATHER_PROPERTY,
			AUX_INFO_PROPERTY,
			AUX_HEATER_OUTLET_PROPERTY,
			AUX_POWER_OUTLET_PROPERTY,
		};
		for (unsigned i = 0; i < sizeof(cancelled_properties) / sizeof(cancelled_properties[0]); i++) {
			if (cancelled_properties[i] != NULL && cancelled_properties[i]->state == INDIGO_BUSY_STATE) {
				cancelled_properties[i]->state = INDIGO_OK_STATE;
			}
		}
		indigo_delete_property(device, AUX_WEATHER_PROPERTY, NULL);
		indigo_delete_property(device, AUX_INFO_PROPERTY, NULL);
		indigo_delete_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
		indigo_delete_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
		if (--PRIVATE_DATA->count == 0) {
			lx200_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
	if (IS_CONNECTED) {
		indigo_execute_handler(device, aux_timer_callback);
	}
}

static void aux_heater_outlet_handler(indigo_device *device) {
	AUX_HEATER_OUTLET_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_HEATER_OUTLET.on_change
	for (int i = 0; i < AUX_HEATER_OUTLET_PROPERTY->count; i++) {
		indigo_item *item = AUX_HEATER_OUTLET_PROPERTY->items + i;
		int val = MIN((int) round(item->number.target * 2.56), 255);
		int slot = ONSTEP_AUX_HEATER_OUTLET_MAPPING[i];
		meade_simple_reply_command(device, ":SXX%d,V%d#", slot, val);
		if (PRIVATE_DATA->response[0] == '1') {
			item->number.value = item->number.target;
		} else {
			AUX_HEATER_OUTLET_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	//- aux.AUX_HEATER_OUTLET.on_change
	indigo_update_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
}

static void aux_power_outlet_handler(indigo_device *device) {
	AUX_POWER_OUTLET_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_POWER_OUTLET.on_change
	// The aux poll mirrors the outlet states the controller reports and may overwrite the values between the
	// copy of the request and this handler, the targets keep the request. An outlet the controller refused is
	// left to the next poll, which shows the state it reports.
	for (int i = 0; i < AUX_POWER_OUTLET_PROPERTY->count; i++) {
		indigo_item *item = AUX_POWER_OUTLET_PROPERTY->items + i;
		bool val = item->sw.target;
		int slot = ONSTEP_AUX_POWER_OUTLET_MAPPING[i];
		if (meade_simple_reply_command(device, ":SXX%d,V%d#", slot, val) && PRIVATE_DATA->response[0] == '1') {
			item->sw.value = val;
		} else {
			AUX_POWER_OUTLET_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	//- aux.AUX_POWER_OUTLET.on_change
	indigo_update_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
}

#pragma mark - Device API (aux)

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result aux_attach(indigo_device *device) {
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_POWERBOX | INDIGO_INTERFACE_AUX_WEATHER) == INDIGO_OK) {
		AUX_WEATHER_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_WEATHER_PROPERTY_NAME, "Info", "Weather info", INDIGO_OK_STATE, INDIGO_RO_PERM, 2);
		if (AUX_WEATHER_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_WEATHER_TEMPERATURE_ITEM, AUX_WEATHER_TEMPERATURE_ITEM_NAME, "Temperature [C]", -50, 100, 0, 0);
		indigo_init_number_item(AUX_WEATHER_PRESSURE_ITEM, AUX_WEATHER_PRESSURE_ITEM_NAME, "Pressure [mb]", 0, 2000, 0, 0);
		AUX_WEATHER_PROPERTY->hidden = true;
		AUX_INFO_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_INFO_PROPERTY_NAME, "Info", "Info", INDIGO_OK_STATE, INDIGO_RO_PERM, 1);
		if (AUX_INFO_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_INFO_VOLTAGE_ITEM, AUX_INFO_VOLTAGE_ITEM_NAME, "Voltage [V]", 0, 15, 0, 0);
		AUX_INFO_PROPERTY->hidden = true;
		AUX_HEATER_OUTLET_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_HEATER_OUTLET_PROPERTY_NAME, AUX_GROUP, "Heater outlets", INDIGO_OK_STATE, INDIGO_RW_PERM, 8);
		if (AUX_HEATER_OUTLET_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_HEATER_OUTLET_1_ITEM, AUX_HEATER_OUTLET_1_ITEM_NAME, "Heater #1 [%]", 0, 100, 1, 0);
		indigo_init_number_item(AUX_HEATER_OUTLET_2_ITEM, AUX_HEATER_OUTLET_2_ITEM_NAME, "Heater #2 [%]", 0, 100, 1, 0);
		indigo_init_number_item(AUX_HEATER_OUTLET_3_ITEM, AUX_HEATER_OUTLET_3_ITEM_NAME, "Heater #3 [%]", 0, 100, 1, 0);
		indigo_init_number_item(AUX_HEATER_OUTLET_4_ITEM, AUX_HEATER_OUTLET_4_ITEM_NAME, "Heater #4 [%]", 0, 100, 1, 0);
		indigo_init_number_item(AUX_HEATER_OUTLET_5_ITEM, AUX_HEATER_OUTLET_5_ITEM_NAME, "Heater #5 [%]", 0, 100, 1, 0);
		indigo_init_number_item(AUX_HEATER_OUTLET_6_ITEM, AUX_HEATER_OUTLET_6_ITEM_NAME, "Heater #6 [%]", 0, 100, 1, 0);
		indigo_init_number_item(AUX_HEATER_OUTLET_7_ITEM, AUX_HEATER_OUTLET_7_ITEM_NAME, "Heater #7 [%]", 0, 100, 1, 0);
		indigo_init_number_item(AUX_HEATER_OUTLET_8_ITEM, AUX_HEATER_OUTLET_8_ITEM_NAME, "Heater #8 [%]", 0, 100, 1, 0);
		AUX_HEATER_OUTLET_PROPERTY->hidden = true;
		AUX_POWER_OUTLET_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_POWER_OUTLET_PROPERTY_NAME, AUX_GROUP, "Power outlets", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 8);
		if (AUX_POWER_OUTLET_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AUX_POWER_OUTLET_1_ITEM, AUX_POWER_OUTLET_1_ITEM_NAME, "Outlet #1", true);
		indigo_init_switch_item(AUX_POWER_OUTLET_2_ITEM, AUX_POWER_OUTLET_2_ITEM_NAME, "Outlet #2", true);
		indigo_init_switch_item(AUX_POWER_OUTLET_3_ITEM, AUX_POWER_OUTLET_3_ITEM_NAME, "Outlet #3", true);
		indigo_init_switch_item(AUX_POWER_OUTLET_4_ITEM, AUX_POWER_OUTLET_4_ITEM_NAME, "Outlet #4", true);
		indigo_init_switch_item(AUX_POWER_OUTLET_5_ITEM, AUX_POWER_OUTLET_5_ITEM_NAME, "Outlet #5", true);
		indigo_init_switch_item(AUX_POWER_OUTLET_6_ITEM, AUX_POWER_OUTLET_6_ITEM_NAME, "Outlet #6", true);
		indigo_init_switch_item(AUX_POWER_OUTLET_7_ITEM, AUX_POWER_OUTLET_7_ITEM_NAME, "Outlet #7", true);
		indigo_init_switch_item(AUX_POWER_OUTLET_8_ITEM, AUX_POWER_OUTLET_8_ITEM_NAME, "Outlet #8", true);
		AUX_POWER_OUTLET_PROPERTY->hidden = true;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return aux_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_WEATHER_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_INFO_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_HEATER_OUTLET_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_POWER_OUTLET_PROPERTY);
	}
	return indigo_aux_enumerate_properties(device, client, property);
}

static indigo_result aux_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_CONNECT(aux_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_HEATER_OUTLET_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(AUX_HEATER_OUTLET_PROPERTY, aux_heater_outlet_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_POWER_OUTLET_PROPERTY, property)) {
		//+ aux.AUX_POWER_OUTLET.on_change_request
		// The aux poll writes the outlet states the controller reports into the values only and the handler sends
		// the targets, so the outlets a request does not carry must keep the reported state in their targets as
		// well. A BUSY property is left alone, the framework drops the request and its handler reads the targets.
		if (AUX_POWER_OUTLET_PROPERTY->state != INDIGO_BUSY_STATE) {
			for (int i = 0; i < AUX_POWER_OUTLET_PROPERTY->count; i++) {
				AUX_POWER_OUTLET_PROPERTY->items[i].sw.target = AUX_POWER_OUTLET_PROPERTY->items[i].sw.value;
			}
		}
		//- aux.AUX_POWER_OUTLET.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_POWER_OUTLET_PROPERTY, aux_power_outlet_handler);
		return INDIGO_OK;
	}
	return indigo_aux_change_property(device, client, property);
}

static indigo_result aux_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		aux_connection_handler(device);
	}
	indigo_release_property(AUX_WEATHER_PROPERTY);
	indigo_release_property(AUX_INFO_PROPERTY);
	indigo_release_property(AUX_HEATER_OUTLET_PROPERTY);
	indigo_release_property(AUX_POWER_OUTLET_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_aux_detach(device);
}

#pragma mark - Device templates

static indigo_device mount_template = INDIGO_DEVICE_INITIALIZER(MOUNT_DEVICE_NAME, mount_attach, mount_enumerate_properties, mount_change_property, NULL, mount_detach);

static indigo_device guider_template = INDIGO_DEVICE_INITIALIZER(GUIDER_DEVICE_NAME, guider_attach, guider_enumerate_properties, guider_change_property, NULL, guider_detach);

static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(FOCUSER_DEVICE_NAME, focuser_attach, focuser_enumerate_properties, focuser_change_property, NULL, focuser_detach);

static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(AUX_DEVICE_NAME, aux_attach, aux_enumerate_properties, aux_change_property, NULL, aux_detach);

#pragma mark - Main code

indigo_result indigo_mount_lx200(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static lx200_private_data *private_data = NULL;
	static indigo_device *mount = NULL;
	static indigo_device *guider = NULL;
	static indigo_device *focuser = NULL;
	static indigo_device *aux = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			static indigo_device_match_pattern patterns[3] = { 0 };
			strcpy(patterns[0].product_string, "NYX");
			strcpy(patterns[0].vendor_string, "Pegasus Astro");
			patterns[1].vendor_id = 0x03C3;
			patterns[1].product_id = 0x4001;
			INDIGO_REGISER_MATCH_PATTERNS(mount_template, patterns, 3);
			private_data = (lx200_private_data *)indigo_safe_malloc(sizeof(lx200_private_data));
			mount = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &mount_template);
			mount->private_data = private_data;
			mount->master_device = mount;
			indigo_attach_device(mount);
			guider = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &guider_template);
			guider->private_data = private_data;
			guider->master_device = mount;
			indigo_attach_device(guider);
			focuser = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &focuser_template);
			focuser->private_data = private_data;
			focuser->master_device = mount;
			indigo_attach_device(focuser);
			aux = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &aux_template);
			aux->private_data = private_data;
			aux->master_device = mount;
			indigo_attach_device(aux);
			break;

		}
		case INDIGO_DRIVER_SHUTDOWN: {
			VERIFY_NOT_CONNECTED(mount);
			VERIFY_NOT_CONNECTED(guider);
			VERIFY_NOT_CONNECTED(focuser);
			VERIFY_NOT_CONNECTED(aux);
			last_action = action;
			if (aux != NULL) {
				indigo_detach_device(aux);
				indigo_safe_free(aux);
				aux = NULL;
			}
			if (focuser != NULL) {
				indigo_detach_device(focuser);
				indigo_safe_free(focuser);
				focuser = NULL;
			}
			if (guider != NULL) {
				indigo_detach_device(guider);
				indigo_safe_free(guider);
				guider = NULL;
			}
			if (mount != NULL) {
				indigo_detach_device(mount);
				indigo_safe_free(mount);
				mount = NULL;
			}
			if (private_data != NULL) {
				indigo_safe_free(private_data);
				private_data = NULL;
			}
			break;

		}
		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
