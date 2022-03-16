// Copyright (c) 2018 Rumen G. Bogdanovski
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
// 2.0 by Rumen G. Bogdanovski

/** INDIGO ASCOL system driver
 \file indigo_system_ascol.c
 */

#define DRIVER_VERSION 0x0009
#define DRIVER_NAME	"indigo_system_ascol"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <assert.h>

#include <indigo/indigo_driver_xml.h>

#include "indigo_system_ascol.h"
#include "libascol/libascol.h"

#define h2d(h) (h * 15.0)
#define d2h(d) (d / 15.0)

#define REFRESH_SECONDS (1.5)

#define PRIVATE_DATA                    ((ascol_private_data *)device->private_data)

#define ALARM_GROUP                        "Alarms"
#define METEO_DATA_GROUP                   "Meteo Data"
#define FLAPS_GROUP                        "Flaps"
#define OIL_GROUP                          "Oil"
#define CORRECTIONS_GROUP                  "Callibrations"
#define SPEEDS_GROUP                       "Speeds"

// Panel
#define ALARM_PROPERTY                     (PRIVATE_DATA->alarm_property)
#define ALARM_ITEMS(index)                 (ALARM_PROPERTY->items+index)
#define ALARM_PROPERTY_NAME                "ASCOL_ALARMS"
#define ALARM_ITEM_NAME_BASE               "ALARM"

#define GLME_PROPERTY                      (PRIVATE_DATA->glme_property)
#define GLME_ITEMS(index)                  (GLME_PROPERTY->items+index)
#define GLME_PROPERTY_NAME                 "ASCOL_GLME"
#define GLME_ITEM_NAME_BASE                "VALUE"

// Telescope
#define OIL_STATE_PROPERTY                 (PRIVATE_DATA->oil_state_property)
#define OIL_STATE_ITEM                     (OIL_STATE_PROPERTY->items+0)
#define OIL_STATE_PROPERTY_NAME            "ASCOL_OIL_STATE"
#define OIL_STATE_ITEM_NAME                "STATE"

#define OIMV_PROPERTY                      (PRIVATE_DATA->oimv_property)
#define OIMV_ITEMS(index)                  (OIMV_PROPERTY->items+index)
#define OIMV_PROPERTY_NAME                 "ASCOL_OIMV"
#define OIMV_ITEM_NAME_BASE                "VALUE"

#define MOUNT_STATE_PROPERTY               (PRIVATE_DATA->mount_state_property)
#define MOUNT_STATE_ITEM                   (MOUNT_STATE_PROPERTY->items+0)
#define RA_STATE_ITEM                      (MOUNT_STATE_PROPERTY->items+1)
#define DEC_STATE_ITEM                     (MOUNT_STATE_PROPERTY->items+2)
#define MOUNT_STATE_PROPERTY_NAME          "ASCOL_MOUNT_STATE"
#define MOUNT_STATE_ITEM_NAME              "MOUNT"
#define RA_STATE_ITEM_NAME                 "RA_AXIS"
#define DEC_STATE_ITEM_NAME                "DEC_AXIS"

#define FLAP_STATE_PROPERTY                (PRIVATE_DATA->flap_state_property)
#define TUBE_FLAP_STATE_ITEM               (FLAP_STATE_PROPERTY->items+0)
#define COUDE_FLAP_STATE_ITEM              (FLAP_STATE_PROPERTY->items+1)
#define FLAP_STATE_PROPERTY_NAME           "ASCOL_FLAP_STATE"
#define TUBE_FLAP_STATE_ITEM_NAME          "TUBE"
#define COUDE_FLAP_STATE_ITEM_NAME         "COUDE"

#define FLAP_TUBE_PROPERTY                 (PRIVATE_DATA->flap_tube_property)
#define FLAP_TUBE_OPEN_ITEM                (FLAP_TUBE_PROPERTY->items+0)
#define FLAP_TUBE_CLOSE_ITEM               (FLAP_TUBE_PROPERTY->items+1)
#define FLAP_TUBE_PROPERTY_NAME            "ASCOL_FLAP_TUBE"
#define FLAP_TUBE_OPEN_ITEM_NAME           "OPEN"
#define FLAP_TUBE_CLOSE_ITEM_NAME          "CLOSE"

#define FLAP_COUDE_PROPERTY                (PRIVATE_DATA->flap_coude_property)
#define FLAP_COUDE_OPEN_ITEM               (FLAP_COUDE_PROPERTY->items+0)
#define FLAP_COUDE_CLOSE_ITEM              (FLAP_COUDE_PROPERTY->items+1)
#define FLAP_COUDE_PROPERTY_NAME           "ASCOL_COUDE_TUBE"
#define FLAP_COUDE_OPEN_ITEM_NAME          "OPEN"
#define FLAP_COUDE_CLOSE_ITEM_NAME         "CLOSE"

#define OIL_POWER_PROPERTY                 (PRIVATE_DATA->oil_power_property)
#define OIL_ON_ITEM                        (OIL_POWER_PROPERTY->items+0)
#define OIL_OFF_ITEM                       (OIL_POWER_PROPERTY->items+1)
#define OIL_POWER_PROPERTY_NAME            "ASCOL_OIL_POWER"
#define OIL_ON_ITEM_NAME                   "ON"
#define OIL_OFF_ITEM_NAME                  "OFF"

#define TELESCOPE_POWER_PROPERTY           (PRIVATE_DATA->telescope_power_property)
#define TELESCOPE_ON_ITEM                  (TELESCOPE_POWER_PROPERTY->items+0)
#define TELESCOPE_OFF_ITEM                 (TELESCOPE_POWER_PROPERTY->items+1)
#define TELESCOPE_POWER_PROPERTY_NAME      "ASCOL_TELESCOPE_POWER"
#define TELESCOPE_ON_ITEM_NAME             "ON"
#define TELESCOPE_OFF_ITEM_NAME            "OFF"

#define AXIS_CALIBRATED_PROPERTY           (PRIVATE_DATA->axis_calibrated_property)
#define RA_CALIBRATED_ITEM                 (AXIS_CALIBRATED_PROPERTY->items+0)
#define DEC_CALIBRATED_ITEM                (AXIS_CALIBRATED_PROPERTY->items+1)
#define AXIS_CALIBRATED_PROPERTY_NAME      "ASCOL_AXIS_CALIBRATED"
#define RA_CALIBRATED_ITEM_NAME            "RA"
#define DEC_CALIBRATED_ITEM_NAME           "DEC"

#define RA_CALIBRATION_PROPERTY            (PRIVATE_DATA->ra_calibration_property)
#define RA_CALIBRATION_START_ITEM          (RA_CALIBRATION_PROPERTY->items+0)
#define RA_CALIBRATION_STOP_ITEM           (RA_CALIBRATION_PROPERTY->items+1)
#define RA_CALIBRATION_PROPERTY_NAME       "ASCOL_RA_CALIBRATION"
#define RA_CALIBRATION_START_ITEM_NAME     "START"
#define RA_CALIBRATION_STOP_ITEM_NAME      "STOP"

#define DEC_CALIBRATION_PROPERTY           (PRIVATE_DATA->dec_calibration_property)
#define DEC_CALIBRATION_START_ITEM         (DEC_CALIBRATION_PROPERTY->items+0)
#define DEC_CALIBRATION_STOP_ITEM          (DEC_CALIBRATION_PROPERTY->items+1)
#define DEC_CALIBRATION_PROPERTY_NAME      "ASCOL_DEC_CALIBRATION"
#define DEC_CALIBRATION_START_ITEM_NAME    "START"
#define DEC_CALIBRATION_STOP_ITEM_NAME     "STOP"

#define ABERRATION_PROPERTY                (PRIVATE_DATA->aberration_property)
#define ABERRATION_ON_ITEM                 (ABERRATION_PROPERTY->items+0)
#define ABERRATION_OFF_ITEM                (ABERRATION_PROPERTY->items+1)
#define ABERRATION_PROPERTY_NAME           "ASCOL_ABERRATION"
#define ABERRATION_ON_ITEM_NAME            "ON"
#define ABERRATION_OFF_ITEM_NAME           "OFF"

#define PRECESSION_PROPERTY                (PRIVATE_DATA->precession_property)
#define PRECESSION_ON_ITEM                 (PRECESSION_PROPERTY->items+0)
#define PRECESSION_OFF_ITEM                (PRECESSION_PROPERTY->items+1)
#define PRECESSION_PROPERTY_NAME           "ASCOL_ABERRATION_NUTATION"
#define PRECESSION_ON_ITEM_NAME            "ON"
#define PRECESSION_OFF_ITEM_NAME           "OFF"

#define REFRACTION_PROPERTY                (PRIVATE_DATA->refraction_property)
#define REFRACTION_ON_ITEM                 (REFRACTION_PROPERTY->items+0)
#define REFRACTION_OFF_ITEM                (REFRACTION_PROPERTY->items+1)
#define REFRACTION_PROPERTY_NAME           "ASCOL_REFRACTION"
#define REFRACTION_ON_ITEM_NAME            "ON"
#define REFRACTION_OFF_ITEM_NAME           "OFF"

#define ERROR_CORRECTION_PROPERTY          (PRIVATE_DATA->error_correction_property)
#define ERROR_CORRECTION_ON_ITEM           (ERROR_CORRECTION_PROPERTY->items+0)
#define ERROR_CORRECTION_OFF_ITEM          (ERROR_CORRECTION_PROPERTY->items+1)
#define ERROR_CORRECTION_PROPERTY_NAME     "ASCOL_ERROR_CORRECTION"
#define ERROR_CORRECTION_ON_ITEM_NAME      "ON"
#define ERROR_CORRECTION_OFF_ITEM_NAME     "OFF"

#define CORRECTION_MODEL_PROPERTY          (PRIVATE_DATA->correction_model_property)
#define CORRECTION_MODEL_INDEX_ITEM        (CORRECTION_MODEL_PROPERTY->items+0)
#define CORRECTION_MODEL_PROPERTY_NAME     "ASCOL_CORRECTION_MODEL"
#define CORRECTION_MODEL_INDEX_ITEM_NAME   "INDEX"

#define GUIDE_MODE_PROPERTY                (PRIVATE_DATA->guide_mode_property)
#define GUIDE_MODE_ON_ITEM                 (GUIDE_MODE_PROPERTY->items+0)
#define GUIDE_MODE_OFF_ITEM                (GUIDE_MODE_PROPERTY->items+1)
#define GUIDE_MODE_PROPERTY_NAME           "ASCOL_GUIDE_MODE"
#define GUIDE_MODE_ON_ITEM_NAME            "ON"
#define GUIDE_MODE_OFF_ITEM_NAME           "OFF"

#define HADEC_COORDINATES_PROPERTY         (PRIVATE_DATA->hadec_coordinates_property)
#define HADEC_COORDINATES_HA_ITEM          (HADEC_COORDINATES_PROPERTY->items+0)
#define HADEC_COORDINATES_DEC_ITEM         (HADEC_COORDINATES_PROPERTY->items+1)
#define HADEC_COORDINATES_PROPERTY_NAME    "ASCOL_HADEC_COORDINATES"
#define HADEC_COORDINATES_HA_ITEM_NAME     "HA"
#define HADEC_COORDINATES_DEC_ITEM_NAME    "DEC"

#define HADEC_RELATIVE_MOVE_PROPERTY       (PRIVATE_DATA->hadec_relative_move_property)
#define HADEC_RELATIVE_MOVE_HA_ITEM        (HADEC_RELATIVE_MOVE_PROPERTY->items+0)
#define HADEC_RELATIVE_MOVE_DEC_ITEM       (HADEC_RELATIVE_MOVE_PROPERTY->items+1)
#define HADEC_RELATIVE_MOVE_PROPERTY_NAME  "ASCOL_HADEC_RELATIVE_MOVE"
#define HADEC_RELATIVE_MOVE_HA_ITEM_NAME   "RHA"
#define HADEC_RELATIVE_MOVE_DEC_ITEM_NAME  "RDEC"

#define RADEC_RELATIVE_MOVE_PROPERTY       (PRIVATE_DATA->radec_relative_move_property)
#define RADEC_RELATIVE_MOVE_RA_ITEM        (RADEC_RELATIVE_MOVE_PROPERTY->items+0)
#define RADEC_RELATIVE_MOVE_DEC_ITEM       (RADEC_RELATIVE_MOVE_PROPERTY->items+1)
#define RADEC_RELATIVE_MOVE_PROPERTY_NAME  "ASCOL_RADEC_RELATIVE_MOVE"
#define RADEC_RELATIVE_MOVE_RA_ITEM_NAME   "RRA"
#define RADEC_RELATIVE_MOVE_DEC_ITEM_NAME  "RDEC"

#define USER_SPEED_PROPERTY                (PRIVATE_DATA->user_speed_property)
#define USER_SPEED_RA_ITEM                 (USER_SPEED_PROPERTY->items+0)
#define USER_SPEED_DEC_ITEM                (USER_SPEED_PROPERTY->items+1)
#define USER_SPEED_PROPERTY_NAME           "ASCOL_USER_SPEED"
#define USER_SPEED_RA_ITEM_NAME            "RA"
#define USER_SPEED_DEC_ITEM_NAME           "DEC"

#define T1_SPEED_PROPERTY                  (PRIVATE_DATA->t1_speed_property)
#define T1_SPEED_ITEM                      (T1_SPEED_PROPERTY->items+0)
#define T1_SPEED_PROPERTY_NAME             "ASCOL_T1_SPEED"
#define T1_SPEED_ITEM_NAME                 "SPEED"

#define T2_SPEED_PROPERTY                  (PRIVATE_DATA->t2_speed_property)
#define T2_SPEED_ITEM                      (T2_SPEED_PROPERTY->items+0)
#define T2_SPEED_PROPERTY_NAME             "ASCOL_T2_SPEED"
#define T2_SPEED_ITEM_NAME                 "SPEED"

#define T3_SPEED_PROPERTY                  (PRIVATE_DATA->t3_speed_property)
#define T3_SPEED_ITEM                      (T3_SPEED_PROPERTY->items+0)
#define T3_SPEED_PROPERTY_NAME             "ASCOL_T3_SPEED"
#define T3_SPEED_ITEM_NAME                 "SPEED"

// Guider
#define GUIDE_CORRECTION_PROPERTY          (PRIVATE_DATA->guide_correction_property)
#define GUIDE_CORRECTION_RA_ITEM           (GUIDE_CORRECTION_PROPERTY->items+0)
#define GUIDE_CORRECTION_DEC_ITEM          (GUIDE_CORRECTION_PROPERTY->items+1)
#define GUIDE_CORRECTION_PROPERTY_NAME     "ASCOL_GUIDE_CORRECTION"
#define GUIDE_CORRECTION_RA_ITEM_NAME      "RA"
#define GUIDE_CORRECTION_DEC_ITEM_NAME     "DEC"

// Dome
#define DOME_POWER_PROPERTY                (PRIVATE_DATA->dome_power_property)
#define DOME_ON_ITEM                       (DOME_POWER_PROPERTY->items+0)
#define DOME_OFF_ITEM                      (DOME_POWER_PROPERTY->items+1)
#define DOME_POWER_PROPERTY_NAME           "ASCOL_DOME_POWER"
#define DOME_ON_ITEM_NAME                  "ON"
#define DOME_OFF_ITEM_NAME                 "OFF"

#define DOME_STATE_PROPERTY                 (PRIVATE_DATA->dome_state_property)
#define DOME_STATE_ITEM                     (DOME_STATE_PROPERTY->items+0)
#define DOME_STATE_PROPERTY_NAME            "ASCOL_DOME_STATE"
#define DOME_STATE_ITEM_NAME                "STATE"

#define DOME_SHUTTER_STATE_PROPERTY         (PRIVATE_DATA->dome_shutter_state_property)
#define DOME_SHUTTER_STATE_ITEM             (DOME_SHUTTER_STATE_PROPERTY->items+0)
#define DOME_SHUTTER_STATE_PROPERTY_NAME    "ASCOL_DOME_SHUTTER_STATE"
#define DOME_SHUTTER_STATE_ITEM_NAME        "STATE"

// Focuser
#define FOCUSER_STATE_PROPERTY               (PRIVATE_DATA->focus_state_property)
#define FOCUSER_STATE_ITEM                   (FOCUSER_STATE_PROPERTY->items+0)
#define FOCUSER_STATE_PROPERTY_NAME          "ASCOL_FOCUSER_STATE"
#define FOCUSER_STATE_ITEM_NAME              "STATE"

#define WARN_PARKED_MSG                      "Mount is parked, please unpark!"
#define WARN_PARKING_PROGRESS_MSG            "Mount parking is in progress, please wait until complete!"

// gp_bits is used as boolean
#define is_connected                   gp_bits

typedef struct {
	int dev_id;
	bool parked;
	bool park_update;

	int count_open;

	ascol_glst_t glst;
	ascol_oimv_t oimv;
	ascol_glme_t glme;

	pthread_mutex_t net_mutex;

	// Panel
	indigo_timer *panel_timer;
	indigo_property *alarm_property;
	indigo_property *glme_property;

	// Mount
	indigo_timer *park_timer;
	indigo_property *oil_state_property;
	indigo_property *oimv_property;
	indigo_property *mount_state_property;
	indigo_property *flap_state_property;
	indigo_property *flap_tube_property;
	indigo_property *flap_coude_property;
	indigo_property *oil_power_property;
	indigo_property *telescope_power_property;
	indigo_property *axis_calibrated_property;
	indigo_property *ra_calibration_property;
	indigo_property *dec_calibration_property;
	indigo_property *aberration_property;
	indigo_property *precession_property;
	indigo_property *refraction_property;
	indigo_property *error_correction_property;
	indigo_property *correction_model_property;
	indigo_property *guide_mode_property;
	indigo_property *hadec_coordinates_property;
	indigo_property *hadec_relative_move_property;
	indigo_property *radec_relative_move_property;
	indigo_property *user_speed_property;
	indigo_property *t1_speed_property;
	indigo_property *t2_speed_property;
	indigo_property *t3_speed_property;

	// Guider
	double guide_rate;
	indigo_timer *guider_timer_ra,
	             *guider_timer_dec,
	             *guide_correction_timer;
	indigo_property *guide_correction_property;

	// Dome
	int dome_target_position, dome_current_position;
	indigo_property *dome_power_property;
	indigo_property *dome_state_property;
	indigo_property *dome_shutter_state_property;

	// Focuser
	int focus_target_position, focus_current_position;
	indigo_property *focus_state_property;

} ascol_private_data;

static ascol_private_data *private_data = NULL;

static indigo_device *panel = NULL;
static indigo_device *mount = NULL;
static indigo_device *mount_guider = NULL;
static indigo_device *focuser = NULL;
static indigo_device *dome = NULL;


