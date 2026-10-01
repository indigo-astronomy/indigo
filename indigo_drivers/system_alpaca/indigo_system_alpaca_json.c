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
// 3.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO ASCOM Alpaca client driver, JSON parser
 \file indigo_system_alpaca_json.c
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <indigo/indigo_bus.h>

#include "indigo_system_alpaca_json.h"

#define ALPACA_JSON_EXACT_MANTISSA				9007199254740992ULL
#define ALPACA_JSON_EXACT_DOUBLE					9007199254740992.0
#define ALPACA_JSON_MANTISSA_LIMIT				1000000000000000000ULL
#define ALPACA_JSON_EXPONENT_LIMIT				100000
#define ALPACA_JSON_EXACT_EXPONENT				22
#define ALPACA_JSON_IMAGE_FIRST_CAPACITY	65536
#define ALPACA_JSON_NAME_SIZE							32

struct alpaca_json_value {
	const char *name;
	const alpaca_json_value *next;
	union {
		bool boolean;
		struct {
			double value;
			int64_t integer;
		} number;
		struct {
			const char *text;
			size_t length;
		} string;
		const alpaca_json_value *first;
	} as;
	uint32_t count;
	uint8_t type;
	bool integer;
};

typedef struct {
	const char *text;
	size_t length;
	size_t position;
	char *error;
	int error_size;
	alpaca_json_value *nodes;
	size_t node_count;
	size_t node_capacity;
	char *pool;
	size_t pool_used;
	size_t pool_size;
} alpaca_json_parser;

typedef struct {
	char *data;
	size_t size;
	size_t used;
	bool truncated;
} alpaca_json_sink;

typedef struct {
	alpaca_json_value *container;
	alpaca_json_value *last;
	bool object;
} alpaca_json_frame;

typedef struct {
	alpaca_json_image_format format;
	void **buffer;
	size_t *capacity;
	bool growable;
	size_t count;
	size_t rounded;
	double minimum;
	double maximum;
	int rank;
	int dimensions[3];
} alpaca_json_image_sink;

static const double alpaca_json_powers[ALPACA_JSON_EXACT_EXPONENT + 1] = { 1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9, 1e10, 1e11, 1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19, 1e20, 1e21, 1e22 };

static bool alpaca_json_fail(alpaca_json_parser *parser, const char *message) {
	if (parser->error != NULL && parser->error_size > 0) {
		snprintf(parser->error, parser->error_size, "%s at offset %llu", message, (unsigned long long)parser->position);
	}
	return false;
}

static int alpaca_json_peek(const alpaca_json_parser *parser) {
	if (parser->position < parser->length) {
		return (unsigned char)parser->text[parser->position];
	}
	return -1;
}

static void alpaca_json_skip_whitespace(alpaca_json_parser *parser) {
	while (parser->position < parser->length) {
		char c = parser->text[parser->position];
		if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
			break;
		}
		parser->position++;
	}
}

static bool alpaca_json_is_digit(int c) {
	return c >= '0' && c <= '9';
}

static bool alpaca_json_equal(const char *name, const char *other, bool ignore_case) {
	if (name == NULL || other == NULL) {
		return false;
	}
	for (;; name++, other++) {
		unsigned char a = (unsigned char)*name;
		unsigned char b = (unsigned char)*other;
		if (ignore_case) {
			if (a >= 'A' && a <= 'Z') {
				a = (unsigned char)(a + ('a' - 'A'));
			}
			if (b >= 'A' && b <= 'Z') {
				b = (unsigned char)(b + ('a' - 'A'));
			}
		}
		if (a != b) {
			return false;
		}
		if (a == 0) {
			return true;
		}
	}
}

// Append a whole character or nothing, so that a truncated copy never ends in the middle of a UTF-8 sequence.
static void alpaca_json_put(alpaca_json_sink *sink, const char *bytes, size_t count) {
	if (sink->data == NULL) {
		return;
	}
	if (sink->truncated || sink->used + count >= sink->size) {
		sink->truncated = true;
		return;
	}
	memcpy(sink->data + sink->used, bytes, count);
	sink->used += count;
}

static void alpaca_json_put_code_point(alpaca_json_sink *sink, unsigned code) {
	char bytes[4] = { 0 };
	size_t count = 0;
	if (code < 0x80) {
		bytes[count++] = (char)code;
	} else if (code < 0x800) {
		bytes[count++] = (char)(0xC0 | (code >> 6));
		bytes[count++] = (char)(0x80 | (code & 0x3F));
	} else if (code < 0x10000) {
		bytes[count++] = (char)(0xE0 | (code >> 12));
		bytes[count++] = (char)(0x80 | ((code >> 6) & 0x3F));
		bytes[count++] = (char)(0x80 | (code & 0x3F));
	} else {
		bytes[count++] = (char)(0xF0 | (code >> 18));
		bytes[count++] = (char)(0x80 | ((code >> 12) & 0x3F));
		bytes[count++] = (char)(0x80 | ((code >> 6) & 0x3F));
		bytes[count++] = (char)(0x80 | (code & 0x3F));
	}
	alpaca_json_put(sink, bytes, count);
}

