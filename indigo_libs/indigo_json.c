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
// 2.0 Build 0 - PoC by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO JSON wire protocol parser
 \file indigo_json.c
 */

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>
#include <assert.h>

#include "indigo_json.h"

static pthread_mutex_t json_mutex = PTHREAD_MUTEX_INITIALIZER;

static bool full_write(int handle, const char *buffer, long length) {
	long remains = length;
	while (true) {
		long bytes_written = write(handle, buffer, remains);
		if (bytes_written < 0)
			return false;
		if (bytes_written == remains)
			return true;
		buffer += bytes_written;
		remains -= bytes_written;
	}
}

static void ws_write(int handle, const char *buffer, long length) {
	uint8_t header[10] = { 0x81 };
	if (length <= 0x7D) {
		header[1] = length;
		full_write(handle, (char *)header, 2);
	} else if (length <= 0xFFFF) {
		header[1] = 0x7E;
		uint16_t payloadLength = htons(length);
		memcpy(header+2, &payloadLength, 2);
		full_write(handle, (char *)header, 4);
	} else {
		header[1] = 0x7F;
		uint64_t payloadLength = htonll(length);
		memcpy(header+2, &payloadLength, 8);
		full_write(handle, (char *)header, 10);
	}
	full_write(handle, buffer, length);
}

static indigo_result json_define_property(indigo_client *client, struct indigo_device *device, indigo_property *property, const char *message) {
	assert(device != NULL);
	assert(client != NULL);
	assert(property != NULL);
	if (client->version == INDIGO_VERSION_NONE)
		return INDIGO_OK;
	pthread_mutex_lock(&json_mutex);
	indigo_json_adapter_context *client_context = (indigo_json_adapter_context *)client->client_context;
	assert(client_context != NULL);
	int handle = client_context->output;
	char *buffer = client_context->output_buffer;
	int size;
	switch (property->type) {
		case INDIGO_TEXT_VECTOR:
			size = sprintf(buffer, "{ \"define\": \"text\", \"device\": \"%s\", \"name\": \"%s\", \"group\": \"%s\", \"label\": \"%s\", \"perm\": \"%s\", \"state\": \"%s\"", property->device, property->name, property->group, property->label, indigo_property_perm_text[property->perm], indigo_property_state_text[property->state]);
			buffer += size;
			if (message) {
				size = sprintf(buffer, " \"message\": \"%s\", \"items\": {", message);
				buffer += size;
			} else {
				size = sprintf(buffer, " \"items\": [");
				buffer += size;
			}
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				size = sprintf(buffer, "%s { \"name\": \"%s\", \"label\": \"%s\", \"value\": \"%s\" }",  i > 0 ? "," : "", item->name, item->label, item->text.value);
				buffer += size;
			}
			size = sprintf(buffer, " ] }");
			size += buffer - client_context->output_buffer;
			break;
		case INDIGO_NUMBER_VECTOR:
			size = sprintf(buffer, "{ \"define\": \"number\", \"device\": \"%s\", \"name\": \"%s\", \"group\": \"%s\", \"label\": \"%s\", \"perm\": \"%s\", \"state\": \"%s\"", property->device, property->name, property->group, property->label, indigo_property_perm_text[property->perm], indigo_property_state_text[property->state]);
			buffer += size;
			if (message) {
				size = sprintf(buffer, " \"message\": \"%s\", \"items\": {", message);
				buffer += size;
			} else {
				size = sprintf(buffer, " \"items\": [");
				buffer += size;
			}
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				size = sprintf(buffer, "%s { \"name\": \"%s\", \"label\": \"%s\", \"min\": %g, \"max\": %g, \"step\": %g, \"value\": %g }",  i > 0 ? "," : "", item->name, item->label, item->number.min, item->number.max, item->number.step, item->number.value);
				buffer += size;
			}
			size = sprintf(buffer, " ] }");
			size += buffer - client_context->output_buffer;
			break;
		case INDIGO_SWITCH_VECTOR:
			size = sprintf(buffer, "{ \"define\": \"switch\", \"device\": \"%s\", \"name\": \"%s\", \"group\": \"%s\", \"label\": \"%s\", \"perm\": \"%s\", \"state\": \"%s\", \"rule\": \"%s\"", property->device, property->name, property->group, property->label, indigo_property_perm_text[property->perm], indigo_property_state_text[property->state], indigo_switch_rule_text[property->rule]);
			buffer += size;
			if (message) {
				size = sprintf(buffer, " \"message\": \"%s\", \"items\": {", message);
				buffer += size;
			} else {
				size = sprintf(buffer, " \"items\": [");
				buffer += size;
			}
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				size = sprintf(buffer, "%s { \"name\": \"%s\", \"label\": \"%s\", \"value\": \"%s\" }",  i > 0 ? "," : "", item->name, item->label, item->sw.value ? "On" : "Off");
				buffer += size;
			}
			size = sprintf(buffer, " ] }");
			size += buffer - client_context->output_buffer;
			break;
		case INDIGO_LIGHT_VECTOR:
			size = sprintf(buffer, "{ \"define\": \"light\", \"device\": \"%s\", \"name\": \"%s\", \"group\": \"%s\", \"label\": \"%s\", \"state\": \"%s\"", property->device, property->name, property->group, property->label, indigo_property_state_text[property->state]);
			buffer += size;
			if (message) {
				size = sprintf(buffer, " \"message\": \"%s\", \"items\": {", message);
				buffer += size;
			} else {
				size = sprintf(buffer, " \"items\": [");
				buffer += size;
			}
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				size = sprintf(buffer, "%s { \"name\": \"%s\", \"label\": \"%s\", \"value\": \"%s\" }",  i > 0 ? "," : "", item->name, item->label, indigo_property_state_text[item->light.value]);
				buffer += size;
			}
			size = sprintf(buffer, " ] }");
			size += buffer - client_context->output_buffer;
			break;
		case INDIGO_BLOB_VECTOR:
			size = sprintf(buffer, "{ \"define\": \"blob\", \"device\": \"%s\", \"name\": \"%s\", \"group\": \"%s\", \"label\": \"%s\", \"state\": \"%s\"", property->device, property->name, property->group, property->label, indigo_property_state_text[property->state]);
			buffer += size;
			if (message) {
				size = sprintf(buffer, " \"message\": \"%s\", \"items\": {", message);
				buffer += size;
			} else {
				size = sprintf(buffer, " \"items\": [");
				buffer += size;
			}
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				size = sprintf(buffer, "%s { \"name\": \"%s\", \"label\": \"%s\" }", i > 0 ? "," : "", item->name, item->label);
				buffer += size;
			}
			size = sprintf(buffer, " ] }");
			size += buffer - client_context->output_buffer;
			break;
	}
	printf("%s\n", buffer);
	pthread_mutex_unlock(&json_mutex);
	return INDIGO_OK;
}