// -------------------------------------------------------------------------------- INDIGO MOUNT device implementation
static indigo_result ascol_mount_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(OIL_POWER_PROPERTY, property))
			indigo_define_property(device, OIL_POWER_PROPERTY, NULL);
		if (indigo_property_match(OIL_STATE_PROPERTY, property))
			indigo_define_property(device, OIL_STATE_PROPERTY, NULL);
		if (indigo_property_match(OIMV_PROPERTY, property))
			indigo_define_property(device, OIMV_PROPERTY, NULL);
		if (indigo_property_match(MOUNT_STATE_PROPERTY, property))
			indigo_define_property(device, MOUNT_STATE_PROPERTY, NULL);
		if (indigo_property_match(FLAP_STATE_PROPERTY, property))
			indigo_define_property(device, FLAP_STATE_PROPERTY, NULL);
		if (indigo_property_match(FLAP_TUBE_PROPERTY, property))
			indigo_define_property(device, FLAP_TUBE_PROPERTY, NULL);
		if (indigo_property_match(FLAP_COUDE_PROPERTY, property))
			indigo_define_property(device, FLAP_COUDE_PROPERTY, NULL);
		if (indigo_property_match(TELESCOPE_POWER_PROPERTY, property))
			indigo_define_property(device, TELESCOPE_POWER_PROPERTY, NULL);
		if (indigo_property_match(AXIS_CALIBRATED_PROPERTY, property))
			indigo_define_property(device, AXIS_CALIBRATED_PROPERTY, NULL);
		if (indigo_property_match(RA_CALIBRATION_PROPERTY, property))
			indigo_define_property(device, RA_CALIBRATION_PROPERTY, NULL);
		if (indigo_property_match(DEC_CALIBRATION_PROPERTY, property))
			indigo_define_property(device, DEC_CALIBRATION_PROPERTY, NULL);
		if (indigo_property_match(ABERRATION_PROPERTY, property))
			indigo_define_property(device, ABERRATION_PROPERTY, NULL);
		if (indigo_property_match(PRECESSION_PROPERTY, property))
			indigo_define_property(device, PRECESSION_PROPERTY, NULL);
		if (indigo_property_match(REFRACTION_PROPERTY, property))
			indigo_define_property(device, REFRACTION_PROPERTY, NULL);
		if (indigo_property_match(ERROR_CORRECTION_PROPERTY, property))
			indigo_define_property(device, ERROR_CORRECTION_PROPERTY, NULL);
		if (indigo_property_match(CORRECTION_MODEL_PROPERTY, property))
			indigo_define_property(device, CORRECTION_MODEL_PROPERTY, NULL);
		if (indigo_property_match(GUIDE_MODE_PROPERTY, property))
			indigo_define_property(device, GUIDE_MODE_PROPERTY, NULL);
		if (indigo_property_match(HADEC_COORDINATES_PROPERTY, property))
			indigo_define_property(device, HADEC_COORDINATES_PROPERTY, NULL);
		if (indigo_property_match(HADEC_RELATIVE_MOVE_PROPERTY, property))
			indigo_define_property(device, HADEC_RELATIVE_MOVE_PROPERTY, NULL);
		if (indigo_property_match(RADEC_RELATIVE_MOVE_PROPERTY, property))
			indigo_define_property(device, RADEC_RELATIVE_MOVE_PROPERTY, NULL);
		if (indigo_property_match(USER_SPEED_PROPERTY, property))
			indigo_define_property(device, USER_SPEED_PROPERTY, NULL);
		if (indigo_property_match(T1_SPEED_PROPERTY, property))
			indigo_define_property(device, T1_SPEED_PROPERTY, NULL);
		if (indigo_property_match(T2_SPEED_PROPERTY, property))
			indigo_define_property(device, T2_SPEED_PROPERTY, NULL);
		if (indigo_property_match(T3_SPEED_PROPERTY, property))
			indigo_define_property(device, T3_SPEED_PROPERTY, NULL);
	}
	return indigo_mount_enumerate_properties(device, NULL, NULL);
}


static bool ascol_device_open(indigo_device *device) {
	if (device->is_connected) return false;

	CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, CONNECTION_PROPERTY, NULL);

	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	if (PRIVATE_DATA->count_open++ == 0) {
		char host[255];
		int port;
		ascol_parse_devname(DEVICE_PORT_ITEM->text.value, host, &port);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Trying to connect to '%s:%d'...", host, port);
		int dev_id = ascol_open(host, port);
		if (dev_id == -1) {
			PRIVATE_DATA->count_open--;
			pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_open(%s) = %d", DEVICE_PORT_ITEM->text.value, dev_id);
			return false;
		} else if (ascol_GLLG(dev_id, AUTHENTICATION_PASSWORD_ITEM->text.value) != ASCOL_OK ) {
			ascol_close(dev_id);
			PRIVATE_DATA->count_open--;
			pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_GLLG(****): Authentication failure");
			return false;
		} else {
			PRIVATE_DATA->dev_id = dev_id;
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Connected");
		}
	}
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	return true;
}


static void mount_handle_eq_coordinates(indigo_device *device) {
	int res = INDIGO_OK;
	char *description;
	uint16_t condition;
	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	condition = asocol_check_conditions(PRIVATE_DATA->glst, ASCOL_COND_OIL_ON | ASCOL_COND_TE_ON | ASCOL_COND_TE_CALIBRATED | ASCOL_COND_TE_TRACK | ASCOL_COND_BRIGE_PARKED, &description);
	if (condition) {
		pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_TGRA(%d) unmet condition(s): %s", PRIVATE_DATA->dev_id, description);
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, "Unmet condition(s): %s", description);
		return;
	}
	MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
	/* GOTO requested */
	res = ascol_TSRA(
		PRIVATE_DATA->dev_id,
		h2d(MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target),
		MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target,
		MOUNT_SIDE_OF_PIER_WEST_ITEM->sw.value
	);
	if (res != INDIGO_OK) {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_TSRA(%d) = %d", PRIVATE_DATA->dev_id, res);
	} else {
		res = ascol_TGRA(PRIVATE_DATA->dev_id, ASCOL_ON);
		if (res != INDIGO_OK) {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_TGRA(%d) = %d", PRIVATE_DATA->dev_id, res);
		}
	}
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	indigo_update_coordinates(device, NULL);
}


static void mount_handle_hadec_coordinates(indigo_device *device) {
	int res = INDIGO_OK;
	char *description;
	uint16_t condition;
	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	condition = asocol_check_conditions(PRIVATE_DATA->glst, ASCOL_COND_OIL_ON | ASCOL_COND_TE_ON | ASCOL_COND_TE_CALIBRATED | ASCOL_COND_TE_STOP | ASCOL_COND_BRIGE_PARKED, &description);
	if (condition) {
		pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_TGHA(%d) unmet condition(s): %s", PRIVATE_DATA->dev_id, description);
		HADEC_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, HADEC_COORDINATES_PROPERTY, "Unmet condition(s): %s", description);
		return;
	}
	HADEC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
	/* GOTO requested */
	res = ascol_TSHA(
		PRIVATE_DATA->dev_id, HADEC_COORDINATES_HA_ITEM->number.target,
		HADEC_COORDINATES_DEC_ITEM->number.target
	);
	if (res != INDIGO_OK) {
		HADEC_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_TSHA(%d) = %d", PRIVATE_DATA->dev_id, res);
	} else {
		res = ascol_TGHA(PRIVATE_DATA->dev_id, ASCOL_ON);
		if (res != INDIGO_OK) {
			HADEC_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_TGHA(%d) = %d", PRIVATE_DATA->dev_id, res);
		}
	}
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	indigo_update_property(device, HADEC_COORDINATES_PROPERTY, NULL);
	indigo_update_coordinates(device, NULL);
}

static void mount_handle_hadec_relative_move(indigo_device *device) {
	int res = INDIGO_OK;
	char *description;
	uint16_t condition;
	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	condition = asocol_check_conditions(PRIVATE_DATA->glst, ASCOL_COND_OIL_ON | ASCOL_COND_TE_ON | ASCOL_COND_TE_CALIBRATED | ASCOL_COND_TE_STOP | ASCOL_COND_BRIGE_PARKED, &description);
	if (condition) {
		pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_TGHR(%d) unmet condition(s): %s", PRIVATE_DATA->dev_id, description);
		HADEC_RELATIVE_MOVE_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, HADEC_RELATIVE_MOVE_PROPERTY, "Unmet condition(s): %s", description);
		return;
	}
	HADEC_RELATIVE_MOVE_PROPERTY->state = INDIGO_OK_STATE;
	/* Relative GOTO requested */
	res = ascol_TSHR(PRIVATE_DATA->dev_id, HADEC_RELATIVE_MOVE_HA_ITEM->number.target, HADEC_RELATIVE_MOVE_DEC_ITEM->number.target);
	if (res != INDIGO_OK) {
		HADEC_RELATIVE_MOVE_PROPERTY->state = INDIGO_ALERT_STATE;
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_TSHR(%d) = %d", PRIVATE_DATA->dev_id, res);
	} else {
		res = ascol_TGHR(PRIVATE_DATA->dev_id, ASCOL_ON);
		if (res != INDIGO_OK) {
			HADEC_RELATIVE_MOVE_PROPERTY->state = INDIGO_ALERT_STATE;
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_TGHR(%d) = %d", PRIVATE_DATA->dev_id, res);
		}
	}
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	indigo_update_property(device, HADEC_RELATIVE_MOVE_PROPERTY, NULL);
	indigo_update_coordinates(device, NULL);
}


static void mount_handle_radec_relative_move(indigo_device *device) {
	int res = INDIGO_OK;
	char *description;
	uint16_t condition;
	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	condition = asocol_check_conditions(PRIVATE_DATA->glst, ASCOL_COND_OIL_ON | ASCOL_COND_TE_ON | ASCOL_COND_TE_CALIBRATED | ASCOL_COND_TE_TRACK | ASCOL_COND_BRIGE_PARKED, &description);
	if (condition) {
		pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_TGRR(%d) unmet condition(s): %s", PRIVATE_DATA->dev_id, description);
		RADEC_RELATIVE_MOVE_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, RADEC_RELATIVE_MOVE_PROPERTY, "Unmet condition(s): %s", description);
		return;
	}
	RADEC_RELATIVE_MOVE_PROPERTY->state = INDIGO_OK_STATE;
	/* Relative GOTO requested */
	res = ascol_TSRR(PRIVATE_DATA->dev_id, RADEC_RELATIVE_MOVE_RA_ITEM->number.target, RADEC_RELATIVE_MOVE_DEC_ITEM->number.target);
	if (res != INDIGO_OK) {
		RADEC_RELATIVE_MOVE_PROPERTY->state = INDIGO_ALERT_STATE;
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_TSRR(%d) = %d", PRIVATE_DATA->dev_id, res);
	} else {
		res = ascol_TGRR(PRIVATE_DATA->dev_id, ASCOL_ON);
		if (res != INDIGO_OK) {
			RADEC_RELATIVE_MOVE_PROPERTY->state = INDIGO_ALERT_STATE;
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_TGRR(%d) = %d", PRIVATE_DATA->dev_id, res);
		}
	}
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	indigo_update_property(device, RADEC_RELATIVE_MOVE_PROPERTY, NULL);
	indigo_update_coordinates(device, NULL);
}


static void mount_handle_user_speed(indigo_device *device) {
	int res = INDIGO_OK;
	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	USER_SPEED_PROPERTY->state = INDIGO_OK_STATE;
	res = ascol_TSUS(PRIVATE_DATA->dev_id, USER_SPEED_RA_ITEM->number.target, USER_SPEED_DEC_ITEM->number.target);
	if (res != INDIGO_OK) {
		USER_SPEED_PROPERTY->state = INDIGO_ALERT_STATE;
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_TSUS(%d) = %d", PRIVATE_DATA->dev_id, res);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	indigo_update_property(device, USER_SPEED_PROPERTY, NULL);
}


static void mount_handle_t1_speed(indigo_device *device) {
	int res = INDIGO_OK;
	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	T1_SPEED_PROPERTY->state = INDIGO_OK_STATE;
	res = ascol_TSS1(PRIVATE_DATA->dev_id, T1_SPEED_ITEM->number.target);
	if (res != INDIGO_OK) {
		T1_SPEED_PROPERTY->state = INDIGO_ALERT_STATE;
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_TSS1(%d) = %d", PRIVATE_DATA->dev_id, res);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	indigo_update_property(device, T1_SPEED_PROPERTY, NULL);
}


static void mount_handle_t2_speed(indigo_device *device) {
	int res = INDIGO_OK;
	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	T2_SPEED_PROPERTY->state = INDIGO_OK_STATE;
	res = ascol_TSS2(PRIVATE_DATA->dev_id, T2_SPEED_ITEM->number.target);
	if (res != INDIGO_OK) {
		T2_SPEED_PROPERTY->state = INDIGO_ALERT_STATE;
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_TSS2(%d) = %d", PRIVATE_DATA->dev_id, res);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	indigo_update_property(device, T2_SPEED_PROPERTY, NULL);
}


static void mount_handle_t3_speed(indigo_device *device) {
	int res = INDIGO_OK;
	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	T3_SPEED_PROPERTY->state = INDIGO_OK_STATE;
	res = ascol_TSS3(PRIVATE_DATA->dev_id, T3_SPEED_ITEM->number.target);
	if (res != INDIGO_OK) {
		T3_SPEED_PROPERTY->state = INDIGO_ALERT_STATE;
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_TSS3(%d) = %d", PRIVATE_DATA->dev_id, res);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	indigo_update_property(device, T3_SPEED_PROPERTY, NULL);
}


static void mount_handle_tracking(indigo_device *device) {
	int res = ASCOL_OK;
	char *description = NULL;
	uint16_t condition = 0;
	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	if (MOUNT_TRACKING_ON_ITEM->sw.value) {
		condition = asocol_check_conditions(PRIVATE_DATA->glst, ASCOL_COND_TE_ON, &description);
		if (condition) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_TETR(%d, ASCOL_ON) unmet condition: %s", PRIVATE_DATA->dev_id, description);
			res = ASCOL_COMMAND_ERROR;
		} else {
			res = ascol_TETR(PRIVATE_DATA->dev_id, ASCOL_ON);
			PRIVATE_DATA->park_update = true;
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_TETR(%d, ASCOL_ON) = %d", PRIVATE_DATA->dev_id, res);
		}
	} else {
		res = ascol_TETR(PRIVATE_DATA->dev_id, ASCOL_OFF);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_TETR(%d, ASCOL_OFF) = %d", PRIVATE_DATA->dev_id, res);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	if(res == ASCOL_OK) {
		MOUNT_TRACKING_PROPERTY->state = INDIGO_BUSY_STATE;
	} else {
		MOUNT_TRACKING_ON_ITEM->sw.value = !MOUNT_TRACKING_ON_ITEM->sw.value;
		MOUNT_TRACKING_OFF_ITEM->sw.value = !MOUNT_TRACKING_OFF_ITEM->sw.value;
		MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
		if (condition) {
			indigo_update_property(device, MOUNT_TRACKING_PROPERTY, "Unmet condition: %s", description);
			return;
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_TETR(%d) = %d", PRIVATE_DATA->dev_id, res);
		}
	}
	indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
}


static void mount_handle_oil_power(indigo_device *device) {
	int res = ASCOL_OK;
	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	if (OIL_ON_ITEM->sw.value) {
		res = ascol_OION(PRIVATE_DATA->dev_id, ASCOL_ON);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_OION(%d, ASCOL_ON) = %d", PRIVATE_DATA->dev_id, res);
	} else {
		res = ascol_OION(PRIVATE_DATA->dev_id, ASCOL_OFF);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_OION(%d, ASCOL_OFF) = %d", PRIVATE_DATA->dev_id, res);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	if(res == ASCOL_OK) {
		OIL_POWER_PROPERTY->state = INDIGO_BUSY_STATE;
	} else {
		OIL_ON_ITEM->sw.value = !OIL_ON_ITEM->sw.value;
		OIL_OFF_ITEM->sw.value = !OIL_OFF_ITEM->sw.value;
		OIL_POWER_PROPERTY->state = INDIGO_ALERT_STATE;
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_OION(%d) = %d", PRIVATE_DATA->dev_id, res);
	}
	indigo_update_property(device, OIL_POWER_PROPERTY, NULL);
}


static void mount_handle_telescope_power(indigo_device *device) {
	int res = ASCOL_OK;
	char *description = NULL;
	uint16_t condition = 0;
	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	if (TELESCOPE_ON_ITEM->sw.value) {
		condition = asocol_check_conditions(PRIVATE_DATA->glst, ASCOL_COND_OIL_ON, &description);
		if (condition) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_TEON(%d, ASCOL_ON) unmet condition: %s", PRIVATE_DATA->dev_id, description);
			res = ASCOL_COMMAND_ERROR;
		} else {
			res = ascol_TEON(PRIVATE_DATA->dev_id, ASCOL_ON);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_TEON(%d, ASCOL_ON) = %d", PRIVATE_DATA->dev_id, res);
		}
	} else {
		res = ascol_TEON(PRIVATE_DATA->dev_id, ASCOL_OFF);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_TEON(%d, ASCOL_OFF) = %d", PRIVATE_DATA->dev_id, res);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	if(res == ASCOL_OK) {
		TELESCOPE_POWER_PROPERTY->state = INDIGO_BUSY_STATE;
	} else {
		TELESCOPE_ON_ITEM->sw.value = !TELESCOPE_ON_ITEM->sw.value;
		TELESCOPE_OFF_ITEM->sw.value = !TELESCOPE_OFF_ITEM->sw.value;
		TELESCOPE_POWER_PROPERTY->state = INDIGO_ALERT_STATE;
		if (condition) {
			indigo_update_property(device, TELESCOPE_POWER_PROPERTY, "Unmet condition: %s", description);
			return;
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_TEON(%d) = %d", PRIVATE_DATA->dev_id, res);
		}
	}
	indigo_update_property(device, TELESCOPE_POWER_PROPERTY, NULL);
}


static void mount_handle_ra_calibration(indigo_device *device) {
	int res = ASCOL_OK;
	char *description = NULL;
	uint16_t condition = 0;
	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	if (RA_CALIBRATION_START_ITEM->sw.value) {
		condition = asocol_check_conditions(PRIVATE_DATA->glst, ASCOL_COND_OIL_ON | ASCOL_COND_TE_ON | ASCOL_COND_TE_STOP | ASCOL_COND_BRIGE_PARKED, &description);
		if (condition) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_TEHC(%d, ASCOL_ON) unmet condition(s): %s", PRIVATE_DATA->dev_id, description);
			res = ASCOL_COMMAND_ERROR;
		} else {
			res = ascol_TEHC(PRIVATE_DATA->dev_id, ASCOL_ON);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_TEHC(%d, ASCOL_ON) = %d", PRIVATE_DATA->dev_id, res);
		}
	} else {
		res = ascol_TEHC(PRIVATE_DATA->dev_id, ASCOL_OFF);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_TEHC(%d, ASCOL_OFF) = %d", PRIVATE_DATA->dev_id, res);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	if(res == ASCOL_OK) {
		RA_CALIBRATION_PROPERTY->state = INDIGO_BUSY_STATE;
	} else {
		RA_CALIBRATION_START_ITEM->sw.value = false;
		RA_CALIBRATION_STOP_ITEM->sw.value = false;
		RA_CALIBRATION_PROPERTY->state = INDIGO_ALERT_STATE;
		if (condition) {
			indigo_update_property(device, RA_CALIBRATION_PROPERTY, "Unmet condition(s): %s", description);
			return;
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_TEHC(%d) = %d", PRIVATE_DATA->dev_id, res);
		}
	}
	indigo_update_property(device, RA_CALIBRATION_PROPERTY, NULL);
}


static void mount_handle_dec_calibration(indigo_device *device) {
	int res = ASCOL_OK;
	char *description = NULL;
	uint16_t condition = 0;
	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	if (DEC_CALIBRATION_START_ITEM->sw.value) {
		condition = asocol_check_conditions(PRIVATE_DATA->glst, ASCOL_COND_OIL_ON | ASCOL_COND_TE_ON | ASCOL_COND_TE_STOP | ASCOL_COND_BRIGE_PARKED, &description);
		if (condition) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_TEDC(%d, ASCOL_ON) unmet condition(s): %s", PRIVATE_DATA->dev_id, description);
			res = ASCOL_COMMAND_ERROR;
		} else {
			res = ascol_TEDC(PRIVATE_DATA->dev_id, ASCOL_ON);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_TEDC(%d, ASCOL_ON) = %d", PRIVATE_DATA->dev_id, res);
		}
	} else {
		res = ascol_TEDC(PRIVATE_DATA->dev_id, ASCOL_OFF);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_TEDC(%d, ASCOL_OFF) = %d", PRIVATE_DATA->dev_id, res);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	if(res == ASCOL_OK) {
		DEC_CALIBRATION_PROPERTY->state = INDIGO_BUSY_STATE;
	} else {
		DEC_CALIBRATION_START_ITEM->sw.value = false;
		DEC_CALIBRATION_STOP_ITEM->sw.value = false;
		DEC_CALIBRATION_PROPERTY->state = INDIGO_ALERT_STATE;
		if (condition) {
			indigo_update_property(device, DEC_CALIBRATION_PROPERTY, "Unmet condition(s): %s", description);
			return;
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_TEDC(%d) = %d", PRIVATE_DATA->dev_id, res);
		}
	}
	indigo_update_property(device, RA_CALIBRATION_PROPERTY, NULL);
}


