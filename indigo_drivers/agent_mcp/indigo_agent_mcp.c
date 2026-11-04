// Copyright (c) 2026 CloudMakers, s. r. o.
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
// 3.0 MCP protocol agent

#include <assert.h>
#include <locale.h>
#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <indigo/indigo_driver.h>
#include <indigo/indigo_server_tcp.h>
#include <indigo/indigo_timer.h>

#define JSMN_STATIC
#include <externals/jsmn/jsmn.h>

#include "indigo_agent_mcp.h"

#define DRIVER_NAME "indigo_agent_mcp"
#define DRIVER_VERSION 0x03000000
#define MCP_VERSION "2025-11-25"
#define MCP_BODY_LIMIT (64 * 1024)
#define MCP_TOKEN_LIMIT 256
#define MCP_DEPTH_LIMIT 16
#define MCP_REQUEST_LIMIT 8
#ifndef MCP_PROGRESS_TIMEOUT
#define MCP_PROGRESS_TIMEOUT 5.0
#endif
#ifndef MCP_TOTAL_TIMEOUT
#define MCP_TOTAL_TIMEOUT 15.0
#endif

static indigo_device *agent_device;
static pthread_mutex_t service_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool service_enabled;
static unsigned active_requests;
static pthread_t dispatch_threads[MCP_REQUEST_LIMIT];
static bool dispatch_active[MCP_REQUEST_LIMIT];

typedef struct {
	const char *text;
	jsmntok_t tokens[MCP_TOKEN_LIMIT];
	int count;
} json_document;

typedef struct cached_property {
	indigo_property *property;
	struct cached_property *next;
} cached_property;

static pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;
static cached_property *properties;
static bool cache_failed;
static indigo_client agent_client;

static void free_cached_property(indigo_property *property) {
	if (property) {
		if (property->type == INDIGO_TEXT_VECTOR) {
			for (int i = 0; i < property->count; i++) {
				free(property->items[i].text.long_value);
			}
		}
		free(property);
	}
}

static indigo_property *copy_cached_property(const indigo_property *property, bool from_bus, bool values) {
	if (property->count < 0 || (size_t)property->count > (SIZE_MAX - sizeof(indigo_property)) / sizeof(indigo_item)) {
		return NULL;
	}
	size_t size = sizeof(indigo_property) + (size_t)property->count * sizeof(indigo_item);
	indigo_property *copy = malloc(size);
	if (!copy) {
		return NULL;
	}
	memcpy(copy, property, size);
	copy->allocated_count = copy->count;
	copy->access_token = 0;
	if (copy->type == INDIGO_TEXT_VECTOR) {
		for (int i = 0; i < copy->count; i++) {
			copy->items[i].text.long_value = NULL;
		}
		for (int i = 0; i < copy->count; i++) {
			if (values && property->items[i].text.long_value) {
				copy->items[i].text.long_value = strdup(property->items[i].text.long_value);
				if (!copy->items[i].text.long_value) {
					free_cached_property(copy);
					return NULL;
				}
			}
		}
	} else if (copy->type == INDIGO_BLOB_VECTOR) {
		for (int i = 0; i < copy->count; i++) {
			indigo_item *item = copy->items + i;
			if (from_bus && !*item->blob.url && property->state == INDIGO_OK_STATE && (item->blob.value || indigo_proxy_blob)) {
				snprintf(item->blob.url, sizeof(item->blob.url), "/blob/%p%s", (void *)(property->items + i), item->blob.format);
			}
			item->blob.value = NULL;
		}
	}
	if (!values) {
		for (int i = 0; i < copy->count; i++) {
			indigo_item *item = copy->items + i;
			if (copy->type == INDIGO_NUMBER_VECTOR) {
				item->number.value = item->number.target = item->number.default_value = item->number.previous_value = item->number.previous_target = 0;
			} else if (copy->type == INDIGO_BLOB_VECTOR) {
				item->blob.value = NULL;
				item->blob.url[0] = 0;
				item->blob.size = 0;
			} else if (copy->type == INDIGO_TEXT_VECTOR) {
				memset(&item->text, 0, sizeof(item->text));
			} else if (copy->type == INDIGO_SWITCH_VECTOR) {
				memset(&item->sw, 0, sizeof(item->sw));
			} else if (copy->type == INDIGO_LIGHT_VECTOR) {
				memset(&item->light, 0, sizeof(item->light));
			}
		}
	}
	return copy;
}

static void clear_cache(void) {
	while (properties) {
		cached_property *entry = properties;
		properties = entry->next;
		free_cached_property(entry->property);
		free(entry);
	}
	cache_failed = false;
}

static indigo_result cache_delete(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	pthread_mutex_lock(&cache_mutex);
	for (cached_property **entry = &properties; *entry;) {
		indigo_property *cached = (*entry)->property;
		if (!strcmp(cached->device, property->device) && (!*property->name || !strcmp(cached->name, property->name))) {
			cached_property *removed = *entry;
			*entry = removed->next;
			free_cached_property(removed->property);
			free(removed);
		} else {
			entry = &(*entry)->next;
		}
	}
	pthread_mutex_unlock(&cache_mutex);
	return INDIGO_OK;
}

static indigo_result cache_define(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (property->hidden || (property->perm != INDIGO_RO_PERM && property->perm != INDIGO_RW_PERM && property->perm != INDIGO_WO_PERM)) {
		return cache_delete(client, device, property, message);
	}
	indigo_property *copy = copy_cached_property(property, true, property->perm != INDIGO_WO_PERM);
	pthread_mutex_lock(&cache_mutex);
	cached_property **entry = &properties;
	while (*entry && (strcmp((*entry)->property->device, property->device) || strcmp((*entry)->property->name, property->name))) {
		entry = &(*entry)->next;
	}
	if (!copy) {
		cache_failed = true;
	} else if (*entry) {
		free_cached_property((*entry)->property);
		(*entry)->property = copy;
	} else {
		*entry = malloc(sizeof(cached_property));
		if (*entry) {
			**entry = (cached_property){ copy, NULL };
		} else {
			free_cached_property(copy);
			cache_failed = true;
		}
	}
	pthread_mutex_unlock(&cache_mutex);
	return INDIGO_OK;
}

