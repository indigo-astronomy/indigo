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

#define DRIVER_VERSION 0x0001
#define DRIVER_NAME	"indigo_system_ascol"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <assert.h>

#include "indigo_driver_xml.h"

#include "libascol/libascol.h"
#include "indigo_system_ascol.h"

#define RC_OK 0

#define h2d(h) (h * 15.0)
#define d2h(d) (d / 15.0)

#define REFRESH_SECONDS (2)

#define PRIVATE_DATA                    ((ascol_private_data *)device->private_data)


#define COMMAND_GUIDE_RATE_PROPERTY     (PRIVATE_DATA->command_guide_rate_property)
#define GUIDE_50_ITEM                   (COMMAND_GUIDE_RATE_PROPERTY->items+0)
#define GUIDE_100_ITEM                  (COMMAND_GUIDE_RATE_PROPERTY->items+1)

#define COMMAND_GUIDE_RATE_PROPERTY_NAME   "COMMAND_GUIDE_RATE"
#define GUIDE_50_ITEM_NAME                 "GUIDE_50"
#define GUIDE_100_ITEM_NAME                "GUIDE_100"

#define ALARM_GROUP                        "Alarms"
#define TELESCOPE_STATE_GROUP              "Telescope Status"
#define METEO_DATA_GROUP                   "Meteo Data"
#define SWITCHES_GROUP                     "System Switches"
#define CORRECTIONS_GROUP                  "Corrections"

#define ALARM_PROPERTY                     (PRIVATE_DATA->alarm_property)
#define ALARM_ITEMS(index)                 (ALARM_PROPERTY->items+index)
#define ALARM_PROPERTY_NAME                "ASCOL_ALARMS"
#define ALARM_ITEM_NAME_BASE               "ALARM"

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

#define GLME_PROPERTY                      (PRIVATE_DATA->glme_property)
#define GLME_ITEMS(index)                  (GLME_PROPERTY->items+index)
#define GLME_PROPERTY_NAME                 "ASCOL_GLME"
#define GLME_ITEM_NAME_BASE                "VALUE"

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
#define GUIDE_MODE_ON_ITEM_NAME            "ON"
#define GUIDE_MODE_PROPERTY_NAME           "ASCOL_GUIDE_MODE"
#define GUIDE_MODE_OFF_ITEM_NAME           "OFF"

#define WARN_PARKED_MSG                    "Mount is parked, please unpark!"
#define WARN_PARKING_PROGRESS_MSG          "Mount parking is in progress, please wait until complete!"

// gp_bits is used as boolean
#define is_connected                   gp_bits

typedef struct {
	int dev_id;
	bool parked;
	bool park_in_progress;
	char tty_name[INDIGO_VALUE_SIZE];
	int count_open;
	int slew_rate;
	int st4_ra_rate, st4_dec_rate;
	int vendor_id;
	pthread_mutex_t net_mutex;
	ascol_glst_t glst;
	ascol_oimv_t oimv;
	ascol_glme_t glme;
	indigo_timer *position_timer, *state_timer, *guider_timer_ra, *guider_timer_dec, *park_timer;
	int guide_rate;
	indigo_property *command_guide_rate_property;
	indigo_property *alarm_property;
	indigo_property *oil_state_property;
	indigo_property *oimv_property;
	indigo_property *mount_state_property;
	indigo_property *flap_state_property;
	indigo_property *glme_property;
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
} ascol_private_data;

// -------------------------------------------------------------------------------- INDIGO MOUNT device implementation

static indigo_result ascol_mount_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(ALARM_PROPERTY, property))
			indigo_define_property(device, ALARM_PROPERTY, NULL);
		if (indigo_property_match(OIL_STATE_PROPERTY, property))
			indigo_define_property(device, OIL_STATE_PROPERTY, NULL);
		if (indigo_property_match(OIMV_PROPERTY, property))
			indigo_define_property(device, OIMV_PROPERTY, NULL);
		if (indigo_property_match(MOUNT_STATE_PROPERTY, property))
			indigo_define_property(device, MOUNT_STATE_PROPERTY, NULL);
		if (indigo_property_match(FLAP_STATE_PROPERTY, property))
			indigo_define_property(device, FLAP_STATE_PROPERTY, NULL);
		if (indigo_property_match(GLME_PROPERTY, property))
			indigo_define_property(device, GLME_PROPERTY, NULL);
		if (indigo_property_match(OIL_POWER_PROPERTY, property))
			indigo_define_property(device, OIL_POWER_PROPERTY, NULL);
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
	}
	return indigo_mount_enumerate_properties(device, NULL, NULL);
}

static bool mount_open(indigo_device *device) {
	if (device->is_connected) return false;

	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	if (PRIVATE_DATA->count_open++ == 0) {
		char host[255];
		int port;
		ascol_parse_devname(DEVICE_PORT_ITEM->text.value, host, &port);
		INDIGO_DRIVER_LOG(DRIVER_NAME, "host = %s, port = %d", host, port);
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
		}
	}
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	return true;
}


static void mount_handle_coordinates(indigo_device *device) {
	int res = RC_OK;
	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);

	/* return if mount not aligned */
	int aligned = 0; //tc_check_align(PRIVATE_DATA->dev_id);
	if (aligned < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_check_align(%d) = %d", PRIVATE_DATA->dev_id, res);
	} else if (aligned == 0) {
		pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_coordinates(device, "Mount is not aligned, please align it first.");
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Mount is not aligned, please align it first.");
		return;
	}

	MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
	/* GOTO requested */
	if(MOUNT_ON_COORDINATES_SET_TRACK_ITEM->sw.value) {
		// res = tc_goto_rade_p(PRIVATE_DATA->dev_id, h2d(MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target), MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target);
		if (res != RC_OK) {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_goto_rade_p(%d) = %d", PRIVATE_DATA->dev_id, res);
		}
	}
	/* SYNC requested */
	else if (MOUNT_ON_COORDINATES_SET_SYNC_ITEM->sw.value) {
		// res = tc_sync_rade_p(PRIVATE_DATA->dev_id, h2d(MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target), MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target);
		if (res != RC_OK) {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_sync_rade_p(%d) = %d", PRIVATE_DATA->dev_id, res);
		}
	}
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	indigo_update_coordinates(device, NULL);
}