static bool full_read(int handle, char *buffer, long length) {
	long remains = length;
	while (true) {
		long bytes_read = read(handle, buffer, remains);
		if (bytes_read <= 0)
			return false;
		if (bytes_read == remains)
			return true;
		buffer += bytes_read;
		remains -= bytes_read;
	}
}

static long ws_read(int handle, char *buffer, long length) {
	uint8_t header[14];
	read(0, header, 6);
	uint8_t *masking_key = header+2;
	uint64_t payload_length = header[1] & 0x7F;
	if (payload_length == 0x7E) {
		full_read(0, (char *)header + 6, 2);
		masking_key = header + 4;
		payload_length = ntohs(*((uint16_t *)(header+2)));
	} else if (payload_length == 0x7F) {
		full_read(0, (char *)header + 6, 8);
		masking_key = header+10;
		payload_length = ntohll(*((uint64_t *)(header+2)));
	}
	if (length < payload_length)
		return -1;
	full_read(0, buffer, payload_length);
	for (uint64_t i = 0; i < payload_length; i++) {
		buffer[i] ^= masking_key[i%4];
	}
	return payload_length;
}

indigo_client *indigo_json_device_adapter(int input, int ouput) {
	static indigo_client client_template = {
		"", NULL, INDIGO_OK, INDIGO_VERSION_NONE, INDIGO_ENABLE_BLOB_ALSO,
		NULL,
		json_define_property,
		NULL,
		NULL,
		NULL,
		NULL
	};
	indigo_client *client = malloc(sizeof(indigo_client));
	assert(client != NULL);
	memcpy(client, &client_template, sizeof(indigo_client));
	indigo_json_adapter_context *client_context = malloc(sizeof(indigo_json_adapter_context));
	assert(client_context != NULL);
	client_context->input = input;
	client_context->output = ouput;
	client->client_context = client_context;
	return client;
}
