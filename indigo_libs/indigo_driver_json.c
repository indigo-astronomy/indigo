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

/** INDIGO JSON wire protocol client side adapter
 \file indigo_driver_json.c
 */

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>
#include <assert.h>
#include <stdint.h>
#include <arpa/inet.h>


#include "indigo_json.h"
#include "indigo_io.h"

//#undef INDIGO_TRACE_PROTOCOL
//#define INDIGO_TRACE_PROTOCOL(c) c

static pthread_mutex_t json_mutex = PTHREAD_MUTEX_INITIALIZER;

static void ws_write(int handle, const char *buffer, long length) {
	uint8_t header[10] = { 0x81 };
	if (length <= 0x7D) {
		header[1] = length;
		indigo_write(handle, (char *)header, 2);
	} else if (length <= 0xFFFF) {
		header[1] = 0x7E;
		uint16_t payloadLength = htons(length);
		memcpy(header+2, &payloadLength, 2);
		indigo_write(handle, (char *)header, 4);
	} else {
		header[1] = 0x7F;
		uint64_t payloadLength = htonll(length);
		memcpy(header+2, &payloadLength, 8);
		indigo_write(handle, (char *)header, 10);
	}
	indigo_write(handle, buffer, length);
}

static indigo_result json_define_property(indigo_client *client, struct indigo_device *device, indigo_property *property, const char *message) {
	assert(device != NULL);
	assert(client != NULL);
	assert(property != NULL);
	if (client->version == INDIGO_VERSION_NONE)
		return INDIGO_OK;
	pthread_mutex_lock(&json_mutex);
	indigo_adapter_context *client_context = (indigo_adapter_context *)client->client_context;
	assert(client_context != NULL);
	int handle = client_context->output;
	char output_buffer[JSON_BUFFER_SIZE];
	char *pnt = output_buffer;
	int size;
	switch (property->type) {
		case INDIGO_TEXT_VECTOR:
			size = sprintf(pnt, "{ \"defTextVector\": { \"version\": %d, \"device\": \"%s\", \"name\": \"%s\", \"group\": \"%s\", \"label\": \"%s\", \"perm\": \"%s\", \"state\": \"%s\"", property->version, property->device, property->name, property->group, property->label, indigo_property_perm_text[property->perm], indigo_property_state_text[property->state]);
			pnt += size;
			if (message) {
				size = sprintf(pnt, ", \"message\": \"%s\", \"items\": [ ", message);
				pnt += size;
			} else {
				size = sprintf(pnt, ", \"items\": [ ");
				pnt += size;
			}
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				size = sprintf(pnt, "%s { \"name\": \"%s\", \"label\": \"%s\", \"value\": \"%s\" }",  i > 0 ? "," : "", item->name, item->label, item->text.value);
				pnt += size;
			}
			size = sprintf(pnt, " ] } }");
			size += pnt - output_buffer;
			break;
		case INDIGO_NUMBER_VECTOR:
			size = sprintf(pnt, "{ \"defNumberVector\": { \"version\": %d, \"device\": \"%s\", \"name\": \"%s\", \"group\": \"%s\", \"label\": \"%s\", \"perm\": \"%s\", \"state\": \"%s\"", property->version, property->device, property->name, property->group, property->label, indigo_property_perm_text[property->perm], indigo_property_state_text[property->state]);
			pnt += size;
			if (message) {
				size = sprintf(pnt, ", \"message\": \"%s\", \"items\": [ ", message);
				pnt += size;
			} else {
				size = sprintf(pnt, ", \"items\": [ ");
				pnt += size;
			}
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				if (property->perm != INDIGO_RO_PERM)
					size = sprintf(pnt, "%s { \"name\": \"%s\", \"label\": \"%s\", \"min\": %g, \"max\": %g, \"step\": %g, \"format\": \"%s\", \"target\": %g, \"value\": %g }",  i > 0 ? "," : "", item->name, item->label, item->number.min, item->number.max, item->number.step, item->number.format, item->number.target, item->number.value);
				else
					size = sprintf(pnt, "%s { \"name\": \"%s\", \"label\": \"%s\", \"min\": %g, \"max\": %g, \"step\": %g, \"format\": \"%s\", \"value\": %g }",  i > 0 ? "," : "", item->name, item->label, item->number.min, item->number.max, item->number.step, item->number.format, item->number.value);
				pnt += size;
			}
			size = sprintf(pnt, " ] } }");
			size += pnt - output_buffer;
			break;
		case INDIGO_SWITCH_VECTOR:
			size = sprintf(pnt, "{ \"defSwitchVector\": { \"version\": %d, \"device\": \"%s\", \"name\": \"%s\", \"group\": \"%s\", \"label\": \"%s\", \"perm\": \"%s\", \"state\": \"%s\", \"rule\": \"%s\"", property->version, property->device, property->name, property->group, property->label, indigo_property_perm_text[property->perm], indigo_property_state_text[property->state], indigo_switch_rule_text[property->rule]);
			pnt += size;
			if (message) {
				size = sprintf(pnt, ", \"message\": \"%s\", \"items\": [ ", message);
				pnt += size;
			} else {
				size = sprintf(pnt, ", \"items\": [ ");
				pnt += size;
			}
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				size = sprintf(pnt, "%s { \"name\": \"%s\", \"label\": \"%s\", \"value\": %s }",  i > 0 ? "," : "", item->name, item->label, item->sw.value ? "true" : "false");
				pnt += size;
			}
			size = sprintf(pnt, " ] } }");
			size += pnt - output_buffer;
			break;
		case INDIGO_LIGHT_VECTOR:
			size = sprintf(pnt, "{ \"defLightVector\": { \"version\": %d, \"device\": \"%s\", \"name\": \"%s\", \"group\": \"%s\", \"label\": \"%s\", \"state\": \"%s\"", property->version, property->device, property->name, property->group, property->label, indigo_property_state_text[property->state]);
			pnt += size;
			if (message) {
				size = sprintf(pnt, ", \"message\": \"%s\", \"items\": [ ", message);
				pnt += size;
			} else {
				size = sprintf(pnt, ", \"items\": [ ");
				pnt += size;
			}
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				size = sprintf(pnt, "%s { \"name\": \"%s\", \"label\": \"%s\", \"value\": \"%s\" }",  i > 0 ? "," : "", item->name, item->label, indigo_property_state_text[item->light.value]);
				pnt += size;
			}
			size = sprintf(pnt, " ] } }");
			size += pnt - output_buffer;
			break;
		case INDIGO_BLOB_VECTOR:
			size = sprintf(pnt, "{ \"defBLOBVector\": { \"version\": %d, \"device\": \"%s\", \"name\": \"%s\", \"group\": \"%s\", \"label\": \"%s\", \"state\": \"%s\"", property->version, property->device, property->name, property->group, property->label, indigo_property_state_text[property->state]);
			pnt += size;
			if (message) {
				size = sprintf(pnt, ", \"message\": \"%s\", \"items\": [ ", message);
				pnt += size;
			} else {
				size = sprintf(pnt, ", \"items\": [ ");
				pnt += size;
			}
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				size = sprintf(pnt, "%s { \"name\": \"%s\", \"label\": \"%s\" }", i > 0 ? "," : "", item->name, item->label);
				pnt += size;
			}
			size = sprintf(pnt, " ] } }");
			size += pnt - output_buffer;
			break;
	}
	if (client_context->web_socket)
		ws_write(handle, output_buffer, size);
	else
		indigo_write(handle, output_buffer, size);
	INDIGO_TRACE_PROTOCOL(indigo_trace("sent: %s\n", output_buffer));
	pthread_mutex_unlock(&json_mutex);
	return INDIGO_OK;
}

