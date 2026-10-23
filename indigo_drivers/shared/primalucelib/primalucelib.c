// Copyright (c) 2026 Rumen G. Bogdanovski
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
// 3.0 by Rumen G. Bogdanovski <rumenastro@gmail.com>, shared library for all PrimaLuce Lab drivers;
//     the protocol code comes from indigo_focuser_primaluce by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** PrimaLuce Lab JSON protocol library
 \file primalucelib.c
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The jsmn implementation, compiled here with internal linkage; primalucelib.h includes only its types.
#define JSMN_STATIC
#define JSMN_STRICT
#define JSMN_PARENT_LINKS
#include "jsmn.h"

#include "primalucelib.h"

#pragma mark - Reply paths

char *PRIMALUCE_REPLY_MODEL_NAME[] = { "res", "get", "MODNAME", NULL };
char *PRIMALUCE_REPLY_SERIAL_NUMBER[] = { "res", "get", "SN", NULL };
char *PRIMALUCE_REPLY_APPLICATION_FIRMWARE[] = { "res", "get", "SWVERS", "SWAPP", NULL };
char *PRIMALUCE_REPLY_WEB_FIRMWARE[] = { "res", "get", "SWVERS", "SWWEB", NULL };
char *PRIMALUCE_REPLY_GET_ERROR[] = { "res", "get", "ERROR", NULL };
char *PRIMALUCE_REPLY_SET_ERROR[] = { "res", "set", "ERROR", NULL };
char *PRIMALUCE_REPLY_CMD_ERROR[] = { "res", "cmd", "ERROR", NULL };
char *PRIMALUCE_REPLY_EXTERNAL_TEMPERATURE[] = { "res", "get", "EXT_T", NULL };
char *PRIMALUCE_REPLY_SUPPLY_VOLTAGE[] = { "res", "get", "VIN_12V", NULL };
char *PRIMALUCE_REPLY_USB_VOLTAGE[] = { "res", "get", "VIN_USB", NULL };
char *PRIMALUCE_REPLY_LEDS[] = { "res", "get", "DIMLEDS", NULL };
char *PRIMALUCE_REPLY_LAN_CONFIG[] = { "res", "get", "LANCFG", NULL };
char *PRIMALUCE_REPLY_ACCESS_POINT_STATUS[] = { "res", "get", "WIFIAP", "STATUS", NULL };
char *PRIMALUCE_REPLY_ACCESS_POINT_SSID[] = { "res", "get", "WIFIAP", "SSID", NULL };
char *PRIMALUCE_REPLY_ACCESS_POINT_PASSWORD[] = { "res", "get", "WIFIAP", "PWD", NULL };
char *PRIMALUCE_REPLY_STATION_SSID[] = { "res", "get", "WIFISTA", "SSID", NULL };
char *PRIMALUCE_REPLY_STATION_PASSWORD[] = { "res", "get", "WIFISTA", "PWD", NULL };
char *PRIMALUCE_REPLY_RECOVERY_DELAY[] = { "res", "get", "RECOVER_DELAY", NULL };
char *PRIMALUCE_REPLY_MODEL_SUBMODEL[] = { "res", "srv", "GET_MODEL_SUBMODEL", NULL };
char *PRIMALUCE_REPLY_FIRST_STORED_POSITION[] = { "res", "get", "PRESET_1", NULL };
char *PRIMALUCE_REPLY_FOCUSER_CALIBRATION_RESTART[] = { "res", "get", "CALRESTART", "MOT1", NULL };
char *PRIMALUCE_REPLY_FOCUSER_ERROR[] = { "res", "get", "MOT1", "ERROR", NULL };
char *PRIMALUCE_REPLY_FOCUSER_POSITION_STEP[] = { "res", "get", "MOT1", "ABS_POS_STEP", NULL };
char *PRIMALUCE_REPLY_FOCUSER_POSITION_REGISTER[] = { "res", "get", "MOT1", "ABS_POS", NULL };
char *PRIMALUCE_REPLY_FOCUSER_MOTOR_STATE[] = { "res", "get", "MOT1", "STATUS", "MST", NULL };
char *PRIMALUCE_REPLY_FOCUSER_MOTOR_TEMPERATURE[] = { "res", "get", "MOT1", "NTC_T", NULL };
char *PRIMALUCE_REPLY_FOCUSER_CALIBRATION_MIN[] = { "res", "get", "MOT1", "CAL_MINPOS", NULL };
char *PRIMALUCE_REPLY_FOCUSER_CALIBRATION_MAX[] = { "res", "get", "MOT1", "CAL_MAXPOS", NULL };
char *PRIMALUCE_REPLY_FOCUSER_BACKLASH[] = { "res", "get", "MOT1", "BKLASH", NULL };
char *PRIMALUCE_REPLY_FOCUSER_SPEED[] = { "res", "get", "MOT1", "SPEED", NULL };
char *PRIMALUCE_REPLY_FOCUSER_HOLD_CURRENT_STATUS[] = { "res", "get", "MOT1", "HOLDCURR_STATUS", NULL };
char *PRIMALUCE_REPLY_FOCUSER_MOVE_ACK[] = { "res", "cmd", "MOT1", "STEP", NULL };
char *PRIMALUCE_REPLY_FOCUSER_GOTO_ACK[] = { "res", "cmd", "MOT1", "GOTO", NULL };
char *PRIMALUCE_REPLY_FOCUSER_ABORT_ACK[] = { "res", "cmd", "MOT1", "MOT_ABORT", NULL };
char *PRIMALUCE_REPLY_FOCUSER_STOP_ACK[] = { "res", "cmd", "MOT1", "MOT_STOP", NULL };
char *PRIMALUCE_REPLY_FOCUSER_FAST_INWARD_ACK[] = { "res", "cmd", "MOT1", "F_INW", NULL };
char *PRIMALUCE_REPLY_FOCUSER_FAST_OUTWARD_ACK[] = { "res", "cmd", "MOT1", "F_OUTW", NULL };
char *PRIMALUCE_REPLY_FOCUSER_BACKLASH_ACK[] = { "res", "set", "MOT1", "BKLASH", NULL };
char *PRIMALUCE_REPLY_FOCUSER_SPEED_ACK[] = { "res", "set", "MOT1", "SPEED", NULL };
char *PRIMALUCE_REPLY_FOCUSER_HOLD_CURRENT_ACK[] = { "res", "set", "MOT1", "HOLDCURR_STATUS", NULL };
char *PRIMALUCE_REPLY_FOCUSER_CALIBRATION_ACK[] = { "res", "cmd", "MOT1", "CAL_FOCUSER", NULL };
char *PRIMALUCE_REPLY_FOCUSER_CALIBRATION_DIR[] = { "res", "get", "MOT1", "CAL_DIR", NULL };
char *PRIMALUCE_REPLY_ROTATOR_CALIBRATION_RESTART[] = { "res", "get", "CALRESTART", "MOT2", NULL };
char *PRIMALUCE_REPLY_ROTATOR_ERROR[] = { "res", "get", "MOT2", "ERROR", NULL };
char *PRIMALUCE_REPLY_ROTATOR_SERIAL_NUMBER[] = { "res", "get", "ARCO_SN", NULL };
char *PRIMALUCE_REPLY_ROTATOR_POSITION_DEG[] = { "res", "get", "MOT2", "POSITION_DEG", NULL };
char *PRIMALUCE_REPLY_ROTATOR_ABS_POS_DEG[] = { "res", "get", "MOT2", "ABS_POS_DEG", NULL };
char *PRIMALUCE_REPLY_ROTATOR_ABS_POS_REGISTER[] = { "res", "get", "MOT2", "ABS_POS", NULL };
char *PRIMALUCE_REPLY_ROTATOR_MOTOR_STATE[] = { "res", "get", "MOT2", "STATUS", "MST", NULL };
char *PRIMALUCE_REPLY_ROTATOR_CALIBRATION_MIN[] = { "res", "get", "MOT2", "CAL_MINPOS", NULL };
char *PRIMALUCE_REPLY_ROTATOR_CALIBRATION_MAX[] = { "res", "get", "MOT2", "CAL_MAXPOS", NULL };
char *PRIMALUCE_REPLY_ROTATOR_CALIBRATION_STATUS[] = { "res", "get", "MOT2", "CAL_STATUS", NULL };
char *PRIMALUCE_REPLY_ROTATOR_CALIBRATION_ACK[] = { "res", "set", "MOT2", "CAL_STATUS", NULL };
char *PRIMALUCE_REPLY_ROTATOR_MOVE_ACK[] = { "res", "cmd", "MOT2", "STEP", NULL };
char *PRIMALUCE_REPLY_ROTATOR_SYNC_ACK[] = { "res", "cmd", "MOT2", "SYNC_POS", NULL };
char *PRIMALUCE_REPLY_ROTATOR_STOP_ACK[] = { "res", "cmd", "MOT2", "MOT_STOP", NULL };
char *PRIMALUCE_REPLY_PANEL_MAX_BRIGHTNESS[] = { "res", "get", "MAX_BRIGHTNESS", NULL };
char *PRIMALUCE_REPLY_PANEL_BRIGHTNESS[] = { "res", "get", "BRIGHTNESS", NULL };
char *PRIMALUCE_REPLY_PANEL_BRIGHTNESS_ACK[] = { "res", "set", "BRIGHTNESS", NULL };
char *PRIMALUCE_REPLY_PANEL_LIGHT[] = { "res", "get", "LIGHT", NULL };
char *PRIMALUCE_REPLY_PANEL_LIGHT_ACK[] = { "res", "set", "LIGHT", NULL };
char *PRIMALUCE_REPLY_COVER_POSITION[] = { "res", "get", "MOT1", "POSITION", NULL };
char *PRIMALUCE_REPLY_COVER_POSITION_ACK[] = { "res", "set", "MOT1", "POSITION", NULL };
char *PRIMALUCE_REPLY_COVER_MOTOR_STATE[] = { "res", "get", "MOT1", "STATUS", "MST", NULL };
char *PRIMALUCE_REPLY_COVER_STOP_ACK[] = { "res", "cmd", "MOT1", "MOT_STOP", NULL };
char *PRIMALUCE_REPLY_COVER_CALIBRATION_ACK[] = { "res", "cmd", "MOT1", "CAL_ALTO", NULL };

