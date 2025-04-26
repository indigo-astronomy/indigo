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
 \file alpaca_ccd.c
 */

#include <math.h>
#include <zlib.h>
#if defined(INDIGO_WINDOWS)
#include <io.h>
#include <fcntl.h>
#endif

#include <indigo/indigo_ccd_driver.h>

#include "indigo_alpaca_common.h"

static indigo_alpaca_error alpaca_get_interfaceversion(indigo_alpaca_device *device, int version, int *value) {
	*value = 3;
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_canabortexposure(indigo_alpaca_device *device, int version, bool *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->ccd.canabortexposure;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_cangetcoolerpower(indigo_alpaca_device *device, int version, bool *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->ccd.cangetcoolerpower;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_cansetccdtemperature(indigo_alpaca_device *device, int version, bool *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->ccd.cansetccdtemperature;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_binx(indigo_alpaca_device *device, int version, int *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = 1;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_biny(indigo_alpaca_device *device, int version, int *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = 1;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_maxbinx(indigo_alpaca_device *device, int version, int *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = 1;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_maxbiny(indigo_alpaca_device *device, int version, int *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = 1;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_camerastate(indigo_alpaca_device *device, int version, int *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->ccd.camerastate;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_cameraxsize(indigo_alpaca_device *device, int version, int *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->ccd.cameraxsize;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_cameraysize(indigo_alpaca_device *device, int version, int *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->ccd.cameraysize;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_startx(indigo_alpaca_device *device, int version, int *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->ccd.startx;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_starty(indigo_alpaca_device *device, int version, int *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->ccd.starty;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_numx(indigo_alpaca_device *device, int version, int *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->ccd.numx;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_numy(indigo_alpaca_device *device, int version, int *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->ccd.numy;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_ccdtemperature(indigo_alpaca_device *device, int version, double *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (isnan(device->ccd.ccdtemperature)) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotImplemented;
	}
	*value = device->ccd.ccdtemperature;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_heatsinktemperature(indigo_alpaca_device *device, int version, double *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_NotImplemented;
}

static indigo_alpaca_error alpaca_get_setccdtemperature(indigo_alpaca_device *device, int version, double *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (isnan(device->ccd.ccdtemperature)) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotImplemented;
	}
	*value = device->ccd.setccdtemperature;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_cooleron(indigo_alpaca_device *device, int version, bool *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (!device->ccd.cansetccdtemperature) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotImplemented;
	}
	*value = device->ccd.cooleron;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_coolerpower(indigo_alpaca_device *device, int version, double *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (!device->ccd.cangetcoolerpower) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotImplemented;
	}
	*value = device->ccd.coolerpower;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_electronsperadu(indigo_alpaca_device *device, int version, double *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->ccd.electronsperadu;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_fullwellcapacity(indigo_alpaca_device *device, int version, double *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->ccd.fullwellcapacity;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_hasshutter(indigo_alpaca_device *device, int version, bool *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = false;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_imageready(indigo_alpaca_device *device, int version, bool *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->ccd.imageready != NULL;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_ispulseguiding(indigo_alpaca_device *device, int version, bool *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = false;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_maxadu(indigo_alpaca_device *device, int version, int *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->ccd.maxadu;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_pixelsizex(indigo_alpaca_device *device, int version, double *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->ccd.pixelsizex;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_pixelsizey(indigo_alpaca_device *device, int version, double *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->ccd.pixelsizey;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_sensortype(indigo_alpaca_device *device, int version, int *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (!strncasecmp(device->ccd.readoutmodes_names[device->ccd.readoutmode], "rgb", 3)) {
		*value = 1;
	} else {
		bool is_bayered = get_bayer_RGGB_offsets(device->ccd.bayer_matrix->text.value, NULL, NULL);
		*value = is_bayered ? 2 : 0;
	}
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_sensorname(indigo_alpaca_device *device, int version, char **value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = "";
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_bayeroffsetx(indigo_alpaca_device *device, int version, int *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = 0;
	bool is_bayered = get_bayer_RGGB_offsets(device->ccd.bayer_matrix->text.value, value, NULL);
	pthread_mutex_unlock(&device->mutex);
	return is_bayered ? indigo_alpaca_error_OK : indigo_alpaca_error_NotImplemented;
}

static indigo_alpaca_error alpaca_get_bayeroffsety(indigo_alpaca_device *device, int version, int *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = 0;
	bool is_bayered = get_bayer_RGGB_offsets(device->ccd.bayer_matrix->text.value, NULL, value);
	pthread_mutex_unlock(&device->mutex);
	return is_bayered ? indigo_alpaca_error_OK : indigo_alpaca_error_NotImplemented;
}

static indigo_alpaca_error alpaca_get_exposuremin(indigo_alpaca_device *device, int version, double *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->ccd.exposuremin;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_exposuremax(indigo_alpaca_device *device, int version, double *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->ccd.exposuremax;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_exposureresolution(indigo_alpaca_device *device, int version, double *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = 0;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_lastexposureduration(indigo_alpaca_device *device, int version, double *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (*device->ccd.lastexposuretarttime == 0) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidOperation;
	}
	*value = device->ccd.lastexposureduration;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_lastexposurestarttime(indigo_alpaca_device *device, int version, char **value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (*device->ccd.lastexposuretarttime == 0) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidOperation;
	}
	*value = device->ccd.lastexposuretarttime;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_gainmin(indigo_alpaca_device *device, int version, int *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (device->ccd.gainmin == 0 && device->ccd.gainmax == 0) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotImplemented;
	}
	*value = device->ccd.gainmin;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_gainmax(indigo_alpaca_device *device, int version, int *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (device->ccd.gainmin == 0 && device->ccd.gainmax == 0) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotImplemented;
	}
	*value = device->ccd.gainmax;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_gain(indigo_alpaca_device *device, int version, int *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (device->ccd.gainmin == 0 && device->ccd.gainmax == 0) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotImplemented;
	}
	*value = device->ccd.gain;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_offsetmin(indigo_alpaca_device *device, int version, int *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (device->ccd.offsetmin == 0 && device->ccd.offsetmax == 0) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotImplemented;
	}
	*value = device->ccd.offsetmin;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_offsetmax(indigo_alpaca_device *device, int version, int *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (device->ccd.offsetmin == 0 && device->ccd.offsetmax == 0) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotImplemented;
	}
	*value = device->ccd.offsetmax;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_offset(indigo_alpaca_device *device, int version, int *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (device->ccd.offsetmin == 0 && device->ccd.offsetmax == 0) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotImplemented;
	}
	*value = device->ccd.offset;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_readoutmode(indigo_alpaca_device *device, int version, int *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->ccd.readoutmode;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_readoutmodes(indigo_alpaca_device *device, int version, char ***value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->ccd.readoutmodes_labels;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_set_binx(indigo_alpaca_device *device, int version, int value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (value != 1) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidValue;
	}
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_set_biny(indigo_alpaca_device *device, int version, int value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (value != 1) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidValue;
	}
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_set_setccdtemperature(indigo_alpaca_device *device, int version, double value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (!device->ccd.cansetccdtemperature) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotImplemented;
	}
	if (value < -273 || value > 50) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidValue;
	}
	indigo_change_number_property_1(indigo_agent_alpaca_client, device->indigo_device, CCD_TEMPERATURE_PROPERTY_NAME, CCD_TEMPERATURE_ITEM_NAME, value);
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_set_cooleron(indigo_alpaca_device *device, int version, bool value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (!device->ccd.cansetccdtemperature) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotImplemented;
	}
	indigo_change_switch_property_1(indigo_agent_alpaca_client, device->indigo_device, CCD_COOLER_PROPERTY_NAME, CCD_COOLER_ON_ITEM_NAME, value);
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_set_startx(indigo_alpaca_device *device, int version, int value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
//	if (value < 0 || value >= device->ccd.cameraxsize) {
//		pthread_mutex_unlock(&device->mutex);
//		return indigo_alpaca_error_InvalidValue;
//	}
//	indigo_change_number_property_1(indigo_agent_alpaca_client, device->indigo_device, CCD_FRAME_PROPERTY_NAME, CCD_FRAME_LEFT_ITEM_NAME, value);
	device->ccd.startx = value;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_set_starty(indigo_alpaca_device *device, int version, int value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
//	if (value < 0 || value >= device->ccd.cameraysize) {
//		pthread_mutex_unlock(&device->mutex);
//		return indigo_alpaca_error_InvalidValue;
//	}
//	indigo_change_number_property_1(indigo_agent_alpaca_client, device->indigo_device, CCD_FRAME_PROPERTY_NAME, CCD_FRAME_TOP_ITEM_NAME, value);
	device->ccd.starty = value;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_set_numx(indigo_alpaca_device *device, int version, int value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
//	if (value <= 0 || value > device->ccd.cameraxsize) {
//		pthread_mutex_unlock(&device->mutex);
//		return indigo_alpaca_error_InvalidValue;
//	}
//	indigo_change_number_property_1(indigo_agent_alpaca_client, device->indigo_device, CCD_FRAME_PROPERTY_NAME, CCD_FRAME_WIDTH_ITEM_NAME, value);
	device->ccd.numx = value;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_set_numy(indigo_alpaca_device *device, int version, int value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
//	if (value <= 0 || value > device->ccd.cameraysize) {
//		pthread_mutex_unlock(&device->mutex);
//		return indigo_alpaca_error_InvalidValue;
//	}
//	indigo_change_number_property_1(indigo_agent_alpaca_client, device->indigo_device, CCD_FRAME_PROPERTY_NAME, CCD_FRAME_HEIGHT_ITEM_NAME, value);
	device->ccd.numy = value;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_set_gain(indigo_alpaca_device *device, int version, int value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (device->ccd.offsetmin == 0 && device->ccd.offsetmax == 0) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotImplemented;
	}
	if (value < device->ccd.gainmin || value > device->ccd.gainmax) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidValue;
	}
	indigo_change_number_property_1(indigo_agent_alpaca_client, device->indigo_device, CCD_GAIN_PROPERTY_NAME, CCD_GAIN_ITEM_NAME, value);
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_set_offset(indigo_alpaca_device *device, int version, int value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (device->ccd.offsetmin == 0 && device->ccd.offsetmax == 0) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotImplemented;
	}
	if (value < device->ccd.offsetmin || value > device->ccd.offsetmax) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidValue;
	}
	indigo_change_number_property_1(indigo_agent_alpaca_client, device->indigo_device, CCD_OFFSET_PROPERTY_NAME, CCD_OFFSET_ITEM_NAME, value);
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_set_readoutmode(indigo_alpaca_device *device, int version, int value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (value < 0 || value > ALPACA_MAX_ITEMS || device->ccd.readoutmodes_names[value] == NULL) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidValue;
	}
	indigo_change_switch_property_1(indigo_agent_alpaca_client, device->indigo_device, CCD_MODE_PROPERTY_NAME, device->ccd.readoutmodes_names[value], true);
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_startexposure(indigo_alpaca_device *device, int version, double duration, bool light) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (duration < 0 || duration > device->ccd.exposuremax) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidValue;
	}
	if (duration < device->ccd.exposuremin && light) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidValue;
	}
	if (device->ccd.startx < 0 || device->ccd.startx >= device->ccd.cameraxsize || device->ccd.starty < 0 || device->ccd.starty >= device->ccd.cameraysize) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidValue;
	}
	if (device->ccd.numx <= 0 || device->ccd.numx > device->ccd.cameraxsize || device->ccd.numy <= 0 || device->ccd.numy > device->ccd.cameraysize) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidValue;
	}
	device->ccd.imageready = NULL;
	device->ccd.camerastate = 2;
	device->ccd.lastexposureduration = duration;
	struct tm* tm_info;
	time_t timer;
	time(&timer);
	tm_info = gmtime(&timer);
	strftime(device->ccd.lastexposuretarttime, sizeof(device->ccd.lastexposuretarttime), "%Y-%m-%dT%H:%M:%S", tm_info);
	static const char *names[] = { CCD_FRAME_LEFT_ITEM_NAME, CCD_FRAME_TOP_ITEM_NAME, CCD_FRAME_WIDTH_ITEM_NAME, CCD_FRAME_HEIGHT_ITEM_NAME };
	const double values[] = { device->ccd.startx, device->ccd.starty, device->ccd.numx, device->ccd.numy };
	indigo_change_number_property(indigo_agent_alpaca_client, device->indigo_device, CCD_FRAME_PROPERTY_NAME, 4, names, values);
	indigo_change_switch_property_1(indigo_agent_alpaca_client, device->indigo_device, CCD_IMAGE_FORMAT_PROPERTY_NAME, CCD_IMAGE_FORMAT_RAW_ITEM_NAME, true);
	indigo_change_switch_property_1(indigo_agent_alpaca_client, device->indigo_device, CCD_FRAME_TYPE_PROPERTY_NAME, light ? CCD_FRAME_TYPE_LIGHT_ITEM_NAME : CCD_FRAME_TYPE_DARK_ITEM_NAME, true);
	indigo_change_number_property_1(indigo_agent_alpaca_client, device->indigo_device, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, duration);
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_abortexposure(indigo_alpaca_device *device, int version) {
	indigo_change_switch_property_1(indigo_agent_alpaca_client, device->indigo_device, CCD_ABORT_EXPOSURE_PROPERTY_NAME, CCD_ABORT_EXPOSURE_ITEM_NAME, true);
	return indigo_alpaca_wait_for_int32(&device->ccd.camerastate, 0, 30);
}

void indigo_alpaca_ccd_update_property(indigo_alpaca_device *alpaca_device, indigo_property *property) {
	if (!strcmp(property->name, CCD_BIN_PROPERTY_NAME)) {
		if (property->state == INDIGO_OK_STATE) {
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				if (!strcmp(item->name, CCD_BIN_HORIZONTAL_ITEM_NAME)) {
					alpaca_device->ccd.binx = (int)item->number.value;
				} else if (!strcmp(item->name, CCD_BIN_VERTICAL_ITEM_NAME)) {
					alpaca_device->ccd.biny = (int)item->number.value;
				}
			}
		}
	} else if (!strcmp(property->name, CCD_ABORT_EXPOSURE_PROPERTY_NAME)) {
		alpaca_device->ccd.canabortexposure = true;
	} else if (!strcmp(property->name, CCD_COOLER_PROPERTY_NAME)) {
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = property->items + i;
			if (!strcmp(item->name, CCD_COOLER_ON_ITEM_NAME)) {
				alpaca_device->ccd.cooleron = item->sw.value;
			}
		}
	} else if (!strcmp(property->name, CCD_COOLER_POWER_PROPERTY_NAME)) {
		alpaca_device->ccd.cangetcoolerpower = true;
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = property->items + i;
			if (!strcmp(item->name, CCD_COOLER_POWER_ITEM_NAME)) {
				alpaca_device->ccd.coolerpower = item->number.value;
			}
		}

	} else if (!strcmp(property->name, CCD_TEMPERATURE_PROPERTY_NAME)) {
		alpaca_device->ccd.cansetccdtemperature = property->perm == INDIGO_RW_PERM;
		if (property->state != INDIGO_ALERT_STATE) {
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				if (!strcmp(item->name, CCD_TEMPERATURE_ITEM_NAME)) {
					alpaca_device->ccd.ccdtemperature = item->number.value;
					alpaca_device->ccd.setccdtemperature = item->number.target;
				}
			}
		}
	} else if (!strcmp(property->name, CCD_FRAME_PROPERTY_NAME)) {
		if (property->state == INDIGO_OK_STATE) {
			if (alpaca_device->ccd.binx == 0) {
				alpaca_device->ccd.binx = 1;
			}
			if (alpaca_device->ccd.biny == 0) {
				alpaca_device->ccd.biny = 1;
			}
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				if (!strcmp(item->name, CCD_FRAME_LEFT_ITEM_NAME)) {
					alpaca_device->ccd.startx = (int)(item->number.value / alpaca_device->ccd.binx);
				} else if (!strcmp(item->name, CCD_FRAME_TOP_ITEM_NAME)) {
					alpaca_device->ccd.starty = (int)(item->number.value / alpaca_device->ccd.biny);
				} else if (!strcmp(item->name, CCD_FRAME_WIDTH_ITEM_NAME)) {
					alpaca_device->ccd.numx = (int)(item->number.value / alpaca_device->ccd.binx);
				} else if (!strcmp(item->name, CCD_FRAME_HEIGHT_ITEM_NAME)) {
					alpaca_device->ccd.numy = (int)(item->number.value / alpaca_device->ccd.biny);
				} else if (!strcmp(item->name, CCD_FRAME_BITS_PER_PIXEL_ITEM_NAME)) {
					alpaca_device->ccd.electronsperadu = 1;
					alpaca_device->ccd.fullwellcapacity = pow(2, item->number.value);
				}
			}
		}
	} else if (!strcmp(property->name, CCD_INFO_PROPERTY_NAME)) {
		if (property->state == INDIGO_OK_STATE) {
			if (alpaca_device->ccd.binx == 0) {
				alpaca_device->ccd.binx = 1;
			}
			if (alpaca_device->ccd.biny == 0) {
				alpaca_device->ccd.biny = 1;
			}
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				if (!strcmp(item->name, CCD_INFO_WIDTH_ITEM_NAME)) {
					alpaca_device->ccd.cameraxsize = (int)item->number.value;
					if (alpaca_device->ccd.numx <= 0) {
						alpaca_device->ccd.numx = (int)(item->number.value / alpaca_device->ccd.binx);
					}
				} else if (!strcmp(item->name, CCD_INFO_HEIGHT_ITEM_NAME)) {
					alpaca_device->ccd.cameraysize = (int)item->number.value;
					if (alpaca_device->ccd.numy <= 0) {
						alpaca_device->ccd.numy = (int)(item->number.value / alpaca_device->ccd.biny);
					}
				} else if (!strcmp(item->name, CCD_INFO_PIXEL_WIDTH_ITEM_NAME)) {
					alpaca_device->ccd.pixelsizex = (item->number.value > 0) ? item->number.value : 1;
				} else if (!strcmp(item->name, CCD_INFO_PIXEL_HEIGHT_ITEM_NAME)) {
					alpaca_device->ccd.pixelsizey = (item->number.value > 0) ? item->number.value : 1;
				} else if (!strcmp(item->name, CCD_INFO_BITS_PER_PIXEL_ITEM_NAME)) {
					alpaca_device->ccd.electronsperadu = 1;
					alpaca_device->ccd.maxadu = (int)pow(2, item->number.value);
				}
			}
		}
	} else if (!strcmp(property->name, CCD_IMAGE_PROPERTY_NAME)) {
		if (property->state == INDIGO_OK_STATE) {
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				if (!strcmp(item->name, CCD_IMAGE_ITEM_NAME)) {
					if (item->blob.value && item->blob.size > 0 && *alpaca_device->ccd.lastexposuretarttime) {
						alpaca_device->ccd.imageready = item;
					} else {
						alpaca_device->ccd.imageready = NULL;
					}
				}
			}
		}
	} else if (!strcmp(property->name, CCD_EXPOSURE_PROPERTY_NAME)) {
		alpaca_device->ccd.camerastate = property->state == INDIGO_BUSY_STATE ? 2 : 0;
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = property->items + i;
			if (!strcmp(item->name, CCD_EXPOSURE_ITEM_NAME)) {
				alpaca_device->ccd.exposuremin = item->number.min;
				alpaca_device->ccd.exposuremax = item->number.max;
			}
		}
	} else if (!strcmp(property->name, CCD_GAIN_PROPERTY_NAME)) {
		if (property->state == INDIGO_OK_STATE) {
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				if (!strcmp(item->name, CCD_GAIN_ITEM_NAME)) {
					alpaca_device->ccd.gain = (int)item->number.value;
					alpaca_device->ccd.gainmin = (int)item->number.min;
					alpaca_device->ccd.gainmax = (int)item->number.max;
				}
			}
		}
	} else if (!strcmp(property->name, CCD_OFFSET_PROPERTY_NAME)) {
		if (property->state == INDIGO_OK_STATE) {
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				if (!strcmp(item->name, CCD_OFFSET_ITEM_NAME)) {
					alpaca_device->ccd.offset = (int)item->number.value;
					alpaca_device->ccd.offsetmin = (int)item->number.min;
					alpaca_device->ccd.offsetmax = (int)item->number.max;
				}
			}
		}
	} else if (!strcmp(property->name, CCD_MODE_PROPERTY_NAME)) {
		if (property->state == INDIGO_OK_STATE) {
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				alpaca_device->ccd.readoutmodes_labels[i] = item->label;
				alpaca_device->ccd.readoutmodes_names[i] = item->name;
				if (item->sw.value) {
					alpaca_device->ccd.readoutmode = i;
				}
			}
		}
	}
}

long indigo_alpaca_ccd_get_command(indigo_alpaca_device *alpaca_device, int version, char *command, char *buffer, long buffer_length) {
	if (!strcmp(command, "supportedactions")) {
		return snprintf(buffer, buffer_length, "\"Value\": [ ], \"ErrorNumber\": 0, \"ErrorMessage\": \"\"");
	}
	if (!strcmp(command, "interfaceversion")) {
		int value = 0;
		indigo_alpaca_error result = alpaca_get_interfaceversion(alpaca_device, version, &value);
		return indigo_alpaca_append_value_int(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "canabortexposure")) {
		bool value = false;
		indigo_alpaca_error result = alpaca_get_canabortexposure(alpaca_device, version, &value);
		return indigo_alpaca_append_value_bool(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "cangetcoolerpower")) {
		bool value = false;
		indigo_alpaca_error result = alpaca_get_cangetcoolerpower(alpaca_device, version, &value);
		return indigo_alpaca_append_value_bool(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "cansetccdtemperature")) {
		bool value = false;
		indigo_alpaca_error result = alpaca_get_cansetccdtemperature(alpaca_device, version, &value);
		return indigo_alpaca_append_value_bool(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "binx")) {
		int value = 0;
		indigo_alpaca_error result = alpaca_get_binx(alpaca_device, version, &value);
		return indigo_alpaca_append_value_int(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "biny")) {
		int value = 0;
		indigo_alpaca_error result = alpaca_get_biny(alpaca_device, version, &value);
		return indigo_alpaca_append_value_int(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "maxbinx")) {
		int value = 0;
		indigo_alpaca_error result = alpaca_get_maxbinx(alpaca_device, version, &value);
		return indigo_alpaca_append_value_int(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "maxbiny")) {
		int value = 0;
		indigo_alpaca_error result = alpaca_get_maxbiny(alpaca_device, version, &value);
		return indigo_alpaca_append_value_int(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "camerastate")) {
		int value = 0;
		indigo_alpaca_error result = alpaca_get_camerastate(alpaca_device, version, &value);
		return indigo_alpaca_append_value_int(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "cameraxsize")) {
		int value = 0;
		indigo_alpaca_error result = alpaca_get_cameraxsize(alpaca_device, version, &value);
		return indigo_alpaca_append_value_int(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "cameraysize")) {
		int value = 0;
		indigo_alpaca_error result = alpaca_get_cameraysize(alpaca_device, version, &value);
		return indigo_alpaca_append_value_int(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "startx")) {
		int value = 0;
		indigo_alpaca_error result = alpaca_get_startx(alpaca_device, version, &value);
		return indigo_alpaca_append_value_int(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "starty")) {
		int value = 0;
		indigo_alpaca_error result = alpaca_get_starty(alpaca_device, version, &value);
		return indigo_alpaca_append_value_int(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "numx")) {
		int value = 0;
		indigo_alpaca_error result = alpaca_get_numx(alpaca_device, version, &value);
		return indigo_alpaca_append_value_int(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "numy")) {
		int value = 0;
		indigo_alpaca_error result = alpaca_get_numy(alpaca_device, version, &value);
		return indigo_alpaca_append_value_int(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "ccdtemperature")) {
		double value = 0;
		indigo_alpaca_error result = alpaca_get_ccdtemperature(alpaca_device, version, &value);
		return indigo_alpaca_append_value_double(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "heatsinktemperature")) {
		double value = 0;
		indigo_alpaca_error result = alpaca_get_heatsinktemperature(alpaca_device, version, &value);
		return indigo_alpaca_append_value_double(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "setccdtemperature")) {
		double value = 0;
		indigo_alpaca_error result = alpaca_get_setccdtemperature(alpaca_device, version, &value);
		return indigo_alpaca_append_value_double(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "cooleron")) {
		bool value = false;
		indigo_alpaca_error result = alpaca_get_cooleron(alpaca_device, version, &value);
		return indigo_alpaca_append_value_bool(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "coolerpower")) {
		double value = 0;
		indigo_alpaca_error result = alpaca_get_coolerpower(alpaca_device, version, &value);
		return indigo_alpaca_append_value_double(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "electronsperadu")) {
		double value = 0;
		indigo_alpaca_error result = alpaca_get_electronsperadu(alpaca_device, version, &value);
		return indigo_alpaca_append_value_double(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "fullwellcapacity")) {
		double value = 0;
		indigo_alpaca_error result = alpaca_get_fullwellcapacity(alpaca_device, version, &value);
		return indigo_alpaca_append_value_double(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "hasshutter")) {
		bool value = false;
		indigo_alpaca_error result = alpaca_get_hasshutter(alpaca_device, version, &value);
		return indigo_alpaca_append_value_bool(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "imageready")) {
		bool value = false;
		indigo_alpaca_error result = alpaca_get_imageready(alpaca_device, version, &value);
		return indigo_alpaca_append_value_bool(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "ispulseguiding")) {
		bool value = false;
		indigo_alpaca_error result = alpaca_get_ispulseguiding(alpaca_device, version, &value);
		return indigo_alpaca_append_value_bool(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "maxadu")) {
		int value = 0;
		indigo_alpaca_error result = alpaca_get_maxadu(alpaca_device, version, &value);
		return indigo_alpaca_append_value_int(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "pixelsizex")) {
		double value = 0;
		indigo_alpaca_error result = alpaca_get_pixelsizex(alpaca_device, version, &value);
		return indigo_alpaca_append_value_double(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "pixelsizey")) {
		double value = 0;
		indigo_alpaca_error result = alpaca_get_pixelsizey(alpaca_device, version, &value);
		return indigo_alpaca_append_value_double(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "sensortype")) {
		int value = 0;
		indigo_alpaca_error result = alpaca_get_sensortype(alpaca_device, version, &value);
		return indigo_alpaca_append_value_int(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "sensorname")) {
		char *value = NULL;
		indigo_alpaca_error result = alpaca_get_sensorname(alpaca_device, version, &value);
		return indigo_alpaca_append_value_string(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "bayeroffsetx")) {
		int value = 0;
		indigo_alpaca_error result = alpaca_get_bayeroffsetx(alpaca_device, version, &value);
		return indigo_alpaca_append_value_int(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "bayeroffsety")) {
		int value = 0;
		indigo_alpaca_error result = alpaca_get_bayeroffsety(alpaca_device, version, &value);
		return indigo_alpaca_append_value_int(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "exposuremin")) {
		double value = 0;
		indigo_alpaca_error result = alpaca_get_exposuremin(alpaca_device, version, &value);
		return indigo_alpaca_append_value_double(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "exposuremax")) {
		double value = 0;
		indigo_alpaca_error result = alpaca_get_exposuremax(alpaca_device, version, &value);
		return indigo_alpaca_append_value_double(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "exposureresolution")) {
		double value = 0;
		indigo_alpaca_error result = alpaca_get_exposureresolution(alpaca_device, version, &value);
		return indigo_alpaca_append_value_double(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "lastexposureduration")) {
		double value = 0;
		indigo_alpaca_error result = alpaca_get_lastexposureduration(alpaca_device, version, &value);
		return indigo_alpaca_append_value_double(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "lastexposurestarttime")) {
		char *value = NULL;
		indigo_alpaca_error result = alpaca_get_lastexposurestarttime(alpaca_device, version, &value);
		return indigo_alpaca_append_value_string(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "gainmin")) {
		int value = 0;
		indigo_alpaca_error result = alpaca_get_gainmin(alpaca_device, version, &value);
		return indigo_alpaca_append_value_int(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "gainmax")) {
		int value = 0;
		indigo_alpaca_error result = alpaca_get_gainmax(alpaca_device, version, &value);
		return indigo_alpaca_append_value_int(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "gain")) {
		int value = 0;
		indigo_alpaca_error result = alpaca_get_gain(alpaca_device, version, &value);
		return indigo_alpaca_append_value_int(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "gains")) {
		return snprintf(buffer, buffer_length, "\"Value\": [ ], \"ErrorNumber\": %d, \"ErrorMessage\": \"%s\"", indigo_alpaca_error_NotImplemented, indigo_alpaca_error_string(indigo_alpaca_error_NotImplemented));
	}
	if (!strcmp(command, "offsetmin")) {
		int value = 0;
		indigo_alpaca_error result = alpaca_get_offsetmin(alpaca_device, version, &value);
		return indigo_alpaca_append_value_int(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "offsetmax")) {
		int value = 0;
		indigo_alpaca_error result = alpaca_get_offsetmax(alpaca_device, version, &value);
		return indigo_alpaca_append_value_int(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "offset")) {
		int value = 0;
		indigo_alpaca_error result = alpaca_get_offset(alpaca_device, version, &value);
		return indigo_alpaca_append_value_int(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "offsets")) {
		return snprintf(buffer, buffer_length, "\"Value\": [ ], \"ErrorNumber\": %d, \"ErrorMessage\": \"%s\"", indigo_alpaca_error_NotImplemented, indigo_alpaca_error_string(indigo_alpaca_error_NotImplemented));
	}
	if (!strcmp(command, "readoutmode")) {
		int value = 0;
		indigo_alpaca_error result = alpaca_get_readoutmode(alpaca_device, version, &value);
		return indigo_alpaca_append_value_int(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "readoutmodes")) {
		char **value = NULL;
		indigo_alpaca_error result = alpaca_get_readoutmodes(alpaca_device, version, &value);
		if (result == indigo_alpaca_error_OK) {
			long index = snprintf(buffer, buffer_length, "\"Value\": [ ");
			for (int i = 0; i < ALPACA_MAX_ITEMS; i++) {
				if (value[i] == NULL) {
					break;
				}
				index += snprintf(buffer + index, buffer_length - index, "%s\"%s\"", i == 0 ? "" : ", ", value[i]);
			}
			index += snprintf(buffer + index, buffer_length - index, " ], \"ErrorNumber\": 0, \"ErrorMessage\": \"\"");
			return index;
		} else {
			return indigo_alpaca_append_error(buffer, buffer_length, result);
		}
	}
	if (!strncmp(command, "can", 3)) {
		return snprintf(buffer, buffer_length, "\"Value\": false, \"ErrorNumber\": 0, \"ErrorMessage\": \"\"");
	}
	return snprintf(buffer, buffer_length, "\"ErrorNumber\": %d, \"ErrorMessage\": \"%s\"", indigo_alpaca_error_NotImplemented, indigo_alpaca_error_string(indigo_alpaca_error_NotImplemented));
}

long indigo_alpaca_ccd_set_command(indigo_alpaca_device *alpaca_device, int version, char *command, char *buffer, long buffer_length, char *param_1, char *param_2) {
	if (!strcmp(command, "binx")) {
		int value = 0;
		indigo_alpaca_error result;
		if (sscanf(param_1, "BinX=%d", &value) == 1) {
			result = alpaca_set_binx(alpaca_device, version, value);
		} else {
			result = indigo_alpaca_error_InvalidValue;
		}
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	if (!strcmp(command, "biny")) {
		int value = 0;
		indigo_alpaca_error result;
		if (sscanf(param_1, "BinY=%d", &value) == 1) {
			result = alpaca_set_biny(alpaca_device, version, value);
		} else {
			result = indigo_alpaca_error_InvalidValue;
		}
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	if (!strcmp(command, "setccdtemperature")) {
		double value = 0;
		indigo_alpaca_error result;
		if (sscanf(param_1, "SetCCDTemperature=%lf", &value) == 1) {
			result = alpaca_set_setccdtemperature(alpaca_device, version, value);
		} else {
			result = indigo_alpaca_error_InvalidValue;
		}
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	if (!strcmp(command, "cooleron")) {
		bool value = !strcasecmp(param_1, "CoolerOn=true");
		indigo_alpaca_error result = alpaca_set_cooleron(alpaca_device, version, value);
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	if (!strcmp(command, "startx")) {
		double value = 0;
		indigo_alpaca_error result;
		if (sscanf(param_1, "StartX=%lf", &value) == 1) {
			result = alpaca_set_startx(alpaca_device, version, (int)value);
		} else {
			result = indigo_alpaca_error_InvalidValue;
		}
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	if (!strcmp(command, "starty")) {
		double value = 0;
		indigo_alpaca_error result;
		if (sscanf(param_1, "StartY=%lf", &value) == 1) {
			result = alpaca_set_starty(alpaca_device, version, (int)value);
		} else {
			result = indigo_alpaca_error_InvalidValue;
		}
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	if (!strcmp(command, "numx")) {
		double value = 0;
		indigo_alpaca_error result;
		if (sscanf(param_1, "NumX=%lf", &value) == 1) {
			result = alpaca_set_numx(alpaca_device, version, (int)value);
		} else {
			result = indigo_alpaca_error_InvalidValue;
		}
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	if (!strcmp(command, "numy")) {
		double value = 0;
		indigo_alpaca_error result;
		if (sscanf(param_1, "NumY=%lf", &value) == 1) {
			result = alpaca_set_numy(alpaca_device, version, (int)value);
		} else {
			result = indigo_alpaca_error_InvalidValue;
		}
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	if (!strcmp(command, "gain")) {
		int value = 0;
		indigo_alpaca_error result;
		if (sscanf(param_1, "Gain=%d", &value) == 1) {
			result = alpaca_set_gain(alpaca_device, version, value);
		} else {
			result = indigo_alpaca_error_InvalidValue;
		}
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	if (!strcmp(command, "offset")) {
		int value = 0;
		indigo_alpaca_error result;
		if (sscanf(param_1, "Offset=%d", &value) == 1) {
			result = alpaca_set_offset(alpaca_device, version, value);
		} else {
			result = indigo_alpaca_error_InvalidValue;
		}
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	if (!strcmp(command, "readoutmode")) {
		int value = 0;
		indigo_alpaca_error result;
		if (sscanf(param_1, "ReadoutMode=%d", &value) == 1) {
			result = alpaca_set_readoutmode(alpaca_device, version, value);
		} else {
			result = indigo_alpaca_error_InvalidValue;
		}
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	if (!strcmp(command, "startexposure")) {
		double duration = 0;
		indigo_alpaca_error result;
		char light_str[INDIGO_VALUE_SIZE] = {0};
		if (
			(sscanf(param_1, "Duration=%lf", &duration) == 1) &&
			(sscanf(param_2, "Light=%s", light_str) == 1)
		) {
			bool light = !strcmp(light_str, "True") || !strcmp(light_str, "true");
			result = alpaca_startexposure(alpaca_device, version, duration, light);
		} else {
			result = indigo_alpaca_error_InvalidValue;
		}
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	if (!strcmp(command, "abortexposure")) {
		indigo_alpaca_error result = alpaca_abortexposure(alpaca_device, version);
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	return snprintf(buffer, buffer_length, "\"ErrorNumber\": %d, \"ErrorMessage\": \"%s\"", indigo_alpaca_error_NotImplemented, indigo_alpaca_error_string(indigo_alpaca_error_NotImplemented));
}

#define PRINTF(fmt, ...) if (use_gzip) gzprintf(gzf, fmt, ##__VA_ARGS__); else indigo_uni_printf(handle, fmt, ##__VA_ARGS__);

void indigo_alpaca_ccd_get_imagearray(indigo_alpaca_device *alpaca_device, int version, indigo_uni_handle *handle, int client_transaction_id, int server_transaction_id, bool use_gzip, bool use_imagebytes) {
	indigo_alpaca_error result = indigo_alpaca_error_OK;
	indigo_blob_entry *entry;
	if (use_imagebytes) {
		indigo_alpaca_metadata metadata = { 0 };
		metadata.metadata_version = 1;
		metadata.client_transaction_id = client_transaction_id;
		metadata.server_transaction_id = server_transaction_id;
		metadata.data_start = sizeof(indigo_alpaca_metadata);
		metadata.image_element_type = metadata.transmission_element_type = indigo_alpaca_type_int32;
		if (alpaca_device->ccd.imageready && (entry = indigo_validate_blob(alpaca_device->ccd.imageready))) {
			pthread_mutex_lock(&entry->mutext);
			indigo_raw_header *header = (indigo_raw_header *)(entry->content);
			int width = header->width;
			int height = header->height;
			int size = width * height;
			metadata.dimension1 = width;
			metadata.dimension2 = height;
			switch (header->signature) {
				case INDIGO_RAW_MONO8: {
					metadata.dimension3 = 0;
					metadata.rank = 2;
					uint8_t *data = (uint8_t *)((char *)entry->content + sizeof(indigo_raw_header));
					uint32_t *buffer = (uint32_t *)indigo_safe_malloc(size * 4), *pnt = buffer;
					for (int col = 0; col < width; col++) {
						for (int row = height - 1;
					} row >= 0; row--)
							*pnt++ = data[row * width + col];
					indigo_uni_printf(handle, "HTTP/1.1 200 OK\r\nContent-Type: application/imagebytes\r\nContent-Length: %d\r\n\r\n", size * 4); // ASCOM BUG, should be + sizeof(metadata)
					handle->log_level = -abs(handle->log_level);
					indigo_uni_write(handle, (const char *)&metadata, sizeof(metadata));
					indigo_uni_write(handle, (const char *)buffer, size * 4);
					handle->log_level = abs(handle->log_level);
					break;
				}
				case INDIGO_RAW_MONO16: {
					metadata.dimension3 = 0;
					metadata.rank = 2;
					uint16_t *data = (uint16_t *)((char *)entry->content + sizeof(indigo_raw_header));
					uint32_t *buffer = (uint32_t *)indigo_safe_malloc(size * 4), *pnt = buffer;
					for (int col = 0; col < width; col++) {
						for (int row = height - 1;
					} row >= 0; row--)
							*pnt++ = data[row * width + col];
					indigo_uni_printf(handle, "HTTP/1.1 200 OK\r\nContent-Type: application/imagebytes\r\nContent-Length: %d\r\n\r\n", size * 4); // ASCOM BUG, should be + sizeof(metadata)
					handle->log_level = -abs(handle->log_level);
					indigo_uni_write(handle, (const char *)&metadata, sizeof(metadata));
					indigo_uni_write(handle, (const char *)buffer, size * 4);
					handle->log_level = abs(handle->log_level);
					indigo_safe_free(buffer);
					break;
				}
				case INDIGO_RAW_RGB24: {
					metadata.dimension3 = 3;
					metadata.rank = 3;
					uint8_t *data = (uint8_t *)((char *)entry->content + sizeof(indigo_raw_header));
					uint32_t *buffer = (uint32_t *)indigo_safe_malloc(size * 12), *pnt = buffer;
					for (int col = 0; col < width; col++) {
						for (int row = height - 1;
					} row >= 0; row--) {
							int base = 3 * (row * width + col);
							*pnt++ = data[base + 2];
							*pnt++ = data[base + 0];
							*pnt++ = data[base + 1];
						}
					indigo_uni_printf(handle, "HTTP/1.1 200 OK\r\nContent-Type: application/imagebytes\r\nContent-Length: %d\r\n\r\n", size * 12); // ASCOM BUG, should be + sizeof(metadata)
					handle->log_level = -abs(handle->log_level);
					indigo_uni_write(handle, (const char *)&metadata, sizeof(metadata));
					indigo_uni_write(handle, (const char *)buffer, size * 12);
					handle->log_level = abs(handle->log_level);
					break;
				}
				case INDIGO_RAW_RGB48: {
					metadata.dimension3 = 3;
					metadata.rank = 3;
					uint16_t *data = (uint16_t *)((char *)entry->content + sizeof(indigo_raw_header));
					uint32_t *buffer = (uint32_t *)indigo_safe_malloc(size * 12), *pnt = buffer;
					for (int col = 0; col < width; col++) {
						for (int row = height - 1;
					} row >= 0; row--) {
							int base = 3 * (row * width + col);
							*pnt++ = data[base + 0];
							*pnt++ = data[base + 1];
							*pnt++ = data[base + 2];
						}
					indigo_uni_printf(handle, "HTTP/1.1 200 OK\r\nContent-Type: application/imagebytes\r\nContent-Length: %d\r\n\r\n", size * 12); // ASCOM BUG, should be + sizeof(metadata)
					handle->log_level = -abs(handle->log_level);
					indigo_uni_write(handle, (const char *)&metadata, sizeof(metadata));
					indigo_uni_write(handle, (const char *)buffer, size * 12);
					handle->log_level = abs(handle->log_level);
					break;
				}
			}
			pthread_mutex_unlock(&entry->mutext);
		} else {
			metadata.error_number = result;
			indigo_uni_printf(handle, "%s", indigo_alpaca_error_string(result));
		}
	} else {
		gzFile gzf = NULL;
		if (use_gzip) {
			indigo_uni_printf(handle, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Encoding: gzip\r\n\r\n");
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
			gzf = gzdopen(handle->fd, "w");
#elif defined(INDIGO_WINDOWS)
			gzf = gzdopen(_open_osfhandle((intptr_t)handle->sock, _O_WRONLY), "w");
#endif
		} else {
			indigo_uni_printf(handle, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n");
		}
		if (alpaca_device->ccd.imageready && (entry = indigo_validate_blob(alpaca_device->ccd.imageready))) {
			pthread_mutex_lock(&entry->mutext);
			indigo_raw_header *header = (indigo_raw_header *)(entry->content);
			int width = header->width;
			int height = header->height;
			switch (header->signature) {
				case INDIGO_RAW_MONO8: {
					PRINTF("{ \"Type\": 2, \"Rank\": 2, \"Value\": [");
					uint8_t *data = (uint8_t *)((char *)entry->content + sizeof(indigo_raw_header));
					for (int col = 0; col < width; col++) {
						if (col == 0) {
							PRINTF("[");
						} else {
							PRINTF(", [");
						}
						for (int row = height - 1; row >= 0; row--) {
							if (row == 0) {
								PRINTF("%d", data[row * width + col]);
							} else {
								PRINTF(", %d", data[row * width + col]);
							}
						}
						PRINTF("]");
					}
					break;
				}
				case INDIGO_RAW_MONO16: {
					PRINTF("{ \"Type\": 2, \"Rank\": 2, \"Value\": [");
					uint16_t *data = (uint16_t *)((char *)entry->content + sizeof(indigo_raw_header));
					for (int col = 0; col < width; col++) {
						if (col == 0) {
							PRINTF("[");
						} else {
							PRINTF(", [");
						}
						for (int row = height - 1; row >= 0; row--) {
							if (row == 0) {
								PRINTF("%d", data[row * width + col]);
							} else {
								PRINTF(", %d", data[row * width + col]);
							}
						}
						PRINTF("]");
					}
					break;
				}
				case INDIGO_RAW_RGB24: {
					PRINTF("{ \"Type\": 2, \"Rank\": 3, \"Value\": [");
					uint8_t *data = (uint8_t *)((char *)entry->content + sizeof(indigo_raw_header));
					for (int col = 0; col < width; col++) {
						if (col == 0) {
							PRINTF("[");
						} else {
							PRINTF(", [");
						}
						for (int row = height - 1; row >= 0; row--) {
							int base = 3 * (row * width + col);
							int r = data[base + 0];
							int g = data[base + 1];
							int b = data[base + 2];
							if (row == 0) {
								PRINTF("[%d,%d,%d]", r, g, b);
							} else {
								PRINTF(",[%d,%d,%d]", r, g, b);
							}
						}
						PRINTF("]");
					}
					break;
				}
				default:
					PRINTF("{ \"Type\": 2, \"Rank\": 2, \"Value\": [");
					break;
			}
			pthread_mutex_unlock(&entry->mutext);
		} else {
			PRINTF("{ \"Type\": 2, \"Rank\": 2, \"Value\": [");
			result = indigo_alpaca_error_InvalidOperation;
		}
		PRINTF("], \"ErrorNumber\": %d, \"ErrorMessage\": \"%s\", \"ClientTransactionID\": %u, \"ServerTransactionID\": %u }", result, indigo_alpaca_error_string(result), client_transaction_id, server_transaction_id);
		if (use_gzip) {
			gzclose(gzf);
		}
	}
}
