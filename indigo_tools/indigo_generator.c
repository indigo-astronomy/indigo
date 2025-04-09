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

typedef enum {
	TOKEN_NONE = 0,
	TOKEN_LBRACE = '{',
	TOKEN_RBRACE = '}',
	TOKEN_EQUAL = '=',
	TOKEN_SEMICOLON = ';',
	TOKEN_IDENTIFIER = 'I',
	TOKEN_STRING = 'S',
	TOKEN_NUMBER = 'N',
	TOKEN_TRUE = 'T',
	TOKEN_FALSE = 'F',
	TOKEN_CODE = 'C',
	TOKEN_EXPRESSION = 'E'
} token_type;

typedef struct item_type {
	struct item_type *next;
	char handle[64];
	char name[64];
	char label[256];
	char value[256];
	char min[256];
	char max[256];
	char step[256];
} item_type;

typedef struct property_type {
	struct property_type *next;
	char type[8];
	char handle[64];
	char name[64];
	char pointer[64];
	char label[256];
	char group[32];
	char perm[3];
	bool always_defined;
	item_type *item;
} property_type;

typedef struct code_type {
	struct code_type *next;
	char *text;
	int size;
} code_type;

typedef struct device_type {
	struct device_type *next;
	char type[16];
	char name[64];
	char interface[128];
	bool additional_instances;
	code_type *code;
	code_type *attach;
	code_type *detach;
	property_type *property;
} device_type;

typedef struct driver_type {
	char name[64];
	char label[256];
	char author[256];
	char copyright[256];
	int version;
	device_type *device;
	code_type *define;
	code_type *code;
	code_type *data;
} driver_type;

bool parsing_code = false;
bool parsing_expression = false;
char *definition_source, *current, *begin, *end;
token_type last_token;
driver_type *driver;
int indentation = 0;

void write_line(const char *format, ...) {
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	putchar('\n');
	va_end(args);
}

void *allocate(int size) {
	void *result = malloc(size);
	memset(result, 0, size);
	return result;
}

void write_code(code_type *code, int indentation) {
	char *pnt = code->text;
	int skip = 0;
	char c = *pnt++;
	while (isspace(c) && pnt - code->text < code->size) {
		c = *pnt++;
		skip++;
	}
	pnt = code->text;
	while (pnt - code->text < code->size) {
		char c = *pnt++;
		for (int i = 0; i < skip && pnt - code->text < code->size; i++) {
			if (!isspace(c))
				break;
			c = *pnt++;
		}
		for (int i = 0; i < indentation; i++)
			putchar('\t');
		putchar(c);
		while (pnt - code->text < code->size) {
			putchar(c = *pnt++);
			if (c == '\n')
				break;
		}
	}
	putchar('\n');
}

void debug(const char *format, ...) {
	static char *spaces = "\t\t\t\t\t\t\t\t\t\t";
	va_list args;
	va_start(args, format);
	char buffer[256];
	vsnprintf(buffer, sizeof(buffer), format, args);
	fprintf(stderr, "%s%s\n", spaces + 10 - indentation, buffer);
	va_end(args);
}

void report_error(const char *format, ...) {
	va_list args;
	va_start(args, format);
	char buffer[256];
	vsnprintf(buffer, sizeof(buffer), format, args);
	fprintf(stderr, "ERROR: %s\n", buffer);
	va_end(args);
}