#pragma mark - Link

bool primalucelib_parse_http_url(const char *url, char *host, size_t host_size, int *port) {
	const char *separator = strchr(url, ':');
	if (separator == NULL || strncmp(separator, "://", 3)) {
		return false;
	}
	const char *start = separator + 3;
	size_t host_length = strcspn(start, ":/");
	if (host_length == 0 || host_length >= host_size) {
		return false;
	}
	memcpy(host, start, host_length);
	host[host_length] = 0;
	*port = start[host_length] == ':' ? atoi(start + host_length + 1) : PRIMALUCE_HTTP_DEFAULT_PORT;
	return true;
}

bool primalucelib_open(primalucelib_link *link, const char *driver_name, const char *port, const char **error_message) {
	link->driver_name = driver_name;
	link->restarting = link->probing = false;
	link->is_http = indigo_uni_is_url(port, "http");
	if (link->is_http) {
		// http://host[:port] or tcp://host[:port] reaches the controller on a WiFi network
		if (!primalucelib_parse_http_url(port, link->http_host, sizeof(link->http_host), &link->http_port)) {
			INDIGO_DRIVER_ERROR(link->driver_name, "Malformed URL '%s'", port);
			link->is_http = false;
			return false;
		}
		return true;
	}
	link->handle = indigo_uni_open_serial_with_speed2(port, PRIMALUCE_SERIAL_BAUDRATE, INDIGO_LOG_DEBUG, error_message);
	return link->handle != NULL;
}

bool primalucelib_is_open(primalucelib_link *link) {
	return link->handle != NULL || link->is_http;
}

bool primalucelib_is_http(primalucelib_link *link) {
	return link->is_http;
}

void primalucelib_close(primalucelib_link *link) {
	if (link->handle != NULL) {
		indigo_uni_close(&link->handle);
	}
	link->is_http = false;
}

// Over the network the controller serves the same JSON requests as GET /ajax.php?jreq=<request>, one request per connection.
static bool primalucelib_http_exchange(primalucelib_link *link, const char *request) {
	char query[3 * PRIMALUCE_MAX_RESPONSE_SIZE + 1];
	char *out = query;
	for (const unsigned char *in = (const unsigned char *)request; *in && out < query + sizeof(query) - 4; in++) {
		if (isalnum(*in) || strchr("-_.~", *in)) {
			*out++ = *in;
		} else {
			out += sprintf(out, "%%%02X", *in);
		}
	}
	*out = 0;
	indigo_uni_handle *handle = indigo_uni_open_client_socket_with_timeout(link->http_host, link->http_port, SOCK_STREAM, INDIGO_DELAY(PRIMALUCE_HTTP_TIMEOUT), INDIGO_LOG_DEBUG);
	if (handle == NULL) {
		INDIGO_DRIVER_ERROR(link->driver_name, "Can't connect to %s:%d", link->http_host, link->http_port);
		return false;
	}
	bool result = false;
	char reply[PRIMALUCE_MAX_RESPONSE_SIZE + 1024];
	long length = 0;
	if (indigo_uni_printf(handle, "GET " PRIMALUCE_HTTP_REQUEST_PATH "%s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", query, link->http_host) > 0) {
		char *body = NULL;
		long content_length = -1;
		// The controller keeps the connection open after the reply, so Content-Length ends the read.
		while (length < (long)sizeof(reply) - 1 && indigo_uni_wait_for_data(handle, INDIGO_DELAY(PRIMALUCE_HTTP_TIMEOUT)) > 0) {
			long count = indigo_uni_read_available(handle, reply + length, sizeof(reply) - 1 - length);
			if (count <= 0) {
				break;
			}
			length += count;
			reply[length] = 0;
			if (body == NULL && (body = strstr(reply, "\r\n\r\n")) != NULL) {
				char *field = strstr(reply, "\r\nContent-Length:");
				if (field != NULL && field < body) {
					content_length = atol(field + 17);
				}
			}
			if (body != NULL && content_length >= 0 && length >= body + 4 - reply + content_length) {
				break;
			}
		}
		reply[length] = 0;
		if (strncmp(reply, "HTTP/1.", 7) || strncmp(reply + 8, " 200 ", 5) || body == NULL) {
			INDIGO_DRIVER_ERROR(link->driver_name, "Unexpected HTTP response: %.40s", reply);
		} else if (strlen(body + 4) >= PRIMALUCE_MAX_RESPONSE_SIZE) {
			INDIGO_DRIVER_ERROR(link->driver_name, "HTTP response too long");
		} else {
			strcpy(link->response, body + 4);
			INDIGO_DRIVER_DEBUG(link->driver_name, "%s", link->response);
			result = *link->response != 0;
		}
	}
	indigo_uni_close(&handle);
	return result;
}