static indigo_result cache_update(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (property->hidden || (property->perm != INDIGO_RO_PERM && property->perm != INDIGO_RW_PERM && property->perm != INDIGO_WO_PERM)) {
		return cache_delete(client, device, property, message);
	}
	pthread_mutex_lock(&cache_mutex);
	for (cached_property *entry = properties; entry; entry = entry->next) {
		indigo_property *cached = entry->property;
		if (strcmp(cached->device, property->device) || strcmp(cached->name, property->name)) {
			continue;
		}
		if (cached->type != property->type) {
			cache_failed = true;
			break;
		}
		cached->state = property->state;
		cached->perm = property->perm;
		if (cached->perm == INDIGO_WO_PERM) {
			indigo_property *copy = copy_cached_property(cached, false, false);
			if (copy) {
				entry->property = copy;
				free_cached_property(cached);
			} else {
				cache_failed = true;
			}
			break;
		}
		for (int i = 0; i < property->count; i++) {
			const indigo_item *source = property->items + i;
			for (int j = 0; j < cached->count; j++) {
				indigo_item *item = cached->items + j;
				if (strcmp(item->name, source->name)) {
					continue;
				}
				switch (cached->type) {
					case INDIGO_TEXT_VECTOR: {
						char *text = source->text.long_value ? strdup(source->text.long_value) : NULL;
						if (source->text.long_value && !text) {
							cache_failed = true;
							break;
						}
						free(item->text.long_value);
						item->text = source->text;
						item->text.long_value = text;
						break;
					}
					case INDIGO_NUMBER_VECTOR:
						item->number.value = source->number.value;
						item->number.target = source->number.target;
						break;
					case INDIGO_SWITCH_VECTOR:
						item->sw.value = source->sw.value;
						break;
					case INDIGO_LIGHT_VECTOR:
						item->light.value = source->light.value;
						break;
					case INDIGO_BLOB_VECTOR:
						item->blob = source->blob;
						if (!*item->blob.url && property->state == INDIGO_OK_STATE && (source->blob.value || indigo_proxy_blob)) {
							snprintf(item->blob.url, sizeof(item->blob.url), "/blob/%p%s", (const void *)source, item->blob.format);
						}
						item->blob.value = NULL;
						break;
				}
				break;
			}
		}
		break;
	}
	pthread_mutex_unlock(&cache_mutex);
	return INDIGO_OK;
}

static indigo_result client_attach(indigo_client *client) {
	return indigo_enumerate_properties(client, &INDIGO_ALL_PROPERTIES);
}

typedef struct {
	char *text;
	size_t size;
	size_t capacity;
	bool failed;
} json_buffer;

static void append_json(json_buffer *buffer, const char *text) {
	if (buffer->failed) {
		return;
	}
	size_t size = strlen(text);
	if (size > SIZE_MAX - buffer->size - 1) {
		buffer->failed = true;
		return;
	}
	size_t needed = buffer->size + size + 1;
	if (needed > buffer->capacity) {
		size_t capacity = needed > SIZE_MAX / 2 ? needed : needed * 2;
		char *copy = realloc(buffer->text, capacity);
		if (!copy) {
			buffer->failed = true;
			return;
		}
		buffer->text = copy;
		buffer->capacity = capacity;
	}
	memcpy(buffer->text + buffer->size, text, size + 1);
	buffer->size += size;
}

static bool valid_utf8(const unsigned char *text, size_t size);

static void append_string(json_buffer *buffer, const char *text) {
	if (!valid_utf8((const unsigned char *)text, strlen(text))) {
		buffer->failed = true;
		return;
	}
	append_json(buffer, "\"");
	for (const unsigned char *p = (const unsigned char *)text; *p && !buffer->failed; p++) {
		char escaped[7];
		if (*p == '"' || *p == '\\') {
			escaped[0] = '\\';
			escaped[1] = *p;
			escaped[2] = 0;
		} else if (*p < 32) {
			snprintf(escaped, sizeof(escaped), "\\u%04x", *p);
		} else {
			escaped[0] = *p;
			escaped[1] = 0;
		}
		append_json(buffer, escaped);
	}
	append_json(buffer, "\"");
}

static void append_number(json_buffer *buffer, double value) {
	char number[64];
	if (isfinite(value)) {
		snprintf(number, sizeof(number), "%.17g", value);
		indigo_fix_locale(number);
		append_json(buffer, number);
	} else {
		append_json(buffer, "null");
	}
}

static void string_field(json_buffer *buffer, const char *name, const char *value) {
	append_string(buffer, name);
	append_json(buffer, ":");
	append_string(buffer, value);
}

static void property_json(json_buffer *buffer, const indigo_property *property, bool values) {
	append_json(buffer, "{");
	string_field(buffer, "device", property->device);
	append_json(buffer, ",");
	string_field(buffer, "name", property->name);
	append_json(buffer, ",");
	string_field(buffer, "label", property->label);
	append_json(buffer, ",");
	string_field(buffer, "group", property->group);
	append_json(buffer, ",");
	string_field(buffer, "type", property->type == INDIGO_TEXT_VECTOR ? "text" : property->type == INDIGO_NUMBER_VECTOR ? "number" : property->type == INDIGO_SWITCH_VECTOR ? "switch" : property->type == INDIGO_LIGHT_VECTOR ? "light" : "blob");
	append_json(buffer, ",");
	string_field(buffer, "state", indigo_property_state_text[property->state]);
	append_json(buffer, ",");
	string_field(buffer, "permission", property->perm == INDIGO_RO_PERM ? "ro" : property->perm == INDIGO_WO_PERM ? "wo" : "rw");
	if (property->type == INDIGO_SWITCH_VECTOR) {
		append_json(buffer, ",");
		string_field(buffer, "rule", indigo_switch_rule_text[property->rule]);
	}
	append_json(buffer, ",\"items\":[");
	for (int i = 0; i < property->count; i++) {
		const indigo_item *item = property->items + i;
		append_json(buffer, i ? ",{" : "{");
		string_field(buffer, "name", item->name);
		append_json(buffer, ",");
		string_field(buffer, "label", item->label);
		if (property->type == INDIGO_NUMBER_VECTOR) {
			append_json(buffer, ",\"min\":");
			append_number(buffer, item->number.min);
			append_json(buffer, ",\"max\":");
			append_number(buffer, item->number.max);
			append_json(buffer, ",\"step\":");
			append_number(buffer, item->number.step);
			append_json(buffer, ",");
			string_field(buffer, "format", item->number.format);
		} else if (property->type == INDIGO_BLOB_VECTOR) {
			append_json(buffer, ",");
			string_field(buffer, "format", item->blob.format);
		}
		if (values && property->type == INDIGO_BLOB_VECTOR) {
			append_json(buffer, ",");
			string_field(buffer, "url", property->state == INDIGO_OK_STATE ? item->blob.url : "");
			append_json(buffer, ",\"size\":");
			char size[32];
			snprintf(size, sizeof(size), "%ld", item->blob.size);
			append_json(buffer, size);
		} else if (values) {
			append_json(buffer, ",\"value\":");
			switch (property->type) {
				case INDIGO_TEXT_VECTOR:
					append_string(buffer, indigo_get_text_item_value((indigo_item *)item));
					break;
				case INDIGO_NUMBER_VECTOR:
					append_number(buffer, item->number.value);
					append_json(buffer, ",\"target\":");
					append_number(buffer, item->number.target);
					break;
				case INDIGO_SWITCH_VECTOR:
					append_json(buffer, item->sw.value ? "true" : "false");
					break;
				case INDIGO_LIGHT_VECTOR:
					append_string(buffer, indigo_property_state_text[item->light.value]);
					break;
				default:
					append_json(buffer, "null");
					break;
			}
		}
		append_json(buffer, "}");
	}
	append_json(buffer, "]");
	append_json(buffer, "}");
}

