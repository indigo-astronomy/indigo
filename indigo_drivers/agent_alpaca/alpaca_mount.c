// Copyright (c) 2021 CloudMakers, s. r. o.
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

/** INDIGO ASCOM ALPACA bridge agent
 \file alpaca_mount.c
 */

#include <indigo/indigo_mount_driver.h>

#include "alpaca_common.h"

static indigo_alpaca_error alpaca_get_interfaceversion(indigo_alpaca_device *device, int version, uint32_t *value) {
	*value = 3;
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_cansetguiderates(indigo_alpaca_device *device, int version, bool *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->mount.cansetguiderates;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_guideratedeclination(indigo_alpaca_device *device, int version, double *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (!device->mount.cansetguiderates) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotImplemented;
	}
	*value = device->mount.guideratedeclination;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_guideraterightascension(indigo_alpaca_device *device, int version, double *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (!device->mount.cansetguiderates) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotImplemented;
	}
	*value = device->mount.guideraterightascension;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_athome(indigo_alpaca_device *device, int version, bool *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->mount.athome;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_atpark(indigo_alpaca_device *device, int version, bool *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->mount.atpark;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_canpark(indigo_alpaca_device *device, int version, bool *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->mount.canpark;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_canslew(indigo_alpaca_device *device, int version, bool *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->mount.canslew;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_cansync(indigo_alpaca_device *device, int version, bool *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->mount.cansync;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_cansettracking(indigo_alpaca_device *device, int version, bool *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->mount.cansettracking;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_siderealtime(indigo_alpaca_device *device, int version, double *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->mount.siderealtime;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_equatorialsystem(indigo_alpaca_device *device, int version, int *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->mount.equatorialsystem;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_declination(indigo_alpaca_device *device, int version, double *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->mount.declination;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_rightascension(indigo_alpaca_device *device, int version, double *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->mount.rightascension;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_altitude(indigo_alpaca_device *device, int version, double *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->mount.altitude;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_azimuth(indigo_alpaca_device *device, int version, double *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->mount.azimuth;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_targetdeclination(indigo_alpaca_device *device, int version, double *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (!device->mount.targetdeclinationset) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidOperation;
	}
	*value = device->mount.targetdeclination;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_targetrightascension(indigo_alpaca_device *device, int version, double *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (!device->mount.targetrightascensionset) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidOperation;
	}
	*value = device->mount.targetrightascension;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_slewing(indigo_alpaca_device *device, int version, bool *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->mount.slewing;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_tracking(indigo_alpaca_device *device, int version, bool *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->mount.tracking;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_trackingrate(indigo_alpaca_device *device, int version, int *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->mount.trackingrate;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_trackingrates(indigo_alpaca_device *device, int version, bool *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	value[0] = device->mount.trackingrates[0];
	value[1] = device->mount.trackingrates[1];
	value[2] = device->mount.trackingrates[2];
	value[3] = device->mount.trackingrates[3];
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_sideofpier(indigo_alpaca_device *device, int version, int *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (device->mount.sideofpier == 0) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotImplemented;
	}
	*value = device->mount.sideofpier - 1;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_alignmentmode(indigo_alpaca_device *device, int version, int *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (device->mount.sideofpier == 0) {
		*value = 1;
	} else {
		*value = 2;
	}
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_set_guideratedeclination(indigo_alpaca_device *device, int version, double value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (!device->mount.cansetguiderates) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotImplemented;
	}
	indigo_change_number_property_1(indigo_agent_alpaca_client, device->indigo_device, MOUNT_GUIDE_RATE_PROPERTY_NAME, MOUNT_GUIDE_RATE_DEC_ITEM_NAME, value);
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_wait_for_double(&device->mount.guideratedeclination, value, 30);
}

static indigo_alpaca_error alpaca_set_guideraterightascension(indigo_alpaca_device *device, int version, double value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (!device->mount.cansetguiderates) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotImplemented;
	}
	indigo_change_number_property_1(indigo_agent_alpaca_client, device->indigo_device, MOUNT_GUIDE_RATE_PROPERTY_NAME, MOUNT_GUIDE_RATE_RA_ITEM_NAME, value);
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_wait_for_double(&device->mount.guideraterightascension, value, 30);
}

static indigo_alpaca_error alpaca_set_targetdeclination(indigo_alpaca_device *device, int version, double value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (value < -90 || value > 90) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidValue;
	}
	device->mount.targetdeclinationset = true;
	device->mount.targetdeclination = value;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_set_targetrightascension(indigo_alpaca_device *device, int version, double value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (value < 0 || value > 24) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidValue;
	}
	device->mount.targetrightascensionset = true;
	device->mount.targetrightascension = value;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_set_tracking(indigo_alpaca_device *device, int version, bool value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	indigo_change_switch_property_1(indigo_agent_alpaca_client, device->indigo_device, MOUNT_TRACKING_PROPERTY_NAME, value ? MOUNT_TRACKING_ON_ITEM_NAME : MOUNT_TRACKING_OFF_ITEM_NAME, true);
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_wait_for_bool(&device->mount.tracking, value, 30);
}

static indigo_alpaca_error alpaca_set_trackingrate(indigo_alpaca_device *device, int version, int value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	switch (value) {
		case 0:
			indigo_change_switch_property_1(indigo_agent_alpaca_client, device->indigo_device, MOUNT_TRACK_RATE_PROPERTY_NAME, MOUNT_TRACK_RATE_SIDEREAL_ITEM_NAME, true);
			break;
		case 1:
			indigo_change_switch_property_1(indigo_agent_alpaca_client, device->indigo_device, MOUNT_TRACK_RATE_PROPERTY_NAME, MOUNT_TRACK_RATE_LUNAR_ITEM_NAME, true);
			break;
		case 2:
			indigo_change_switch_property_1(indigo_agent_alpaca_client, device->indigo_device, MOUNT_TRACK_RATE_PROPERTY_NAME, MOUNT_TRACK_RATE_SOLAR_ITEM_NAME, true);
			break;
		case 3:
			indigo_change_switch_property_1(indigo_agent_alpaca_client, device->indigo_device, MOUNT_TRACK_RATE_PROPERTY_NAME, MOUNT_TRACK_RATE_KING_ITEM_NAME, true);
			break;
		default:
			pthread_mutex_unlock(&device->mutex);
			return indigo_alpaca_error_InvalidValue;
	}
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_wait_for_int32(&device->mount.trackingrate, value, 30);
}

static indigo_alpaca_error alpaca_park(indigo_alpaca_device *device, int version) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (!device->mount.canpark) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotImplemented;
	}
	indigo_change_switch_property_1(indigo_agent_alpaca_client, device->indigo_device, MOUNT_PARK_PROPERTY_NAME, MOUNT_PARK_PARKED_ITEM_NAME, true);
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_unpark(indigo_alpaca_device *device, int version) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (!device->mount.canpark) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotImplemented;
	}
	if (!device->mount.canunpark) {
		device->mount.atpark = false;
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_OK;
	}
	indigo_change_switch_property_1(indigo_agent_alpaca_client, device->indigo_device, MOUNT_PARK_PROPERTY_NAME, MOUNT_PARK_UNPARKED_ITEM_NAME, true);
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_slewtotarget(indigo_alpaca_device *device, int version) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (!device->mount.targetdeclinationset || !device->mount.targetrightascensionset) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidOperation;
	}
	if (device->mount.atpark) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidWhileParked;
	}
	indigo_change_switch_property_1(indigo_agent_alpaca_client, device->indigo_device, MOUNT_ON_COORDINATES_SET_PROPERTY_NAME, MOUNT_ON_COORDINATES_SET_TRACK_ITEM_NAME, true);
	static const char *names[] = { MOUNT_EQUATORIAL_COORDINATES_RA_ITEM_NAME, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM_NAME };
	const double values[] = { device->mount.targetrightascension, device->mount.targetdeclination };
	device->mount.slewing = true;
	indigo_change_number_property(indigo_agent_alpaca_client, device->indigo_device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, 2, names, values);
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_wait_for_bool(&device->mount.slewing, false, 300);
}

static indigo_alpaca_error alpaca_slewtotargetasync(indigo_alpaca_device *device, int version) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (!device->mount.targetdeclinationset || !device->mount.targetrightascensionset) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidOperation;
	}
	if (device->mount.atpark) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidWhileParked;
	}
	indigo_change_switch_property_1(indigo_agent_alpaca_client, device->indigo_device, MOUNT_ON_COORDINATES_SET_PROPERTY_NAME, MOUNT_ON_COORDINATES_SET_TRACK_ITEM_NAME, true);
	static const char *names[] = { MOUNT_EQUATORIAL_COORDINATES_RA_ITEM_NAME, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM_NAME };
	const double values[] = { device->mount.targetrightascension, device->mount.targetdeclination };
	device->mount.slewing = true;
	indigo_change_number_property(indigo_agent_alpaca_client, device->indigo_device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, 2, names, values);
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_synctotarget(indigo_alpaca_device *device, int version) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (!device->mount.targetdeclinationset || !device->mount.targetrightascensionset) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidOperation;
	}
	if (device->mount.atpark) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidWhileParked;
	}
	indigo_change_switch_property_1(indigo_agent_alpaca_client, device->indigo_device, MOUNT_ON_COORDINATES_SET_PROPERTY_NAME, MOUNT_ON_COORDINATES_SET_SYNC_ITEM_NAME, true);
	static const char *names[] = { MOUNT_EQUATORIAL_COORDINATES_RA_ITEM_NAME, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM_NAME };
	const double values[] = { device->mount.targetrightascension, device->mount.targetdeclination };
	indigo_change_number_property(indigo_agent_alpaca_client, device->indigo_device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, 2, names, values);
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_slewtocoordinates(indigo_alpaca_device *device, int version, double rightascension, double declination) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (device->mount.atpark) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidWhileParked;
	}
	if (rightascension < 0 || rightascension > 24) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidValue;
	}
	if (declination < -90 || declination > 90) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidValue;
	}
	device->mount.targetrightascension = rightascension;
	device->mount.targetdeclination = declination;
	indigo_change_switch_property_1(indigo_agent_alpaca_client, device->indigo_device, MOUNT_ON_COORDINATES_SET_PROPERTY_NAME, MOUNT_ON_COORDINATES_SET_TRACK_ITEM_NAME, true);
	static const char *names[] = { MOUNT_EQUATORIAL_COORDINATES_RA_ITEM_NAME, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM_NAME };
	const double values[] = { rightascension, declination };
	device->mount.slewing = true;
	indigo_change_number_property(indigo_agent_alpaca_client, device->indigo_device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, 2, names, values);
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_wait_for_bool(&device->mount.slewing, false, 300);
}

static indigo_alpaca_error alpaca_slewtocoordinatesasync(indigo_alpaca_device *device, int version, double rightascension, double declination) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (device->mount.atpark) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidWhileParked;
	}
	if (rightascension < 0 || rightascension > 24) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidValue;
	}
	if (declination < -90 || declination > 90) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidValue;
	}
	device->mount.targetrightascension = rightascension;
	device->mount.targetdeclination = declination;
	indigo_change_switch_property_1(indigo_agent_alpaca_client, device->indigo_device, MOUNT_ON_COORDINATES_SET_PROPERTY_NAME, MOUNT_ON_COORDINATES_SET_TRACK_ITEM_NAME, true);
	static const char *names[] = { MOUNT_EQUATORIAL_COORDINATES_RA_ITEM_NAME, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM_NAME };
	const double values[] = { rightascension, declination };
	device->mount.slewing = true;
	indigo_change_number_property(indigo_agent_alpaca_client, device->indigo_device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, 2, names, values);
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_synctocoordinates(indigo_alpaca_device *device, int version, double rightascension, double declination) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (device->mount.atpark) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidWhileParked;
	}
	if (rightascension < 0 || rightascension > 24) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidValue;
	}
	if (declination < -90 || declination > 90) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidValue;
	}
	device->mount.targetrightascension = rightascension;
	device->mount.targetdeclination = declination;
	indigo_change_switch_property_1(indigo_agent_alpaca_client, device->indigo_device, MOUNT_ON_COORDINATES_SET_PROPERTY_NAME, MOUNT_ON_COORDINATES_SET_SYNC_ITEM_NAME, true);
	static const char *names[] = { MOUNT_EQUATORIAL_COORDINATES_RA_ITEM_NAME, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM_NAME };
	const double values[] = { rightascension, declination };
	indigo_change_number_property(indigo_agent_alpaca_client, device->indigo_device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, 2, names, values);
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_wait_for_bool(&device->mount.slewing, false, 300);
}