static void mount_handle_tracking(indigo_device *device) {
	int res = ASCOL_OK;
	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	if (MOUNT_TRACKING_ON_ITEM->sw.value) {
		res = ascol_TETR(PRIVATE_DATA->dev_id, ASCOL_ON);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_TETR(%d, ASCOL_ON) = %d", PRIVATE_DATA->dev_id, res);
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
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_TETR(%d) = %d", PRIVATE_DATA->dev_id, res);
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
	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	if (TELESCOPE_ON_ITEM->sw.value) {
		res = ascol_TEON(PRIVATE_DATA->dev_id, ASCOL_ON);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_TEON(%d, ASCOL_ON) = %d", PRIVATE_DATA->dev_id, res);
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
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_TEON(%d) = %d", PRIVATE_DATA->dev_id, res);
	}
	indigo_update_property(device, TELESCOPE_POWER_PROPERTY, NULL);
}


static void mount_handle_ra_calibration(indigo_device *device) {
	int res = ASCOL_OK;
	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	if (RA_CALIBRATION_START_ITEM->sw.value) {
		res = ascol_TEHC(PRIVATE_DATA->dev_id, ASCOL_ON);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_TEHC(%d, ASCOL_ON) = %d", PRIVATE_DATA->dev_id, res);
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
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_TEHC(%d) = %d", PRIVATE_DATA->dev_id, res);
	}
	indigo_update_property(device, RA_CALIBRATION_PROPERTY, NULL);
}


static void mount_handle_dec_calibration(indigo_device *device) {
	int res = ASCOL_OK;
	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	if (DEC_CALIBRATION_START_ITEM->sw.value) {
		res = ascol_TEDC(PRIVATE_DATA->dev_id, ASCOL_ON);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ascol_TEDC(%d, ASCOL_ON) = %d", PRIVATE_DATA->dev_id, res);
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
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_TEDC(%d) = %d", PRIVATE_DATA->dev_id, res);
	}
	indigo_update_property(device, RA_CALIBRATION_PROPERTY, NULL);
}


static void mount_handle_aberration(indigo_device *device) {
	int res = ASCOL_OK;
	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
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
	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
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
	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
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
	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
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
	int index = (int)CORRECTION_MODEL_INDEX_ITEM->number.value;
	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
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


static void mount_handle_guidemode(indigo_device *device) {
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


static void mount_handle_slew_rate(indigo_device *device) {
	if(MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value) {
		PRIVATE_DATA->slew_rate = PRIVATE_DATA->guide_rate;
	} else if (MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value) {
		PRIVATE_DATA->slew_rate = 4;
	} else if (MOUNT_SLEW_RATE_FIND_ITEM->sw.value) {
		PRIVATE_DATA->slew_rate = 6;
	} else if (MOUNT_SLEW_RATE_MAX_ITEM->sw.value) {
		PRIVATE_DATA->slew_rate = 9;
	} else {
		MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value = true;
		PRIVATE_DATA->slew_rate = PRIVATE_DATA->guide_rate;
	}
	MOUNT_SLEW_RATE_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_SLEW_RATE_PROPERTY, NULL);
}


static void mount_handle_motion_ns(indigo_device *device) {
	int dev_id = PRIVATE_DATA->dev_id;
	int res = RC_OK;

	if (PRIVATE_DATA->slew_rate == 0) mount_handle_slew_rate(device);

	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	if(MOUNT_MOTION_NORTH_ITEM->sw.value) {
		// res = tc_slew_fixed(dev_id, TC_AXIS_DE, TC_DIR_POSITIVE, PRIVATE_DATA->slew_rate);
		MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
	} else if (MOUNT_MOTION_SOUTH_ITEM->sw.value) {
		// res = tc_slew_fixed(dev_id, TC_AXIS_DE, TC_DIR_NEGATIVE, PRIVATE_DATA->slew_rate);
		MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
	} else {
		// res = tc_slew_fixed(dev_id, TC_AXIS_DE, TC_DIR_POSITIVE, 0); // STOP move
		MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_OK_STATE;
	}
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);

	if (res != RC_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_slew_fixed(%d) = %d", dev_id, res);
		MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
	}

	indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
}


static void mount_handle_motion_ne(indigo_device *device) {
	int dev_id = PRIVATE_DATA->dev_id;
	int res = RC_OK;

	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	if(MOUNT_MOTION_EAST_ITEM->sw.value) {
		// res = tc_slew_fixed(dev_id, TC_AXIS_RA, TC_DIR_POSITIVE, PRIVATE_DATA->slew_rate);
		MOUNT_MOTION_RA_PROPERTY->state = INDIGO_BUSY_STATE;
	} else if (MOUNT_MOTION_WEST_ITEM->sw.value) {
		// res = tc_slew_fixed(dev_id, TC_AXIS_RA, TC_DIR_NEGATIVE, PRIVATE_DATA->slew_rate);
		MOUNT_MOTION_RA_PROPERTY->state = INDIGO_BUSY_STATE;
	} else {
		// res = tc_slew_fixed(dev_id, TC_AXIS_RA, TC_DIR_POSITIVE, 0); // STOP move
		MOUNT_MOTION_RA_PROPERTY->state = INDIGO_OK_STATE;
	}
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);

	if (res != RC_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_slew_fixed(%d) = %d", dev_id, res);
		MOUNT_MOTION_RA_PROPERTY->state = INDIGO_ALERT_STATE;
	}

	indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
}


static bool mount_set_location(indigo_device *device) {
	int res;
	double lon = MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value;
	if (lon > 180) lon -= 360.0;
	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	// res = tc_set_location(PRIVATE_DATA->dev_id, lon, MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value);
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	if (res != RC_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_set_location(%d) = %d", PRIVATE_DATA->dev_id, res);
		return false;
	}
	return true;
}


static void mount_handle_st4_guiding_rate(indigo_device *device) {
	int dev_id = PRIVATE_DATA->dev_id;
	int res = RC_OK;

	int offset = 1;                                             /* for Ceslestron 0 is 1% and 99 is 100% */
	//if (PRIVATE_DATA->vendor_id == VNDR_SKYWATCHER) offset = 0; /* there is no offset for Sky-Watcher */

	MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_OK_STATE;

	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	/* reset only if input value is changed - better begaviour for Sky-Watcher as there are no separate RA and DEC rates */
	if ((int)(MOUNT_GUIDE_RATE_RA_ITEM->number.value) != PRIVATE_DATA->st4_ra_rate) {
		// res = tc_set_autoguide_rate(dev_id, TC_AXIS_RA, (int)(MOUNT_GUIDE_RATE_RA_ITEM->number.value)-1);
		if (res != RC_OK) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_set_autoguide_rate(%d) = %d", dev_id, res);
			MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			PRIVATE_DATA->st4_ra_rate = (int)(MOUNT_GUIDE_RATE_RA_ITEM->number.value);
		}
	}

	/* reset only if input value is changed - better begaviour for Sky-Watcher as there are no separate RA and DEC rates */
	if ((int)(MOUNT_GUIDE_RATE_DEC_ITEM->number.value) != PRIVATE_DATA->st4_dec_rate) {
		// res = tc_set_autoguide_rate(dev_id, TC_AXIS_DE, (int)(MOUNT_GUIDE_RATE_DEC_ITEM->number.value)-1);
		if (res != RC_OK) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_set_autoguide_rate(%d) = %d", dev_id, res);
			MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			PRIVATE_DATA->st4_dec_rate = (int)(MOUNT_GUIDE_RATE_DEC_ITEM->number.value);
		}
	}

	/* read set values as Sky-Watcher rounds to 12, 25 ,50, 75 and 100 % */
	int st4_ra_rate = 0; // tc_get_autoguide_rate(dev_id, TC_AXIS_RA);
	if (st4_ra_rate < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_get_autoguide_rate(%d) = %d", dev_id, st4_ra_rate);
	} else {
		MOUNT_GUIDE_RATE_RA_ITEM->number.value = st4_ra_rate + offset;
	}

	int st4_dec_rate = 0; // tc_get_autoguide_rate(dev_id, TC_AXIS_DE);
	if (st4_dec_rate < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_get_autoguide_rate(%d) = %d", dev_id, st4_dec_rate);
	} else {
		MOUNT_GUIDE_RATE_DEC_ITEM->number.value = st4_dec_rate + offset;
	}

	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);

	indigo_update_property(device, MOUNT_GUIDE_RATE_PROPERTY, NULL);
}


