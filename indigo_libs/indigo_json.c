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
#include <stdint.h>
#include <arpa/inet.h>

#include "indigo_json.h"
#include "indigo_io.h"

//#undef INDIGO_TRACE_PROTOCOL
//#define INDIGO_TRACE_PROTOCOL(c) c
//
//#undef INDIGO_DEBUG_PROTOCOL
//#define INDIGO_DEBUG_PROTOCOL(c) c

#define PROPERTY_SIZE sizeof(indigo_property)+INDIGO_MAX_ITEMS*(sizeof(indigo_item))

static long ws_read(int handle, char *buffer, long length) {
	uint8_t header[14];
	if (indigo_read(handle, (char *)header, 6) <= 0)
		return -1;
	INDIGO_TRACE_PROTOCOL(indigo_trace("ws_read -> %2x", header[0]));
	uint8_t *masking_key = header+2;
	uint64_t payload_length = header[1] & 0x7F;
	if (payload_length == 0x7E) {
		if (indigo_read(handle, (char *)header + 6, 2) <= 0)
			return -1;
		masking_key = header + 4;
		payload_length = ntohs(*((uint16_t *)(header+2)));
	} else if (payload_length == 0x7F) {
		if (indigo_read(handle, (char *)header + 6, 8) <= 0)
			return -1;
		masking_key = header+10;
		payload_length = ntohll(*((uint64_t *)(header+2)));
	}
	if (length < payload_length)
		return -1;
	if (indigo_read(handle, buffer, payload_length) <= 0)
		return -1;
	for (uint64_t i = 0; i < payload_length; i++) {
		buffer[i] ^= masking_key[i%4];
	}
	return payload_length;
}

typedef enum {
	ERROR,
	IDLE,
	BEGIN_STRUCT,
	NAME,
	NAME1,
	NAME2,
	TEXT_VALUE,
	NUMBER_VALUE,
	LOGICAL_VALUE,
	VALUE1,
	VALUE2,
	END_STRUCT,
	BEGIN_ARRAY,
	END_ARRAY
} parser_state;


static char *parser_state_name[] = {
	"ERROR",
	"IDLE",
	"BEGIN_STRUCT",
	"NAME",
	"NAME1",
	"NAME2",
	"TEXT_VALUE",
	"NUMBER_VALUE",
	"LOGICAL_VALUE",
	"VALUE1",
	"VALUE2",
	"END_STRUCT",
	"BEGIN_ARRAY",
	"END_ARRAY"
};

typedef void *(* parser_handler)(parser_state state, char *name, char *value, indigo_property *property, indigo_device *device, indigo_client *client, char *message);

static void *top_level_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_device *device, indigo_client *client, char *message);
static void *new_text_vector_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_device *device, indigo_client *client, char *message);
static void *new_number_vector_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_device *device, indigo_client *client, char *message);
static void *new_switch_vector_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_device *device, indigo_client *client, char *message);