// Length of the well formed UTF-8 sequence starting at bytes, 0 if there is none.
static size_t alpaca_json_utf8_length(const unsigned char *bytes, size_t available) {
	unsigned char lead = bytes[0];
	unsigned char low = 0x80;
	unsigned char high = 0xBF;
	size_t length = 0;
	if (lead >= 0xC2 && lead <= 0xDF) {
		length = 2;
	} else if (lead >= 0xE0 && lead <= 0xEF) {
		length = 3;
		if (lead == 0xE0) {
			low = 0xA0;
		} else if (lead == 0xED) {
			high = 0x9F;
		}
	} else if (lead >= 0xF0 && lead <= 0xF4) {
		length = 4;
		if (lead == 0xF0) {
			low = 0x90;
		} else if (lead == 0xF4) {
			high = 0x8F;
		}
	} else {
		return 0;
	}
	if (available < length || bytes[1] < low || bytes[1] > high) {
		return 0;
	}
	for (size_t i = 2; i < length; i++) {
		if ((bytes[i] & 0xC0) != 0x80) {
			return 0;
		}
	}
	return length;
}

static bool alpaca_json_scan_hex(alpaca_json_parser *parser, unsigned *code) {
	unsigned value = 0;
	for (int i = 0; i < 4; i++) {
		int c = alpaca_json_peek(parser);
		if (c >= '0' && c <= '9') {
			value = (value << 4) | (unsigned)(c - '0');
		} else if (c >= 'a' && c <= 'f') {
			value = (value << 4) | (unsigned)(c - 'a' + 10);
		} else if (c >= 'A' && c <= 'F') {
			value = (value << 4) | (unsigned)(c - 'A' + 10);
		} else {
			return alpaca_json_fail(parser, "Invalid \\u escape");
		}
		parser->position++;
	}
	*code = value;
	return true;
}

static bool alpaca_json_scan_escape(alpaca_json_parser *parser, alpaca_json_sink *sink) {
	parser->position++;
	int c = alpaca_json_peek(parser);
	char plain = 0;
	switch (c) {
		case '"':
		case '\\':
		case '/':
			plain = (char)c;
			break;
		case 'b':
			plain = '\b';
			break;
		case 'f':
			plain = '\f';
			break;
		case 'n':
			plain = '\n';
			break;
		case 'r':
			plain = '\r';
			break;
		case 't':
			plain = '\t';
			break;
		case 'u':
			break;
		default:
			return alpaca_json_fail(parser, "Invalid escape");
	}
	parser->position++;
	if (c != 'u') {
		alpaca_json_put(sink, &plain, 1);
		return true;
	}
	unsigned code = 0;
	if (!alpaca_json_scan_hex(parser, &code)) {
		return false;
	}
	if (code >= 0xDC00 && code <= 0xDFFF) {
		return alpaca_json_fail(parser, "Unpaired low surrogate");
	}
	if (code >= 0xD800 && code <= 0xDBFF) {
		unsigned low = 0;
		if (parser->length - parser->position < 2 || parser->text[parser->position] != '\\' || parser->text[parser->position + 1] != 'u') {
			return alpaca_json_fail(parser, "Unpaired high surrogate");
		}
		parser->position += 2;
		if (!alpaca_json_scan_hex(parser, &low)) {
			return false;
		}
		if (low < 0xDC00 || low > 0xDFFF) {
			return alpaca_json_fail(parser, "Unpaired high surrogate");
		}
		code = 0x10000 + ((code - 0xD800) << 10) + (low - 0xDC00);
	}
	alpaca_json_put_code_point(sink, code);
	return true;
}

// Scan a string starting at its opening quote. The unescaped text is never longer than the source.
static bool alpaca_json_scan_string(alpaca_json_parser *parser, alpaca_json_sink *sink) {
	parser->position++;
	while (parser->position < parser->length) {
		const char *bytes = parser->text + parser->position;
		unsigned char c = (unsigned char)*bytes;
		if (c == '"') {
			parser->position++;
			if (sink->data != NULL) {
				sink->data[sink->used] = 0;
			}
			return true;
		}
		if (c < 0x20) {
			return alpaca_json_fail(parser, "Control character in string");
		}
		if (c == '\\') {
			if (!alpaca_json_scan_escape(parser, sink)) {
				return false;
			}
		} else if (c < 0x80) {
			alpaca_json_put(sink, bytes, 1);
			parser->position++;
		} else {
			size_t count = alpaca_json_utf8_length((const unsigned char *)bytes, parser->length - parser->position);
			if (count == 0) {
				alpaca_json_put(sink, "?", 1);
				parser->position++;
			} else {
				alpaca_json_put(sink, bytes, count);
				parser->position += count;
			}
		}
	}
	return alpaca_json_fail(parser, "Unterminated string");
}