static indigo_result json_update_property(indigo_client *client, struct indigo_device *device, indigo_property *property, const char *message) {
	assert(device != NULL);
	assert(client != NULL);
	assert(property != NULL);
	if (client->version == INDIGO_VERSION_NONE)
		return INDIGO_OK;
	pthread_mutex_lock(&json_mutex);
	indigo_adapter_context *client_context = (indigo_adapter_context *)client->client_context;
	assert(client_context != NULL);
	int handle = client_context->output;
	char output_buffer[JSON_BUFFER_SIZE];
	char *pnt = output_buffer;
	int size;
	switch (property->type) {
		case INDIGO_TEXT_VECTOR:
			size = sprintf(pnt, "{ \"setTextVector\": { \"device\": \"%s\", \"name\": \"%s\", \"state\": \"%s\"", property->device, property->name, indigo_property_state_text[property->state]);
			pnt += size;
			if (message) {
				size = sprintf(pnt, ", \"message\": \"%s\", \"items\": [ ", message);
				pnt += size;
			} else {
				size = sprintf(pnt, ", \"items\": [ ");
				pnt += size;
			}
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				size = sprintf(pnt, "%s { \"name\": \"%s\", \"value\": \"%s\" }",  i > 0 ? "," : "", item->name, item->text.value);
				pnt += size;
			}
			size = sprintf(pnt, " ] } }");
			size += pnt - output_buffer;
			break;
		case INDIGO_NUMBER_VECTOR:
			size = sprintf(pnt, "{ \"setNumberVector\": { \"device\": \"%s\", \"name\": \"%s\", \"state\": \"%s\"", property->device, property->name, indigo_property_state_text[property->state]);
			pnt += size;
			if (message) {
				size = sprintf(pnt, ", \"message\": \"%s\", \"items\": [ ", message);
				pnt += size;
			} else {
				size = sprintf(pnt, ", \"items\": [ ");
				pnt += size;
			}
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				if (property->perm != INDIGO_RO_PERM)
					size = sprintf(pnt, "%s { \"name\": \"%s\", \"target\": %g, \"value\": %g }",  i > 0 ? "," : "", item->name, item->number.target, item->number.value);
				else
					size = sprintf(pnt, "%s { \"name\": \"%s\", \"value\": %g }",  i > 0 ? "," : "", item->name, item->number.value);
				pnt += size;
			}
			size = sprintf(pnt, " ] } }");
			size += pnt - output_buffer;
			break;
		case INDIGO_SWITCH_VECTOR:
			size = sprintf(pnt, "{ \"setSwitchVector\": { \"device\": \"%s\", \"name\": \"%s\", \"state\": \"%s\"", property->device, property->name, indigo_property_state_text[property->state]);
			pnt += size;
			if (message) {
				size = sprintf(pnt, ", \"message\": \"%s\", \"items\": [ ", message);
				pnt += size;
			} else {
				size = sprintf(pnt, ", \"items\": [ ");
				pnt += size;
			}
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				size = sprintf(pnt, "%s { \"name\": \"%s\", \"value\": %s }",  i > 0 ? "," : "", item->name, item->sw.value ? "true" : "false");
				pnt += size;
			}
			size = sprintf(pnt, " ] } }");
			size += pnt - output_buffer;
			break;
		case INDIGO_LIGHT_VECTOR:
			size = sprintf(pnt, "{ \"setLightVector\": { \"device\": \"%s\", \"name\": \"%s\", \"state\": \"%s\"", property->device, property->name, indigo_property_state_text[property->state]);
			pnt += size;
			if (message) {
				size = sprintf(pnt, ", \"message\": \"%s\", \"items\": [ ", message);
				pnt += size;
			} else {
				size = sprintf(pnt, ", \"items\": [ ");
				pnt += size;
			}
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				size = sprintf(pnt, "%s { \"name\": \"%s\", \"value\": \"%s\" }",  i > 0 ? "," : "", item->name, indigo_property_state_text[item->light.value]);
				pnt += size;
			}
			size = sprintf(pnt, " ] } }");
			size += pnt - output_buffer;
			break;
		case INDIGO_BLOB_VECTOR:
			size = sprintf(pnt, "{ \"setBLOBVector\": { \"device\": \"%s\", \"name\": \"%s\", \"state\": \"%s\"", property->device, property->name, indigo_property_state_text[property->state]);
			pnt += size;
			if (message) {
				size = sprintf(pnt, ", \"message\": \"%s\", \"items\": [ ", message);
				pnt += size;
			} else {
				size = sprintf(pnt, ", \"items\": [ ");
				pnt += size;
			}
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				if (property->state == INDIGO_OK_STATE)
					size = sprintf(pnt, "%s { \"name\": \"%s\", \"value\": \"/blob/%p%s\" }", i > 0 ? "," : "", item->name, item, item->blob.format);
				else
					size = sprintf(pnt, "%s { \"name\": \"%s\" }", i > 0 ? "," : "", item->name);
				pnt += size;
			}
			size = sprintf(pnt, " ] } }");
			size += pnt - output_buffer;
			break;
	}
	if (client_context->web_socket)
		ws_write(handle, output_buffer, size);
	else
		indigo_write(handle, output_buffer, size);
	INDIGO_TRACE_PROTOCOL(indigo_trace("sent: %s\n", output_buffer));
	pthread_mutex_unlock(&json_mutex);
	return INDIGO_OK;
}

