// Copyright (c) 2016-2025 CloudMakers, s. r. o.
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
#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <assert.h>
#include <stdint.h>

#include <indigo/indigo_json.h>
#include <indigo/indigo_io.h>

#if defined(INDIGO_LINUX)
#include <arpa/inet.h>
#endif

#ifndef ntohll
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define ntohll(x) (((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))
#else
#define ntohll(x) (x)
#endif
#endif

#ifndef htonll
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define htonll(x) (((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
#else
#define htonll(x) (x)
#endif
#endif

static long ws_read(indigo_uni_handle* handle, char* buffer, long length) {
	uint8_t header[14];
	long bytes_read = indigo_uni_read(handle, (char*)header, 6);
	if (bytes_read <= 0) {
		return bytes_read;
	}
	INDIGO_TRACE_PARSER(indigo_trace("ws_read -> %2x", header[0]));
	uint8_t* masking_key = header + 2;
	uint64_t payload_length = header[1] & 0x7F;
	if (payload_length == 0x7E) {
		bytes_read = indigo_uni_read(handle, (char*)header + 6, 2);
		if (bytes_read <= 0) {
			return bytes_read;
		}
		masking_key = header + 4;
		payload_length = ntohs(*((uint16_t*)(header + 2)));
	} else if (payload_length == 0x7F) {
		bytes_read = indigo_uni_read(handle, (char*)header + 6, 8);
		if (bytes_read <= 0) {
			return bytes_read;
		}
		masking_key = header + 10;
		payload_length = ntohll(*((uint64_t*)(header + 2)));
	}
	if (length < payload_length) {
		errno = ENODATA;
		return -1;
	}
	bytes_read = indigo_uni_read(handle, buffer, (long)payload_length);
	if (bytes_read <= 0) {
		return bytes_read;
	}
	for (uint64_t i = 0; i < payload_length; i++) {
		buffer[i] ^= masking_key[i % 4];
	}
	return (long)payload_length;
}

typedef enum {
	ERROR_STATE,
	IDLE_STATE,
	BEGIN_STRUCT_STATE,
	NAME_STATE,
	NAME1_STATE,
	NAME2_STATE,
	TEXT_VALUE_STATE,
	NUMBER_VALUE_STATE,
	LOGICAL_VALUE_STATE,
	VALUE1_STATE,
	VALUE2_STATE,
	END_STRUCT_STATE,
	BEGIN_ARRAY_STATE,
	END_ARRAY_STATE
} parser_state;


#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#else
#pragma warning(push)
#pragma warning(disable: 4101)
#endif

static char* parser_state_name[] = {
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

#ifdef __clang__
#pragma clang diagnostic pop
#else
#pragma warning(pop)
#endif

typedef void* (*parser_handler)(parser_state state, char* name, char* value, indigo_property** property_ref, indigo_device* device, indigo_client* client, char* message);

static void* top_level_handler(parser_state state, char* name, char* value, indigo_property** property_ref, indigo_device* device, indigo_client* client, char* message);
static void* new_text_vector_handler(parser_state state, char* name, char* value, indigo_property** property_ref, indigo_device* device, indigo_client* client, char* message);
static void* new_number_vector_handler(parser_state state, char* name, char* value, indigo_property** property_ref, indigo_device* device, indigo_client* client, char* message);
static void* new_switch_vector_handler(parser_state state, char* name, char* value, indigo_property** property_ref, indigo_device* device, indigo_client* client, char* message);

static void* get_properties_handler(parser_state state, char* name, char* value, indigo_property** property_ref, indigo_device* device, indigo_client* client, char* message) {
	INDIGO_TRACE_PARSER(indigo_trace("JSON Parser: %s %s '%s' '%s'", __FUNCTION__, parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	indigo_property* property = *property_ref;
	if (state == NUMBER_VALUE_STATE && !strcmp(name, "version")) {
		client->version = (int)atol(value);
	} else if (state == TEXT_VALUE_STATE && !strcmp(name, "client")) {
		INDIGO_COPY_NAME(client->name, value);
	} else if (state == TEXT_VALUE_STATE && !strcmp(name, "device")) {
		INDIGO_COPY_NAME(property->device, value);
	} else if (state == TEXT_VALUE_STATE && !strcmp(name, "name")) {
		INDIGO_COPY_NAME(property->name, value);
	} else if (state == END_STRUCT_STATE) {
		indigo_enumerate_properties(client, property);
		return top_level_handler;
	}
	return get_properties_handler;
}

static void* one_text_handler(parser_state state, char* name, char* value, indigo_property** property_ref, indigo_device* device, indigo_client* client, char* message) {
	INDIGO_TRACE_PARSER(indigo_trace("JSON Parser: %s %s '%s' '%s'", __FUNCTION__, parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	indigo_property* property = *property_ref;
	if (state == END_ARRAY_STATE) {
		return new_text_vector_handler;
	}
	if (state == BEGIN_STRUCT_STATE) {
		*property_ref = property = indigo_resize_property(property, property->count + 1);
	} else if (state == TEXT_VALUE_STATE && !strcmp(name, "name")) {
		INDIGO_COPY_NAME(property->items[property->count - 1].name, value);
	} else if (state == TEXT_VALUE_STATE && !strcmp(name, "value")) {
		indigo_set_text_item_value(property->items + property->count - 1, value);
	}
	return one_text_handler;
}

static void* new_text_vector_handler(parser_state state, char* name, char* value, indigo_property** property_ref, indigo_device* device, indigo_client* client, char* message) {
	INDIGO_TRACE_PARSER(indigo_trace("JSON Parser: %s %s '%s' '%s'", __FUNCTION__, parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	indigo_property* property = *property_ref;
	if (state == BEGIN_ARRAY_STATE && !strcmp(name, "items")) {
		*property_ref = property = indigo_resize_property(property, 0);
		return one_text_handler;
	}
	if (state == TEXT_VALUE_STATE) {
		if (!strcmp(name, "device")) {
			INDIGO_COPY_NAME(property->device, value);
		} else if (!strcmp(name, "name")) {
			INDIGO_COPY_NAME(property->name, value);
		} else if (!strcmp(name, "token")) {
			property->access_token = strtol(value, NULL, 16);
		}
	} else if (state == END_STRUCT_STATE) {
		indigo_change_property(client, property);
		indigo_clear_property(property);
		return top_level_handler;
	}
	return new_text_vector_handler;
}

static void* one_number_handler(parser_state state, char* name, char* value, indigo_property** property_ref, indigo_device* device, indigo_client* client, char* message) {
	INDIGO_TRACE_PARSER(indigo_trace("JSON Parser: %s %s '%s' '%s'", __FUNCTION__, parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	indigo_property* property = *property_ref;
	if (state == END_ARRAY_STATE) {
		return new_number_vector_handler;
	}
	if (state == BEGIN_STRUCT_STATE) {
		*property_ref = property = indigo_resize_property(property, property->count + 1);
	} else if (state == TEXT_VALUE_STATE && !strcmp(name, "name")) {
		INDIGO_COPY_NAME(property->items[property->count - 1].name, value);
	} else if (state == NUMBER_VALUE_STATE && !strcmp(name, "value")) {
		property->items[property->count - 1].number.value = indigo_atod(value);
	}
	return one_number_handler;
}

static void* new_number_vector_handler(parser_state state, char* name, char* value, indigo_property** property_ref, indigo_device* device, indigo_client* client, char* message) {
	INDIGO_TRACE_PARSER(indigo_trace("JSON Parser: %s %s '%s' '%s'", __FUNCTION__, parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	indigo_property* property = *property_ref;
	if (state == BEGIN_ARRAY_STATE && !strcmp(name, "items")) {
		*property_ref = property = indigo_resize_property(property, 0);
		return one_number_handler;
	}
	if (state == TEXT_VALUE_STATE) {
		if (!strcmp(name, "device")) {
			INDIGO_COPY_NAME(property->device, value);
		} else if (!strcmp(name, "name")) {
			INDIGO_COPY_NAME(property->name, value);
		} else if (!strcmp(name, "token")) {
			property->access_token = strtol(value, NULL, 16);
		}
	} else if (state == END_STRUCT_STATE) {
		indigo_change_property(client, property);
		indigo_clear_property(property);
		return top_level_handler;
	}
	return new_number_vector_handler;
}

static void* one_switch_handler(parser_state state, char* name, char* value, indigo_property** property_ref, indigo_device* device, indigo_client* client, char* message) {
	INDIGO_TRACE_PARSER(indigo_trace("JSON Parser: %s %s '%s' '%s'", __FUNCTION__, parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	indigo_property* property = *property_ref;
	if (state == END_ARRAY_STATE) {
		return new_switch_vector_handler;
	}
	if (state == BEGIN_STRUCT_STATE) {
		*property_ref = property = indigo_resize_property(property, property->count + 1);
	} else if (state == TEXT_VALUE_STATE && !strcmp(name, "name")) {
		INDIGO_COPY_NAME(property->items[property->count - 1].name, value);
	} else if (state == LOGICAL_VALUE_STATE && !strcmp(name, "value")) {
		property->items[property->count - 1].sw.value = strcmp(value, "true") == 0;
	}
	return one_switch_handler;
}

static void* new_switch_vector_handler(parser_state state, char* name, char* value, indigo_property** property_ref, indigo_device* device, indigo_client* client, char* message) {
	INDIGO_TRACE_PARSER(indigo_trace("JSON Parser: %s %s '%s' '%s'", __FUNCTION__, parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	indigo_property* property = *property_ref;
	if (state == BEGIN_ARRAY_STATE && !strcmp(name, "items")) {
		*property_ref = property = indigo_resize_property(property, 0);
		return one_switch_handler;
	}
	if (state == TEXT_VALUE_STATE) {
		if (!strcmp(name, "device")) {
			INDIGO_COPY_NAME(property->device, value);
		} else if (!strcmp(name, "name")) {
			INDIGO_COPY_NAME(property->name, value);
		} else if (!strcmp(name, "token")) {
			property->access_token = strtol(value, NULL, 16);
		}
	} else if (state == END_STRUCT_STATE) {
		indigo_change_property(client, property);
		indigo_clear_property(property);
		return top_level_handler;
	}
	return new_switch_vector_handler;
}

static void* top_level_handler(parser_state state, char* name, char* value, indigo_property** property_ref, indigo_device* device, indigo_client* client, char* message) {
	INDIGO_TRACE_PARSER(indigo_trace("JSON Parser: %s %s '%s' '%s'", __FUNCTION__, parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	indigo_property* property = *property_ref;
	if (state == BEGIN_STRUCT_STATE) {
		indigo_clear_property(property);
		if (name != NULL) {
			if (!strcmp(name, "getProperties")) {
				return get_properties_handler;
			}
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

void indigo_json_parse(indigo_device* device, indigo_client* client) {
	indigo_adapter_context* context = (indigo_adapter_context*)client->client_context;
	indigo_uni_handle** handle = context->input;
	char* buffer = indigo_safe_malloc(JSON_BUFFER_SIZE);
	char* value_buffer = indigo_safe_malloc(JSON_BUFFER_SIZE);
	char* name_buffer = indigo_safe_malloc(INDIGO_NAME_SIZE);
	indigo_property* property = indigo_safe_malloc(sizeof(indigo_property) + INDIGO_PREALLOCATED_COUNT * sizeof(indigo_item));
	property->allocated_count = INDIGO_PREALLOCATED_COUNT;
	char* pointer = buffer;
	char* value_pointer = value_buffer;
	char* name_pointer = name_buffer;
	*pointer = 0;
	char c = 0;
	char q = '"';
	int depth = 0;
	bool is_escaped = false;
	parser_handler handler = top_level_handler;
	parser_state state = IDLE_STATE;
	while (true) {
		assert(pointer - buffer <= JSON_BUFFER_SIZE);
		assert(name_pointer - name_buffer <= INDIGO_NAME_SIZE);
		if (state == ERROR_STATE) {
			indigo_error("JSON Parser: syntax error");
			goto exit_loop;
		}
		while ((c = *pointer++) == 0) {
			long count = context->web_socket ? ws_read(*handle, buffer, JSON_BUFFER_SIZE) : indigo_uni_read_line(*handle, buffer, JSON_BUFFER_SIZE);
			if (count <= 0) {
				goto exit_loop;
			}
			pointer = buffer;
			buffer[count] = 0;
		}
		switch (state) {
		case ERROR_STATE:
			goto exit_loop;
		case IDLE_STATE:
			if (isspace(c)) {
			} else if (c == '{') {
				name_pointer = name_buffer;
				*name_pointer = 0;
				value_pointer = value_buffer;
				*value_pointer = 0;
				state = BEGIN_STRUCT_STATE;
				INDIGO_TRACE_PARSER(indigo_trace("JSON Parser: '%c' IDLE -> BEGIN_STRUCT", c));
				depth++;
				handler = handler(BEGIN_STRUCT_STATE, NULL, NULL, &property, device, client, NULL);
			}
			break;
		case BEGIN_STRUCT_STATE:
			if (isspace(c)) {
			} else if (c == '"' || c == '\'') {
				q = c;
				state = NAME_STATE;
				name_pointer = name_buffer;
				INDIGO_TRACE_PARSER(indigo_trace("JSON Parser: '%c' BEGIN_STRUCT -> NAME", c));
			} else {
				state = ERROR_STATE;
				INDIGO_TRACE_PARSER(indigo_trace("JSON Parser: '%c' BEGIN_STRUCT -> ERROR", c));
			}
			break;
		case BEGIN_ARRAY_STATE:
			if (isspace(c)) {
			} else if (c == '{') {
				state = BEGIN_STRUCT_STATE;
				INDIGO_TRACE_PARSER(indigo_trace("JSON Parser: '%c' BEGIN_ARRAY -> BEGIN_STRUCT", c));
				depth++;
				handler = handler(BEGIN_STRUCT_STATE, NULL, NULL, &property, device, client, NULL);
			}
			break;
		case NAME_STATE:
			if (c == q) {
				state = NAME1_STATE;
				*name_pointer = 0;
				INDIGO_TRACE_PARSER(indigo_trace("JSON Parser: '%c' NAME -> NAME1", c));
			} else if (name_pointer - name_buffer < INDIGO_NAME_SIZE) {
				*name_pointer++ = c;
			} else {
				state = ERROR_STATE;
				INDIGO_TRACE_PARSER(indigo_trace("JSON Parser: '%c' NAME -> ERROR", c));
			}
			break;
		case NAME1_STATE:
			if (isspace(c)) {
			} else if (c == ':') {
				state = NAME2_STATE;
			}
			break;
		case NAME2_STATE:
			if (isspace(c)) {
			} else if (c == '{') {
				state = BEGIN_STRUCT_STATE;
				INDIGO_TRACE_PARSER(indigo_trace("JSON Parser: '%c' NAME2 -> BEGIN_STRUCT", c));
				handler = handler(BEGIN_STRUCT_STATE, name_buffer, NULL, &property, device, client, NULL);
				depth++;
			} else if (c == '[') {
				state = BEGIN_ARRAY_STATE;
				INDIGO_TRACE_PARSER(indigo_trace("JSON Parser: '%c' NAME2 -> BEGIN_ARRAY", c));
				handler = handler(BEGIN_ARRAY_STATE, name_buffer, NULL, &property, device, client, NULL);
			} else if (c == '"' || c == '\'') {
				q = c;
				state = TEXT_VALUE_STATE;
				value_pointer = value_buffer;
				INDIGO_TRACE_PARSER(indigo_trace("JSON Parser: '%c' NAME2 -> TEXT_VALUE", c));
			} else if (isdigit(c) || c == '-') {
				state = NUMBER_VALUE_STATE;
				value_pointer = value_buffer;
				*value_pointer++ = c;
				INDIGO_TRACE_PARSER(indigo_trace("JSON Parser: '%c' NAME2 -> NUMBER_VALUE", c));
			} else if (isalpha(c)) {
				state = LOGICAL_VALUE_STATE;
				value_pointer = value_buffer;
				*value_pointer++ = c;
				INDIGO_TRACE_PARSER(indigo_trace("JSON Parser: '%c' NAME2 -> LOGICAL_VALUE", c));
			}
			break;
		case TEXT_VALUE_STATE:
			if (c == q && !is_escaped) {
				state = VALUE1_STATE;
				pointer--;
				*value_pointer = 0;
				handler = handler(TEXT_VALUE_STATE, name_buffer, value_buffer, &property, device, client, NULL);
				INDIGO_TRACE_PARSER(indigo_trace("JSON Parser: '%c' TEXT_VALUE -> VALUE1", c));
			} else if (c == '\\') {
				is_escaped = true;
			} else if (value_pointer - value_buffer < JSON_BUFFER_SIZE) {
				if (is_escaped) {
					if (c == 'n') {
						c = '\n';
					} else if (c == 'r')
						c = '\r';
					else if (c == 't')
						c = '\t';
				}
				*value_pointer++ = c;
				is_escaped = false;
			} else {
				state = ERROR_STATE;
				INDIGO_TRACE_PARSER(indigo_trace("JSON Parser: '%c' TEXT_VALUE -> ERROR", c));
			}
			break;
		case NUMBER_VALUE_STATE:
			if ((isdigit(c) || c == '.' || c == 'e' || c == 'E' || c == '-') && value_pointer - value_buffer < JSON_BUFFER_SIZE) {
				*value_pointer++ = c;
			} else {
				state = VALUE1_STATE;
				pointer--;
				*value_pointer = 0;
				handler = handler(NUMBER_VALUE_STATE, name_buffer, value_buffer, &property, device, client, NULL);
				INDIGO_TRACE_PARSER(indigo_trace("JSON Parser: '%c' NUMBER_VALUE -> VALUE1", c));
			}
			break;
		case LOGICAL_VALUE_STATE:
			if ((isalpha(c)) && value_pointer - value_buffer < JSON_BUFFER_SIZE) {
				*value_pointer++ = c;
			} else {
				*value_pointer = 0;
				if (!strcmp(value_buffer, "true") || !strcmp(value_buffer, "false")) {
					handler = handler(LOGICAL_VALUE_STATE, name_buffer, value_buffer, &property, device, client, NULL);
					state = VALUE1_STATE;
					pointer--;
					INDIGO_TRACE_PARSER(indigo_trace("JSON Parser: '%c' LOGICAL_VALUE -> VALUE1", c));
				} else {
					state = ERROR_STATE;
					INDIGO_TRACE_PARSER(indigo_trace("JSON Parser: '%c' LOGICAL_VALUE -> ERROR", c));
				}
			}
			break;
		case VALUE1_STATE:
			if (isspace(c)) {
			} else if (c == ',') {
				state = VALUE2_STATE;
				INDIGO_TRACE_PARSER(indigo_trace("JSON Parser: '%c' VALUE1 -> VALUE2", c));
			} else if (c == '}') {
				handler = handler(END_STRUCT_STATE, NULL, NULL, &property, device, client, NULL);
				depth--;
				if (depth == 0) {
					state = IDLE_STATE;
					INDIGO_TRACE_PARSER(indigo_trace("JSON Parser: '%c' VALUE2 -> IDLE", c));
				}
			} else if (c == ']') {
				handler = handler(END_ARRAY_STATE, NULL, NULL, &property, device, client, NULL);
				depth--;
				if (depth == 0) {
					state = IDLE_STATE;
					INDIGO_TRACE_PARSER(indigo_trace("JSON Parser: '%c' VALUE2 -> IDLE", c));
				}
			}
			break;
		case VALUE2_STATE:
			if (isspace(c)) {
			} else if (c == '"' || c == '\'') {
				q = c;
				state = NAME_STATE;
				name_pointer = name_buffer;
				INDIGO_TRACE_PARSER(indigo_trace("JSON Parser: '%c' VALUE2 -> NAME", c));
			} else if (c == '{') {
				state = BEGIN_STRUCT_STATE;
				INDIGO_TRACE_PARSER(indigo_trace("JSON Parser: '%c' NAME2 -> BEGIN_STRUCT", c));
				handler = handler(BEGIN_STRUCT_STATE, NULL, NULL, &property, device, client, NULL);
				depth++;
			} else {
				state = ERROR_STATE;
				INDIGO_TRACE_PARSER(indigo_trace("JSON Parser: '%c' VALUE2 -> ERROR", c));
			}
			break;
		default:
			assert(false);
			break;
		}
	}
exit_loop:
	indigo_safe_free(buffer);
	indigo_safe_free(value_buffer);
	indigo_safe_free(name_buffer);
	indigo_release_property(property);
	indigo_log("JSON Parser: parser finished");
}

#define BUFFER_COUNT	10
static char* escape_buffer[BUFFER_COUNT] = { NULL };
static long escape_buffer_size[BUFFER_COUNT] = { 0 };
static bool free_escape_buffers_registered = false;

static void free_escape_buffers(void) {
	for (int i = 0; i < BUFFER_COUNT; i++) {
		indigo_safe_free(escape_buffer[i]);
	}
}

const char* indigo_json_escape(const char* string) {
	if (strpbrk(string, "\"\n\t\t")) {
		if (!free_escape_buffers_registered) {
			atexit(free_escape_buffers);
			free_escape_buffers_registered = true;
		}
		long length = 5 * (long)strlen(string);
		static int	buffer_index = 0;
		int index = buffer_index = (buffer_index + 1) % BUFFER_COUNT;
		char* buffer;
		if (escape_buffer[index] == NULL) {
			escape_buffer[index] = buffer = indigo_safe_malloc(escape_buffer_size[index] = length);
		} else if (escape_buffer_size[index] < length) {
			escape_buffer[index] = buffer = indigo_safe_realloc(escape_buffer[index], escape_buffer_size[index] = length);
		} else {
			buffer = escape_buffer[index];
		}
		const char* in = string;
		char* out = buffer;
		char c;
		while ((c = *in++)) {
			switch (c) {
			case '"':
				*out++ = '\\';
				*out++ = '"';
				break;
			case '\n':
				*out++ = '\\';
				*out++ = 'n';
				break;
			case '\r':
				*out++ = '\\';
				*out++ = 'r';
				break;
			case '\t':
				*out++ = '\\';
				*out++ = 't';
				break;
			default:
				*out++ = c;
			}
		}
		*out = 0;
		return buffer;
	}
	return string;
}

static pthread_mutex_t json_mutex = PTHREAD_MUTEX_INITIALIZER;

static long ws_write(indigo_uni_handle *handle, const char *buffer, long length) {
	uint8_t header[10] = { 0x81 };
	long result;
	handle->log_level = -abs(handle->log_level);
	if (length <= 0x7D) {
		header[1] = (uint8_t)length;
		result = indigo_uni_write(handle, (char *)header, 2);
	} else if (length <= 0xFFFF) {
		header[1] = 0x7E;
		uint16_t payloadLength = htons((uint16_t)length);
		memcpy(header+2, &payloadLength, 2);
		result = indigo_uni_write(handle, (char *)header, 4);
	} else {
		header[1] = 0x7F;
		uint64_t payloadLength = htonll((uint64_t)length);
		memcpy(header+2, &payloadLength, 8);
		result = indigo_uni_write(handle, (char *)header, 10);
	}
	result = result + indigo_uni_write(handle, buffer, length);
	handle->log_level = -abs(handle->log_level);
	return result;
}

#define SPRINTF(...) { \
size = sprintf(__VA_ARGS__); \
pnt += size; \
size = (long)(pnt - output_buffer); \
if (size + 1024 > buffer_size) { \
buffer_size *= 2; \
printf("realloc to %ld\n", buffer_size); \
output_buffer = indigo_safe_realloc(output_buffer, buffer_size); \
pnt = output_buffer + size; \
} \
}

indigo_result indigo_json_device_adapter_define_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	assert(device != NULL);
	assert(client != NULL);
	assert(property != NULL);
	if (!indigo_reshare_remote_devices && device->is_remote) {
		return INDIGO_OK;
	}
	if (client->version == INDIGO_VERSION_NONE) {
		return INDIGO_OK;
	}
	indigo_adapter_context *client_context = (indigo_adapter_context *)client->client_context;
	assert(client_context != NULL);
	pthread_mutex_lock(&json_mutex);
	indigo_uni_handle **handle = client_context->output;
	if (handle == NULL || *handle == NULL) {
		pthread_mutex_unlock(&json_mutex);
		return INDIGO_OK;
	}
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
			size += (long)(pnt - output_buffer);
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
			size += (long)(pnt - output_buffer);
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
			size += (long)(pnt - output_buffer);
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
			size += (long)(pnt - output_buffer);
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
			size += (long)(pnt - output_buffer);
			break;
	}
	if ((client_context->web_socket ? ws_write(*handle, output_buffer, size) : indigo_uni_write(*handle, output_buffer, size)) < 0) {
		indigo_uni_close(client_context->input);
		indigo_uni_close(client_context->output);
	}
	free(output_buffer);
	pthread_mutex_unlock(&json_mutex);
	return INDIGO_OK;
}

indigo_result indigo_json_device_adapter_update_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	assert(device != NULL);
	assert(client != NULL);
	assert(property != NULL);
	if (!indigo_reshare_remote_devices && device->is_remote) {
		return INDIGO_OK;
	}
	if (client->version == INDIGO_VERSION_NONE) {
		return INDIGO_OK;
	}
	indigo_adapter_context *client_context = (indigo_adapter_context *)client->client_context;
	assert(client_context != NULL);
	pthread_mutex_lock(&json_mutex);
	indigo_uni_handle **handle = client_context->output;
	if (handle == NULL || *handle == NULL) {
		pthread_mutex_unlock(&json_mutex);
		return INDIGO_OK;
	}
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
			size += (long)(pnt - output_buffer);
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
			size += (long)(pnt - output_buffer);
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
			size += (long)(pnt - output_buffer);
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
			size += (long)(pnt - output_buffer);
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
			size += (long)(pnt - output_buffer);
			break;
	}
	if ((client_context->web_socket ? ws_write(*handle, output_buffer, size) : indigo_uni_write(*handle, output_buffer, size)) < 0) {
		indigo_uni_close(client_context->input);
		indigo_uni_close(client_context->output);
	}
	free(output_buffer);
	pthread_mutex_unlock(&json_mutex);
	return INDIGO_OK;
}

indigo_result indigo_json_device_adapter_delete_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	assert(device != NULL);
	assert(client != NULL);
	assert(property != NULL);
	if (!indigo_reshare_remote_devices && device->is_remote) {
		return INDIGO_OK;
	}
	if (client->version == INDIGO_VERSION_NONE) {
		return INDIGO_OK;
	}
	indigo_adapter_context *client_context = (indigo_adapter_context *)client->client_context;
	assert(client_context != NULL);
	pthread_mutex_lock(&json_mutex);
	indigo_uni_handle **handle = client_context->output;
	if (handle == NULL || *handle == NULL) {
		pthread_mutex_unlock(&json_mutex);
		return INDIGO_OK;
	}
	char *output_buffer = indigo_safe_malloc(JSON_BUFFER_SIZE);
	char *pnt = output_buffer;
	long size;
	if (*property->name == 0) {
		size = sprintf(pnt, "{ \"deleteProperty\": { \"device\": \"%s\"", device->name);
	} else {
		size = sprintf(pnt, "{ \"deleteProperty\": { \"device\": \"%s\", \"name\": \"%s\"", property->device, property->name);
	}
	pnt += size;
	if (message) {
		size = sprintf(pnt, ", \"message\": \"%s\" } }", message);
	} else {
		size = sprintf(pnt, " } }");
	}
	size += (long)(pnt - output_buffer);
	if ((client_context->web_socket ? ws_write(*handle, output_buffer, size) : indigo_uni_write(*handle, output_buffer, size)) < 0) {
		indigo_uni_close(client_context->input);
		indigo_uni_close(client_context->output);
	}
	free(output_buffer);
	pthread_mutex_unlock(&json_mutex);
	return INDIGO_OK;
}

indigo_result indigo_json_device_adapter_message_property(indigo_client *client, indigo_device *device, const char *message) {
	assert(device != NULL);
	assert(client != NULL);
	if (!indigo_reshare_remote_devices && device->is_remote) {
		return INDIGO_OK;
	}
	indigo_adapter_context *client_context = (indigo_adapter_context *)client->client_context;
	assert(client_context != NULL);
	pthread_mutex_lock(&json_mutex);
	indigo_uni_handle **handle = client_context->output;
	if (handle == NULL || *handle == NULL) {
		pthread_mutex_unlock(&json_mutex);
		return INDIGO_OK;
	}
	char *output_buffer = indigo_safe_malloc(JSON_BUFFER_SIZE);
	char *pnt = output_buffer;
	long size = sprintf(pnt, "{ \"message\": \"%s\" }", message);
	if ((client_context->web_socket ? ws_write(*handle, output_buffer, size) : indigo_uni_write(*handle, output_buffer, size)) < 0) {
		indigo_uni_close(client_context->input);
		indigo_uni_close(client_context->output);
	}
	free(output_buffer);
	pthread_mutex_unlock(&json_mutex);
	return INDIGO_OK;
}