bool get_token(void) {
	if (parsing_code) {
		while (true) {
			if (*current == 0)
				return false;
			if (*current != '\n')
				break;
			current++;
		}
		int depth = 0;
		begin = current;
		while (true) {
			if (*current == 0)
				return false;
			if (*current == '{') {
				depth++;
			} else if (*current == '}') {
				depth--;
				if (depth == -1) {
					end = current;
					while (end > begin) {
						if (!isspace(*(end - 1)))
							break;
						end--;
					}
					last_token = TOKEN_CODE;
					return true;
				}
			}
			current++;
		}
		return true;
	} else if (parsing_expression) {
		while (true) {
			if (*current == 0)
				return false;
			if (!isspace(*current))
				break;
			current++;
		}
		begin = current;
		while (true) {
			if (*current == 0)
				return false;
			if (*current == ';') {
				end = current;
				while (end > begin) {
					if (!isspace(*(end - 1)))
						break;
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
				current++;
			} else if (*current == '{') {
				current++;
				indentation += 1;
				last_token = TOKEN_LBRACE;
				return true;
			} else if (*current == '}') {
				current++;
				indentation -= 1;
				last_token = TOKEN_RBRACE;
				return true;
			} else if (*current == '=') {
				current++;
				last_token = TOKEN_EQUAL;
				return true;
			} else if (*current == ';') {
				current++;
				last_token = TOKEN_SEMICOLON;
				return true;
			} else if (*current == '/' && *(current + 1) == '/') {
				while (*current != 0 && *current != '\n')
					current++;
			} else if (*current == '"') {
				begin = ++current;
				while (*current != 0 && *current != '"')
					current++;
				end = current++;
				last_token = TOKEN_STRING;
				return true;
			} else if (strncmp(current, "true", 4) == 0) {
				current += 4;
				last_token = TOKEN_TRUE;
				return true;
			} else if (strncmp(current, "false", 5) == 0) {
				current += 4;
				return TOKEN_FALSE;
			} else if (isalpha(*current)) {
				begin = current++;
				while (isalnum(*current) || *current == '_')
					current++;
				end = current;
				last_token = TOKEN_IDENTIFIER;
				return true;
			} else if (isnumber(*current) || *current == '-') {
				begin = current++;
				while (isnumber(*current) || *current == '.' || *current == '-' || *current == 'E' || *current == 'e')
					current++;
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
		if (!get_token())
			return false;
	}
	if (last_token != token)
		return false;
	if (value != NULL) {
		bool result = strncmp(value, begin, strlen(value)) == 0;
		if (result)
			last_token = TOKEN_NONE;
		return result;
	}
	last_token = TOKEN_NONE;
	return true;
}

void copy(char *target, int max_size) {
	memset(target, 0, max_size);
	int size = (int)(end - begin);
	if (size > max_size)
		size = max_size - 1;
	strncpy(target, begin, size);
}

void append(void **head, void *next) {
	if (*head == NULL)
		*head = next;
	else
		append(*head, next);
}

bool parse_string(char *name, char *value, int max_size) {
	if (match(TOKEN_IDENTIFIER, name)) {
		if (!match(TOKEN_EQUAL, NULL))
			return false;
		if (!match(TOKEN_STRING, NULL))
			return false;
		copy(value, max_size);
		debug("%s = \"%s\";", name, value);
		match(TOKEN_SEMICOLON, NULL);
		return true;
	}
	return false;
}

bool parse_bool(char *name, bool *value) {
	if (match(TOKEN_IDENTIFIER, name)) {
		if (!match(TOKEN_EQUAL, NULL)) {
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
		return false;
	}
	return false;
}

bool parse_int(char *name, int *value) {
	if (match(TOKEN_IDENTIFIER, name)) {
		if (!match(TOKEN_EQUAL, NULL)) {
			return false;
		}
		if (!match(TOKEN_NUMBER, NULL)) {
			return false;
		}
		char number[18];
		copy(number, sizeof(number));
		*value = (int)atof(number);
		debug("%s = %d;", name, *value);
		match(TOKEN_SEMICOLON, NULL);
		return true;
	}
	return false;
}

bool parse_double(char *name, double *value) {
	if (match(TOKEN_IDENTIFIER, name)) {
		if (!match(TOKEN_EQUAL, NULL)) {
			return false;
		}
		if (!match(TOKEN_NUMBER, NULL)) {
			return false;
		}
		char number[18];
		copy(number, sizeof(number));
		*value = atof(number);
		debug("%s = %g;", name, *value);
		match(TOKEN_SEMICOLON, NULL);
		return true;
	}
	return false;
}

bool parse_code(char *name, code_type **codes) {
	if (match(TOKEN_IDENTIFIER, name)) {
		if (match(TOKEN_LBRACE, NULL)) {
			parsing_code = true;
			bool result = get_token();
			parsing_code = false;
			last_token = TOKEN_NONE;
			if (!result)
				return false;
			code_type *code = allocate(sizeof(code_type));
			if (match(TOKEN_RBRACE, NULL)) {
				code->text = begin;
				code->size = (int)(end - begin);
				append((void **)codes, code);
				debug("%s {", name);
				debug("  ... %d character(s) ...", code->size);
				debug("}");
				return true;
			}
			report_error("Missing '}'");
			return false;
		}
	}
	return false;
}

bool parse_expression(char *name, char *value, int max_size) {
	if (match(TOKEN_IDENTIFIER, name)) {
		if (!match(TOKEN_EQUAL, NULL)) {
			return false;
		}
		parsing_expression = true;
		bool result = get_token();
		parsing_expression = false;
		last_token = TOKEN_NONE;
		if (!result)
			return false;
		copy(value, max_size);
		debug("%s = %s;", name, value);
		match(TOKEN_SEMICOLON, NULL);
		return true;
	}
	return false;
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

bool parse_item(char *type, item_type **items) {
	if (match(TOKEN_IDENTIFIER, "item")) {
		item_type *item = allocate(sizeof(item_type));
		if (match(TOKEN_IDENTIFIER, NULL)) {
			char id[64];
			copy(id, sizeof(id));
			snprintf(item->label, sizeof(item->label), "%s", id);
			make_upper_case(id);
			snprintf(item->handle, sizeof(item->handle), "%s_ITEM", id);
			snprintf(item->name, sizeof(item->name), "%s_ITEM_NAME", id);
			debug("item %s {", item->handle);
			if (match(TOKEN_LBRACE, NULL)) {
				while (!match(TOKEN_RBRACE, NULL)) {
					if (parse_expression("label", item->label, sizeof(item->label)))
						continue;
					if (parse_expression("handle", item->handle, sizeof(item->handle)))
						continue;
					if (parse_expression("name", item->name, sizeof(item->name)))
						continue;
					if (*type == 'n') {
						if (parse_expression("min", item->min, sizeof(item->min)))
							continue;
						if (parse_expression("max", item->max, sizeof(item->max)))
							continue;
						if (parse_expression("step", item->step, sizeof(item->step)))
							continue;
					}
					if (*type != 'l') {
						if (parse_expression("value", item->value, sizeof(item->value)))
							continue;
					}
					report_error("Invalid token");
					return false;
				}
				debug("}");
				append((void **)items, item);
				return true;
			}
			report_error("Missing '{'");
		}
		report_error("Missing item name");
		return false;
	}
	return false;
}

bool parse_property(property_type **properties) {
	if (match(TOKEN_IDENTIFIER, "text") || match(TOKEN_IDENTIFIER, "number") || match(TOKEN_IDENTIFIER, "switch") || match(TOKEN_IDENTIFIER, "light")) {
		property_type *property = allocate(sizeof(property_type));
		copy(property->type, sizeof(property->type));
		if (match(TOKEN_IDENTIFIER, NULL)) {
			char id[64];
			copy(id, sizeof(id));
			snprintf(property->label, sizeof(property->label), "%s", id);
			make_upper_case(id);
			snprintf(property->handle, sizeof(property->handle), "%s_PROPERTY", id);
			snprintf(property->name, sizeof(property->name), "%s_PROPERTY_NAME", id);
			make_lower_case(id);
			snprintf(property->pointer, sizeof(property->pointer), "%s_property", id);
			strcpy(property->group, "MAIN_GROUP");
			strcpy(property->perm, "RW");
			debug("%s %s {", property->type, property->handle);
			if (match(TOKEN_LBRACE, NULL)) {
				while (!match(TOKEN_RBRACE, NULL)) {
					if (parse_expression("label", property->label, sizeof(property->label)))
						continue;
					if (parse_expression("handle", property->handle, sizeof(property->handle)))
						continue;
					if (parse_expression("name", property->name, sizeof(property->name)))
						continue;
					if (parse_expression("pointer", property->pointer, sizeof(property->pointer)))
						continue;
					if (parse_expression("group", property->group, sizeof(property->group)))
						continue;
					if (parse_expression("perm", property->perm, sizeof(property->perm)))
						continue;
					if (parse_expression("pointer", property->pointer, sizeof(property->pointer)))
						continue;
					if (parse_bool("always_defined", &property->always_defined))
						continue;
					if (parse_item(property->type, &property->item))
						continue;
				}
				debug("}");
				append((void **)properties, property);
				return true;
			}
			report_error("Missing '{'");
		}
		report_error("Missing property name");
		return false;
	}
	return false;
}

bool parse_device(device_type **devices) {
	if (match(TOKEN_IDENTIFIER, "ccd") || match(TOKEN_IDENTIFIER, "wheel") || match(TOKEN_IDENTIFIER, "focuser") || match(TOKEN_IDENTIFIER, "mount") || match(TOKEN_IDENTIFIER, "gps") || match(TOKEN_IDENTIFIER, "aux")) {
		device_type *device = allocate(sizeof(device_type));
		copy(device->type, sizeof(device->type));
		debug("%s {", device->type);
		if (match(TOKEN_LBRACE, NULL)) {
			while (!match(TOKEN_RBRACE, NULL)) {
				if (parse_string("name", device->name, sizeof(device->name)))
					continue;
				if (parse_expression("interface", device->interface, sizeof(device->interface)))
					continue;
				if (parse_bool("additional_instances", &device->additional_instances))
					continue;
				if (parse_property(&device->property))
					continue;
				if (parse_code("code", &device->code))
					continue;
				if (parse_code("attach", &device->attach))
					continue;
				if (parse_code("detach", &device->detach))
					continue;
			}
			debug("}");
			append((void **)devices, device);
			return true;
		}
		report_error("Missing '{'");
		return false;
	}
	return false;
}

bool parse_driver(void) {
	driver = allocate(sizeof(driver_type));
	if (match(TOKEN_IDENTIFIER, "driver")) {
		if (match(TOKEN_IDENTIFIER, NULL)) {
			copy(driver->name, sizeof(driver->name));
			debug("driver %s {", driver->name);
			if (match(TOKEN_LBRACE, NULL)) {
				while (!match(TOKEN_RBRACE, NULL)) {
					if (parse_string("author", driver->author, sizeof(driver->author)))
						continue;
					if (parse_string("copyright", driver->copyright, sizeof(driver->copyright)))
						continue;
					if (parse_string("label", driver->label, sizeof(driver->label)))
						continue;
					if (parse_int("version", &driver->version))
						continue;
					if (parse_device(&driver->device))
						continue;
					if (parse_code("define", &driver->define))
						continue;
					if (parse_code("data", &driver->data))
						continue;
					if (parse_code("code", &driver->code))
						continue;
					report_error("Unexpected token");
					return false;
				}
				debug("}");
				return true;
			}
			report_error("Missing '{'");
			return false;
		}
		report_error("Missing driver name");
		return false;
	}
	report_error("Missing 'driver' keyword");
	return false;
}

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

void write_license(void) {
	write_line("// DO NOT MODIFY THIS FILE, IT IS MACHINE GENERATED!");
	write_line("");
	write_line("// %s", driver->copyright);
	write_line("// All rights reserved.");
	write_line("");
	write_line("// You can use this software under the terms of 'INDIGO Astronomy");
	write_line("// open-source license' (see LICENSE.md).\n");
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
	write_line("// version history");
	write_line("// 3.0 %s", driver->author);
	write_line("");
}

void write_h_source(void) {
	write_license();
	write_line("#ifndef %s_%s_h", driver->device->type, driver->name);
	write_line("#define %s_%s_h", driver->device->type, driver->name);
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
	write_line("INDIGO_EXTERN indigo_result indigo_%s_%s(indigo_driver_action action, indigo_driver_info *info);", driver->device->type, driver->name);
	write_line("");
	write_line("#ifdef __cplusplus");
	write_line("}");
	write_line("#endif");
	write_line("");
	write_line("#endif");
}

void write_c_main_source(void) {
	write_license();
	write_line("#include <indigo/indigo_driver_xml.h>");
	write_line("");
	write_line("#include \"indigo_%s_%s.h\"", driver->device->type, driver->name);
	write_line("");
	write_line("int main(int argc, const char * argv[]) {");
	write_line("\tindigo_main_argc = argc;");
	write_line("\tindigo_main_argv = argv;");
	write_line("\tindigo_client *protocol_adapter = indigo_xml_device_adapter(&indigo_stdin_handle, &indigo_stdout_handle);");
	write_line("\tindigo_start();");
	write_line("\tindigo_%s_%s(INDIGO_DRIVER_INIT, NULL);", driver->device->type, driver->name);
	write_line("\tindigo_attach_client(protocol_adapter);");
	write_line("\tindigo_xml_parse(NULL, protocol_adapter);");
	write_line("\tindigo_aux_ppb(INDIGO_DRIVER_SHUTDOWN, NULL);");
	write_line("\tindigo_stop();");
	write_line("\treturn 0;");
	write_line("}");
}

void write_c_source(void) {
	write_license();
	write_line("#pragma mark - Includes");
	write_line("");
	write_line("#include <stdlib.h>");
	write_line("#include <string.h>");
	write_line("#include <math.h>");
	write_line("#include <assert.h>");
	write_line("#include <pthread.h>");
	write_line("");
	write_line("#include <indigo/indigo_driver_xml.h>");
	device_type *device = driver->device;
	while (device) {
		write_line("#include <indigo/indigo_%s_driver.h>", device->type);
		device = device->next;
	}
	write_line("#include <indigo/indigo_uni_io.h>");
	write_line("");
	write_line("#include \"indigo_%s_%s.h\"", driver->device->type, driver->name);
	write_line("");
	write_line("#pragma mark - Common definitions");
	write_line("");
	write_line("#define DRIVER_VERSION %04X", driver->version);
	write_line("#define DRIVER_NAME \"indigo_%s_%s\"", driver->device->type, driver->name);
	write_line("");
	code_type *code = driver->define;
	while (code) {
		write_code(code, 0);
		code = code->next;
	}
	write_line("");
	write_line("#pragma mark - Property definitions");
	write_line("");
	device = driver->device;
	while (device) {
		property_type *property = device->property;
		while (property) {
			write_line("// %s", property->name);
			write_line("");
			write_line("#define %s (PRIVATE_DATA->%s_property)", property->handle, property->pointer);
			item_type *item = property->item;
			int index = 0;
			while (item) {
				write_line("#define %s_ITEM (%s->items + %d)", item->handle, property->handle, index++);
				item = item->next;
			}
			property = property->next;
		}
		device = device->next;
	}
	write_line("");
	write_line("#pragma mark - Private data definition");
	write_line("");
	write_line("#define PRIVATE_DATA ((%s_private_data *)device->private_data)", driver->name);
	write_line("");
	write_line("typedef struct {");
	write_line("\tpthread_mutex_t mutex;");
	code = driver->data;
	while (code) {
		write_code(code, 1);
		code = code->next;
	}
	device = driver->device;
	while (device) {
		property_type *property = device->property;
		while (property) {
			write_line("\tindigo_property *%s;", property->pointer);
			property = property->next;
		}
		device = device->next;
	}
	write_line("} %s_private_data;", driver->name);
	write_line("");
	write_line("#pragma mark - Low level communication (common)");
	write_line("");
	code = driver->code;
	if (code) {
		write_line("// Custom code ");
		write_line("");
		while (code) {
			write_code(code, 0);
			write_line("");
			code = code->next;
		}
	}
	device = driver->device;
	bool master_device = true;
	while (device) {
		write_line("#pragma mark - Low level communication (%s)", device->type);
		write_line("");
		code = device->code;
		if (code) {
			write_line("// Custom code ");
			while (code) {
				write_code(code, 0);
				write_line("");
				code = code->next;
			}
		}
		write_line("#pragma mark - High level communication (%s)", device->type);
		write_line("");
		write_line("static indigo_result %s_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);", device->type);
		write_line("");
		write_line("static indigo_result %s_attach(indigo_device *device) {", device->type);
		if (*device->type == 'a')
			write_line("\tif (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, %s) == INDIGO_OK) {", device->interface);
		else
			write_line("\tif (indigo_%s_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {", device->type);
		write_line("");
		code = device->attach;
		if (code) {
			write_line("\t\t// Custom code ");
			write_line("");
			while (code) {
				write_code(code, 2);
				write_line("");
				code = code->next;
			}
		}
		property_type *property = device->property;
		while (property) {
			write_line("\t\t// %s", property->name);
			write_line("");
			item_type *item = property->item;
			int count = 0;
			while (item) {
				count++;
				item = item->next;
			}
			switch (*property->type) {
				case 't':
					write_line("\t\t%s = indigo_init_text_property(NULL, device->name, %s, %s, %s, INDIGO_OK_STATE, INDIGO_%s_PERM, %d);", property->handle, property->name, property->group, property->label, property->perm, count);
					write_line("\t\tif (%s == NULL) {", property->handle);
					write_line("\t\t\treturn INDIGO_FAILED;");
					write_line("\t\t}");
					item = property->item;
					while (item) {
						write_line("\t\tindigo_init_text_item(%s, %s, %s, %s);", item->handle, item->name, item->label, item->value);
						item = item->next;
					}
					break;
				case 'n':
					write_line("\t\t%s = indigo_init_number_property(NULL, device->name, %s, %s, %s, INDIGO_OK_STATE, INDIGO_%s_PERM, %d);", property->handle, property->name, property->group, property->label, property->perm, count);
					write_line("\t\tif (%s == NULL) {", property->handle);
					write_line("\t\t\treturn INDIGO_FAILED;");
					write_line("\t\t}");
					item = property->item;
					while (item) {
						write_line("\t\tindigo_init_number_item(%s, %s, %s, %s, %s, %s, %s);", item->handle, item->name, item->label, item->min, item->max, item->step, item->value);
						item = item->next;
					}
					break;
				case 's':
					break;
				case 'l':
					break;
			}
			write_line("");
			property = property->next;
		}
		if (device->additional_instances)
			write_line("\t\tADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;");
		write_line("\t\tINDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);");
		if (master_device)
			write_line("\t\tpthread_mutex_init(&PRIVATE_DATA->mutex, NULL);");
		write_line("\t\treturn %s_enumerate_properties(device, NULL, NULL);", device->type);
		write_line("\t}");
		write_line("\treturn INDIGO_FAILED;");
		write_line("}");
		write_line("");
		write_line("static indigo_result %s_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {", device->type);
		bool first_one = true;
		property = device->property;
		while (property) {
			if (!property->always_defined) {
				if (first_one) {
					write_line("\tif (IS_CONNECTED) {");
					first_one = false;
				}
				write_line("\t\tindigo_define_matching_property(%s);", property->handle);
			}
			property = property->next;
		}
		if (!first_one) {
			write_line("\t}");
		}
		property = device->property;
		while (property) {
			if (property->always_defined) {
				write_line("\tindigo_define_matching_property(%s);", property->handle);
			}
			property = property->next;
		}
		write_line("\treturn indigo_%s_enumerate_properties(device, NULL, NULL);", device->type);
		write_line("}");
		write_line("");
		write_line("static indigo_result %s_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {", device->type);
		write_line("\treturn indigo_%s_change_property(device, client, property);", device->type);
		write_line("}");
		write_line("");
		write_line("static indigo_result %s_detach(indigo_device *device) {", device->type);
		write_line("\tif (IS_CONNECTED) {");
		write_line("\t\tindigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);");
		write_line("\t\t%s_connection_handler(device);", device->type);
		write_line("\t}");
		code = device->detach;
		if (code) {
			write_line("\t\t// Custom code ");
			write_line("");
			while (code) {
				write_code(code, 0);
				write_line("");
				code = code->next;
			}
		}
		property = device->property;
		while (property) {
			write_line("\tindigo_release_property(%s);", property->handle);
			property = property->next;
		}
		write_line("\tINDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);");
		if (master_device)
			write_line("\tpthread_mutex_destroy(&PRIVATE_DATA->mutex);");
		write_line("\treturn indigo_%s_detach(device);", device->type);
		write_line("}");
		write_line("");
		master_device = false;
		device = device->next;
	}
}

int main(int argc, char **argv) {
	freopen(argv[1], "r", stdin);
	fprintf(stderr, "Reading %s ...\n", argv[1]);
	read_definition_source();
	fprintf(stderr, "Parsing ...\n\n");
	parse_driver();
	fprintf(stderr, "\nParsed ...\n");
	char file_name[64];
	snprintf(file_name, sizeof(file_name), "indigo_%s_%s.h", driver->device->type, driver->name);
	fprintf(stderr, "Writing %s ...\n", file_name);
	freopen(file_name, "w", stdout);
	write_h_source();
	snprintf(file_name, sizeof(file_name), "indigo_%s_%s_main.c", driver->device->type, driver->name);
	fprintf(stderr, "Writing %s ...\n", file_name);
	freopen(file_name, "w", stdout);
	write_c_main_source();
	snprintf(file_name, sizeof(file_name), "indigo_%s_%s.c", driver->device->type, driver->name);
	fprintf(stderr, "Writing %s ...\n", file_name);
	freopen(file_name, "w", stdout);
	write_c_source();
}
