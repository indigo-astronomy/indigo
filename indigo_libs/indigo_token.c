// Copyright (c) 2020 by Rumen G.Boganovski.
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
// 2.0 by Rumen G.Bogdanovski <rumen@skyrchive.org>

/** INDIGO Token handling
 \file indigo_token.c
 */

#if defined(INDIGO_WINDOWS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <string.h>
#include <stdio.h>
#include <indigo/indigo_token.h>
#include <indigo/indigo_bus.h>

typedef struct {
	char device[INDIGO_NAME_SIZE];
	indigo_token token;
} indigo_device_token;

static indigo_device_token tokens[MAX_TOKENS] = {0};
static indigo_token master_token = 0;

static pthread_mutex_t token_mutex = PTHREAD_MUTEX_INITIALIZER;

indigo_token indigo_string_to_token(const char *token_string) {
	indigo_token token = 0;
	if (token_string == NULL) return 0;
	if (sscanf(token_string, "%llx", &token) == 1) return token;
	return 0;
}

bool indigo_add_device_token(const char *device, indigo_token token) {
	if (device == NULL) return false;
	int slot = -1;
	pthread_mutex_lock(&token_mutex);
	for (int i = 0; i < MAX_TOKENS; i++) {
		if (tokens[i].device[0] == '\0') {
			if (slot < 0) slot = i;
		} else if (!strncmp(tokens[i].device, device, INDIGO_NAME_SIZE)) {
			slot = i;
			break;
		}
	}
	if (slot >= 0 && slot < MAX_TOKENS) {
		tokens[slot].token = token;
		indigo_copy_name(tokens[slot].device, device);
		pthread_mutex_unlock(&token_mutex);
		INDIGO_DEBUG(indigo_debug("ACL: Token for '%s' = 0x%x added at slot %d", device, token, slot));
		return true;
	}
	pthread_mutex_unlock(&token_mutex);
	INDIGO_DEBUG(indigo_debug("ACL: No slots available."));
	return false;
}

bool indigo_remove_device_token(const char *device) {
	if (device == NULL) return false;
	pthread_mutex_lock(&token_mutex);
	for (int i = 0; i < MAX_TOKENS; i++) {
		if (!strncmp(tokens[i].device, device, INDIGO_NAME_SIZE)) {
			tokens[i].token = 0;
			tokens[i].device[0] = '\0';
			pthread_mutex_unlock(&token_mutex);
			INDIGO_DEBUG(indigo_debug("ACL: Token for '%s' removed", device));
			return true;
		}
	}
	pthread_mutex_unlock(&token_mutex);
	INDIGO_DEBUG(indigo_debug("ACL: No token for '%s' to be removed", device));
	return false;
}

indigo_token indigo_get_device_token(const char *device) {
	if (device == NULL) return 0;
	pthread_mutex_lock(&token_mutex);
	for (int i = 0; i < MAX_TOKENS; i++) {
		if (!strncmp(tokens[i].device, device, INDIGO_NAME_SIZE)) {
			indigo_token token = tokens[i].token;
			pthread_mutex_unlock(&token_mutex);
			INDIGO_DEBUG(indigo_debug("ACL: Token found '%s' = 0x%x", device, token));
			return token;
		}
	}
	pthread_mutex_unlock(&token_mutex);
	//INDIGO_DEBUG(indigo_debug("ACL: No token for '%s'", device));
	return 0;
}

indigo_token indigo_get_device_or_master_token(const char *device) {
	indigo_token token = indigo_get_device_token(device);

	if (token != 0) return token;

	if (master_token != 0) {
		pthread_mutex_lock(&token_mutex);
		indigo_token token = master_token;
		pthread_mutex_unlock(&token_mutex);
		INDIGO_DEBUG(indigo_debug("ACL: Master token found '%s' = 0x%x", device, token));
		return token;
	}
	return 0;
}

indigo_token indigo_get_master_token(void) {
	pthread_mutex_lock(&token_mutex);
	indigo_token token = master_token;
	pthread_mutex_unlock(&token_mutex);
	return token;
}

void indigo_set_master_token(indigo_token token) {
	pthread_mutex_lock(&token_mutex);
	master_token = token;
	pthread_mutex_unlock(&token_mutex);
	INDIGO_DEBUG(indigo_debug("ACL: set master_token = 0x%x", master_token));
}

void indigo_clear_device_tokens(void) {
	pthread_mutex_lock(&token_mutex);
	memset(tokens, 0, sizeof(tokens));
	pthread_mutex_unlock(&token_mutex);
}

bool indigo_load_device_tokens_from_file(const char *file_name) {
	FILE* fp;
	char buffer[INDIGO_VALUE_SIZE + 50];

	fp = fopen(file_name, "r");
	if (fp == NULL) {
		INDIGO_ERROR(indigo_error("ACL: Can not open ACL file '%s'", file_name));
		return false;
	}

	INDIGO_DEBUG(indigo_debug("ACL: Loading ACL from file '%s'", file_name));
	int line_count = 0;
	while(fgets(buffer, INDIGO_NAME_SIZE + 50, fp)) {
		char device[INDIGO_NAME_SIZE];
		indigo_token token;

		line_count++;

		// skip lines starting with #
		if (buffer[0] == '#') continue;

		// remove training spaces
		int len = (int)strlen(buffer) - 1;
		if (len >= 0) {
			while (buffer[len] == ' ' || buffer[len] == '\t' || buffer[len] == '\n') len--;
			buffer[len + 1] = '\0';
		}
		// skip empty lines
		if (buffer[0] == '\0') continue;

		if (sscanf(buffer, "%llx %256[^\n]s", &token, device) == 2) {
			if (!strncmp(device, "@", 256)) {
				indigo_set_master_token(token);
			} else {
				indigo_add_device_token(device, token);
			}
		} else {
			fclose(fp);
			INDIGO_ERROR(indigo_error("ACL: Error in ACL file '%s' at line %d", file_name, line_count));
			return false;
		}
	}
	fclose(fp);
	return true;
}

bool indigo_save_device_tokens_to_file(const char *file_name) {
	FILE* fp;

	fp = fopen(file_name, "w");
	if (fp == NULL) {
		INDIGO_ERROR(indigo_error("ACL: Can not open ACL file '%s' for writing", file_name));
		return false;
	}

	INDIGO_DEBUG(indigo_debug("ACL: Saving ACL to file '%s'", file_name));
	fprintf(fp, "# Device ACL saved by INDIGO\n");
	for (int i = 0; i < MAX_TOKENS; i++) {
		if (tokens[i].device[0] != '\0') {
			if (fprintf(fp, "%8llx %s\n", tokens[i].token, tokens[i].device) < 0) {
				INDIGO_ERROR(indigo_error("ACL: Can not save ACL to file '%s'", file_name));
				fclose(fp);
				return false;
			}
		}
	}
	fclose(fp);
	return true;
}
