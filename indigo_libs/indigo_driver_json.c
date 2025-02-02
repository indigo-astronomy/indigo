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
#include <ctype.h>
#include <pthread.h>
#include <assert.h>
#include <stdint.h>
#include <arpa/inet.h>


#include <indigo/indigo_json.h>
#include <indigo/indigo_io.h>

//#undef INDIGO_TRACE_PROTOCOL
//#define INDIGO_TRACE_PROTOCOL(c) c

static pthread_mutex_t json_mutex = PTHREAD_MUTEX_INITIALIZER;

static long ws_write(indigo_uni_handle *handle, const char *buffer, long length) {
	uint8_t header[10] = { 0x81 };
	long result;
	if (length <= 0x7D) {
		header[1] = length;
		result = indigo_uni_write(handle, (char *)header, 2);
	} else if (length <= 0xFFFF) {
		header[1] = 0x7E;
		uint16_t payloadLength = htons(length);
		memcpy(header+2, &payloadLength, 2);
		result = indigo_uni_write(handle, (char *)header, 4);
	} else {
		header[1] = 0x7F;
		uint64_t payloadLength = htonll(length);
		memcpy(header+2, &payloadLength, 8);
		result = indigo_uni_write(handle, (char *)header, 10);
	}
	result = result + indigo_uni_write(handle, buffer, length);
	return result;
}

#define SPRINTF(...) { \
	size = sprintf(__VA_ARGS__); \
	pnt += size; \
	size = pnt - output_buffer; \
	if (size + 1024 > buffer_size) { \
		buffer_size *= 2; \
printf("realloc to %ld\n", buffer_size); \
		output_buffer = indigo_safe_realloc(output_buffer, buffer_size); \
		pnt = output_buffer + size; \
	} \
}