static bool valid_utf8(const unsigned char *text, size_t size) {
	for (size_t i = 0; i < size;) {
		unsigned c = text[i++];
		if (c < 0x80) {
			continue;
		}
		unsigned count = c >= 0xc2 && c <= 0xdf ? 1 : c >= 0xe0 && c <= 0xef ? 2 : c >= 0xf0 && c <= 0xf4 ? 3 : 0;
		if (!count || count > size - i) {
			return false;
		}
		unsigned code = c & (0x7f >> count);
		unsigned minimum = count == 1 ? 0x80 : count == 2 ? 0x800 : 0x10000;
		while (count--) {
			c = text[i++];
			if ((c & 0xc0) != 0x80) {
				return false;
			}
			code = (code << 6) | (c & 0x3f);
		}
		if (code < minimum || code > 0x10ffff || (code >= 0xd800 && code <= 0xdfff)) {
			return false;
		}
	}
	return true;
}

static unsigned hex_quad(const char *text) {
	unsigned result = 0;
	for (int i = 0; i < 4; i++) {
		unsigned c = (unsigned char)text[i];
		result = (result << 4) | (c <= '9' ? c - '0' : c <= 'F' ? c - 'A' + 10 : c - 'a' + 10);
	}
	return result;
}

static bool valid_string(const json_document *document, int index) {
	const jsmntok_t *token = document->tokens + index;
	for (int i = token->start; i < token->end;) {
		unsigned c = (unsigned char)document->text[i++];
		if (c < 32) {
			return false;
		}
		if (c == '\\') {
			c = (unsigned char)document->text[i++];
			if (c == 'u') {
				unsigned code = hex_quad(document->text + i);
				i += 4;
				if (code >= 0xd800 && code <= 0xdbff) {
					if (i + 6 > token->end || document->text[i] != '\\' || document->text[i + 1] != 'u') {
						return false;
					}
					code = hex_quad(document->text + i + 2);
					if (code < 0xdc00 || code > 0xdfff) {
						return false;
					}
					i += 6;
				} else if (code >= 0xdc00 && code <= 0xdfff) {
					return false;
				}
			}
		}
	}
	return true;
}

static int token_begin(const json_document *document, int index) {
	return document->tokens[index].start - (document->tokens[index].type == JSMN_STRING);
}

static int token_end(const json_document *document, int index) {
	return document->tokens[index].end + (document->tokens[index].type == JSMN_STRING);
}

static int skip_space(const char *text, int position, int end) {
	while (position < end && text[position] && strchr(" \t\r\n", text[position])) {
		position++;
	}
	return position;
}

static bool valid_primitive(const char *text, int size) {
	if ((size == 4 && (!memcmp(text, "true", 4) || !memcmp(text, "null", 4))) || (size == 5 && !memcmp(text, "false", 5))) {
		return true;
	}
	int i = size > 0 && text[0] == '-';
	if (i == size) {
		return false;
	}
	if (text[i] == '0') {
		i++;
	} else {
		if (text[i] < '1' || text[i] > '9') {
			return false;
		}
		while (i < size && text[i] >= '0' && text[i] <= '9') {
			i++;
		}
	}
	if (i < size && text[i] == '.') {
		int start = ++i;
		while (i < size && text[i] >= '0' && text[i] <= '9') {
			i++;
		}
		if (i == start) {
			return false;
		}
	}
	if (i < size && (text[i] == 'e' || text[i] == 'E')) {
		i++;
		if (i < size && (text[i] == '+' || text[i] == '-')) {
			i++;
		}
		int start = i;
		while (i < size && text[i] >= '0' && text[i] <= '9') {
			i++;
		}
		if (i == start) {
			return false;
		}
	}
	return i == size;
}

// jsmn provides bounded tokens; separators, primitive grammar and full-document
// consumption are validated here because tokenization alone is not JSON validation.
static bool valid_value(const json_document *document, int index, unsigned depth, int *next) {
	if (index >= document->count || depth > MCP_DEPTH_LIMIT) {
		return false;
	}
	const jsmntok_t *token = document->tokens + index;
	*next = index + 1;
	if (token->type == JSMN_STRING) {
		return valid_string(document, index);
	}
	if (token->type == JSMN_PRIMITIVE) {
		return valid_primitive(document->text + token->start, token->end - token->start);
	}
	int position = skip_space(document->text, token->start + 1, token->end - 1);
	int child = index + 1;
	while (position < token->end - 1) {
		if (child >= document->count || token_begin(document, child) != position) {
			return false;
		}
		if (token->type == JSMN_OBJECT) {
			if (document->tokens[child].type != JSMN_STRING || !valid_string(document, child)) {
				return false;
			}
			position = skip_space(document->text, token_end(document, child), token->end - 1);
			if (position >= token->end - 1 || document->text[position++] != ':') {
				return false;
			}
			position = skip_space(document->text, position, token->end - 1);
			if (++child >= document->count || token_begin(document, child) != position) {
				return false;
			}
		}
		int value = child;
		if (!valid_value(document, value, depth + 1, &child)) {
			return false;
		}
		position = skip_space(document->text, token_end(document, value), token->end - 1);
		if (position == token->end - 1) {
			break;
		}
		if (position >= token->end - 1 || document->text[position++] != ',') {
			return false;
		}
		position = skip_space(document->text, position, token->end - 1);
		if (position == token->end - 1) {
			return false;
		}
	}
	*next = child;
	return position == token->end - 1;
}

