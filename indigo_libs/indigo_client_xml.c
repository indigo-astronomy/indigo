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

/** INDIGO XML wire protocol driver side adapter
 \file indigo_client_xml.c
 */

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>
#include <assert.h>

#include "indigo_xml.h"
#include "indigo_io.h"
#include "indigo_version.h"
#include "indigo_client_xml.h"

static pthread_mutex_t xml_mutex = PTHREAD_MUTEX_INITIALIZER;

static indigo_result xml_client_parser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	if (!indigo_reshare_remote_devices && client && client->is_remote)
		return INDIGO_OK;
	pthread_mutex_lock(&xml_mutex);
	indigo_adapter_context *device_context = (indigo_adapter_context *)device->device_context;
	assert(device_context != NULL);
	int handle = device_context->output;
	char device_name[INDIGO_NAME_SIZE];
	if (property != NULL && *property->device) {
		strncpy(device_name, property->device, INDIGO_NAME_SIZE);
		if (indigo_use_host_suffix) {
			char *at = strrchr(device_name, '@');
			if (at != NULL) {
				while (at > device_name && at[-1] == ' ')
					at--;
				*at = 0;
			}
		}
	}
	if (property != NULL) {
		if (*property->device && *indigo_property_name(device->version, property)) {
			indigo_printf(handle, "<getProperties version='1.7' switch='%d.%d' device='%s' name='%s'/>\n", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF, indigo_xml_escape(device_name), indigo_property_name(device->version, property));
		} else if (*property->device) {
			indigo_printf(handle, "<getProperties version='1.7' switch='%d.%d' device='%s'/>\n", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF, indigo_xml_escape(device_name));
		} else if (*indigo_property_name(device->version, property)) {
			indigo_printf(handle, "<getProperties version='1.7' switch='%d.%d' name='%s'/>\n", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF, indigo_property_name(device->version, property));
		} else {
			indigo_printf(handle, "<getProperties version='1.7' switch='%d.%d'/>\n", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF);
		}
	} else {
		indigo_printf(handle, "<getProperties version='1.7' switch='%d.%d'/>\n", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF);
	}
	pthread_mutex_unlock(&xml_mutex);
	return INDIGO_OK;
}

static indigo_result xml_client_parser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(property != NULL);
	if (!indigo_reshare_remote_devices && client && client->is_remote)
		return INDIGO_OK;
	pthread_mutex_lock(&xml_mutex);
	indigo_adapter_context *device_context = (indigo_adapter_context *)device->device_context;
	assert(device_context != NULL);
	int handle = device_context->output;
	char device_name[INDIGO_NAME_SIZE];
	strncpy(device_name, property->device, INDIGO_NAME_SIZE);
	if (indigo_use_host_suffix) {
		char *at = strrchr(device_name, '@');
		if (at != NULL) {
			while (at > device_name && at[-1] == ' ')
				at--;
			*at = 0;
		}
	}
	switch (property->type) {
	case INDIGO_TEXT_VECTOR:
		indigo_printf(handle, "<newTextVector device='%s' name='%s'>\n", indigo_xml_escape(device_name), indigo_property_name(device->version, property), indigo_property_state_text[property->state]);
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = &property->items[i];
			indigo_printf(handle, "<oneText name='%s'>%s</oneText>\n", indigo_item_name(device->version, property, item), indigo_xml_escape(item->text.value));
		}
		indigo_printf(handle, "</newTextVector>\n");
		break;
	case INDIGO_NUMBER_VECTOR:
		indigo_printf(handle, "<newNumberVector device='%s' name='%s'>\n", indigo_xml_escape(device_name), indigo_property_name(device->version, property), indigo_property_state_text[property->state]);
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = &property->items[i];
			indigo_printf(handle, "<oneNumber name='%s'>%g</oneNumber>\n", indigo_item_name(device->version, property, item), item->number.value);
		}
		indigo_printf(handle, "</newNumberVector>\n");
		break;
	case INDIGO_SWITCH_VECTOR:
		indigo_printf(handle, "<newSwitchVector device='%s' name='%s'>\n", indigo_xml_escape(device_name), indigo_property_name(device->version, property), indigo_property_state_text[property->state]);
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = &property->items[i];
			indigo_printf(handle, "<oneSwitch name='%s'>%s</oneSwitch>\n", indigo_item_name(device->version, property, item), item->sw.value ? "On" : "Off");
		}
		indigo_printf(handle, "</newSwitchVector>\n");
		break;
	default:
		break;
	}
	pthread_mutex_unlock(&xml_mutex);
	return INDIGO_OK;
}

static indigo_result xml_client_parser_enable_blob(indigo_device *device, indigo_client *client, indigo_property *property, indigo_enable_blob_mode mode) {
	assert(device != NULL);
	assert(property != NULL);
	if (!indigo_reshare_remote_devices && client && client->is_remote)
		return INDIGO_OK;
	pthread_mutex_lock(&xml_mutex);
	indigo_adapter_context *device_context = (indigo_adapter_context *)device->device_context;
	assert(device_context != NULL);
	int handle = device_context->output;
	char device_name[INDIGO_NAME_SIZE];
	strncpy(device_name, property->device, INDIGO_NAME_SIZE);
	if (indigo_use_host_suffix) {
		char *at = strrchr(device_name, '@');
		if (at != NULL) {
			while (at > device_name && at[-1] == ' ')
				at--;
			*at = 0;
		}
	}
	char *mode_text = "Also";
	if (mode == INDIGO_ENABLE_BLOB_NEVER)
		mode_text = "Never";
	else if (mode == INDIGO_ENABLE_BLOB_URL)
		mode_text = "URL";
	if (*property->name)
		indigo_printf(handle, "<enableBLOB device='%s' name='%s'>%s</enableBLOB>\n", indigo_xml_escape(device_name), indigo_property_name(device->version, property), mode_text);
	else
		indigo_printf(handle, "<enableBLOB device='%s'>%s</enableBLOB>\n", indigo_xml_escape(device_name), mode_text);
	pthread_mutex_unlock(&xml_mutex);
	return INDIGO_OK;
}

static indigo_result xml_client_parser_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_adapter_context *device_context = (indigo_adapter_context *)device->device_context;
	close(device_context->input);
	close(device_context->output);
	return INDIGO_OK;
}

indigo_device *indigo_xml_client_adapter(char *name, char *url_prefix, int input, int output) {
	static indigo_device device_template = {
		"", -1, false, 0, NULL, NULL, INDIGO_OK, INDIGO_VERSION_LEGACY,
		NULL,
		xml_client_parser_enumerate_properties,
		xml_client_parser_change_property,
		xml_client_parser_enable_blob,
		xml_client_parser_detach
	};
	indigo_device *device = malloc(sizeof(indigo_device));
	assert(device != NULL);
	memcpy(device, &device_template, sizeof(indigo_device));
	sprintf(device->name, "@ %s", name);
	device->is_remote = input == output; // is socket, otherwise is pipe
	indigo_adapter_context *device_context = malloc(sizeof(indigo_adapter_context));
	assert(device_context != NULL);
	device_context->input = input;
	device_context->output = output;
	strncpy(device_context->url_prefix, url_prefix, INDIGO_NAME_SIZE);
	device->device_context = device_context;
	return device;
}