static indigo_result json_define_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	assert(device != NULL);
	assert(client != NULL);
	assert(property != NULL);
	if (!indigo_reshare_remote_devices && device->is_remote)
		return INDIGO_OK;
	if (client->version == INDIGO_VERSION_NONE)
		return INDIGO_OK;
	pthread_mutex_lock(&json_mutex);
	indigo_adapter_context *client_context = (indigo_adapter_context *)client->client_context;
	assert(client_context != NULL);
	indigo_uni_handle *handle = client_context->output;
	long buffer_size = JSON_BUFFER_SIZE;
	char *output_buffer = indigo_safe_malloc(buffer_size);
	char *pnt = output_buffer;
	long size;
	char b1[32], b2[32], b3[32], b4[32], b5[32];
	switch (property->type) {
		case INDIGO_TEXT_VECTOR:
			SPRINTF(pnt, "{ \"defTextVector\": { \"version\": %d, \"device\": \"%s\", \"name\": \"%s\", \"group\": \"%s\", \"label\": \"%s\", \"perm\": \"%s\", \"state\": \"%s\"", property->version, property->device, property->name, property->group, indigo_json_escape(property->label), indigo_property_perm_text[property->perm], indigo_property_state_text[property->state]);
			if (*property->hints) {
				SPRINTF(pnt, ", \"hints\": \"%s\"", indigo_json_escape(property->hints));
			}
			if (message) {
				SPRINTF(pnt, ", \"message\": \"%s\", \"items\": [ ", indigo_json_escape(message));
			} else {
				SPRINTF(pnt, ", \"items\": [ ");
			}
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				SPRINTF(pnt, "%s { \"name\": \"%s\", \"label\": \"%s\", \"value\": \"%s\" }",  i > 0 ? "," : "", item->name, indigo_json_escape(item->label), indigo_json_escape(indigo_get_text_item_value(item)));
			}
			size = sprintf(pnt, " ] } }");
			size += pnt - output_buffer;
			break;
		case INDIGO_NUMBER_VECTOR:
			SPRINTF(pnt, "{ \"defNumberVector\": { \"version\": %d, \"device\": \"%s\", \"name\": \"%s\", \"group\": \"%s\", \"label\": \"%s\", \"perm\": \"%s\", \"state\": \"%s\"", property->version, property->device, property->name, property->group, indigo_json_escape(property->label), indigo_property_perm_text[property->perm], indigo_property_state_text[property->state]);
			if (*property->hints) {
				SPRINTF(pnt, ", \"hints\": \"%s\"", indigo_json_escape(property->hints));
			}
			if (message) {
				SPRINTF(pnt, ", \"message\": \"%s\", \"items\": [ ", indigo_json_escape(message));
			} else {
				SPRINTF(pnt, ", \"items\": [ ");
			}
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				if (property->perm != INDIGO_RO_PERM) {
					SPRINTF(pnt, "%s { \"name\": \"%s\", \"label\": \"%s\", \"min\": %s, \"max\": %s, \"step\": %s, \"format\": \"%s\", \"target\": %s, \"value\": %s }",  i > 0 ? "," : "", item->name, indigo_json_escape(item->label), indigo_dtoa(item->number.min, b1), indigo_dtoa(item->number.max, b2), indigo_dtoa(item->number.step, b3), item->number.format, indigo_dtoa(item->number.target, b4), indigo_dtoa(item->number.value, b5));
				} else {
					SPRINTF(pnt, "%s { \"name\": \"%s\", \"label\": \"%s\", \"min\": %s, \"max\": %s, \"step\": %s, \"format\": \"%s\", \"value\": %s }",  i > 0 ? "," : "", item->name, indigo_json_escape(item->label), indigo_dtoa(item->number.min, b1), indigo_dtoa(item->number.max, b2), indigo_dtoa(item->number.step, b3), item->number.format, indigo_dtoa(item->number.value, b4));
				}
			}
			size = sprintf(pnt, " ] } }");
			size += pnt - output_buffer;
			break;
		case INDIGO_SWITCH_VECTOR:
			SPRINTF(pnt, "{ \"defSwitchVector\": { \"version\": %d, \"device\": \"%s\", \"name\": \"%s\", \"group\": \"%s\", \"label\": \"%s\", \"perm\": \"%s\", \"state\": \"%s\", \"rule\": \"%s\"", property->version, property->device, property->name, property->group, indigo_json_escape(property->label), indigo_property_perm_text[property->perm], indigo_property_state_text[property->state], indigo_switch_rule_text[property->rule]);
			if (*property->hints) {
				SPRINTF(pnt, ", \"hints\": \"%s\"", indigo_json_escape(property->hints));
			}
			if (message) {
				SPRINTF(pnt, ", \"message\": \"%s\", \"items\": [ ", indigo_json_escape(message));
			} else {
				SPRINTF(pnt, ", \"items\": [ ");
			}
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				SPRINTF(pnt, "%s { \"name\": \"%s\", \"label\": \"%s\", \"value\": %s }",  i > 0 ? "," : "", item->name, indigo_json_escape(item->label), item->sw.value ? "true" : "false");
			}
			size = sprintf(pnt, " ] } }");
			size += pnt - output_buffer;
			break;
		case INDIGO_LIGHT_VECTOR:
			SPRINTF(pnt, "{ \"defLightVector\": { \"version\": %d, \"device\": \"%s\", \"name\": \"%s\", \"group\": \"%s\", \"label\": \"%s\", \"state\": \"%s\"", property->version, property->device, property->name, property->group, indigo_json_escape(property->label), indigo_property_state_text[property->state]);
			if (*property->hints) {
				SPRINTF(pnt, ", \"hints\": \"%s\"", indigo_json_escape(property->hints));
			}
			if (message) {
				SPRINTF(pnt, ", \"message\": \"%s\", \"items\": [ ", indigo_json_escape(message));
			} else {
				SPRINTF(pnt, ", \"items\": [ ");
			}
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				SPRINTF(pnt, "%s { \"name\": \"%s\", \"label\": \"%s\", \"value\": \"%s\" }",  i > 0 ? "," : "", item->name, indigo_json_escape(item->label), indigo_property_state_text[item->light.value]);
			}
			size = sprintf(pnt, " ] } }");
			size += pnt - output_buffer;
			break;
		case INDIGO_BLOB_VECTOR:
			SPRINTF(pnt, "{ \"defBLOBVector\": { \"version\": %d, \"device\": \"%s\", \"name\": \"%s\", \"group\": \"%s\", \"label\": \"%s\", \"state\": \"%s\"", property->version, property->device, property->name, property->group, indigo_json_escape(property->label), indigo_property_state_text[property->state]);
			if (*property->hints) {
				SPRINTF(pnt, ", \"hints\": \"%s\"", indigo_json_escape(property->hints));
			}
			if (message) {
				SPRINTF(pnt, ", \"message\": \"%s\", \"items\": [ ", indigo_json_escape(message));
			} else {
				SPRINTF(pnt, ", \"items\": [ ");
			}
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				if ((property->state == INDIGO_OK_STATE && item->blob.value) || indigo_proxy_blob) {
					SPRINTF(pnt, "%s { \"name\": \"%s\", \"label\": \"%s\", \"value\": \"/blob/%p%s\" }", i > 0 ? "," : "", item->name, indigo_json_escape(item->label), item, item->blob.format);
				} else if (property->state == INDIGO_OK_STATE && *item->blob.url) {
					SPRINTF(pnt, "%s { \"name\": \"%s\", \"label\": \"%s\", \"value\": \"%s\" }", i > 0 ? "," : "", item->name, indigo_json_escape(item->label), item->blob.url);
				} else {
					SPRINTF(pnt, "%s { \"name\": \"%s\", \"label\": \"%s\"  }", i > 0 ? "," : "", item->name, indigo_json_escape(item->label));
				}
			}
			size = sprintf(pnt, " ] } }");
			size += pnt - output_buffer;
			break;
	}
	if ((client_context->web_socket ? ws_write(handle, output_buffer, size) : indigo_uni_write(handle, output_buffer, size)) < 0) {
		indigo_uni_close(&client_context->input);
		indigo_uni_close(&client_context->output);
	}
	free(output_buffer);
	pthread_mutex_unlock(&json_mutex);
	return INDIGO_OK;
}

