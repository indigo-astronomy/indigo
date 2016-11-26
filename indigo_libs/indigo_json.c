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
// 2.0 Build 0 - PoC by Peter Polakovic <peter.polakovic@cloudmakers.eu>

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

#include "indigo_json.h"

//#undef INDIGO_TRACE_PROTOCOL
//#define INDIGO_TRACE_PROTOCOL(c) c

static bool full_read(int handle, char *buffer, long length) {
	long remains = length;
	while (true) {
		long bytes_read = read(handle, buffer, remains);
		if (bytes_read <= 0) {
			return false;
		}
		if (bytes_read == remains) {
			return true;
		}
		buffer += bytes_read;
		remains -= bytes_read;
	}
}

static long ws_read(int handle, char *buffer, long length) {
	uint8_t header[14];
	if (!full_read(handle, (char *)header, 6))
		return -1;
	indigo_trace("ws_read -> %2x", header[0]);
	uint8_t *masking_key = header+2;
	uint64_t payload_length = header[1] & 0x7F;
	if (payload_length == 0x7E) {
		if (!full_read(handle, (char *)header + 6, 2))
			return -1;
		masking_key = header + 4;
		payload_length = ntohs(*((uint16_t *)(header+2)));
	} else if (payload_length == 0x7F) {
		if (!full_read(handle, (char *)header + 6, 8))
			return -1;
		masking_key = header+10;
		payload_length = ntohll(*((uint64_t *)(header+2)));
	}
	if (length < payload_length)
		return -1;
	if (!full_read(handle, buffer, payload_length))
		return -1;
	for (uint64_t i = 0; i < payload_length; i++) {
		buffer[i] ^= masking_key[i%4];
	}
	return payload_length;
}

static long stream_read(int handle, char *buffer, int length) {
	int i = 0;
	char c = '\0';
	long n = 0;
	while (i < length) {
		n = read(handle, &c, 1);
		if (n > 0 && c != '\r' && c != '\n') {
			buffer[i++] = c;
		} else {
			break;
		}
	}
	buffer[i] = '\0';
	return n == -1 ? -1 : i;
}

static indigo_property_state parse_state(char *value) {
	if (!strcmp(value, "Ok"))
		return INDIGO_OK_STATE;
	if (!strcmp(value, "Busy"))
		return INDIGO_BUSY_STATE;
	if (!strcmp(value, "Alert"))
		return INDIGO_ALERT_STATE;
	return INDIGO_IDLE_STATE;
}

static indigo_property_perm parse_perm(char *value) {
	if (!strcmp(value, "rw"))
		return INDIGO_RW_PERM;
	if (!strcmp(value, "wo"))
		return INDIGO_WO_PERM;
	return INDIGO_RO_PERM;
}

static indigo_rule parse_rule(char *value) {
	if (!strcmp(value, "OneOfMany"))
		return INDIGO_ONE_OF_MANY_RULE;
	if (!strcmp(value, "AtMostOne"))
		return INDIGO_AT_MOST_ONE_RULE;
	return INDIGO_ANY_OF_MANY_RULE;
}



void indigo_json_parse(indigo_device *device, indigo_client *client) {
	indigo_adapter_context *context = (indigo_adapter_context*)client->client_context;
	int input = context->input;
	char buffer[JSON_BUFFER_SIZE];
	while (true) {
		long size = context->web_socket ? ws_read(input, buffer, JSON_BUFFER_SIZE) : stream_read(input, buffer, JSON_BUFFER_SIZE);
		if (size == -1)
			break;
		
		buffer[size] = 0;
		indigo_log("%s", buffer);
		if (client->version == INDIGO_VERSION_NONE) {
			client->version = INDIGO_VERSION_CURRENT;
			indigo_enumerate_properties(client, &INDIGO_ALL_PROPERTIES);
		}
	}
}
