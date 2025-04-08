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
	TOKEN_CODE = 'C'
} token_type;

typedef struct item_type {
	struct item_type *next;
	char name[64];
	char label[256];
} item_type;

typedef struct property_type {
	struct property_type *next;
	char type[8];
	char name[64];
	char lower_case_name[64];
	char label[256];
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
	code_type *code;
	property_type *property;
} device_type;

typedef struct {
	char name[64];
	char label[256];
	char author[256];
	char copyright[256];
	int version;
	device_type *device;
	code_type *code;
	code_type *data;
} driver_type;

bool parsing_code = false;
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

void write_code(code_type *code, int indentation) {
	char *pnt = code->text;
	while (pnt - code->text < code->size) {
		char c = *pnt++;
		if (isspace(c))
			continue;
		for (int i = 0; i < indentation; i++)
			putchar(' ');
		putchar(c);
		while (pnt - code->text < code->size) {
			putchar(c = *pnt++);
			if (c == '\n')
				break;
		}
	}
}

void debug(const char *format, ...) {
	static char *spaces = "          ";
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
		int depth = 0;
		begin = current;
		while (true) {
			if (*current == '{') {
				depth++;
			} else if (*current == '}') {
				depth--;
				if (depth == -1) {
					end = current - 1;
					last_token = TOKEN_CODE;
					return true;
				}
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
				indentation += 2;
				last_token = TOKEN_LBRACE;
				return true;
			} else if (*current == '}') {
				current++;
				indentation -= 2;
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
			get_token();
			parsing_code = false;
			last_token = TOKEN_NONE;
			code_type *code = malloc(sizeof(code_type));
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

bool parse_item(item_type **items) {
	if (match(TOKEN_IDENTIFIER, "item")) {
		item_type *item = malloc(sizeof(item_type));
		if (match(TOKEN_IDENTIFIER, NULL)) {
			copy(item->name, sizeof(item->name));
			debug("item %s {", item->name);
			if (match(TOKEN_LBRACE, NULL)) {
				while (!match(TOKEN_RBRACE, NULL)) {
					if (parse_string("label", item->label, sizeof(item->label)))
						continue;
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
		property_type *property = malloc(sizeof(property_type));
		copy(property->type, sizeof(property->type));
		if (match(TOKEN_IDENTIFIER, NULL)) {
			copy(property->name, sizeof(property->name));
			char *n = property->name;
			char *lc = property->lower_case_name;
			while (*n) {
				*lc++ = tolower(*n++);
			}
			*lc = 0;
			debug("%s %s {", property->type, property->name);
			if (match(TOKEN_LBRACE, NULL)) {
				int index = 0;
				while (!match(TOKEN_RBRACE, NULL)) {
					if (parse_string("label", property->label, sizeof(property->label)))
						continue;
					if (parse_string("lower_case_name", property->lower_case_name, sizeof(property->lower_case_name)))
						continue;
					if (parse_item(&property->item))
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
		device_type *device = malloc(sizeof(device_type));
		copy(device->type, sizeof(device->type));
		debug("%s {", device->type);
		if (match(TOKEN_LBRACE, NULL)) {
			while (!match(TOKEN_RBRACE, NULL)) {
				if (parse_string("name", device->name, sizeof(device->name)))
					continue;
				if (parse_property(&device->property))
					continue;
				if (parse_code("code", &device->code))
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
	driver = malloc(sizeof(driver_type));
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
					if (parse_code("code", &driver->code))
						continue;
					if (parse_code("data", &driver->data))
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
	current = definition_source = malloc(max_size);
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
	write_line("  indigo_main_argc = argc;");
	write_line("  indigo_main_argv = argv;");
	write_line("  indigo_client *protocol_adapter = indigo_xml_device_adapter(&indigo_stdin_handle, &indigo_stdout_handle);");
	write_line("  indigo_start();");
	write_line("  indigo_%s_%s(INDIGO_DRIVER_INIT, NULL);", driver->device->type, driver->name);
	write_line("  indigo_attach_client(protocol_adapter);");
	write_line("  indigo_xml_parse(NULL, protocol_adapter);");
	write_line("  indigo_aux_ppb(INDIGO_DRIVER_SHUTDOWN, NULL);");
	write_line("  indigo_stop();");
	write_line("  return 0;");
	write_line("}");
}

void write_c_source(void) {
	write_license();
	write_line("#define DRIVER_VERSION %04X", driver->version);
	write_line("#define DRIVER_NAME \"indigo_%s_%s\"", driver->device->type, driver->name);
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
	write_line("#define PRIVATE_DATA ((%s_private_data *)device->private_data)", driver->name);
	write_line("");
	device = driver->device;
	while (device) {
		property_type *property = device->property;
		while (property) {
			write_line("#define %s_PROPERTY (PRIVATE_DATA->%s_property)", property->name, property->lower_case_name);
			item_type *item = property->item;
			int index = 0;
			while (item) {
				write_line("#define %s_ITEM (%s_PROPERTY->items + %d)", item->name, property->name, index++);
				item = item->next;
			}
			property = property->next;
		}
		device = device->next;
	}
	write_line("");
	write_line("typedef struct {");
	code_type *data = driver->data;
	while (data) {
		write_code(data, 2);
		data = data->next;
	}
	device = driver->device;
	while (device) {
		property_type *property = device->property;
		while (property) {
			write_line("  indigo_property *%s_property;", property->lower_case_name);
			property = property->next;
		}
		device = device->next;
	}
	write_line("} %s_private_data;", driver->name);
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