// The verb of a request, "get", "set", "cmd" or "srv", from its {"req":{"<verb>": prefix.
static bool primalucelib_request_verb(const char *request, char *verb, size_t size) {
	const char *start = strstr(request, "{\"req\":{\"");
	if (start == NULL) {
		return false;
	}
	start += 9;
	size_t length = strcspn(start, "\"");
	if (length == 0 || length >= size || start[length] != '"') {
		return false;
	}
	memcpy(verb, start, length);
	verb[length] = 0;
	return true;
}

// Parses the response buffer and NUL-terminates its string tokens. False for a quoted string or a
// reply that is not complete JSON.
static bool primalucelib_parse_reply(primalucelib_link *link) {
	memset(link->tokens, 0, sizeof(link->tokens));
	jsmn_init(&link->parser);
	if (*link->response == '"' || jsmn_parse(&link->parser, link->response, PRIMALUCE_MAX_RESPONSE_SIZE, link->tokens, PRIMALUCE_MAX_TOKENS) <= 0) {
		INDIGO_DRIVER_DEBUG(link->driver_name, "Failed to parse");
		return false;
	}
	for (int i = 0; i < PRIMALUCE_MAX_TOKENS; i++) {
		if (link->tokens[i].type == JSMN_UNDEFINED) {
			break;
		}
		if (link->tokens[i].type == JSMN_STRING) {
			link->response[link->tokens[i].end] = 0;
		}
	}
	return true;
}

// A reply answers a request when it carries the request verb or a refusal. Anything else is the
// late answer to an earlier request.
static bool primalucelib_reply_answers(primalucelib_link *link, char *verb) {
	char *verb_path[] = { PRIMALUCE_KEY_RESPONSE, verb, NULL };
	return primalucelib_reply_has(link, verb_path) || primalucelib_reply_has(link, PRIMALUCE_REPLY_CMD_ERROR) || primalucelib_reply_has(link, PRIMALUCE_REPLY_SET_ERROR) || primalucelib_reply_has(link, PRIMALUCE_REPLY_GET_ERROR);
}

bool primalucelib_vrequest(primalucelib_link *link, const char *format, va_list args) {
	char request[PRIMALUCE_MAX_RESPONSE_SIZE];
	char verb[16];
	link->last_error[0] = 0;
	if (link->restarting && !link->probing) {
		INDIGO_COPY_VALUE(link->last_error, "The controller is restarting");
		return false;
	}
	vsnprintf(request, sizeof(request), format, args);
	if (link->is_http) {
		if (!primalucelib_http_exchange(link, request)) {
			link->link_failed = true;
			return false;
		}
		// The controller answered, whatever the answer is.
		link->link_failed = false;
		if (!primalucelib_parse_reply(link)) {
			return false;
		}
	} else {
		// One request, one reply: anything still pending belongs to an earlier request, for example a
		// reply that arrived after its read timed out, and would shift every following reply by one.
		primalucelib_discard_input(link);
		if (indigo_uni_write(link->handle, request, (long)strlen(request)) <= 0) {
			link->link_failed = true;
			return false;
		}
		bool has_verb = primalucelib_request_verb(request, verb, sizeof(verb));
		int stale_replies = 0;
		while (true) {
			long result = indigo_uni_read_section(link->handle, link->response, PRIMALUCE_MAX_RESPONSE_SIZE - 1, "\n", "\r\n", link->probing ? INDIGO_DELAY(PRIMALUCE_RESTART_PROBE_TIMEOUT) : -1);
			if (result < 1) {
				link->link_failed = true;
				return false;
			}
			// Diagnostic output of the controller, not a reply.
			if (*link->response == '[') {
				INDIGO_DRIVER_DEBUG(link->driver_name, "Ignored");
				continue;
			}
			// The controller answered, whatever the answer is.
			link->link_failed = false;
			if (!primalucelib_parse_reply(link)) {
				return false;
			}
			if (has_verb && !primalucelib_reply_answers(link, verb)) {
				INDIGO_DRIVER_ERROR(link->driver_name, "Dropped a reply that answers another request");
				if (++stale_replies < PRIMALUCE_MAX_STALE_REPLIES) {
					continue;
				}
				return false;
			}
			break;
		}
	}
	// A rejected request is answered as an ERROR under the request verb
	char *error = primalucelib_reply_string(link, PRIMALUCE_REPLY_CMD_ERROR);
	if (error == NULL) {
		error = primalucelib_reply_string(link, PRIMALUCE_REPLY_SET_ERROR);
	}
	if (error == NULL) {
		error = primalucelib_reply_string(link, PRIMALUCE_REPLY_GET_ERROR);
	}
	if (error != NULL) {
		INDIGO_DRIVER_ERROR(link->driver_name, "Request rejected: %s", error);
		INDIGO_COPY_VALUE(link->last_error, error);
		return false;
	}
	return true;
}

bool primalucelib_request(primalucelib_link *link, const char *format, ...) {
	va_list args;
	va_start(args, format);
	bool result = primalucelib_vrequest(link, format, args);
	va_end(args);
	return result;
}

void primalucelib_discard_input(primalucelib_link *link) {
	indigo_uni_discard(link->handle);
}

bool primalucelib_link_failed(primalucelib_link *link) {
	return link->link_failed;
}

const char *primalucelib_last_error(primalucelib_link *link) {
	return link->last_error;
}

void primalucelib_clear_last_error(primalucelib_link *link) {
	link->last_error[0] = 0;
}

#pragma mark - Reply access