static void mount_handle_aberration(indigo_device *device) {
	int res = ASCOL_OK;
	char *description;
	uint16_t condition;
	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	condition = asocol_check_conditions(PRIVATE_DATA->glst, ASCOL_COND_TE_STOP, &description);
	if (condition) {
		pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_TSCA(%d) unmet condition(s): %s", PRIVATE_DATA->dev_id, description);
		ABERRATION_PROPERTY->state = INDIGO_ALERT_STATE;
		ABERRATION_ON_ITEM->sw.value = !ABERRATION_ON_ITEM->sw.value;
		ABERRATION_OFF_ITEM->sw.value = !ABERRATION_ON_ITEM->sw.value;
		indigo_update_property(device, ABERRATION_PROPERTY, "Unmet condition(s): %s", description);
		return;
	}
	if (ABERRATION_ON_ITEM->sw.value) {
		res = ascol_TSCA(PRIVATE_DATA->dev_id, ASCOL_ON);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_TSCA(%d, ASCOL_ON) = %d", PRIVATE_DATA->dev_id, res);
	} else {
		res = ascol_TSCA(PRIVATE_DATA->dev_id, ASCOL_OFF);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_TSCA(%d, ASCOL_OFF) = %d", PRIVATE_DATA->dev_id, res);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	if(res == ASCOL_OK) {
		ABERRATION_PROPERTY->state = INDIGO_BUSY_STATE;
	} else {
		ABERRATION_PROPERTY->state = INDIGO_ALERT_STATE;
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_TSCA(%d) = %d", PRIVATE_DATA->dev_id, res);
	}
	indigo_update_property(device, ABERRATION_PROPERTY, NULL);
}


static void mount_handle_precession(indigo_device *device) {
	int res = ASCOL_OK;
	char *description;
	uint16_t condition;
	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	condition = asocol_check_conditions(PRIVATE_DATA->glst, ASCOL_COND_TE_STOP, &description);
	if (condition) {
		pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_TSCP(%d) unmet condition(s): %s", PRIVATE_DATA->dev_id, description);
		PRECESSION_PROPERTY->state = INDIGO_ALERT_STATE;
		PRECESSION_ON_ITEM->sw.value = !PRECESSION_ON_ITEM->sw.value;
		PRECESSION_OFF_ITEM->sw.value = !PRECESSION_ON_ITEM->sw.value;
		indigo_update_property(device, PRECESSION_PROPERTY, "Unmet condition(s): %s", description);
		return;
	}
	if (PRECESSION_ON_ITEM->sw.value) {
		res = ascol_TSCP(PRIVATE_DATA->dev_id, ASCOL_ON);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_TSCP(%d, ASCOL_ON) = %d", PRIVATE_DATA->dev_id, res);
	} else {
		res = ascol_TSCP(PRIVATE_DATA->dev_id, ASCOL_OFF);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_TSCP(%d, ASCOL_OFF) = %d", PRIVATE_DATA->dev_id, res);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	if(res == ASCOL_OK) {
		PRECESSION_PROPERTY->state = INDIGO_BUSY_STATE;
	} else {
		PRECESSION_PROPERTY->state = INDIGO_ALERT_STATE;
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_TSCP(%d) = %d", PRIVATE_DATA->dev_id, res);
	}
	indigo_update_property(device, PRECESSION_PROPERTY, NULL);
}


static void mount_handle_refraction(indigo_device *device) {
	int res = ASCOL_OK;
	char *description;
	uint16_t condition;
	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	condition = asocol_check_conditions(PRIVATE_DATA->glst, ASCOL_COND_TE_STOP, &description);
	if (condition) {
		pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_TSCR(%d) unmet condition(s): %s", PRIVATE_DATA->dev_id, description);
		REFRACTION_PROPERTY->state = INDIGO_ALERT_STATE;
		REFRACTION_ON_ITEM->sw.value = !REFRACTION_ON_ITEM->sw.value;
		REFRACTION_OFF_ITEM->sw.value = !REFRACTION_ON_ITEM->sw.value;
		indigo_update_property(device, REFRACTION_PROPERTY, "Unmet condition(s): %s", description);
		return;
	}
	if (REFRACTION_ON_ITEM->sw.value) {
		res = ascol_TSCR(PRIVATE_DATA->dev_id, ASCOL_ON);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_TSCR(%d, ASCOL_ON) = %d", PRIVATE_DATA->dev_id, res);
	} else {
		res = ascol_TSCR(PRIVATE_DATA->dev_id, ASCOL_OFF);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_TSCR(%d, ASCOL_OFF) = %d", PRIVATE_DATA->dev_id, res);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	if(res == ASCOL_OK) {
		REFRACTION_PROPERTY->state = INDIGO_BUSY_STATE;
	} else {
		REFRACTION_PROPERTY->state = INDIGO_ALERT_STATE;
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_TSCR(%d) = %d", PRIVATE_DATA->dev_id, res);
	}
	indigo_update_property(device, REFRACTION_PROPERTY, NULL);
}


static void mount_handle_error_correction(indigo_device *device) {
	int res = ASCOL_OK;
	char *description;
	uint16_t condition;
	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	condition = asocol_check_conditions(PRIVATE_DATA->glst, ASCOL_COND_TE_STOP, &description);
	if (condition) {
		pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_TSCM(%d) unmet condition(s): %s", PRIVATE_DATA->dev_id, description);
		ERROR_CORRECTION_PROPERTY->state = INDIGO_ALERT_STATE;
		ERROR_CORRECTION_ON_ITEM->sw.value = !ERROR_CORRECTION_ON_ITEM->sw.value;
		ERROR_CORRECTION_OFF_ITEM->sw.value = !ERROR_CORRECTION_ON_ITEM->sw.value;
		indigo_update_property(device, ERROR_CORRECTION_PROPERTY, "Unmet condition(s): %s", description);
		return;
	}
	if (ERROR_CORRECTION_ON_ITEM->sw.value) {
		res = ascol_TSCM(PRIVATE_DATA->dev_id, ASCOL_ON);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_TSCM(%d, ASCOL_ON) = %d", PRIVATE_DATA->dev_id, res);
	} else {
		res = ascol_TSCM(PRIVATE_DATA->dev_id, ASCOL_OFF);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_TSCM(%d, ASCOL_OFF) = %d", PRIVATE_DATA->dev_id, res);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	if(res == ASCOL_OK) {
		ERROR_CORRECTION_PROPERTY->state = INDIGO_BUSY_STATE;
	} else {
		ERROR_CORRECTION_PROPERTY->state = INDIGO_ALERT_STATE;
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_TSCM(%d) = %d", PRIVATE_DATA->dev_id, res);
	}
	indigo_update_property(device, ERROR_CORRECTION_PROPERTY, NULL);
}


static void mount_handle_correction_model(indigo_device *device) {
	int res = ASCOL_OK;
	char *description;
	uint16_t condition;
	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	condition = asocol_check_conditions(PRIVATE_DATA->glst, ASCOL_COND_TE_STOP, &description);
	if (condition) {
		pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_TSCS(%d) unmet condition(s): %s", PRIVATE_DATA->dev_id, description);
		CORRECTION_MODEL_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, CORRECTION_MODEL_PROPERTY, "Unmet condition(s): %s", description);
		return;
	}
	int index = (int)CORRECTION_MODEL_INDEX_ITEM->number.value;
	res = ascol_TSCS(PRIVATE_DATA->dev_id, index);
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_TSCS(%d, %d) = %d", PRIVATE_DATA->dev_id, index, res);
	if(res == ASCOL_OK) {
		CORRECTION_MODEL_PROPERTY->state = INDIGO_BUSY_STATE;
	} else {
		CORRECTION_MODEL_PROPERTY->state = INDIGO_ALERT_STATE;
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_TSCS(%d, %d) = %d", PRIVATE_DATA->dev_id, index, res);
	}
	indigo_update_property(device, CORRECTION_MODEL_PROPERTY, NULL);
}


static void mount_handle_guide_mode(indigo_device *device) {
	int res = ASCOL_OK;
	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	if (GUIDE_MODE_ON_ITEM->sw.value) {
		res = ascol_TSGM(PRIVATE_DATA->dev_id, ASCOL_ON);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_TSGM(%d, ASCOL_ON) = %d", PRIVATE_DATA->dev_id, res);
	} else {
		res = ascol_TSGM(PRIVATE_DATA->dev_id, ASCOL_OFF);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_TSGM(%d, ASCOL_OFF) = %d", PRIVATE_DATA->dev_id, res);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	if(res == ASCOL_OK) {
		GUIDE_MODE_PROPERTY->state = INDIGO_BUSY_STATE;
	} else {
		GUIDE_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_TSGM(%d) = %d", PRIVATE_DATA->dev_id, res);
	}
	indigo_update_property(device, GUIDE_MODE_PROPERTY, NULL);
}


static void mount_handle_flap_tube(indigo_device *device) {
	int res = ASCOL_OK;
	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	if (FLAP_TUBE_OPEN_ITEM->sw.value) {
		res = ascol_FTOC(PRIVATE_DATA->dev_id, ASCOL_ON);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_FTOC(%d, ASCOL_ON) = %d", PRIVATE_DATA->dev_id, res);
	} else {
		res = ascol_FTOC(PRIVATE_DATA->dev_id, ASCOL_OFF);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_FTOC(%d, ASCOL_OFF) = %d", PRIVATE_DATA->dev_id, res);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	if(res == ASCOL_OK) {
		FLAP_TUBE_PROPERTY->state = INDIGO_BUSY_STATE;
	} else {
		FLAP_TUBE_PROPERTY->state = INDIGO_ALERT_STATE;
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_FTOC(%d) = %d", PRIVATE_DATA->dev_id, res);
	}
	indigo_update_property(device, FLAP_TUBE_PROPERTY, NULL);
}


static void mount_handle_flap_coude(indigo_device *device) {
	int res = ASCOL_OK;
	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	if (FLAP_COUDE_OPEN_ITEM->sw.value) {
		res = ascol_FCOC(PRIVATE_DATA->dev_id, ASCOL_ON);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_FCOC(%d, ASCOL_ON) = %d", PRIVATE_DATA->dev_id, res);
	} else {
		res = ascol_FCOC(PRIVATE_DATA->dev_id, ASCOL_OFF);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_FCOC(%d, ASCOL_OFF) = %d", PRIVATE_DATA->dev_id, res);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	if(res == ASCOL_OK) {
		FLAP_COUDE_PROPERTY->state = INDIGO_BUSY_STATE;
	} else {
		FLAP_COUDE_PROPERTY->state = INDIGO_ALERT_STATE;
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_FCOC(%d) = %d", PRIVATE_DATA->dev_id, res);
	}
	indigo_update_property(device, FLAP_COUDE_PROPERTY, NULL);
}


static void mount_handle_park(indigo_device *device) {
	bool error_flag = false;

	/* UNPARK */
	if (MOUNT_PARK_UNPARKED_ITEM->sw.value) {
		MOUNT_PARK_UNPARKED_ITEM->sw.value = true;
		MOUNT_PARK_PARKED_ITEM->sw.value = false;
		PRIVATE_DATA->parked = false;
		indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
		return;
	}

	/* PARK */
	if ((PRIVATE_DATA->glst.telescope_state != TE_STATE_TRACK) &&
	    (PRIVATE_DATA->glst.telescope_state != TE_STATE_STOP)) {
		MOUNT_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_PARK_PROPERTY, "Can not park - Telescope is either moving or off.");
		return;
	}

	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	int res = ascol_TETR(PRIVATE_DATA->dev_id, ASCOL_OFF);
	if (res != ASCOL_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_TETR(%d, ASCOL_OFF) = %d", PRIVATE_DATA->dev_id, res);
		error_flag = true;
	}
	res = ascol_TSHA(PRIVATE_DATA->dev_id, -90, 89.99);
	if (res != ASCOL_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_TSHA(%d, -90, 89.99) = %d", PRIVATE_DATA->dev_id, res);
		error_flag = true;
	}
	res = ascol_TGHA(PRIVATE_DATA->dev_id, ASCOL_ON);
	if (res != ASCOL_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_TGHA(%d, ASCOL_ON) = %d", PRIVATE_DATA->dev_id, res);
		error_flag = true;
	}
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);

	if (error_flag) {
		MOUNT_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
	}

	PRIVATE_DATA->park_update = true;
	indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
}


static bool mount_handle_abort_motion(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	int res = ascol_TGRA(PRIVATE_DATA->dev_id, ASCOL_OFF);
	if (res != ASCOL_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_TGRA(%d, ASCOL_OFF) = %d", PRIVATE_DATA->dev_id, res);
	}
	res = ascol_TGHA(PRIVATE_DATA->dev_id, ASCOL_OFF);
	if (res != ASCOL_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_TGHA(%d, ASCOL_OFF) = %d", PRIVATE_DATA->dev_id, res);
	}
	res = ascol_TGRR(PRIVATE_DATA->dev_id, ASCOL_OFF);
	if (res != ASCOL_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_TGRR(%d, ASCOL_OFF) = %d", PRIVATE_DATA->dev_id, res);
	}
	res = ascol_TGHR(PRIVATE_DATA->dev_id, ASCOL_OFF);
	if (res != ASCOL_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_TGHR(%d, ASCOL_OFF) = %d", PRIVATE_DATA->dev_id, res);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);

	MOUNT_ABORT_MOTION_ITEM->sw.value = false;
	MOUNT_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_ABORT_MOTION_PROPERTY, "Aborted.");
	MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_PARK_PROPERTY, "Aborted.");
	return true;
}


static void ascol_device_close(indigo_device *device) {
	if (!device->is_connected) return;

	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	if (--PRIVATE_DATA->count_open == 0) {
		ascol_close(PRIVATE_DATA->dev_id);
		PRIVATE_DATA->dev_id = -1;
	}
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
}


