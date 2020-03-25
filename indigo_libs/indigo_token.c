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

#include <string.h>
#include <indigo/indigo_token.h>
#include <indigo/indigo_bus.h>


typedef struct {
	char device[INDIGO_NAME_SIZE];
	indigo_token token;
} indigo_device_token;

static indigo_device_token tokens[MAX_TOKENS] = {0};
static indigo_token master_token = 0;

static pthread_mutex_t token_mutex = PTHREAD_MUTEX_INITIALIZER;

bool indigo_add_device_token(const char *device, indigo_token token) {
	int slot = -1;
	for (int i = 0; i < MAX_TOKENS; i++) {
		if (tokens[i].device[0] == '\0') {
			slot = i;
		} else if (!strncmp(tokens[i].device, device, INDIGO_NAME_SIZE)) {
			slot = i;
			break;
		}
	}
	if (slot >= 0 && slot < MAX_TOKENS) {
		tokens[slot].token = token;
		strncpy(tokens[slot].device, device, INDIGO_NAME_SIZE);
		INDIGO_DEBUG(indigo_debug("INDIGO Bus: Token for '%s' = 0x%x added at slot %d", device, token, slot));
		return true;
	}
	INDIGO_DEBUG(indigo_debug("INDIGO Bus: No slots available."));
	return false;
}

bool indigo_remove_device_token(const char *device) {
	for (int i = 0; i < MAX_TOKENS; i++) {
		if (!strncmp(tokens[i].device, device, INDIGO_NAME_SIZE)) {
			tokens[i].token = 0;
			tokens[i].device[0] = '\0';
			INDIGO_DEBUG(indigo_debug("INDIGO Bus: Token for '%s' removed", device));
			return true;
		}
	}
	INDIGO_DEBUG(indigo_debug("INDIGO Bus: No token for '%s' to be removed", device));
	return false;
}

indigo_token indigo_get_device_token(const char *device) {
	if (master_token != 0) {
		INDIGO_DEBUG(indigo_debug("INDIGO Bus: Master token found '%s' = 0x%x", device, master_token));
		return master_token;
	}

	for (int i = 0; i < MAX_TOKENS; i++) {
		if (!strncmp(tokens[i].device, device, INDIGO_NAME_SIZE)) {
			indigo_token token = tokens[i].token;
			INDIGO_DEBUG(indigo_debug("INDIGO Bus: Token found '%s' = 0x%x", device, token));
			return token;
		}
	}
	INDIGO_DEBUG(indigo_debug("INDIGO Bus: No token for '%s'", device));
	return 0;
}

indigo_token indigo_get_master_token() {
	return master_token;
}

void indigo_set_master_token(indigo_token token) {
	master_token = token;
	INDIGO_DEBUG(indigo_debug("INDIGO Bus: set master_token = 0x%x", master_token));
}


void indigo_clear_device_tokens() {
	memset(tokens, 0, sizeof(tokens));
}
