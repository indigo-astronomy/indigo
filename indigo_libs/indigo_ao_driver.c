// Copyright (c) 2016 CloudMakers, s. r. o.
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

/** INDIGO AO Driver base
 \file indigo_ao_driver.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "indigo_ao_driver.h"

indigo_result indigo_ao_attach(indigo_device *device, unsigned version) {
	assert(device != NULL);
	assert(device != NULL);
	if (AO_CONTEXT == NULL) {
		device->device_context = malloc(sizeof(indigo_ao_context));
		assert(device->device_context);
		memset(device->device_context, 0, sizeof(indigo_ao_context));
	}
	if (AO_CONTEXT != NULL) {
		if (indigo_device_attach(device, version, INDIGO_INTERFACE_AO) == INDIGO_OK) {
			// -------------------------------------------------------------------------------- AO_GUIDE_DEC
			AO_GUIDE_DEC_PROPERTY = indigo_init_number_property(NULL, device->name, AO_GUIDE_DEC_PROPERTY_NAME, AO_MAIN_GROUP, "DEC guiding", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
			if (AO_GUIDE_DEC_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_number_item(AO_GUIDE_NORTH_ITEM, AO_GUIDE_NORTH_ITEM_NAME, "Guide north", 0, 100, 0, 0);
			indigo_init_number_item(AO_GUIDE_SOUTH_ITEM, AO_GUIDE_SOUTH_ITEM_NAME, "Guide south", 0, 100, 0, 0);
			// -------------------------------------------------------------------------------- AO_GUIDE_RA
			AO_GUIDE_RA_PROPERTY = indigo_init_number_property(NULL, device->name, AO_GUIDE_RA_PROPERTY_NAME, AO_MAIN_GROUP, "RA guiding", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
			if (AO_GUIDE_RA_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_number_item(AO_GUIDE_EAST_ITEM, AO_GUIDE_EAST_ITEM_NAME, "Guide east", 0, 100, 0, 0);
			indigo_init_number_item(AO_GUIDE_WEST_ITEM, AO_GUIDE_WEST_ITEM_NAME, "Guide west", 0, 100, 0, 0);
			// -------------------------------------------------------------------------------- AO_RESET
			AO_RESET_PROPERTY = indigo_init_switch_property(NULL, device->name, AO_RESET_PROPERTY_NAME, AO_MAIN_GROUP, "Reset", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 2);
			if (AO_RESET_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_switch_item(AO_CENTER_ITEM, AO_CENTER_ITEM_NAME, "Center", false);
			indigo_init_switch_item(AO_UNJAM_ITEM, AO_UNJAM_ITEM_NAME, "Unjam", false);
			// --------------------------------------------------------------------------------
			return INDIGO_OK;
		}
	}
	return INDIGO_FAILED;
}

indigo_result indigo_ao_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	if (IS_CONNECTED) {
		if (indigo_property_match(AO_GUIDE_DEC_PROPERTY, property))
			indigo_define_property(device, AO_GUIDE_DEC_PROPERTY, NULL);
		if (indigo_property_match(AO_GUIDE_RA_PROPERTY, property))
			indigo_define_property(device, AO_GUIDE_RA_PROPERTY, NULL);
		if (indigo_property_match(AO_RESET_PROPERTY, property))
			indigo_define_property(device, AO_RESET_PROPERTY, NULL);
	}
	return indigo_device_enumerate_properties(device, client, property);
}

indigo_result indigo_ao_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (IS_CONNECTED) {
			indigo_define_property(device, AO_GUIDE_DEC_PROPERTY, NULL);
			indigo_define_property(device, AO_GUIDE_RA_PROPERTY, NULL);
			indigo_define_property(device, AO_RESET_PROPERTY, NULL);
		} else {
			indigo_delete_property(device, AO_GUIDE_DEC_PROPERTY, NULL);
			indigo_delete_property(device, AO_GUIDE_RA_PROPERTY, NULL);
			indigo_delete_property(device, AO_RESET_PROPERTY, NULL);
		}
		// --------------------------------------------------------------------------------
	}
	return indigo_device_change_property(device, client, property);
}

indigo_result indigo_ao_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_release_property(AO_GUIDE_DEC_PROPERTY);
	indigo_release_property(AO_GUIDE_RA_PROPERTY);
	indigo_release_property(AO_RESET_PROPERTY);
	return indigo_device_detach(device);
}