static indigo_result json_update_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	assert(device != NULL);
	assert(client != NULL);
	assert(property != NULL);
	if (!indigo_reshare_remote_devices && device->is_remote)
		return INDIGO_OK;
	if (client->version == INDIGO_VERSION_NONE)
		return INDIGO_OK;
	indigo_adapter_context *client_context = (indigo_adapter_context *)client->client_context;
	indigo_uni_handle *handle = client_context->output;
	if (handle == NULL)
		return INDIGO_OK;
	pthread_mutex_lock(&json_mutex);
	assert(client_context != NULL);
	long buffer_size = JSON_BUFFER_SIZE;
	char *output_buffer = indigo_safe_malloc(buffer_size);
	char *pnt = output_buffer;
	long size;
	char b1[32], b2[32];
	switch (property->type) {
		case INDIGO_TEXT_VECTOR:
			SPRINTF(pnt, "{ \"setTextVector\": { \"device\": \"%s\", \"name\": \"%s\", \"state\": \"%s\"", property->device, property->name, indigo_property_state_text[property->state]);
			if (message) {
				SPRINTF(pnt, ", \"message\": \"%s\", \"items\": [ ", message);
			} else {
				SPRINTF(pnt, ", \"items\": [ ");
			}
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				SPRINTF(pnt, "%s { \"name\": \"%s\", \"value\": \"%s\" }",  i > 0 ? "," : "", item->name, indigo_json_escape(indigo_get_text_item_value(item)));
			}
			size = sprintf(pnt, " ] } }");
			size += pnt - output_buffer;
			break;
		case INDIGO_NUMBER_VECTOR:
			SPRINTF(pnt, "{ \"setNumberVector\": { \"device\": \"%s\", \"name\": \"%s\", \"state\": \"%s\"", property->device, property->name, indigo_property_state_text[property->state]);
			if (message) {
				SPRINTF(pnt, ", \"message\": \"%s\", \"items\": [ ", message);
			} else {
				SPRINTF(pnt, ", \"items\": [ ");
			}
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				if (property->perm != INDIGO_RO_PERM) {
					SPRINTF(pnt, "%s { \"name\": \"%s\", \"target\": %s, \"value\": %s }",  i > 0 ? "," : "", item->name, indigo_dtoa(item->number.target, b1), indigo_dtoa(item->number.value, b2));
				} else {
					SPRINTF(pnt, "%s { \"name\": \"%s\", \"value\": %s }",  i > 0 ? "," : "", item->name, indigo_dtoa(item->number.value, b1));
				}
			}
			size = sprintf(pnt, " ] } }");
			size += pnt - output_buffer;
			break;
		case INDIGO_SWITCH_VECTOR:
			SPRINTF(pnt, "{ \"setSwitchVector\": { \"device\": \"%s\", \"name\": \"%s\", \"state\": \"%s\"", property->device, property->name, indigo_property_state_text[property->state]);
			if (message) {
				SPRINTF(pnt, ", \"message\": \"%s\", \"items\": [ ", message);
			} else {
				SPRINTF(pnt, ", \"items\": [ ");
			}
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				SPRINTF(pnt, "%s { \"name\": \"%s\", \"value\": %s }",  i > 0 ? "," : "", item->name, item->sw.value ? "true" : "false");
			}
			size = sprintf(pnt, " ] } }");
			size += pnt - output_buffer;
			break;
		case INDIGO_LIGHT_VECTOR:
			SPRINTF(pnt, "{ \"setLightVector\": { \"device\": \"%s\", \"name\": \"%s\", \"state\": \"%s\"", property->device, property->name, indigo_property_state_text[property->state]);
			if (message) {
				SPRINTF(pnt, ", \"message\": \"%s\", \"items\": [ ", message);
			} else {
				SPRINTF(pnt, ", \"items\": [ ");
			}
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				SPRINTF(pnt, "%s { \"name\": \"%s\", \"value\": \"%s\" }",  i > 0 ? "," : "", item->name, indigo_property_state_text[item->light.value]);
			}
			size = sprintf(pnt, " ] } }");
			size += pnt - output_buffer;
			break;
		case INDIGO_BLOB_VECTOR:
			SPRINTF(pnt, "{ \"setBLOBVector\": { \"device\": \"%s\", \"name\": \"%s\", \"state\": \"%s\"", property->device, property->name, indigo_property_state_text[property->state]);
			if (message) {
				SPRINTF(pnt, ", \"message\": \"%s\", \"items\": [ ", message);
			} else {
				SPRINTF(pnt, ", \"items\": [ ");
			}
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				if ((property->state == INDIGO_OK_STATE && item->blob.value) || indigo_proxy_blob) {
					SPRINTF(pnt, "%s { \"name\": \"%s\", \"value\": \"/blob/%p%s\" }", i > 0 ? "," : "", item->name, item, item->blob.format);
				} else if (property->state == INDIGO_OK_STATE && *item->blob.url) {
					SPRINTF(pnt, "%s { \"name\": \"%s\", \"value\": \"%s\" }", i > 0 ? "," : "", item->name, item->blob.url);
				} else {
					SPRINTF(pnt, "%s { \"name\": \"%s\" }", i > 0 ? "," : "", item->name);
				}
			}
			size = sprintf(pnt, " ] } }");
			size += pnt - output_buffer;
			break;
	}
	if ((client_context->web_socket ? ws_write(handle, output_buffer, size) : indigo_uni_write(handle, output_buffer, size)) < 0) {
		indigo_uni_close(&client_context->input);
		indigo_uni_close(&client_context->output);
	}
	free(output_buffer);
	pthread_mutex_unlock(&json_mutex);
	return INDIGO_OK;
}

