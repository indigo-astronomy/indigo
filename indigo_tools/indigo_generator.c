// Copyright (c) 2025 CloudMakers, s. r. o.
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
// 3.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <libgen.h>
#include <limits.h>

#pragma mark - typedefs

typedef enum {
	TOKEN_NONE = 0, TOKEN_LBRACE = '{', TOKEN_RBRACE = '}', TOKEN_EQUAL = '=', TOKEN_SEMICOLON = ';', TOKEN_IDENTIFIER = 'I', TOKEN_STRING = 'S', TOKEN_NUMBER = 'N', TOKEN_TRUE = 'T', TOKEN_FALSE = 'F', TOKEN_CODE = 'C', TOKEN_EXPRESSION = 'E'
} token_type;

typedef struct code_type {
	struct code_type *next;
	char *text;
	int size;
} code_type;

typedef struct item_type {
	struct item_type *next;
	char id[128], handle[128], name[128], define_name[128], label[256], value[256], min[256], max[256], step[256], format[16];
	code_type *attach;
} item_type;

typedef struct property_type {
	struct property_type *next;
	char type[12], id[128], handle[128], name[128], define_name[128], pointer[128], handler[64], label[256], group[32],  perm[32], rule[32], hidden[64];
	bool always_defined, handle_change, asynchronous_change, persistent, preserve_values;
	int max_name_length;
	code_type *code, *on_attach, *on_change, *on_detach;
	item_type *items;
} property_type;

typedef struct device_type {
	struct device_type *next;
	char type[16], handle[64], name[64], interface[128];
	bool additional_instances;
	code_type *code, *on_timer, *on_attach, *on_connect, *on_disconnect, *on_detach;
	property_type *properties;
} device_type;

typedef struct libusb_type {
	bool hotplug;
	char pid[128],vid[128];
} libusb_type;

typedef struct pattern_type {
	struct pattern_type *next;
	char pid[16],vid[16], product[64], vendor[64], serial[64], exact_match[64];
} pattern_type;

typedef struct serial_type {
	bool configurable_speed;
	pattern_type *patterns;
} serial_type;

typedef struct definition_type {
	struct definition_type *next;
	char name[64], value[128];
} definition_type;

typedef struct driver_type {
	char name[64], label[256], author[256], copyright[256];
	int version;
	bool virtual;
	definition_type *definions;
	device_type *devices;
	serial_type *serial;
	libusb_type *libusb;
	code_type *include, *define, *code, *data, *on_init, *on_shutdown;
} driver_type;

#pragma mark - shared variables

bool parsing_code = false, parsing_expression = false, verbose_log = false, empty_line = false;
char *definition_source, *definition_source_basename, *definition_source_dirname, *current, *begin, *end;
token_type last_token;
device_type *device = NULL;
property_type *property = NULL;
definition_type *defs = NULL;
code_type *todos = NULL;
driver_type driver;
int indentation = 0;
int line = 1, column = 1;

#pragma mark - helpers

void *allocate(int size) {
	void *result = malloc(size);
	memset(result, 0, size);
	return result;
}

void make_upper_case(char *name) {
	int c;
	while ((c = *name)) {
		*name++ = toupper(c);
	}
}

void make_lower_case(char *name) {
	int c;
	while ((c = *name)) {
		*name++ = tolower(c);
	}
}

void debug(int offset, const char *format, ...) {
	if (verbose_log) {
		static char *spaces = "\t\t\t\t\t\t\t\t\t\t";
		va_list args;
		va_start(args, format);
		char buffer[256];
		vsnprintf(buffer, sizeof(buffer), format, args);
		fprintf(stderr, "%s%s\n", spaces + 10 - indentation - offset, buffer);
		va_end(args);
	}
}

void report_error(const char *format, ...) {
	va_list args;
	va_start(args, format);
	char buffer[256];
	vsnprintf(buffer, sizeof(buffer), format, args);
	fprintf(stderr, "ERROR (%d:%d): %s\n", line, column, buffer);
	va_end(args);
}

void report_unexpected_token_error(void) {
	switch (last_token) {
		case TOKEN_NONE:
			report_error("Unexpected token NONE");
			break;
		case TOKEN_LBRACE:
		case TOKEN_RBRACE:
		case TOKEN_EQUAL:
		case TOKEN_SEMICOLON:
			report_error("Unexpected token '%c'", last_token);
			break;
		case TOKEN_IDENTIFIER:
		case TOKEN_STRING:
		case TOKEN_NUMBER:
			report_error("Unexpected token '%.*s'", end - begin, begin);
			break;
		case TOKEN_TRUE:
			report_error("Unexpected token 'true");
			break;
		case TOKEN_FALSE:
			report_error("Unexpected token 'false");
			break;
		case TOKEN_CODE:
			report_error("Unexpected token CODE");
		case TOKEN_EXPRESSION:
			report_error("Unexpected token EXPRESSION");
		default:
			break;
	}
}

#pragma mark - parser

#define FORWARD(n) if (*(current += n) == '\n') { line++; column = 0; } else { column++; }

void copy(char *target, int max_size) {
	memset(target, 0, max_size);
	int size = (int)(end - begin);
	if (size > max_size) {
		size = max_size - 1;
	}
	strncpy(target, begin, size);
}

void append(void **head, void *next) {
	if (*head == NULL) {
		*head = next;
	} else {
		append(*head, next);
	}
}

bool get_token(void) {
	if (parsing_code) {
		while (true) {
			if (*current == 0) {
				return false;
			}
			if (*current != '\n') {
				break;
			}
			FORWARD(1);
		}
		int depth = 0;
		begin = current;
		while (true) {
			if (*current == 0) {
				return false;
			}
			if (*current == '{') {
				depth++;
			} else if (*current == '}') {
				depth--;
				if (depth == -1) {
					end = current;
					while (end > begin) {
						if (!isspace(*(end - 1))) {
							break;
						}
						end--;
					}
					last_token = TOKEN_CODE;
					return true;
				}
			}
			FORWARD(1);
		}
		return true;
	} else if (parsing_expression) {
		while (true) {
			if (*current == 0) {
				return false;
			}
			if (!isspace(*current)) {
				break;
			}
			FORWARD(1);
		}
		begin = current;
		while (true) {
			if (*current == 0) {
				return false;
			}
			if (*current == ';') {
				end = current;
				while (end > begin) {
					if (!isspace(*(end - 1))) {
						break;
					}
					end--;
				}
				last_token = TOKEN_EXPRESSION;
				return true;
			}
			current++;
		}
		return true;
	} else {
		while (*current != 0) {
			if (isspace(*current)) {
				FORWARD(1);
			} else if (*current == '{') {
				FORWARD(1);
				indentation += 1;
				last_token = TOKEN_LBRACE;
				return true;
			} else if (*current == '}') {
				FORWARD(1);
				indentation -= 1;
				last_token = TOKEN_RBRACE;
				return true;
			} else if (*current == '=') {
				FORWARD(1);
				last_token = TOKEN_EQUAL;
				return true;
			} else if (*current == ';') {
				FORWARD(1);
				last_token = TOKEN_SEMICOLON;
				return true;
			} else if (*current == '/' && *(current + 1) == '/') {
				code_type *todo = NULL;
				if (strncmp(current, "// TODO:", 8) == 0) {
					todo = allocate(sizeof(code_type));
					todo->text = current;
				}
				while (*current != 0 && *current != '\n') {
					FORWARD(1);
				}
				if (todo) {
					todo->size = (int)(current - todo->text);
					append((void **)&todos, todo);
				}
			} else if (*current == '"') {
				FORWARD(1);
				begin = current;
				while (*current != 0 && *current != '"') {
					FORWARD(1);
				}
				end = current;
				FORWARD(1);
				last_token = TOKEN_STRING;
				return true;
			} else if (strncmp(current, "true", 4) == 0) {
				FORWARD(4);
				last_token = TOKEN_TRUE;
				return true;
			} else if (strncmp(current, "false", 5) == 0) {
				FORWARD(5);
				last_token = TOKEN_FALSE;
				return true;
			} else if (isalpha(*current)) {
				begin = current;
				FORWARD(1);
				while (isalnum(*current) || *current == '_') {
					FORWARD(1);
				}
				end = current;
				last_token = TOKEN_IDENTIFIER;
				return true;
			} else if (isdigit(*current) || *current == '-') {
				begin = current;
				FORWARD(1);
				while (isdigit(*current) || *current == '.' || *current == '-' || *current == 'E' || *current == 'e') {
					FORWARD(1);
				}
				end = current;
				last_token = TOKEN_NUMBER;
				return true;
			}
		}
	}
	return false;
}

bool match(token_type token, char *value) {
	if (last_token == TOKEN_NONE) {
		if (!get_token()) {
			return false;
		}
	}
	if (last_token != token) {
		return false;
	}
	if (value != NULL) {
		bool result = strncmp(value, begin, strlen(value)) == 0;
		if (result) {
			last_token = TOKEN_NONE;
		}
		return result;
	}
	last_token = TOKEN_NONE;
	return true;
}

bool parse_string_attribute(char *name, char *value, int max_size) {
	if (!match(TOKEN_IDENTIFIER, name)) {
		return false;
	}
	if (!match(TOKEN_EQUAL, NULL)) {
		report_error("Missing '='");
		return false;
	}
	if (!match(TOKEN_STRING, NULL)) {
		report_error("Missing string");
		return false;
	}
	copy(value, max_size);
	debug(0, "%s = \"%s\";", name, value);
	match(TOKEN_SEMICOLON, NULL);
	return true;
}

bool parse_identifier_attribute(char *name, char *value, int max_size) {
	if (!match(TOKEN_IDENTIFIER, name)) {
		return false;
	}
	if (!match(TOKEN_EQUAL, NULL)) {
		report_error("Missing '='");
		return false;
	}
	if (!match(TOKEN_IDENTIFIER, NULL)) {
		report_error("Missing identifier");
		return false;
	}
	copy(value, max_size);
	debug(0, "%s = %s;", name, value);
	match(TOKEN_SEMICOLON, NULL);
	return true;
}

bool parse_bool_attribute(char *name, bool *value) {
	if (!match(TOKEN_IDENTIFIER, name)) {
		return false;
	}
	if (!match(TOKEN_EQUAL, NULL)) {
		report_error("Missing '='");
		return false;
	}
	if (match(TOKEN_FALSE, NULL)) {
		match(TOKEN_SEMICOLON, NULL);
		*value = false;
		return true;
	}
	if (match(TOKEN_TRUE, NULL)) {
		match(TOKEN_SEMICOLON, NULL);
		*value = true;
		return true;
	}
	report_error("Missing bool");
	return false;
}

bool parse_int_attribute(char *name, int *value) {
	if (!match(TOKEN_IDENTIFIER, name)) {
		return false;
	}
	if (!match(TOKEN_EQUAL, NULL)) {
		report_error("Missing '='");
		return false;
	}
	if (!match(TOKEN_NUMBER, NULL)) {
		report_error("Missing double");
		return false;
	}
	char number[18];
	copy(number, sizeof(number));
	*value = (int)atof(number);
	debug(0, "%s = %d;", name, *value);
	match(TOKEN_SEMICOLON, NULL);
	return true;
}

bool parse_double_attribute(char *name, double *value) {
	if (!match(TOKEN_IDENTIFIER, name)) {
		return false;
	}
	if (!match(TOKEN_EQUAL, NULL)) {
		report_error("Missing '='");
		return false;
	}
	if (!match(TOKEN_NUMBER, NULL)) {
		report_error("Missing int");
		return false;
	}
	char number[18];
	copy(number, sizeof(number));
	*value = atof(number);
	debug(0, "%s = %g;", name, *value);
	match(TOKEN_SEMICOLON, NULL);
	return true;
}