static int primalucelib_find_token(primalucelib_link *link, int start, char *path[]) {
	if (link->tokens[start].type != JSMN_OBJECT) {
		return -1;
	}
	int count = link->tokens[start].size;
	int index = start + 1;
	char *name = *path;
	for (int i = 0; i < count; i++) {
		if (link->tokens[index].type != JSMN_STRING) {
			return -1;
		}
		char *n = link->response + link->tokens[index].start;
		int l = link->tokens[index].end - link->tokens[index].start;
		if (strlen(name) == (size_t)l && !strncmp(n, name, l)) {
			index++;
			if (*++path == NULL) {
				return index;
			}
			return primalucelib_find_token(link, index, path);
		} else {
			while (true) {
				index++;
				if (link->tokens[index].type == JSMN_UNDEFINED) {
					return -1;
				}
				if (link->tokens[index].parent == start) {
					break;
				}
			}
		}
	}
	return -1;
}

int primalucelib_reply_token(primalucelib_link *link, char *path[]) {
	return primalucelib_find_token(link, 0, path);
}

bool primalucelib_reply_has(primalucelib_link *link, char *path[]) {
	return primalucelib_find_token(link, 0, path) != -1;
}

char *primalucelib_reply_string(primalucelib_link *link, char *path[]) {
	int index = primalucelib_find_token(link, 0, path);
	if (index == -1 || link->tokens[index].type != JSMN_STRING) {
		return NULL;
	}
	return link->response + link->tokens[index].start;
}

double primalucelib_reply_number(primalucelib_link *link, char *path[]) {
	int index = primalucelib_find_token(link, 0, path);
	if (index == -1) {
		return 0;
	}
	if (link->tokens[index].type == JSMN_PRIMITIVE || link->tokens[index].type == JSMN_STRING) {
		return atof(link->response + link->tokens[index].start);
	}
	return 0;
}

bool primalucelib_reply_is_done(primalucelib_link *link, char *path[]) {
	return primalucelib_is_done(primalucelib_reply_string(link, path));
}

#pragma mark - Values

bool primalucelib_is_done(const char *acknowledgement) {
	return acknowledgement != NULL && !strcmp(acknowledgement, PRIMALUCE_DONE);
}

bool primalucelib_is_power_supply_error(const char *acknowledgement) {
	return acknowledgement != NULL && !strcmp(acknowledgement, PRIMALUCE_POWER_SUPPLY_ERROR);
}

bool primalucelib_is_motor_stopped(const char *motor_state) {
	return motor_state != NULL && !strcmp(motor_state, PRIMALUCE_MOTOR_STOPPED);
}

bool primalucelib_is_rotator_calibration_finished(const char *calibration_status) {
	return calibration_status != NULL && !strcmp(calibration_status, PRIMALUCE_ROTATOR_CALIBRATION_STOPPED);
}

bool primalucelib_is_no_probe_temperature(double temperature) {
	return temperature <= PRIMALUCE_NO_PROBE_TEMPERATURE;
}

primalucelib_model primalucelib_model_from_name(const char *model_name) {
	if (model_name == NULL) {
		return PRIMALUCELIB_MODEL_UNKNOWN;
	}
	if (!strncmp(model_name, PRIMALUCE_MODEL_NAME_SESTOSENSO3, strlen(PRIMALUCE_MODEL_NAME_SESTOSENSO3))) {
		return PRIMALUCELIB_MODEL_SESTOSENSO3;
	}
	if (!strncmp(model_name, PRIMALUCE_MODEL_NAME_SESTOSENSO, strlen(PRIMALUCE_MODEL_NAME_SESTOSENSO))) {
		return PRIMALUCELIB_MODEL_SESTOSENSO;
	}
	if (!strncmp(model_name, PRIMALUCE_MODEL_NAME_ESATTO, strlen(PRIMALUCE_MODEL_NAME_ESATTO))) {
		return PRIMALUCELIB_MODEL_ESATTO;
	}
	if (!strncmp(model_name, PRIMALUCE_MODEL_NAME_GIOTTO, strlen(PRIMALUCE_MODEL_NAME_GIOTTO))) {
		return PRIMALUCELIB_MODEL_GIOTTO;
	}
	if (!strncmp(model_name, PRIMALUCE_MODEL_NAME_ALTO, strlen(PRIMALUCE_MODEL_NAME_ALTO))) {
		return PRIMALUCELIB_MODEL_ALTO;
	}
	return PRIMALUCELIB_MODEL_UNKNOWN;
}

bool primalucelib_model_is_focuser(primalucelib_model model) {
	return model == PRIMALUCELIB_MODEL_SESTOSENSO || model == PRIMALUCELIB_MODEL_SESTOSENSO3 || model == PRIMALUCELIB_MODEL_ESATTO;
}

bool primalucelib_model_is_sestosenso(primalucelib_model model) {
	return model == PRIMALUCELIB_MODEL_SESTOSENSO || model == PRIMALUCELIB_MODEL_SESTOSENSO3;
}

#pragma mark - Controller

bool primalucelib_get_state(primalucelib_link *link) {
	return primalucelib_request(link, PRIMALUCE_REQUEST_GET_STATE);
}

char *primalucelib_get_model_name(primalucelib_link *link) {
	return primalucelib_request(link, PRIMALUCE_REQUEST_GET_MODEL_NAME) ? primalucelib_reply_string(link, PRIMALUCE_REPLY_MODEL_NAME) : NULL;
}

char *primalucelib_get_firmware_version(primalucelib_link *link) {
	return primalucelib_request(link, PRIMALUCE_REQUEST_GET_FIRMWARE_VERSION) ? primalucelib_reply_string(link, PRIMALUCE_REPLY_APPLICATION_FIRMWARE) : NULL;
}

bool primalucelib_disable_log_output(primalucelib_link *link) {
	return primalucelib_request(link, PRIMALUCE_REQUEST_DISABLE_LOG_OUTPUT);
}

bool primalucelib_get_environment(primalucelib_link *link) {
	return primalucelib_request(link, PRIMALUCE_REQUEST_GET_ENVIRONMENT) && primalucelib_reply_has(link, PRIMALUCE_REPLY_EXTERNAL_TEMPERATURE);
}

double primalucelib_reply_external_temperature(primalucelib_link *link) {
	return primalucelib_reply_number(link, PRIMALUCE_REPLY_EXTERNAL_TEMPERATURE);
}

double primalucelib_reply_supply_voltage(primalucelib_link *link) {
	return primalucelib_reply_number(link, PRIMALUCE_REPLY_SUPPLY_VOLTAGE);
}

double primalucelib_reply_usb_voltage(primalucelib_link *link) {
	return primalucelib_reply_number(link, PRIMALUCE_REPLY_USB_VOLTAGE);
}

double primalucelib_reply_motor_temperature(primalucelib_link *link) {
	return primalucelib_reply_number(link, PRIMALUCE_REPLY_FOCUSER_MOTOR_TEMPERATURE);
}

bool primalucelib_set_leds(primalucelib_link *link, const char *mode) {
	return primalucelib_request(link, PRIMALUCE_REQUEST_SET_LEDS, mode);
}