// Correctly rounded conversion of a validated number token which the exact path can not handle. strtod()
// follows LC_NUMERIC, so the decimal point is replaced by whatever the current locale prints.
static bool alpaca_json_convert_number(const char *token, size_t length, double *number) {
	char probe[16] = { 0 };
	char local[128] = { 0 };
	snprintf(probe, sizeof(probe), "%.1f", 0.5);
	size_t probe_length = strlen(probe);
	const char *separator = ".";
	size_t separator_length = 1;
	if (probe_length > 2 && probe[0] == '0' && probe[probe_length - 1] == '5') {
		separator = probe + 1;
		separator_length = probe_length - 2;
	}
	size_t size = length + separator_length + 1;
	char *buffer = size > sizeof(local) ? indigo_safe_malloc(size) : local;
	size_t used = 0;
	for (size_t i = 0; i < length; i++) {
		if (token[i] == '.') {
			memcpy(buffer + used, separator, separator_length);
			used += separator_length;
		} else {
			buffer[used++] = token[i];
		}
	}
	buffer[used] = 0;
	char *end = NULL;
	double value = strtod(buffer, &end);
	bool result = end == buffer + used && isfinite(value);
	if (buffer != local) {
		indigo_safe_free(buffer);
	}
	*number = value;
	return result;
}

// Scan a number token. *is_integer is set if the token has neither a fraction nor an exponent and fits into int64_t.
static bool alpaca_json_scan_number(alpaca_json_parser *parser, double *number, int64_t *integer, bool *is_integer) {
	const char *text = parser->text;
	size_t length = parser->length;
	size_t start = parser->position;
	size_t position = start;
	uint64_t mantissa = 0;
	int exponent = 0;
	bool negative = false;
	bool exact = true;
	bool integral = true;
	if (position < length && text[position] == '-') {
		negative = true;
		position++;
	}
	if (position >= length || !alpaca_json_is_digit(text[position])) {
		parser->position = position;
		return alpaca_json_fail(parser, "Invalid number");
	}
	if (text[position] == '0') {
		position++;
		if (position < length && alpaca_json_is_digit(text[position])) {
			parser->position = position;
			return alpaca_json_fail(parser, "Leading zero in number");
		}
	} else {
		while (position < length && alpaca_json_is_digit(text[position])) {
			if (mantissa < ALPACA_JSON_MANTISSA_LIMIT) {
				mantissa = mantissa * 10 + (uint64_t)(text[position] - '0');
			} else {
				exact = false;
			}
			position++;
		}
	}
	if (position < length && text[position] == '.') {
		integral = false;
		position++;
		if (position >= length || !alpaca_json_is_digit(text[position])) {
			parser->position = position;
			return alpaca_json_fail(parser, "Missing fraction digits");
		}
		while (position < length && alpaca_json_is_digit(text[position])) {
			if (mantissa < ALPACA_JSON_MANTISSA_LIMIT && exponent > -ALPACA_JSON_EXPONENT_LIMIT) {
				mantissa = mantissa * 10 + (uint64_t)(text[position] - '0');
				exponent--;
			} else {
				exact = false;
			}
			position++;
		}
	}
	if (position < length && (text[position] == 'e' || text[position] == 'E')) {
		int value = 0;
		bool negative_exponent = false;
		integral = false;
		position++;
		if (position < length && (text[position] == '+' || text[position] == '-')) {
			negative_exponent = text[position] == '-';
			position++;
		}
		if (position >= length || !alpaca_json_is_digit(text[position])) {
			parser->position = position;
			return alpaca_json_fail(parser, "Missing exponent digits");
		}
		while (position < length && alpaca_json_is_digit(text[position])) {
			if (value < ALPACA_JSON_EXPONENT_LIMIT) {
				value = value * 10 + (text[position] - '0');
			} else {
				exact = false;
			}
			position++;
		}
		exponent += negative_exponent ? -value : value;
	}
	*is_integer = false;
	*integer = 0;
	if (integral && exact) {
		if (negative && mantissa <= (uint64_t)INT64_MAX + 1) {
			*integer = mantissa == (uint64_t)INT64_MAX + 1 ? INT64_MIN : -(int64_t)mantissa;
			*is_integer = true;
		} else if (!negative && mantissa <= (uint64_t)INT64_MAX) {
			*integer = (int64_t)mantissa;
			*is_integer = true;
		}
	}
	if (exact && mantissa <= ALPACA_JSON_EXACT_MANTISSA && exponent >= -ALPACA_JSON_EXACT_EXPONENT && exponent <= ALPACA_JSON_EXACT_EXPONENT) {
		double value = (double)mantissa;
		if (exponent < 0) {
			value /= alpaca_json_powers[-exponent];
		} else {
			value *= alpaca_json_powers[exponent];
		}
		*number = negative ? -value : value;
	} else if (!alpaca_json_convert_number(text + start, position - start, number)) {
		return alpaca_json_fail(parser, "Number out of range");
	}
	parser->position = position;
	return true;
}