static indigo_alpaca_error alpaca_abortslew(indigo_alpaca_device *device, int version) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (device->mount.atpark) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidWhileParked;
	}
	indigo_change_switch_property_1(indigo_agent_alpaca_client, device->indigo_device, MOUNT_ABORT_MOTION_PROPERTY_NAME, MOUNT_ABORT_MOTION_ITEM_NAME, true);
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_wait_for_bool(&device->mount.slewing, false, 30);
}

void indigo_alpaca_mount_update_property(indigo_alpaca_device *alpaca_device, indigo_property *property) {
	if (!strcmp(property->name, MOUNT_GUIDE_RATE_PROPERTY_NAME)) {
		alpaca_device->mount.cansetguiderates = true;
		if (property->state == INDIGO_OK_STATE) {
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				if (!strcmp(item->name, MOUNT_GUIDE_RATE_DEC_ITEM_NAME)) {
					alpaca_device->mount.guideratedeclination = item->number.value;
				} else if (!strcmp(item->name, MOUNT_GUIDE_RATE_RA_ITEM_NAME)) {
					alpaca_device->mount.guideraterightascension = item->number.value;
					if (property->count == 1)
						alpaca_device->mount.guideratedeclination = item->number.value;
				}
			}
		}
	} else if (!strcmp(property->name, MOUNT_PARK_PROPERTY_NAME)) {
		alpaca_device->mount.canpark = true;
		if (property->state == INDIGO_OK_STATE) {
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				if (!strcmp(item->name, MOUNT_PARK_PARKED_ITEM_NAME)) {
					alpaca_device->mount.atpark = item->sw.value;
					alpaca_device->mount.canpark = true;
				} else if (!strcmp(item->name, MOUNT_PARK_UNPARKED_ITEM_NAME)) {
					alpaca_device->mount.canunpark = true;
				}
			}
		}
	} else if (!strcmp(property->name, MOUNT_LST_TIME_PROPERTY_NAME)) {
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = property->items + i;
			if (!strcmp(item->name, MOUNT_LST_TIME_ITEM_NAME)) {
				alpaca_device->mount.siderealtime = item->number.value;
			}
		}
	} else if (!strcmp(property->name, MOUNT_EPOCH_PROPERTY_NAME)) {
		if (property->state == INDIGO_OK_STATE) {
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				if (!strcmp(item->name, MOUNT_EPOCH_ITEM_NAME)) {
					switch ((int)item->number.value) {
						case 0:
							alpaca_device->mount.equatorialsystem = 1;
							break;
							break;
						case 2000:
							alpaca_device->mount.equatorialsystem = 2;
							break;
						case 2050:
							alpaca_device->mount.equatorialsystem = 3;
							break;
						case 1950:
							alpaca_device->mount.equatorialsystem = 4;
							break;
						default:
							alpaca_device->mount.equatorialsystem = 0;
							break;
					}
				}
			}
		}
	} else if (!strcmp(property->name, MOUNT_ON_COORDINATES_SET_PROPERTY_NAME)) {
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = property->items + i;
			if (!strcmp(item->name, MOUNT_ON_COORDINATES_SET_TRACK_ITEM_NAME)) {
				alpaca_device->mount.canslew = true;
			} else if (!strcmp(item->name, MOUNT_ON_COORDINATES_SET_SYNC_ITEM_NAME)) {
				alpaca_device->mount.cansync = true;
			}
		}
	} else if (!strcmp(property->name, MOUNT_TRACKING_PROPERTY_NAME)) {
		alpaca_device->mount.cansettracking = property->perm == INDIGO_RW_PERM;
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = property->items + i;
			if (!strcmp(item->name, MOUNT_TRACKING_ON_ITEM_NAME)) {
				alpaca_device->mount.tracking = item->sw.value;
			}
		}
	} else if (!strcmp(property->name, MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME)) {
		alpaca_device->mount.slewing = property->state == INDIGO_BUSY_STATE;
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = property->items + i;
			if (!strcmp(item->name, MOUNT_EQUATORIAL_COORDINATES_RA_ITEM_NAME)) {
				alpaca_device->mount.rightascension = item->number.value;
			} else if (!strcmp(item->name, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM_NAME)) {
				alpaca_device->mount.declination = item->number.value;
			}
		}
	} else if (!strcmp(property->name, MOUNT_HORIZONTAL_COORDINATES_PROPERTY_NAME)) {
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = property->items + i;
			if (!strcmp(item->name, MOUNT_HORIZONTAL_COORDINATES_ALT_ITEM_NAME)) {
				alpaca_device->mount.altitude = item->number.value;
			} else if (!strcmp(item->name, MOUNT_HORIZONTAL_COORDINATES_AZ_ITEM_NAME)) {
				alpaca_device->mount.azimuth = item->number.value;
			}
		}
	} else if (!strcmp(property->name, MOUNT_TRACK_RATE_PROPERTY_NAME)) {
		if (property->state == INDIGO_OK_STATE) {
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				if (!strcmp(item->name, MOUNT_TRACK_RATE_SIDEREAL_ITEM_NAME)) {
					if (item->sw.value)
						alpaca_device->mount.trackingrate = 0;
					alpaca_device->mount.trackingrates[0] = true;
				} else if (!strcmp(item->name, MOUNT_TRACK_RATE_LUNAR_ITEM_NAME)) {
					if (item->sw.value)
						alpaca_device->mount.trackingrate = 1;
					alpaca_device->mount.trackingrates[1] = true;
				} else if (!strcmp(item->name, MOUNT_TRACK_RATE_SOLAR_ITEM_NAME)) {
					if (item->sw.value)
						alpaca_device->mount.trackingrate = 2;
					alpaca_device->mount.trackingrates[2] = true;
				} else if (!strcmp(item->name, MOUNT_TRACK_RATE_KING_ITEM_NAME)) {
					if (item->sw.value)
						alpaca_device->mount.trackingrate = 3;
					alpaca_device->mount.trackingrates[3] = true;
				}
			}
		}
	} else if (!strcmp(property->name, MOUNT_SIDE_OF_PIER_PROPERTY_NAME)) {
		if (property->state == INDIGO_OK_STATE) {
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				if (!strcmp(item->name, MOUNT_SIDE_OF_PIER_WEST_ITEM_NAME)) {
					alpaca_device->mount.sideofpier = item->sw.value ? 2 : 1;
				}
			}
		}
	}
}