bool parse_expression_attribute(char *name, char *value, int max_size) {
	if (!match(TOKEN_IDENTIFIER, name)) {
		return false;
	}
	if (!match(TOKEN_EQUAL, NULL)) {
		report_error("Missing '='");
		return false;
	}
	parsing_expression = true;
	bool result = get_token();
	parsing_expression = false;
	last_token = TOKEN_NONE;
	if (!result) {
		report_error("Failed to parse C expression");
		return false;
	}
	copy(value, max_size);
	debug(0, "%s = %s;", name, value);
	match(TOKEN_SEMICOLON, NULL);
	return true;
}

bool parse_code_block(char *name, code_type **codes) {
	if (!match(TOKEN_IDENTIFIER, name))
		return false;
	if (!match(TOKEN_LBRACE, NULL)) {
		report_error("Missing '{'");
		return false;
	}
	debug(-1, "%s {", name);
	parsing_code = true;
	bool result = get_token();
	parsing_code = false;
	last_token = TOKEN_NONE;
	if (!result) {
		report_error("Failed to parse C code");
		return false;
	}
	code_type *code = allocate(sizeof(code_type));
	if (!match(TOKEN_RBRACE, NULL)) {
		report_error("Missing '}'");
		return false;
	}
	code->text = begin;
	code->size = (int)(end - begin);
	append((void **)codes, code);
	debug(0, "  ... %d character(s) ...", code->size);
	debug(0, "}");
	return true;
}

bool parse_item_block(char *type, item_type **items) {
	if (!match(TOKEN_IDENTIFIER, "item")) {
		return false;
	}
	if (!match(TOKEN_IDENTIFIER, NULL)) {
		report_error("Missing item name");
		return false;
	}
	char id[64];
	copy(id, sizeof(id));
	item_type *item = allocate(sizeof(item_type));
	snprintf(item->label, sizeof(item->label), "%s", id);
	make_upper_case(id);
	snprintf(item->id, sizeof(item->id), "%s", id);
	snprintf(item->handle, sizeof(item->handle), "%s_ITEM", id);
	snprintf(item->name, sizeof(item->name), "%s_ITEM_NAME", id);
	if (*type == 't') {
		strcpy(item->value, "\"\"");
	} else if (*type == 'n') {
		strcpy(item->min, "0");
		strcpy(item->max, "0");
		strcpy(item->step, "0");
		strcpy(item->value, "0");
	} else if (*type == 's') {
		strcpy(item->value, "false");
	} else if (*type == 'l') {
		strcpy(item->value, "INDIGO_IDLE_STATE");
	}
	if (match(TOKEN_LBRACE, NULL)) {
		debug(-1, "item %s {", item->handle);
		while (!match(TOKEN_RBRACE, NULL)) {
			if (parse_expression_attribute("label", item->label, sizeof(item->label))) {
				continue;
			}
			if (parse_expression_attribute("handle", item->handle, sizeof(item->handle))) {
				continue;
			}
			if (parse_expression_attribute("name", id, sizeof(id))) {
				if (id[0] == '"') {
					strncpy(item->define_name, id + 1, strlen(id) - 2);
				} else {
					strcpy(item->name, id);
				}
				continue;
			}
			if (parse_expression_attribute("value", item->value, sizeof(item->value))) {
				continue;
			}
			if (*type == 'n') {
				if (parse_expression_attribute("min", item->min, sizeof(item->min))) {
					continue;
				}
				if (parse_expression_attribute("max", item->max, sizeof(item->max))) {
					continue;
				}
				if (parse_expression_attribute("step", item->step, sizeof(item->step))) {
					continue;
				}
				if (parse_expression_attribute("format", item->format, sizeof(item->format))) {
					continue;
				}
			}
			report_unexpected_token_error();
			return false;
		}
		debug(0, "}");
	} else if (match(TOKEN_SEMICOLON, NULL)) {
		debug(0, "item %s;", item->handle);
	} else {
		report_error("Missing '{' or ';'");
		return false;
	}
	append((void **)items, item);
	return true;
}

bool parse_property_block(device_type *device, property_type **properties) {
	if (!((match(TOKEN_IDENTIFIER, "text") || match(TOKEN_IDENTIFIER, "number") || match(TOKEN_IDENTIFIER, "switch") || match(TOKEN_IDENTIFIER, "light") || match(TOKEN_IDENTIFIER, "inherited")))) {
		return false;
	}
	property_type *property = allocate(sizeof(property_type));
	copy(property->type, sizeof(property->type));
	if (!match(TOKEN_IDENTIFIER, NULL)) {
		report_error("Missing property name");
		return false;
	}
	char id[64];
	copy(id, sizeof(id));
	snprintf(property->label, sizeof(property->label), "%s", id);
	make_upper_case(id);
	snprintf(property->id, sizeof(property->id), "%s", id);
	snprintf(property->handle, sizeof(property->handle), "%s_PROPERTY", id);
	snprintf(property->name, sizeof(property->name), "%s_PROPERTY_NAME", id);
	make_lower_case(id);
	snprintf(property->pointer, sizeof(property->pointer), "%s_property", id);
	if (strncmp(id, device->type, strlen(device->type)) != 0) {
		snprintf(property->handler, sizeof(property->handler), "%s_%s_handler", device->type, id);
	} else {
		snprintf(property->handler, sizeof(property->handler), "%s_handler", id);
	}
	strcpy(property->group, "MAIN_GROUP");
	strcpy(property->perm, "INDIGO_RW_PERM");
	strcpy(property->rule, "INDIGO_ONE_OF_MANY_RULE");
	property->handle_change = property->type[0] != 'l';
	property->asynchronous_change = true;
	if (match(TOKEN_LBRACE, NULL)) {
		debug(-1, "%s %s {", property->type, property->handle);
		while (!match(TOKEN_RBRACE, NULL)) {
			if (parse_expression_attribute("label", property->label, sizeof(property->label))) {
				continue;
			}
			if (parse_expression_attribute("handle", property->handle, sizeof(property->handle))) {
				continue;
			}
			if (parse_expression_attribute("name", id, sizeof(id))) {
				if (id[0] == '"') {
					strncpy(property->define_name, id + 1, strlen(id) - 2);
				} else {
					strcpy(property->name, id);
				}
				continue;
			}
			if (parse_expression_attribute("handler", property->handler, sizeof(property->handler))) {
				continue;
			}
			if (parse_bool_attribute("handle_change", &property->handle_change)) {
				continue;
			}
			if (parse_bool_attribute("asynchronous_change", &property->asynchronous_change)) {
				continue;
			}
			if (parse_bool_attribute("persistent", &property->persistent)) {
				continue;
			}
			if (parse_bool_attribute("preserve_values", &property->preserve_values)) {
				continue;
			}
			if (parse_code_block("code", &property->code)) {
				continue;
			}
			if (parse_code_block("on_attach", &property->on_attach)) {
				continue;
			}
			if (parse_code_block("on_change", &property->on_change)) {
				continue;
			}
			if (parse_code_block("on_detach", &property->on_detach)) {
				continue;
			}
			if (parse_expression_attribute("hidden", property->hidden, sizeof(property->hidden))) {
				continue;
			}
			if (parse_expression_attribute("perm", property->perm, sizeof(property->perm))) {
				if (strcmp(property->perm, "INDIGO_RO_PERM") == 0) {
					property->handle_change = false;
				}
				continue;
			}
			if (parse_expression_attribute("rule", property->rule, sizeof(property->rule))) {
				continue;
			}
			if (property->type[0] != 'i') {
				if (parse_expression_attribute("group", property->group, sizeof(property->group))) {
					continue;
				}
				if (parse_expression_attribute("pointer", property->pointer, sizeof(property->pointer))) {
					continue;
				}
				if (parse_bool_attribute("always_defined", &property->always_defined)) {
					continue;
				}
				if (parse_item_block(property->type, &property->items)) {
					continue;
				}
			}
			report_unexpected_token_error();
			return false;
		}
		debug(0, "}");
	} else if (match(TOKEN_SEMICOLON, NULL)) {
		debug(0, "%s %s;", property->type, property->handle);
	} else {
		report_error("Missing '{' or ';'");
		return false;
	}
	append((void **)properties, property);
	property->max_name_length = 30;
	int name_length = (int)strlen(property->handle);
	if (name_length > property->max_name_length) {
		property->max_name_length = name_length;
	}
	for (item_type *item = property->items; item; item = item->next) {
		if (*item->define_name) {
			name_length = (int)strlen(item->name);
			if (name_length > property->max_name_length) {
				property->max_name_length = name_length;
			}
		}
	}
	if (*property->define_name) {
		name_length = (int)strlen(property->name);
		if (name_length > property->max_name_length) {
			property->max_name_length = name_length;
		}
	}
	for (item_type *item = property->items; item; item = item->next) {
		name_length = (int)strlen(item->handle);
		if (name_length > property->max_name_length) {
			property->max_name_length = name_length;
		}
	}
	return true;
}

bool parse_device_block(driver_type *driver) {
	if (!(match(TOKEN_IDENTIFIER, "ccd") || match(TOKEN_IDENTIFIER, "wheel") || match(TOKEN_IDENTIFIER, "focuser") || match(TOKEN_IDENTIFIER, "mount") || match(TOKEN_IDENTIFIER, "guider") || match(TOKEN_IDENTIFIER, "rotator") || match(TOKEN_IDENTIFIER, "dome") || match(TOKEN_IDENTIFIER, "gps") || match(TOKEN_IDENTIFIER, "ao") || match(TOKEN_IDENTIFIER, "aux"))) {
		return false;
	}
	device_type *device = allocate(sizeof(device_type));
	char type[8];
	copy(type, sizeof(type));
	make_lower_case(type);
	snprintf(device->type, sizeof(device->type), "%s", type);
	make_upper_case(type);
	snprintf(device->handle, sizeof(device->handle), "%s_DEVICE_NAME", type);
	snprintf(device->interface, sizeof(device->interface), "0");
	if (driver->devices) {
		snprintf(device->name, sizeof(device->name), "%s (%s)", driver->label, device->type);
	} else {
		snprintf(device->name, sizeof(device->name), "%s", driver->label);
	}
	device->additional_instances = false;
	if (match(TOKEN_LBRACE, NULL)) {
		debug(-1, "%s {", device->type);
		while (!match(TOKEN_RBRACE, NULL)) {
			if (parse_expression_attribute("name", device->name, sizeof(device->name))) {
				continue;
			}
			if (parse_expression_attribute("interface", device->interface, sizeof(device->interface))) {
				continue;
			}
			if (parse_bool_attribute("additional_instances", &device->additional_instances)) {
				continue;
			}
			if (parse_property_block(device, &device->properties)) {
				continue;
			}
			if (parse_code_block("code", &device->code)) {
				continue;
			}
			if (parse_code_block("on_timer", &device->on_timer)) {
				continue;
			}
			if (parse_code_block("on_attach", &device->on_attach)) {
				continue;
			}
			if (parse_code_block("on_connect", &device->on_connect)) {
				continue;
			}
			if (parse_code_block("on_disconnect", &device->on_disconnect)) {
				continue;
			}
			if (parse_code_block("on_detach", &device->on_detach)) {
				continue;
			}
			report_unexpected_token_error();
			return false;
		}
		debug(0, "}");
	} else if (match(TOKEN_SEMICOLON, NULL)) {
		debug(0, "%s;", device->type);
	} else {
		report_error("Missing '{'");
		return false;
	}
	append((void **)&driver->devices, device);
	return true;
}