static bool alpaca_json_scan_literal(alpaca_json_parser *parser, const char *literal) {
	size_t length = strlen(literal);
	if (parser->length - parser->position < length || memcmp(parser->text + parser->position, literal, length) != 0) {
		return alpaca_json_fail(parser, "Invalid literal");
	}
	parser->position += length;
	return true;
}

// Scan a string and keep its unescaped copy in the pool of the document if a DOM is being built.
static bool alpaca_json_store_string(alpaca_json_parser *parser, bool build, const char **text, size_t *length) {
	alpaca_json_sink sink = { 0 };
	if (build) {
		if (parser->pool_used >= parser->pool_size) {
			return alpaca_json_fail(parser, "String pool exhausted");
		}
		sink.data = parser->pool + parser->pool_used;
		sink.size = parser->pool_size - parser->pool_used;
	}
	if (!alpaca_json_scan_string(parser, &sink)) {
		return false;
	}
	if (sink.truncated) {
		return alpaca_json_fail(parser, "String pool exhausted");
	}
	if (build) {
		parser->pool_used += sink.used + 1;
		*text = sink.data;
		*length = sink.used;
	}
	return true;
}

// Scan a value which is not a container; node is NULL if no DOM is being built.
static bool alpaca_json_parse_scalar(alpaca_json_parser *parser, alpaca_json_value *node) {
	int c = alpaca_json_peek(parser);
	if (c == '"') {
		const char *text = NULL;
		size_t length = 0;
		if (!alpaca_json_store_string(parser, node != NULL, &text, &length)) {
			return false;
		}
		if (node != NULL) {
			node->type = ALPACA_JSON_STRING;
			node->as.string.text = text;
			node->as.string.length = length;
		}
		return true;
	}
	if (c == '-' || alpaca_json_is_digit(c)) {
		double number = 0;
		int64_t integer = 0;
		bool is_integer = false;
		if (!alpaca_json_scan_number(parser, &number, &integer, &is_integer)) {
			return false;
		}
		if (node != NULL) {
			node->type = ALPACA_JSON_NUMBER;
			node->as.number.value = number;
			node->as.number.integer = integer;
			node->integer = is_integer;
		}
		return true;
	}
	if (c == 't' || c == 'f') {
		if (!alpaca_json_scan_literal(parser, c == 't' ? "true" : "false")) {
			return false;
		}
		if (node != NULL) {
			node->type = ALPACA_JSON_BOOL;
			node->as.boolean = c == 't';
		}
		return true;
	}
	if (c == 'n') {
		if (!alpaca_json_scan_literal(parser, "null")) {
			return false;
		}
		if (node != NULL) {
			node->type = ALPACA_JSON_NULL;
		}
		return true;
	}
	return alpaca_json_fail(parser, c < 0 ? "Unexpected end of text" : "Unexpected character");
}

static bool alpaca_json_parse_member_name(alpaca_json_parser *parser, bool build, const char **name) {
	size_t length = 0;
	alpaca_json_skip_whitespace(parser);
	if (alpaca_json_peek(parser) != '"') {
		return alpaca_json_fail(parser, "Expected member name");
	}
	if (!alpaca_json_store_string(parser, build, name, &length)) {
		return false;
	}
	alpaca_json_skip_whitespace(parser);
	if (alpaca_json_peek(parser) != ':') {
		return alpaca_json_fail(parser, "Expected ':'");
	}
	parser->position++;
	alpaca_json_skip_whitespace(parser);
	return true;
}