long indigo_alpaca_mount_get_command(indigo_alpaca_device *alpaca_device, int version, char *command, char *buffer, long buffer_length) {
	if (!strcmp(command, "supportedactions")) {
		return snprintf(buffer, buffer_length, "\"Value\": [ ], \"ErrorNumber\": 0, \"ErrorMessage\": \"\"");
	}
	if (!strcmp(command, "interfaceversion")) {
		uint32_t value;
		indigo_alpaca_error result = alpaca_get_interfaceversion(alpaca_device, version, &value);
	return indigo_alpaca_append_value_int(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "cansetguiderates")) {
		bool value = false;
		indigo_alpaca_error result = alpaca_get_cansetguiderates(alpaca_device, version, &value);
		return indigo_alpaca_append_value_bool(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "guideratedeclination")) {
		double value = false;
		indigo_alpaca_error result = alpaca_get_guideratedeclination(alpaca_device, version, &value);
	return indigo_alpaca_append_value_double(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "guideraterightascension")) {
		double value = false;
		indigo_alpaca_error result = alpaca_get_guideraterightascension(alpaca_device, version, &value);
	return indigo_alpaca_append_value_double(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "athome")) {
		bool value = false;
		indigo_alpaca_error result = alpaca_get_athome(alpaca_device, version, &value);
		return indigo_alpaca_append_value_bool(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "atpark")) {
		bool value = false;
		indigo_alpaca_error result = alpaca_get_atpark(alpaca_device, version, &value);
		return indigo_alpaca_append_value_bool(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "canpark") || !strcmp(command, "canunpark")) {
		bool value = false;
		indigo_alpaca_error result = alpaca_get_canpark(alpaca_device, version, &value);
		return indigo_alpaca_append_value_bool(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "canslew") || !strcmp(command, "canslewasync")) {
		bool value = false;
		indigo_alpaca_error result = alpaca_get_canslew(alpaca_device, version, &value);
		return indigo_alpaca_append_value_bool(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "cansync")) {
		bool value = false;
		indigo_alpaca_error result = alpaca_get_cansync(alpaca_device, version, &value);
		return indigo_alpaca_append_value_bool(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "cansettracking")) {
		bool value = false;
		indigo_alpaca_error result = alpaca_get_cansettracking(alpaca_device, version, &value);
		return indigo_alpaca_append_value_bool(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "siderealtime")) {
		double value = 0;
		indigo_alpaca_error result = alpaca_get_siderealtime(alpaca_device, version, &value);
	return indigo_alpaca_append_value_double(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "equatorialsystem")) {
		int value = 0;
		indigo_alpaca_error result = alpaca_get_equatorialsystem(alpaca_device, version, &value);
	return indigo_alpaca_append_value_int(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "declination")) {
		double value = 0;
		indigo_alpaca_error result = alpaca_get_declination(alpaca_device, version, &value);
	return indigo_alpaca_append_value_double(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "rightascension")) {
		double value = 0;
		indigo_alpaca_error result = alpaca_get_rightascension(alpaca_device, version, &value);
	return indigo_alpaca_append_value_double(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "altitude")) {
		double value = 0;
		indigo_alpaca_error result = alpaca_get_altitude(alpaca_device, version, &value);
	return indigo_alpaca_append_value_double(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "azimuth")) {
		double value = 0;
		indigo_alpaca_error result = alpaca_get_azimuth(alpaca_device, version, &value);
	return indigo_alpaca_append_value_double(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "targetdeclination")) {
		double value = 0;
		indigo_alpaca_error result = alpaca_get_targetdeclination(alpaca_device, version, &value);
	return indigo_alpaca_append_value_double(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "targetrightascension")) {
		double value = 0;
		indigo_alpaca_error result = alpaca_get_targetrightascension(alpaca_device, version, &value);
	return indigo_alpaca_append_value_double(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "slewing")) {
		bool value = false;
		indigo_alpaca_error result = alpaca_get_slewing(alpaca_device, version, &value);
		return indigo_alpaca_append_value_bool(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "tracking")) {
		bool value = false;
		indigo_alpaca_error result = alpaca_get_tracking(alpaca_device, version, &value);
		return indigo_alpaca_append_value_bool(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "trackingrate")) {
		int value = false;
		indigo_alpaca_error result = alpaca_get_trackingrate(alpaca_device, version, &value);
	return indigo_alpaca_append_value_int(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "trackingrates")) {
		bool value[4] = { false };
		indigo_alpaca_error result = alpaca_get_trackingrates(alpaca_device, version, value);
		if (value[0])
			return snprintf(buffer, buffer_length, "\"Value\": [ 0%s%s%s ], \"ErrorNumber\": %d, \"ErrorMessage\": \"%s\"", value[0] ? ", 1" : "", value[1] ? ", 2" : "", value[3] ? ", 3" : "", result, indigo_alpaca_error_string(result));
		return snprintf(buffer, buffer_length, "\"Value\": [ ], \"ErrorNumber\": 0, \"ErrorMessage\": \"\"");
	}
	if (!strcmp(command, "sideofpier")) {
		int value = false;
		indigo_alpaca_error result = alpaca_get_sideofpier(alpaca_device, version, &value);
	return indigo_alpaca_append_value_int(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "alignmentmode")) {
		int value = false;
		indigo_alpaca_error result = alpaca_get_alignmentmode(alpaca_device, version, &value);
	return indigo_alpaca_append_value_int(buffer, buffer_length, value, result);
	}
//--------------- unfinished
	if (!strncmp(command, "can", 3)) {
		return snprintf(buffer, buffer_length, "\"Value\": false, \"ErrorNumber\": 0, \"ErrorMessage\": \"\"");
	}
	if (!strcmp(command, "axisrates")) {
		return snprintf(buffer, buffer_length, "\"Value\": [ ], \"ErrorNumber\": 0, \"ErrorMessage\": \"\"");
	}
	if (!strcmp(command, "declinationrate")) {
		return snprintf(buffer, buffer_length, "\"Value\": 0, \"ErrorNumber\": 0, \"ErrorMessage\": \"\"");
	}
	if (!strcmp(command, "rightascensionrate")) {
		return snprintf(buffer, buffer_length, "\"Value\": 0, \"ErrorNumber\": 0, \"ErrorMessage\": \"\"");
	}
	return snprintf(buffer, buffer_length, "\"ErrorNumber\": %d, \"ErrorMessage\": \"%s\"", indigo_alpaca_error_NotImplemented, indigo_alpaca_error_string(indigo_alpaca_error_NotImplemented));
}

