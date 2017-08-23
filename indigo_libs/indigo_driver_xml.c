// Copyright (c) 2016 CloudMakers, s. r. o.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following
// disclaimer in the documentation and/or other materials provided
// with the distribution.
//
// 3. The name of the author may not be used to endorse or promote
// products derived from this software without specific prior
// written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR 'AS IS' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// version history
// 2.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO XML wire protocol client side adapter
 \file indigo_driver_xml.c
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
#include "indigo_base64.h"
#include "indigo_version.h"
#include "indigo_driver_xml.h"

#define RAW_BUF_SIZE 98304
#define BASE64_BUF_SIZE 131072  /* BASE64_BUF_SIZE >= (RAW_BUF_SIZE + 2) / 3 * 4 */

static pthread_mutex_t write_mutex = PTHREAD_MUTEX_INITIALIZER;

static const char *message_attribute(const char *message) {
	if (message) {
		static char buffer[INDIGO_VALUE_SIZE];
		snprintf(buffer, INDIGO_VALUE_SIZE, " message='%s'", indigo_xml_escape((char *)message));
		return buffer;
	}
	return "";
}

static indigo_result xml_device_adapter_define_property(indigo_client *client, struct indigo_device *device, indigo_property *property, const char *message) {
	assert(device != NULL);
	assert(client != NULL);
	assert(property != NULL);
	if (!indigo_reshare_remote_devices && device->is_remote)
		return INDIGO_OK;
	if (client->version == INDIGO_VERSION_NONE)
		return INDIGO_OK;
	pthread_mutex_lock(&write_mutex);
	indigo_adapter_context *client_context = (indigo_adapter_context *)client->client_context;
	assert(client_context != NULL);
	int handle = client_context->output;
	switch (property->type) {
	case INDIGO_TEXT_VECTOR:
		indigo_printf(handle, "<defTextVector device='%s' name='%s' group='%s' label='%s' perm='%s' state='%s'%s>\n", indigo_xml_escape(property->device), indigo_property_name(client->version, property), indigo_xml_escape(property->group), indigo_xml_escape(property->label), indigo_property_perm_text[property->perm], indigo_property_state_text[property->state], message_attribute(message));
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = &property->items[i];
			indigo_printf(handle, "<defText name='%s' label='%s'>%s</defText>\n", indigo_item_name(client->version, property, item), item->label, item->text.value);
		}
		indigo_printf(handle, "</defTextVector>\n");
		break;
	case INDIGO_NUMBER_VECTOR:
		indigo_printf(handle, "<defNumberVector device='%s' name='%s' group='%s' label='%s' perm='%s' state='%s'%s>\n", indigo_xml_escape(property->device), indigo_property_name(client->version, property), indigo_xml_escape(property->group), indigo_xml_escape(property->label), indigo_property_perm_text[property->perm], indigo_property_state_text[property->state], message_attribute(message));
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = &property->items[i];
			if (client->version >= INDIGO_VERSION_2_0 && property->perm != INDIGO_RO_PERM)
				indigo_printf(handle, "<defNumber name='%s' label='%s' format='%s' min='%g' max='%g' step='%g' target='%g'>%g</defNumber>\n", indigo_item_name(client->version, property, item), item->label, item->number.format, item->number.min, item->number.max, item->number.step, item->number.target, item->number.value);
			else
				indigo_printf(handle, "<defNumber name='%s' label='%s' format='%s' min='%g' max='%g' step='%g'>%g</defNumber>\n", indigo_item_name(client->version, property, item), item->label, item->number.format, item->number.min, item->number.max, item->number.step, item->number.value);
		}
		indigo_printf(handle, "</defNumberVector>\n");
		break;
	case INDIGO_SWITCH_VECTOR:
		indigo_printf(handle, "<defSwitchVector device='%s' name='%s' group='%s' label='%s' perm='%s' state='%s' rule='%s'%s>\n", indigo_xml_escape(property->device), indigo_property_name(client->version, property), indigo_xml_escape(property->group), indigo_xml_escape(property->label), indigo_property_perm_text[property->perm], indigo_property_state_text[property->state], indigo_switch_rule_text[property->rule], message_attribute(message));
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = &property->items[i];
			indigo_printf(handle, "<defSwitch name='%s' label='%s'>%s</defSwitch>\n", indigo_item_name(client->version, property, item), item->label, item->sw.value ? "On" : "Off");
		}
		indigo_printf(handle, "</defSwitchVector>\n");
		break;
	case INDIGO_LIGHT_VECTOR:
		indigo_printf(handle, "<defLightVector device='%s' name='%s' group='%s' label='%s' perm='%s' state='%s'%s>\n", indigo_xml_escape(property->device), indigo_property_name(client->version, property), indigo_xml_escape(property->group), indigo_xml_escape(property->label), indigo_property_perm_text[property->perm], indigo_property_state_text[property->state], message_attribute(message));
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = &property->items[i];
			indigo_printf(handle, " <defLight name='%s' label='%s'>%s</defLight>\n", indigo_item_name(client->version, property, item), item->label, indigo_property_state_text[item->light.value]);
		}
		indigo_printf(handle, "</defLightVector>\n");
		break;
	case INDIGO_BLOB_VECTOR:
		indigo_printf(handle, "<defBLOBVector device='%s' name='%s' group='%s' label='%s' perm='%s' state='%s'%s>\n", indigo_xml_escape(property->device), indigo_property_name(client->version, property), indigo_xml_escape(property->group), indigo_xml_escape(property->label), indigo_property_perm_text[property->perm], indigo_property_state_text[property->state], message_attribute(message));
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = &property->items[i];
			indigo_printf(handle, "<defBLOB name='%s' label='%s'/>\n", indigo_item_name(client->version, property, item), item->label);
		}
		indigo_printf(handle, "</defBLOBVector>\n");
		break;
	}
	pthread_mutex_unlock(&write_mutex);
	return INDIGO_OK;
}

