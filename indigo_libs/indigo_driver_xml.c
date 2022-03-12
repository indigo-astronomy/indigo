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

#include <indigo/indigo_xml.h>
#include <indigo/indigo_io.h>
#include <indigo/indigo_base64.h>
#include <indigo/indigo_version.h>
#include <indigo/indigo_driver_xml.h>

#define RAW_BUF_SIZE 98304
#define BASE64_BUF_SIZE 131072  /* BASE64_BUF_SIZE >= (RAW_BUF_SIZE + 2) / 3 * 4 */
#define INDIGO_PRINTF(...) if (!indigo_printf(__VA_ARGS__)) goto failure

static pthread_mutex_t write_mutex = PTHREAD_MUTEX_INITIALIZER;

static const char *message_attribute(const char *message) {
	if (message) {
		static char buffer[INDIGO_VALUE_SIZE];
		snprintf(buffer, INDIGO_VALUE_SIZE, " message='%s'", indigo_xml_escape((char *)message));
		return buffer;
	}
	return "";
}

static const char *hints_attribute(const char *hints) {
	if (*hints) {
		static char buffer[INDIGO_VALUE_SIZE];
		snprintf(buffer, INDIGO_VALUE_SIZE, " hints='%s'", indigo_xml_escape((char *)hints));
		return buffer;
	}
	return "";
}