static bool token_is(const json_document *document, int index, const char *expected) {
	if (index < 0 || document->tokens[index].type != JSMN_STRING) {
		return false;
	}
	const jsmntok_t *token = document->tokens + index;
	for (int i = token->start; i < token->end;) {
		unsigned c = (unsigned char)document->text[i++];
		bool unicode = false;
		if (c == '\\') {
			c = (unsigned char)document->text[i++];
			if (c == 'u') {
				unicode = true;
				c = hex_quad(document->text + i);
				i += 4;
			} else if (c == 'b') {
				c = '\b';
			} else if (c == 'f') {
				c = '\f';
			} else if (c == 'n') {
				c = '\n';
			} else if (c == 'r') {
				c = '\r';
			} else if (c == 't') {
				c = '\t';
			}
		}
		if (c >= 0xd800 && c <= 0xdbff) {
			c = 0x10000 + ((c - 0xd800) << 10) + hex_quad(document->text + i + 2) - 0xdc00;
			i += 6;
		}
		if (unicode && c > 127) {
			unsigned char bytes[4];
			int count = c <= 0x7ff ? 2 : c <= 0xffff ? 3 : 4;
			unsigned code = c;
			for (int j = count - 1; j > 0; j--) {
				bytes[j] = 0x80 | (code & 63);
				code >>= 6;
			}
			bytes[0] = (count == 2 ? 0xc0 : count == 3 ? 0xe0 : 0xf0) | code;
			for (int j = 0; j < count; j++) {
				if (!*expected || bytes[j] != (unsigned char)*expected++) {
					return false;
				}
			}
		} else if (!*expected || c != (unsigned char)*expected++) {
			return false;
		}
	}
	return *expected == 0;
}

static int json_member(const json_document *document, int object, const char *name) {
	if (object < 0 || document->tokens[object].type != JSMN_OBJECT) {
		return -1;
	}
	int result = -1;
	for (int i = object + 1; i < document->count && document->tokens[i].start < document->tokens[object].end;) {
		int value = i + 1;
		if (token_is(document, i, name)) {
			if (result >= 0) {
				return -2;
			}
			result = value;
		}
		i = value + 1;
		while (i < document->count && document->tokens[i].start < document->tokens[value].end) {
			i++;
		}
	}
	return result;
}

static void http_reply(indigo_uni_handle *handle, int status, const char *body) {
	const char *reason = status == 200 ? "OK" : status == 202 ? "Accepted" : status == 403 ? "Forbidden" : status == 404 ? "Not Found" : status == 405 ? "Method Not Allowed" : status == 406 ? "Not Acceptable" : status == 408 ? "Request Timeout" : status == 411 ? "Length Required" : status == 413 ? "Content Too Large" : status == 415 ? "Unsupported Media Type" : status == 417 ? "Expectation Failed" : status == 500 ? "Internal Server Error" : status == 503 ? "Service Unavailable" : "Bad Request";
	indigo_uni_printf(handle, "HTTP/1.1 %d %s\r\n%s%sContent-Length: %zu\r\nConnection: close\r\n\r\n%s", status, reason, status == 405 ? "Allow: POST\r\n" : "", body ? "Content-Type: application/json\r\n" : "", body ? strlen(body) : 0, body ? body : "");
}

static void rpc_reply(indigo_uni_handle *handle, int status, const char *id, int id_size, const char *field, const char *value) {
	json_buffer response = { 0 };
	char request_id[257];
	memcpy(request_id, id, id_size);
	request_id[id_size] = 0;
	append_json(&response, "{\"jsonrpc\":\"2.0\",\"id\":");
	append_json(&response, request_id);
	append_json(&response, ",");
	append_string(&response, field);
	append_json(&response, ":");
	append_json(&response, value);
	append_json(&response, "}");
	http_reply(handle, response.failed ? 500 : status, response.failed ? NULL : response.text);
	free(response.text);
}

static void rpc_error(indigo_uni_handle *handle, int status, const char *id, int id_size, int code, const char *message) {
	char error[256];
	snprintf(error, sizeof(error), "{\"code\":%d,\"message\":\"%s\"}", code, message);
	rpc_reply(handle, status, id, id_size, "error", error);
}

static const char tool_catalog[] = "{\"tools\":["
	"{\"name\":\"list_devices\",\"description\":\"List INDIGO devices with visible properties, including write-only schemas.\",\"inputSchema\":{\"type\":\"object\",\"properties\":{},\"additionalProperties\":false},\"annotations\":{\"readOnlyHint\":true}},"
	"{\"name\":\"list_properties\",\"description\":\"List visible property and item schemas for an exact INDIGO device name. Write-only values are omitted.\",\"inputSchema\":{\"type\":\"object\",\"properties\":{\"device\":{\"type\":\"string\",\"minLength\":1}},\"required\":[\"device\"],\"additionalProperties\":false},\"annotations\":{\"readOnlyHint\":true}},"
	"{\"name\":\"get_property\",\"description\":\"Read the latest observed INDIGO property values and state. BLOBs are URLs; this does not trigger hardware reads or changes.\",\"inputSchema\":{\"type\":\"object\",\"properties\":{\"device\":{\"type\":\"string\",\"minLength\":1},\"property\":{\"type\":\"string\",\"minLength\":1}},\"required\":[\"device\",\"property\"],\"additionalProperties\":false},\"annotations\":{\"readOnlyHint\":true}},"
	"{\"name\":\"set_property\",\"description\":\"Submit visible RW/WO text, number or switch item changes to the INDIGO bus. Dispatch does not confirm token acceptance or operation completion; observe state with get_property, or list_properties for write-only properties. Omitted token means zero; no master token is inherited. BLOB writes are unsupported.\",\"inputSchema\":{\"type\":\"object\",\"properties\":{\"device\":{\"type\":\"string\",\"minLength\":1},\"property\":{\"type\":\"string\",\"minLength\":1},\"items\":{\"type\":\"object\",\"minProperties\":1,\"additionalProperties\":{\"type\":[\"string\",\"number\",\"boolean\"]}},\"token\":{\"type\":\"string\",\"pattern\":\"^[0-9a-fA-F]{1,16}$\"}},\"required\":[\"device\",\"property\",\"items\"],\"additionalProperties\":false},\"annotations\":{\"readOnlyHint\":false,\"destructiveHint\":true,\"idempotentHint\":false}}]}";

static bool only_arguments(const json_document *document, int object, bool device, bool property) {
	if (object == -1) {
		return !device && !property;
	}
	if (object < 0 || document->tokens[object].type != JSMN_OBJECT) {
		return false;
	}
	for (int i = object + 1; i < document->count && document->tokens[i].start < document->tokens[object].end;) {
		if (!(device && token_is(document, i, "device")) && !(property && token_is(document, i, "property"))) {
			return false;
		}
		int end = token_end(document, i + 1);
		i += 2;
		while (i < document->count && document->tokens[i].start < end) {
			i++;
		}
	}
	return true;
}