static indigo_result json_delete_property(indigo_client *client, struct indigo_device *device, indigo_property *property, const char *message) {
	assert(device != NULL);
	assert(client != NULL);
	assert(property != NULL);
	if (client->version == INDIGO_VERSION_NONE)
		return INDIGO_OK;
	pthread_mutex_lock(&json_mutex);
	indigo_adapter_context *client_context = (indigo_adapter_context *)client->client_context;
	assert(client_context != NULL);
	int handle = client_context->output;
	char output_buffer[JSON_BUFFER_SIZE];
	char *pnt = output_buffer;
	int size;
	if (*property->name == 0)
		size = sprintf(pnt, "{ \"deleteProperty\": { \"device\": \"%s\"", device->name);
	else
		size = sprintf(pnt, "{ \"deleteProperty\": { \"device\": \"%s\", \"name\": \"%s\"", property->device, property->name);
	pnt += size;
	if (message) {
		size = sprintf(pnt, ", \"message\": \"%s\" } }", message);
	} else {
		size = sprintf(pnt, " } }");
	}
	size += pnt - output_buffer;
	if (client_context->web_socket)
		ws_write(handle, output_buffer, size);
	else
		indigo_write(handle, output_buffer, size);
	INDIGO_TRACE_PROTOCOL(indigo_trace("sent: %s\n", output_buffer));
	pthread_mutex_unlock(&json_mutex);
	return INDIGO_OK;
}