// Parse one value of any type without recursion. With build set, the nodes are appended to parser->nodes and
// the first one is the root; without it the value is only validated and skipped.
static bool alpaca_json_parse_tree(alpaca_json_parser *parser, bool build) {
	alpaca_json_frame stack[ALPACA_JSON_MAX_DEPTH];
	int depth = 0;
	const char *name = NULL;
	alpaca_json_skip_whitespace(parser);
	for (;;) {
		alpaca_json_value *node = NULL;
		bool opened = false;
		int c = alpaca_json_peek(parser);
		if (build) {
			if (parser->node_count >= parser->node_capacity) {
				return alpaca_json_fail(parser, "Node pool exhausted");
			}
			node = parser->nodes + parser->node_count++;
			node->name = name;
			if (depth > 0) {
				alpaca_json_frame *frame = stack + depth - 1;
				if (frame->last != NULL) {
					frame->last->next = node;
				} else {
					frame->container->as.first = node;
				}
				frame->last = node;
				frame->container->count++;
			}
		}
		if (c == '{' || c == '[') {
			if (depth == ALPACA_JSON_MAX_DEPTH) {
				return alpaca_json_fail(parser, "Nesting too deep");
			}
			if (node != NULL) {
				node->type = c == '{' ? ALPACA_JSON_OBJECT : ALPACA_JSON_ARRAY;
			}
			stack[depth].container = node;
			stack[depth].last = NULL;
			stack[depth].object = c == '{';
			depth++;
			parser->position++;
			alpaca_json_skip_whitespace(parser);
			if (alpaca_json_peek(parser) == (c == '{' ? '}' : ']')) {
				parser->position++;
				depth--;
			} else {
				opened = true;
			}
		} else if (!alpaca_json_parse_scalar(parser, node)) {
			return false;
		}
		if (!opened) {
			bool more = false;
			while (depth > 0 && !more) {
				alpaca_json_skip_whitespace(parser);
				c = alpaca_json_peek(parser);
				if (c == ',') {
					parser->position++;
					more = true;
				} else if (c == (stack[depth - 1].object ? '}' : ']')) {
					parser->position++;
					depth--;
				} else {
					return alpaca_json_fail(parser, c < 0 ? "Unexpected end of text" : "Expected ',' or closing bracket");
				}
			}
			if (!more) {
				return true;
			}
		}
		name = NULL;
		if (stack[depth - 1].object) {
			if (!alpaca_json_parse_member_name(parser, build, &name)) {
				return false;
			}
		} else {
			alpaca_json_skip_whitespace(parser);
		}
	}
}

// Upper bound of the number of values in the text: every value but the root follows '[', '{' or ','.
static size_t alpaca_json_count_values(const char *text, size_t length) {
	size_t count = 1;
	bool string = false;
	for (size_t i = 0; i < length; i++) {
		char c = text[i];
		if (string) {
			if (c == '\\') {
				i++;
			} else if (c == '"') {
				string = false;
			}
		} else if (c == '"') {
			string = true;
		} else if (c == ',' || c == '[' || c == '{') {
			count++;
		}
	}
	return count;
}

static bool alpaca_json_start(alpaca_json_parser *parser, const char *text, size_t length, size_t limit, char *error, int error_size) {
	memset(parser, 0, sizeof(alpaca_json_parser));
	parser->text = text;
	parser->length = length;
	parser->error = error;
	parser->error_size = error_size;
	if (error != NULL && error_size > 0) {
		*error = 0;
	}
	if (text == NULL) {
		parser->length = 0;
		return alpaca_json_fail(parser, "No text");
	}
	if (length > limit) {
		return alpaca_json_fail(parser, "Text too long");
	}
	if (length >= 3 && (unsigned char)text[0] == 0xEF && (unsigned char)text[1] == 0xBB && (unsigned char)text[2] == 0xBF) {
		parser->position = 3;
	}
	return true;
}

static bool alpaca_json_finish(alpaca_json_parser *parser) {
	alpaca_json_skip_whitespace(parser);
	if (parser->position != parser->length) {
		return alpaca_json_fail(parser, "Unexpected text after value");
	}
	return true;
}

alpaca_json_value *alpaca_json_parse(const char *text, size_t length, char *error, int error_size) {
	alpaca_json_parser parser = { 0 };
	if (!alpaca_json_start(&parser, text, length, ALPACA_JSON_MAX_SIZE, error, error_size)) {
		return NULL;
	}
	parser.node_capacity = alpaca_json_count_values(text, length);
	parser.pool_size = length + 1;
	parser.nodes = indigo_safe_malloc(parser.node_capacity * sizeof(alpaca_json_value) + parser.pool_size);
	parser.pool = (char *)(parser.nodes + parser.node_capacity);
	if (!alpaca_json_parse_tree(&parser, true) || !alpaca_json_finish(&parser)) {
		indigo_safe_free(parser.nodes);
		return NULL;
	}
	return parser.nodes;
}

void alpaca_json_free(alpaca_json_value *root) {
	indigo_safe_free(root);
}

alpaca_json_type alpaca_json_get_type(const alpaca_json_value *value) {
	return value != NULL ? (alpaca_json_type)value->type : ALPACA_JSON_NONE;
}

bool alpaca_json_is_null(const alpaca_json_value *value) {
	return value != NULL && value->type == ALPACA_JSON_NULL;
}

static bool alpaca_json_is_container(const alpaca_json_value *value) {
	return value != NULL && (value->type == ALPACA_JSON_ARRAY || value->type == ALPACA_JSON_OBJECT);
}