static void tool_result(indigo_uni_handle *handle, const char *id, int id_size, const char *data, const char *error) {
	json_buffer result = { 0 };
	append_json(&result, "{\"content\":[{\"type\":\"text\",\"text\":");
	append_string(&result, error ? error : data);
	append_json(&result, "}]");
	if (error) {
		append_json(&result, ",\"isError\":true}");
	} else {
		append_json(&result, ",\"structuredContent\":");
		append_json(&result, data);
		append_json(&result, "}");
	}
	if (result.failed) {
		rpc_error(handle, 200, id, id_size, -32603, "Allocation failed");
	} else {
		rpc_reply(handle, 200, id, id_size, "result", result.text);
	}
	free(result.text);
}

static char *decode_string(const json_document *document, int index) {
	if (index < 0 || document->tokens[index].type != JSMN_STRING) {
		return NULL;
	}
	json_buffer value = { 0 };
	append_json(&value, "");
	for (int i = document->tokens[index].start; i < document->tokens[index].end && !value.failed;) {
		unsigned c = (unsigned char)document->text[i++];
		bool unicode = false;
		if (c == '\\') {
			c = (unsigned char)document->text[i++];
			if (c == 'u') {
				unicode = true;
				c = hex_quad(document->text + i);
				i += 4;
				if (c >= 0xd800 && c <= 0xdbff) {
					c = 0x10000 + ((c - 0xd800) << 10) + hex_quad(document->text + i + 2) - 0xdc00;
					i += 6;
				}
			} else if (c == 'b') {
				c = '\b';
			} else if (c == 'f') {
				c = '\f';
			} else if (c == 'n') {
				c = '\n';
			} else if (c == 'r') {
				c = '\r';
			} else if (c == 't') {
				c = '\t';
			}
		}
		if (!c) {
			value.failed = true;
			break;
		}
		char bytes[5] = { 0 };
		if (unicode && c > 127) {
			int count = c <= 0x7ff ? 2 : c <= 0xffff ? 3 : 4;
			for (int j = count - 1; j > 0; j--) {
				bytes[j] = 0x80 | (c & 63);
				c >>= 6;
			}
			bytes[0] = (count == 2 ? 0xc0 : count == 3 ? 0xe0 : 0xf0) | c;
		} else {
			bytes[0] = c;
		}
		append_json(&value, bytes);
	}
	if (value.failed) {
		free(value.text);
		return NULL;
	}
	return value.text;
}

static bool numeric_value(const json_document *document, int index, double *value) {
	const jsmntok_t *token = document->tokens + index;
	char c = document->text[token->start];
	if (token->type != JSMN_PRIMITIVE || !((c >= '0' && c <= '9') || c == '-')) {
		return false;
	}
	size_t length = (size_t)(token->end - token->start);
	char *text = malloc(length + 1);
	if (!text) {
		return false;
	}
	memcpy(text, document->text + token->start, length);
	text[length] = 0;
	char *point = strchr(text, '.');
	const char *decimal = localeconv()->decimal_point;
	if (point && strcmp(decimal, ".")) {
		size_t size = strlen(text), decimal_size = strlen(decimal);
		if (decimal_size > SIZE_MAX - size) {
			free(text);
			return false;
		}
		char *localized = malloc(size + decimal_size);
		if (!localized) {
			free(text);
			return false;
		}
		size_t prefix = (size_t)(point - text);
		memcpy(localized, text, prefix);
		memcpy(localized + prefix, decimal, decimal_size);
		memcpy(localized + prefix + decimal_size, point + 1, size - prefix);
		free(text);
		text = localized;
	}
	char *end;
	*value = strtod(text, &end);
	bool valid = !*end && isfinite(*value);
	free(text);
	return valid;
}