static void mount_handle_utc(indigo_device *device) {
	time_t utc_time = 0; // indigo_isototime(MOUNT_UTC_ITEM->text.value);
	if (utc_time == -1) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Wrong date/time format!");
		MOUNT_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, "Wrong date/time format!");
		return;
	}

	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	/* set mount time to UTC */
	int res = 0; // tc_set_time(PRIVATE_DATA->dev_id, utc_time, atoi(MOUNT_UTC_OFFEST_ITEM->text.value), 0);
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);

	if (res != RC_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_set_time(%d) = %d", PRIVATE_DATA->dev_id, res);
		MOUNT_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, "Can not set mount date/time.");
		return;
	}

	MOUNT_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
	return;
}

static bool mount_set_utc_from_host(indigo_device *device) {
	time_t utc_time = indigo_utc(NULL);
	if (utc_time == -1) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Can not get host UT");
		return false;
	}

	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	/* set mount time to UTC */
	int res = 0; // tc_set_time(PRIVATE_DATA->dev_id, utc_time, 0, 0);
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);

	if (res != RC_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_set_time(%d) = %d", PRIVATE_DATA->dev_id, res);
		return false;
	}
	return true;
}


static bool mount_cancel_slew(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);

	int res = 0; // tc_goto_cancel(PRIVATE_DATA->dev_id);
	if (res != RC_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_goto_cancel(%d) = %d", PRIVATE_DATA->dev_id, res);
	}

	MOUNT_MOTION_NORTH_ITEM->sw.value = false;
	MOUNT_MOTION_SOUTH_ITEM->sw.value = false;
	MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
	MOUNT_MOTION_WEST_ITEM->sw.value = false;
	MOUNT_MOTION_EAST_ITEM->sw.value = false;
	MOUNT_MOTION_RA_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
	MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value;
	MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value;
	MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_coordinates(device, NULL);
	MOUNT_ABORT_MOTION_ITEM->sw.value = false;
	MOUNT_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_ABORT_MOTION_PROPERTY, "Aborted.");

	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	return true;
}


static void mount_close(indigo_device *device) {
	if (!device->is_connected) return;

	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	if (--PRIVATE_DATA->count_open == 0) {
		ascol_close(PRIVATE_DATA->dev_id);
		PRIVATE_DATA->dev_id = -1;
	}
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
}


static void park_timer_callback(indigo_device *device) {
	int res;
	int dev_id = PRIVATE_DATA->dev_id;

	if (dev_id < 0) return;

	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	if ( 0 /* tc_goto_in_progress(dev_id) */) {
		MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
		PRIVATE_DATA->park_in_progress = true;
	} else {
		//res = tc_set_tracking_mode(dev_id, TC_TRACK_OFF);
		if (res != RC_OK) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_set_tracking_mode(%d) = %d", dev_id, res);
		} else {
			MOUNT_TRACKING_OFF_ITEM->sw.value = true;
			MOUNT_TRACKING_ON_ITEM->sw.value = false;
			indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
		}
		MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
		PRIVATE_DATA->park_in_progress = false;
	}
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);

	if (PRIVATE_DATA->park_in_progress) {
		indigo_reschedule_timer(device, REFRESH_SECONDS, &PRIVATE_DATA->park_timer);
	} else {
		PRIVATE_DATA->park_timer = NULL;
		indigo_update_property(device, MOUNT_PARK_PROPERTY, "Mount Parked.");
	}
}


static void position_timer_callback(indigo_device *device) {
	int res;
	double ra, dec, lon, lat;
	int dev_id = PRIVATE_DATA->dev_id;

	if (dev_id < 0) return;

	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	if ( 0 /* tc_goto_in_progress(dev_id) */) {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
	} else {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
	}

	//res = tc_get_rade_p(dev_id, &ra, &dec);
	if (res != RC_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_get_rade_p(%d) = %d", dev_id, res);
	}

	//res = tc_get_location(dev_id, &lon, &lat);
	if (res != RC_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_get_location(%d) = %d", dev_id, res);
	}
	if (lon < 0) lon += 360;

	time_t ttime;
	int tz, dst;
	//res = (int)tc_get_time(dev_id, &ttime, &tz, &dst);
	if (res == -1) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_get_time(%d) = %d", dev_id, res);
		MOUNT_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		MOUNT_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
	}
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);

	MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = d2h(ra);
	MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = dec;
	indigo_update_coordinates(device, NULL);
	MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = lon;
	MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = lat;
	indigo_update_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);

	indigo_timetoiso(ttime - 3600 * dst, MOUNT_UTC_ITEM->text.value, INDIGO_VALUE_SIZE);
	indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);

	indigo_reschedule_timer(device, REFRESH_SECONDS, &PRIVATE_DATA->position_timer);
}


