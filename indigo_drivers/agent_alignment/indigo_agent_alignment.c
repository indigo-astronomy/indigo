// Copyright (c) 2018 CloudMakers, s. r. o.
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

/** INDIGO Alignment model calibration agent
 \file indigo_agent_alignment.c
 */

#define DRIVER_VERSION 0x0001
#define DRIVER_NAME	"indigo_agent_alignment"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

#include "indigo_driver_xml.h"
#include "indigo_filter.h"
#include "indigo_mount_driver.h"
#include "indigo_agent_alignment.h"

#define DEVICE_PRIVATE_DATA												((agent_private_data *)device->private_data)
#define CLIENT_PRIVATE_DATA												((agent_private_data *)FILTER_CLIENT_CONTEXT->device->private_data)

#define AGENT_ALIGNMENT_POINT_RA_ITEM(property)   		(property->items+0)
#define AGENT_ALIGNMENT_POINT_DEC_ITEM(property)   		(property->items+1)
#define AGENT_ALIGNMENT_POINT_RAW_RA_ITEM(property)  	(property->items+2)
#define AGENT_ALIGNMENT_POINT_RAW_DEC_ITEM(property)  (property->items+3)
#define AGENT_ALIGNMENT_POINT_LST_ITEM(property)  		(property->items+4)
#define AGENT_ALIGNMENT_POINT_SOP_ITEM(property)  		(property->items+5)

typedef struct {
	int alignment_point_count;
	indigo_property **alignment_point_properties;
	indigo_device *mount;
} agent_private_data;

// -------------------------------------------------------------------------------- INDIGO agent device implementation

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result agent_device_attach(indigo_device *device) {
	assert(device != NULL);
	assert(DEVICE_PRIVATE_DATA != NULL);
	if (indigo_filter_device_attach(device, DRIVER_VERSION, INDIGO_INTERFACE_CCD) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- Device properties
		FILTER_MOUNT_LIST_PROPERTY->hidden = false;
		// --------------------------------------------------------------------------------
		CONNECTION_PROPERTY->hidden = true;
		CONFIG_PROPERTY->hidden = true;
		PROFILE_PROPERTY->hidden = true;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return agent_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (client != NULL && client == FILTER_DEVICE_CONTEXT->client)
		return INDIGO_OK;
	int alignment_point_count = DEVICE_PRIVATE_DATA->alignment_point_count;
	indigo_property **alignment_properties = DEVICE_PRIVATE_DATA->alignment_point_properties;
	if (alignment_properties) {
		for (int i = 0; i < alignment_point_count; i++)
			indigo_define_property(device, alignment_properties[i], NULL);
	}
	return indigo_filter_enumerate_properties(device, client, property);
}

static indigo_result agent_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (client == FILTER_DEVICE_CONTEXT->client)
		return INDIGO_OK;
	
	
	int alignment_point_count = DEVICE_PRIVATE_DATA->alignment_point_count;
	indigo_property **alignment_properties = DEVICE_PRIVATE_DATA->alignment_point_properties;
	indigo_device *mount = DEVICE_PRIVATE_DATA->mount;
	if (mount && alignment_properties) {
		indigo_alignment_point *alignment_points = ((indigo_mount_context *)(mount->device_context))->alignment_points;
		for (int i = 0; i < alignment_point_count; i++) {
			indigo_property *alignment_property = alignment_properties[i];
			if (indigo_property_match(alignment_property, property)) {
				indigo_property_copy_values(alignment_property, property, false);
				alignment_points->ra = AGENT_ALIGNMENT_POINT_RA_ITEM(alignment_property)->number.value;
				alignment_points->dec = AGENT_ALIGNMENT_POINT_DEC_ITEM(alignment_property)->number.value;
				alignment_points->raw_ra = AGENT_ALIGNMENT_POINT_RAW_RA_ITEM(alignment_property)->number.value;
				alignment_points->raw_dec = AGENT_ALIGNMENT_POINT_RAW_DEC_ITEM(alignment_property)->number.value;
				alignment_points->lst = AGENT_ALIGNMENT_POINT_LST_ITEM(alignment_property)->number.value;
				alignment_points->side_of_pier = AGENT_ALIGNMENT_POINT_SOP_ITEM(alignment_property)->number.value;
				indigo_mount_update_alignment_points(DEVICE_PRIVATE_DATA->mount);
				indigo_update_property(device, alignment_property, NULL);
			}
		}
	}

	
	return indigo_filter_change_property(device, client, property);
}