static indigo_result json_delete_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	assert(device != NULL);
	assert(client != NULL);
	assert(property != NULL);
	if (!indigo_reshare_remote_devices && device->is_remote)
		return INDIGO_OK;
	if (client->version == INDIGO_VERSION_NONE)
		return INDIGO_OK;
	pthread_mutex_lock(&json_mutex);
	indigo_adapter_context *client_context = (indigo_adapter_context *)client->client_context;
	assert(client_context != NULL);
	indigo_uni_handle *handle = client_context->output;
	char *output_buffer = indigo_safe_malloc(JSON_BUFFER_SIZE);
	char *pnt = output_buffer;
	long size;
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
		indigo_uni_write(handle, output_buffer, size);
	if ((client_context->web_socket ? ws_write(handle, output_buffer, size) : indigo_uni_write(handle, output_buffer, size)) < 0) {
		indigo_uni_close(&client_context->input);
		indigo_uni_close(&client_context->output);
	}
	free(output_buffer);
	pthread_mutex_unlock(&json_mutex);
	return INDIGO_OK;
}

static indigo_result json_message_property(indigo_client *client, indigo_device *device, const char *message) {
	assert(device != NULL);
	assert(client != NULL);
	if (!indigo_reshare_remote_devices && device->is_remote)
		return INDIGO_OK;
	pthread_mutex_lock(&json_mutex);
	indigo_adapter_context *client_context = (indigo_adapter_context *)client->client_context;
	assert(client_context != NULL);
	indigo_uni_handle *handle = client_context->output;
	char *output_buffer = indigo_safe_malloc(JSON_BUFFER_SIZE);
	char *pnt = output_buffer;
	long size = sprintf(pnt, "{ \"message\": \"%s\" }", message);
	if ((client_context->web_socket ? ws_write(handle, output_buffer, size) : indigo_uni_write(handle, output_buffer, size)) < 0) {
		indigo_uni_close(&client_context->input);
		indigo_uni_close(&client_context->output);
	}
	free(output_buffer);
	pthread_mutex_unlock(&json_mutex);
	return INDIGO_OK;
}

static indigo_result json_detach(indigo_client *client) {
	assert(client != NULL);
	indigo_adapter_context *client_context = (indigo_adapter_context *)client->client_context;
	indigo_uni_close(&client_context->input);
	indigo_uni_close(&client_context->output);
	return INDIGO_OK;
}

indigo_client *indigo_json_device_adapter(indigo_uni_handle *input, indigo_uni_handle *output, bool web_socket) {
	static indigo_client client_template = {
		"JSON Driver Adapter", false, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT, NULL,
		NULL,
		json_define_property,
		json_update_property,
		json_delete_property,
		json_message_property,
		json_detach,

	};
	indigo_client *client = indigo_safe_malloc_copy(sizeof(indigo_client), &client_template);
	indigo_adapter_context *client_context = indigo_safe_malloc(sizeof(indigo_adapter_context));
	snprintf(client->name, sizeof(client->name), "JSON Driver Adapter #%d", input->fd);
	client_context->input = input;
	client_context->output = output;
	client_context->web_socket = web_socket;
	client->client_context = client_context;
	client->is_remote = input->type == INDIGO_TCP_HANDLE;
	return client;
}

void indigo_release_json_device_adapter(indigo_client *client) {
	assert(client != NULL);
	assert(client->client_context != NULL);
	free(client->client_context);
	free(client);
}