static void state_timer_callback(indigo_device *device) {
	static ascol_glst_t prev_glst = {0};
	static ascol_oimv_t prev_oimv = {0};
	static ascol_glme_t prev_glme = {0};
	static bool first_call = true;

	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	int res = ascol_GLST(PRIVATE_DATA->dev_id, &PRIVATE_DATA->glst);
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	if (res != ASCOL_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_GLST(%d) = %d", PRIVATE_DATA->dev_id, res);
		ALARM_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, ALARM_PROPERTY, "Could not read Global Status");
		OIL_STATE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, OIL_STATE_PROPERTY, "Could not read Global Status");
		MOUNT_STATE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_STATE_PROPERTY, "Could not read Global Status");
		FLAP_STATE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FLAP_STATE_PROPERTY, "Could not read Global Status");
		AXIS_CALIBRATED_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, AXIS_CALIBRATED_PROPERTY, "Could not read Global Status");
		goto OIL_MEASURE;
	}

	char *descr, *descrs;
	int index;
	if (first_call || memcmp(prev_glst.alarm_bits, PRIVATE_DATA->glst.alarm_bits, 10)) {
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

	if (first_call || (prev_glst.state_bits != PRIVATE_DATA->glst.state_bits)) {
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

	if (first_call || (prev_glst.oil_state != PRIVATE_DATA->glst.oil_state) ||
	   (OIL_POWER_PROPERTY->state == INDIGO_BUSY_STATE)) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Updating OIL_STATE_PROPERTY (dev = %d)", PRIVATE_DATA->dev_id);
		OIL_STATE_PROPERTY->state = INDIGO_OK_STATE;
		ascol_get_oil_state(PRIVATE_DATA->glst, &descr, &descrs);
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

	if (first_call || (prev_glst.telescope_state != PRIVATE_DATA->glst.telescope_state) ||
	   (prev_glst.ra_axis_state != PRIVATE_DATA->glst.ra_axis_state) ||
	   (prev_glst.de_axis_state != PRIVATE_DATA->glst.de_axis_state) ||
	   (TELESCOPE_POWER_PROPERTY->state == INDIGO_BUSY_STATE) ||
	   (MOUNT_TRACKING_PROPERTY->state == INDIGO_BUSY_STATE) ||
	   (RA_CALIBRATION_PROPERTY->state == INDIGO_BUSY_STATE) ||
	   (DEC_CALIBRATION_PROPERTY->state == INDIGO_BUSY_STATE)) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Updating MOUNT_STATE_PROPERTY (dev = %d)", PRIVATE_DATA->dev_id);
		MOUNT_STATE_PROPERTY->state = INDIGO_OK_STATE;
		ascol_get_telescope_state(PRIVATE_DATA->glst, &descr, &descrs);
		snprintf(MOUNT_STATE_ITEM->text.value, INDIGO_VALUE_SIZE, "%s - %s", descrs, descr);
		ascol_get_ra_axis_state(PRIVATE_DATA->glst, &descr, &descrs);
		snprintf(RA_STATE_ITEM->text.value, INDIGO_VALUE_SIZE, "%s - %s", descrs, descr);
		ascol_get_de_axis_state(PRIVATE_DATA->glst, &descr, &descrs);
		snprintf(DEC_STATE_ITEM->text.value, INDIGO_VALUE_SIZE, "%s - %s", descrs, descr);
		indigo_update_property(device, MOUNT_STATE_PROPERTY, NULL);

		if ((PRIVATE_DATA->glst.telescope_state == TE_STATE_OFF) ||
		    (PRIVATE_DATA->glst.telescope_state == TE_STATE_INIT)) {
			TELESCOPE_ON_ITEM->sw.value = false;
			TELESCOPE_OFF_ITEM->sw.value = true;
			TELESCOPE_POWER_PROPERTY->state = INDIGO_OK_STATE;
		} else if((PRIVATE_DATA->glst.telescope_state == TE_STATE_STOP) ||
		          (PRIVATE_DATA->glst.telescope_state == TE_STATE_TRACK)) {
			TELESCOPE_ON_ITEM->sw.value = true;
			TELESCOPE_OFF_ITEM->sw.value = false;
			TELESCOPE_POWER_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			TELESCOPE_ON_ITEM->sw.value = true;
			TELESCOPE_OFF_ITEM->sw.value = false;
			TELESCOPE_POWER_PROPERTY->state = INDIGO_BUSY_STATE;
		}
		indigo_update_property(device, TELESCOPE_POWER_PROPERTY, NULL);

		if ((PRIVATE_DATA->glst.telescope_state == TE_STATE_OFF) ||
		    (PRIVATE_DATA->glst.telescope_state == TE_STATE_STOP) ||
		    (PRIVATE_DATA->glst.telescope_state == TE_STATE_INIT)) {
			MOUNT_TRACKING_ON_ITEM->sw.value = false;
			MOUNT_TRACKING_OFF_ITEM->sw.value = true;
			MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
		} else if(PRIVATE_DATA->glst.telescope_state == TE_STATE_TRACK) {
			MOUNT_TRACKING_ON_ITEM->sw.value = true;
			MOUNT_TRACKING_OFF_ITEM->sw.value = false;
			MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			MOUNT_TRACKING_PROPERTY->state = INDIGO_BUSY_STATE;
		}
		indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);

		if ((PRIVATE_DATA->glst.ra_axis_state == 0) ||
			(PRIVATE_DATA->glst.ra_axis_state == 1)) {
			RA_CALIBRATION_START_ITEM->sw.value = false;
			RA_CALIBRATION_STOP_ITEM->sw.value = false;
			RA_CALIBRATION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			RA_CALIBRATION_PROPERTY->state = INDIGO_BUSY_STATE;
		}
		indigo_update_property(device, RA_CALIBRATION_PROPERTY, NULL);

		if ((PRIVATE_DATA->glst.de_axis_state == 0) ||
			(PRIVATE_DATA->glst.de_axis_state == 1)) {
			DEC_CALIBRATION_START_ITEM->sw.value = false;
			DEC_CALIBRATION_STOP_ITEM->sw.value = false;
			DEC_CALIBRATION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			DEC_CALIBRATION_PROPERTY->state = INDIGO_BUSY_STATE;
		}
		indigo_update_property(device, DEC_CALIBRATION_PROPERTY, NULL);
	}

	if (first_call || (IS_ABEARRATION_CORR(prev_glst) != IS_ABEARRATION_CORR(PRIVATE_DATA->glst)) ||
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

	if (first_call || (IS_PRECESSION_CORR(prev_glst) != IS_PRECESSION_CORR(PRIVATE_DATA->glst)) ||
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

	if (first_call || (IS_REFRACTION_CORR(prev_glst) != IS_REFRACTION_CORR(PRIVATE_DATA->glst)) ||
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

	if (first_call || (IS_ERR_MODEL_CORR(prev_glst) != IS_ERR_MODEL_CORR(PRIVATE_DATA->glst)) ||
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

	if (first_call || (prev_glst.selected_model_index != PRIVATE_DATA->glst.selected_model_index) ||
	   (CORRECTION_MODEL_PROPERTY->state == INDIGO_BUSY_STATE)) {
		CORRECTION_MODEL_PROPERTY->state = INDIGO_OK_STATE;
		if (!first_call && ((int)CORRECTION_MODEL_INDEX_ITEM->number.value != PRIVATE_DATA->glst.selected_model_index)) {
			CORRECTION_MODEL_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		CORRECTION_MODEL_INDEX_ITEM->number.value = (double)PRIVATE_DATA->glst.selected_model_index;
		indigo_update_property(device, CORRECTION_MODEL_PROPERTY, NULL);
	}

	if (first_call || (IS_GUIDE_MODE_ON(prev_glst) != IS_GUIDE_MODE_ON(PRIVATE_DATA->glst)) ||
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

	if (first_call || (prev_glst.flap_tube_state != PRIVATE_DATA->glst.flap_tube_state) ||
	   (prev_glst.flap_coude_state != PRIVATE_DATA->glst.flap_coude_state)) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Updating FLAP_STATE_PROPERTY (dev = %d)", PRIVATE_DATA->dev_id);
		FLAP_STATE_PROPERTY->state = INDIGO_OK_STATE;
		ascol_get_flap_tube_state(PRIVATE_DATA->glst, &descr, &descrs);
		snprintf(TUBE_FLAP_STATE_ITEM->text.value, INDIGO_VALUE_SIZE, "%s - %s", descrs, descr);
		ascol_get_flap_coude_state(PRIVATE_DATA->glst, &descr, &descrs);
		snprintf(COUDE_FLAP_STATE_ITEM->text.value, INDIGO_VALUE_SIZE, "%s - %s", descrs, descr);
		indigo_update_property(device, FLAP_STATE_PROPERTY, NULL);
	}
	/* should be copied every time as there are several properties
	   relaying on this and we have no track which one changed */
	prev_glst = PRIVATE_DATA->glst;

	OIL_MEASURE:
	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	res = ascol_OIMV(PRIVATE_DATA->dev_id, &PRIVATE_DATA->oimv);
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	if (res != ASCOL_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_OIMV(%d) = %d", PRIVATE_DATA->dev_id, res);
		OIMV_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, OIMV_PROPERTY, "Could not read oil sensrs");
		goto METEO_MEASURE;
	}

	if (first_call || memcmp(&prev_oimv, &PRIVATE_DATA->oimv, sizeof(prev_oimv))) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Updating OIMV_PROPERTY (dev = %d)", PRIVATE_DATA->dev_id);
		OIMV_PROPERTY->state = INDIGO_OK_STATE;
		for (int index = 0; index < ASCOL_OIMV_N; index++) {
			OIMV_ITEMS(index)->number.value = PRIVATE_DATA->oimv.value[index];
		}
		indigo_update_property(device, OIMV_PROPERTY, NULL);
		prev_oimv = PRIVATE_DATA->oimv;
	}

	METEO_MEASURE:
	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	res = ascol_GLME(PRIVATE_DATA->dev_id, &PRIVATE_DATA->glme);
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	if (res != ASCOL_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ascol_GLME(%d) = %d", PRIVATE_DATA->dev_id, res);
		GLME_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, GLME_PROPERTY, "Could not read Meteo sensrs");
		goto RESCHEDULE_TIMER;
	}

	if (first_call || memcmp(&prev_glme, &PRIVATE_DATA->glme, sizeof(prev_glme))) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Updating GLME_PROPERTY (dev = %d)", PRIVATE_DATA->dev_id);
		GLME_PROPERTY->state = INDIGO_OK_STATE;
		for (int index = 0; index < ASCOL_GLME_N; index++) {
			GLME_ITEMS(index)->number.value = PRIVATE_DATA->glme.value[index];
		}
		indigo_update_property(device, GLME_PROPERTY, NULL);
		prev_glme = PRIVATE_DATA->glme;
	}

	RESCHEDULE_TIMER:
	first_call = false;
	indigo_reschedule_timer(device, REFRESH_SECONDS, &PRIVATE_DATA->state_timer);
}