char *primalucelib_get_lan_config(primalucelib_link *link) {
	return primalucelib_request(link, PRIMALUCE_REQUEST_GET_LAN_CONFIG) ? primalucelib_reply_string(link, PRIMALUCE_REPLY_LAN_CONFIG) : NULL;
}

bool primalucelib_set_lan_config(primalucelib_link *link, const char *mode) {
	return primalucelib_request(link, PRIMALUCE_REQUEST_SET_LAN_CONFIG, mode);
}

bool primalucelib_reboot(primalucelib_link *link) {
	return primalucelib_request(link, PRIMALUCE_REQUEST_REBOOT);
}

bool primalucelib_wait_for_restart(primalucelib_link *link) {
	// The restart prints a boot log on the serial line before the controller answers again.
	for (int i = 0; i < PRIMALUCE_RESTART_POLLS; i++) {
		indigo_usleep(INDIGO_DELAY(PRIMALUCE_RESTART_POLL_INTERVAL));
		primalucelib_discard_input(link);
		if (primalucelib_get_model_name(link) != NULL) {
			return true;
		}
	}
	INDIGO_DRIVER_ERROR(link->driver_name, "No answer after restart");
	return false;
}

void primalucelib_begin_restart(primalucelib_link *link) {
	link->restarting = true;
}

bool primalucelib_is_restarting(primalucelib_link *link) {
	return link->restarting;
}

bool primalucelib_probe_restart(primalucelib_link *link) {
	link->probing = true;
	bool answered = primalucelib_get_model_name(link) != NULL;
	if (answered && !link->is_http) {
		// The controller keeps the requests it received while it restarted and answers all of them once it is
		// back (a SESTO SENSO 3 answered 12 probes at once), which would outnumber PRIMALUCE_MAX_STALE_REPLIES for
		// the next request; they are read away until the line is quiet.
		for (int i = 0; i < PRIMALUCE_RESTART_POLLS && indigo_uni_read_section(link->handle, link->response, PRIMALUCE_MAX_RESPONSE_SIZE - 1, "\n", "\r\n", INDIGO_DELAY(PRIMALUCE_RESTART_PROBE_TIMEOUT)) > 0; i++) {
			INDIGO_DRIVER_DEBUG(link->driver_name, "Dropped a late reply after the restart");
		}
	}
	link->probing = false;
	if (answered) {
		link->restarting = false;
	}
	return answered;
}

void primalucelib_end_restart(primalucelib_link *link) {
	link->restarting = false;
}

bool primalucelib_set_access_point(primalucelib_link *link, bool on) {
	return primalucelib_request(link, PRIMALUCE_REQUEST_SET_ACCESS_POINT, on ? PRIMALUCE_ACCESS_POINT_ON : PRIMALUCE_ACCESS_POINT_OFF);
}

bool primalucelib_set_access_point_credentials(primalucelib_link *link, const char *ssid, const char *password) {
	return primalucelib_request(link, PRIMALUCE_REQUEST_SET_ACCESS_POINT_CREDENTIALS, ssid, password);
}

bool primalucelib_set_station_credentials(primalucelib_link *link, const char *ssid, const char *password) {
	return primalucelib_request(link, PRIMALUCE_REQUEST_SET_STATION_CREDENTIALS, ssid, password);
}

bool primalucelib_get_submodel(primalucelib_link *link, char *submodel, size_t size) {
	char *text;
	if (!primalucelib_request(link, PRIMALUCE_REQUEST_GET_MODEL_SUBMODEL) || (text = primalucelib_reply_string(link, PRIMALUCE_REPLY_MODEL_SUBMODEL)) == NULL) {
		return false;
	}
	// "Model = SESTOSENSO3, SubModel = SESTOSENSO3SC, ARCO = Not enabled"
	char *start = strstr(text, "SubModel = ");
	if (start == NULL || size == 0) {
		return false;
	}
	start += 11;
	size_t length = strcspn(start, ",");
	if (length >= size) {
		length = size - 1;
	}
	memcpy(submodel, start, length);
	submodel[length] = 0;
	return true;
}

bool primalucelib_get_recovery_delay(primalucelib_link *link, int *seconds) {
	if (!primalucelib_request(link, PRIMALUCE_REQUEST_GET_RECOVERY_DELAY) || !primalucelib_reply_has(link, PRIMALUCE_REPLY_RECOVERY_DELAY)) {
		return false;
	}
	*seconds = (int)primalucelib_reply_number(link, PRIMALUCE_REPLY_RECOVERY_DELAY);
	return true;
}

bool primalucelib_set_recovery_delay(primalucelib_link *link, int seconds) {
	char *path[] = { PRIMALUCE_KEY_RESPONSE, PRIMALUCE_VERB_SET, PRIMALUCE_KEY_RECOVERY_DELAY, NULL };
	return primalucelib_request(link, PRIMALUCE_REQUEST_SET_RECOVERY_DELAY, seconds) && primalucelib_reply_is_done(link, path);
}

#pragma mark - Focuser (MOT1)

bool primalucelib_focuser_request_position(primalucelib_link *link, bool position_register) {
	return primalucelib_request(link, position_register ? PRIMALUCE_REQUEST_FOCUSER_GET_POSITION_REGISTER : PRIMALUCE_REQUEST_FOCUSER_GET_POSITION_STEP);
}

bool primalucelib_reply_has_focuser_position(primalucelib_link *link, bool position_register) {
	return primalucelib_reply_has(link, position_register ? PRIMALUCE_REPLY_FOCUSER_POSITION_REGISTER : PRIMALUCE_REPLY_FOCUSER_POSITION_STEP);
}

double primalucelib_reply_focuser_position(primalucelib_link *link, bool position_register) {
	return primalucelib_reply_number(link, position_register ? PRIMALUCE_REPLY_FOCUSER_POSITION_REGISTER : PRIMALUCE_REPLY_FOCUSER_POSITION_STEP);
}

bool primalucelib_focuser_get_position(primalucelib_link *link, bool position_register, double *position) {
	if (!primalucelib_focuser_request_position(link, position_register) || !primalucelib_reply_has_focuser_position(link, position_register)) {
		return false;
	}
	*position = primalucelib_reply_focuser_position(link, position_register);
	return true;
}

bool primalucelib_focuser_request_motor_state(primalucelib_link *link) {
	return primalucelib_request(link, PRIMALUCE_REQUEST_FOCUSER_GET_MOTOR_STATE);
}

char *primalucelib_reply_focuser_motor_state(primalucelib_link *link) {
	return primalucelib_reply_string(link, PRIMALUCE_REPLY_FOCUSER_MOTOR_STATE);
}

bool primalucelib_focuser_move_to(primalucelib_link *link, bool use_goto, int position, char **acknowledgement) {
	*acknowledgement = NULL;
	if (!primalucelib_request(link, use_goto ? PRIMALUCE_REQUEST_FOCUSER_GOTO : PRIMALUCE_REQUEST_FOCUSER_MOVE_ABSOLUTE, position)) {
		return false;
	}
	*acknowledgement = primalucelib_reply_string(link, use_goto ? PRIMALUCE_REPLY_FOCUSER_GOTO_ACK : PRIMALUCE_REPLY_FOCUSER_MOVE_ACK);
	return true;
}