static void set_property(indigo_uni_handle *handle, const json_document *document, int arguments, const char *id, int id_size) {
	int device = json_member(document, arguments, "device");
	int name = json_member(document, arguments, "property");
	int items = json_member(document, arguments, "items");
	int token = json_member(document, arguments, "token");
	if (device < 0 || name < 0 || document->tokens[device].type != JSMN_STRING || document->tokens[name].type != JSMN_STRING || document->tokens[device].start == document->tokens[device].end || document->tokens[name].start == document->tokens[name].end || items < 0 || document->tokens[items].type != JSMN_OBJECT || !document->tokens[items].size || token == -2 || (token >= 0 && document->tokens[token].type != JSMN_STRING)) {
		rpc_error(handle, 200, id, id_size, -32602, "Invalid set_property arguments");
		return;
	}
	for (int i = arguments + 1; i < document->count && document->tokens[i].start < document->tokens[arguments].end;) {
		if (!token_is(document, i, "device") && !token_is(document, i, "property") && !token_is(document, i, "items") && !token_is(document, i, "token")) {
			rpc_error(handle, 200, id, id_size, -32602, "Unknown set_property argument");
			return;
		}
		int end = token_end(document, i + 1);
		i += 2;
		while (i < document->count && document->tokens[i].start < end) {
			i++;
		}
	}
	indigo_token access_token = 0;
	if (token >= 0) {
		char *text = decode_string(document, token);
		bool valid = text && *text && strlen(text) <= 16;
		for (const char *p = text; valid && *p; p++) {
			unsigned c = (unsigned char)*p;
			if (c >= '0' && c <= '9') {
				c -= '0';
			} else if (c >= 'a' && c <= 'f') {
				c -= 'a' - 10;
			} else if (c >= 'A' && c <= 'F') {
				c -= 'A' - 10;
			} else {
				valid = false;
				break;
			}
			access_token = (access_token << 4) | c;
		}
		free(text);
		if (!valid) {
			rpc_error(handle, 200, id, id_size, -32602, "Invalid token");
			return;
		}
	}
	indigo_property *schema = NULL;
	pthread_mutex_lock(&cache_mutex);
	if (!cache_failed) {
		for (cached_property *entry = properties; entry; entry = entry->next) {
			if (token_is(document, device, entry->property->device) && token_is(document, name, entry->property->name)) {
				schema = copy_cached_property(entry->property, false, false);
				break;
			}
		}
	}
	pthread_mutex_unlock(&cache_mutex);
	if (!schema || (schema->perm != INDIGO_RW_PERM && schema->perm != INDIGO_WO_PERM) || (schema->type != INDIGO_TEXT_VECTOR && schema->type != INDIGO_NUMBER_VECTOR && schema->type != INDIGO_SWITCH_VECTOR)) {
		free_cached_property(schema);
		tool_result(handle, id, id_size, NULL, "Property is unavailable, read-only, or has an unsupported write type");
		return;
	}
	indigo_property *request = calloc(1, sizeof(indigo_property) + (size_t)document->tokens[items].size * sizeof(indigo_item));
	if (!request) {
		free_cached_property(schema);
		rpc_error(handle, 200, id, id_size, -32603, "Allocation failed");
		return;
	}
	memcpy(request, schema, sizeof(indigo_property));
	request->allocated_count = document->tokens[items].size;
	request->count = 0;
	request->access_token = access_token;
	bool valid = true;
	for (int i = items + 1; valid && i < document->count && document->tokens[i].start < document->tokens[items].end;) {
		indigo_item *source = NULL;
		for (int j = 0; j < schema->count; j++) {
			if (token_is(document, i, schema->items[j].name)) {
				source = schema->items + j;
				break;
			}
		}
		if (!source) {
			valid = false;
			break;
		}
		for (int j = 0; j < request->count; j++) {
			if (!strcmp(request->items[j].name, source->name)) {
				valid = false;
			}
		}
		indigo_item *item = request->items + request->count++;
		*item = *source;
		if (request->type == INDIGO_TEXT_VECTOR) {
			char *text = decode_string(document, i + 1);
			if (!text) {
				valid = false;
			} else {
				INDIGO_COPY_VALUE(item->text.value, text);
				item->text.length = (long)strlen(text) + 1;
				if (item->text.length > INDIGO_VALUE_SIZE) {
					item->text.long_value = text;
				} else {
					free(text);
				}
			}
		} else if (request->type == INDIGO_NUMBER_VECTOR) {
			valid = numeric_value(document, i + 1, &item->number.value) && valid;
			item->number.target = item->number.value;
		} else {
			const jsmntok_t *value = document->tokens + i + 1;
			bool on = value->type == JSMN_PRIMITIVE && value->end - value->start == 4 && !memcmp(document->text + value->start, "true", 4);
			bool off = value->type == JSMN_PRIMITIVE && value->end - value->start == 5 && !memcmp(document->text + value->start, "false", 5);
			valid = valid && (on || off);
			item->sw.value = item->sw.target = on;
		}
		int end = token_end(document, i + 1);
		i += 2;
		while (i < document->count && document->tokens[i].start < end) {
			i++;
		}
	}
	free_cached_property(schema);
	if (!valid) {
		free_cached_property(request);
		rpc_error(handle, 200, id, id_size, -32602, "Invalid or duplicate property item");
		return;
	}
	pthread_mutex_lock(&service_mutex);
	unsigned slot = 0;
	while (slot < MCP_REQUEST_LIMIT && dispatch_active[slot]) {
		slot++;
	}
	assert(slot < MCP_REQUEST_LIMIT);
	dispatch_threads[slot] = pthread_self();
	dispatch_active[slot] = true;
	pthread_mutex_unlock(&service_mutex);
	indigo_result result = indigo_change_property(&agent_client, request);
	pthread_mutex_lock(&service_mutex);
	dispatch_active[slot] = false;
	pthread_mutex_unlock(&service_mutex);
	free_cached_property(request);
	if (result == INDIGO_OK) {
		tool_result(handle, id, id_size, "{\"dispatched\":true,\"completion\":\"not_confirmed\"}", NULL);
	} else {
		tool_result(handle, id, id_size, NULL, "Bus dispatch failed");
	}
}

static void call_tool(indigo_uni_handle *handle, const json_document *document, int params, const char *id, int id_size) {
	int name = json_member(document, params, "name");
	int arguments = json_member(document, params, "arguments");
	if (token_is(document, name, "set_property")) {
		set_property(handle, document, arguments, id, id_size);
		return;
	}
	bool devices = token_is(document, name, "list_devices");
	bool list = token_is(document, name, "list_properties");
	bool get = token_is(document, name, "get_property");
	if (!devices && !list && !get) {
		rpc_error(handle, 200, id, id_size, -32602, "Unknown tool");
		return;
	}
	int device = json_member(document, arguments, "device");
	int property = json_member(document, arguments, "property");
	if (!only_arguments(document, arguments, !devices, get) || (!devices && (device < 0 || document->tokens[device].type != JSMN_STRING || document->tokens[device].start == document->tokens[device].end)) || (get && (property < 0 || document->tokens[property].type != JSMN_STRING || document->tokens[property].start == document->tokens[property].end))) {
		rpc_error(handle, 200, id, id_size, -32602, "Invalid tool arguments");
		return;
	}
	cached_property *snapshot = NULL;
	cached_property **tail = &snapshot;
	pthread_mutex_lock(&cache_mutex);
	bool failed = cache_failed;
	for (cached_property *entry = properties; !failed && entry; entry = entry->next) {
		if ((get && entry->property->perm == INDIGO_WO_PERM) || (!devices && !token_is(document, device, entry->property->device)) || (get && !token_is(document, property, entry->property->name))) {
			continue;
		}
		bool duplicate = false;
		if (devices) {
			for (cached_property *other = snapshot; other; other = other->next) {
				if (!strcmp(other->property->device, entry->property->device)) {
					duplicate = true;
					break;
				}
			}
		}
		if (duplicate) {
			continue;
		}
		cached_property *copy = malloc(sizeof(cached_property));
		if (!copy) {
			failed = true;
			break;
		}
		indigo_property header = *entry->property;
		header.count = 0;
		*copy = (cached_property){ copy_cached_property(devices ? &header : entry->property, false, get), NULL };
		*tail = copy;
		tail = &copy->next;
		failed = copy->property == NULL;
	}
	pthread_mutex_unlock(&cache_mutex);
	json_buffer data = { 0 };
	if (!failed && (devices || snapshot)) {
		if (!get) {
			append_json(&data, devices ? "{\"devices\":[" : "{\"properties\":[");
		}
		bool first = true;
		for (cached_property *entry = snapshot; entry; entry = entry->next) {
			append_json(&data, first ? "" : ",");
			if (devices) {
				append_json(&data, "{");
				string_field(&data, "name", entry->property->device);
				append_json(&data, "}");
			} else {
				property_json(&data, entry->property, get);
			}
			first = false;
		}
		if (!get) {
			append_json(&data, "]}");
		}
	}
	tool_result(handle, id, id_size, data.text, failed || data.failed ? "Unable to serialize property snapshot; reload the MCP agent if its cache is unavailable." : !devices && !snapshot ? (get ? "Property not found" : "Device not found") : NULL);
	free(data.text);
	while (snapshot) {
		cached_property *entry = snapshot;
		snapshot = entry->next;
		free_cached_property(entry->property);
		free(entry);
	}
}