static indigo_result mount_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_mount_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		pthread_mutex_init(&PRIVATE_DATA->net_mutex, NULL);
		// -------------------------------------------------------------------------------- SIMULATION
		SIMULATION_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- MOUNT_ON_COORDINATES_SET
		MOUNT_ON_COORDINATES_SET_PROPERTY->count = 2;
		// --------------------------------------------------------------------------------
		AUTHENTICATION_PROPERTY->hidden = false;
		AUTHENTICATION_PROPERTY->count = 1;
		// -------------------------------------------------------------------------------- DEVICE_PORT
		DEVICE_PORT_PROPERTY->hidden = false;
		strncpy(DEVICE_PORT_ITEM->text.value, "ascol://localhost:2000", INDIGO_VALUE_SIZE);
		// -------------------------------------------------------------------------------- DEVICE_PORTS
		DEVICE_PORTS_PROPERTY->hidden = true;
		// --------------------------------------------------------------------------------

		MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->hidden = false;
		MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->count = 2; // we can not set elevation from the protocol

		MOUNT_LST_TIME_PROPERTY->hidden = true;
		MOUNT_UTC_TIME_PROPERTY->hidden = false;
		MOUNT_PARK_PARKED_ITEM->sw.value = false;
		MOUNT_PARK_UNPARKED_ITEM->sw.value = true;
		//MOUNT_PARK_PROPERTY->hidden = true;
		//MOUNT_UTC_TIME_PROPERTY->count = 1;
		//MOUNT_UTC_TIME_PROPERTY->perm = INDIGO_RO_PERM;
		MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;

		strncpy(MOUNT_GUIDE_RATE_PROPERTY->label,"ST4 guide rate", INDIGO_VALUE_SIZE);

		MOUNT_TRACK_RATE_PROPERTY->hidden = false;
		strncpy(MOUNT_TRACKING_PROPERTY->group, SWITCHES_GROUP, INDIGO_NAME_SIZE);

		MOUNT_SLEW_RATE_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------- ALARM
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
		// --------------------------------------------------------------------------- OIL STATE
		OIL_STATE_PROPERTY = indigo_init_text_property(NULL, device->name, OIL_STATE_PROPERTY_NAME, TELESCOPE_STATE_GROUP, "Oil State", INDIGO_IDLE_STATE, INDIGO_RO_PERM, 1);
		if (OIL_STATE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_text_item(OIL_STATE_ITEM, OIL_STATE_ITEM_NAME, "State", "");
		// --------------------------------------------------------------------------- MOUNT STATE
		MOUNT_STATE_PROPERTY = indigo_init_text_property(NULL, device->name, MOUNT_STATE_PROPERTY_NAME, TELESCOPE_STATE_GROUP, "Mount State", INDIGO_IDLE_STATE, INDIGO_RO_PERM, 3);
		if (MOUNT_STATE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_text_item(MOUNT_STATE_ITEM, MOUNT_STATE_ITEM_NAME, "Mount", "");
		indigo_init_text_item(RA_STATE_ITEM, RA_STATE_ITEM_NAME, "RA Axis", "");
		indigo_init_text_item(DEC_STATE_ITEM, DEC_STATE_ITEM_NAME, "DEC Axis", "");
		// --------------------------------------------------------------------------- FLAP STATE
		FLAP_STATE_PROPERTY = indigo_init_text_property(NULL, device->name, FLAP_STATE_PROPERTY_NAME, TELESCOPE_STATE_GROUP, "Flaps State", INDIGO_IDLE_STATE, INDIGO_RO_PERM, 2);
		if (FLAP_STATE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_text_item(TUBE_FLAP_STATE_ITEM, TUBE_FLAP_STATE_ITEM_NAME, "Tube Flap", "");
		indigo_init_text_item(COUDE_FLAP_STATE_ITEM, COUDE_FLAP_STATE_ITEM_NAME, "Coude Flap", "");

		char item_name[INDIGO_NAME_SIZE];
		char item_label[INDIGO_NAME_SIZE];
		// --------------------------------------------------------------------------- OIMV
		OIMV_PROPERTY = indigo_init_number_property(NULL, device->name, OIMV_PROPERTY_NAME, TELESCOPE_STATE_GROUP, "Oil Sesors", INDIGO_OK_STATE, INDIGO_RO_PERM, ASCOL_OIMV_N);
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
		// --------------------------------------------------------------------------- GLME
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
		// -------------------------------------------------------------------------- OIL_POWER
		OIL_POWER_PROPERTY = indigo_init_switch_property(NULL, device->name, OIL_POWER_PROPERTY_NAME, SWITCHES_GROUP, "Oil Power", INDIGO_BUSY_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (OIL_POWER_PROPERTY == NULL)
			return INDIGO_FAILED;

		indigo_init_switch_item(OIL_ON_ITEM, OIL_ON_ITEM_NAME, "On", false);
		indigo_init_switch_item(OIL_OFF_ITEM, OIL_OFF_ITEM_NAME, "Off", true);
		// -------------------------------------------------------------------------- TELESCOPE_POWER
		TELESCOPE_POWER_PROPERTY = indigo_init_switch_property(NULL, device->name, TELESCOPE_POWER_PROPERTY_NAME, SWITCHES_GROUP, "Telescope Power", INDIGO_BUSY_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (TELESCOPE_POWER_PROPERTY == NULL)
			return INDIGO_FAILED;

		indigo_init_switch_item(TELESCOPE_ON_ITEM, TELESCOPE_ON_ITEM_NAME, "On", false);
		indigo_init_switch_item(TELESCOPE_OFF_ITEM, TELESCOPE_OFF_ITEM_NAME, "Off", true);
		// -------------------------------------------------------------------------- AXIS_CALIBRATED
		AXIS_CALIBRATED_PROPERTY = indigo_init_light_property(NULL, device->name, AXIS_CALIBRATED_PROPERTY_NAME, SWITCHES_GROUP, "Axis Calibrated", INDIGO_IDLE_STATE, 2);
		if (AXIS_CALIBRATED_PROPERTY == NULL)
			return INDIGO_FAILED;

		indigo_init_light_item(RA_CALIBRATED_ITEM, RA_CALIBRATED_ITEM_NAME, "RA Axis", INDIGO_IDLE_STATE);
		indigo_init_light_item(DEC_CALIBRATED_ITEM, DEC_CALIBRATED_ITEM_NAME, "DEC Axis", INDIGO_IDLE_STATE);
		// -------------------------------------------------------------------------- RA_CALIBRATION
		RA_CALIBRATION_PROPERTY = indigo_init_switch_property(NULL, device->name, RA_CALIBRATION_PROPERTY_NAME, SWITCHES_GROUP, "RA Calibration", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 2);
		if (RA_CALIBRATION_PROPERTY == NULL)
			return INDIGO_FAILED;

		indigo_init_switch_item(RA_CALIBRATION_START_ITEM, RA_CALIBRATION_START_ITEM_NAME, "Start", false);
		indigo_init_switch_item(RA_CALIBRATION_STOP_ITEM, RA_CALIBRATION_STOP_ITEM_NAME, "Stop", false);
		// -------------------------------------------------------------------------- DEC_CALIBRATION
		DEC_CALIBRATION_PROPERTY = indigo_init_switch_property(NULL, device->name, DEC_CALIBRATION_PROPERTY_NAME, SWITCHES_GROUP, "DEC Calibration", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 2);
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
		GUIDE_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, GUIDE_MODE_PROPERTY_NAME, CORRECTIONS_GROUP, "Guide Mode Correction", INDIGO_BUSY_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (GUIDE_MODE_PROPERTY == NULL)
			return INDIGO_FAILED;

		indigo_init_switch_item(GUIDE_MODE_ON_ITEM, GUIDE_MODE_ON_ITEM_NAME, "On", false);
		indigo_init_switch_item(GUIDE_MODE_OFF_ITEM, GUIDE_MODE_OFF_ITEM_NAME, "Off", true);
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
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			if (!device->is_connected) {
				if (mount_open(device)) {
					int dev_id = PRIVATE_DATA->dev_id;
					CONNECTION_PROPERTY->state = INDIGO_OK_STATE;

					/* initialize info prop */
					int vendor_id = 0; // guess_mount_vendor(dev_id);
					/* if (vendor_id < 0) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "guess_mount_vendor(%d) = %d", dev_id, vendor_id);
					} else if (vendor_id == VNDR_SKYWATCHER) {
						strncpy(MOUNT_INFO_VENDOR_ITEM->text.value, "Sky-Watcher", INDIGO_VALUE_SIZE);
					} else if (vendor_id == VNDR_CELESTRON) {
						strncpy(MOUNT_INFO_VENDOR_ITEM->text.value, "Celestron", INDIGO_VALUE_SIZE);
					} */
					PRIVATE_DATA->vendor_id = vendor_id;

					int model_id = 0; //tc_get_model(dev_id);
					if (model_id < 0) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_get_model(%d) = %d", dev_id, model_id);
					} else {
						//get_model_name(model_id,MOUNT_INFO_MODEL_ITEM->text.value,  INDIGO_VALUE_SIZE);
					}

					int firmware = 0; //tc_get_version(dev_id, NULL, NULL);
					if (firmware < 0) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_get_version(%d) = %d", dev_id, firmware);
					} else {
						/* if (vendor_id == VNDR_SKYWATCHER) {
							snprintf(MOUNT_INFO_FIRMWARE_ITEM->text.value, INDIGO_VALUE_SIZE, "%2d.%02d.%02d", GET_RELEASE(firmware), GET_REVISION(firmware), GET_PATCH(firmware));
						} else {
							snprintf(MOUNT_INFO_FIRMWARE_ITEM->text.value, INDIGO_VALUE_SIZE, "%2d.%02d", GET_RELEASE(firmware), GET_REVISION(firmware));
						} */
					}

					/* initialize guidingrate prop */
					int offset = 1;                                             /* for Ceslestron 0 is 1% and 99 is 100% */
					//if (PRIVATE_DATA->vendor_id == VNDR_SKYWATCHER) offset = 0; /* there is no offset for Sky-Watcher */

					int st4_ra_rate = 0; //tc_get_autoguide_rate(dev_id, TC_AXIS_RA);
					if (st4_ra_rate < 0) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_get_autoguide_rate(%d) = %d", dev_id, st4_ra_rate);
					} else {
						MOUNT_GUIDE_RATE_RA_ITEM->number.value = st4_ra_rate + offset;
						PRIVATE_DATA->st4_ra_rate = st4_ra_rate + offset;
					}

					int st4_dec_rate = 0; //tc_get_autoguide_rate(dev_id, TC_AXIS_DE);
					if (st4_dec_rate < 0) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_get_autoguide_rate(%d) = %d", dev_id, st4_dec_rate);
					} else {
						MOUNT_GUIDE_RATE_DEC_ITEM->number.value = st4_dec_rate + offset;
						PRIVATE_DATA->st4_dec_rate = st4_dec_rate + offset;
					}

					/* initialize tracking prop */
					int mode = 0; //tc_get_tracking_mode(PRIVATE_DATA->dev_id);
					if (mode < 0) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_get_tracking_mode(%d) = %d", PRIVATE_DATA->dev_id, mode);
					} else if (mode ==  0 /* TC_TRACK_OFF */) {
						MOUNT_TRACKING_OFF_ITEM->sw.value = true;
						MOUNT_TRACKING_ON_ITEM->sw.value = false;
						MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
						indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
					} else {
						MOUNT_TRACKING_OFF_ITEM->sw.value = false;
						MOUNT_TRACKING_ON_ITEM->sw.value = true;
						MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
						indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
					}
					indigo_define_property(device, ALARM_PROPERTY, NULL);
					indigo_define_property(device, OIL_STATE_PROPERTY, NULL);
					indigo_define_property(device, OIMV_PROPERTY, NULL);
					indigo_define_property(device, MOUNT_STATE_PROPERTY, NULL);
					indigo_define_property(device, FLAP_STATE_PROPERTY, NULL);
					indigo_define_property(device, GLME_PROPERTY, NULL);
					indigo_define_property(device, OIL_POWER_PROPERTY, NULL);
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

					device->is_connected = true;
					/* start updates */
					PRIVATE_DATA->position_timer = indigo_set_timer(device, 0, position_timer_callback);
					PRIVATE_DATA->state_timer = indigo_set_timer(device, 0, state_timer_callback);
				} else {
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
				}
			}
		} else {
			if (device->is_connected) {
				indigo_cancel_timer(device, &PRIVATE_DATA->position_timer);
				indigo_cancel_timer(device, &PRIVATE_DATA->state_timer);
				mount_close(device);
				indigo_delete_property(device, ALARM_PROPERTY, NULL);
				indigo_delete_property(device, OIL_STATE_PROPERTY, NULL);
				indigo_delete_property(device, OIMV_PROPERTY, NULL);
				indigo_delete_property(device, MOUNT_STATE_PROPERTY, NULL);
				indigo_delete_property(device, FLAP_STATE_PROPERTY, NULL);
				indigo_delete_property(device, GLME_PROPERTY, NULL);
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
				device->is_connected = false;
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			}
		}
	} else if (indigo_property_match(MOUNT_PARK_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_PARK
		if(PRIVATE_DATA->park_in_progress) {
			indigo_update_property(device, MOUNT_PARK_PROPERTY, WARN_PARKING_PROGRESS_MSG);
			return INDIGO_OK;
		}
		indigo_property_copy_values(MOUNT_PARK_PROPERTY, property, false);
		if (MOUNT_PARK_PARKED_ITEM->sw.value) {
			PRIVATE_DATA->parked = true;  /* a but premature but need to cancel other movements from now on until unparked */
			PRIVATE_DATA->park_in_progress = true;

			pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
			int res = 0; //tc_goto_azalt_p(PRIVATE_DATA->dev_id, 0, 90);
			pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
			if (res != RC_OK) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_goto_azalt_p(%d) = %d", PRIVATE_DATA->dev_id, res);
			}

			MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_PARK_PROPERTY, "Parking...");
			PRIVATE_DATA->park_timer = indigo_set_timer(device, 2, park_timer_callback);
		} else {
			MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_PARK_PROPERTY, "Unparking...");

			pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
			int res = 0; //tc_set_tracking_mode(PRIVATE_DATA->dev_id, TC_TRACK_EQ);
			pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
			if (res != RC_OK) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_set_tracking_mode(%d) = %d", PRIVATE_DATA->dev_id, res);
			} else {
				MOUNT_TRACKING_OFF_ITEM->sw.value = false;
				MOUNT_TRACKING_ON_ITEM->sw.value = true;
				indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
			}

			PRIVATE_DATA->parked = false;
			MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, MOUNT_PARK_PROPERTY, "Mount unparked.");
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_GEOGRAPTHIC_COORDINATES
		if (IS_CONNECTED) {
			indigo_property_copy_values(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, property, false);
			if (MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value < 0)
				MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value += 360;
			if (mount_set_location(device)) {
				MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			indigo_update_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_SET_HOST_TIME_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_SET_HOST_TIME_PROPERTY
		indigo_property_copy_values(MOUNT_SET_HOST_TIME_PROPERTY, property, false);
		if(MOUNT_SET_HOST_TIME_ITEM->sw.value) {
			if(mount_set_utc_from_host(device)) {
				MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		}
		MOUNT_SET_HOST_TIME_ITEM->sw.value = false;
		MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_SET_HOST_TIME_PROPERTY, NULL);
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
		// -------------------------------------------------------------------------------- GUIDMODE_PROPERTY
		if (IS_CONNECTED) {
			indigo_property_copy_values(GUIDE_MODE_PROPERTY, property, false);
			mount_handle_guidemode(device);
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
		mount_handle_coordinates(device);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_UTC_TIME_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_UTC_TIME_PROPERTY
		indigo_property_copy_values(MOUNT_UTC_TIME_PROPERTY, property, false);
		mount_handle_utc(device);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_TRACKING_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_TRACKING
		if(PRIVATE_DATA->parked) {
			indigo_update_property(device, MOUNT_TRACKING_PROPERTY, WARN_PARKED_MSG);
			return INDIGO_OK;
		}
		indigo_property_copy_values(MOUNT_TRACKING_PROPERTY, property, false);
		mount_handle_tracking(device);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_GUIDE_RATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_GUIDE_RATE
		if (IS_CONNECTED) {
			indigo_property_copy_values(MOUNT_GUIDE_RATE_PROPERTY, property, false);
			mount_handle_st4_guiding_rate(device);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_SLEW_RATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_SLEW_RATE
		if (IS_CONNECTED) {
			indigo_property_copy_values(MOUNT_SLEW_RATE_PROPERTY, property, false);
			mount_handle_slew_rate(device);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_MOTION_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_MOTION_NS
		if(PRIVATE_DATA->parked) {
			indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, WARN_PARKED_MSG);
			return INDIGO_OK;
		}
		indigo_property_copy_values(MOUNT_MOTION_DEC_PROPERTY, property, false);
		mount_handle_motion_ns(device);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_MOTION_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_MOTION_WE
		if(PRIVATE_DATA->parked) {
			indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, WARN_PARKED_MSG);
			return INDIGO_OK;
		}
		indigo_property_copy_values(MOUNT_MOTION_RA_PROPERTY, property, false);
		mount_handle_motion_ne(device);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_ABORT_MOTION
		indigo_property_copy_values(MOUNT_ABORT_MOTION_PROPERTY, property, false);
		mount_cancel_slew(device);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_mount_change_property(device, client, property);
}


static indigo_result mount_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);
	indigo_cancel_timer(device, &PRIVATE_DATA->position_timer);
	indigo_cancel_timer(device, &PRIVATE_DATA->state_timer);

	indigo_release_property(ALARM_PROPERTY);
	indigo_release_property(OIL_STATE_PROPERTY);
	indigo_release_property(OIMV_PROPERTY);
	indigo_release_property(MOUNT_STATE_PROPERTY);
	indigo_release_property(FLAP_STATE_PROPERTY);
	indigo_release_property(GLME_PROPERTY);
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
	if (PRIVATE_DATA->dev_id > 0) mount_close(device);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_mount_detach(device);
}