bool primalucelib_focuser_stop(primalucelib_link *link, char **reason) {
	char *state = NULL;
	*reason = NULL;
	if (primalucelib_request(link, PRIMALUCE_REQUEST_FOCUSER_ABORT) && (state = primalucelib_reply_string(link, PRIMALUCE_REPLY_FOCUSER_ABORT_ACK)) != NULL && primalucelib_is_done(state)) {
		return true;
	}
	if (!primalucelib_request(link, PRIMALUCE_REQUEST_FOCUSER_STOP)) {
		return false;
	}
	state = primalucelib_reply_string(link, PRIMALUCE_REPLY_FOCUSER_STOP_ACK);
	if (!primalucelib_is_done(state)) {
		*reason = state;
		return false;
	}
	return true;
}

bool primalucelib_focuser_fast_move(primalucelib_link *link, bool outward) {
	return primalucelib_request(link, outward ? PRIMALUCE_REQUEST_FOCUSER_FAST_OUTWARD : PRIMALUCE_REQUEST_FOCUSER_FAST_INWARD) && primalucelib_reply_is_done(link, outward ? PRIMALUCE_REPLY_FOCUSER_FAST_OUTWARD_ACK : PRIMALUCE_REPLY_FOCUSER_FAST_INWARD_ACK);
}

bool primalucelib_focuser_set_backlash(primalucelib_link *link, int steps) {
	return primalucelib_request(link, PRIMALUCE_REQUEST_FOCUSER_SET_BACKLASH, steps) && primalucelib_reply_is_done(link, PRIMALUCE_REPLY_FOCUSER_BACKLASH_ACK);
}

bool primalucelib_focuser_set_speed(primalucelib_link *link, int speed) {
	return primalucelib_request(link, PRIMALUCE_REQUEST_FOCUSER_SET_SPEED, speed) && primalucelib_reply_is_done(link, PRIMALUCE_REPLY_FOCUSER_SPEED_ACK);
}

bool primalucelib_focuser_set_hold_current(primalucelib_link *link, bool on) {
	return primalucelib_request(link, PRIMALUCE_REQUEST_FOCUSER_SET_HOLD_CURRENT, on ? 1 : 0) && primalucelib_reply_is_done(link, PRIMALUCE_REPLY_FOCUSER_HOLD_CURRENT_ACK);
}

static bool primalucelib_focuser_settings_done(primalucelib_link *link, const char *keys[], int count) {
	for (int i = 0; i < count; i++) {
		char *path[] = { PRIMALUCE_KEY_RESPONSE, PRIMALUCE_VERB_SET, PRIMALUCE_KEY_FOCUSER_MOTOR, (char *)keys[i], NULL };
		if (!primalucelib_reply_is_done(link, path)) {
			return false;
		}
	}
	return true;
}

bool primalucelib_focuser_set_motor_rates(primalucelib_link *link, int acceleration, int deceleration, int run) {
	const char *keys[] = { PRIMALUCE_KEY_ACCELERATION_SPEED, PRIMALUCE_KEY_DECELERATION_SPEED, PRIMALUCE_KEY_RUN_SPEED };
	return primalucelib_request(link, PRIMALUCE_REQUEST_FOCUSER_SET_MOTOR_RATES, acceleration, deceleration, run) && primalucelib_focuser_settings_done(link, keys, 3);
}

bool primalucelib_focuser_set_motor_currents(primalucelib_link *link, int acceleration, int deceleration, int run, int hold) {
	const char *keys[] = { PRIMALUCE_KEY_ACCELERATION_CURRENT, PRIMALUCE_KEY_DECELERATION_CURRENT, PRIMALUCE_KEY_RUN_CURRENT, PRIMALUCE_KEY_HOLD_CURRENT };
	return primalucelib_request(link, PRIMALUCE_REQUEST_FOCUSER_SET_MOTOR_CURRENTS, acceleration, deceleration, run, hold) && primalucelib_focuser_settings_done(link, keys, 4);
}

bool primalucelib_focuser_get_motor_config(primalucelib_link *link) {
	return primalucelib_request(link, PRIMALUCE_REQUEST_FOCUSER_GET_MOTOR_CONFIG);
}

static double primalucelib_reply_member(primalucelib_link *link, const char *node, const char *key) {
	char *path[] = { PRIMALUCE_KEY_RESPONSE, PRIMALUCE_VERB_GET, (char *)node, (char *)key, NULL };
	return primalucelib_reply_number(link, path);
}

void primalucelib_reply_motor_config(primalucelib_link *link, primalucelib_run_config *config) {
	config->acceleration_speed = primalucelib_reply_member(link, PRIMALUCE_KEY_FOCUSER_MOTOR, PRIMALUCE_KEY_ACCELERATION_SPEED);
	config->run_speed = primalucelib_reply_member(link, PRIMALUCE_KEY_FOCUSER_MOTOR, PRIMALUCE_KEY_RUN_SPEED);
	config->deceleration_speed = primalucelib_reply_member(link, PRIMALUCE_KEY_FOCUSER_MOTOR, PRIMALUCE_KEY_DECELERATION_SPEED);
	config->acceleration_current = primalucelib_reply_member(link, PRIMALUCE_KEY_FOCUSER_MOTOR, PRIMALUCE_KEY_ACCELERATION_CURRENT);
	config->run_current = primalucelib_reply_member(link, PRIMALUCE_KEY_FOCUSER_MOTOR, PRIMALUCE_KEY_RUN_CURRENT);
	config->deceleration_current = primalucelib_reply_member(link, PRIMALUCE_KEY_FOCUSER_MOTOR, PRIMALUCE_KEY_DECELERATION_CURRENT);
	config->hold_current = primalucelib_reply_member(link, PRIMALUCE_KEY_FOCUSER_MOTOR, PRIMALUCE_KEY_HOLD_CURRENT);
}

void primalucelib_reply_run_preset(primalucelib_link *link, const char *preset, primalucelib_run_config *config) {
	config->acceleration_speed = primalucelib_reply_member(link, preset, PRIMALUCE_KEY_PRESET_ACCELERATION_SPEED);
	config->run_speed = primalucelib_reply_member(link, preset, PRIMALUCE_KEY_PRESET_RUN_SPEED);
	config->deceleration_speed = primalucelib_reply_member(link, preset, PRIMALUCE_KEY_PRESET_DECELERATION_SPEED);
	config->acceleration_current = primalucelib_reply_member(link, preset, PRIMALUCE_KEY_PRESET_ACCELERATION_CURRENT);
	config->run_current = primalucelib_reply_member(link, preset, PRIMALUCE_KEY_PRESET_RUN_CURRENT);
	config->deceleration_current = primalucelib_reply_member(link, preset, PRIMALUCE_KEY_PRESET_DECELERATION_CURRENT);
	config->hold_current = primalucelib_reply_member(link, preset, PRIMALUCE_KEY_PRESET_HOLD_CURRENT);
}