static void process_rpc(indigo_uni_handle *handle, const char *body, size_t size, const char *version) {
	json_document document = { .text = body };
	jsmn_parser parser;
	jsmn_init(&parser);
	document.count = valid_utf8((const unsigned char *)body, size) ? jsmn_parse(&parser, body, size, document.tokens, MCP_TOKEN_LIMIT) : -1;
	int next;
	if (document.count <= 0 || token_begin(&document, 0) != skip_space(body, 0, (int)size) || !valid_value(&document, 0, 0, &next) || next != document.count || skip_space(body, token_end(&document, 0), (int)size) != (int)size) {
		rpc_error(handle, 400, "null", 4, -32700, "Parse error");
		return;
	}
	int id = json_member(&document, 0, "id");
	int method = json_member(&document, 0, "method");
	int params = json_member(&document, 0, "params");
	int jsonrpc = json_member(&document, 0, "jsonrpc");
	bool valid_id = id == -1;
	if (id >= 0) {
		const jsmntok_t *token = document.tokens + id;
		valid_id = token->type == JSMN_STRING;
		if (token->type == JSMN_PRIMITIVE) {
			int i = token->start + (body[token->start] == '-');
			while (i < token->end && body[i] >= '0' && body[i] <= '9') {
				i++;
			}
			valid_id = i == token->end;
		}
		valid_id = valid_id && token_end(&document, id) - token_begin(&document, id) <= 256;
	}
	if (!token_is(&document, jsonrpc, "2.0") || method < 0 || document.tokens[method].type != JSMN_STRING || !valid_id || id == -2 || params == -2 || (params >= 0 && document.tokens[params].type != JSMN_OBJECT)) {
		rpc_error(handle, 400, "null", 4, -32600, "Invalid request");
		return;
	}
	const char *id_text = id >= 0 ? body + token_begin(&document, id) : "null";
	int id_size = id >= 0 ? token_end(&document, id) - token_begin(&document, id) : 4;
	bool initialize = token_is(&document, method, "initialize");
	bool ping = token_is(&document, method, "ping");
	if ((*version && strcmp(version, MCP_VERSION)) || (!*version && !initialize && !ping)) {
		rpc_error(handle, 400, id_text, id_size, -32600, "Unsupported protocol version");
		return;
	}
	if (id == -1) {
		if (initialize || ping || token_is(&document, method, "tools/list") || token_is(&document, method, "tools/call")) {
			rpc_error(handle, 400, "null", 4, -32600, "Request id required");
		} else {
			http_reply(handle, 202, NULL);
		}
		return;
	}
	if (initialize) {
		int protocol = json_member(&document, params, "protocolVersion");
		int capabilities = json_member(&document, params, "capabilities");
		int client = json_member(&document, params, "clientInfo");
		int name = json_member(&document, client, "name");
		int client_version = json_member(&document, client, "version");
		if (protocol < 0 || document.tokens[protocol].type != JSMN_STRING || capabilities < 0 || document.tokens[capabilities].type != JSMN_OBJECT || name < 0 || document.tokens[name].type != JSMN_STRING || client_version < 0 || document.tokens[client_version].type != JSMN_STRING) {
			rpc_error(handle, 200, id_text, id_size, -32602, "Invalid initialize parameters");
		} else {
			rpc_reply(handle, 200, id_text, id_size, "result", "{\"protocolVersion\":\"" MCP_VERSION "\",\"capabilities\":{\"tools\":{}},\"serverInfo\":{\"name\":\"INDIGO MCP Agent\",\"version\":\"3.0.0.0\"}}");
		}
	} else if (ping) {
		rpc_reply(handle, 200, id_text, id_size, "result", "{}");
	} else if (token_is(&document, method, "tools/list")) {
		rpc_reply(handle, 200, id_text, id_size, "result", tool_catalog);
	} else if (token_is(&document, method, "tools/call")) {
		call_tool(handle, &document, params, id_text, id_size);
	} else {
		rpc_error(handle, 200, id_text, id_size, -32601, "Method not found");
	}
}

static bool wait_for_input(indigo_uni_handle *handle, double deadline) {
	double remaining = deadline - indigo_monotonic_time();
	if (remaining > MCP_PROGRESS_TIMEOUT) {
		remaining = MCP_PROGRESS_TIMEOUT;
	}
	return remaining > 0 && indigo_uni_wait_for_data(handle, (long)(remaining * 1000000)) > 0 && indigo_monotonic_time() < deadline;
}

static long header_line(indigo_uni_handle *handle, char *line, size_t capacity, double deadline) {
	size_t used = 0;
	while (used + 1 < capacity) {
		char c;
		if (!wait_for_input(handle, deadline) || indigo_uni_read_available(handle, &c, 1) != 1 || c == 0) {
			return -1;
		}
		if (c == '\n') {
			if (used && line[used - 1] == '\r') {
				used--;
			}
			line[used] = 0;
			return (long)used;
		}
		line[used++] = c;
	}
	return -1;
}

static bool header_token(const char *name) {
	if (!*name) {
		return false;
	}
	for (; *name; name++) {
		unsigned c = (unsigned char)*name;
		if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == 96 || strchr("!#$%&'*+-.^_|~", c))) {
			return false;
		}
	}
	return true;
}

static bool has_media_type(const char *text, const char *media_type) {
	size_t length = strlen(media_type);
	while (*text) {
		while (*text == ' ' || *text == '\t' || *text == ',') {
			text++;
		}
		if (!strncasecmp(text, media_type, length) && (!text[length] || text[length] == ';' || text[length] == ',' || text[length] == ' ' || text[length] == '\t')) {
			return true;
		}
		text = strchr(text, ',');
		if (!text) {
			break;
		}
	}
	return false;
}