static void mount_update_state() {
	indigo_device *device = mount;
	if ((device == NULL) || (!IS_CONNECTED)) return;

	static ascol_glst_t prev_glst = {0};
	static ascol_oimv_t prev_oimv = {0};
	static bool update_all = true;

	char *descr, *descrs;

	if (update_all || (prev_glst.state_bits != PRIVATE_DATA->glst.state_bits)) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Updating AXIS_CALIBRATED_PROPERTY (dev = %d)", PRIVATE_DATA->dev_id);
		AXIS_CALIBRATED_PROPERTY->state = INDIGO_OK_STATE;
		if(IS_RA_CALIBRATED(PRIVATE_DATA->glst)) {
			RA_CALIBRATED_ITEM->light.value = INDIGO_OK_STATE;
		} else {
			RA_CALIBRATED_ITEM->light.value = INDIGO_ALERT_STATE;
			AXIS_CALIBRATED_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		if(IS_DA_CALIBRATED(PRIVATE_DATA->glst)) {
			DEC_CALIBRATED_ITEM->light.value = INDIGO_OK_STATE;
		} else {
			DEC_CALIBRATED_ITEM->light.value = INDIGO_ALERT_STATE;
			AXIS_CALIBRATED_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, AXIS_CALIBRATED_PROPERTY, NULL);
	}

	if (update_all || (prev_glst.oil_state != PRIVATE_DATA->glst.oil_state) ||
	   (OIL_POWER_PROPERTY->state == INDIGO_BUSY_STATE)) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Updating OIL_STATE_PROPERTY (dev = %d)", PRIVATE_DATA->dev_id);
		OIL_STATE_PROPERTY->state = INDIGO_OK_STATE;
		pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
		ascol_get_oil_state(PRIVATE_DATA->glst, &descr, &descrs);
		pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
		snprintf(OIL_STATE_ITEM->text.value, INDIGO_VALUE_SIZE, "%s - %s", descrs, descr);
		indigo_update_property(device, OIL_STATE_PROPERTY, NULL);

		if (PRIVATE_DATA->glst.oil_state == OIL_STATE_OFF) {
			OIL_ON_ITEM->sw.value = false;
			OIL_OFF_ITEM->sw.value = true;
			OIL_POWER_PROPERTY->state = INDIGO_OK_STATE;
		} else if(PRIVATE_DATA->glst.oil_state == OIL_STATE_ON) {
			OIL_ON_ITEM->sw.value = true;
			OIL_OFF_ITEM->sw.value = false;
			OIL_POWER_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			OIL_ON_ITEM->sw.value = true;
			OIL_OFF_ITEM->sw.value = false;
			OIL_POWER_PROPERTY->state = INDIGO_BUSY_STATE;
		}
		indigo_update_property(device, OIL_POWER_PROPERTY, NULL);
	}

	if (update_all || (prev_glst.telescope_state != PRIVATE_DATA->glst.telescope_state) ||
	   (prev_glst.ra_axis_state != PRIVATE_DATA->glst.ra_axis_state) ||
	   (prev_glst.de_axis_state != PRIVATE_DATA->glst.de_axis_state) ||
	   (TELESCOPE_POWER_PROPERTY->state == INDIGO_BUSY_STATE) ||
	   (MOUNT_TRACKING_PROPERTY->state == INDIGO_BUSY_STATE) ||
	   (RA_CALIBRATION_PROPERTY->state == INDIGO_BUSY_STATE) ||
	   (DEC_CALIBRATION_PROPERTY->state == INDIGO_BUSY_STATE)) {
		MOUNT_STATE_PROPERTY->state = INDIGO_OK_STATE;
		pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
		ascol_get_telescope_state(PRIVATE_DATA->glst, &descr, &descrs);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Updating MOUNT_STATE_PROPERTY (dev = %d) %d %s %s", PRIVATE_DATA->dev_id,PRIVATE_DATA->glst.telescope_state, descrs, descr);
		snprintf(MOUNT_STATE_ITEM->text.value, INDIGO_VALUE_SIZE, "%s - %s", descrs, descr);
		ascol_get_ra_axis_state(PRIVATE_DATA->glst, &descr, &descrs);
		snprintf(RA_STATE_ITEM->text.value, INDIGO_VALUE_SIZE, "%s - %s", descrs, descr);
		ascol_get_de_axis_state(PRIVATE_DATA->glst, &descr, &descrs);
		snprintf(DEC_STATE_ITEM->text.value, INDIGO_VALUE_SIZE, "%s - %s", descrs, descr);
		pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
		indigo_update_property(device, MOUNT_STATE_PROPERTY, NULL);

		TELESCOPE_POWER_PROPERTY->state = INDIGO_OK_STATE;
		if ((PRIVATE_DATA->glst.telescope_state == TE_STATE_OFF) ||
		    (PRIVATE_DATA->glst.telescope_state == TE_STATE_INIT)) {
			TELESCOPE_ON_ITEM->sw.value = false;
			TELESCOPE_OFF_ITEM->sw.value = true;
		} else {
			TELESCOPE_ON_ITEM->sw.value = true;
			TELESCOPE_OFF_ITEM->sw.value = false;
		}
		indigo_update_property(device, TELESCOPE_POWER_PROPERTY, NULL);

		MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
		if ((PRIVATE_DATA->glst.telescope_state == TE_STATE_TRACK) ||
		    ((PRIVATE_DATA->glst.telescope_state >= TE_STATE_ST_DECC1) &&
		    (PRIVATE_DATA->glst.telescope_state <= TE_STATE_ST_CLU3))) {
			MOUNT_TRACKING_ON_ITEM->sw.value = true;
			MOUNT_TRACKING_OFF_ITEM->sw.value = false;
		} else {
			MOUNT_TRACKING_ON_ITEM->sw.value = false;
			MOUNT_TRACKING_OFF_ITEM->sw.value = true;
		}
		indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);

		if ((PRIVATE_DATA->glst.ra_axis_state == 0) ||
			(PRIVATE_DATA->glst.ra_axis_state == 1)) {
			RA_CALIBRATION_START_ITEM->sw.value = false;
			RA_CALIBRATION_STOP_ITEM->sw.value = false;
			RA_CALIBRATION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			RA_CALIBRATION_PROPERTY->state = INDIGO_BUSY_STATE;
			RA_CALIBRATION_START_ITEM->sw.value = true;
		}
		indigo_update_property(device, RA_CALIBRATION_PROPERTY, NULL);

		if ((PRIVATE_DATA->glst.de_axis_state == 0) ||
			(PRIVATE_DATA->glst.de_axis_state == 1)) {
			DEC_CALIBRATION_START_ITEM->sw.value = false;
			DEC_CALIBRATION_STOP_ITEM->sw.value = false;
			DEC_CALIBRATION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			DEC_CALIBRATION_PROPERTY->state = INDIGO_BUSY_STATE;
			DEC_CALIBRATION_START_ITEM->sw.value = true;
		}
		indigo_update_property(device, DEC_CALIBRATION_PROPERTY, NULL);
	}

	if (update_all || (IS_ABEARRATION_CORR(prev_glst) != IS_ABEARRATION_CORR(PRIVATE_DATA->glst)) ||
	   (ABERRATION_PROPERTY->state == INDIGO_BUSY_STATE)) {
		ABERRATION_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_ABEARRATION_CORR(PRIVATE_DATA->glst)) {
			ABERRATION_ON_ITEM->sw.value = true;
			ABERRATION_OFF_ITEM->sw.value = false;
		} else {
			ABERRATION_ON_ITEM->sw.value = false;
			ABERRATION_OFF_ITEM->sw.value = true;
		}
		indigo_update_property(device, ABERRATION_PROPERTY, NULL);
	}

	if (update_all || (IS_PRECESSION_CORR(prev_glst) != IS_PRECESSION_CORR(PRIVATE_DATA->glst)) ||
	   (PRECESSION_PROPERTY->state == INDIGO_BUSY_STATE)) {
		PRECESSION_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_PRECESSION_CORR(PRIVATE_DATA->glst)) {
			PRECESSION_ON_ITEM->sw.value = true;
			PRECESSION_OFF_ITEM->sw.value = false;
		} else {
			PRECESSION_ON_ITEM->sw.value = false;
			PRECESSION_OFF_ITEM->sw.value = true;
		}
		indigo_update_property(device, PRECESSION_PROPERTY, NULL);
	}

	if (update_all || (IS_REFRACTION_CORR(prev_glst) != IS_REFRACTION_CORR(PRIVATE_DATA->glst)) ||
	   (REFRACTION_PROPERTY->state == INDIGO_BUSY_STATE)) {
		REFRACTION_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_REFRACTION_CORR(PRIVATE_DATA->glst)) {
			REFRACTION_ON_ITEM->sw.value = true;
			REFRACTION_OFF_ITEM->sw.value = false;
		} else {
			REFRACTION_ON_ITEM->sw.value = false;
			REFRACTION_OFF_ITEM->sw.value = true;
		}
		indigo_update_property(device, REFRACTION_PROPERTY, NULL);
	}

	if (update_all || (IS_ERR_MODEL_CORR(prev_glst) != IS_ERR_MODEL_CORR(PRIVATE_DATA->glst)) ||
	   (ERROR_CORRECTION_PROPERTY->state == INDIGO_BUSY_STATE)) {
		ERROR_CORRECTION_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_ERR_MODEL_CORR(PRIVATE_DATA->glst)) {
			ERROR_CORRECTION_ON_ITEM->sw.value = true;
			ERROR_CORRECTION_OFF_ITEM->sw.value = false;
		} else {
			ERROR_CORRECTION_ON_ITEM->sw.value = false;
			ERROR_CORRECTION_OFF_ITEM->sw.value = true;
		}
		indigo_update_property(device, ERROR_CORRECTION_PROPERTY, NULL);
	}

	if (update_all || (prev_glst.selected_model_index != PRIVATE_DATA->glst.selected_model_index) ||
	   (CORRECTION_MODEL_PROPERTY->state == INDIGO_BUSY_STATE)) {
		CORRECTION_MODEL_PROPERTY->state = INDIGO_OK_STATE;
		CORRECTION_MODEL_INDEX_ITEM->number.value = (double)PRIVATE_DATA->glst.selected_model_index;
		indigo_update_property(device, CORRECTION_MODEL_PROPERTY, NULL);
	}

	if (update_all || (IS_GUIDE_MODE_ON(prev_glst) != IS_GUIDE_MODE_ON(PRIVATE_DATA->glst)) ||
	   (GUIDE_MODE_PROPERTY->state == INDIGO_BUSY_STATE)) {
		GUIDE_MODE_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_GUIDE_MODE_ON(PRIVATE_DATA->glst)) {
			GUIDE_MODE_ON_ITEM->sw.value = true;
			GUIDE_MODE_OFF_ITEM->sw.value = false;
		} else {
			GUIDE_MODE_ON_ITEM->sw.value = false;
			GUIDE_MODE_OFF_ITEM->sw.value = true;
		}
		indigo_update_property(device, GUIDE_MODE_PROPERTY, NULL);
	}

	if (update_all || (prev_glst.flap_tube_state != PRIVATE_DATA->glst.flap_tube_state) ||
	   (prev_glst.flap_coude_state != PRIVATE_DATA->glst.flap_coude_state)) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Updating FLAP_STATE_PROPERTY (dev = %d)", PRIVATE_DATA->dev_id);
		FLAP_STATE_PROPERTY->state = INDIGO_OK_STATE;
		ascol_get_flap_tube_state(PRIVATE_DATA->glst, &descr, &descrs);
		snprintf(TUBE_FLAP_STATE_ITEM->text.value, INDIGO_VALUE_SIZE, "%s - %s", descrs, descr);
		ascol_get_flap_coude_state(PRIVATE_DATA->glst, &descr, &descrs);
		snprintf(COUDE_FLAP_STATE_ITEM->text.value, INDIGO_VALUE_SIZE, "%s - %s", descrs, descr);
		indigo_update_property(device, FLAP_STATE_PROPERTY, NULL);
	}

	if (update_all || (prev_glst.flap_tube_state != PRIVATE_DATA->glst.flap_tube_state) ||
	   (FLAP_TUBE_PROPERTY->state == INDIGO_BUSY_STATE)) {
		if (PRIVATE_DATA->glst.flap_tube_state == SF_STATE_OPEN) {
			FLAP_TUBE_OPEN_ITEM->sw.value = true;
			FLAP_TUBE_CLOSE_ITEM->sw.value = false;
			FLAP_TUBE_PROPERTY->state = INDIGO_OK_STATE;
		} else if(PRIVATE_DATA->glst.flap_tube_state == SF_STATE_CLOSE) {
			FLAP_TUBE_OPEN_ITEM->sw.value = false;
			FLAP_TUBE_CLOSE_ITEM->sw.value = true;
			FLAP_TUBE_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			FLAP_COUDE_PROPERTY->state = INDIGO_BUSY_STATE;
		}
		indigo_update_property(device, FLAP_TUBE_PROPERTY, NULL);
	}

	if (update_all || (prev_glst.flap_coude_state != PRIVATE_DATA->glst.flap_coude_state) ||
	   (FLAP_COUDE_PROPERTY->state == INDIGO_BUSY_STATE)) {
		if (PRIVATE_DATA->glst.flap_coude_state == SF_STATE_OPEN) {
			FLAP_COUDE_OPEN_ITEM->sw.value = true;
			FLAP_COUDE_CLOSE_ITEM->sw.value = false;
			FLAP_COUDE_PROPERTY->state = INDIGO_OK_STATE;
		} else if(PRIVATE_DATA->glst.flap_coude_state == SF_STATE_CLOSE) {
			FLAP_COUDE_OPEN_ITEM->sw.value = false;
			FLAP_COUDE_CLOSE_ITEM->sw.value = true;
			FLAP_COUDE_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			FLAP_COUDE_PROPERTY->state = INDIGO_BUSY_STATE;
		}
		indigo_update_property(device, FLAP_COUDE_PROPERTY, NULL);
	}

	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	int res = ascol_OIMV(PRIVATE_DATA->dev_id, &PRIVATE_DATA->oimv);
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	if (res != ASCOL_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_OIMV(%d) = %d", PRIVATE_DATA->dev_id, res);
		OIMV_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, OIMV_PROPERTY, "Could not read oil sensrs");
	} else if (update_all || memcmp(&prev_oimv, &PRIVATE_DATA->oimv, sizeof(prev_oimv))) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Updating OIMV_PROPERTY (dev = %d)", PRIVATE_DATA->dev_id);
		OIMV_PROPERTY->state = INDIGO_OK_STATE;
		for (int index = 0; index < ASCOL_OIMV_N; index++) {
			OIMV_ITEMS(index)->number.value = PRIVATE_DATA->oimv.value[index];
		}
		indigo_update_property(device, OIMV_PROPERTY, NULL);
		prev_oimv = PRIVATE_DATA->oimv;
	}

	char west;
	double ra, ha, dec;
	if ((PRIVATE_DATA->glst.telescope_state >= TE_STATE_INIT) &&
	    (PRIVATE_DATA->glst.telescope_state <= TE_STATE_OFF_REQ)) {
		if (MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state != INDIGO_ALERT_STATE) {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		}
		if (HADEC_COORDINATES_PROPERTY->state != INDIGO_ALERT_STATE) {
			HADEC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		}
		if (HADEC_RELATIVE_MOVE_PROPERTY->state != INDIGO_ALERT_STATE) {
			HADEC_RELATIVE_MOVE_PROPERTY->state = INDIGO_OK_STATE;
		}
		if (RADEC_RELATIVE_MOVE_PROPERTY->state != INDIGO_ALERT_STATE) {
			RADEC_RELATIVE_MOVE_PROPERTY->state = INDIGO_OK_STATE;
		}
	} else {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
		HADEC_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
		HADEC_RELATIVE_MOVE_PROPERTY->state = INDIGO_BUSY_STATE;
		RADEC_RELATIVE_MOVE_PROPERTY->state = INDIGO_BUSY_STATE;
	}
	indigo_update_property(device, HADEC_RELATIVE_MOVE_PROPERTY, NULL);
	indigo_update_property(device, RADEC_RELATIVE_MOVE_PROPERTY, NULL);

	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	res = ascol_TRRD(PRIVATE_DATA->dev_id, &ra, &dec, &west);
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	if (res != ASCOL_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_TRRD(%d) = %d", PRIVATE_DATA->dev_id, res);
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_coordinates(device, NULL);
		indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, NULL);
	} else {
		MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = d2h(ra);
		MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = dec;
		indigo_update_coordinates(device, NULL);
		indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, NULL);
	}

	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	res = ascol_TRHD(PRIVATE_DATA->dev_id, &ha, &dec);
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	if (res != ASCOL_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_TRHD(%d) = %d", PRIVATE_DATA->dev_id, res);
		HADEC_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_coordinates(device, NULL);
		indigo_update_property(device, HADEC_COORDINATES_PROPERTY, NULL);
	} else {
		HADEC_COORDINATES_HA_ITEM->number.value = ha;
		HADEC_COORDINATES_DEC_ITEM->number.value = dec;
		indigo_update_coordinates(device, NULL);
		indigo_update_property(device, HADEC_COORDINATES_PROPERTY, NULL);
	}

	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	res = ascol_TRUS(PRIVATE_DATA->dev_id, &ra, &dec);
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	if (res != ASCOL_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_TRUS(%d) = %d", PRIVATE_DATA->dev_id, res);
		USER_SPEED_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, USER_SPEED_PROPERTY, NULL);
	} else if ((USER_SPEED_RA_ITEM->number.value != ra) || (USER_SPEED_DEC_ITEM->number.value != dec)){
		USER_SPEED_RA_ITEM->number.value = ra;
		USER_SPEED_DEC_ITEM->number.value = dec;
		indigo_update_property(device, USER_SPEED_PROPERTY, NULL);
	}

	double speed;
	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	res = ascol_TRS1(PRIVATE_DATA->dev_id, &speed);
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	if (res != ASCOL_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_TRS1(%d) = %d", PRIVATE_DATA->dev_id, res);
		T1_SPEED_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, T1_SPEED_PROPERTY, NULL);
	} else if (T1_SPEED_ITEM->number.value != speed) {
		T1_SPEED_ITEM->number.value = speed;
		indigo_update_property(device, T1_SPEED_PROPERTY, NULL);
	}

	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	res = ascol_TRS2(PRIVATE_DATA->dev_id, &speed);
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	if (res != ASCOL_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_TRS2(%d) = %d", PRIVATE_DATA->dev_id, res);
		T2_SPEED_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, T2_SPEED_PROPERTY, NULL);
	} else if (T2_SPEED_ITEM->number.value != speed) {
		T1_SPEED_ITEM->number.value = speed;
		indigo_update_property(device, T2_SPEED_PROPERTY, NULL);
	}

	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	res = ascol_TRS3(PRIVATE_DATA->dev_id, &speed);
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	if (res != ASCOL_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_TRS3(%d) = %d", PRIVATE_DATA->dev_id, res);
		T3_SPEED_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, T3_SPEED_PROPERTY, NULL);
	} else if (T3_SPEED_ITEM->number.value != speed) {
		T3_SPEED_ITEM->number.value = speed;
		indigo_update_property(device, T3_SPEED_PROPERTY, NULL);
	}

	if (PRIVATE_DATA->park_update || (MOUNT_PARK_PROPERTY->state == INDIGO_BUSY_STATE)) {
		if ((round(HADEC_COORDINATES_HA_ITEM->number.value*100) == -9000) &&
		   (round(HADEC_COORDINATES_DEC_ITEM->number.value*100) == 8999) &&
		   (PRIVATE_DATA->glst.telescope_state == TE_STATE_STOP)) {
			MOUNT_PARK_PARKED_ITEM->sw.value = true;
			MOUNT_PARK_UNPARKED_ITEM->sw.value = false;
			MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
			PRIVATE_DATA->parked = true;
		} else {
			MOUNT_PARK_PARKED_ITEM->sw.value = false;
			MOUNT_PARK_UNPARKED_ITEM->sw.value = true;
			indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
			PRIVATE_DATA->parked = false;
		}
		PRIVATE_DATA->park_update = false;
	}

	/* should be copied every time as there are several properties
	   relaying on this and we have no track which one changed */
	prev_glst = PRIVATE_DATA->glst;
	update_all = false;
}