// -------------------------------------------------------------------------------- INDIGO guider device implementation

static indigo_result ascol_guider_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(COMMAND_GUIDE_RATE_PROPERTY, property))
			indigo_define_property(device, COMMAND_GUIDE_RATE_PROPERTY, NULL);
	}
	return indigo_guider_enumerate_properties(device, NULL, NULL);
}


static void guider_timer_callback_ra(indigo_device *device) {
	PRIVATE_DATA->guider_timer_ra = NULL;
	int dev_id = PRIVATE_DATA->dev_id;
	int res;

	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	//res = tc_slew_fixed(dev_id, TC_AXIS_RA, TC_DIR_POSITIVE, 0); // STOP move
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	if (res != RC_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_slew_fixed(%d) = %d", dev_id, res);
	}

	GUIDER_GUIDE_EAST_ITEM->number.value = 0;
	GUIDER_GUIDE_WEST_ITEM->number.value = 0;
	GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
}


static void guider_timer_callback_dec(indigo_device *device) {
	PRIVATE_DATA->guider_timer_dec = NULL;
	int dev_id = PRIVATE_DATA->dev_id;
	int res;

	pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
	//res = tc_slew_fixed(dev_id, TC_AXIS_DE, TC_DIR_POSITIVE, 0); // STOP move
	pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
	if (res != RC_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_slew_fixed(%d) = %d", dev_id, res);
	}

	GUIDER_GUIDE_NORTH_ITEM->number.value = 0;
	GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
	GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
}