static indigo_result xml_device_adapter_update_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	FILE* fh;
	int handle2;
	assert(device != NULL);
	assert(client != NULL);
	assert(property != NULL);
	if (!indigo_reshare_remote_devices && device->is_remote)
		return INDIGO_OK;
	if (client->version == INDIGO_VERSION_NONE)
		return INDIGO_OK;
	pthread_mutex_lock(&write_mutex);
	indigo_adapter_context *client_context = (indigo_adapter_context *)client->client_context;
	assert(client_context != NULL);
	int handle = client_context->output;
	switch (property->type) {
		case INDIGO_TEXT_VECTOR:
			indigo_printf(handle, "<setTextVector device='%s' name='%s' state='%s'%s>\n", indigo_xml_escape(property->device), indigo_property_name(client->version, property), indigo_property_state_text[property->state], message_attribute(message));
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				indigo_printf(handle, "<oneText name='%s'>%s</oneText>\n", indigo_item_name(client->version, property, item), indigo_xml_escape(item->text.value));
			}
			indigo_printf(handle, "</setTextVector>\n");
			break;
		case INDIGO_NUMBER_VECTOR:
			indigo_printf(handle, "<setNumberVector device='%s' name='%s' state='%s'%s>\n", indigo_xml_escape(property->device), indigo_property_name(client->version, property), indigo_property_state_text[property->state], message_attribute(message));
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				if (client->version >= INDIGO_VERSION_2_0 && property->perm != INDIGO_RO_PERM)
					indigo_printf(handle, "<oneNumber name='%s' target='%g'>%g</oneNumber>\n", indigo_item_name(client->version, property, item), item->number.target, item->number.value);
				else
					indigo_printf(handle, "<oneNumber name='%s'>%g</oneNumber>\n", indigo_item_name(client->version, property, item), item->number.value);
			}
			indigo_printf(handle, "</setNumberVector>\n");
			break;
		case INDIGO_SWITCH_VECTOR:
			indigo_printf(handle, "<setSwitchVector device='%s' name='%s' state='%s'%s>\n", indigo_xml_escape(property->device), indigo_property_name(client->version, property), indigo_property_state_text[property->state], message_attribute(message));
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				indigo_printf(handle, "<oneSwitch name='%s'>%s</oneSwitch>\n", indigo_item_name(client->version, property, item), item->sw.value ? "On" : "Off");
			}
			indigo_printf(handle, "</setSwitchVector>\n");
			break;
		case INDIGO_LIGHT_VECTOR:
			indigo_printf(handle, "<setLightVector device='%s' name='%s' state='%s'%s>\n", indigo_xml_escape(property->device), indigo_property_name(client->version, property), indigo_property_state_text[property->state], message_attribute(message));
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				indigo_printf(handle, "<oneLight name='%s'>%s</oneLight>\n", indigo_item_name(client->version, property, item), indigo_property_state_text[item->light.value]);
			}
			indigo_printf(handle, "</setLightVector>\n");
			break;
		case INDIGO_BLOB_VECTOR: {
			indigo_enable_blob_mode mode = INDIGO_ENABLE_BLOB_NEVER;
			indigo_enable_blob_mode_record *record = client->enable_blob_mode_records;
			while (record) {
				if ((*record->device == 0 || !strcmp(property->device, record->device)) && (*record->name == 0 || !strcmp(property->name, record->name))) {
					mode = record->mode;
					break;
				}
				record = record->next;
			}
			if (mode != INDIGO_ENABLE_BLOB_NEVER) {
				indigo_printf(handle, "<setBLOBVector device='%s' name='%s' state='%s'%s>\n", indigo_xml_escape(property->device), indigo_property_name(client->version, property), indigo_property_state_text[property->state], message_attribute(message));
				if (property->state == INDIGO_OK_STATE) {
					for (int i = 0; i < property->count; i++) {
						indigo_item *item = &property->items[i];
						long input_length = item->blob.size;
						unsigned char *data = item->blob.value;
						if (mode == INDIGO_ENABLE_BLOB_URL) {
							if (*item->blob.url == 0)
								indigo_printf(handle, "<oneBLOB name='%s' path='/blob/%p%s'/>\n", indigo_item_name(client->version, property, item), item, item->blob.format);
							else
								indigo_printf(handle, "<oneBLOB name='%s' url='%s'/>\n", indigo_item_name(client->version, property, item), item->blob.url);
						} else {
							indigo_printf(handle, "<oneBLOB name='%s' format='%s' size='%ld'>\n", indigo_item_name(client->version, property, item), item->blob.format, item->blob.size);
							handle2 = dup(handle);
							fh = fdopen(handle2, "w");
							if (property->version >= INDIGO_VERSION_2_0) {
								while (input_length) {
									char encoded_data[BASE64_BUF_SIZE + 1];
									long len = (RAW_BUF_SIZE < input_length) ?  RAW_BUF_SIZE : input_length;
									long enclen = base64_encode((unsigned char*)encoded_data, (unsigned char*)data, len);
									fwrite(encoded_data, 1, enclen, fh);
									input_length -= len;
									data += len;
								}
							} else {
								static char encoded_data[74];
								while (input_length) {
									/* 54 raw = 72 encoded */
									long len = (54 < input_length) ?  54 : input_length;
									long enclen = base64_encode((unsigned char*)encoded_data, (unsigned char*)data, len);
									encoded_data[enclen] = '\n';
									fwrite(encoded_data, 1, enclen, fh);
									input_length -= len;
									data += len;
								}
							}
							fflush(fh);
							fclose(fh);
							indigo_printf(handle, "</oneBLOB>\n");
						}
					}
				}
				indigo_printf(handle, "</setBLOBVector>\n");
			}
			break;
		}
	}
	pthread_mutex_unlock(&write_mutex);
	return INDIGO_OK;
}