bool parse_libusb_block(driver_type *driver) {
	if (!match(TOKEN_IDENTIFIER, "libusb")) {
		return false;
	}
	libusb_type *libusb = allocate(sizeof(libusb_type));
	libusb->hotplug = true;
	if (match(TOKEN_LBRACE, NULL)) {
		debug(0, "pattern {");
		while (!match(TOKEN_RBRACE, NULL)) {
			if (parse_bool_attribute("hotplug", &libusb->hotplug)) {
				continue;
			}
			if (parse_expression_attribute("pid", libusb->pid, sizeof(libusb->pid))) {
				continue;
			}
			if (parse_expression_attribute("vid", libusb->vid, sizeof(libusb->vid))) {
				continue;
			}
			report_unexpected_token_error();
			return false;
		}
		debug(0, "}");
	} else if (match(TOKEN_SEMICOLON, NULL)) {
		debug(0, "libusb;");
	} else {
		report_error("Missing '{'");
		return false;
	}
	driver->libusb = libusb;
	return true;
}

bool parse_pattern_block(serial_type *serial) {
	if (!match(TOKEN_IDENTIFIER, "pattern")) {
		return false;
	}
	pattern_type *pattern = allocate(sizeof(pattern_type));
	if (match(TOKEN_LBRACE, NULL)) {
		debug(0, "pattern {");
		while (!match(TOKEN_RBRACE, NULL)) {
			if (parse_expression_attribute("pid", pattern->pid, sizeof(pattern->pid))) {
				continue;
			}
			if (parse_expression_attribute("vid", pattern->vid, sizeof(pattern->vid))) {
				continue;
			}
			if (parse_expression_attribute("exact_match", pattern->exact_match, sizeof(pattern->exact_match))) {
				continue;
			}
			if (parse_expression_attribute("product", pattern->product, sizeof(pattern->product))) {
				continue;
			}
			if (parse_expression_attribute("vendor", pattern->vendor, sizeof(pattern->vendor))) {
				continue;
			}
			if (parse_expression_attribute("serial", pattern->serial, sizeof(pattern->serial))) {
				continue;
			}
			report_unexpected_token_error();
			return false;
		}
		debug(0, "}");
	} else if (match(TOKEN_SEMICOLON, NULL)) {
		debug(0, "pattern;");
	} else {
		report_error("Missing '{'");
		return false;
	}
	append((void **)&serial->patterns, pattern);
	return true;
}

bool parse_serial_block(driver_type *driver) {
	if (!match(TOKEN_IDENTIFIER, "serial")) {
		return false;
	}
	serial_type *serial = (serial_type*)allocate(sizeof(serial_type));
	if (match(TOKEN_LBRACE, NULL)) {
		debug(-1, "serial {");
		while (!match(TOKEN_RBRACE, NULL)) {
			if (parse_bool_attribute("configurable_speed", &serial->configurable_speed)) {
				continue;
			}
			if (parse_pattern_block(serial)) {
				continue;
			}
			report_unexpected_token_error();
			return false;
		}
		debug(0, "}");
	} else if (match(TOKEN_SEMICOLON, NULL)) {
		debug(0, "serial;");
	} else {
		report_error("Missing '{' or ';'");
		return false;
	}
	driver->serial = serial;
	return true;
}

bool parse_driver_block(void) {
	if (!match(TOKEN_IDENTIFIER, "driver")) {
		report_error("Missing 'driver' keyword");
		return false;
	}
	if (!match(TOKEN_IDENTIFIER, NULL)) {
		report_error("Missing driver name");
		return false;
	}
	copy(driver.name, sizeof(driver.name));
	if (!match(TOKEN_LBRACE, NULL)) {
		report_error("Missing '{'");
		return false;
	}
	debug(-1, "driver %s {", driver.name);
	while (!match(TOKEN_RBRACE, NULL)) {
		if (parse_string_attribute("author", driver.author, sizeof(driver.author))) {
			continue;
		}
		if (parse_string_attribute("copyright", driver.copyright, sizeof(driver.copyright))) {
			continue;
		}
		if (parse_string_attribute("label", driver.label, sizeof(driver.label))) {
			continue;
		}
		if (parse_int_attribute("version", &driver.version)) {
			continue;
		}
		if (parse_serial_block(&driver)) {
			continue;
		}
		if (parse_libusb_block(&driver)) {
			continue;
		}
		if (parse_device_block(&driver)) {
			continue;
		}
		if (parse_code_block("include", &driver.include)) {
			continue;
		}
		if (parse_code_block("define", &driver.define)) {
			continue;
		}
		if (parse_code_block("data", &driver.data)) {
			continue;
		}
		if (parse_code_block("code", &driver.code)) {
			continue;
		}
		if (parse_code_block("on_init", &driver.on_init)) {
			continue;
		}
		if (parse_code_block("on_shutdown", &driver.on_shutdown)) {
			continue;
		}
		if (last_token == TOKEN_IDENTIFIER) {
			definition_type *definition = allocate(sizeof(definition_type));
			copy(definition->name, sizeof(definition->name));
			if (parse_expression_attribute(definition->name, definition->value, sizeof(definition->value))) {
				append((void **)&driver.definions, definition);
				continue;
			}
		}
		report_unexpected_token_error();
		return false;
	}
	debug(0, "}");
	if (driver.devices == NULL) {
		report_error("No device defined");
		return false;
	}
	if (driver.serial == NULL && driver.libusb == NULL) {
		driver.virtual = true;
	}
	return true;
}

#pragma mark - code generator

void read_definition_source(void) {
	int max_size = 16 * 1024;
	current = definition_source = allocate(max_size);
	int c;
	while ((c = getchar()) != -1) {
		*current++ = c;
		long size = current - definition_source;
		if (size == max_size - 1) {
			definition_source = realloc(definition_source, max_size *= 2);
			current = definition_source + size;
		}
	}
	*current = 0;
	current = definition_source;
}

void write_line(const char *format, ...) {
	if (*format == 0 && empty_line) {
		return;
	}
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	putchar('\n');
	va_end(args);
	empty_line = *format == 0;
}

bool c_code_starts_with(code_type *code, const char *format, ...) {
	if (code) {
		char buffer[1024];
		va_list args;
		va_start(args, format);
		vsnprintf(buffer, sizeof(buffer), format, args);
		va_end(args);
		char *c = code->text;
		while (isspace(*c) && (c - code->text < code->size)) {
			c++;
		}
		int s1 = code->size - (int)(c - code->text);
		int s2 = (int)strlen(buffer);
		int s = s1 < s2 ? s1 : s2;
		return strncmp(buffer, c, s) == 0;
	}
	return false;
}

void write_code_block(code_type *code, int indentation) {
	char *text = code->text;
	int size = code->size;
	char *pnt = text;
	int skip = 0;
	char c = *pnt++;
	while (isspace(c) && pnt - code->text < code->size) {
		c = *pnt++;
		skip++;
		if (c == '\n') {
			skip = 0;
			text = pnt;
			c = *pnt++;
		}
	}
	pnt = text;
	size = code->size - (int)(text - code->text);
	while (pnt - text < size) {
		char c = *pnt++;
		for (int i = 0; i < skip && pnt - text < size; i++) {
			if (!isspace(c) || c == '\n') {
				break;
			}
			c = *pnt++;
		}
		if (c != '\n') {
			if (indentation == 0) {
				if (c == '#' && !(strncmp(pnt, "define ", 7))) {
					char *name = pnt + 7;
					pnt = pnt + 7;
					while (*pnt && *pnt != ' ' && *pnt != '\t') {
						pnt++;
					}
					printf("#define %-20.*s ", (int)(pnt - name), name);
					while (*pnt && (*pnt == ' ' || *pnt == '\t')) {
						pnt++;
					}
					c = *pnt;
					pnt++;
				}
			} else {
				for (int i = 0; i < indentation; i++) {
					putchar('\t');
				}
			}
			putchar(c);
			while (pnt - text < size) {
				putchar(c = *pnt++);
				if (c == '\n') {
					break;
				}
			}
		} else {
			putchar(c);
		}
	}
	putchar('\n');
}

void write_c_code_blocks(code_type *code, int indentation, char *format, ...) {
	static char *spaces = "\t\t\t\t\t\t\t\t\t\t";
	char id[128];
	va_list args;
	va_start(args, format);
	vsnprintf(id, sizeof(id), format, args);
	va_end(args);
	if (code) {
		if (indentation == 0) {
			write_line("");
			write_line("%s//+ %s", spaces + 10 - indentation, id);
			write_line("");
		} else {
			write_line("%s//+ %s", spaces + 10 - indentation, id);
		}
		for (; code; code = code->next) {
			write_code_block(code, indentation);
		}
		if (indentation == 0) {
			empty_line = false;
			write_line("");
			write_line("%s//- %s", spaces + 10 - indentation, id);
			write_line("");
		} else {
			write_line("%s//- %s", spaces + 10 - indentation, id);
		}
	}
}

void write_license(void) {
	write_line("// %s", driver.copyright);
	write_line("// All rights reserved.");
	write_line("");
	write_line("// You may use this software under the terms of 'INDIGO Astronomy");
	write_line("// open-source license' (see LICENSE.md).");
	write_line("");
	write_line("// THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS");
	write_line("// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED");
	write_line("// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE");
	write_line("// ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY");
	write_line("// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL");
	write_line("// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE");
	write_line("// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS");
	write_line("// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,");
	write_line("// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING");
	write_line("// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS");
	write_line("// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.");
	write_line("");
	write_line("// This file generated from %s", definition_source_basename);
	write_line("");
}

void write_h_source(void) {
	write_license();
	write_line("");
	write_line("#ifndef %s_%s_h", driver.devices->type, driver.name);
	write_line("#define %s_%s_h", driver.devices->type, driver.name);
	write_line("");
	write_line("#include <indigo/indigo_driver.h>");
	write_line("");
	write_line("#if defined(INDIGO_WINDOWS)");
	write_line("#if defined(INDIGO_WINDOWS_DLL)");
	write_line("#define INDIGO_EXTERN __declspec(dllexport)");
	write_line("#else");
	write_line("#define INDIGO_EXTERN __declspec(dllimport)");
	write_line("#endif");
	write_line("#else");
	write_line("#define INDIGO_EXTERN extern");
	write_line("#endif");
	write_line("");
	write_line("#ifdef __cplusplus");
	write_line("extern \"C\" {");
	write_line("#endif");
	write_line("");
	write_line("INDIGO_EXTERN indigo_result indigo_%s_%s(indigo_driver_action action, indigo_driver_info *info);", driver.devices->type, driver.name);
	write_line("");
	write_line("#ifdef __cplusplus");
	write_line("}");
	write_line("#endif");
	write_line("");
	write_line("#endif");
}

void write_c_main_source(void) {
	write_license();
	write_line("");
	write_line("#include <indigo/indigo_driver_xml.h>");
	write_line("");
	write_line("#include \"indigo_%s_%s.h\"", driver.devices->type, driver.name);
	write_line("");
	write_line("int main(int argc, const char * argv[]) {");
	write_line("\tindigo_main_argc = argc;");
	write_line("\tindigo_main_argv = argv;");
	write_line("\tindigo_client *protocol_adapter = indigo_xml_device_adapter(&indigo_stdin_handle, &indigo_stdout_handle);");
	write_line("\tindigo_start();");
	write_line("\tindigo_%s_%s(INDIGO_DRIVER_INIT, NULL);", driver.devices->type, driver.name);
	write_line("\tindigo_attach_client(protocol_adapter);");
	write_line("\tindigo_xml_parse(NULL, protocol_adapter);");
	write_line("\tindigo_%s_%s(INDIGO_DRIVER_SHUTDOWN, NULL);", driver.devices->type, driver.name);
	write_line("\tindigo_stop();");
	write_line("\treturn 0;");
	write_line("}");
}

void write_c_include_section(void) {
	write_line("");
	write_line("#pragma mark - Includes");
	write_line("");
	write_line("#include <stdlib.h>");
	write_line("#include <string.h>");
	write_line("#include <math.h>");
	write_line("#include <assert.h>");
	write_line("#include <pthread.h>");
	write_c_code_blocks(driver.include, 0, "include");
	write_line("#include <indigo/indigo_driver_xml.h>");
	for (device_type *device = driver.devices; device; device = device->next) {
		write_line("#include <indigo/indigo_%s_driver.h>", device->type);
	}
	write_line("#include <indigo/indigo_uni_io.h>");
	if (driver.libusb) {
		write_line("#include <indigo/indigo_usb_utils.h>");
	}
	write_line("");
	write_line("#include \"indigo_%s_%s.h\"", driver.devices->type, driver.name);
	write_line("");
}