int alpaca_json_size(const alpaca_json_value *value) {
	return alpaca_json_is_container(value) ? (int)value->count : 0;
}

static const alpaca_json_value *alpaca_json_find(const alpaca_json_value *object, const char *name, bool ignore_case) {
	if (object == NULL || object->type != ALPACA_JSON_OBJECT || name == NULL) {
		return NULL;
	}
	for (const alpaca_json_value *member = object->as.first; member != NULL; member = member->next) {
		if (alpaca_json_equal(member->name, name, ignore_case)) {
			return member;
		}
	}
	return NULL;
}

const alpaca_json_value *alpaca_json_get(const alpaca_json_value *object, const char *name) {
	return alpaca_json_find(object, name, true);
}

const alpaca_json_value *alpaca_json_get_exact(const alpaca_json_value *object, const char *name) {
	return alpaca_json_find(object, name, false);
}

const alpaca_json_value *alpaca_json_array_get(const alpaca_json_value *array, int index) {
	if (!alpaca_json_is_container(array) || index < 0 || (uint32_t)index >= array->count) {
		return NULL;
	}
	const alpaca_json_value *element = array->as.first;
	while (index-- > 0) {
		element = element->next;
	}
	return element;
}

const alpaca_json_value *alpaca_json_first(const alpaca_json_value *container) {
	return alpaca_json_is_container(container) ? container->as.first : NULL;
}

const alpaca_json_value *alpaca_json_next(const alpaca_json_value *value) {
	return value != NULL ? value->next : NULL;
}

const char *alpaca_json_get_name(const alpaca_json_value *value) {
	return value != NULL ? value->name : NULL;
}

bool alpaca_json_get_bool(const alpaca_json_value *value, bool *result) {
	if (value == NULL || value->type != ALPACA_JSON_BOOL) {
		return false;
	}
	if (result != NULL) {
		*result = value->as.boolean;
	}
	return true;
}

// Integer value of a double if it is integral and exactly representable.
static bool alpaca_json_to_integer(double number, int64_t *result) {
	if (!(number >= -ALPACA_JSON_EXACT_DOUBLE && number <= ALPACA_JSON_EXACT_DOUBLE) || number != floor(number)) {
		return false;
	}
	*result = (int64_t)number;
	return true;
}

bool alpaca_json_get_int(const alpaca_json_value *value, int64_t *result) {
	int64_t integer = 0;
	if (value == NULL || value->type != ALPACA_JSON_NUMBER) {
		return false;
	}
	if (value->integer) {
		integer = value->as.number.integer;
	} else if (!alpaca_json_to_integer(value->as.number.value, &integer)) {
		return false;
	}
	if (result != NULL) {
		*result = integer;
	}
	return true;
}

bool alpaca_json_get_double(const alpaca_json_value *value, double *result) {
	if (value == NULL || value->type != ALPACA_JSON_NUMBER) {
		return false;
	}
	if (result != NULL) {
		*result = value->as.number.value;
	}
	return true;
}

bool alpaca_json_get_string(const alpaca_json_value *value, const char **result) {
	if (value == NULL || value->type != ALPACA_JSON_STRING) {
		return false;
	}
	if (result != NULL) {
		*result = value->as.string.text;
	}
	return true;
}

size_t alpaca_json_get_string_length(const alpaca_json_value *value) {
	return value != NULL && value->type == ALPACA_JSON_STRING ? value->as.string.length : 0;
}

// Make room for one more element. No more than (remaining bytes / 2 + 1) elements can follow.
static bool alpaca_json_image_reserve(alpaca_json_parser *parser, alpaca_json_image_sink *sink) {
	if (!sink->growable) {
		return alpaca_json_fail(parser, "Image buffer too small");
	}
	size_t limit = sink->count + (parser->length - parser->position) / 2 + 1;
	size_t capacity = *sink->capacity < ALPACA_JSON_IMAGE_FIRST_CAPACITY / 2 ? ALPACA_JSON_IMAGE_FIRST_CAPACITY : *sink->capacity * 2;
	if (capacity > limit) {
		capacity = limit;
	}
	*sink->buffer = indigo_safe_realloc(*sink->buffer, capacity * (sink->format == ALPACA_JSON_IMAGE_INT32 ? sizeof(int32_t) : sizeof(double)));
	*sink->capacity = capacity;
	return true;
}