long indigo_alpaca_mount_set_command(indigo_alpaca_device *alpaca_device, int version, char *command, char *buffer, long buffer_length, char *param_1, char *param_2) {
	if (!strcmp(command, "guideratedeclination")) {
		double value = 0;
		indigo_alpaca_error result;
		if (sscanf(param_1, "GuideRateDeclination=%lf", &value) == 1)
			result = alpaca_set_guideratedeclination(alpaca_device, version, value);
		else
			result = indigo_alpaca_error_InvalidValue;
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	if (!strcmp(command, "guideraterightascension")) {
		double value = 0;
		indigo_alpaca_error result;
		if (sscanf(param_1, "GuideRateRightAscension=%lf", &value) == 1)
			result = alpaca_set_guideraterightascension(alpaca_device, version, value);
		else
			result = indigo_alpaca_error_InvalidValue;
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	if (!strcmp(command, "tracking")) {
		bool value = !strcasecmp(param_1, "Tracking=true");
		indigo_alpaca_error result = alpaca_set_tracking(alpaca_device, version, value);
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	if (!strcmp(command, "trackingrate")) {
		int value = 0;
		indigo_alpaca_error result;
		if (sscanf(param_1, "TrackingRate=%d", &value) == 1)
			result = alpaca_set_trackingrate(alpaca_device, version, value);
		else
			result = indigo_alpaca_error_InvalidValue;
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	if (!strcmp(command, "targetdeclination")) {
		double value = 0;
		indigo_alpaca_error result;
		if (sscanf(param_1, "TargetDeclination=%lf", &value) == 1)
			result = alpaca_set_targetdeclination(alpaca_device, version, value);
		else
			result = indigo_alpaca_error_InvalidValue;
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	if (!strcmp(command, "targetrightascension")) {
		double value = 0;
		indigo_alpaca_error result;
		if (sscanf(param_1, "TargetRightAscension=%lf", &value) == 1)
			result = alpaca_set_targetrightascension(alpaca_device, version, value);
		else
			result = indigo_alpaca_error_InvalidValue;
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	if (!strcmp(command, "park")) {
		indigo_alpaca_error result = alpaca_park(alpaca_device, version);
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	if (!strcmp(command, "unpark")) {
		indigo_alpaca_error result = alpaca_unpark(alpaca_device, version);
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	if (!strcmp(command, "abortslew")) {
		indigo_alpaca_error result = alpaca_abortslew(alpaca_device, version);
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	if (!strcmp(command, "slewtotarget")) {
		indigo_alpaca_error result = alpaca_slewtotarget(alpaca_device, version);
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	if (!strcmp(command, "slewtotargetasync")) {
		indigo_alpaca_error result = alpaca_slewtotargetasync(alpaca_device, version);
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	if (!strcmp(command, "synctotarget")) {
		indigo_alpaca_error result = alpaca_synctotarget(alpaca_device, version);
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	if (!strcmp(command, "slewtocoordinates")) {
		double rightascension = 0;
		double declination = 0;
		indigo_alpaca_error result;
		if (sscanf(param_1, "Declination=%lf", &declination) == 1 && sscanf(param_2, "RightAscension=%lf", &rightascension) == 1)
			result = alpaca_slewtocoordinates(alpaca_device, version, rightascension, declination);
		else
			result = indigo_alpaca_error_InvalidValue;
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	if (!strcmp(command, "slewtocoordinatesasync")) {
		double rightascension = 0;
		double declination = 0;
		indigo_alpaca_error result;
		if (sscanf(param_1, "Declination=%lf", &declination) == 1 && sscanf(param_2, "RightAscension=%lf", &rightascension) == 1)
			result = alpaca_slewtocoordinatesasync(alpaca_device, version, rightascension, declination);
		else
			result = indigo_alpaca_error_InvalidValue;
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	if (!strcmp(command, "synctocoordinates")) {
		double rightascension = 0;
		double declination = 0;
		indigo_alpaca_error result;
		if (sscanf(param_1, "Declination=%lf", &declination) == 1 && sscanf(param_2, "RightAscension=%lf", &rightascension) == 1)
			result = alpaca_synctocoordinates(alpaca_device, version, rightascension, declination);
		else
			result = indigo_alpaca_error_InvalidValue;
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	return snprintf(buffer, buffer_length, "\"ErrorNumber\": %d, \"ErrorMessage\": \"%s\"", indigo_alpaca_error_NotImplemented, indigo_alpaca_error_string(indigo_alpaca_error_NotImplemented));
}