void write_c_define_section(void) {
	write_line("");
	write_line("#pragma mark - Common definitions");
	write_line("");
	write_line("#define %-20s 0x%08X", "DRIVER_VERSION", 0x03000000 + driver.version);
	write_line("#define %-20s \"indigo_%s_%s\"", "DRIVER_NAME", driver.devices->type, driver.name);
	write_line("#define %-20s \"%s\"", "DRIVER_LABEL", driver.label);
	for (device_type *device = driver.devices; device; device = device->next) {
		write_line("#define %-20s %s", device->handle, device->name);
	}
	if (driver.libusb) {
		write_line("#define %-20s 5", "MAX_DEVICES");
	}
	write_line("#define %-20s ((%s_private_data *)device->private_data)", "PRIVATE_DATA", driver.name);
	if (driver.definions) {
		write_line("//+ definitions");
		for (definition_type *definiton = driver.definions; definiton; definiton = definiton->next) {
			write_line("#define %-20s %s", definiton->name, definiton->value);
		}
		write_line("//- definitions");
	}
	write_c_code_blocks(driver.define, 0, "define");
}

void write_c_property_definition_section(void) {
	bool first_one = true;
	for (device_type *device = driver.devices; device; device = device->next) {
		for (property_type *property = device->properties; property; property = property->next) {
			if (property->type[0] != 'i') {
				if (first_one) {
					write_line("");
					write_line("#pragma mark - Property definitions");
					write_line("");
					first_one = false;
				}
//				write_line("// %s handles definition", property->id);
				write_line("#define %-*s (PRIVATE_DATA->%s)", property->max_name_length, property->handle, property->pointer);
				int index = 0;
				for (item_type *item = property->items; item; item = item->next) {
					write_line("#define %-*s (%s->items + %d)", property->max_name_length, item->handle, property->handle, index++);
				}
				write_line("");
				if (*property->define_name) {
					write_line("#define %-*s \"%s\"", property->max_name_length, property->name, property->define_name);
				}
				for (item_type *item = property->items; item; item = item->next) {
					if (*item->define_name) {
						write_line("#define %-*s \"%s\"", property->max_name_length, item->name, item->define_name);
					}
				}
				write_line("");
			}
		}
	}
}

void write_c_private_data_section(void) {
	bool is_multi_device = driver.devices != NULL && driver.devices->next != NULL;
	write_line("");
	write_line("#pragma mark - Private data definition");
	write_line("");
	write_line("typedef struct {");
	if (is_multi_device) {
		write_line("\tint count;");
	}
	if (driver.serial) {
		write_line("\tindigo_uni_handle *handle;");
	} else if (driver.libusb) {
		write_line("\tlibusb_device *usbdev;");
	}
	for (device_type *device = driver.devices; device; device = device->next) {
		for (property_type *property = device->properties; property; property = property->next) {
			if (property->type[0] != 'i') {
				write_line("\tindigo_property *%s;", property->pointer);
			}
		}
	}
	write_c_code_blocks(driver.data, 1, "data");
	write_line("} %s_private_data;", driver.name);
	write_line("");
}

void write_c_low_level_code_section(void) {
	if (driver.code) {
		write_line("");
		write_line("#pragma mark - Low level code");
		write_line("");
		write_c_code_blocks(driver.code, 0, "code");
		for (device_type *device = driver.devices; device; device = device->next) {
			write_c_code_blocks(device->code, 0, "%s.code", device->type);
			for (property_type *property = device->properties; property; property = property->next) {
				write_c_code_blocks(property->code, 0, "%s.%s.code", device->type, property->id);
			}
		}
	}
}

void write_c_timer_callback(device_type *device) {
//	write_line("");
//	write_line("// %s state checking timer callback", device->type);
	write_line("");
	write_line("static void %s_timer_callback(indigo_device *device) {", device->type);
	write_line("\tif (!IS_CONNECTED) {");
	write_line("\t\treturn;");
	write_line("\t}");
	write_c_code_blocks(device->on_timer, 1, "%s.on_timer", device->type);
	write_line("}");
	write_line("");
}

void write_c_connection_change_handler(device_type *device) {
	bool is_multi_device = driver.devices != NULL && driver.devices->next != NULL;
	bool is_master_device = device == driver.devices;
//	write_line("");
//	write_line("// CONNECTION change handler");
	write_line("");
	write_line("static void %s_connection_handler(indigo_device *device) {", device->type);
	write_line("\tif (CONNECTION_CONNECTED_ITEM->sw.value) {");
	if (driver.virtual) {
		write_c_code_blocks(device->on_connect, 2, "%s.on_connect", device->type);
		for (property_type *property2 = device->properties; property2; property2 = property2->next) {
			if (property2->type[0] != 'i' && !property2->always_defined) {
				write_line("\t\t\tindigo_define_property(device, %s, NULL);", property2->handle);
			}
		}
		if (device->on_timer != NULL) {
			write_line("\t\tindigo_execute_handler(device, %s_timer_callback);", device->type);
		}
		write_line("\t\tCONNECTION_PROPERTY->state = INDIGO_OK_STATE;");
		write_line("\t\tindigo_send_message(device, \"Connected to %%s\", device->name);");
	} else {
		write_line("\t\tbool connection_result = true;");
		if (is_multi_device) {
			write_line("\t\tif (PRIVATE_DATA->count++ == 0) {");
			if (is_master_device) {
				write_line("\t\t\tconnection_result = %s_open(device);", driver.name);
			} else {
				write_line("\t\t\tconnection_result = %s_open(device->master_device);", driver.name);
			}
			write_line("\t\t}");
		} else {
			if (is_master_device) {
				write_line("\t\tconnection_result = %s_open(device);", driver.name);
			} else {
				write_line("\t\tconnection_result = %s_open(device->master_device);", driver.name);
			}
		}
		write_line("\t\tif (connection_result) {");
		write_c_code_blocks(device->on_connect, 3, "%s.on_connect", device->type);
		for (property_type *property2 = device->properties; property2; property2 = property2->next) {
			if (property2->type[0] != 'i' && !property2->always_defined) {
				write_line("\t\t\tindigo_define_property(device, %s, NULL);", property2->handle);
			}
		}
		if (device->on_timer != NULL) {
			write_line("\t\t\tindigo_execute_handler(device, %s_timer_callback);", device->type);
		}
		write_line("\t\t\tCONNECTION_PROPERTY->state = INDIGO_OK_STATE;");
		if (driver.serial) {
			write_line("\t\t\tindigo_send_message(device, \"Connected to %%s on %%s\", %s, DEVICE_PORT_ITEM->text.value);", device->handle);
		} else {
			write_line("\t\t\tindigo_send_message(device, \"Connected to %%s\", device->name);");
		}
		write_line("\t\t} else {");
		if (driver.serial) {
			write_line("\t\t\tindigo_send_message(device, \"Failed to connect to %%s on %%s\", %s, DEVICE_PORT_ITEM->text.value);", device->handle);
		} else {
			write_line("\t\t\tindigo_send_message(device, \"Failed to connect to %%s\", device->name);");
		}
		if (is_multi_device) {
			write_line("\t\t\tPRIVATE_DATA->count--;");
		}
		write_line("\t\t\tCONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;");
		write_line("\t\t\tindigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);");
		write_line("\t\t}");
	}
	write_line("\t} else {");
	for (property_type *property2 = device->properties; property2; property2 = property2->next) {
		if (property2->type[0] != 'i' && !property2->always_defined) {
			write_line("\t\tindigo_delete_property(device, %s, NULL);", property2->handle);
		}
	}
	write_c_code_blocks(device->on_disconnect, 2, "%s.on_disconnect", device->type);
	if (!driver.virtual) {
		if (is_multi_device) {
			write_line("\t\tif (--PRIVATE_DATA->count == 0) {");
			write_line("\t\t\t%s_close(device);", driver.name);
			write_line("\t\t}");
		} else {
			write_line("\t\t%s_close(device);", driver.name);
		}
	}
	write_line("\t\tindigo_send_message(device, \"Disconnected from %%s\", device->name);");
	write_line("\t\tCONNECTION_PROPERTY->state = INDIGO_OK_STATE;");
	write_line("\t}");
	write_line("\tindigo_%s_change_property(device, NULL, CONNECTION_PROPERTY);", device->type);
	write_line("}");
	write_line("");
}

void write_c_property_change_handler(device_type *device, property_type *property) {
//	write_line("");
//	write_line("// %s change handler", property->id);
	write_line("");
	write_line("static void %s(indigo_device *device) {", property->handler);
	if (!c_code_starts_with(property->on_change, "%s->state = ", property->handle)) {
		write_line("\t%s->state = INDIGO_OK_STATE;", property->handle);
	}
	write_c_code_blocks(property->on_change, 1, "%s.%s.on_change", device->type, property->id);
	write_line("\tindigo_update_property(device, %s, NULL);", property->handle);
	write_line("}");
	write_line("");
}

void write_c_high_level_code_section(device_type *device) {
	write_line("");
	write_line("#pragma mark - High level code (%s)", device->type);
	write_line("");
	if (device->on_timer != NULL) {
		write_c_timer_callback(device);
	}
	write_c_connection_change_handler(device);
	for (property_type *property = device->properties; property; property = property->next) {
		if (property->handle_change) {
			write_c_property_change_handler(device, property);
		}
	}
}