// Scan one number of the image and store it. Short plain integers, which is what cameras send, bypass the general scanner.
static bool alpaca_json_image_element(alpaca_json_parser *parser, alpaca_json_image_sink *sink) {
	const char *text = parser->text;
	size_t length = parser->length;
	size_t position = parser->position;
	int64_t integer = 0;
	double number = 0;
	bool negative = false;
	bool is_integer = false;
	if (position < length && text[position] == '-') {
		negative = true;
		position++;
	}
	size_t first = position;
	while (position < length && position - first < 10 && alpaca_json_is_digit(text[position])) {
		integer = integer * 10 + (text[position] - '0');
		position++;
	}
	if (position > first && (position - first == 1 || text[first] != '0') && (position == length || (text[position] != '.' && text[position] != 'e' && text[position] != 'E' && !alpaca_json_is_digit(text[position])))) {
		if (negative) {
			integer = -integer;
		}
		number = (double)integer;
		is_integer = true;
		parser->position = position;
	} else if (!alpaca_json_scan_number(parser, &number, &integer, &is_integer)) {
		return false;
	}
	if (sink->count >= *sink->capacity && !alpaca_json_image_reserve(parser, sink)) {
		return false;
	}
	if (sink->format == ALPACA_JSON_IMAGE_INT32) {
		if (!is_integer) {
			double nearest = round(number);
			if (!(nearest >= INT32_MIN && nearest <= INT32_MAX)) {
				return alpaca_json_fail(parser, "Image value out of range");
			}
			if (nearest != number) {
				sink->rounded++;
			}
			integer = (int64_t)nearest;
			number = nearest;
		} else if (integer < INT32_MIN || integer > INT32_MAX) {
			return alpaca_json_fail(parser, "Image value out of range");
		}
		((int32_t *)*sink->buffer)[sink->count] = (int32_t)integer;
	} else {
		((double *)*sink->buffer)[sink->count] = number;
	}
	if (sink->count == 0) {
		sink->minimum = number;
		sink->maximum = number;
	} else if (number < sink->minimum) {
		sink->minimum = number;
	} else if (number > sink->maximum) {
		sink->maximum = number;
	}
	sink->count++;
	return true;
}

static bool alpaca_json_accept(alpaca_json_parser *parser, char c) {
	alpaca_json_skip_whitespace(parser);
	if (alpaca_json_peek(parser) != c) {
		return false;
	}
	parser->position++;
	alpaca_json_skip_whitespace(parser);
	return true;
}

// Scan the planes of one pixel, Value[x][y], starting after its opening bracket.
static bool alpaca_json_image_planes(alpaca_json_parser *parser, alpaca_json_image_sink *sink) {
	int planes = 0;
	do {
		if (!alpaca_json_image_element(parser, sink)) {
			return false;
		}
		planes++;
	} while (alpaca_json_accept(parser, ','));
	if (!alpaca_json_accept(parser, ']')) {
		return alpaca_json_fail(parser, "Expected ',' or ']' in image array");
	}
	if (sink->dimensions[2] == 0) {
		sink->dimensions[2] = planes;
	} else if (planes != sink->dimensions[2]) {
		return alpaca_json_fail(parser, "Ragged image array");
	}
	return true;
}

// Scan one column, Value[x], starting after its opening bracket.
static bool alpaca_json_image_column(alpaca_json_parser *parser, alpaca_json_image_sink *sink) {
	int rows = 0;
	do {
		int rank = 2;
		if (alpaca_json_accept(parser, '[')) {
			rank = 3;
		}
		if (sink->rank == 0) {
			sink->rank = rank;
		} else if (sink->rank != rank) {
			return alpaca_json_fail(parser, "Mixed nesting in image array");
		}
		if (!(rank == 3 ? alpaca_json_image_planes(parser, sink) : alpaca_json_image_element(parser, sink))) {
			return false;
		}
		rows++;
	} while (alpaca_json_accept(parser, ','));
	if (!alpaca_json_accept(parser, ']')) {
		return alpaca_json_fail(parser, "Expected ',' or ']' in image array");
	}
	if (sink->dimensions[1] == 0) {
		sink->dimensions[1] = rows;
	} else if (rows != sink->dimensions[1]) {
		return alpaca_json_fail(parser, "Ragged image array");
	}
	return true;
}

// Scan Value starting at its opening bracket.
static bool alpaca_json_image_array(alpaca_json_parser *parser, alpaca_json_image_sink *sink) {
	int columns = 0;
	parser->position++;
	do {
		if (!alpaca_json_accept(parser, '[')) {
			return alpaca_json_fail(parser, "Image array is not of rank 2 or 3");
		}
		if (!alpaca_json_image_column(parser, sink)) {
			return false;
		}
		columns++;
	} while (alpaca_json_accept(parser, ','));
	if (!alpaca_json_accept(parser, ']')) {
		return alpaca_json_fail(parser, "Expected ',' or ']' in image array");
	}
	sink->dimensions[0] = columns;
	return true;
}

