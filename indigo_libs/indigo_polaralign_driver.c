// Copyright (c) 2025 by Rumen G.Bogdanovski 
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
// 2.0 by Rumen G.Bogdanovski <rumenastro@gmail.com>

/** INDIGO Polar Aligner Driver base
 \file indigo_polaralign_driver.c
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

#include <indigo/indigo_polaralign_driver.h>
#include <indigo/indigo_io.h>

indigo_result indigo_polaralign_attach(indigo_device *device, const char *driver_name, unsigned version) {
	assert(device != NULL);
	if (POLARALIGN_CONTEXT == NULL) {
		device->device_context = indigo_safe_malloc(sizeof(indigo_polaralign_context));
	}
	if (POLARALIGN_CONTEXT != NULL) {
		if (indigo_device_attach(device, driver_name, version, INDIGO_INTERFACE_POLARALIGN) == INDIGO_OK) {
			// -------------------------------------------------------------------------------- POLARALIGN_OFFSET
			POLARALIGN_OFFSET_PROPERTY = indigo_init_number_property(NULL, device->name, POLARALIGN_OFFSET_PROPERTY_NAME, POLARALIGN_MAIN_GROUP, "Polar alignment offsets", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
			if (POLARALIGN_OFFSET_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_number_item(POLARALIGN_OFFSET_ALTITUDE_ITEM, POLARALIGN_OFFSET_ALTITUDE_ITEM_NAME, "Altitude offset [arcmin]", -999, 999, 0.1, 0);
			indigo_init_number_item(POLARALIGN_OFFSET_AZIMUTH_ITEM, POLARALIGN_OFFSET_AZIMUTH_ITEM_NAME, "Azimuth offset [arcmin]", -999, 999, 0.1, 0);
			// -------------------------------------------------------------------------------- POLARALIGN_ABORT_MOTION
			POLARALIGN_ABORT_MOTION_PROPERTY = indigo_init_switch_property(NULL, device->name, POLARALIGN_ABORT_MOTION_PROPERTY_NAME, POLARALIGN_MAIN_GROUP, "Abort motion", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
			if (POLARALIGN_ABORT_MOTION_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_switch_item(POLARALIGN_ABORT_MOTION_ITEM, POLARALIGN_ABORT_MOTION_ITEM_NAME, "Abort motion", false);
			// --------------------------------------------------------------------------------
			return INDIGO_OK;
		}
	}
	return INDIGO_FAILED;
}

indigo_result indigo_polaralign_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	if (IS_CONNECTED) {
		indigo_define_matching_property(POLARALIGN_OFFSET_PROPERTY);
		indigo_define_matching_property(POLARALIGN_ABORT_MOTION_PROPERTY);
	}
	return indigo_device_enumerate_properties(device, client, property);
}

indigo_result indigo_polaralign_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (IS_CONNECTED) {
			indigo_define_property(device, POLARALIGN_OFFSET_PROPERTY, NULL);
			indigo_define_property(device, POLARALIGN_ABORT_MOTION_PROPERTY, NULL);
		} else {
			POLARALIGN_OFFSET_PROPERTY->state = INDIGO_OK_STATE;
			indigo_delete_property(device, POLARALIGN_OFFSET_PROPERTY, NULL);
			indigo_delete_property(device, POLARALIGN_ABORT_MOTION_PROPERTY, NULL);
		}
	// -------------------------------------------------------------------------------- POLARALIGN_OFFSET
	} else if (indigo_property_match_changeable(POLARALIGN_OFFSET_PROPERTY, property)) {
		indigo_property_copy_values(POLARALIGN_OFFSET_PROPERTY, property, false);
		POLARALIGN_OFFSET_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, POLARALIGN_OFFSET_PROPERTY, NULL);
		return INDIGO_OK;
	// -------------------------------------------------------------------------------- POLARALIGN_ABORT_MOTION
	} else if (indigo_property_match_changeable(POLARALIGN_ABORT_MOTION_PROPERTY, property)) {
		indigo_property_copy_values(POLARALIGN_ABORT_MOTION_PROPERTY, property, false);
		POLARALIGN_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		POLARALIGN_ABORT_MOTION_ITEM->sw.value = false;
		indigo_update_property(device, POLARALIGN_ABORT_MOTION_PROPERTY, NULL);
		return INDIGO_OK;
	}
	return indigo_device_change_property(device, client, property);
}

indigo_result indigo_polaralign_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_release_property(POLARALIGN_OFFSET_PROPERTY);
	indigo_release_property(POLARALIGN_ABORT_MOTION_PROPERTY);
	return indigo_device_detach(device);
}