void write_c_attach(device_type *device) {
	bool is_master_device = device == driver.devices;
//	write_line("");
//	write_line("// %s attach API callback", device->type);
	write_line("");
	write_line("static indigo_result %s_attach(indigo_device *device) {", device->type);
	if (strcmp(device->type, "aux") == 0) {
		write_line("\tif (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, %s) == INDIGO_OK) {", device->interface);
	} else {
		write_line("\tif (indigo_%s_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {", device->type);
	}
	if (device->additional_instances) {
		write_line("\t\tADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;");
	}
	if (driver.serial && is_master_device) {
		write_line("\t\tDEVICE_PORT_PROPERTY->hidden = false;");
		write_line("\t\tDEVICE_PORTS_PROPERTY->hidden = false;");
		write_line("\t\tindigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);");
		if (driver.serial->configurable_speed) {
			write_line("\t\tDEVICE_BAUDRATE_PROPERTY->hidden = false;");
		}
	}
	write_c_code_blocks(device->on_attach, 2, "%s.on_attach", device->type);
	for (property_type *property = device->properties; property; property = property->next) {
		if (property->type[0] != 'i') {
			int count = 0;
			for (item_type *item = property->items; item; item = item->next) {
				count++;
			}
			switch (property->type[0]) {
				case 't':
					write_line("\t\t%s = indigo_init_text_property(NULL, device->name, %s, %s, %s, INDIGO_OK_STATE, %s, %d);", property->handle, property->name, property->group, property->label, property->perm, count);
					write_line("\t\tif (%s == NULL) {", property->handle);
					write_line("\t\t\treturn INDIGO_FAILED;");
					write_line("\t\t}");
					for (item_type *item = property->items; item; item = item->next) {
						write_line("\t\tindigo_init_text_item(%s, %s, %s, %s);", item->handle, item->name, item->label, item->value);
					}
					break;
				case 'n':
					write_line("\t\t%s = indigo_init_number_property(NULL, device->name, %s, %s, %s, INDIGO_OK_STATE, %s, %d);", property->handle, property->name, property->group, property->label, property->perm, count);
					write_line("\t\tif (%s == NULL) {", property->handle);
					write_line("\t\t\treturn INDIGO_FAILED;");
					write_line("\t\t}");
					for (item_type *item = property->items; item; item = item->next) {
						write_line("\t\tindigo_init_number_item(%s, %s, %s, %s, %s, %s, %s);", item->handle, item->name, item->label, item->min, item->max, item->step, item->value);
						if (*item->format) {
							write_line("\t\tstrcpy(%s->number.format, %s);", item->handle, item->format);
						}
					}
					break;
				case 's':
					write_line("\t\t%s = indigo_init_switch_property(NULL, device->name, %s, %s, %s, INDIGO_OK_STATE, %s, %s, %d);", property->handle, property->name, property->group, property->label, property->perm, property->rule, count);
					write_line("\t\tif (%s == NULL) {", property->handle);
					write_line("\t\t\treturn INDIGO_FAILED;");
					write_line("\t\t}");
					for (item_type *item = property->items; item; item = item->next) {
						write_line("\t\tindigo_init_switch_item(%s, %s, %s, %s);", item->handle, item->name, item->label, item->value);
					}
					break;
				case 'l':
					write_line("\t\t%s = indigo_init_light_property(NULL, device->name, %s, %s, %s, INDIGO_OK_STATE, %d);", property->handle, property->name, property->group, property->label, count);
					write_line("\t\tif (%s == NULL) {", property->handle);
					write_line("\t\t\treturn INDIGO_FAILED;");
					write_line("\t\t}");
					for (item_type *item = property->items; item; item = item->next) {
						write_line("\t\tindigo_init_light_item(%s, %s, %s, %s);", item->handle, item->name, item->label, item->value);
					}
					break;
			}
		}
		if (property->type[0] == 'i') {
			if (*property->hidden) {
				write_line("\t\t%s->hidden = %s;", property->handle, property->hidden);
			} else {
				write_line("\t\t%s->hidden = false;", property->handle);
			}
		} else {
			if (*property->hidden && strcmp(property->hidden, "true") == 0) {
				write_line("\t\t%s->hidden = true;", property->handle);
			}
		}
		write_c_code_blocks(property->on_attach, 2, "%s.%s.on_attach", device->type, property->id);
	}
	write_line("\t\tINDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);");
	write_line("\t\treturn %s_enumerate_properties(device, NULL, NULL);", device->type);
	write_line("\t}");
	write_line("\treturn INDIGO_FAILED;");
	write_line("}");
	write_line("");}

void write_c_enumerate(device_type *device) {
//	write_line("");
//	write_line("// %s enumerate API callback", device->type);
	write_line("");
	write_line("static indigo_result %s_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {", device->type);
	bool first_one = true;
	for (property_type *property = device->properties; property; property = property->next) {
		if (property->type[0] != 'i') {
			if (!property->always_defined) {
				if (first_one) {
					write_line("\tif (IS_CONNECTED) {");
					first_one = false;
				}
				write_line("\t\tINDIGO_DEFINE_MATCHING_PROPERTY(%s);", property->handle);
			}
		}
	}
	if (!first_one) {
		write_line("\t}");
	}
	for (property_type *property = device->properties; property; property = property->next) {
		if (property->type[0] != 'i') {
			if (property->always_defined) {
				write_line("\tINDIGO_DEFINE_MATCHING_PROPERTY(%s);", property->handle);
			}
		}
	}
	write_line("\treturn indigo_%s_enumerate_properties(device, NULL, NULL);", device->type);
	write_line("}");
	write_line("");
}

void write_c_change_property(device_type *device) {
//	write_line("");
//	write_line("// %s change property API callback", device->type);
	write_line("");
	write_line("static indigo_result %s_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {", device->type);
	bool persistent = false;
	write_line("\tif (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {");
	write_line("\t\tif (!indigo_ignore_connection_change(device, property)) {");
	write_line("\t\t\tindigo_property_copy_values(CONNECTION_PROPERTY, property, false);");
	write_line("\t\t\tCONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;");
	write_line("\t\t\tindigo_update_property(device, CONNECTION_PROPERTY, NULL);");
	write_line("\t\t\tindigo_execute_handler(device, %s_connection_handler);", device->type);
	write_line("\t\t}");
	write_line("\t\treturn INDIGO_OK;");
	for (property_type *property = device->properties; property; property = property->next) {
		if (property->handle_change) {
			persistent |= property->persistent;
			write_line("\t} else if (indigo_property_match_changeable(%s, property)) {", property->handle);
			if (property->asynchronous_change) {
				if (property->preserve_values) {
					write_line("\t\tINDIGO_COPY_TARGETS_PROCESS_CHANGE(%s, %s);", property->handle, property->handler);
				} else {
					write_line("\t\tINDIGO_COPY_VALUES_PROCESS_CHANGE(%s, %s);", property->handle, property->handler);
				}
			} else {
				if (property->preserve_values) {
					write_line("\t\tINDIGO_COPY_TARGETS_PROCESS_SYNC_CHANGE(%s, %s);", property->handle, property->handler);
				} else {
					write_line("\t\tINDIGO_COPY_VALUES_PROCESS_SYNC_CHANGE(%s, %s);", property->handle, property->handler);
				}
			}
			write_line("\t\treturn INDIGO_OK;");
		}
	}
	if (persistent) {
		write_line("\t} else if (indigo_property_match_changeable(CONFIG_PROPERTY, property)) {");
		write_line("\t\tif (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {");
		for (property_type *property = device->properties; property; property = property->next) {
			if (property->persistent) {
				write_line("\t\t\tindigo_save_property(device, NULL, %s);", property->handle);
			}
		}
		write_line("\t\t}");
	}
	write_line("\t}");
	write_line("\treturn indigo_%s_change_property(device, client, property);", device->type);
	write_line("}");
	write_line("");

}

void write_c_detach(device_type *device) {
//	write_line("");
//	write_line("// %s detach API callback", device->type);
	write_line("");
	write_line("static indigo_result %s_detach(indigo_device *device) {", device->type);
	write_line("\tif (IS_CONNECTED) {");
	write_line("\t\tindigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);");
	write_line("\t\t%s_connection_handler(device);", device->type);
	write_line("\t}");
	write_c_code_blocks(device->on_detach, 1, "%s.on_detach", device->type);
	for (property_type *property = device->properties; property; property = property->next) {
		write_c_code_blocks(property->on_detach, 1, "%s.%s.on_attach", device->type, property->id);
		if (property->type[0] != 'i') {
			write_line("\tindigo_release_property(%s);", property->handle);
		}
	}
	write_line("\tINDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);");
	write_line("\treturn indigo_%s_detach(device);", device->type);
	write_line("}");
	write_line("");
}

void write_c_device_api_section(device_type *device) {
	write_line("");
	write_line("#pragma mark - Device API (%s)", device->type);
	write_line("");
	write_line("static indigo_result %s_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);", device->type);
	write_line("");
	write_c_attach(device);
	write_c_enumerate(device);
	write_c_change_property(device);
	write_c_detach(device);
}

void write_c_device_templates_section(void) {
	write_line("");
	write_line("#pragma mark - Device templates");
	write_line("");
	for (device_type *device = driver.devices; device; device = device->next) {
		write_line("static indigo_device %s_template = INDIGO_DEVICE_INITIALIZER(%s, %s_attach, %s_enumerate_properties, %s_change_property, NULL, %s_detach);", device->type, device->handle, device->type, device->type, device->type, device->type);
		write_line("");
	}
	write_line("");
}

void write_c_hotplug_section(void) {
	write_line("");
	write_line("#pragma mark - Hot-plug code");
	write_line("");
	write_line("static indigo_device *devices[MAX_DEVICES];");
	write_line("static pthread_mutex_t hotplug_mutex = PTHREAD_MUTEX_INITIALIZER;");
	write_line("");	
	write_line("static void process_plug_event(libusb_device *dev) {");
	write_line("\tconst char *name;");
	write_line("\tpthread_mutex_lock(&hotplug_mutex);");
	write_line("\tif (%s_match(dev, &name)) {", driver.name);
	write_line("\t\t%s_private_data *private_data = indigo_safe_malloc(sizeof(%s_private_data));", driver.name, driver.name);
	write_line("\t\tprivate_data->usbdev = dev;");
	write_line("\t\tlibusb_ref_device(dev);");
	for (device_type *device = driver.devices; device; device = device->next) {
		write_line("\t\t\tindigo_device *%s = indigo_safe_malloc_copy(sizeof(indigo_device), &%s_template);", device->type, device->type);
		write_line("\t\t\t%s->private_data = private_data;", device->type);
		if (device != driver.devices) {
			write_line("\t\t\t%s->master_device = %s;", device->type, driver.devices->type);
		}
		write_line("\t\t\tsnprintf(%s->name, INDIGO_NAME_SIZE, %s, name);", device->type, device->name);
		write_line("\t\t\tfor (int j = 0; j < MAX_DEVICES; j++) {");
		write_line("\t\t\t\tif (devices[j] == NULL) {");
		write_line("\t\t\t\t\tindigo_async((void *)(void *)indigo_attach_device, devices[j] = %s);", device->type);
		write_line("\t\t\t\t\tbreak;");
		write_line("\t\t\t\t}");
		write_line("\t\t\t}");
	}
	write_line("\t}");
	write_line("\tpthread_mutex_unlock(&hotplug_mutex);");
	write_line("}");
	write_line("");
	write_line("static void process_unplug_event(libusb_device *dev) {");
	write_line("\tpthread_mutex_lock(&hotplug_mutex);");
	write_line("\t%s_private_data *private_data = NULL;", driver.name);
	write_line("\tfor (int j = 0; j < MAX_DEVICES; j++) {");
	write_line("\t\tif (devices[j] != NULL) {");
	write_line("\t\t\tindigo_device *device = devices[j];");
	write_line("\t\t\tif (PRIVATE_DATA->usbdev == dev) {");
	write_line("\t\t\t\tprivate_data = PRIVATE_DATA;");
	write_line("\t\t\t\tindigo_detach_device(device);");
	write_line("\t\t\t\tfree(device);");
	write_line("\t\t\t\tdevices[j] = NULL;");
	write_line("\t\t\t}");
	write_line("\t\t}");
	write_line("\t}");
	write_line("\tif (private_data != NULL) {");
	write_line("\t\tlibusb_unref_device(dev);");
	write_line("\t\tfree(private_data);");
	write_line("\t}");
	write_line("\tpthread_mutex_unlock(&hotplug_mutex);");
	write_line("}");
	write_line("");
	write_line("static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {");
	write_line("\tswitch (event) {");
	write_line("\t\tcase LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {");
	write_line("\t\t\tINDIGO_ASYNC(process_plug_event, dev);");
	write_line("\t\t\tbreak;");
	write_line("\t\t}");
	write_line("\t\tcase LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: {");
	write_line("\t\t\tprocess_unplug_event(dev);");
	write_line("\t\t\tbreak;");
	write_line("\t\t}");
	write_line("\t}");
	write_line("\treturn 0;");
	write_line("}");
	write_line("");
	write_line("static libusb_hotplug_callback_handle callback_handle;");
	write_line("");
}