static indigo_result xml_device_adapter_define_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	assert(device != NULL);
	assert(client != NULL);
	assert(property != NULL);
	if (!indigo_reshare_remote_devices && device->is_remote)
		return INDIGO_OK;
	if (client->version == INDIGO_VERSION_NONE)
		return INDIGO_OK;
	indigo_adapter_context *client_context = (indigo_adapter_context *)client->client_context;
	if (client_context->output <= 0)
		return INDIGO_OK;
	pthread_mutex_lock(&write_mutex);
	assert(client_context != NULL);
	int handle = client_context->output;
	char b1[32], b2[32], b3[32], b4[32], b5[32];
	switch (property->type) {
	case INDIGO_TEXT_VECTOR:
		INDIGO_PRINTF(handle, "<defTextVector device='%s' name='%s' group='%s' label='%s' perm='%s' state='%s'%s%s>\n", indigo_xml_escape(property->device), indigo_property_name(client->version, property), indigo_xml_escape(property->group), indigo_xml_escape(property->label), indigo_property_perm_text[property->perm], indigo_property_state_text[property->state], hints_attribute(property->hints), message_attribute(message));
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = &property->items[i];
			INDIGO_PRINTF(handle, "<defText name='%s' label='%s'%s>%s</defText>\n", indigo_item_name(client->version, property, item), indigo_xml_escape(item->label), hints_attribute(item->hints), indigo_xml_escape(indigo_get_text_item_value(item)));
		}
		INDIGO_PRINTF(handle, "</defTextVector>\n");
		break;
	case INDIGO_NUMBER_VECTOR:
		INDIGO_PRINTF(handle, "<defNumberVector device='%s' name='%s' group='%s' label='%s' perm='%s' state='%s'%s%s>\n", indigo_xml_escape(property->device), indigo_property_name(client->version, property), indigo_xml_escape(property->group), indigo_xml_escape(property->label), indigo_property_perm_text[property->perm], indigo_property_state_text[property->state], hints_attribute(property->hints), message_attribute(message));
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = &property->items[i];
			if (client->version >= INDIGO_VERSION_2_0 && property->perm != INDIGO_RO_PERM) {
				INDIGO_PRINTF(handle, "<defNumber name='%s' label='%s' format='%s' min='%s' max='%s' step='%s' target='%s'>%s</defNumber>\n", indigo_item_name(client->version, property, item), indigo_xml_escape(item->label), item->number.format, indigo_dtoa(item->number.min, b1), indigo_dtoa(item->number.max, b2), indigo_dtoa(item->number.step, b3), indigo_dtoa(item->number.target, b4), indigo_dtoa(item->number.value, b5));
			} else {
				INDIGO_PRINTF(handle, "<defNumber name='%s' label='%s'%s format='%s' min='%s' max='%s' step='%s'>%s</defNumber>\n", indigo_item_name(client->version, property, item), indigo_xml_escape(item->label), hints_attribute(item->hints), item->number.format, indigo_dtoa(item->number.min, b1), indigo_dtoa(item->number.max, b2), indigo_dtoa(item->number.step, b3), indigo_dtoa(item->number.value, b4));
			}
		}
		INDIGO_PRINTF(handle, "</defNumberVector>\n");
		break;
	case INDIGO_SWITCH_VECTOR:
		INDIGO_PRINTF(handle, "<defSwitchVector device='%s' name='%s' group='%s' label='%s' perm='%s' state='%s' rule='%s'%s%s>\n", indigo_xml_escape(property->device), indigo_property_name(client->version, property), indigo_xml_escape(property->group), indigo_xml_escape(property->label), indigo_property_perm_text[property->perm], indigo_property_state_text[property->state], indigo_switch_rule_text[property->rule], hints_attribute(property->hints), message_attribute(message));
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = &property->items[i];
			INDIGO_PRINTF(handle, "<defSwitch name='%s' label='%s'%s>%s</defSwitch>\n", indigo_item_name(client->version, property, item), indigo_xml_escape(item->label), hints_attribute(item->hints), item->sw.value ? "On" : "Off");
		}
		INDIGO_PRINTF(handle, "</defSwitchVector>\n");
		break;
	case INDIGO_LIGHT_VECTOR:
		INDIGO_PRINTF(handle, "<defLightVector device='%s' name='%s' group='%s' label='%s' perm='%s' state='%s'%s%s>\n", indigo_xml_escape(property->device), indigo_property_name(client->version, property), indigo_xml_escape(property->group), indigo_xml_escape(property->label), indigo_property_perm_text[property->perm], indigo_property_state_text[property->state], hints_attribute(property->hints), message_attribute(message));
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = &property->items[i];
			INDIGO_PRINTF(handle, " <defLight name='%s' label='%s'%s>%s</defLight>\n", indigo_item_name(client->version, property, item), indigo_xml_escape(item->label), hints_attribute(item->hints), indigo_property_state_text[item->light.value]);
		}
		INDIGO_PRINTF(handle, "</defLightVector>\n");
		break;
	case INDIGO_BLOB_VECTOR:
		INDIGO_PRINTF(handle, "<defBLOBVector device='%s' name='%s' group='%s' label='%s' perm='%s' state='%s'%s%s>\n", indigo_xml_escape(property->device), indigo_property_name(client->version, property), indigo_xml_escape(property->group), indigo_xml_escape(property->label), indigo_property_perm_text[property->perm], indigo_property_state_text[property->state], hints_attribute(property->hints), message_attribute(message));
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = &property->items[i];
			if (property->perm == INDIGO_WO_PERM && client->version >= INDIGO_VERSION_2_0) {
				if (item->blob.url[0] == 0 || indigo_proxy_blob) {
					INDIGO_PRINTF(handle, "<defBLOB name='%s' path='/blob/%p' label='%s'%s/>\n", indigo_item_name(client->version, property, item), item, indigo_xml_escape(item->label), hints_attribute(item->hints));
				} else {
					INDIGO_PRINTF(handle, "<defBLOB name='%s' url='%s' label='%s'%s/>\n", indigo_item_name(client->version, property, item), item->blob.url, indigo_xml_escape(item->label), hints_attribute(item->hints));
				}
			} else {
				INDIGO_PRINTF(handle, "<defBLOB name='%s' label='%s'%s/>\n", indigo_item_name(client->version, property, item), indigo_xml_escape(item->label), hints_attribute(item->hints));
			}
		}
		INDIGO_PRINTF(handle, "</defBLOBVector>\n");
		break;
	}
	pthread_mutex_unlock(&write_mutex);
	return INDIGO_OK;
