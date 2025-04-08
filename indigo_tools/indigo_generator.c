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

typedef enum {
	TOKEN_EOF = 0, TOKEN_LBRACE = '{', TOKEN_RBRACE = '}', TOKEN_EQUAL = '=', TOKEN_SEMICOLON = ';', TOKEN_IDENTIFIER = 'I', TOKEN_STRING = 'S', TOKEN_NUMBER = 'N', TOKEN_TRUE = 'T', TOKEN_FALSE = 'F', TOKEN_CODE = 'C', TOKEN_COMMENT = '/', TOKEN_ERROR = 'E'
} token_type;

bool parsing_code = false;

char *definition_source, *current, *begin, *end;

token_type token(void) {
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
					return TOKEN_CODE;
				}
			}
			current++;
		}
	} else {
		while (true) {
			if (*current == 0) {
				return TOKEN_EOF;
			} else if (isspace(*current)) {
				current++;
			} else if (*current == '{') {
				current++;
				return TOKEN_LBRACE;
			} else if (*current == '}') {
				current++;
				return TOKEN_RBRACE;
			} else if (*current == '=') {
				current++;
				return TOKEN_EQUAL;
			} else if (*current == ';') {
				current++;
				return TOKEN_SEMICOLON;
			} else if (*current == '/' && *(current + 1) == '/') {
				while (*current != 0 && *current != '\n') {
					current++;
				}
				return TOKEN_COMMENT;
			} else if (*current == '"') {
				begin = ++current;
				while (*current != 0 && *current != '"') {
					current++;
				}
				end = current++;
				return TOKEN_STRING;
			} else if (strncmp(current, "true", 4) == 0) {
				current += 4;
				return TOKEN_TRUE;
			} else if (strncmp(current, "false", 5) == 0) {
				current += 4;
				return TOKEN_FALSE;
			} else if (isalpha(*current)) {
				begin = current++;
				while (isalnum(*current) || *current == '_') {
					current++;
				}
				end = current;
				return TOKEN_IDENTIFIER;
			} else if (isnumber(*current) || *current == '-') {
				begin = current++;
				while (isnumber(*current) || *current == '.' || *current == '-' || *current == 'E' || *current == 'e') {
					current++;
				}
				end = current;
				return TOKEN_NUMBER;
			}
		}
	}
	return TOKEN_ERROR;
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

int main(int argc, char **argv) {
	freopen(argv[1], "r", stdin);
	read_definition_source();
	token_type t;
	while ((t = token()) != TOKEN_EOF) {
		if (t == TOKEN_IDENTIFIER || t == TOKEN_STRING || t == TOKEN_NUMBER) {
			printf("%c %.*s\n", t, (int)(end - begin), begin);
		} else {
			printf("%c\n", t);
		}
	}
}