static void guider_handle_guide_rate(indigo_device *device) {
	if(GUIDE_50_ITEM->sw.value) {
		PRIVATE_DATA->guide_rate = 1;
	} else if (GUIDE_100_ITEM->sw.value) {
		PRIVATE_DATA->guide_rate = 2;
	}
	COMMAND_GUIDE_RATE_PROPERTY->state = INDIGO_OK_STATE;
	if (PRIVATE_DATA->guide_rate == 1)
		indigo_update_property(device, COMMAND_GUIDE_RATE_PROPERTY, "Command guide rate set to 7.5\"/s (50%% sidereal).");
	else if (PRIVATE_DATA->guide_rate == 2)
		indigo_update_property(device, COMMAND_GUIDE_RATE_PROPERTY, "Command guide rate set to 15\"/s (100%% sidereal).");
	else
		indigo_update_property(device, COMMAND_GUIDE_RATE_PROPERTY, "Command guide rate set.");
}


static indigo_result guider_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_guider_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		PRIVATE_DATA->guide_rate = 1; /* 1 -> 0.5 siderial rate , 2 -> siderial rate */
		COMMAND_GUIDE_RATE_PROPERTY = indigo_init_switch_property(NULL, device->name, COMMAND_GUIDE_RATE_PROPERTY_NAME, GUIDER_MAIN_GROUP, "Guide rate", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (COMMAND_GUIDE_RATE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(GUIDE_50_ITEM, GUIDE_50_ITEM_NAME, "50% sidereal", true);
		indigo_init_switch_item(GUIDE_100_ITEM, GUIDE_100_ITEM_NAME, "100% sidereal", false);

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
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			if (!device->is_connected) {
				if (mount_open(device)) {
					device->is_connected = true;
					indigo_define_property(device, COMMAND_GUIDE_RATE_PROPERTY, NULL);
					PRIVATE_DATA->guider_timer_ra = NULL;
					PRIVATE_DATA->guider_timer_dec = NULL;
					CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
					GUIDER_GUIDE_DEC_PROPERTY->hidden = false;
					GUIDER_GUIDE_RA_PROPERTY->hidden = false;
				} else {
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
				}
			}
		} else {
			if (device->is_connected) {
				mount_close(device);
				indigo_delete_property(device, COMMAND_GUIDE_RATE_PROPERTY, NULL);
				device->is_connected = false;
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			}
		}
	} else if (indigo_property_match(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_DEC
		indigo_cancel_timer(device, &PRIVATE_DATA->guider_timer_dec);
		indigo_property_copy_values(GUIDER_GUIDE_DEC_PROPERTY, property, false);
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
		int duration = GUIDER_GUIDE_NORTH_ITEM->number.value;
		if (duration > 0) {
			pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
			int res = 0; //tc_slew_fixed(PRIVATE_DATA->dev_id, TC_AXIS_DE, TC_DIR_POSITIVE, PRIVATE_DATA->guide_rate);
			pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
			if (res != RC_OK) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_slew_fixed(%d) = %d", PRIVATE_DATA->dev_id, res);
			}
			GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
			PRIVATE_DATA->guider_timer_dec = indigo_set_timer(device, duration/1000.0, guider_timer_callback_dec);
		} else {
			int duration = GUIDER_GUIDE_SOUTH_ITEM->number.value;
			if (duration > 0) {
				pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
				int res = 0; //tc_slew_fixed(PRIVATE_DATA->dev_id, TC_AXIS_DE, TC_DIR_NEGATIVE, PRIVATE_DATA->guide_rate);
				pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
				if (res != RC_OK) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_slew_fixed(%d) = %d", PRIVATE_DATA->dev_id, res);
				}
				GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
				PRIVATE_DATA->guider_timer_dec = indigo_set_timer(device, duration/1000.0, guider_timer_callback_dec);
			}
		}
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDER_GUIDE_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_RA
		indigo_cancel_timer(device, &PRIVATE_DATA->guider_timer_ra);
		indigo_property_copy_values(GUIDER_GUIDE_RA_PROPERTY, property, false);
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
		int duration = GUIDER_GUIDE_EAST_ITEM->number.value;
		if (duration > 0) {
			pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
			int res = 0; //tc_slew_fixed(PRIVATE_DATA->dev_id, TC_AXIS_RA, TC_DIR_POSITIVE, PRIVATE_DATA->guide_rate);
			pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
			if (res != RC_OK) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_slew_fixed(%d) = %d", PRIVATE_DATA->dev_id, res);
			}
			GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
			PRIVATE_DATA->guider_timer_ra = indigo_set_timer(device, duration/1000.0, guider_timer_callback_ra);
		} else {
			int duration = GUIDER_GUIDE_WEST_ITEM->number.value;
			if (duration > 0) {
				pthread_mutex_lock(&PRIVATE_DATA->net_mutex);
				int res = 0; //tc_slew_fixed(PRIVATE_DATA->dev_id, TC_AXIS_RA, TC_DIR_NEGATIVE, PRIVATE_DATA->guide_rate);
				pthread_mutex_unlock(&PRIVATE_DATA->net_mutex);
				if (res != RC_OK) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_slew_fixed(%d) = %d", PRIVATE_DATA->dev_id, res);
				}
				GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
				PRIVATE_DATA->guider_timer_ra = indigo_set_timer(device, duration/1000.0, guider_timer_callback_ra);
			}
		}
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(COMMAND_GUIDE_RATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- COMMAND_GUIDE_RATE
		indigo_property_copy_values(COMMAND_GUIDE_RATE_PROPERTY, property, false);
		guider_handle_guide_rate(device);
		return INDIGO_OK;
	}
	// --------------------------------------------------------------------------------
	return indigo_guider_change_property(device, client, property);
}

static indigo_result guider_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);

	indigo_release_property(COMMAND_GUIDE_RATE_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_guider_detach(device);
}

// --------------------------------------------------------------------------------

static ascol_private_data *private_data = NULL;

static indigo_device *mount = NULL;
static indigo_device *mount_guider = NULL;

indigo_result indigo_system_ascol(indigo_driver_action action, indigo_driver_info *info) {
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
	ascol_debug = 1;

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "2M RCC Telescope", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;
		private_data = malloc(sizeof(ascol_private_data));
		assert(private_data != NULL);
		memset(private_data, 0, sizeof(ascol_private_data));
		private_data->dev_id = -1;
		private_data->count_open = 0;
		mount = malloc(sizeof(indigo_device));
		assert(mount != NULL);
		memcpy(mount, &mount_template, sizeof(indigo_device));
		mount->private_data = private_data;
		indigo_attach_device(mount);
		mount_guider = malloc(sizeof(indigo_device));
		assert(mount_guider != NULL);
		memcpy(mount_guider, &mount_guider_template, sizeof(indigo_device));
		mount_guider->private_data = private_data;
		indigo_attach_device(mount_guider);
		break;

	case INDIGO_DRIVER_SHUTDOWN:
		last_action = action;
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