// Scan the value of one member of the envelope, starting at its first character.
static bool alpaca_json_image_member(alpaca_json_parser *parser, const char *name, alpaca_json_image_sink *sink, alpaca_json_image *image) {
	int c = alpaca_json_peek(parser);
	if (c == '[' && alpaca_json_equal(name, "Value", true)) {
		if (image->has_value) {
			return alpaca_json_fail(parser, "Duplicate Value");
		}
		image->has_value = true;
		return alpaca_json_image_array(parser, sink);
	}
	if (c == '"' && alpaca_json_equal(name, "ErrorMessage", true)) {
		alpaca_json_sink message = { image->error_message, sizeof(image->error_message), 0, false };
		return alpaca_json_scan_string(parser, &message);
	}
	bool is_type = alpaca_json_equal(name, "Type", true);
	bool is_rank = alpaca_json_equal(name, "Rank", true);
	bool is_error = alpaca_json_equal(name, "ErrorNumber", true);
	bool is_client = alpaca_json_equal(name, "ClientTransactionID", true);
	bool is_server = alpaca_json_equal(name, "ServerTransactionID", true);
	if ((c == '-' || alpaca_json_is_digit(c)) && (is_type || is_rank || is_error || is_client || is_server)) {
		double number = 0;
		int64_t integer = 0;
		bool is_integer = false;
		if (!alpaca_json_scan_number(parser, &number, &integer, &is_integer)) {
			return false;
		}
		if (!is_integer && !alpaca_json_to_integer(number, &integer)) {
			return true;
		}
		if (is_error) {
			image->has_error_number = true;
			image->error_number = integer;
		} else if (is_type && integer >= 0 && integer <= INT32_MAX) {
			image->type = (int)integer;
		} else if (is_rank && integer >= 0 && integer <= INT32_MAX) {
			image->rank = (int)integer;
		} else if (is_client && integer >= 0 && integer <= UINT32_MAX) {
			image->has_client_transaction_id = true;
			image->client_transaction_id = (uint32_t)integer;
		} else if (is_server && integer >= 0 && integer <= UINT32_MAX) {
			image->has_server_transaction_id = true;
			image->server_transaction_id = (uint32_t)integer;
		}
		return true;
	}
	return alpaca_json_parse_tree(parser, false);
}

bool alpaca_json_parse_image(const char *text, size_t length, alpaca_json_image_format format, void **buffer, size_t *capacity, bool growable, alpaca_json_image *image, char *error, int error_size) {
	alpaca_json_parser parser = { 0 };
	alpaca_json_image_sink sink = { 0 };
	if (!alpaca_json_start(&parser, text, length, ALPACA_JSON_IMAGE_MAX_SIZE, error, error_size)) {
		return false;
	}
	if (buffer == NULL || capacity == NULL || image == NULL || (format != ALPACA_JSON_IMAGE_INT32 && format != ALPACA_JSON_IMAGE_DOUBLE)) {
		return alpaca_json_fail(&parser, "Invalid argument");
	}
	if (*buffer == NULL) {
		*capacity = 0;
	}
	memset(image, 0, sizeof(alpaca_json_image));
	sink.format = format;
	sink.buffer = buffer;
	sink.capacity = capacity;
	sink.growable = growable;
	alpaca_json_skip_whitespace(&parser);
	if (alpaca_json_peek(&parser) != '{') {
		return alpaca_json_fail(&parser, "Expected an object");
	}
	parser.position++;
	bool more = !alpaca_json_accept(&parser, '}');
	while (more) {
		char name[ALPACA_JSON_NAME_SIZE] = { 0 };
		alpaca_json_sink name_sink = { name, sizeof(name), 0, false };
		alpaca_json_skip_whitespace(&parser);
		if (alpaca_json_peek(&parser) != '"') {
			return alpaca_json_fail(&parser, "Expected member name");
		}
		if (!alpaca_json_scan_string(&parser, &name_sink)) {
			return false;
		}
		if (!alpaca_json_accept(&parser, ':')) {
			return alpaca_json_fail(&parser, "Expected ':'");
		}
		if (!alpaca_json_image_member(&parser, name, &sink, image)) {
			return false;
		}
		if (alpaca_json_accept(&parser, '}')) {
			more = false;
		} else if (!alpaca_json_accept(&parser, ',')) {
			return alpaca_json_fail(&parser, alpaca_json_peek(&parser) < 0 ? "Unexpected end of text" : "Expected ',' or '}'");
		}
	}
	if (!alpaca_json_finish(&parser)) {
		return false;
	}
	if (image->has_value) {
		if (image->rank != 0 && image->rank != sink.rank) {
			return alpaca_json_fail(&parser, "Rank does not match Value");
		}
		image->rank = sink.rank;
		image->dimensions[0] = sink.dimensions[0];
		image->dimensions[1] = sink.dimensions[1];
		image->dimensions[2] = sink.rank == 3 ? sink.dimensions[2] : 0;
		image->count = sink.count;
		image->minimum = sink.minimum;
		image->maximum = sink.maximum;
		image->rounded = sink.rounded;
	}
	return true;
}