void write_c_main_section(void) {
	write_line("");
	write_line("#pragma mark - Main code");
//	write_line("");
//	write_line("// %s driver entry point", driver.label);
	write_line("");
	write_line("indigo_result indigo_%s_%s(indigo_driver_action action, indigo_driver_info *info) {", driver.devices->type, driver.name);
	write_line("\tstatic indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;");
	if (driver.virtual || driver.serial) {
		write_line("\tstatic %s_private_data *private_data = NULL;", driver.name);
		for (device_type *device = driver.devices; device; device = device->next) {
			write_line("\tstatic indigo_device *%s = NULL;", device->type);
		}
	}
	write_line("");
	device_type *master_device = driver.devices;
	write_line("\tSET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);");
	write_line("");
	write_line("\tif (action == last_action) {");
	write_line("\t\treturn INDIGO_OK;");
	write_line("\t}");
	write_line("");
	write_line("\tswitch (action) {");
	write_line("\t\tcase INDIGO_DRIVER_INIT:");
	write_line("\t\t\tlast_action = action;");
	write_c_code_blocks(driver.on_init, 3, "on_init");
	if (driver.virtual || driver.serial) {
		if (driver.serial && driver.serial->patterns) {
			int index = 0;
			for (pattern_type *pattern = driver.serial->patterns; pattern; pattern = pattern->next) {
				index++;
			}
			write_line("\t\t\tstatic indigo_device_match_pattern patterns[%d] = { 0 };", index);
			index = 0;
			for (pattern_type *pattern = driver.serial->patterns; pattern; pattern = pattern->next) {
				if (*pattern->product) {
					write_line("\t\t\tstrcpy(patterns[%d].product_string, %s);", index, pattern->product);
				}
				if (*pattern->vendor) {
					write_line("\t\t\tstrcpy(patterns[%d].vendor_string, %s);", index, pattern->vendor);
				}
				if (*pattern->serial) {
					write_line("\t\t\tstrcpy(patterns[%d].serial_string, %s);", index, pattern->serial);
				}
				if (*pattern->vid) {
					write_line("\t\t\tpatterns[%d].vendor_id = %s;", index, pattern->vid);
				}
				if (*pattern->pid) {
					write_line("\t\t\tpatterns[%d].product_id = %s;", index, pattern->pid);
				}
				if (*pattern->exact_match) {
					write_line("\t\t\tpatterns[%d].exact_match = %s;", index, pattern->exact_match);
				}
				index++;
			}
			write_line("\t\t\tINDIGO_REGISER_MATCH_PATTERNS(%s_template, patterns, %d);", master_device->type, index);
		}
		write_line("\t\t\tprivate_data = indigo_safe_malloc(sizeof(%s_private_data));", driver.name);
		for (device_type *device = driver.devices; device; device = device->next) {
			write_line("\t\t\t%s = indigo_safe_malloc_copy(sizeof(indigo_device), &%s_template);", device->type, device->type);
			write_line("\t\t\t%s->private_data = private_data;", device->type);
			if (device != driver.devices) {
				write_line("\t\t\t%s->master_device = %s;", device->type, driver.devices->type);
			}
			write_line("\t\t\tindigo_attach_device(%s);", device->type);
		}
	} else if (driver.libusb) {
		if (driver.libusb->hotplug) {
			write_line("\t\t\tfor (int i = 0; i < MAX_DEVICES; i++) {");
			write_line("\t\t\t\tdevices[i] = 0;");
			write_line("\t\t\t}");
			write_line("\t\t\tindigo_start_usb_event_handler();");
			write_line("\t\t\tint rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, %s, %s, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);", *driver.libusb->vid ? driver.libusb->vid : "LIBUSB_HOTPLUG_MATCH_ANY", *driver.libusb->pid ? driver.libusb->pid : "LIBUSB_HOTPLUG_MATCH_ANY");
			write_line("\t\t\tINDIGO_DRIVER_DEBUG(DRIVER_NAME, \"libusb_hotplug_register_callback ->  %%s\", rc < 0 ? libusb_error_name(rc) : \"OK\");");
		} else {
			// TBD
		}
	}
	write_line("\t\t\tbreak;");
	write_line("");
	write_line("\t\tcase INDIGO_DRIVER_SHUTDOWN:");
	if (driver.virtual || driver.serial) {
		for (device_type *device = driver.devices; device; device = device->next) {
			write_line("\t\t\tVERIFY_NOT_CONNECTED(%s);", device->type);
		}
		write_line("\t\t\tlast_action = action;");
		for (device_type *device = driver.devices; device; device = device->next) {
			write_line("\t\t\tif (%s != NULL) {", device->type);
			write_line("\t\t\t\tindigo_detach_device(%s);", device->type);
			write_line("\t\t\t\tfree(%s);", device->type);
			write_line("\t\t\t\t%s = NULL;", device->type);
			write_line("\t\t\t}", device->type);
		}
		write_line("\t\t\tif (private_data != NULL) {");
		write_line("\t\t\t\tfree(private_data);");
		write_line("\t\t\t\tprivate_data = NULL;");
		write_line("\t\t\t}");
	} else if (driver.libusb) {
		if (driver.libusb->hotplug) {
			write_line("\t\t\tfor (int i = 0; i < MAX_DEVICES; i++) {");
			write_line("\t\t\t\tVERIFY_NOT_CONNECTED(devices[i]);");
			write_line("\t\t\t}");
			write_line("\t\t\tlast_action = action;");
			write_line("\t\t\tlibusb_hotplug_deregister_callback(NULL, callback_handle);");
			write_line("\t\t\tINDIGO_DRIVER_DEBUG(DRIVER_NAME, \"libusb_hotplug_deregister_callback\");");
			write_line("\t\t\tfor (int i = 0; i < MAX_DEVICES; i++) {");
			write_line("\t\t\t\tif (devices[i] != NULL) {");
			write_line("\t\t\t\t\tindigo_device *device = devices[i];");
			write_line("\t\t\t\t\thotplug_callback(NULL, PRIVATE_DATA->usbdev, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, NULL);");
			write_line("\t\t\t\t}");
			write_line("\t\t\t}");
		} else {
			// TBD
		}
	}
	write_c_code_blocks(driver.on_shutdown, 2, "on_shutdown");
	write_line("\t\t\tbreak;");
	write_line("");
	write_line("\t\tcase INDIGO_DRIVER_INFO:");
	write_line("\t\t\tbreak;");
	write_line("\t}");
	write_line("");
	write_line("\treturn INDIGO_OK;");
	write_line("}");
}

void write_c_source(void) {
	write_license();
	if (todos) {
		for (code_type *todo = todos; todo; todo = todo->next) {
			write_line("%.*s", todo->size, todo->text);
		}
		write_line("");
	}
	write_c_include_section();
	write_c_define_section();
	write_c_property_definition_section();
	write_c_private_data_section();
	write_c_low_level_code_section();
	for (device_type *device = driver.devices; device; device = device->next) {
		write_c_high_level_code_section(device);
		write_c_device_api_section(device);
	}
	write_c_device_templates_section();
	if (driver.libusb && driver.libusb->hotplug) {
		write_c_hotplug_section();
	}
	write_c_main_section();
}

#pragma mark - parse c code

device_type *get_device(char *type) {
	for (device_type *device = driver.devices; device; device = device->next) {
		if (strcmp(device->type, type) == 0) {
			return device;
		}
	}
	device_type *device = allocate(sizeof(device_type));
	strncpy(device->type, type, sizeof(device->type));
	append((void **)&driver.devices, device);
	return device;
}

property_type *get_property(device_type *device, char *property_handle) {
	for (property_type *property = device->properties; property; property = property->next) {
		if (strcmp(property->handle, property_handle) == 0) {
			return property;
		}
	}
	property_type*property = allocate(sizeof(property_type));
	strncpy(property->handle, property_handle, sizeof(property->id));
	int l = (int)strlen(property_handle);
	if (l > 9 && strncmp(property_handle + l - 9, "_PROPERTY", 9) == 0) {
		property_handle[l - 9] = 0;
		strncpy(property->id, property_handle, sizeof(property->id));
	}
	if (*property->type == 0) {
		strncpy(property->type, "inherited", sizeof(property->type));
	}
	append((void **)&device->properties, property);
	return property;
}

item_type *get_item(property_type *property, char *item_handle) {
	for (item_type *item = property->items; item; item = item->next) {
		if (strcmp(item->handle, item_handle) == 0) {
			return item;
		}
	}
	item_type *item = allocate(sizeof(item_type));
	int l = (int)strlen(item_handle);
	strncpy(item->handle, item_handle, sizeof(item->handle));
	if (l > 5 && strncmp(item_handle + l - 5, "_ITEM", 5) == 0) {
		item_handle[l - 5] = 0;
		strncpy(item->id, item_handle, sizeof(item->id));
	}
	append((void **)&property->items, item);
	return item;
}

char *translate_name(char *name) {
	for (definition_type *def = defs; def; def = def->next) {
		if (strcmp(def->name, name) == 0) {
			return def->value;
		}
	}
	return name;
}