static void *get_properties_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_device *device, indigo_client *client, char *message) {
	INDIGO_TRACE_PROTOCOL(indigo_trace("JSON Parser: %s %s '%s' '%s'", __FUNCTION__, parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == NUMBER_VALUE && !strcmp(name, "version")) {
		client->version = (int)atol(value);
	} else if (state == END_STRUCT) {
		indigo_enumerate_properties(client, property);
		return top_level_handler;
	}
	return get_properties_handler;
}

static void *one_text_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_device *device, indigo_client *client, char *message) {
	INDIGO_TRACE_PROTOCOL(indigo_trace("JSON Parser: %s %s '%s' '%s'", __FUNCTION__, parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == END_ARRAY)
		return new_text_vector_handler;
	if (state == END_STRUCT) {
		if (property->count < INDIGO_MAX_ITEMS)
			property->count++;
	} else if (state == TEXT_VALUE && !strcmp(name, "name")) {
		strncpy(property->items[property->count].name, value, INDIGO_NAME_SIZE);
	} else if (state == TEXT_VALUE && !strcmp(name, "value")) {
		strncpy(property->items[property->count].text.value, value, INDIGO_VALUE_SIZE);
	}
	return one_text_handler;
}

static void *new_text_vector_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_device *device, indigo_client *client, char *message) {
	INDIGO_TRACE_PROTOCOL(indigo_trace("JSON Parser: %s %s '%s' '%s'", __FUNCTION__, parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == BEGIN_ARRAY && !strcmp(name, "items")) {
		property->count = 0;
		return one_text_handler;
	}
	if (state == TEXT_VALUE) {
		if (!strcmp(name, "device")) {
			strncpy(property->device, value, INDIGO_NAME_SIZE);
		} else if (!strcmp(name, "name")) {
			strncpy(property->name, value, INDIGO_NAME_SIZE);
		}
	} else if (state == END_STRUCT) {
		indigo_change_property(client, property);
		return top_level_handler;
	}
	return new_text_vector_handler;
}

static void *one_number_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_device *device, indigo_client *client, char *message) {
	INDIGO_TRACE_PROTOCOL(indigo_trace("JSON Parser: %s %s '%s' '%s'", __FUNCTION__, parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == END_ARRAY)
		return new_number_vector_handler;
	if (state == END_STRUCT) {
		if (property->count < INDIGO_MAX_ITEMS)
			property->count++;
	} else if (state == TEXT_VALUE && !strcmp(name, "name")) {
		strncpy(property->items[property->count].name, value, INDIGO_NAME_SIZE);
	} else if (state == NUMBER_VALUE && !strcmp(name, "value")) {
		property->items[property->count].number.value = atof(value);
	}
	return one_number_handler;
}

static void *new_number_vector_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_device *device, indigo_client *client, char *message) {
	INDIGO_TRACE_PROTOCOL(indigo_trace("JSON Parser: %s %s '%s' '%s'", __FUNCTION__, parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == BEGIN_ARRAY && !strcmp(name, "items")) {
		property->count = 0;
		return one_number_handler;
	}
	if (state == TEXT_VALUE) {
		if (!strcmp(name, "device")) {
			strncpy(property->device, value, INDIGO_NAME_SIZE);
		} else if (!strcmp(name, "name")) {
			strncpy(property->name, value, INDIGO_NAME_SIZE);
		}
	} else if (state == END_STRUCT) {
		indigo_change_property(client, property);
		return top_level_handler;
	}
	return new_number_vector_handler;
}

static void *one_switch_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_device *device, indigo_client *client, char *message) {
	INDIGO_TRACE_PROTOCOL(indigo_trace("JSON Parser: %s %s '%s' '%s'", __FUNCTION__, parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == END_ARRAY)
		return new_switch_vector_handler;
	if (state == END_STRUCT) {
		if (property->count < INDIGO_MAX_ITEMS)
			property->count++;
	} else if (state == TEXT_VALUE && !strcmp(name, "name")) {
		strncpy(property->items[property->count].name, value, INDIGO_NAME_SIZE);
	} else if (state == LOGICAL_VALUE && !strcmp(name, "value")) {
		property->items[property->count].sw.value = strcmp(value, "true") == 0;
	}
	return one_switch_handler;
}

static void *new_switch_vector_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_device *device, indigo_client *client, char *message) {
	INDIGO_TRACE_PROTOCOL(indigo_trace("JSON Parser: %s %s '%s' '%s'", __FUNCTION__, parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == BEGIN_ARRAY && !strcmp(name, "items")) {
		property->count = 0;
		return one_switch_handler;
	}
	if (state == TEXT_VALUE) {
		if (!strcmp(name, "device")) {
			strncpy(property->device, value, INDIGO_NAME_SIZE);
		} else if (!strcmp(name, "name")) {
			strncpy(property->name, value, INDIGO_NAME_SIZE);
		}
	} else if (state == END_STRUCT) {
		indigo_change_property(client, property);
		return top_level_handler;
	}
	return new_switch_vector_handler;
}

static void *top_level_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_device *device, indigo_client *client, char *message) {
	INDIGO_TRACE_PROTOCOL(indigo_trace("JSON Parser: %s %s '%s' '%s'", __FUNCTION__, parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == BEGIN_STRUCT) {
		memset(property, 0, PROPERTY_SIZE);
		if (name != NULL) {
			if (!strcmp(name, "getProperties"))
				return get_properties_handler;
			if (!strcmp(name, "newTextVector")) {
				property->type = INDIGO_TEXT_VECTOR;
				property->version = client->version;
				return new_text_vector_handler;
			}
			if (!strcmp(name, "newNumberVector")) {
				property->type = INDIGO_NUMBER_VECTOR;
				property->version = client->version;
				return new_number_vector_handler;
			}
			if (!strcmp(name, "newSwitchVector")) {
				property->type = INDIGO_SWITCH_VECTOR;
				property->version = client->version;
				return new_switch_vector_handler;
			}
		}
	}
	return top_level_handler;
}

void indigo_json_parse(indigo_device *device, indigo_client *client) {
	indigo_adapter_context *context = (indigo_adapter_context*)client->client_context;
	int handle = context->input;
	char buffer[JSON_BUFFER_SIZE];
	char *pointer = buffer;
	char *buffer_end = NULL;
	char property_buffer[PROPERTY_SIZE];
	char message[INDIGO_VALUE_SIZE];
	char name_buffer[INDIGO_NAME_SIZE];
	char *name_pointer = name_buffer;
	char value_buffer[INDIGO_VALUE_SIZE];
	char *value_pointer = value_buffer;
	*pointer = 0;
	char c = 0;
	char q = '"';
	int depth = 0;
	parser_handler handler = top_level_handler;
	parser_state state = IDLE;
	indigo_property *property = (indigo_property *)property_buffer;
	memset(property_buffer, 0, PROPERTY_SIZE);

	while (true) {
		assert(pointer - buffer <= JSON_BUFFER_SIZE);
		assert(name_pointer - name_buffer <= INDIGO_NAME_SIZE);
		if (state == ERROR) {
			indigo_error("JSON Parser: syntax error");
			goto exit_loop;
		}
		while ((c = *pointer++) == 0) {
			ssize_t count = (int)context->web_socket ? ws_read(handle, buffer, JSON_BUFFER_SIZE) : indigo_read_line(handle, buffer, JSON_BUFFER_SIZE);
			if (count <= 0) {
				goto exit_loop;
			}
			pointer = buffer;
			buffer_end = buffer + count;
			buffer[count] = 0;
			INDIGO_TRACE_PROTOCOL(indigo_trace("received: %s", buffer));
		}
		switch (state) {
			case ERROR:
				goto exit_loop;
			case IDLE:
				if (isspace(c)) {
				} else if (c == '{') {
					name_pointer = name_buffer;
					*name_pointer = 0;
					value_pointer = value_buffer;
					*value_pointer = 0;
					state = BEGIN_STRUCT;
					INDIGO_TRACE_PROTOCOL(indigo_trace("JSON Parser: '%c' IDLE -> BEGIN_STRUCT", c));
					depth++;
					handler = handler(BEGIN_STRUCT, NULL, NULL, property, device, client, message);
				}
				break;
			case BEGIN_STRUCT:
				if (isspace(c)) {
				} else if (c == '"' || c == '\'') {
					q = c;
					state = NAME;
					name_pointer = name_buffer;
					INDIGO_TRACE_PROTOCOL(indigo_trace("JSON Parser: '%c' BEGIN_STRUCT -> NAME", c));
				} else {
					state = ERROR;
					INDIGO_TRACE_PROTOCOL(indigo_trace("JSON Parser: '%c' BEGIN_STRUCT -> ERROR", c));
				}
				break;
			case BEGIN_ARRAY:
				if (isspace(c)) {
				} else if (c == '{') {
					state = BEGIN_STRUCT;
					INDIGO_TRACE_PROTOCOL(indigo_trace("JSON Parser: '%c' BEGIN_ARRAY -> BEGIN_STRUCT", c));
					depth++;
					handler = handler(BEGIN_STRUCT, NULL, NULL, property, device, client, message);
				}
				break;
			case NAME:
				if (c == q) {
					state = NAME1;
					*name_pointer = 0;
					INDIGO_TRACE_PROTOCOL(indigo_trace("JSON Parser: '%c' NAME -> NAME1", c));
				} else if (name_pointer - name_buffer <INDIGO_NAME_SIZE) {
					*name_pointer++ = c;
				} else {
					state = ERROR;
					INDIGO_TRACE_PROTOCOL(indigo_trace("JSON Parser: '%c' NAME -> ERROR", c));
				}
				break;
			case NAME1:
				if (isspace(c)) {
				} else if (c == ':') {
					state = NAME2;
				}
				break;
			case NAME2:
				if (isspace(c)) {
				} else if (c == '{') {
					state = BEGIN_STRUCT;
					INDIGO_TRACE_PROTOCOL(indigo_trace("JSON Parser: '%c' NAME2 -> BEGIN_STRUCT", c));
					handler = handler(BEGIN_STRUCT, name_buffer, NULL, property, device, client, message);
					depth++;
				} else if (c == '[') {
					state = BEGIN_ARRAY;
					INDIGO_TRACE_PROTOCOL(indigo_trace("JSON Parser: '%c' NAME2 -> BEGIN_ARRAY", c));
					handler = handler(BEGIN_ARRAY, name_buffer, NULL, property, device, client, message);
				} else if (c == '"' || c == '\'') {
					q = c;
					state = TEXT_VALUE;
					value_pointer = value_buffer;
					INDIGO_TRACE_PROTOCOL(indigo_trace("JSON Parser: '%c' NAME2 -> TEXT_VALUE", c));
				} else if (isdigit(c) || c == '-') {
					state = NUMBER_VALUE;
					value_pointer = value_buffer;
					*value_pointer++ = c;
					INDIGO_TRACE_PROTOCOL(indigo_trace("JSON Parser: '%c' NAME2 -> NUMBER_VALUE", c));
				} else if (isalpha(c)) {
					state = LOGICAL_VALUE;
					value_pointer = value_buffer;
					*value_pointer++ = c;
					INDIGO_TRACE_PROTOCOL(indigo_trace("JSON Parser: '%c' NAME2 -> LOGICAL_VALUE", c));
				}
				break;
			case TEXT_VALUE:
				if (c == q) {
					state = VALUE1;
					pointer--;
					*value_pointer = 0;
					handler = handler(TEXT_VALUE, name_buffer, value_buffer, property, device, client, message);
					INDIGO_TRACE_PROTOCOL(indigo_trace("JSON Parser: '%c' TEXT_VALUE -> VALUE1", c));
				} else if (value_pointer - value_buffer <INDIGO_VALUE_SIZE) {
					*value_pointer++ = c;
				} else {
					state = ERROR;
					INDIGO_TRACE_PROTOCOL(indigo_trace("JSON Parser: '%c' TEXT_VALUE -> ERROR", c));
				}
				break;
			case NUMBER_VALUE:
				if ((isdigit(c) || c == '.') && value_pointer - value_buffer <INDIGO_VALUE_SIZE) {
					*value_pointer++ = c;
				} else {
					state = VALUE1;
					pointer--;
					*value_pointer = 0;
					handler = handler(NUMBER_VALUE, name_buffer, value_buffer, property, device, client, message);
					INDIGO_TRACE_PROTOCOL(indigo_trace("JSON Parser: '%c' NUMBER_VALUE -> VALUE1", c));
				}
				break;
			case LOGICAL_VALUE:
				if ((isalpha(c)) && value_pointer - value_buffer <INDIGO_VALUE_SIZE) {
					*value_pointer++ = c;
				} else {
					*value_pointer = 0;
					if (!strcmp(value_buffer, "true") || !strcmp(value_buffer, "false")) {
						handler = handler(LOGICAL_VALUE, name_buffer, value_buffer, property, device, client, message);
						state = VALUE1;
						pointer--;
						INDIGO_TRACE_PROTOCOL(indigo_trace("JSON Parser: '%c' LOGICAL_VALUE -> VALUE1", c));
					} else {
						state = ERROR;
						INDIGO_TRACE_PROTOCOL(indigo_trace("JSON Parser: '%c' LOGICAL_VALUE -> ERROR", c));
					}
				}
				break;
			case VALUE1:
				if (isspace(c)) {
				} else if (c == ',') {
					state = VALUE2;
					INDIGO_TRACE_PROTOCOL(indigo_trace("JSON Parser: '%c' VALUE1 -> VALUE2", c));
				} else if (c == '}') {
					handler = handler(END_STRUCT, NULL, NULL, property, device, client, message);
					depth--;
					if (depth == 0) {
						state = IDLE;
						INDIGO_TRACE_PROTOCOL(indigo_trace("JSON Parser: '%c' VALUE2 -> IDLE", c));
					}
				} else if (c == ']') {
					handler = handler(END_ARRAY, NULL, NULL, property, device, client, message);
					depth--;
					if (depth == 0) {
						state = IDLE;
						INDIGO_TRACE_PROTOCOL(indigo_trace("JSON Parser: '%c' VALUE2 -> IDLE", c));
					}
				}
				break;
			case VALUE2:
				if (isspace(c)) {
				} else if (c == '"' || c == '\'') {
					q = c;
					state = NAME;
					name_pointer = name_buffer;
					INDIGO_TRACE_PROTOCOL(indigo_trace("JSON Parser: '%c' VALUE2 -> NAME", c));
				} else if (c == '{') {
					state = BEGIN_STRUCT;
					INDIGO_TRACE_PROTOCOL(indigo_trace("JSON Parser: '%c' NAME2 -> BEGIN_STRUCT", c));
					handler = handler(BEGIN_STRUCT, NULL, NULL, property, device, client, message);
					depth++;
				} else {
					state = ERROR;
					INDIGO_TRACE_PROTOCOL(indigo_trace("JSON Parser: '%c' VALUE2 -> ERROR", c));
				}
				break;
			default:
				assert(false);
				break;
		}
	}
exit_loop:
	close(handle);
	indigo_log("JSON Parser: parser finished");
}