failure:
	if (client_context->output == client_context->input) {
		close(client_context->input);
	} else {
		close(client_context->input);
		close(client_context->output);
	}
	client_context->output = client_context->input = -1;
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
	indigo_adapter_context *client_context = (indigo_adapter_context *)client->client_context;
	if (client_context->output <= 0)
		return INDIGO_OK;
	pthread_mutex_lock(&write_mutex);
	assert(client_context != NULL);
	int handle = client_context->output;
	char b1[32], b2[32];
	switch (property->type) {
		case INDIGO_TEXT_VECTOR:
			INDIGO_PRINTF(handle, "<setTextVector device='%s' name='%s' state='%s'%s>\n", indigo_xml_escape(property->device), indigo_property_name(client->version, property), indigo_property_state_text[property->state], message_attribute(message));
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				INDIGO_PRINTF(handle, "<oneText name='%s'>%s</oneText>\n", indigo_item_name(client->version, property, item), indigo_xml_escape(indigo_get_text_item_value(item)));
			}
			INDIGO_PRINTF(handle, "</setTextVector>\n");
			break;
		case INDIGO_NUMBER_VECTOR:
			INDIGO_PRINTF(handle, "<setNumberVector device='%s' name='%s' state='%s'%s>\n", indigo_xml_escape(property->device), indigo_property_name(client->version, property), indigo_property_state_text[property->state], message_attribute(message));
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				if (client->version >= INDIGO_VERSION_2_0 && property->perm != INDIGO_RO_PERM) {
					INDIGO_PRINTF(handle, "<oneNumber name='%s' target='%s'>%s</oneNumber>\n", indigo_item_name(client->version, property, item), indigo_dtoa(item->number.target, b1), indigo_dtoa(item->number.value, b2));
				} else {
					INDIGO_PRINTF(handle, "<oneNumber name='%s'>%s</oneNumber>\n", indigo_item_name(client->version, property, item), indigo_dtoa(item->number.value, b1));
				}
			}
			INDIGO_PRINTF(handle, "</setNumberVector>\n");
			break;
		case INDIGO_SWITCH_VECTOR:
			INDIGO_PRINTF(handle, "<setSwitchVector device='%s' name='%s' state='%s'%s>\n", indigo_xml_escape(property->device), indigo_property_name(client->version, property), indigo_property_state_text[property->state], message_attribute(message));
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				INDIGO_PRINTF(handle, "<oneSwitch name='%s'>%s</oneSwitch>\n", indigo_item_name(client->version, property, item), item->sw.value ? "On" : "Off");
			}
			INDIGO_PRINTF(handle, "</setSwitchVector>\n");
			break;
		case INDIGO_LIGHT_VECTOR:
			INDIGO_PRINTF(handle, "<setLightVector device='%s' name='%s' state='%s'%s>\n", indigo_xml_escape(property->device), indigo_property_name(client->version, property), indigo_property_state_text[property->state], message_attribute(message));
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				INDIGO_PRINTF(handle, "<oneLight name='%s'>%s</oneLight>\n", indigo_item_name(client->version, property, item), indigo_property_state_text[item->light.value]);
			}
			INDIGO_PRINTF(handle, "</setLightVector>\n");
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
				INDIGO_PRINTF(handle, "<setBLOBVector device='%s' name='%s' state='%s'%s>\n", indigo_xml_escape(property->device), indigo_property_name(client->version, property), indigo_property_state_text[property->state], message_attribute(message));
				if (property->state == INDIGO_OK_STATE) {
					for (int i = 0; i < property->count; i++) {
						indigo_item *item = &property->items[i];
						if (mode == INDIGO_ENABLE_BLOB_URL && client->version >= INDIGO_VERSION_2_0) {
							if (item->blob.value || indigo_proxy_blob) {
								INDIGO_PRINTF(handle, "<oneBLOB name='%s' path='/blob/%p%s'/>\n", indigo_item_name(client->version, property, item), item, item->blob.format);
							} else {
								INDIGO_PRINTF(handle, "<oneBLOB name='%s' url='%s'/>\n", indigo_item_name(client->version, property, item), item->blob.url);
							}
						} else {
							long input_length = item->blob.size;
							unsigned char *data = item->blob.value;
							INDIGO_PRINTF(handle, "<oneBLOB name='%s' format='%s' size='%ld'>\n", indigo_item_name(client->version, property, item), item->blob.format, item->blob.size);
							handle2 = dup(handle);
							fh = fdopen(handle2, "w");
							if (client->version >= INDIGO_VERSION_2_0) {
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
							INDIGO_PRINTF(handle, "</oneBLOB>\n");
						}
					}
				}
				INDIGO_PRINTF(handle, "</setBLOBVector>\n");
			}
			break;
		}
	}
	pthread_mutex_unlock(&write_mutex);
	return INDIGO_OK;