static int read_request(indigo_uni_handle *handle, char **body, size_t *size, char *version) {
	char line[1024];
	bool have_length = false, json = false, accept_json = false, accept_sse = false, have_type = false, have_version = false;
	double deadline = indigo_monotonic_time() + MCP_TOTAL_TIMEOUT;
	for (unsigned i = 0; i <= 64; i++) {
		long length = header_line(handle, line, sizeof(line), deadline);
		if (length < 0 || (length > 0 && i == 64)) {
			return 400;
		}
		if (!length) {
			if (!have_length) {
				return 411;
			}
			if (!json) {
				return 415;
			}
			if (!accept_json || !accept_sse) {
				return 406;
			}
			*body = malloc(*size + 1);
			if (!*body) {
				return 503;
			}
			size_t used = 0;
			while (used < *size) {
				if (!wait_for_input(handle, deadline)) {
					return 408;
				}
				long count = indigo_uni_read_available(handle, *body + used, (long)(*size - used));
				if (count <= 0) {
					return 400;
				}
				used += (size_t)count;
			}
			(*body)[*size] = 0;
			return 200;
		}
		char *value = strchr(line, ':');
		if (!value) {
			return 400;
		}
		*value++ = 0;
		if (!header_token(line)) {
			return 400;
		}
		for (const char *p = value; *p; p++) {
			if (((unsigned char)*p < 32 && *p != '\t') || (unsigned char)*p == 127) {
				return 400;
			}
		}
		while (*value == ' ' || *value == '\t') {
			value++;
		}
		char *end = value + strlen(value);
		while (end > value && (end[-1] == ' ' || end[-1] == '\t')) {
			*--end = 0;
		}
		if (!strcasecmp(line, "Content-Length")) {
			if (have_length || !*value) {
				return 400;
			}
			have_length = true;
			for (const char *p = value; *p; p++) {
				if (*p < '0' || *p > '9') {
					return 400;
				}
				unsigned digit = *p - '0';
				if (*size > (MCP_BODY_LIMIT - digit) / 10) {
					return 413;
				}
				*size = *size * 10 + digit;
			}
		} else if (!strcasecmp(line, "Transfer-Encoding")) {
			return 400;
		} else if (!strcasecmp(line, "Expect")) {
			return 417;
		} else if (!strcasecmp(line, "Origin")) {
			return 403;
		} else if (!strcasecmp(line, "Content-Type")) {
			if (have_type) {
				return 400;
			}
			have_type = true;
			json = has_media_type(value, "application/json") && !strchr(value, ',');
		} else if (!strcasecmp(line, "Accept")) {
			accept_json |= has_media_type(value, "application/json");
			accept_sse |= has_media_type(value, "text/event-stream");
		} else if (!strcasecmp(line, "MCP-Protocol-Version")) {
			if (have_version || !*value || strlen(value) >= 32) {
				return 400;
			}
			have_version = true;
			strcpy(version, value);
		}
	}
	return 400;
}

static bool mcp_handler(indigo_uni_handle *handle, char *method, char *path, char *params) {
	if (strcmp(path, "/mcp")) {
		http_reply(handle, 404, NULL);
		return false;
	}
	if (strcmp(method, "POST")) {
		http_reply(handle, 405, NULL);
		return false;
	}
	handle->log_level = -abs(handle->log_level);
	pthread_mutex_lock(&service_mutex);
	bool admitted = service_enabled && active_requests < MCP_REQUEST_LIMIT;
	if (admitted) {
		active_requests++;
	}
	pthread_mutex_unlock(&service_mutex);
	if (!admitted) {
		http_reply(handle, 503, NULL);
		return false;
	}
	char *body = NULL;
	size_t size = 0;
	char version[32] = "";
	int status = read_request(handle, &body, &size, version);
	if (status == 200) {
		process_rpc(handle, body, size, version);
	} else {
		http_reply(handle, status, NULL);
	}
	free(body);
	pthread_mutex_lock(&service_mutex);
	active_requests--;
	pthread_mutex_unlock(&service_mutex);
	return false;
}

static indigo_result device_attach(indigo_device *device) {
	assert(device != NULL);
	indigo_result result = indigo_device_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AGENT);
	if (result == INDIGO_OK) {
		CONFIG_PROPERTY->hidden = true;
		PROFILE_PROPERTY->hidden = true;
		ADDITIONAL_INSTANCES_PROPERTY->hidden = true;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
	}
	return result;
}

static indigo_result change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(property != NULL);
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		pthread_mutex_lock(&service_mutex);
		service_enabled = CONNECTION_CONNECTED_ITEM->sw.value;
		pthread_mutex_unlock(&service_mutex);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		return INDIGO_OK;
	}
	return indigo_device_change_property(device, client, property);
}

static indigo_result device_detach(indigo_device *device) {
	assert(device != NULL);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_device_detach(device);
}

indigo_result indigo_agent_mcp(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static const indigo_device device_template = INDIGO_DEVICE_INITIALIZER(MCP_AGENT_NAME, device_attach, indigo_device_enumerate_properties, change_property, NULL, device_detach);
	SET_DRIVER_INFO(info, "MCP agent", __FUNCTION__, DRIVER_VERSION, false, last_action);
	if (action == last_action) {
		return INDIGO_OK;
	}
	switch (action) {
		case INDIGO_DRIVER_INIT: {
			agent_device = malloc(sizeof(indigo_device));
			if (!agent_device) {
				return INDIGO_FAILED;
			}
			memcpy(agent_device, &device_template, sizeof(indigo_device));
			indigo_result result = indigo_attach_device(agent_device);
			if (result != INDIGO_OK || agent_device->last_result != INDIGO_OK) {
				if (result == INDIGO_OK) {
					indigo_detach_device(agent_device);
				}
				free(agent_device);
				agent_device = NULL;
				return INDIGO_FAILED;
			}
			agent_client = (indigo_client){ .name = "MCP Agent client", .version = INDIGO_VERSION_CURRENT, .attach = client_attach, .define_property = cache_define, .update_property = cache_update, .delete_property = cache_delete };
			result = indigo_attach_client(&agent_client);
			if (result != INDIGO_OK || agent_client.last_result != INDIGO_OK) {
				if (result == INDIGO_OK) {
					indigo_detach_client(&agent_client);
				}
				clear_cache();
				indigo_detach_device(agent_device);
				free(agent_device);
				agent_device = NULL;
				return INDIGO_FAILED;
			}
			indigo_server_add_handler("/mcp", mcp_handler);
			last_action = action;
			break;
		}
		case INDIGO_DRIVER_SHUTDOWN:
			pthread_mutex_lock(&service_mutex);
			for (unsigned i = 0; i < MCP_REQUEST_LIMIT; i++) {
				if (dispatch_active[i] && pthread_equal(dispatch_threads[i], pthread_self())) {
					pthread_mutex_unlock(&service_mutex);
					return INDIGO_BUSY;
				}
			}
			service_enabled = false;
			pthread_mutex_unlock(&service_mutex);
			indigo_server_remove_resource("/mcp");
			indigo_detach_client(&agent_client);
			clear_cache();
			if (agent_device) {
				indigo_detach_device(agent_device);
				free(agent_device);
				agent_device = NULL;
			}
			last_action = action;
			break;
		case INDIGO_DRIVER_INFO:
			break;
	}
	return INDIGO_OK;
}