void read_c_source(void) {
	char line[1024];
	char *s0, s1[128], s2[128], s3[128], s4[128], s5[128], s6[128], s7[128];
	int i1, i2;
	double d1;
	while (fgets(line, sizeof(line), stdin)) {
		line[strcspn(line, "\n")] = 0;
		if ((s0 = strstr(line, "//* "))) {
			memmove(s0, s0 + 4, strlen(s0 + 4) + 1);
		}
		if (strncmp(line, "// ", 3) == 0 || strncmp(line, "//\n", 3) == 0) {
			if (strncmp(line, "// TODO:", 8) == 0) {
				code_type *todo = allocate(sizeof(code_type));
				todo->text = strdup(line);
				append((void **)&todos, todo);
			}
			if (sscanf(line, "// Copyright (c) %d - %d %127[^\0]", &i1, &i2, s1) == 3) {
				snprintf(driver.copyright, sizeof(driver.copyright), "Copyright (c) %d-2025 %s", i1, s1);
			} else if (sscanf(line, "// Copyright (c) %d %127[^\0]", &i1, s1) == 2) {
				if (i1 < 2025) {
					snprintf(driver.copyright, sizeof(driver.copyright), "Copyright (c) %d-2025 %s", i1, s1);
				} else {
					snprintf(driver.copyright, sizeof(driver.copyright), "Copyright (c) 2025 %s", s1);
				}
			} else if (sscanf(line, "// %lf by %127[^\0]", &d1, s1) == 2) {
				strncpy(driver.author, s1, sizeof(driver.author));
			} else if (sscanf(line, "// %lf %127[^\0]", &d1, s1) == 2) {
				strncpy(driver.author, s1, sizeof(driver.author));
			}
		} else if (sscanf(line, " //+ %127[^\"]\"", s1) == 1) {
			code_type *code = allocate(sizeof(code_type));
			int current_length = 0;
			int current_size = 1024;
			code->text = malloc(current_size);
			while (fgets(line, sizeof(line), stdin)) {
				if (strstr(line, "//-")) {
					break;
				}
				int line_length = (int)strlen(line);
				if (line_length == 1 && current_length == 0) {
					continue;
				}
				if (current_length + line_length >= current_size) {
					code->text = realloc(code->text, current_size += 1024);
				}
				strcpy(code->text + current_length, line);
				current_length += line_length;
			}
			while (current_length > 1 && code->text[current_length - 1] == '\n') {
				current_length--;
			}
			code->size = current_length;
			if (code->size <= 1) {
				free(code->text);
				free(code);
			} else if (sscanf(s1, "%127[^.].%127[^.].%127s", s2, s3, s4) == 3) {
				device_type *device = get_device(s2);
				strcat(s3, "_PROPERTY");
				property_type *property = get_property(device, s3);
				if (strcmp(s4, "on_change") == 0) {
					append((void **)&property->on_change, code);
				} else if (strcmp(s4, "on_attach") == 0) {
					append((void **)&property->on_attach, code);
				} else if (strcmp(s4, "on_detach") == 0) {
					append((void **)&property->on_detach, code);
				}
			} else if (sscanf(s1, "%127[^.].%127s", s2, s3) == 2) {
				device_type *device = get_device(s2);
				if (strcmp(s1, "code") == 0) {
					append((void **)&device->code, code);
				} else if (strcmp(s3, "on_timer") == 0) {
					append((void **)&device->on_timer, code);
				} else if (strcmp(s3, "on_attach") == 0) {
					append((void **)&device->on_attach, code);
				} else if (strcmp(s3, "on_detach") == 0) {
					append((void **)&device->on_detach, code);
				} else if (strcmp(s3, "on_connect") == 0) {
					append((void **)&device->on_connect, code);
				} else if (strcmp(s3, "on_disconnect") == 0) {
					append((void **)&device->on_disconnect, code);
				}
			} else if (strcmp(s1, "include") == 0) {
				append((void **)&driver.include, code);
			} else if (strcmp(s1, "define") == 0 || strcmp(s1, "definitions") == 0) {
				append((void **)&driver.define, code);
			} else if (strcmp(s1, "data") == 0) {
				append((void **)&driver.data, code);
			} else if (strcmp(s1, "code") == 0) {
				append((void **)&driver.code, code);
			} else if (strcmp(s1, "on_init") == 0) {
				append((void **)&driver.on_init, code);
			} else if (strcmp(s1, "on_shutdown") == 0) {
				append((void **)&driver.on_shutdown, code);
			}
		} else if (sscanf(line, "#define DRIVER_VERSION 0x%x", &i1) == 1) {
			driver.version = (i1 & 0xFF);
		} else if (sscanf(line, "#define DRIVER_NAME \"indigo_%*[^_]_%127[^\"]\"", s1) == 1) {
			strncpy(driver.name, s1, sizeof(driver.name));
		} else if (sscanf(line, "#define DRIVER_LABEL \"%127[^\"]\"", s1) == 1) {
			strncpy(driver.label, s1, sizeof(driver.label));
		} else if (sscanf(line, "#define %127[^_]_DEVICE_NAME \"%127[^\"]\"", s1, s2) == 2) {
			make_lower_case(s1);
			device = get_device(s1);
			strncpy(device->name, s2, sizeof(device->name));
		} else if (sscanf(line, "#define %127s %127[^\0]", s1, s2) == 2) {
			int len = (int)strlen(s1);
			if (strncmp(s1 + len - 14, "_PROPERTY_NAME", 14) == 0) {
				definition_type *def = allocate(sizeof(definition_type));
				strncpy(def->name, s1, sizeof(def->name));
				strncpy(def->value, s2, sizeof(def->value));
				append((void **)&defs, def);
			} else if (strncmp(s1 + len - 10, "_ITEM_NAME", 10) == 0) {
				definition_type *def = allocate(sizeof(definition_type));
				strncpy(def->name, s1, sizeof(def->name));
				strncpy(def->value, s2, sizeof(def->value));
				append((void **)&defs, def);
			}
		} else if (sscanf(line, "	SET_DRIVER_INFO(%*[^,], \"%127[^\"]\")", s1) == 1) {
			strncpy(driver.label, s1, sizeof(driver.label));
		} else if (strstr(line, "ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;")) {
			if (device) {
				device->additional_instances = true;
			}
		} else if (strstr(line, "DEVICE_PORTS_PROPERTY->hidden = false;")) {
		} else if (strstr(line, "DEVICE_PORT_PROPERTY->hidden = false;")) {
			if (driver.serial == NULL) {
				driver.serial = allocate(sizeof(serial_type));
			}
		} else if (strstr(line, "DEVICE_BAUDRATE_PROPERTY->hidden = false;")) {
			if (driver.serial == NULL) {
				driver.serial = allocate(sizeof(serial_type));
			}
			driver.serial->configurable_speed = true;
		} else if (sscanf(line, " %127[^-]->hidden = %127[^;];", s1, s2) == 2) {
			if (device) {
				property_type *property = get_property(device, s1);
				strncpy(property->hidden, s2, sizeof(property->hidden));
			}
		} else if (sscanf(line, " strcpy(patterns[%d].%127[^,], %127[^)]);", &i1, s1, s2) == 3) {
			if (driver.serial == NULL) {
				driver.serial = allocate(sizeof(serial_type));
			}
			pattern_type *pattern = driver.serial->patterns;
			for (int i = 0; i < i1; i++) {
				if (pattern != NULL) {
					pattern = pattern->next;
				} else {
					append((void **)&driver.serial->patterns, pattern = allocate(sizeof(pattern_type)));
				}
			}
			if (pattern == NULL) {
				append((void **)&driver.serial->patterns, pattern = allocate(sizeof(pattern_type)));
			}
			if (strcmp(s1, "product_string") == 0) {
				strncpy(pattern->product, s2, sizeof(pattern->product));
			} else if (strcmp(s1, "vendor_string") == 0) {
				strncpy(pattern->vendor, s2, sizeof(pattern->vendor));
			} else if (strcmp(s1, "serial_string") == 0) {
				strncpy(pattern->serial, s2, sizeof(pattern->serial));
			}
		} else if (sscanf(line, " patterns[%d].%127[^= ] = %127[^;];", &i1, s1, s2) == 3) {
			if (driver.serial == NULL) {
				driver.serial = allocate(sizeof(serial_type));
			}
			pattern_type *pattern = driver.serial->patterns;
			for (int i = 0; i < i1; i++) {
				if (pattern != NULL) {
					pattern = pattern->next;
				} else {
					append((void **)&driver.serial->patterns, pattern = allocate(sizeof(pattern_type)));
				}
			}
			if (pattern == NULL) {
				append((void **)&driver.serial->patterns, pattern = allocate(sizeof(pattern_type)));
			}
			if (strcmp(s1, "product_id") == 0) {
				strncpy(pattern->pid, s2, sizeof(pattern->pid));
			} else if (strcmp(s1, "vendor_id") == 0) {
				strncpy(pattern->vid, s2, sizeof(pattern->vid));
			} else if (strcmp(s1, "vendor_id") == 0) {
				strncpy(pattern->exact_match, s2, sizeof(pattern->exact_match));
			}
		} else if (sscanf(line, " int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, %127[^,], %127[^,])", s1, s2) == 2) {
			driver.libusb = allocate(sizeof(libusb_type));
			if (strcmp(s1, "LIBUSB_HOTPLUG_MATCH_ANY")) {
				strncpy(driver.libusb->vid, s1, sizeof(driver.libusb->vid));
			}
			if (strcmp(s2, "LIBUSB_HOTPLUG_MATCH_ANY")) {
				strncpy(driver.libusb->pid, s2, sizeof(driver.libusb->pid));
			}
		} else if (sscanf(line, " static indigo_device %127[^_]_template = %127[^(](", s1, s2) == 2 && strcmp(s2, "INDIGO_DEVICE_INITIALIZER") == 0) {
			if (fgets(line, sizeof(line), stdin)) {
				if (sscanf(line, " \"%127[^\"]\"", s3) == 1) {
					device = get_device(s1);
					strncpy(device->name, s3, sizeof(device->name));
				}
			}
		} else if (sscanf(line, " if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, %127[^)])", s1) == 1) {
			device = get_device("aux");
			strncpy(device->interface, s1, sizeof(device->interface));
		} else if (sscanf(line, " if (indigo_%127[^_]_%127[^(]()", s1, s2) == 2 && strcmp(s2, "attach") == 0) {
			device = get_device(s1);
		} else if (sscanf(line, "static indigo_result %127[^_]_%127[^(]() {", s1, s2) == 2 && strcmp(s2, "change_property") == 0) {
			device = get_device(s1);
		} else if (sscanf(line, "static indigo_result %127[^_]_%127[^(]()", s1, s2) == 2 && strcmp(s2, "enumerate_properties") == 0 && line[strlen(line) - 1] == '{') {
			device = get_device(s1);
			bool in_is_connected = false;
			while (fgets(line, sizeof(line), stdin)) {
				line[strcspn(line, "\n")] = 0;
				if (strchr(line, '}')) {
					if (in_is_connected) {
						in_is_connected = false;
					} else {
						break;
					}
				} else if (strstr(line, "if (IS_CONNECTED) {")) {
					in_is_connected = true;
				} else if (sscanf(line, " %127[^(](%127[^)])", s1, s2) == 2 && strcmp(s1, "INDIGO_DEFINE_MATCHING_PROPERTY") == 0) {
					property_type *property = get_property(device, s2);
					property->always_defined = !in_is_connected;
				}
			}
		} else if (sscanf(line, " %s = indigo_init_switch_property(NULL, device->name, %127[^,], %127[^,], %127[^,], INDIGO_OK_STATE, %127[^,], %127[^,], %*d)", s1, s2, s3, s4, s5, s6) == 6) {
			if (device) {
				property = get_property(device, s1);
				strncpy(property->type, "switch", sizeof(property->type));
				strncpy(property->name, translate_name(s2), sizeof(property->name));
				strncpy(property->group, s3, sizeof(property->group));
				strncpy(property->label, s4, sizeof(property->label));
				strncpy(property->perm, s5, sizeof(property->perm));
				strncpy(property->rule, s6, sizeof(property->rule));
			}
		} else if (sscanf(line, " %s = indigo_init_light_property(NULL, device->name, %127[^,], %127[^,], %127[^,], INDIGO_OK_STATE, %*d)", s1, s2, s3, s4) == 4) {
			if (device) {
				property = get_property(device, s1);
				strncpy(property->type, "light", sizeof(property->type));
				strncpy(property->name, translate_name(s2), sizeof(property->name));
				strncpy(property->group, s3, sizeof(property->group));
				strncpy(property->label, s4, sizeof(property->label));
			}
		} else if (sscanf(line, " %s = indigo_init_%127[^_]_property(NULL, device->name, %127[^,], %127[^,], %127[^,], INDIGO_OK_STATE, %127[^,], %*d)", s1, s2, s3, s4, s5, s6) == 6) {
			if (device) {
				property = get_property(device, s1);
				strncpy(property->type, s2, sizeof(property->type));
				strncpy(property->name, translate_name(s3), sizeof(property->name));
				strncpy(property->group, s4, sizeof(property->group));
				strncpy(property->label, s5, sizeof(property->label));
				strncpy(property->perm, s6, sizeof(property->perm));
			}
		} else if (sscanf(line, " indigo_init_number_item(%127[^,], %127[^,], %127[^,], %127[^,], %127[^,], %127[^,], %127[^)]);", s1, s2, s3, s4, s5, s6, s7) == 7) {
			if (property) {
				item_type *item = get_item(property, s1);
				strncpy(item->name, translate_name(s2), sizeof(item->name));
				strncpy(item->label, s3, sizeof(item->label));
				strncpy(item->min, s4, sizeof(item->min));
				strncpy(item->max, s5, sizeof(item->max));
				strncpy(item->step, s6, sizeof(item->step));
				strncpy(item->value, s7, sizeof(item->value));
			}
		} else if (sscanf(line, " indigo_init_sexagesimal_number_item(%127[^,], %127[^,], %127[^,], %127[^,], %127[^,], %127[^,], %127[^)]);", s1, s2, s3, s4, s5, s6, s7) == 7) {
			if (property) {
				item_type *item = get_item(property, s1);
				strncpy(item->name, translate_name(s2), sizeof(item->name));
				strncpy(item->label, s3, sizeof(item->label));
				strncpy(item->min, s4, sizeof(item->min));
				strncpy(item->max, s5, sizeof(item->max));
				strncpy(item->step, s6, sizeof(item->step));
				strncpy(item->value, s7, sizeof(item->value));
				strncpy(item->format, "%12.9m", sizeof(item->format));
			}
		} else if (sscanf(line, " indigo_init_%127[^_]_item(%127[^,], %127[^,], %127[^,], %127[^)]);", s1, s2, s3, s4, s5) == 5) {
			if (property) {
				item_type *item = get_item(property, s2);
				strncpy(item->name, translate_name(s3), sizeof(item->name));
				strncpy(item->label, s4, sizeof(item->label));
				strncpy(item->value, s5, sizeof(item->value));
			}
		} else if (sscanf(line, " strcpy(%127[^-]->number.format, %127[^)]);", s1, s2) == 2) {
			if (property) {
				item_type *item = get_item(property, s1);
				strncpy(item->format, s2, sizeof(item->format));
			}
		} else if (sscanf(line, " } else if (%127[^(](%127[^,], property)) {", s1, s2) == 2 && strcmp(s1, "indigo_property_match_changeable") == 0) {
			if (strcmp(s2, "CONFIG_PROPERTY") && strcmp(s2, "CONNECTION_PROPERTY")) {
				if (device) {
					get_property(device, s2);
				}
			}
		} else if (sscanf(line, " %127[^(](device, NULL, %127[^)])", s1, s2) == 2 && strcmp(s1, "indigo_save_property") == 0) {
			if (device) {
				property_type *property = get_property(device, s2);
				property->persistent = true;
			}
		}
	}
}