bool primalucelib_focuser_apply_run_preset(primalucelib_link *link, const char *name) {
	return primalucelib_request(link, PRIMALUCE_REQUEST_FOCUSER_APPLY_RUN_PRESET, name);
}

bool primalucelib_focuser_apply_user_run_preset(primalucelib_link *link, int index) {
	return primalucelib_request(link, PRIMALUCE_REQUEST_FOCUSER_APPLY_USER_RUN_PRESET, index);
}

bool primalucelib_focuser_set_user_run_preset(primalucelib_link *link, int index, const primalucelib_run_config *config) {
	return primalucelib_request(link, PRIMALUCE_REQUEST_FOCUSER_SET_USER_RUN_PRESET, index, (int)config->acceleration_speed, (int)config->deceleration_speed, (int)config->run_speed, (int)config->acceleration_current, (int)config->deceleration_current, (int)config->run_current, (int)config->hold_current);
}

bool primalucelib_focuser_set_stored_position(primalucelib_link *link, int index, int position) {
	// The controller refuses a zero position, so it is never sent.
	if (position <= 0) {
		return false;
	}
	char key[16];
	snprintf(key, sizeof(key), PRIMALUCE_KEY_STORED_POSITION_PREFIX "%d", index);
	char *path[] = { PRIMALUCE_KEY_RESPONSE, PRIMALUCE_VERB_SET, key, PRIMALUCE_KEY_STORED_POSITION_STEPS, NULL };
	return primalucelib_request(link, PRIMALUCE_REQUEST_FOCUSER_SET_STORED_POSITION, index, position) && primalucelib_reply_is_done(link, path);
}

bool primalucelib_focuser_set_stored_position_name(primalucelib_link *link, int index, const char *name) {
	// The name is sent inside a JSON string without escaping.
	if (strchr(name, '"') != NULL || strchr(name, '\\') != NULL) {
		return false;
	}
	char key[16];
	snprintf(key, sizeof(key), PRIMALUCE_KEY_STORED_POSITION_PREFIX "%d", index);
	char *path[] = { PRIMALUCE_KEY_RESPONSE, PRIMALUCE_VERB_SET, key, PRIMALUCE_KEY_STORED_POSITION_NAME, NULL };
	return primalucelib_request(link, PRIMALUCE_REQUEST_FOCUSER_SET_STORED_NAME, index, name) && primalucelib_reply_is_done(link, path);
}

double primalucelib_reply_stored_position(primalucelib_link *link, int index) {
	char key[16];
	snprintf(key, sizeof(key), PRIMALUCE_KEY_STORED_POSITION_PREFIX "%d", index);
	return primalucelib_reply_member(link, key, PRIMALUCE_KEY_STORED_POSITION_STEPS);
}

char *primalucelib_reply_stored_position_name(primalucelib_link *link, int index) {
	char key[16];
	snprintf(key, sizeof(key), PRIMALUCE_KEY_STORED_POSITION_PREFIX "%d", index);
	char *path[] = { PRIMALUCE_KEY_RESPONSE, PRIMALUCE_VERB_GET, key, PRIMALUCE_KEY_STORED_POSITION_NAME, NULL };
	return primalucelib_reply_string(link, path);
}

bool primalucelib_focuser_calibration_step(primalucelib_link *link, const char *step) {
	return primalucelib_request(link, PRIMALUCE_REQUEST_FOCUSER_CALIBRATION_STEP, step);
}

bool primalucelib_focuser_get_calibration(primalucelib_link *link, double *min_position, double *max_position) {
	if (!primalucelib_request(link, PRIMALUCE_REQUEST_FOCUSER_GET_CALIBRATION) || !primalucelib_reply_has(link, PRIMALUCE_REPLY_FOCUSER_CALIBRATION_MAX)) {
		return false;
	}
	*min_position = primalucelib_reply_number(link, PRIMALUCE_REPLY_FOCUSER_CALIBRATION_MIN);
	*max_position = primalucelib_reply_number(link, PRIMALUCE_REPLY_FOCUSER_CALIBRATION_MAX);
	return true;
}

char *primalucelib_focuser_get_calibration_direction(primalucelib_link *link) {
	return primalucelib_request(link, PRIMALUCE_REQUEST_FOCUSER_GET_CALIBRATION_DIR) ? primalucelib_reply_string(link, PRIMALUCE_REPLY_FOCUSER_CALIBRATION_DIR) : NULL;
}

bool primalucelib_focuser_set_calibration_direction(primalucelib_link *link, const char *direction) {
	return primalucelib_request(link, PRIMALUCE_REQUEST_FOCUSER_SET_CALIBRATION_DIR, direction);
}

bool primalucelib_focuser_calibration_jog(primalucelib_link *link, int steps) {
	char step[32];
	snprintf(step, sizeof(step), steps < 0 ? "MoveIn-%d" : "MoveOut-%d", abs(steps));
	return primalucelib_focuser_calibration_step(link, step) && primalucelib_reply_is_done(link, PRIMALUCE_REPLY_FOCUSER_CALIBRATION_ACK);
}

#pragma mark - Rotator (ARCO, MOT2)

bool primalucelib_rotator_enable(primalucelib_link *link, bool on) {
	return primalucelib_request(link, PRIMALUCE_REQUEST_ROTATOR_ENABLE, on ? 1 : 0);
}

char *primalucelib_rotator_get_serial_number(primalucelib_link *link) {
	return primalucelib_request(link, PRIMALUCE_REQUEST_ROTATOR_GET_SERIAL_NUMBER) ? primalucelib_reply_string(link, PRIMALUCE_REPLY_ROTATOR_SERIAL_NUMBER) : NULL;
}

primalucelib_angle_source primalucelib_rotator_angle_source(bool has_position_deg, bool has_abs_pos_register) {
	return has_position_deg ? PRIMALUCELIB_ANGLE_POSITION_DEG : (has_abs_pos_register ? PRIMALUCELIB_ANGLE_ABS_POS_REGISTER : PRIMALUCELIB_ANGLE_ABS_POS_DEG);
}

double primalucelib_reply_rotator_angle(primalucelib_link *link, primalucelib_angle_source source) {
	switch (source) {
		case PRIMALUCELIB_ANGLE_POSITION_DEG:
			return primalucelib_reply_number(link, PRIMALUCE_REPLY_ROTATOR_POSITION_DEG);
		case PRIMALUCELIB_ANGLE_ABS_POS_REGISTER:
			return primalucelib_reply_number(link, PRIMALUCE_REPLY_ROTATOR_ABS_POS_REGISTER);
		default:
			return primalucelib_reply_number(link, PRIMALUCE_REPLY_ROTATOR_ABS_POS_DEG);
	}
}