static indigo_result mount_attach(indigo_device *device) {
	int index = 0;
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_mount_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		pthread_mutex_init(&PRIVATE_DATA->net_mutex, NULL);
		// -------------------------------------------------------------------------------- SIMULATION
		SIMULATION_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- MOUNT_ON_COORDINATES_SET
		MOUNT_ON_COORDINATES_SET_PROPERTY->hidden = false;
		MOUNT_ON_COORDINATES_SET_PROPERTY->count = 1;
		// -------------------------------------------------------------------------------- AUTHENTICATION
		AUTHENTICATION_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- DEVICE_PORT
		DEVICE_PORT_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- DEVICE_PORTS
		DEVICE_PORTS_PROPERTY->hidden = true;
		// --------------------------------------------------------------------------------
		MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->hidden = false;
		//MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->count = 3;

		MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.max=89.99999;
		MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value=89.99999;
		MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.min=-89.99999;
		MOUNT_UTC_TIME_PROPERTY->hidden = true;

		MOUNT_PARK_PROPERTY->hidden = false;
		PRIVATE_DATA->park_update = true;

		MOUNT_SIDE_OF_PIER_PROPERTY->hidden = false;
		MOUNT_MOTION_DEC_PROPERTY->hidden = true;
		MOUNT_MOTION_RA_PROPERTY->hidden = true;
		//MOUNT_UTC_TIME_PROPERTY->count = 1;
		//MOUNT_UTC_TIME_PROPERTY->perm = INDIGO_RO_PERM;
		MOUNT_INFO_PROPERTY->hidden = true;
		MOUNT_SET_HOST_TIME_PROPERTY->hidden = true;
		MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
		MOUNT_TRACK_RATE_PROPERTY->hidden = true;
		//indigo_copy_name(MOUNT_TRACKING_PROPERTY->group, SWITCHES_GROUP);
		MOUNT_SLEW_RATE_PROPERTY->hidden = true;
		MOUNT_SNOOP_DEVICES_PROPERTY->hidden = true;
		// --------------------------------------------------------------------------- OIL STATE
		OIL_STATE_PROPERTY = indigo_init_text_property(NULL, device->name, OIL_STATE_PROPERTY_NAME, OIL_GROUP, "Oil State", INDIGO_IDLE_STATE, INDIGO_RO_PERM, 1);
		if (OIL_STATE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_text_item(OIL_STATE_ITEM, OIL_STATE_ITEM_NAME, "State", "");
		// --------------------------------------------------------------------------- FLAP STATE
		FLAP_STATE_PROPERTY = indigo_init_text_property(NULL, device->name, FLAP_STATE_PROPERTY_NAME, FLAPS_GROUP, "Flaps State", INDIGO_IDLE_STATE, INDIGO_RO_PERM, 2);
		if (FLAP_STATE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_text_item(TUBE_FLAP_STATE_ITEM, TUBE_FLAP_STATE_ITEM_NAME, "Tube Flap", "");
		indigo_init_text_item(COUDE_FLAP_STATE_ITEM, COUDE_FLAP_STATE_ITEM_NAME, "Coude Flap", "");

		char item_name[INDIGO_NAME_SIZE];
		char item_label[INDIGO_NAME_SIZE];
		// -------------------------------------------------------------------------- FLAP_TUBE
		FLAP_TUBE_PROPERTY = indigo_init_switch_property(NULL, device->name, FLAP_TUBE_PROPERTY_NAME, FLAPS_GROUP, "Tube Flap", INDIGO_BUSY_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (FLAP_TUBE_PROPERTY == NULL)
			return INDIGO_FAILED;

		indigo_init_switch_item(FLAP_TUBE_OPEN_ITEM, FLAP_TUBE_OPEN_ITEM_NAME, "Open", false);
		indigo_init_switch_item(FLAP_TUBE_CLOSE_ITEM, FLAP_TUBE_CLOSE_ITEM_NAME, "Close", true);
		// -------------------------------------------------------------------------- FLAP_COUDE
		FLAP_COUDE_PROPERTY = indigo_init_switch_property(NULL, device->name, FLAP_COUDE_PROPERTY_NAME, FLAPS_GROUP, "Coude Flap", INDIGO_BUSY_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (FLAP_COUDE_PROPERTY == NULL)
			return INDIGO_FAILED;

		indigo_init_switch_item(FLAP_COUDE_OPEN_ITEM, FLAP_COUDE_OPEN_ITEM_NAME, "Open", false);
		indigo_init_switch_item(FLAP_COUDE_CLOSE_ITEM, FLAP_COUDE_CLOSE_ITEM_NAME, "Close", true);
		// --------------------------------------------------------------------------- OIMV
		OIMV_PROPERTY = indigo_init_number_property(NULL, device->name, OIMV_PROPERTY_NAME, OIL_GROUP, "Oil Sesors", INDIGO_OK_STATE, INDIGO_RO_PERM, ASCOL_OIMV_N);
		if (OIMV_PROPERTY == NULL)
			return INDIGO_FAILED;

		ascol_OIMV(ASCOL_DESCRIBE, &PRIVATE_DATA->oimv);
		for (index = 0; index < ASCOL_OIMV_N; index++) {
			snprintf(item_name, INDIGO_NAME_SIZE, "%s_%d", OIMV_ITEM_NAME_BASE, index);
			snprintf(item_label, INDIGO_NAME_SIZE, "%s (%s)",
			         PRIVATE_DATA->oimv.description[index],
			         PRIVATE_DATA->oimv.unit[index]
			);
			indigo_init_number_item(OIMV_ITEMS(index), item_name, item_label, -1000, 1000, 0.01, 0);
		}
		// -------------------------------------------------------------------------- OIL_POWER
		OIL_POWER_PROPERTY = indigo_init_switch_property(NULL, device->name, OIL_POWER_PROPERTY_NAME, OIL_GROUP, "Oil Power", INDIGO_BUSY_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (OIL_POWER_PROPERTY == NULL)
			return INDIGO_FAILED;

		indigo_init_switch_item(OIL_ON_ITEM, OIL_ON_ITEM_NAME, "On", false);
		indigo_init_switch_item(OIL_OFF_ITEM, OIL_OFF_ITEM_NAME, "Off", true);
		// -------------------------------------------------------------------------- AXIS_CALIBRATED
		AXIS_CALIBRATED_PROPERTY = indigo_init_light_property(NULL, device->name, AXIS_CALIBRATED_PROPERTY_NAME, CORRECTIONS_GROUP, "Axis Calibrated", INDIGO_IDLE_STATE, 2);
		if (AXIS_CALIBRATED_PROPERTY == NULL)
			return INDIGO_FAILED;

		indigo_init_light_item(RA_CALIBRATED_ITEM, RA_CALIBRATED_ITEM_NAME, "RA Axis", INDIGO_IDLE_STATE);
		indigo_init_light_item(DEC_CALIBRATED_ITEM, DEC_CALIBRATED_ITEM_NAME, "DEC Axis", INDIGO_IDLE_STATE);
		// -------------------------------------------------------------------------- RA_CALIBRATION
		RA_CALIBRATION_PROPERTY = indigo_init_switch_property(NULL, device->name, RA_CALIBRATION_PROPERTY_NAME, CORRECTIONS_GROUP, "RA Calibration", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 2);
		if (RA_CALIBRATION_PROPERTY == NULL)
			return INDIGO_FAILED;

		indigo_init_switch_item(RA_CALIBRATION_START_ITEM, RA_CALIBRATION_START_ITEM_NAME, "Start", false);
		indigo_init_switch_item(RA_CALIBRATION_STOP_ITEM, RA_CALIBRATION_STOP_ITEM_NAME, "Stop", false);
		// -------------------------------------------------------------------------- DEC_CALIBRATION
		DEC_CALIBRATION_PROPERTY = indigo_init_switch_property(NULL, device->name, DEC_CALIBRATION_PROPERTY_NAME, CORRECTIONS_GROUP, "DEC Calibration", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 2);
		if (DEC_CALIBRATION_PROPERTY == NULL)
			return INDIGO_FAILED;

		indigo_init_switch_item(DEC_CALIBRATION_START_ITEM, DEC_CALIBRATION_START_ITEM_NAME, "Start", false);
		indigo_init_switch_item(DEC_CALIBRATION_STOP_ITEM, DEC_CALIBRATION_STOP_ITEM_NAME, "Stop", false);
		// -------------------------------------------------------------------------- ABERRATION_CORRECTION
		ABERRATION_PROPERTY = indigo_init_switch_property(NULL, device->name, ABERRATION_PROPERTY_NAME, CORRECTIONS_GROUP, "Aberration Correction", INDIGO_BUSY_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (ABERRATION_PROPERTY == NULL)
			return INDIGO_FAILED;

		indigo_init_switch_item(ABERRATION_ON_ITEM, ABERRATION_ON_ITEM_NAME, "On", false);
		indigo_init_switch_item(ABERRATION_OFF_ITEM, ABERRATION_OFF_ITEM_NAME, "Off", true);
		// -------------------------------------------------------------------------- PRECESSION_CORRECTION
		PRECESSION_PROPERTY = indigo_init_switch_property(NULL, device->name, PRECESSION_PROPERTY_NAME, CORRECTIONS_GROUP, "Precession and Nutation Correction", INDIGO_BUSY_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (PRECESSION_PROPERTY == NULL)
			return INDIGO_FAILED;

		indigo_init_switch_item(PRECESSION_ON_ITEM, PRECESSION_ON_ITEM_NAME, "On", false);
		indigo_init_switch_item(PRECESSION_OFF_ITEM, PRECESSION_OFF_ITEM_NAME, "Off", true);
		// -------------------------------------------------------------------------- REFRACTION_CORRECTION
		REFRACTION_PROPERTY = indigo_init_switch_property(NULL, device->name, REFRACTION_PROPERTY_NAME, CORRECTIONS_GROUP, "Refraction Correction", INDIGO_BUSY_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (REFRACTION_PROPERTY == NULL)
			return INDIGO_FAILED;

		indigo_init_switch_item(REFRACTION_ON_ITEM, REFRACTION_ON_ITEM_NAME, "On", false);
		indigo_init_switch_item(REFRACTION_OFF_ITEM, REFRACTION_OFF_ITEM_NAME, "Off", true);
		// -------------------------------------------------------------------------- ERROR_CORRECTION
		ERROR_CORRECTION_PROPERTY = indigo_init_switch_property(NULL, device->name, ERROR_CORRECTION_PROPERTY_NAME, CORRECTIONS_GROUP, "Error Correction", INDIGO_BUSY_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (ERROR_CORRECTION_PROPERTY == NULL)
			return INDIGO_FAILED;

		indigo_init_switch_item(ERROR_CORRECTION_ON_ITEM, ERROR_CORRECTION_ON_ITEM_NAME, "On", false);
		indigo_init_switch_item(ERROR_CORRECTION_OFF_ITEM, ERROR_CORRECTION_OFF_ITEM_NAME, "Off", true);
		// -------------------------------------------------------------------------- CORRECTION_MODEL
		CORRECTION_MODEL_PROPERTY = indigo_init_number_property(NULL, device->name, CORRECTION_MODEL_PROPERTY_NAME, CORRECTIONS_GROUP, "Correction Model", INDIGO_BUSY_STATE, INDIGO_RW_PERM, 1);
		if (CORRECTION_MODEL_PROPERTY == NULL)
			return INDIGO_FAILED;

		indigo_init_number_item(CORRECTION_MODEL_INDEX_ITEM, CORRECTION_MODEL_INDEX_ITEM_NAME, "Index", 0, 4, 1, 0);
		// -------------------------------------------------------------------------- GUIDE_MODE
		GUIDE_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, GUIDE_MODE_PROPERTY_NAME, CORRECTIONS_GROUP, "Guide Mode", INDIGO_BUSY_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (GUIDE_MODE_PROPERTY == NULL)
			return INDIGO_FAILED;

		indigo_init_switch_item(GUIDE_MODE_ON_ITEM, GUIDE_MODE_ON_ITEM_NAME, "On", false);
		indigo_init_switch_item(GUIDE_MODE_OFF_ITEM, GUIDE_MODE_OFF_ITEM_NAME, "Off", true);
		// --------------------------------------------------------------------------- MOUNT STATE
		MOUNT_STATE_PROPERTY = indigo_init_text_property(NULL, device->name, MOUNT_STATE_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Mount State", INDIGO_IDLE_STATE, INDIGO_RO_PERM, 3);
		if (MOUNT_STATE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_text_item(MOUNT_STATE_ITEM, MOUNT_STATE_ITEM_NAME, "Mount", "");
		indigo_init_text_item(RA_STATE_ITEM, RA_STATE_ITEM_NAME, "RA Axis", "");
		indigo_init_text_item(DEC_STATE_ITEM, DEC_STATE_ITEM_NAME, "DEC Axis", "");
		// -------------------------------------------------------------------------- TELESCOPE_POWER
		TELESCOPE_POWER_PROPERTY = indigo_init_switch_property(NULL, device->name, TELESCOPE_POWER_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Telescope Power", INDIGO_BUSY_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (TELESCOPE_POWER_PROPERTY == NULL)
			return INDIGO_FAILED;

		indigo_init_switch_item(TELESCOPE_ON_ITEM, TELESCOPE_ON_ITEM_NAME, "On", false);
		indigo_init_switch_item(TELESCOPE_OFF_ITEM, TELESCOPE_OFF_ITEM_NAME, "Off", true);
		// -------------------------------------------------------------------------- HADEC_COORDINATES
		HADEC_COORDINATES_PROPERTY = indigo_init_number_property(NULL, device->name, HADEC_COORDINATES_PROPERTY_NAME, MOUNT_MAIN_GROUP, "HA DEC Coordinates", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (HADEC_COORDINATES_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_sexagesimal_number_item(HADEC_COORDINATES_HA_ITEM, HADEC_COORDINATES_HA_ITEM_NAME, "Hour Angle (-180° to 330°)", -180, 330, 0.0001, 0);
		indigo_init_sexagesimal_number_item(HADEC_COORDINATES_DEC_ITEM, HADEC_COORDINATES_DEC_ITEM_NAME, "Declination (-90° to 90°)", -90, 270, 0.0001, 0);
		// -------------------------------------------------------------------------- HADEC_RELATIVE_MOVE
		HADEC_RELATIVE_MOVE_PROPERTY = indigo_init_number_property(NULL, device->name, HADEC_RELATIVE_MOVE_PROPERTY_NAME, MOUNT_MAIN_GROUP, "HA DEC Relative Move", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (HADEC_RELATIVE_MOVE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(HADEC_RELATIVE_MOVE_HA_ITEM, HADEC_RELATIVE_MOVE_HA_ITEM_NAME, "Hour Angle (-36000\" to 36000\")", -36000, 36000, 0.01, 0);
		indigo_init_number_item(HADEC_RELATIVE_MOVE_DEC_ITEM, HADEC_RELATIVE_MOVE_DEC_ITEM_NAME, "Declination (-36000\" to 36000\")", -36000, 36000, 0.01, 0);
		// -------------------------------------------------------------------------- RADEC_RELATIVE_MOVE
		RADEC_RELATIVE_MOVE_PROPERTY = indigo_init_number_property(NULL, device->name, RADEC_RELATIVE_MOVE_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Relative Move", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (RADEC_RELATIVE_MOVE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(RADEC_RELATIVE_MOVE_RA_ITEM, RADEC_RELATIVE_MOVE_RA_ITEM_NAME, "Right Ascension (-36000\" to 36000\")", -36000, 36000, 0.01, 0);
		indigo_init_number_item(RADEC_RELATIVE_MOVE_DEC_ITEM, RADEC_RELATIVE_MOVE_DEC_ITEM_NAME, "Declination (-36000\" to 36000\")", -36000, 36000, 0.01, 0);
		// -------------------------------------------------------------------------- USER_SPEED
		USER_SPEED_PROPERTY = indigo_init_number_property(NULL, device->name, USER_SPEED_PROPERTY_NAME, SPEEDS_GROUP, "User Speed", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (USER_SPEED_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(USER_SPEED_RA_ITEM, USER_SPEED_RA_ITEM_NAME, "Right Ascension (-10.0\"/s to 10.0\"/s)", -10, 10, 0.0001, 0);
		indigo_init_number_item(USER_SPEED_DEC_ITEM, USER_SPEED_DEC_ITEM_NAME, "Declination (-10.0\"/s to 10.0\"/s)", -10, 10, 0.0001, 0);
		// -------------------------------------------------------------------------- T1_SPEED
		T1_SPEED_PROPERTY = indigo_init_number_property(NULL, device->name, T1_SPEED_PROPERTY_NAME, SPEEDS_GROUP, "T1 Speed", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (T1_SPEED_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(T1_SPEED_ITEM, T1_SPEED_ITEM_NAME, "Rate (100.0\"/s to 5000.0\"/s)", 100, 5000, 0.01, 0);
		// -------------------------------------------------------------------------- T2_SPEED
		T2_SPEED_PROPERTY = indigo_init_number_property(NULL, device->name, T2_SPEED_PROPERTY_NAME, SPEEDS_GROUP, "T2 Speed", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (T2_SPEED_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(T2_SPEED_ITEM, T2_SPEED_ITEM_NAME, "Rate (1.0\"/s to 120.0\"/s)", 1, 120, 0.01, 0);
		// -------------------------------------------------------------------------- T3_SPEED
		T3_SPEED_PROPERTY = indigo_init_number_property(NULL, device->name, T3_SPEED_PROPERTY_NAME, SPEEDS_GROUP, "T3 Speed", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (T3_SPEED_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(T3_SPEED_ITEM, T3_SPEED_ITEM_NAME, "Rate (1.0\"/s to 120.0\"/s)", 1, 120, 0.01, 0);
		// --------------------------------------------------------------------------

		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_mount_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}


static indigo_result mount_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	// -------------------------------------------------------------------------------- CONNECTION
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			if (!device->is_connected) {
				if (ascol_device_open(device)) {
					CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
					indigo_define_property(device, OIL_POWER_PROPERTY, NULL);
					indigo_define_property(device, OIL_STATE_PROPERTY, NULL);
					indigo_define_property(device, OIMV_PROPERTY, NULL);
					indigo_define_property(device, MOUNT_STATE_PROPERTY, NULL);
					indigo_define_property(device, FLAP_STATE_PROPERTY, NULL);
					indigo_define_property(device, FLAP_TUBE_PROPERTY, NULL);
					indigo_define_property(device, FLAP_COUDE_PROPERTY, NULL);
					indigo_define_property(device, TELESCOPE_POWER_PROPERTY, NULL);
					indigo_define_property(device, AXIS_CALIBRATED_PROPERTY, NULL);
					indigo_define_property(device, RA_CALIBRATION_PROPERTY, NULL);
					indigo_define_property(device, DEC_CALIBRATION_PROPERTY, NULL);
					indigo_define_property(device, ABERRATION_PROPERTY, NULL);
					indigo_define_property(device, PRECESSION_PROPERTY, NULL);
					indigo_define_property(device, REFRACTION_PROPERTY, NULL);
					indigo_define_property(device, ERROR_CORRECTION_PROPERTY, NULL);
					indigo_define_property(device, CORRECTION_MODEL_PROPERTY, NULL);
					indigo_define_property(device, GUIDE_MODE_PROPERTY, NULL);
					indigo_define_property(device, HADEC_COORDINATES_PROPERTY, NULL);
					indigo_define_property(device, HADEC_RELATIVE_MOVE_PROPERTY, NULL);
					indigo_define_property(device, RADEC_RELATIVE_MOVE_PROPERTY, NULL);
					indigo_define_property(device, USER_SPEED_PROPERTY, NULL);
					indigo_define_property(device, T1_SPEED_PROPERTY, NULL);
					indigo_define_property(device, T2_SPEED_PROPERTY, NULL);
					indigo_define_property(device, T3_SPEED_PROPERTY, NULL);

					device->is_connected = true;
				} else {
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
				}
			}
		} else {
			if (device->is_connected) {
				ascol_device_close(device);
				indigo_delete_property(device, OIL_STATE_PROPERTY, NULL);
				indigo_delete_property(device, OIMV_PROPERTY, NULL);
				indigo_delete_property(device, MOUNT_STATE_PROPERTY, NULL);
				indigo_delete_property(device, FLAP_STATE_PROPERTY, NULL);
				indigo_delete_property(device, FLAP_TUBE_PROPERTY, NULL);
				indigo_delete_property(device, FLAP_COUDE_PROPERTY, NULL);
				indigo_delete_property(device, OIL_POWER_PROPERTY, NULL);
				indigo_delete_property(device, TELESCOPE_POWER_PROPERTY, NULL);
				indigo_delete_property(device, AXIS_CALIBRATED_PROPERTY, NULL);
				indigo_delete_property(device, RA_CALIBRATION_PROPERTY, NULL);
				indigo_delete_property(device, DEC_CALIBRATION_PROPERTY, NULL);
				indigo_delete_property(device, ABERRATION_PROPERTY, NULL);
				indigo_delete_property(device, PRECESSION_PROPERTY, NULL);
				indigo_delete_property(device, REFRACTION_PROPERTY, NULL);
				indigo_delete_property(device, ERROR_CORRECTION_PROPERTY, NULL);
				indigo_delete_property(device, CORRECTION_MODEL_PROPERTY, NULL);
				indigo_delete_property(device, GUIDE_MODE_PROPERTY, NULL);
				indigo_delete_property(device, HADEC_COORDINATES_PROPERTY, NULL);
				indigo_delete_property(device, HADEC_RELATIVE_MOVE_PROPERTY, NULL);
				indigo_delete_property(device, RADEC_RELATIVE_MOVE_PROPERTY, NULL);
				indigo_delete_property(device, USER_SPEED_PROPERTY, NULL);
				indigo_delete_property(device, T1_SPEED_PROPERTY, NULL);
				indigo_delete_property(device, T2_SPEED_PROPERTY, NULL);
				indigo_delete_property(device, T3_SPEED_PROPERTY, NULL);
				device->is_connected = false;
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			}
		}
	} else if (indigo_property_match(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_GEOGRAPTHIC_COORDINATES
		if (IS_CONNECTED) {
			indigo_property_copy_values(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, property, false);
			if (MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value < 0)
				MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value += 360;
			MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(OIL_POWER_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- OIL_POWER_PROPERTY
		if (IS_CONNECTED) {
			indigo_property_copy_values(OIL_POWER_PROPERTY, property, false);
			mount_handle_oil_power(device);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(TELESCOPE_POWER_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- TELESCOPE_POWER_PROPERTY
		if (IS_CONNECTED) {
			indigo_property_copy_values(TELESCOPE_POWER_PROPERTY, property, false);
			mount_handle_telescope_power(device);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(RA_CALIBRATION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- RA_CALIBRATION_PROPERTY
		if (IS_CONNECTED) {
			indigo_property_copy_values(RA_CALIBRATION_PROPERTY, property, false);
			mount_handle_ra_calibration(device);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(DEC_CALIBRATION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DEC_CALIBRATION_PROPERTY
		if (IS_CONNECTED) {
			indigo_property_copy_values(DEC_CALIBRATION_PROPERTY, property, false);
			mount_handle_dec_calibration(device);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(ABERRATION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- ABERRATION_PROPERTY
		if (IS_CONNECTED) {
			indigo_property_copy_values(ABERRATION_PROPERTY, property, false);
			mount_handle_aberration(device);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(PRECESSION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- PRECESSION_PROPERTY
		if (IS_CONNECTED) {
			indigo_property_copy_values(PRECESSION_PROPERTY, property, false);
			mount_handle_precession(device);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(REFRACTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- REFRACTION_PROPERTY
		if (IS_CONNECTED) {
			indigo_property_copy_values(REFRACTION_PROPERTY, property, false);
			mount_handle_refraction(device);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(ERROR_CORRECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- ERROR_CORRECTION_PROPERTY
		if (IS_CONNECTED) {
			indigo_property_copy_values(ERROR_CORRECTION_PROPERTY, property, false);
			mount_handle_error_correction(device);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(CORRECTION_MODEL_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CORRECTION_MODEL_PROPERTY
		if (IS_CONNECTED) {
			indigo_property_copy_values(CORRECTION_MODEL_PROPERTY, property, false);
			mount_handle_correction_model(device);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDE_MODE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDE_MODE_PROPERTY
		if (IS_CONNECTED) {
			indigo_property_copy_values(GUIDE_MODE_PROPERTY, property, false);
			mount_handle_guide_mode(device);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(FLAP_TUBE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FLAP_TUBE_PROPERTY
		if (IS_CONNECTED) {
			indigo_property_copy_values(FLAP_TUBE_PROPERTY, property, false);
			mount_handle_flap_tube(device);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(FLAP_COUDE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FLAP_COUDE_PROPERTY
		if (IS_CONNECTED) {
			indigo_property_copy_values(FLAP_COUDE_PROPERTY, property, false);
			mount_handle_flap_coude(device);
		}
		return INDIGO_OK;

	} else if (indigo_property_match(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_EQUATORIAL_COORDINATES
		if(PRIVATE_DATA->parked) {
			indigo_update_coordinates(device, WARN_PARKED_MSG);
			return INDIGO_OK;
		}
		double ra = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value;
		double dec = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value;
		indigo_property_copy_values(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property, false);
		MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = ra;
		MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = dec;
		mount_handle_eq_coordinates(device);
		return INDIGO_OK;
	} else if (indigo_property_match(HADEC_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- HADEC_COORDINATES
		if(PRIVATE_DATA->parked) {
			indigo_update_coordinates(device, WARN_PARKED_MSG);
			return INDIGO_OK;
		}
		double ha = HADEC_COORDINATES_HA_ITEM->number.value;
		double dec = HADEC_COORDINATES_DEC_ITEM->number.value;
		indigo_property_copy_values(HADEC_COORDINATES_PROPERTY, property, false);
		HADEC_COORDINATES_HA_ITEM->number.value = ha;
		HADEC_COORDINATES_DEC_ITEM->number.value = dec;
		mount_handle_hadec_coordinates(device);
		return INDIGO_OK;
	} else if (indigo_property_match(HADEC_RELATIVE_MOVE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- HADEC_RELATIVE_MOVE
		if(PRIVATE_DATA->parked) {
			indigo_update_coordinates(device, WARN_PARKED_MSG);
			return INDIGO_OK;
		}
		indigo_property_copy_values(HADEC_RELATIVE_MOVE_PROPERTY, property, false);
		mount_handle_hadec_relative_move(device);
		return INDIGO_OK;
	} else if (indigo_property_match(RADEC_RELATIVE_MOVE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- RADEC_RELATIVE_MOVE
		if(PRIVATE_DATA->parked) {
			indigo_update_coordinates(device, WARN_PARKED_MSG);
			return INDIGO_OK;
		}
		indigo_property_copy_values(RADEC_RELATIVE_MOVE_PROPERTY, property, false);
		mount_handle_radec_relative_move(device);
		return INDIGO_OK;
	} else if (indigo_property_match(USER_SPEED_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- USER_SPEED
		if (IS_CONNECTED) {
			indigo_property_copy_values(USER_SPEED_PROPERTY, property, false);
			mount_handle_user_speed(device);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(T1_SPEED_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- T1_SPEED
		if (IS_CONNECTED) {
			indigo_property_copy_values(T1_SPEED_PROPERTY, property, false);
			mount_handle_t1_speed(device);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(T2_SPEED_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- T2_SPEED
		if (IS_CONNECTED) {
			indigo_property_copy_values(T2_SPEED_PROPERTY, property, false);
			mount_handle_t2_speed(device);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(T3_SPEED_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- T3_SPEED
		if (IS_CONNECTED) {
			indigo_property_copy_values(T3_SPEED_PROPERTY, property, false);
			mount_handle_t3_speed(device);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_TRACKING_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_TRACKING
		//if(PRIVATE_DATA->parked) {
		//	indigo_update_property(device, MOUNT_TRACKING_PROPERTY, WARN_PARKED_MSG);
		//	return INDIGO_OK;
		//}
		indigo_property_copy_values(MOUNT_TRACKING_PROPERTY, property, false);
		mount_handle_tracking(device);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_SLEW_RATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_SLEW_RATE
		if (IS_CONNECTED) {
			indigo_property_copy_values(MOUNT_SLEW_RATE_PROPERTY, property, false);
			//mount_handle_slew_rate(device);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_PARK_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_PARK
		indigo_property_copy_values(MOUNT_PARK_PROPERTY, property, false);
		mount_handle_park(device);
		indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_ABORT_MOTION
		if (IS_CONNECTED) {
			indigo_property_copy_values(MOUNT_ABORT_MOTION_PROPERTY, property, false);
			mount_handle_abort_motion(device);
		}
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_mount_change_property(device, client, property);
}


static indigo_result mount_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);

	indigo_release_property(OIL_STATE_PROPERTY);
	indigo_release_property(OIMV_PROPERTY);
	indigo_release_property(MOUNT_STATE_PROPERTY);
	indigo_release_property(FLAP_STATE_PROPERTY);
	indigo_release_property(FLAP_TUBE_PROPERTY);
	indigo_release_property(FLAP_COUDE_PROPERTY);
	indigo_release_property(OIL_POWER_PROPERTY);
	indigo_release_property(TELESCOPE_POWER_PROPERTY);
	indigo_release_property(AXIS_CALIBRATED_PROPERTY);
	indigo_release_property(RA_CALIBRATION_PROPERTY);
	indigo_release_property(DEC_CALIBRATION_PROPERTY);
	indigo_release_property(ABERRATION_PROPERTY);
	indigo_release_property(PRECESSION_PROPERTY);
	indigo_release_property(REFRACTION_PROPERTY);
	indigo_release_property(ERROR_CORRECTION_PROPERTY);
	indigo_release_property(CORRECTION_MODEL_PROPERTY);
	indigo_release_property(GUIDE_MODE_PROPERTY);
	indigo_release_property(HADEC_COORDINATES_PROPERTY);
	indigo_release_property(HADEC_RELATIVE_MOVE_PROPERTY);
	indigo_release_property(RADEC_RELATIVE_MOVE_PROPERTY);
	indigo_release_property(USER_SPEED_PROPERTY);
	indigo_release_property(T1_SPEED_PROPERTY);
	indigo_release_property(T2_SPEED_PROPERTY);
	indigo_release_property(T3_SPEED_PROPERTY);
	if (PRIVATE_DATA->dev_id > 0) ascol_device_close(device);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_mount_detach(device);
}


// -------------------------------------------------------------------------------- INDIGO guider device implementation

static indigo_result ascol_guider_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(GUIDE_CORRECTION_PROPERTY, property))
			indigo_define_property(device, GUIDE_CORRECTION_PROPERTY, NULL);
	}
	return indigo_guider_enumerate_properties(device, NULL, NULL);
}


static void guider_timer_callback_ra(indigo_device *device) {
	PRIVATE_DATA->guider_timer_ra = NULL;
	GUIDER_GUIDE_EAST_ITEM->number.value = 0;
	GUIDER_GUIDE_WEST_ITEM->number.value = 0;
	GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
}


static void guider_timer_callback_dec(indigo_device *device) {
	PRIVATE_DATA->guider_timer_dec = NULL;
	GUIDER_GUIDE_NORTH_ITEM->number.value = 0;
	GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
	GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
}


static void guider_update_state() {
	indigo_device *device = mount_guider;
	if ((device == NULL) || (!IS_CONNECTED)) return;

	double ra_corr, dec_corr;
	static double prev_ra_corr, prev_dec_corr;
	static bool update_all = true;

	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	int res = ascol_TRGV(PRIVATE_DATA->dev_id, &ra_corr, &dec_corr);
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	if (res != ASCOL_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_TRGV(%d) = %d", PRIVATE_DATA->dev_id, res);
		GUIDE_CORRECTION_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, GUIDE_CORRECTION_PROPERTY, NULL);
		return;
	} else if (update_all || (prev_ra_corr != ra_corr) || (prev_dec_corr != dec_corr) ||
	    GUIDE_CORRECTION_PROPERTY->state == INDIGO_BUSY_STATE) {
		if (!IS_CORRECTION_ON(PRIVATE_DATA->glst)) {
			GUIDE_CORRECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			GUIDE_CORRECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		}
		GUIDE_CORRECTION_RA_ITEM->number.value = ra_corr;
		GUIDE_CORRECTION_DEC_ITEM->number.value = dec_corr;
		indigo_update_property(device, GUIDE_CORRECTION_PROPERTY, NULL);
	}
	prev_ra_corr = ra_corr;
	prev_dec_corr = dec_corr;
	update_all = false;
}


static indigo_result guider_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_guider_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		GUIDER_GUIDE_DEC_PROPERTY->hidden = false;
		GUIDER_GUIDE_RA_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------- GUIDER_RATE
		GUIDER_RATE_PROPERTY->hidden = false;
		GUIDER_RATE_ITEM->number.min = 10;
		GUIDER_RATE_ITEM->number.max = 80;
		// -------------------------------------------------------------------------- GUIDE_CORRECTION
		GUIDE_CORRECTION_PROPERTY = indigo_init_number_property(NULL, device->name, GUIDE_CORRECTION_PROPERTY_NAME, GUIDER_MAIN_GROUP, "Guide Corrections", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (GUIDE_CORRECTION_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(GUIDE_CORRECTION_RA_ITEM, GUIDE_CORRECTION_RA_ITEM_NAME, "RA Correction (-3600\" to 3600\")", -3600, 3600, 0.1, 0);
		indigo_init_number_item(GUIDE_CORRECTION_DEC_ITEM, GUIDE_CORRECTION_DEC_ITEM_NAME, "Dec Correction (-3600\" to 3600\")", -3600, 3600, 0.1, 0);
		// ---------------------------------------------------------------------------
		PRIVATE_DATA->guide_rate = GUIDER_RATE_ITEM->number.value * 0.15; /* 15("/s) * guiderate(%) / 100.0 */
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}


static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	// -------------------------------------------------------------------------------- CONNECTION
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			if (!device->is_connected) {
				if (ascol_device_open(device)) {
					device->is_connected = true;
					PRIVATE_DATA->guider_timer_ra = NULL;
					PRIVATE_DATA->guider_timer_dec = NULL;
					CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
					indigo_define_property(device, GUIDE_CORRECTION_PROPERTY, NULL);
				} else {
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
				}
			}
		} else {
			if (device->is_connected) {
				ascol_device_close(device);
				indigo_delete_property(device, GUIDE_CORRECTION_PROPERTY, NULL);
				device->is_connected = false;
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			}
		}
	} else if (indigo_property_match(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_DEC
		indigo_cancel_timer(device, &PRIVATE_DATA->guider_timer_dec);
		indigo_property_copy_values(GUIDER_GUIDE_DEC_PROPERTY, property, false);
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;

		double ra_gv, dec_gv, new_dec_gv = 0;
		pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
		int res = ascol_TRGV(PRIVATE_DATA->dev_id, &ra_gv, &dec_gv);
		pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
		if (res != ASCOL_OK) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_TRGV(%d) = %d", PRIVATE_DATA->dev_id, res);
		}
		/* convert pulse duration to arcseconds as ASCOL works with arcsec */
		double duration = GUIDER_GUIDE_NORTH_ITEM->number.value / 1000.0;
		if (duration > 0) {
			new_dec_gv = dec_gv + roundf(10.0 * duration * PRIVATE_DATA->guide_rate) / 10.0;
		} else {
			duration = GUIDER_GUIDE_SOUTH_ITEM->number.value / 1000.0;
			if (duration > 0) {
				new_dec_gv = dec_gv - roundf(10.0 * duration * PRIVATE_DATA->guide_rate) /10.0;
			}
		}
		if (duration > 0) {
			pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
			res = ascol_TSGV(PRIVATE_DATA->dev_id, ra_gv, new_dec_gv);
			pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
			if (res != ASCOL_OK) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_TSGV(%d, %f, %f) = %d", PRIVATE_DATA->dev_id, ra_gv, new_dec_gv, res);
			}
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Guide r_ra = %.1f\", r_DEC = %.1f\", pulse = %.3f sec", ra_gv, new_dec_gv, duration);
			GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
			indigo_set_timer(device, duration, guider_timer_callback_dec, &PRIVATE_DATA->guider_timer_dec);
		} else {
			indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDER_GUIDE_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_RA
		indigo_cancel_timer(device, &PRIVATE_DATA->guider_timer_ra);
		indigo_property_copy_values(GUIDER_GUIDE_RA_PROPERTY, property, false);
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
		double ra_gv, dec_gv, new_ra_gv = 0;
		pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
		int res = ascol_TRGV(PRIVATE_DATA->dev_id, &ra_gv, &dec_gv);
		pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
		if (res != ASCOL_OK) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_TRGV(%d) = %d", PRIVATE_DATA->dev_id, res);
		}
		/* convert pulse duration to arcseconds as ASCOL works with arcsec */
		double duration = GUIDER_GUIDE_EAST_ITEM->number.value / 1000.0;
		if (duration > 0) {
			new_ra_gv = ra_gv + roundf(10.0 * duration * PRIVATE_DATA->guide_rate) / 10.0;
		} else {
			duration = GUIDER_GUIDE_WEST_ITEM->number.value / 1000.0;
			if (duration > 0) {
				new_ra_gv = ra_gv - roundf(10.0 * duration * PRIVATE_DATA->guide_rate) / 10.0;
			}
		}
		if (duration > 0) {
			pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
			res = ascol_TSGV(PRIVATE_DATA->dev_id, new_ra_gv, dec_gv);
			pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
			if (res != ASCOL_OK) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_TSGV(%d, %f, %f) = %d", PRIVATE_DATA->dev_id, new_ra_gv, dec_gv, res);
			}
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Guide r_RA = %.1f\", r_dec = %.1f\", pulse = %.3f sec", new_ra_gv, dec_gv, duration);
			GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
			indigo_set_timer(device, duration, guider_timer_callback_ra, &PRIVATE_DATA->guider_timer_dec);
		} else {
			indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDE_CORRECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDE_CORRECTION
		indigo_property_copy_values(GUIDE_CORRECTION_PROPERTY, property, false);
		GUIDE_CORRECTION_PROPERTY->state = INDIGO_OK_STATE;
		pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
		int res = ascol_TSGV(PRIVATE_DATA->dev_id, GUIDE_CORRECTION_RA_ITEM->number.value, GUIDE_CORRECTION_DEC_ITEM->number.value);
		pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
		if (res != ASCOL_OK) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_TSGV(%d) = %d", PRIVATE_DATA->dev_id, res);
		}
		GUIDE_CORRECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDER_RATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_RATE
		indigo_property_copy_values(GUIDER_RATE_PROPERTY, property, false);
		GUIDER_RATE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_RATE_PROPERTY, NULL);
		PRIVATE_DATA->guide_rate = GUIDER_RATE_ITEM->number.value * 0.15; /* 15("/s) * guiderate(%) / 100.0 */
		return INDIGO_OK;
	}
	// --------------------------------------------------------------------------------
	return indigo_guider_change_property(device, client, property);
}

static indigo_result guider_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);

	indigo_release_property(GUIDE_CORRECTION_PROPERTY);

	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_guider_detach(device);
}


// -------------------------------------------------------------------------------- DOME

static void dome_update_state() {
	indigo_device *device = dome;
	if ((device == NULL) || (!IS_CONNECTED)) return;

	static ascol_glst_t prev_glst = {0};
	static bool update_all = true;
	static bool update_horizontal = false;
	static double prev_dome_az = 0;
	char *descrs, *descr;

	if (DOME_ABORT_MOTION_PROPERTY->state == INDIGO_BUSY_STATE) {
		update_all = true;
		DOME_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		DOME_ABORT_MOTION_ITEM->sw.value = false;
		indigo_update_property(device, DOME_ABORT_MOTION_PROPERTY, NULL);
	}

	if (update_all || (prev_glst.dome_state != PRIVATE_DATA->glst.dome_state) ||
	   (DOME_POWER_PROPERTY->state == INDIGO_BUSY_STATE) ||
	   (DOME_SLAVING_PROPERTY->state == INDIGO_BUSY_STATE) ||
	   (DOME_HORIZONTAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE) ||
	   (DOME_STEPS_PROPERTY->state == INDIGO_BUSY_STATE)) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Updating DOME_STATE_PROPERTY (dev = %d)", PRIVATE_DATA->dev_id);
		ascol_get_dome_state(PRIVATE_DATA->glst, &descr, &descrs);
		snprintf(DOME_STATE_ITEM->text.value, INDIGO_VALUE_SIZE, "%s - %s", descrs, descr);
		indigo_update_property(device, DOME_STATE_PROPERTY, NULL);

		if (PRIVATE_DATA->glst.dome_state == DOME_STATE_OFF) {
			DOME_ON_ITEM->sw.value = false;
			DOME_OFF_ITEM->sw.value = true;
			DOME_STATE_PROPERTY->state = INDIGO_OK_STATE;
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			DOME_STEPS_PROPERTY->state = INDIGO_OK_STATE;
			update_horizontal = true;
		} else if ((PRIVATE_DATA->glst.dome_state == DOME_STATE_STOP) ||
		           (PRIVATE_DATA->glst.dome_state == DOME_STATE_AUTO_STOP)) {
			DOME_ON_ITEM->sw.value = true;
			DOME_OFF_ITEM->sw.value = false;
			DOME_STATE_PROPERTY->state = INDIGO_OK_STATE;
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			DOME_STEPS_PROPERTY->state = INDIGO_OK_STATE;
			update_horizontal = true;
		} else {
			DOME_ON_ITEM->sw.value = true;
			DOME_OFF_ITEM->sw.value = false;
			DOME_STATE_PROPERTY->state = INDIGO_BUSY_STATE;
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
			DOME_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		}
		indigo_update_property(device, DOME_STATE_PROPERTY, NULL);

		if (update_all || (DOME_POWER_PROPERTY->state == INDIGO_BUSY_STATE) ||
		   (prev_glst.dome_state != PRIVATE_DATA->glst.dome_state)) {
			DOME_POWER_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, DOME_POWER_PROPERTY, NULL);
		}
		if (update_all || (DOME_SLAVING_PROPERTY->state == INDIGO_BUSY_STATE)) {
			DOME_SLAVING_PROPERTY->state = INDIGO_OK_STATE;
			if ((PRIVATE_DATA->glst.dome_state == DOME_STATE_AUTO_STOP) ||
			    (PRIVATE_DATA->glst.dome_state == DOME_STATE_AUTO_MINUS) ||
			    (PRIVATE_DATA->glst.dome_state == DOME_STATE_AUTO_PLUS)) {
				DOME_SLAVING_ENABLE_ITEM->sw.value = true;
				DOME_SLAVING_DISABLE_ITEM->sw.value = false;
			} else {
				DOME_SLAVING_ENABLE_ITEM->sw.value = false;
				DOME_SLAVING_DISABLE_ITEM->sw.value = true;
			}
			indigo_update_property(device, DOME_SLAVING_PROPERTY, NULL);
		}
	}

	if (update_all || (prev_glst.slit_state != PRIVATE_DATA->glst.slit_state) ||
	   (DOME_SHUTTER_PROPERTY->state == INDIGO_BUSY_STATE)) {
		if (PRIVATE_DATA->glst.slit_state == SF_STATE_OPEN) {
			DOME_SHUTTER_STATE_PROPERTY->state = INDIGO_OK_STATE;
			DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
			DOME_SHUTTER_OPENED_ITEM->sw.value = true;
			DOME_SHUTTER_CLOSED_ITEM->sw.value = false;
		} else if (PRIVATE_DATA->glst.slit_state == SF_STATE_CLOSE) {
			DOME_SHUTTER_STATE_PROPERTY->state = INDIGO_OK_STATE;
			DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
			DOME_SHUTTER_OPENED_ITEM->sw.value = false;
			DOME_SHUTTER_CLOSED_ITEM->sw.value = true;
		} else {
			DOME_SHUTTER_STATE_PROPERTY->state = INDIGO_BUSY_STATE;
			DOME_SHUTTER_PROPERTY->state = INDIGO_BUSY_STATE;
		}
		ascol_get_slit_state(PRIVATE_DATA->glst, &descr, &descrs);
		snprintf(DOME_SHUTTER_STATE_ITEM->text.value, INDIGO_VALUE_SIZE, "%s - %s", descrs, descr);
		indigo_update_property(device, DOME_SHUTTER_STATE_PROPERTY, NULL);
		indigo_update_property(device, DOME_SHUTTER_PROPERTY, NULL);
	}

	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	int res = ascol_DOPO(PRIVATE_DATA->dev_id, &DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value);
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	if (res != ASCOL_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_DOPO(%d) = %d", PRIVATE_DATA->dev_id, res);
		DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
		DOME_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, "Could not read Dome Position");
		indigo_update_property(device, DOME_STEPS_PROPERTY, "Could not read Dome Position");
	} else if (update_all || update_horizontal ||
	          (prev_dome_az != DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value) ||
	          (DOME_HORIZONTAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE) ||
	          (DOME_STEPS_PROPERTY->state == INDIGO_BUSY_STATE)) {
		indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
		indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
		update_horizontal = false;
	}

	/* should be copied every time as there are several properties
	   relaying on this and we have no track which one changed */
	prev_glst = PRIVATE_DATA->glst;

	update_all = false;
	prev_dome_az = DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value;
}


static void dome_handle_power(indigo_device *device) {
	int res = ASCOL_OK;
	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	if (DOME_ON_ITEM->sw.value) {
		res = ascol_DOON(PRIVATE_DATA->dev_id, ASCOL_ON);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_DOON(%d, ASCOL_ON) = %d", PRIVATE_DATA->dev_id, res);
	} else {
		res = ascol_DOON(PRIVATE_DATA->dev_id, ASCOL_OFF);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_DOON(%d, ASCOL_OFF) = %d", PRIVATE_DATA->dev_id, res);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	if(res == ASCOL_OK) {
		DOME_POWER_PROPERTY->state = INDIGO_BUSY_STATE;
	} else {
		DOME_ON_ITEM->sw.value = !DOME_ON_ITEM->sw.value;
		DOME_OFF_ITEM->sw.value = !DOME_OFF_ITEM->sw.value;
		DOME_POWER_PROPERTY->state = INDIGO_ALERT_STATE;
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_DOON(%d) = %d", PRIVATE_DATA->dev_id, res);
	}
	indigo_update_property(device, DOME_POWER_PROPERTY, NULL);
}


static void dome_handle_auto_mode(indigo_device *device) {
	int res = ASCOL_OK;
	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	if (DOME_SLAVING_ENABLE_ITEM->sw.value) {
		res = ascol_DOAM(PRIVATE_DATA->dev_id);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_DOAM(%d) = %d", PRIVATE_DATA->dev_id, res);
	} else {
		res = ascol_DOST(PRIVATE_DATA->dev_id);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_DOST(%d) = %d", PRIVATE_DATA->dev_id, res);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	if(res == ASCOL_OK) {
		DOME_SLAVING_PROPERTY->state = INDIGO_BUSY_STATE;
	} else {
		DOME_SLAVING_ENABLE_ITEM->sw.value = !DOME_SLAVING_ENABLE_ITEM->sw.value;
		DOME_SLAVING_DISABLE_ITEM->sw.value = !DOME_SLAVING_DISABLE_ITEM->sw.value;
		DOME_SLAVING_PROPERTY->state = INDIGO_ALERT_STATE;
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_DOAM(%d) /ascol_DOST() = %d", PRIVATE_DATA->dev_id, res);
	}
	indigo_update_property(device, DOME_SLAVING_PROPERTY, NULL);
}


static void dome_handle_slit(indigo_device *device) {
	int res = ASCOL_OK;
	char *description;
	uint16_t condition;
	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	condition = asocol_check_conditions(PRIVATE_DATA->glst, ASCOL_COND_DOME_ON, &description);
	if (condition) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_DOSO(%d) unmet condition: %s", PRIVATE_DATA->dev_id, description);
		res = ASCOL_COMMAND_ERROR;
	} else {
		if (DOME_SHUTTER_OPENED_ITEM->sw.value) {
			res = ascol_DOSO(PRIVATE_DATA->dev_id, ASCOL_ON);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_DOSO(%d, ASCOL_ON) = %d", PRIVATE_DATA->dev_id, res);
		} else {
			res = ascol_DOSO(PRIVATE_DATA->dev_id, ASCOL_OFF);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_DOSO(%d, ASCOL_OFF) = %d", PRIVATE_DATA->dev_id, res);
		}
	}
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	if(res == ASCOL_OK) {
		DOME_SHUTTER_PROPERTY->state = INDIGO_BUSY_STATE;
	} else {
		DOME_SHUTTER_OPENED_ITEM->sw.value = !DOME_SHUTTER_OPENED_ITEM->sw.value;
		DOME_SHUTTER_CLOSED_ITEM->sw.value = !DOME_SHUTTER_CLOSED_ITEM->sw.value;
		DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
		if (condition) {
			indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Unmet condition: %s", description);
			return;
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_DOSO(%d) = %d", PRIVATE_DATA->dev_id, res);
		}
	}
	indigo_update_property(device, DOME_SHUTTER_PROPERTY, NULL);
}


static void dome_handle_abort(indigo_device *device) {
	int res = ASCOL_OK;
	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	if (DOME_ABORT_MOTION_ITEM->sw.value) {
		res = ascol_DOST(PRIVATE_DATA->dev_id);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_DOST(%d) = %d", PRIVATE_DATA->dev_id, res);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	if(res == ASCOL_OK) {
		DOME_ABORT_MOTION_PROPERTY->state = INDIGO_BUSY_STATE;
	} else {
		DOME_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_DOAM(%d) /ascol_DOST() = %d", PRIVATE_DATA->dev_id, res);
	}
	//DOME_ABORT_MOTION_ITEM->sw.value = false;
	indigo_update_property(device, DOME_ABORT_MOTION_PROPERTY, NULL);
}


static void dome_handle_coordinates(indigo_device *device) {
	int res = INDIGO_OK;
	char *description;
	uint16_t condition;
	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	condition = asocol_check_conditions(PRIVATE_DATA->glst, ASCOL_COND_DOME_ON, &description);
	if (condition) {
		pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_DOGA(%d) unmet condition(s): %s", PRIVATE_DATA->dev_id, description);
		ascol_DOPO(PRIVATE_DATA->dev_id, &DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value);
		DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, "Unmet condition(s): %s", description);
		return;
	}
	DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
	/* GOTO requested */
	res = ascol_DOSA(PRIVATE_DATA->dev_id, DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.target);
	if (res != INDIGO_OK) {
		DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_DOSA(%d) = %d", PRIVATE_DATA->dev_id, res);
	} else {
		res = ascol_DOGA(PRIVATE_DATA->dev_id);
		if (res != INDIGO_OK) {
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_DOGA(%d) = %d", PRIVATE_DATA->dev_id, res);
		}
	}
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
}


static void dome_handle_steps(indigo_device *device) {
	int res = INDIGO_OK;
	int sign = 0;
	char *description;
	uint16_t condition;
	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	condition = asocol_check_conditions(PRIVATE_DATA->glst, ASCOL_COND_DOME_ON, &description);
	if (condition) {
		pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_DOGR(%d) unmet condition(s): %s", PRIVATE_DATA->dev_id, description);
		DOME_STEPS_ITEM->number.value = 0;
		DOME_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, DOME_STEPS_PROPERTY, "Unmet condition(s): %s", description);
		return;
	}
	DOME_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
	/* Dome relative GOTO requested */
	if (DOME_DIRECTION_MOVE_CLOCKWISE_ITEM->sw.value)
		sign = 1; /* move clockwise */
	else if (DOME_DIRECTION_MOVE_COUNTERCLOCKWISE_ITEM->sw.value)
		sign = -1; /* move counterclockwise */
	res = ascol_DOSR(PRIVATE_DATA->dev_id, sign * DOME_STEPS_ITEM->number.target);
	if (res != INDIGO_OK) {
		DOME_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		DOME_STEPS_ITEM->number.value = 0;
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_DOSR(%d) = %d", PRIVATE_DATA->dev_id, res);
	} else {
		res = ascol_DOGR(PRIVATE_DATA->dev_id);
		if (res != INDIGO_OK) {
			DOME_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			DOME_STEPS_ITEM->number.value = 0;
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_DOGR(%d) = %d", PRIVATE_DATA->dev_id, res);
		}
	}
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
}


static indigo_result ascol_dome_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(DOME_POWER_PROPERTY, property))
			indigo_define_property(device, DOME_POWER_PROPERTY, NULL);
		if (indigo_property_match(DOME_STATE_PROPERTY, property))
			indigo_define_property(device, DOME_STATE_PROPERTY, NULL);
		if (indigo_property_match(DOME_SHUTTER_STATE_PROPERTY, property))
			indigo_define_property(device, DOME_SHUTTER_STATE_PROPERTY, NULL);
	}
	return indigo_dome_enumerate_properties(device, NULL, NULL);
}


static indigo_result dome_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_dome_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- DOME_SPEED
		DOME_SPEED_ITEM->number.value = 1;
		// -------------------------------------------------------------------------------- AUTHENTICATION_PROPERTY
		AUTHENTICATION_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- DEVICE_PORT
		DEVICE_PORT_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- DEVICE_PORTS
		DEVICE_PORTS_PROPERTY->hidden = true;
		// --------------------------------------------------------------------------------
		DOME_SNOOP_DEVICES_PROPERTY->hidden = true;
		DOME_EQUATORIAL_COORDINATES_PROPERTY->hidden = true;
		DOME_GEOGRAPHIC_COORDINATES_PROPERTY->hidden = true;
		DOME_DIMENSION_PROPERTY->hidden = true;
		DOME_SPEED_PROPERTY->hidden = true;
		DOME_PARK_PROPERTY->hidden = true;

		// ------------------------------------------------------------------------- DOME_STEPS
		indigo_copy_value(DOME_STEPS_ITEM->label, "Relaive move (0 to 180°)");
		DOME_STEPS_ITEM->number.min = 0;
		DOME_STEPS_ITEM->number.max = 179.99;

		// -------------------------------------------------------------------------- DOME_POWER
		DOME_POWER_PROPERTY = indigo_init_switch_property(NULL, device->name, DOME_POWER_PROPERTY_NAME, DOME_MAIN_GROUP, "Dome Power", INDIGO_BUSY_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (DOME_POWER_PROPERTY == NULL)
			return INDIGO_FAILED;

		indigo_init_switch_item(DOME_ON_ITEM, DOME_ON_ITEM_NAME, "On", false);
		indigo_init_switch_item(DOME_OFF_ITEM, DOME_OFF_ITEM_NAME, "Off", true);
		// --------------------------------------------------------------------------- DOME STATE
		DOME_STATE_PROPERTY = indigo_init_text_property(NULL, device->name, DOME_STATE_PROPERTY_NAME, DOME_MAIN_GROUP, "Dome State", INDIGO_BUSY_STATE, INDIGO_RO_PERM, 1);
		if (DOME_STATE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_text_item(DOME_STATE_ITEM, DOME_STATE_ITEM_NAME, "State", "");
		// --------------------------------------------------------------------------- DOME SHUTTER STATE
		DOME_SHUTTER_STATE_PROPERTY = indigo_init_text_property(NULL, device->name, DOME_SHUTTER_STATE_PROPERTY_NAME, DOME_MAIN_GROUP, "Dome Shutter State", INDIGO_BUSY_STATE, INDIGO_RO_PERM, 1);
		if (DOME_SHUTTER_STATE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_text_item(DOME_SHUTTER_STATE_ITEM, DOME_SHUTTER_STATE_ITEM_NAME, "State", "");
		// --------------------------------------------------------------------------------
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_dome_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}


static indigo_result dome_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			if (!device->is_connected) {
				if (ascol_device_open(device)) {
					CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
					indigo_define_property(device, DOME_POWER_PROPERTY, NULL);
					indigo_define_property(device, DOME_STATE_PROPERTY, NULL);
					indigo_define_property(device, DOME_SHUTTER_STATE_PROPERTY, NULL);
					device->is_connected = true;
				} else {
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
				}
			}
		} else {
			if (device->is_connected) {
				ascol_device_close(device);
				indigo_delete_property(device, DOME_POWER_PROPERTY, NULL);
				indigo_delete_property(device, DOME_STATE_PROPERTY, NULL);
				indigo_delete_property(device, DOME_SHUTTER_STATE_PROPERTY, NULL);
				device->is_connected = false;
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			}
		}
	} else if (indigo_property_match(DOME_POWER_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DOME_POWER_PROPERTY
		if (IS_CONNECTED) {
			indigo_property_copy_values(DOME_POWER_PROPERTY, property, false);
			dome_handle_power(device);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(DOME_SLAVING_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DOME_SLAVING_PROPERTY
		if (IS_CONNECTED) {
			indigo_property_copy_values(DOME_SLAVING_PROPERTY, property, false);
			dome_handle_auto_mode(device);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(DOME_STEPS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DOME_STEPS
		if (IS_CONNECTED) {
			indigo_property_copy_values(DOME_STEPS_PROPERTY, property, false);
			dome_handle_steps(device);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(DOME_HORIZONTAL_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DOME_HORIZONTAL_COORDINATES
		if (IS_CONNECTED) {
			indigo_property_copy_values(DOME_HORIZONTAL_COORDINATES_PROPERTY, property, false);
			dome_handle_coordinates(device);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(DOME_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DOME_ABORT_MOTION
		if (IS_CONNECTED) {
			indigo_property_copy_values(DOME_ABORT_MOTION_PROPERTY, property, false);
			dome_handle_abort(device);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(DOME_SHUTTER_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DOME_SHUTTER
		if (IS_CONNECTED) {
			indigo_property_copy_values(DOME_SHUTTER_PROPERTY, property, false);
			dome_handle_slit(device);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(DOME_PARK_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DOME_PARK
		indigo_property_copy_values(DOME_PARK_PROPERTY, property, false);
		if (DOME_PARK_UNPARKED_ITEM->sw.value) {
			indigo_set_switch(DOME_PARK_PROPERTY, DOME_PARK_UNPARKED_ITEM, true);
			DOME_PARK_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			DOME_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
			if (PRIVATE_DATA->dome_current_position > 180) {
				DOME_DIRECTION_PROPERTY->state = INDIGO_OK_STATE;
				indigo_set_switch(DOME_DIRECTION_PROPERTY, DOME_DIRECTION_MOVE_CLOCKWISE_ITEM, true);
				DOME_STEPS_ITEM->number.value = 360 - PRIVATE_DATA->dome_current_position;
			} else if (PRIVATE_DATA->dome_current_position < 180) {
				DOME_DIRECTION_PROPERTY->state = INDIGO_OK_STATE;
				indigo_set_switch(DOME_DIRECTION_PROPERTY, DOME_DIRECTION_MOVE_COUNTERCLOCKWISE_ITEM, true);
				DOME_STEPS_ITEM->number.value = PRIVATE_DATA->dome_current_position;
			}
			PRIVATE_DATA->dome_target_position = 0;
			DOME_DIRECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, DOME_DIRECTION_PROPERTY, NULL);
			DOME_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
		}
		indigo_update_property(device, DOME_PARK_PROPERTY, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_dome_change_property(device, client, property);
}


static indigo_result dome_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);

	indigo_release_property(DOME_POWER_PROPERTY);
	indigo_release_property(DOME_STATE_PROPERTY);
	indigo_release_property(DOME_SHUTTER_STATE_PROPERTY);

	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_dome_detach(device);
}


// -------------------------------------------------------------------------------- FOCUSER

static void focus_update_state() {
	indigo_device *device = focuser;
	if ((device == NULL) || (!IS_CONNECTED)) return;

	static ascol_glst_t prev_glst = {0};
	static bool update_all = true;
	static bool update_focus = true;
	static double prev_focus_pos = 0;
	char *descrs, *descr;

	if (FOCUSER_ABORT_MOTION_PROPERTY->state == INDIGO_BUSY_STATE) {
		update_all = true;
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
		indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
	}

	if (update_all || (prev_glst.focus_state != PRIVATE_DATA->glst.focus_state) ||
	   (FOCUSER_STATE_PROPERTY->state == INDIGO_BUSY_STATE) ||
	   (FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE)) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Updating FOCUSER_STATE_PROPERTY (dev = %d)", PRIVATE_DATA->dev_id);
		ascol_get_focus_state(PRIVATE_DATA->glst, &descr, &descrs);
		snprintf(FOCUSER_STATE_ITEM->text.value, INDIGO_VALUE_SIZE, "%s - %s", descrs, descr);
		FOCUSER_STATE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_STATE_PROPERTY, NULL);

		if ((PRIVATE_DATA->glst.focus_state == FOCUS_STATE_OFF) ||
		   (PRIVATE_DATA->glst.focus_state == FOCUS_STATE_STOP)) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
			update_focus = true;
		} else {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		}
	}

	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	int res = ascol_FOPO(PRIVATE_DATA->dev_id, &FOCUSER_POSITION_ITEM->number.value);
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	if (res != ASCOL_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_FOPO(%d) = %d", PRIVATE_DATA->dev_id, res);
		FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
		FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, "Could not read Fosus Position");
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, "Could not read Fosus Position");
	} else if (update_all || update_focus ||
	          (prev_focus_pos != FOCUSER_POSITION_ITEM->number.value) ||
	          (FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE) ||
	          (FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE)) {
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		update_focus = false;
	}

	/* should be copied every time as there are several properties
	   relaying on this and we have no track which one changed */
	prev_glst = PRIVATE_DATA->glst;

	update_all = false;
	prev_focus_pos = FOCUSER_POSITION_ITEM->number.value;
}


static void focus_handle_position(indigo_device *device) {
	int res = INDIGO_OK;
	char *description;
	uint16_t condition;
	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	condition = asocol_check_conditions(PRIVATE_DATA->glst, ASCOL_COND_FOCUS_ON, &description);
	if (condition) {
		pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_FOGA(%d) unmet condition(s): %s", PRIVATE_DATA->dev_id, description);
		ascol_FOPO(PRIVATE_DATA->dev_id, &FOCUSER_POSITION_ITEM->number.value);
		FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, "Unmet condition(s): %s", description);
		return;
	}
	FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
	/* focuser GOTO requested */
	res = ascol_FOSA(PRIVATE_DATA->dev_id, FOCUSER_POSITION_ITEM->number.target);
	if (res != INDIGO_OK) {
		FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_FOSA(%d) = %d", PRIVATE_DATA->dev_id, res);
	} else {
		res = ascol_FOGA(PRIVATE_DATA->dev_id);
		if (res != INDIGO_OK) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_FOGA(%d) = %d", PRIVATE_DATA->dev_id, res);
		}
	}
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
}


static void focus_handle_steps(indigo_device *device) {
	int res = INDIGO_OK;
	int sign = 0; /*  move out */
	char *description;
	uint16_t condition;
	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	condition = asocol_check_conditions(PRIVATE_DATA->glst, ASCOL_COND_FOCUS_ON, &description);
	if (condition) {
		pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_FOGR(%d) unmet condition(s): %s", PRIVATE_DATA->dev_id, description);
		FOCUSER_STEPS_ITEM->number.value = 0;
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, "Unmet condition(s): %s", description);
		return;
	}
	FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
	/* focuser relative GOTO requested */
	if (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value)
		sign = -1; /* move in */
	else if (FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM->sw.value)
		sign = 1; /* move out */
	res = ascol_FOSR(PRIVATE_DATA->dev_id, sign * FOCUSER_STEPS_ITEM->number.target);
	if (res != INDIGO_OK) {
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		FOCUSER_STEPS_ITEM->number.value = 0;
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_FOSR(%d) = %d", PRIVATE_DATA->dev_id, res);
	} else {
		res = ascol_FOGR(PRIVATE_DATA->dev_id);
		if (res != INDIGO_OK) {
			FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			FOCUSER_STEPS_ITEM->number.value = 0;
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_FOGR(%d) = %d", PRIVATE_DATA->dev_id, res);
		}
	}
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
}


static indigo_result ascol_focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(FOCUSER_STATE_PROPERTY, property))
			indigo_define_property(device, FOCUSER_STATE_PROPERTY, NULL);
	}
	return indigo_focuser_enumerate_properties(device, NULL, NULL);
}


static void focus_handle_abort(indigo_device *device) {
	int res = ASCOL_OK;
	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	if (FOCUSER_ABORT_MOTION_ITEM->sw.value) {
		res = ascol_FOST(PRIVATE_DATA->dev_id);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_FOST(%d) = %d", PRIVATE_DATA->dev_id, res);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	if(res == ASCOL_OK) {
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_BUSY_STATE;
	} else {
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_FOST(%d) = %d", PRIVATE_DATA->dev_id, res);
	}

	indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
}


static indigo_result focuser_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		// --------------------------------------------------------------------------------
		AUTHENTICATION_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- DEVICE_PORT
		DEVICE_PORT_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- DEVICE_PORTS
		DEVICE_PORTS_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- FOCUSER_SPEED
		FOCUSER_SPEED_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		FOCUSER_POSITION_PROPERTY->perm = INDIGO_RW_PERM;
		// -------------------------------------------------------------------------------- FOCUSER_TEMPERATURE / FOCUSER_COMPENSATION
		FOCUSER_TEMPERATURE_PROPERTY->hidden = true;
		FOCUSER_COMPENSATION_PROPERTY->hidden = true;
		FOCUSER_MODE_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- FOCUSER_BACKLASH
		FOCUSER_BACKLASH_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		strncpy(FOCUSER_STEPS_ITEM->label,"Distance (mm)", INDIGO_VALUE_SIZE);
		FOCUSER_STEPS_ITEM->number.min = 0;
		FOCUSER_STEPS_ITEM->number.max = 100;
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		strncpy(FOCUSER_POSITION_ITEM->label,"Absolute position (mm)", INDIGO_VALUE_SIZE);
		FOCUSER_POSITION_ITEM->number.min = 0;
		FOCUSER_POSITION_ITEM->number.max = 100;
		// -------------------------------------------------------------------------------- FOCUSER STATE
		FOCUSER_STATE_PROPERTY = indigo_init_text_property(NULL, device->name, FOCUSER_STATE_PROPERTY_NAME, FOCUSER_MAIN_GROUP, "Focuser State", INDIGO_BUSY_STATE, INDIGO_RO_PERM, 1);
		if (FOCUSER_STATE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_text_item(FOCUSER_STATE_ITEM, FOCUSER_STATE_ITEM_NAME, "State", "");
		// --------------------------------------------------------------------------------
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return ascol_focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}


static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			if (!device->is_connected) {
				if (ascol_device_open(device)) {
					CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
					indigo_define_property(device, FOCUSER_STATE_PROPERTY, NULL);
					device->is_connected = true;
				} else {
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
				}
			}
		} else {
			if (device->is_connected) {
				ascol_device_close(device);
				indigo_delete_property(device, FOCUSER_STATE_PROPERTY, NULL);
				device->is_connected = false;
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			}
		}
	} else if (indigo_property_match(FOCUSER_STEPS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		if (IS_CONNECTED) {
			indigo_property_copy_values(FOCUSER_STEPS_PROPERTY, property, false);
			focus_handle_steps(device);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_POSITION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		if (IS_CONNECTED) {
			indigo_property_copy_values(FOCUSER_POSITION_PROPERTY, property, false);
			focus_handle_position(device);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_ABORT_MOTION
		if (IS_CONNECTED) {
			indigo_property_copy_values(FOCUSER_ABORT_MOTION_PROPERTY, property, false);
			focus_handle_abort(device);
		}
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_focuser_change_property(device, client, property);
}


static indigo_result focuser_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);

	indigo_release_property(FOCUSER_STATE_PROPERTY);

	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}


// ---------------------------------------------------------------------------------- PANEL

static void panel_attach_devices(indigo_device *device) {
	assert(device != NULL);

	static indigo_device mount_template = INDIGO_DEVICE_INITIALIZER(
		SYSTEM_ASCOL_NAME,
		mount_attach,
		ascol_mount_enumerate_properties,
		mount_change_property,
		NULL,
		mount_detach
	);
	static indigo_device mount_guider_template = INDIGO_DEVICE_INITIALIZER(
		SYSTEM_ASCOL_GUIDER_NAME,
		guider_attach,
		ascol_guider_enumerate_properties,
		guider_change_property,
		NULL,
		guider_detach
	);
	static indigo_device dome_template = INDIGO_DEVICE_INITIALIZER(
		SYSTEM_ASCOL_DOME_NAME,
		dome_attach,
		ascol_dome_enumerate_properties,
		dome_change_property,
		NULL,
		dome_detach
	);
	static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(
		SYSTEM_ASCOL_FOCUSER_NAME,
		focuser_attach,
		ascol_focuser_enumerate_properties,
		focuser_change_property,
		NULL,
		focuser_detach
	);

	mount = indigo_safe_malloc_copy(sizeof(indigo_device), &mount_template);
	mount->private_data = device->private_data;
	indigo_attach_device(mount);

	mount_guider = indigo_safe_malloc_copy(sizeof(indigo_device), &mount_guider_template);
	mount_guider->private_data = device->private_data;
	indigo_attach_device(mount_guider);

	dome = indigo_safe_malloc_copy(sizeof(indigo_device), &dome_template);
	dome->private_data = device->private_data;
	indigo_attach_device(dome);

	focuser = indigo_safe_malloc_copy(sizeof(indigo_device), &focuser_template);
	focuser->private_data = device->private_data;
	indigo_attach_device(focuser);
}


static void panel_detach_devices() {
	if (mount != NULL) {
		indigo_detach_device(mount);
		free(mount);
		mount = NULL;
	}
	if (mount_guider != NULL) {
		indigo_detach_device(mount_guider);
		free(mount_guider);
		mount_guider = NULL;
	}
	if (dome != NULL) {
		indigo_detach_device(dome);
		free(dome);
		dome = NULL;
	}
	if (focuser != NULL) {
		indigo_detach_device(focuser);
		free(focuser);
		dome = NULL;
	}
}


static void panel_timer_callback(indigo_device *device) {
	static ascol_glst_t prev_glst = {0};
	static ascol_glme_t prev_glme = {0};
	static bool update_all = true;
	char *descr;
	int index;

	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	int res = ascol_GLST(PRIVATE_DATA->dev_id, &PRIVATE_DATA->glst);
	if (res != ASCOL_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_GLST(%d) = %d", PRIVATE_DATA->dev_id, res);
		ALARM_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, ALARM_PROPERTY, "Could not read Global Status");
	} else if (update_all || memcmp(prev_glst.alarm_bits, PRIVATE_DATA->glst.alarm_bits, 10)) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Updating ALARM_PROPERTY (dev = %d)", PRIVATE_DATA->dev_id);
		ALARM_PROPERTY->state = INDIGO_OK_STATE;
		index = 0;
		for (int alarm = 0; alarm <= ALARM_MAX; alarm++) {
			int alarm_state;
			ascol_check_alarm(PRIVATE_DATA->glst, alarm, &descr, &alarm_state);
			if (descr[0] != '\0') {
				if (alarm_state) {
					ALARM_ITEMS(index)->light.value = INDIGO_ALERT_STATE;
					ALARM_PROPERTY->state = INDIGO_ALERT_STATE;
				} else {
					ALARM_ITEMS(index)->light.value = INDIGO_OK_STATE;
				}
				index++;
			}
		}
		indigo_update_property(device, ALARM_PROPERTY, NULL);
	}

	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	res = ascol_GLME(PRIVATE_DATA->dev_id, &PRIVATE_DATA->glme);
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	if (res != ASCOL_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_GLME(%d) = %d", PRIVATE_DATA->dev_id, res);
		GLME_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, GLME_PROPERTY, "Could not read Meteo sensrs");
	} else if (update_all || memcmp(&prev_glme, &PRIVATE_DATA->glme, sizeof(prev_glme))) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Updating GLME_PROPERTY (dev = %d)", PRIVATE_DATA->dev_id);
		GLME_PROPERTY->state = INDIGO_OK_STATE;
		for (int index = 0; index < ASCOL_GLME_N; index++) {
			GLME_ITEMS(index)->number.value = PRIVATE_DATA->glme.value[index];
		}
		indigo_update_property(device, GLME_PROPERTY, NULL);
		prev_glme = PRIVATE_DATA->glme;
	}

	/* should be copied every time as there are several properties
	   relaying on this and we have no track which one changed */
	prev_glst = PRIVATE_DATA->glst;

	// Update other devices
	mount_update_state();
	dome_update_state();
	focus_update_state();
	guider_update_state();

	// Reschedule execution
	update_all = false;
	indigo_reschedule_timer(device, REFRESH_SECONDS, &PRIVATE_DATA->panel_timer);
}

static indigo_result panel_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- DEVICE_PORT, DEVICE_PORTS
		DEVICE_PORTS_PROPERTY->hidden = true;
		AUTHENTICATION_PROPERTY->hidden = false;
		AUTHENTICATION_PROPERTY->count = 1;
		// -------------------------------------------------------------------------------- DEVICE_PORT
		DEVICE_PORT_PROPERTY->hidden = false;
		indigo_copy_value(DEVICE_PORT_ITEM->text.value, "ascol://192.168.2.230:2002");
		// -------------------------------------------------------------------------------- ALARM
		ALARM_PROPERTY = indigo_init_light_property(NULL, device->name, ALARM_PROPERTY_NAME, ALARM_GROUP, "Alarms", INDIGO_IDLE_STATE, ALARM_MAX+1);
		if (ALARM_PROPERTY == NULL)
			return INDIGO_FAILED;

		int index = 0;
		for (int alarm = 0; alarm <= ALARM_MAX; alarm++) {
			char alarm_name[INDIGO_NAME_SIZE];
			char *alarm_descr;
			ascol_check_alarm(PRIVATE_DATA->glst, alarm, &alarm_descr, NULL);
			if (alarm_descr[0] != '\0') {
				snprintf(alarm_name, INDIGO_NAME_SIZE, "ALARM_%d_BIT_%d", alarm/16, alarm%16);
				indigo_init_light_item(ALARM_ITEMS(index), alarm_name, alarm_descr, INDIGO_IDLE_STATE);
				index++;
			}
		}
		ALARM_PROPERTY->count = index;
		// --------------------------------------------------------------------------- GLME
		char item_name[INDIGO_NAME_SIZE];
		char item_label[INDIGO_NAME_SIZE];
		GLME_PROPERTY = indigo_init_number_property(NULL, device->name, GLME_PROPERTY_NAME, METEO_DATA_GROUP, "Meteo Sesors", INDIGO_OK_STATE, INDIGO_RO_PERM, ASCOL_GLME_N);
		if (GLME_PROPERTY == NULL)
			return INDIGO_FAILED;

		ascol_GLME(ASCOL_DESCRIBE, &PRIVATE_DATA->glme);
		for (index = 0; index < ASCOL_GLME_N; index++) {
			snprintf(item_name, INDIGO_NAME_SIZE, "%s_%d", GLME_ITEM_NAME_BASE, index);
			snprintf(item_label, INDIGO_NAME_SIZE, "%s (%s)",
			         PRIVATE_DATA->glme.description[index],
			         PRIVATE_DATA->glme.unit[index]
			);
			indigo_init_number_item(GLME_ITEMS(index), item_name, item_label, -1000, 1000, 0.01, 0);
		}
		// --------------------------------------------------------------------------------
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_aux_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result panel_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(ALARM_PROPERTY, property))
			indigo_define_property(device, ALARM_PROPERTY, NULL);
		if (indigo_property_match(GLME_PROPERTY, property))
			indigo_define_property(device, GLME_PROPERTY, NULL);
	}
	return indigo_aux_enumerate_properties(device, NULL, NULL);
}

static indigo_result panel_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			if (!device->is_connected) {
				if (ascol_device_open(device)) {
					CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
					indigo_define_property(device, ALARM_PROPERTY, NULL);
					indigo_define_property(device, GLME_PROPERTY, NULL);
					device->is_connected = true;
					/* start updates */
					panel_attach_devices(device);
					indigo_set_timer(device, 0, panel_timer_callback, &PRIVATE_DATA->panel_timer);
				} else {
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
				}
			}
		} else {
			if (device->is_connected) {
				panel_detach_devices();
				indigo_cancel_timer(device, &PRIVATE_DATA->panel_timer);
				ascol_device_close(device);
				indigo_delete_property(device, ALARM_PROPERTY, NULL);
				indigo_define_property(device, GLME_PROPERTY, NULL);
				device->is_connected = false;
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			}
		}
	}
	return indigo_aux_change_property(device, client, property);
}

static indigo_result panel_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		indigo_device_disconnect(NULL, device->name);
		indigo_cancel_timer(device, &PRIVATE_DATA->panel_timer);
	}

	indigo_release_property(ALARM_PROPERTY);
	indigo_release_property(GLME_PROPERTY);

	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_aux_detach(device);
}


// --------------------------------------------------------------------------------

indigo_result indigo_system_ascol(indigo_driver_action action, indigo_driver_info *info) {

	static indigo_device panel_template = INDIGO_DEVICE_INITIALIZER(
		SYSTEM_ASCOL_PANEL_NAME,
		panel_attach,
		panel_enumerate_properties,
		panel_change_property,
		NULL,
		panel_detach
	);

	//ascol_debug = 1;

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "ASCOL Telescope System", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;
		private_data = indigo_safe_malloc(sizeof(ascol_private_data));
		private_data->dev_id = -1;
		private_data->count_open = 0;

		panel = indigo_safe_malloc_copy(sizeof(indigo_device), &panel_template);
		panel->private_data = private_data;
		indigo_attach_device(panel);
		break;

	case INDIGO_DRIVER_SHUTDOWN:
		VERIFY_NOT_CONNECTED(panel);
		last_action = action;
		if (panel != NULL) {
			indigo_detach_device(panel);
			free(panel);
			panel = NULL;
		}
		if (private_data != NULL) {
			free(private_data);
			private_data = NULL;
		}
		break;

	case INDIGO_DRIVER_INFO:
		break;
	}

	return INDIGO_OK;
}