void write_definition_source(void) {
	write_line("// %s", driver.copyright);
	write_line("// All rights reserved.");
	write_line("//");
	write_line("// You can use this software under the terms of 'INDIGO Astronomy");
	write_line("// open-source license' (see LICENSE.md).");
	write_line("//");
	write_line("// THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS");
	write_line("// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED");
	write_line("// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE");
	write_line("// ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY");
	write_line("// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL");
	write_line("// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE");
	write_line("// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS");
	write_line("// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,");
	write_line("// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING");
	write_line("// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS");
	write_line("// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.");
	write_line("");
	write_line("// %s driver definition", driver.label);
	write_line("");
	if (todos) {
		for (code_type *todo = todos; todo; todo = todo->next) {
			write_line(todo->text);
		}
		write_line("");
	}
	write_line("driver %s {", driver.name);
	write_line("\tlabel = \"%s\";", driver.label);
	write_line("\tauthor = \"%s\";", driver.author);
	write_line("\tcopyright = \"%s\";", driver.copyright);
	write_line("\tversion = %d;", driver.version);
	if (driver.serial) {
		if (driver.serial->configurable_speed || driver.serial->patterns) {
			write_line("\tserial {");
			if (driver.serial->configurable_speed) {
				write_line("\t\tconfigurable_speed = true;");
			}
			for (pattern_type *pattern = driver.serial->patterns; pattern; pattern = pattern->next) {
				write_line("\t\tpattern {");
				if (*pattern->product) {
					write_line("\t\t\tproduct = %s;", pattern->product);
				}
				if (*pattern->vendor) {
					write_line("\t\t\tvendor = %s;", pattern->vendor);
				}
				if (*pattern->serial) {
					write_line("\t\t\tserial = %s;", pattern->serial);
				}
				if (*pattern->vid) {
					write_line("\t\t\tvid = %s;", pattern->vid);
				}
				if (*pattern->pid) {
					write_line("\t\t\tpid = %s;", pattern->pid);
				}
				if (*pattern->exact_match) {
					write_line("\t\t\texact_match = %s;", pattern->exact_match);
				}
				write_line("\t\t}");
			}
			write_line("\t}");
		} else {
			write_line("\tserial;");
		}
	} else if (driver.libusb) {
		write_line("\tlibusb {");
		write_line("\t\thotplug = true;");
		if (*driver.libusb->vid) {
			write_line("\t\tvid = %s;", driver.libusb->vid);
		}
		if (*driver.libusb->pid) {
			write_line("\t\tpid = %s;", driver.libusb->pid);
		}
		write_line("\t}");
	}
	if (driver.include) {
		write_line("\tinclude {");
		for (code_type *code = driver.include; code; code = code->next) {
			write_code_block(code, 2);
		}
		write_line("\t}");
	} else {
		write_line("\t// include { }");
	}
	if (driver.define) {
		write_line("\tdefine {");
		for (code_type *code = driver.define; code; code = code->next) {
			write_code_block(code, 2);
		}
		write_line("\t}");
	} else {
		write_line("\t// define { }");
	}
	if (driver.data) {
		write_line("\tdata {");
		for (code_type *code = driver.data; code; code = code->next) {
			write_code_block(code, 2);
		}
		write_line("\t}");
	} else {
		write_line("\t// data { }");
	}
	if (driver.code) {
		write_line("\tcode {");
		for (code_type *code = driver.code; code; code = code->next) {
			write_code_block(code, 2);
		}
		write_line("\t}");
	} else {
		write_line("\t// code { }");
	}
	if (driver.on_init) {
		write_line("\ton_init {");
		for (code_type *code = driver.on_init; code; code = code->next) {
			write_code_block(code, 2);
		}
		write_line("\t}");
	} else {
		write_line("\t// on_init { }");
	}
	if (driver.on_shutdown) {
		write_line("\ton_shutdown {");
		for (code_type *code = driver.on_shutdown; code; code = code->next) {
			write_code_block(code, 2);
		}
		write_line("\t}");
	} else {
		write_line("\t// on_shutdown { }");
	}
	for (device_type *device = driver.devices; device; device = device->next) {
		write_line("\t%s {", device->type);
		write_line("\t\tname = \"%s\";", device->name);
		if (*device->interface) {
			write_line("\t\tinterface = %s;", device->interface);
		}
		if (device->additional_instances) {
			write_line("\t\tadditional_instances = true;");
		}
		if (device->code) {
			write_line("\t\tcode {");
			for (code_type *code = device->code; code; code = code->next) {
				write_code_block(code, 3);
			}
			write_line("\t\t}");
		} else {
			write_line("\t\t// code { }");
		}
		if (device->on_timer) {
			write_line("\t\ton_timer {");
			for (code_type *code = device->on_timer; code; code = code->next) {
				write_code_block(code, 3);
			}
			write_line("\t\t}");
		} else {
			write_line("\t\t// on_timer { }");
		}
		if (device->on_connect) {
			write_line("\t\ton_connect {");
			for (code_type *code = device->on_connect; code; code = code->next) {
				write_code_block(code, 3);
			}
			write_line("\t\t}");
		} else {
			write_line("\t\t// on_connect { }");
		}
		if (device->on_disconnect) {
			write_line("\t\ton_disconnect {");
			for (code_type *code = device->on_disconnect; code; code = code->next) {
				write_code_block(code, 3);
			}
			write_line("\t\t}");
		} else {
			write_line("\t\t// on_disconnect { }");
		}
		if (device->on_attach) {
			write_line("\t\ton_attach {");
			for (code_type *code = device->on_attach; code; code = code->next) {
				write_code_block(code, 3);
			}
			write_line("\t\t}");
		} else {
			write_line("\t\t// on_attach { }");
		}
		if (device->on_detach) {
			write_line("\t\ton_detach {");
			for (code_type *code = device->on_detach; code; code = code->next) {
				write_code_block(code, 3);
			}
			write_line("\t\t}");
		} else {
			write_line("\t\t// on_detach { }");
		}
		for (property_type *property = device->properties; property; property = property->next) {
			write_line("\t\t%s %s {", property->type, property->id);
			if (property->persistent) {
				write_line("\t\t\tpersistent = true;");
			}
			if (*property->hidden) {
				write_line("\t\t\thidden = %s;", property->hidden);
			}
			if (property->on_change) {
				write_line("\t\t\ton_change {");
				for (code_type *code = property->on_change; code; code = code->next) {
					write_code_block(code, 4);
				}
				write_line("\t\t\t}");
			} else {
				write_line("\t\t\t// on_change { }");
			}
			if (property->on_attach) {
				write_line("\t\t\ton_attach {");
				for (code_type *code = property->on_attach; code; code = code->next) {
					write_code_block(code, 4);
				}
				write_line("\t\t\t}");
			} else {
				write_line("\t\t\t// on_attach { }");
			}
			if (property->on_detach) {
				write_line("\t\t\ton_detach {");
				for (code_type *code = property->on_detach; code; code = code->next) {
					write_code_block(code, 4);
				}
				write_line("\t\t\t}");
			} else {
				write_line("\t\t\t// on_detach { }");
			}
			if (property->type[0] == 'i') {
			} else {
				write_line("\t\t\tname = %s;", property->name);
				write_line("\t\t\tgroup = %s;", property->group);
				write_line("\t\t\tlabel = %s;", property->label);
				if (*property->perm && strcmp(property->perm, "INDIGO_RW_PERM")) {
					write_line("\t\t\tperm = %s;", property->perm);
				}
				if (*property->rule && strcmp(property->rule, "INDIGO_ONE_OF_MANY_RULE")) {
					write_line("\t\t\trule = %s;", property->rule);
				}
				if (property->always_defined) {
					write_line("\t\t\talways_defined = true;");
				}
				write_line("\t\t\t// handle_change = false;");
				write_line("\t\t\t// asynchronous_change = false;");
				for (item_type *item = property->items; item; item = item->next) {
					write_line("\t\t\titem %s {", item->id);
					write_line("\t\t\t\tname = %s;", item->name);
					write_line("\t\t\t\tlabel = %s;", item->label);
					write_line("\t\t\t\tvalue = %s;", item->value);
					if (property->type[0] == 'n') {
						write_line("\t\t\t\tmin = %s;", item->min);
						write_line("\t\t\t\tmax = %s;", item->max);
						write_line("\t\t\t\tstep = %s;", item->step);
						if (*item->format) {
							write_line("\t\t\t\tformat = %s;", item->format);
						}
					}
					write_line("\t\t\t}");
				}
			}
			write_line("\t\t}");
		}
		write_line("\t}");
	}
	write_line("}");
}
#pragma mark - main

int main(int argc, char **argv) {
	char **arg = argv;
	char *definition_file = NULL;
	bool help = false;
	bool create = false;
	while (*++arg) {
		if (strcmp(*arg, "-v") == 0 || strcmp(*arg, "--verbose") == 0) {
			verbose_log = true;
		} else if (strcmp(*arg, "-c") == 0 || strcmp(*arg, "--create") == 0) {
			create = true;
		} else if (strcmp(*arg, "-h") == 0 || strcmp(*arg, "--help") == 0) {
			help = true;
		} else {
			definition_file = *arg;
		}
	}
	if (help || definition_file == NULL) {
		fprintf(stderr, "indigo_generator [-h|--help] [-v|--verbose] [-c|--create] definition_file\n");
		return 1;
	}
	definition_source_basename = basename(definition_file);
	definition_source_dirname = dirname(definition_file);
	if (!create && access(definition_file, F_OK) == 0) {
		freopen(definition_file, "r", stdin);
		fprintf(stderr, "Reading %s ...\n", definition_source_basename);
		read_definition_source();
		debug(0, "Parsing ...\n\n");
		if (!parse_driver_block()) {
			fprintf(stderr, "\nFailed to parse definition file\n");
			return 1;
		}
		if (driver.devices) {
			char file_name[PATH_MAX];
			snprintf(file_name, sizeof(file_name), "%s/indigo_%s_%s.h", definition_source_dirname, driver.devices->type, driver.name);
			fprintf(stderr, "Writing %s ...\n", file_name);
			freopen(file_name, "w", stdout);
			write_h_source();
			snprintf(file_name, sizeof(file_name), "%s/indigo_%s_%s_main.c", definition_source_dirname, driver.devices->type, driver.name);
			fprintf(stderr, "Writing %s ...\n", file_name);
			freopen(file_name, "w", stdout);
			write_c_main_source();
			snprintf(file_name, sizeof(file_name), "%s/indigo_%s_%s.c", definition_source_dirname, driver.devices->type, driver.name);
			fprintf(stderr, "Writing %s ...\n", file_name);
			freopen(file_name, "w", stdout);
			write_c_source();
		}
	} else {
		debug(0, "Creating %s", definition_source_basename);
		char source_file[PATH_MAX];
		strcpy(source_file, definition_file);
		char *ext = strrchr(source_file, '.');
		if (ext == NULL) {
			report_error("No extension in definition file name");
			return 1;
		}
		strcpy(ext, ".c");
		if (access(source_file, F_OK) != 0) {
			strcpy(ext, ".cpp");
			if (access(source_file, F_OK) != 0) {
				report_error("No base source file name found");
				return 1;
			}
		}
		freopen(source_file, "r", stdin);
		freopen(definition_file, "w", stdout);
		read_c_source();
		write_definition_source();
	}
}