static indigo_result json_message_property(indigo_client *client, struct indigo_device *device, const char *message) {
	assert(device != NULL);
	assert(client != NULL);
	pthread_mutex_lock(&json_mutex);
	indigo_adapter_context *client_context = (indigo_adapter_context *)client->client_context;
	assert(client_context != NULL);
	int handle = client_context->output;
	char output_buffer[JSON_BUFFER_SIZE];
	char *pnt = output_buffer;
	int size = sprintf(pnt, "{ \"message\": \"%s\" }", message);
	if (client_context->web_socket)
		ws_write(handle, output_buffer, size);
	else
		indigo_write(handle, output_buffer, size);
	INDIGO_TRACE_PROTOCOL(indigo_trace("sent: %s\n", output_buffer));
	pthread_mutex_unlock(&json_mutex);
	return INDIGO_OK;
}

static indigo_result json_detach(indigo_client *client) {
	assert(client != NULL);
	indigo_adapter_context *client_context = (indigo_adapter_context *)client->client_context;
	close(client_context->input);
	close(client_context->output);
	return INDIGO_OK;
}

indigo_client *indigo_json_device_adapter(int input, int ouput, bool web_socket) {
	static indigo_client client_template = {
		"", NULL, INDIGO_OK, INDIGO_VERSION_CURRENT, NULL,
		NULL,
		json_define_property,
		json_update_property,
		json_delete_property,
		json_message_property,
		json_detach,

	};
	indigo_client *client = malloc(sizeof(indigo_client));
	assert(client != NULL);
	memcpy(client, &client_template, sizeof(indigo_client));
	indigo_adapter_context *client_context = malloc(sizeof(indigo_adapter_context));
	assert(client_context != NULL);
	client_context->input = input;
	client_context->output = ouput;
	client_context->web_socket = web_socket;
	client->client_context = client_context;
	indigo_enable_blob_mode_record *record = malloc(sizeof(indigo_enable_blob_mode_record));
	memset(record, 0, sizeof(indigo_enable_blob_mode_record));
	record->mode = INDIGO_ENABLE_BLOB_URL;
	client->enable_blob_mode_records = record;
	return client;
}

void indigo_release_json_device_adapter(indigo_client *client) {
	assert(client != NULL);
	assert(client->client_context != NULL);
	indigo_enable_blob_mode_record *record = client->enable_blob_mode_records;
	while (record) {
		indigo_enable_blob_mode_record *tmp = record;
		record = record->next;
		free(tmp);
	}
	free(client->client_context);
	free(client);
}
