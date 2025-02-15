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

/** INDIGO Guider Driver base
 \file indigo_guider_driver.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <sys/stat.h>

#include <indigo/indigo_guider_driver.h>

indigo_result indigo_guider_attach(indigo_device *device, const char* driver_name, unsigned version) {
	assert(device != NULL);
	assert(device != NULL);
	if (GUIDER_CONTEXT == NULL) {
		device->device_context = indigo_safe_malloc(sizeof(indigo_guider_context));
	}
	if (GUIDER_CONTEXT != NULL) {
		if (indigo_device_attach(device, driver_name, version, INDIGO_INTERFACE_GUIDER) == INDIGO_OK) {
			// -------------------------------------------------------------------------------- GUIDER_GUIDE_DEC
			GUIDER_GUIDE_DEC_PROPERTY = indigo_init_number_property(NULL, device->name, GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_MAIN_GROUP, "DEC guiding", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
			if (GUIDER_GUIDE_DEC_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_number_item(GUIDER_GUIDE_NORTH_ITEM, GUIDER_GUIDE_NORTH_ITEM_NAME, "Guide north", 0, 10000, 0, 0);
			indigo_init_number_item(GUIDER_GUIDE_SOUTH_ITEM, GUIDER_GUIDE_SOUTH_ITEM_NAME, "Guide south", 0, 10000, 0, 0);
			// -------------------------------------------------------------------------------- GUIDER_GUIDE_RA
			GUIDER_GUIDE_RA_PROPERTY = indigo_init_number_property(NULL, device->name, GUIDER_GUIDE_RA_PROPERTY_NAME, GUIDER_MAIN_GROUP, "RA guiding", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
			if (GUIDER_GUIDE_RA_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_number_item(GUIDER_GUIDE_EAST_ITEM, GUIDER_GUIDE_EAST_ITEM_NAME, "Guide east", 0, 10000, 0, 0);
			indigo_init_number_item(GUIDER_GUIDE_WEST_ITEM, GUIDER_GUIDE_WEST_ITEM_NAME, "Guide west", 0, 10000, 0, 0);
			// -------------------------------------------------------------------------------- GUIDER_RATE
			GUIDER_RATE_PROPERTY = indigo_init_number_property(NULL, device->name, GUIDER_RATE_PROPERTY_NAME, GUIDER_MAIN_GROUP, "Guiding rate", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
			if (GUIDER_RATE_PROPERTY == NULL)
				return INDIGO_FAILED;
			GUIDER_RATE_PROPERTY->hidden = true;
			GUIDER_RATE_PROPERTY->count = 1;
			indigo_init_number_item(GUIDER_RATE_ITEM, GUIDER_RATE_ITEM_NAME, "Guiding rate (% of sidereal)", 10, 90, 0, 50);
			indigo_init_number_item(GUIDER_DEC_RATE_ITEM, GUIDER_DEC_RATE_ITEM_NAME, "DEC Guiding rate (% of sidereal)", 10, 90, 0, 50);
			// --------------------------------------------------------------------------------
			return INDIGO_OK;
		}
	}
	return INDIGO_FAILED;
}

indigo_result indigo_guider_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	if (IS_CONNECTED) {
		indigo_define_matching_property(GUIDER_GUIDE_DEC_PROPERTY);
		indigo_define_matching_property(GUIDER_GUIDE_RA_PROPERTY);
		indigo_define_matching_property(GUIDER_RATE_PROPERTY);
	}
	return indigo_device_enumerate_properties(device, client, property);
}

indigo_result indigo_guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (IS_CONNECTED) {
			indigo_define_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
			indigo_define_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
			indigo_define_property(device, GUIDER_RATE_PROPERTY, NULL);
		} else {
			GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
			GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
			indigo_delete_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
			indigo_delete_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
			indigo_delete_property(device, GUIDER_RATE_PROPERTY, NULL);
		}
		// --------------------------------------------------------------------------------
	}
	return indigo_device_change_property(device, client, property);
}

indigo_result indigo_guider_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_release_property(GUIDER_GUIDE_DEC_PROPERTY);
	indigo_release_property(GUIDER_GUIDE_RA_PROPERTY);
	indigo_release_property(GUIDER_RATE_PROPERTY);
	return indigo_device_detach(device);
}
