// Copyright (c) 2018 CloudMakers, s. r. o.
// All rights reserved.
//
//  This code is partially based on PHD driver by Craig Stark
//  https://github.com/OpenPHDGuiding/phd2/blob/master/scope_eqmac.cpp
//  Copyright (c) 2008-2010 Craig Stark.
//  All rights reserved.
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


/** INDIGO EQMac guider driver
 \file indigo_guider_eqmac.c
 */

#define DRIVER_VERSION 0x0001
#define DRIVER_NAME "indigo_guider_eqmac"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>

#include <CoreServices/CoreServices.h>

#include <stdbool.h>
#include "indigo_guider_eqmac.h"
#include "indigo_driver_xml.h"

AppleEvent E6Return;
AppleEvent E6Event;
SInt16 E6ReturnCode;

OSErr eqmacGuide(double ra, double dec) {
	OSErr err;
	char *eqmc = "au.id.hulse.EQMac";
	FourCharCode indigoSig = 'IDGO';
	AEAddressDesc addDesc;
	AEEventClass evClass = 'phdG';
	AEEventID evID = 'evGD';
	AEKeyword  keyObject;
	AESendMode mode = kAEWaitReply;
	err = AECreateDesc(typeApplicationBundleID, (Ptr) eqmc, strlen(eqmc), &addDesc);
	if (err != noErr) return err;
	err = AECreateAppleEvent(evClass, evID, &addDesc, kAutoGenerateReturnID, kAnyTransactionID, &E6Event);
	if (err != noErr) {
		AEDisposeDesc(&addDesc);
		return err;
	}
	err = AECreateDesc(typeApplSignature, (Ptr) &indigoSig, sizeof(FourCharCode), &addDesc);
	if (err != noErr) {
		AEDisposeDesc(&E6Event);
		return err;
	}
	err = AECreateAppleEvent(evClass, evID, &addDesc, kAutoGenerateReturnID, kAnyTransactionID, &E6Return);
	if (err != noErr) {
		AEDisposeDesc(&E6Event);
		AEDisposeDesc(&addDesc);
		return err;
	}
	keyObject = 'prEW';
	err = AEPutParamPtr(&E6Event, keyObject, typeIEEE64BitFloatingPoint,  &ra, sizeof(double));
	if (err != noErr) {
		AEDisposeDesc(&E6Event);
		AEDisposeDesc(&addDesc);
		return err;
	}
	keyObject = 'prNS';
	err = AEPutParamPtr(&E6Event, keyObject, typeIEEE64BitFloatingPoint, &dec, sizeof(double));
	if (err != noErr) {
		AEDisposeDesc(&E6Event);
		AEDisposeDesc(&addDesc);
		return err;
	}
	err = AESendMessage(&E6Event, &E6Return, mode, kAEDefaultTimeout);
	if (err != noErr) {
		AEDisposeDesc(&E6Event);
		AEDisposeDesc(&addDesc);
		return err;
	}
	keyObject = 'prRC';
	Size returnSize;
	DescType returnType;
	err = AEGetParamPtr(&E6Return, keyObject, typeSInt16, &returnType, &E6ReturnCode, sizeof(SInt16), &returnSize);
	
	AEDisposeDesc(&E6Event);
	AEDisposeDesc(&addDesc);
	return 0;
}

// -------------------------------------------------------------------------------- INDIGO guider device implementation

static indigo_result guider_attach(indigo_device *device) {
	assert(device != NULL);
	if (indigo_guider_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		return indigo_guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);

	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			OSErr err = eqmacGuide(0.0, 0.0);
			if (E6ReturnCode == -1) {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
				indigo_update_property(device, CONNECTION_PROPERTY, "Not connected to a mount");
				return INDIGO_OK;
			} else if (err == -600) {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
				indigo_update_property(device, CONNECTION_PROPERTY, "EQMac is not running");
				return INDIGO_OK;
			}
		}
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	} else if (indigo_property_match(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_DEC
		indigo_property_copy_values(GUIDER_GUIDE_DEC_PROPERTY, property, false);
		OSErr err = 0;
		double duration = GUIDER_GUIDE_NORTH_ITEM->number.value;
		if (duration > 0) {
			err = eqmacGuide(0.0, duration / 1000);
		} else {
			double duration = GUIDER_GUIDE_SOUTH_ITEM->number.value;
			if (duration > 0) {
				err = eqmacGuide(0.0, -duration / 1000);
			} else {
				return INDIGO_OK;
			}
		}
		if (E6ReturnCode == -1) {
			GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, "Not connected to a mount");
		} else if (err == -600) {
			GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, "EQMac is not running");
		} else {
			GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDER_GUIDE_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_RA
		indigo_property_copy_values(GUIDER_GUIDE_RA_PROPERTY, property, false);
		OSErr err = 0;
		double duration = GUIDER_GUIDE_EAST_ITEM->number.value;
		if (duration > 0) {
			err = eqmacGuide(duration / 1000, 0.0);
		} else {
			double duration = GUIDER_GUIDE_WEST_ITEM->number.value;
			if (duration > 0) {
				err = eqmacGuide(-duration / 1000, 0.0);
			} else {
				return INDIGO_OK;
			}
		}
		if (E6ReturnCode == -1) {
			GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, "Not connected to a mount");
		} else if (err == -600) {
			GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, "EQMac is not running");
		} else {
			GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
		}
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_guider_change_property(device, client, property);
}

static indigo_result guider_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);
	indigo_global_unlock(device);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_guider_detach(device);
}

static indigo_device guider_template = INDIGO_DEVICE_INITIALIZER(
	EQMAC_GUIDER_NAME,
	guider_attach,
	indigo_guider_enumerate_properties,
	guider_change_property,
	NULL,
	guider_detach
);

static indigo_device *guider = NULL;

indigo_result indigo_guider_eqmac(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "EQMac Guider", __FUNCTION__, DRIVER_VERSION, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;			
			guider = malloc(sizeof(indigo_device));
			assert(guider != NULL);
			memcpy(guider, &guider_template, sizeof(indigo_device));
			guider->private_data = NULL;
			indigo_attach_device(guider);
			break;
		case INDIGO_DRIVER_SHUTDOWN:
			last_action = action;
			if (guider != NULL) {
				indigo_detach_device(guider);
				free(guider);
				guider = NULL;
			}
			break;
		case INDIGO_DRIVER_INFO:
			break;
	}
	return INDIGO_OK;
}