static indigo_result agent_device_detach(indigo_device *device) {
	assert(device != NULL);

	// TBD

	return indigo_filter_device_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO agent client implementation

static indigo_result agent_define_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (*FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_MOUNT_INDEX] && !strcmp(property->device, FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_MOUNT_INDEX])) {
		bool delete = false;
		bool define = false;
		indigo_device *agent_device = FILTER_CLIENT_CONTEXT->device;
		if (!strcmp(property->name, CONNECTION_PROPERTY_NAME)) {
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				if (item->sw.value && !strcmp(item->name, CONNECTION_CONNECTED_ITEM_NAME) && property->state == INDIGO_OK_STATE) {
					define = true;
				} else if (item->sw.value && !strcmp(item->name, CONNECTION_DISCONNECTED_ITEM_NAME)) {
					delete = true;
				}
			}
		} else if (!strcmp(property->name, MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY_NAME)) {
			delete = true;
			define = true;
		}
		if (delete) {
			int alignment_point_count = CLIENT_PRIVATE_DATA->alignment_point_count;
			indigo_property **alignment_properties = CLIENT_PRIVATE_DATA->alignment_point_properties;
			if (alignment_properties) {
				for (int j = 0; j < alignment_point_count; j++) {
					indigo_property *alignment_property = alignment_properties[j];
					indigo_delete_property(agent_device, alignment_property, NULL);
					indigo_release_property(alignment_property);
				}
				free(alignment_properties);
				CLIENT_PRIVATE_DATA->alignment_point_count = 0;
				CLIENT_PRIVATE_DATA->alignment_point_properties = NULL;
				CLIENT_PRIVATE_DATA->mount = NULL;
			}
		}
		if (define) {
			int alignment_point_count = MOUNT_CONTEXT->alignment_point_count;
			indigo_property **alignment_properties = malloc(alignment_point_count * sizeof(indigo_property *));
			if (alignment_properties) {
				CLIENT_PRIVATE_DATA->mount = device;
				indigo_alignment_point *points = MOUNT_CONTEXT->alignment_points;
				for (int j = 0; j < alignment_point_count; j++) {
					char name[INDIGO_NAME_SIZE], label[INDIGO_NAME_SIZE];
					sprintf(name, AGENT_ALIGNMENT_POINT_PROPERY_NAME, j);
					sprintf(label, "Alignment point #%d", j);
					indigo_property *alignment_property = indigo_init_number_property(NULL, agent_device->name, name, "Alignment points", label, INDIGO_OK_STATE, INDIGO_RW_PERM, 6);
					indigo_init_number_item(AGENT_ALIGNMENT_POINT_RA_ITEM(alignment_property), AGENT_ALIGNMENT_POINT_RA_ITEM_NAME, "Right ascension (0 to 24 hrs)", 0, 24, 0, points[j].ra);
					indigo_init_number_item(AGENT_ALIGNMENT_POINT_DEC_ITEM(alignment_property), AGENT_ALIGNMENT_POINT_DEC_ITEM_NAME, "Declination (-90 to 90°))", -90, 90, 0, points[j].dec);
					indigo_init_number_item(AGENT_ALIGNMENT_POINT_RAW_RA_ITEM(alignment_property), AGENT_ALIGNMENT_POINT_RAW_RA_ITEM_NAME, "Raw right ascension (0 to 24 hrs)", 0, 24, 0, points[j].raw_ra);
					indigo_init_number_item(AGENT_ALIGNMENT_POINT_RAW_DEC_ITEM(alignment_property), AGENT_ALIGNMENT_POINT_RAW_DEC_ITEM_NAME, "Raw declination (-90 to 90°))", -90, 90, 0, points[j].raw_dec);
					indigo_init_number_item(AGENT_ALIGNMENT_POINT_LST_ITEM(alignment_property), AGENT_ALIGNMENT_POINT_LST_ITEM_NAME, "LST Time", 0, 24, 0, points[j].lst);
					indigo_init_number_item(AGENT_ALIGNMENT_POINT_SOP_ITEM(alignment_property), AGENT_ALIGNMENT_POINT_SOP_ITEM_NAME, "Side of pier", 0, 1, 0, points[j].side_of_pier);
					alignment_properties[j] = alignment_property;
					indigo_define_property(agent_device, alignment_property, NULL);
				}
				CLIENT_PRIVATE_DATA->alignment_point_count = alignment_point_count;
				CLIENT_PRIVATE_DATA->alignment_point_properties = alignment_properties;
			}
		}
	}
	return indigo_filter_define_property(client, device, property, message);
}

static indigo_result agent_update_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {

	// TBD
	
	return indigo_filter_update_property(client, device, property, message);
}

static indigo_result agent_delete_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {

	// TBD

	return indigo_filter_delete_property(client, device, property, message);
}
// -------------------------------------------------------------------------------- Initialization

static agent_private_data *private_data = NULL;

static indigo_device *agent_device = NULL;
static indigo_client *agent_client = NULL;

indigo_result indigo_agent_alignment(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device agent_device_template = INDIGO_DEVICE_INITIALIZER(
		IMAGER_AGENT_NAME,
		agent_device_attach,
		agent_enumerate_properties,
		agent_change_property,
		NULL,
		agent_device_detach
	);

	static indigo_client agent_client_template = {
		IMAGER_AGENT_NAME, false, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT, NULL,
		indigo_filter_client_attach,
		agent_define_property,
		agent_update_property,
		agent_delete_property,
		NULL,
		indigo_filter_client_detach
	};

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, IMAGER_AGENT_NAME, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch(action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = malloc(sizeof(agent_private_data));
			assert(private_data != NULL);
			memset(private_data, 0, sizeof(agent_private_data));
			agent_device = malloc(sizeof(indigo_device));
			assert(agent_device != NULL);
			memcpy(agent_device, &agent_device_template, sizeof(indigo_device));
			agent_device->private_data = private_data;
			indigo_attach_device(agent_device);

			agent_client = malloc(sizeof(indigo_client));
			assert(agent_client != NULL);
			memcpy(agent_client, &agent_client_template, sizeof(indigo_client));
			agent_client->client_context = agent_device->device_context;
			indigo_attach_client(agent_client);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			last_action = action;
			if (agent_client != NULL) {
				indigo_detach_client(agent_client);
				free(agent_client);
				agent_client = NULL;
			}
			if (agent_device != NULL) {
				indigo_detach_device(agent_device);
				free(agent_device);
				agent_device = NULL;
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