static indigo_result xml_device_adapter_delete_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	assert(device != NULL);
	assert(client != NULL);
	assert(property != NULL);
	if (!indigo_reshare_remote_devices && device->is_remote)
		return INDIGO_OK;
	if (client->version == INDIGO_VERSION_NONE)
		return INDIGO_OK;
	pthread_mutex_lock(&write_mutex);
	indigo_adapter_context *client_context = (indigo_adapter_context *)client->client_context;
	assert(client_context != NULL);
	int handle = client_context->output;
	if (*property->name)
		indigo_printf(handle, "<delProperty device='%s' name='%s'%s/>\n", indigo_xml_escape(property->device), indigo_property_name(client->version, property), message_attribute(message));
	else
		indigo_printf(handle, "<delProperty device='%s'%s/>\n", device->name, message_attribute(message));
	pthread_mutex_unlock(&write_mutex);
	return INDIGO_OK;
}

static indigo_result xml_device_adapter_send_message(indigo_client *client, indigo_device *device, const char *message) {
	assert(device != NULL);
	assert(client != NULL);
	if (!indigo_reshare_remote_devices && device->is_remote)
		return INDIGO_OK;
	if (client->version == INDIGO_VERSION_NONE)
		return INDIGO_OK;
	pthread_mutex_lock(&write_mutex);
	indigo_adapter_context *client_context = (indigo_adapter_context *)client->client_context;
	assert(client_context != NULL);
	int handle = client_context->output;
	if (message)
		indigo_printf(handle, "<message%s/>\n", message_attribute(message));
	pthread_mutex_unlock(&write_mutex);
	return INDIGO_OK;
}

indigo_client *indigo_xml_device_adapter(int input, int ouput) {
	static indigo_client client_template = {
		"", NULL, INDIGO_OK, INDIGO_VERSION_NONE, NULL,
		NULL,
		xml_device_adapter_define_property,
		xml_device_adapter_update_property,
		xml_device_adapter_delete_property,
		xml_device_adapter_send_message,
		NULL
	};
	indigo_client *client = malloc(sizeof(indigo_client));
	assert(client != NULL);
	memcpy(client, &client_template, sizeof(indigo_client));
	indigo_adapter_context *client_context = malloc(sizeof(indigo_adapter_context));
	assert(client_context != NULL);
	client_context->input = input;
	client_context->output = ouput;
	client->client_context = client_context;
	return client;
}

void indigo_release_xml_device_adapter(indigo_client *client) {
	assert(client != NULL);
	assert(client->client_context != NULL);
	free(client->client_context);
	free(client);
}