failure:
	if (client_context->output == client_context->input) {
		close(client_context->input);
	} else {
		close(client_context->input);
		close(client_context->output);
	}
	client_context->output = client_context->input = -1;
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
	indigo_adapter_context *client_context = (indigo_adapter_context *)client->client_context;
	if (client_context->output <= 0)
		return INDIGO_OK;
	pthread_mutex_lock(&write_mutex);
	assert(client_context != NULL);
	int handle = client_context->output;
	if (*property->name) {
		INDIGO_PRINTF(handle, "<delProperty device='%s' name='%s'%s/>\n", indigo_xml_escape(property->device), indigo_property_name(client->version, property), message_attribute(message));
	} else {
		INDIGO_PRINTF(handle, "<delProperty device='%s'%s/>\n", device->name, message_attribute(message));
	}
	pthread_mutex_unlock(&write_mutex);
	return INDIGO_OK;
failure:
	if (client_context->output == client_context->input) {
		close(client_context->input);
	} else {
		close(client_context->input);
		close(client_context->output);
	}
	client_context->output = client_context->input = -1;
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
	indigo_adapter_context *client_context = (indigo_adapter_context *)client->client_context;
	if (client_context->output <= 0)
		return INDIGO_OK;
	pthread_mutex_lock(&write_mutex);
	assert(client_context != NULL);
	int handle = client_context->output;
	if (message) {
		if (device) {
			INDIGO_PRINTF(handle, "<message device='%s'%s/>\n", device->name, message_attribute(message));
		} else {
			INDIGO_PRINTF(handle, "<message%s/>\n", message_attribute(message));
		}
	}
	pthread_mutex_unlock(&write_mutex);
	return INDIGO_OK;
failure:
	if (client_context->output == client_context->input) {
		close(client_context->input);
	} else {
		close(client_context->input);
		close(client_context->output);
	}
	client_context->output = client_context->input = -1;
	pthread_mutex_unlock(&write_mutex);
	return INDIGO_OK;
}

indigo_client *indigo_xml_device_adapter(int input, int ouput) {
	static indigo_client client_template = {
		"XML Driver Adapter", false, NULL, INDIGO_OK, INDIGO_VERSION_NONE, NULL,
		NULL,
		xml_device_adapter_define_property,
		xml_device_adapter_update_property,
		xml_device_adapter_delete_property,
		xml_device_adapter_send_message,
		NULL
	};
	indigo_client *client = indigo_safe_malloc_copy(sizeof(indigo_client), &client_template);
	indigo_adapter_context *client_context = indigo_safe_malloc(sizeof(indigo_adapter_context));
	client_context->input = input;
	client_context->output = ouput;
	client->client_context = client_context;
	client->is_remote = input == ouput;
	return client;
}

void indigo_release_xml_device_adapter(indigo_client *client) {
	assert(client != NULL);
	assert(client->client_context != NULL);
	indigo_enable_blob_mode_record *blob_record = client->enable_blob_mode_records;
	while (blob_record) {
		client->enable_blob_mode_records = blob_record->next;
		free(blob_record);
		blob_record = client->enable_blob_mode_records;
	}
	free(client->client_context);
	free(client);
}