bool primalucelib_rotator_get_angle(primalucelib_link *link, primalucelib_angle_source source, double *angle) {
	bool result;
	switch (source) {
		case PRIMALUCELIB_ANGLE_POSITION_DEG:
			result = primalucelib_request(link, PRIMALUCE_REQUEST_ROTATOR_GET_POSITION_DEG);
			break;
		case PRIMALUCELIB_ANGLE_ABS_POS_REGISTER:
			result = primalucelib_request(link, PRIMALUCE_REQUEST_ROTATOR_GET_ABS_POS_REGISTER);
			break;
		default:
			result = primalucelib_request(link, PRIMALUCE_REQUEST_ROTATOR_GET_ABS_POS_DEG);
			break;
	}
	if (result) {
		*angle = primalucelib_reply_rotator_angle(link, source);
	}
	return result;
}

char *primalucelib_reply_rotator_motor_state(primalucelib_link *link) {
	return primalucelib_reply_string(link, PRIMALUCE_REPLY_ROTATOR_MOTOR_STATE);
}

bool primalucelib_rotator_move_to(primalucelib_link *link, double angle, char **acknowledgement) {
	*acknowledgement = NULL;
	if (!primalucelib_request(link, PRIMALUCE_REQUEST_ROTATOR_MOVE_ABSOLUTE, angle)) {
		return false;
	}
	*acknowledgement = primalucelib_reply_string(link, PRIMALUCE_REPLY_ROTATOR_MOVE_ACK);
	return true;
}

bool primalucelib_rotator_sync(primalucelib_link *link, double angle) {
	return primalucelib_request(link, PRIMALUCE_REQUEST_ROTATOR_SYNC, angle) && primalucelib_reply_is_done(link, PRIMALUCE_REPLY_ROTATOR_SYNC_ACK);
}

bool primalucelib_rotator_stop(primalucelib_link *link) {
	return primalucelib_request(link, PRIMALUCE_REQUEST_ROTATOR_STOP) && primalucelib_reply_is_done(link, PRIMALUCE_REPLY_ROTATOR_STOP_ACK);
}

bool primalucelib_rotator_start_calibration(primalucelib_link *link) {
	return primalucelib_request(link, PRIMALUCE_REQUEST_ROTATOR_SET_CALIBRATION, PRIMALUCE_ROTATOR_CALIBRATION_RUNNING) && primalucelib_reply_is_done(link, PRIMALUCE_REPLY_ROTATOR_CALIBRATION_ACK);
}

bool primalucelib_rotator_stop_calibration(primalucelib_link *link) {
	return primalucelib_request(link, PRIMALUCE_REQUEST_ROTATOR_SET_CALIBRATION, PRIMALUCE_ROTATOR_CALIBRATION_STOPPED);
}

char *primalucelib_rotator_get_calibration_status(primalucelib_link *link) {
	return primalucelib_request(link, PRIMALUCE_REQUEST_ROTATOR_GET_CALIBRATION) ? primalucelib_reply_string(link, PRIMALUCE_REPLY_ROTATOR_CALIBRATION_STATUS) : NULL;
}

#pragma mark - Flat panel (GIOTTO)

bool primalucelib_panel_get_max_brightness(primalucelib_link *link, int *brightness) {
	if (!primalucelib_request(link, PRIMALUCE_REQUEST_PANEL_GET_MAX_BRIGHTNESS) || !primalucelib_reply_has(link, PRIMALUCE_REPLY_PANEL_MAX_BRIGHTNESS)) {
		return false;
	}
	*brightness = (int)primalucelib_reply_number(link, PRIMALUCE_REPLY_PANEL_MAX_BRIGHTNESS);
	return true;
}

bool primalucelib_panel_get_brightness(primalucelib_link *link, int *brightness) {
	if (!primalucelib_request(link, PRIMALUCE_REQUEST_PANEL_GET_BRIGHTNESS) || !primalucelib_reply_has(link, PRIMALUCE_REPLY_PANEL_BRIGHTNESS)) {
		return false;
	}
	*brightness = (int)primalucelib_reply_number(link, PRIMALUCE_REPLY_PANEL_BRIGHTNESS);
	return true;
}

bool primalucelib_panel_set_brightness(primalucelib_link *link, int brightness) {
	return primalucelib_request(link, PRIMALUCE_REQUEST_PANEL_SET_BRIGHTNESS, brightness) && primalucelib_reply_is_done(link, PRIMALUCE_REPLY_PANEL_BRIGHTNESS_ACK);
}

bool primalucelib_panel_get_light(primalucelib_link *link, bool *on) {
	if (!primalucelib_request(link, PRIMALUCE_REQUEST_PANEL_GET_LIGHT) || !primalucelib_reply_has(link, PRIMALUCE_REPLY_PANEL_LIGHT)) {
		return false;
	}
	*on = primalucelib_reply_number(link, PRIMALUCE_REPLY_PANEL_LIGHT) == 1;
	return true;
}

bool primalucelib_panel_set_light(primalucelib_link *link, bool on) {
	return primalucelib_request(link, PRIMALUCE_REQUEST_PANEL_SET_LIGHT, on ? 1 : 0) && primalucelib_reply_is_done(link, PRIMALUCE_REPLY_PANEL_LIGHT_ACK);
}

#pragma mark - Cover (ALTO)

bool primalucelib_cover_get_position(primalucelib_link *link, int *position) {
	if (!primalucelib_request(link, PRIMALUCE_REQUEST_COVER_GET_POSITION) || !primalucelib_reply_has(link, PRIMALUCE_REPLY_COVER_POSITION)) {
		return false;
	}
	*position = (int)primalucelib_reply_number(link, PRIMALUCE_REPLY_COVER_POSITION);
	return true;
}

bool primalucelib_cover_set_position(primalucelib_link *link, int position) {
	return primalucelib_request(link, PRIMALUCE_REQUEST_COVER_SET_POSITION, position) && primalucelib_reply_is_done(link, PRIMALUCE_REPLY_COVER_POSITION_ACK);
}

char *primalucelib_cover_get_motor_state(primalucelib_link *link) {
	return primalucelib_request(link, PRIMALUCE_REQUEST_COVER_GET_MOTOR_STATE) ? primalucelib_reply_string(link, PRIMALUCE_REPLY_COVER_MOTOR_STATE) : NULL;
}

bool primalucelib_cover_stop(primalucelib_link *link) {
	return primalucelib_request(link, PRIMALUCE_REQUEST_COVER_STOP) && primalucelib_reply_is_done(link, PRIMALUCE_REPLY_COVER_STOP_ACK);
}

bool primalucelib_cover_calibration_step(primalucelib_link *link, const char *step) {
	return primalucelib_request(link, PRIMALUCE_REQUEST_COVER_CALIBRATION_STEP, step) && primalucelib_reply_is_done(link, PRIMALUCE_REPLY_COVER_CALIBRATION_ACK);
}
