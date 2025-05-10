// Copyright (c) 2021 CloudMakers, s. r. o.
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
// 2.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO ASCOM ALPACA bridge agent
 \file indigo_agent_alpaca.c
 */

#define DRIVER_VERSION 0x03000004
#define DRIVER_NAME	"indigo_agent_alpaca"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <errno.h>

#if defined(INDIGO_MACOS) || defined(INDIGO_LINUX)
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#elif defined(INDIGO_WINDOWS)
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#endif

#include <indigo/indigo_bus.h>
#include <indigo/indigo_server_tcp.h>

#include "indigo_agent_alpaca.h"
#include "indigo_alpaca_common.h"

#define PRIVATE_DATA													private_data

#define AGENT_DISCOVERY_PROPERTY							(PRIVATE_DATA->discovery_property)
#define AGENT_DISCOVERY_PORT_ITEM							(AGENT_DISCOVERY_PROPERTY->items+0)

#define AGENT_CAMERA_BAYERPAT_PROPERTY						(PRIVATE_DATA->camera_bayerpat_property)

#define AGENT_DEVICES_PROPERTY								(PRIVATE_DATA->devices_property)

#define DISCOVERY_REQUEST											"alpacadiscovery1"
#define DISCOVERY_RESPONSE										"{ \"AlpacaPort\":%d }"

typedef struct {
	indigo_property *discovery_property;
	indigo_property *devices_property;
	indigo_property *camera_bayerpat_property;
	indigo_timer *discovery_server_timer;
	pthread_mutex_t mutex;
} alpaca_agent_private_data;

static alpaca_agent_private_data *private_data = NULL;

#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
#define INVALID_SOCKET -1
static int discovery_server_socket = INVALID_SOCKET;
#elif defined(INDIGO_WINDOWS)
static SOCKET discovery_server_socket = INVALID_SOCKET;
#endif

static indigo_alpaca_device *alpaca_devices = NULL;
static int server_transaction_id = 0;

indigo_device *indigo_agent_alpaca_device = NULL;
indigo_client *indigo_agent_alpaca_client = NULL;

static void save_config(indigo_device *device) {
	if (pthread_mutex_trylock(&DEVICE_CONTEXT->config_mutex) == 0) {
		pthread_mutex_unlock(&DEVICE_CONTEXT->config_mutex);
		pthread_mutex_lock(&private_data->mutex);
		indigo_save_property(device, NULL, AGENT_DEVICES_PROPERTY);
		indigo_save_property(device, NULL, AGENT_CAMERA_BAYERPAT_PROPERTY);
		if (DEVICE_CONTEXT->property_save_file_handle != NULL) {
			CONFIG_PROPERTY->state = INDIGO_OK_STATE;
			indigo_uni_close(&DEVICE_CONTEXT->property_save_file_handle);
		} else {
			CONFIG_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		CONFIG_SAVE_ITEM->sw.value = false;
		indigo_update_property(device, CONFIG_PROPERTY, NULL);
		pthread_mutex_unlock(&private_data->mutex);
	}
}

// -------------------------------------------------------------------------------- ALPACA bridge implementation

#if defined(INDIGO_MACOS) || defined(INDIGO_LINUX)
#define LAST_ERROR	strerror(errno)
#elif defined(INDIGO_WINDOWS)
#define LAST_ERROR indigo_last_wsa_error()
#endif

static void start_discovery_server(indigo_device *device) {
	int port = (int)AGENT_DISCOVERY_PORT_ITEM->number.value;
	discovery_server_socket = socket(PF_INET, SOCK_DGRAM, 0);
	if (discovery_server_socket == INVALID_SOCKET) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to create socket (%s)", LAST_ERROR);
		return;
	}
	int reuse = 1;
	if (setsockopt(discovery_server_socket, SOL_SOCKET, SO_REUSEADDR, (char *) &reuse, sizeof(reuse)) < 0) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
		close(discovery_server_socket);
#elif defined(INDIGO_WINDOWS)
		closesocket(discovery_server_socket);
#endif
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "setsockopt() failed (%s)", LAST_ERROR);
		return;
	}
	struct sockaddr_in server_address;
	unsigned int server_address_length = sizeof(server_address);
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(port);
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(discovery_server_socket, (struct sockaddr *)&server_address, server_address_length) < 0) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
		close(discovery_server_socket);
#elif defined(INDIGO_WINDOWS)
		closesocket(discovery_server_socket);
#endif
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "bind() failed (%s)", LAST_ERROR);
		return;
	}
	INDIGO_DRIVER_LOG(DRIVER_NAME, "Discovery server started on port %d", port);
	fd_set readfd;
	struct sockaddr_in client_address;
	unsigned int client_address_length = sizeof(client_address);
	char buffer[128];
	struct timeval tv;
	while (discovery_server_socket != INVALID_SOCKET) {
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		FD_ZERO(&readfd);
		FD_SET(discovery_server_socket, &readfd);
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
		int ret = select(discovery_server_socket + 1, &readfd, NULL, NULL, &tv);
#elif defined(INDIGO_WINDOWS)
		int ret = select(0, &readfd, NULL, NULL, &tv);
#endif
		if (ret > 0) {
			if (FD_ISSET(discovery_server_socket, &readfd)) {
				recvfrom(discovery_server_socket, buffer, sizeof(buffer), 0, (struct sockaddr*)&client_address, &client_address_length);
				if (strstr(buffer, DISCOVERY_REQUEST)) {
					INDIGO_DRIVER_LOG(DRIVER_NAME, "Discovery request from %s", inet_ntoa(client_address.sin_addr));
					sprintf(buffer, DISCOVERY_RESPONSE, indigo_server_tcp_port);
					sendto(discovery_server_socket, buffer, (int)strlen(buffer), 0, (struct sockaddr*)&client_address, client_address_length);
				}
			}
		}
	}
	INDIGO_DRIVER_LOG(DRIVER_NAME, "Discovery server stopped on port %d", port);
	return;
}

static void shutdown_discovery_server() {
	if (discovery_server_socket) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
		shutdown(discovery_server_socket, SHUT_RDWR);
		close(discovery_server_socket);
#elif defined(INDIGO_WINDOWS)
		shutdown(discovery_server_socket, SD_BOTH);
		closesocket(discovery_server_socket);
#endif
		discovery_server_socket = INVALID_SOCKET;
	}
}

static void parse_url_params(char *params, int *client_id, int *client_transaction_id, int *id) {
	if (params == NULL) {
		return;
	}
	while (true) {
		char *token = strtok_r(params, "&", &params);
		if (token == NULL) {
			break;
		}
		if (!strncasecmp(token, "ClientID", 8)) {
			if ((token = strchr(token, '='))) {
				*client_id = atoi(token + 1);
			}
		} else if (!strncasecmp(token, "ClientTransactionID", 19)) {
			if ((token = strchr(token, '='))) {
				*client_transaction_id = atoi(token + 1);
			}
		} else if (id && !strncasecmp(token, "ID", 2)) {
			if ((token = strchr(token, '='))) {
				*id = (int)atol(token + 1);
			}
		}
	}
}

static void send_json_response(indigo_uni_handle *handle, char *path, int status_code, const char *status_text, char *body) {
	if (indigo_uni_printf(handle,
			"HTTP/1.1 %3d %s\r\n"
			"Content-Type: application/json\r\n"
			"Content-Length: %d\r\n"
			"\r\n"
			"%s", status_code, status_text, strlen(body), body)) {
		if (status_code == 200) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s -> 200 %s", path, status_text);
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s -> %3d %s", path, status_code, status_text);
		}
		INDIGO_DRIVER_TRACE(DRIVER_NAME, "%s", body);
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "% -> Failed", path);
	}
}

static void send_text_response(indigo_uni_handle *handle, char *path, int status_code, const char *status_text, char *body) {
	if (indigo_uni_printf(handle,
			"HTTP/1.1 %3d %s\r\n"
			"Content-Type: text/plain\r\n"
			"Content-Length: %d\r\n"
			"\r\n"
			"%s", status_code, status_text, strlen(body), body)) {
		if (status_code == 200) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s -> 200 %s", path, status_text);
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s -> %3d %s", path, status_code, status_text);
		}
		INDIGO_DRIVER_TRACE(DRIVER_NAME, "%s", body);
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "% -> Failed", path);
	}
}

static bool alpaca_setup_handler(indigo_uni_handle *handle, char *method, char *path, char *params) {
	if (indigo_uni_printf(handle,
			"HTTP/1.1 301 Moved Permanently\r\n"
			"Location: /mng.html\r\n"
			"Content-Type: text/plain\r\n"
			"Content-Length: 0\r\n"
			"\r\n"
												)) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "% -> OK", path);
	} else {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "% -> Failed", path);
	}
	return true;
}

static bool alpaca_apiversions_handler(indigo_uni_handle *handle, char *method, char *path, char *params) {
	int client_id = 0, client_transaction_id = 0;
	char buffer[128];
	parse_url_params(params, &client_id, &client_transaction_id, NULL);
	snprintf(buffer, sizeof(buffer), "{ \"Value\": [ 1 ], \"ClientTransactionID\": %u, \"ServerTransactionID\": %u }", client_transaction_id, server_transaction_id++);
	send_json_response(handle, path, 200, "OK", buffer);
	return true;
}

static bool alpaca_v1_description_handler(indigo_uni_handle *handle, char *method, char *path, char *params) {
	int client_id = 0, client_transaction_id = 0;
	char buffer[512];
	parse_url_params(params, &client_id, &client_transaction_id, NULL);
	snprintf(buffer, sizeof(buffer), "{ \"Value\": { \"ServerName\": \"INDIGO-Alpaca Bridge\", \"ServerVersion\": \"%d.%d-%s\", \"Manufacturer\": \"The INDIGO Initiative\", \"ManufacturerURL\": \"https://www.indigo-astronomy.org\" }, \"ClientTransactionID\": %u, \"ServerTransactionID\": %u }", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF, INDIGO_BUILD, client_transaction_id, server_transaction_id++);
	send_json_response(handle, path, 200, "OK", buffer);
	return true;
}

static bool alpaca_v1_configureddevices_handler(indigo_uni_handle *handle, char *method, char *path, char *params) {
	int client_id = 0, client_transaction_id = 0;
	char *buffer = indigo_alloc_large_buffer();
	parse_url_params(params, &client_id, &client_transaction_id, NULL);
	long index = snprintf(buffer, INDIGO_BUFFER_SIZE, "{ \"Value\": [ ");
	indigo_alpaca_device *alpaca_device = alpaca_devices;
	bool comma_needed = false;
	while (alpaca_device) {
		if (alpaca_device->device_type) {
			if (comma_needed) {
				buffer[index++] = ',';
				buffer[index++] = ' ';
			} else {
				comma_needed = true;
			}
			index += snprintf(buffer + index, INDIGO_BUFFER_SIZE - index, "{ \"DeviceName\": \"%s\", \"DeviceType\": \"%s\", \"DeviceNumber\": %d, \"UniqueID\": \"%s\" }", alpaca_device->device_name, alpaca_device->device_type, alpaca_device->device_number, alpaca_device->device_uid);
		}
		alpaca_device = alpaca_device->next;
	}
	snprintf(buffer + index, INDIGO_BUFFER_SIZE - index, "], \"ClientTransactionID\": %u, \"ServerTransactionID\": %u }", client_transaction_id, server_transaction_id++);
	send_json_response(handle, path, 200, "OK", buffer);
	indigo_free_large_buffer(buffer);
	return true;
}

int string_cmp(const void * a, const void * b) {
	 return strncasecmp((char *)a, (char *)b, 128);
}

static bool alpaca_v1_api_handler(indigo_uni_handle *handle, char *method, char *path, char *params) {
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "< %s %s %s", method, path, params);
	int client_id = 0, client_transaction_id = 0;
	int id = 0;
	char *device_type = strstr(path, "/api/v1/");
	if (device_type == NULL) {
		send_text_response(handle, path, 400, "Bad Request", "Wrong API prefix");
		return true;
	}
	device_type += 8;
	char *device_number = strchr(device_type, '/');
	if (device_number == NULL) {
		send_text_response(handle, path, 400, "Bad Request", "Missing device type");
		return true;
	}
	*device_number++ = 0;
	char *command = strchr(device_number, '/');
	if (command == NULL) {
		send_text_response(handle, path, 400, "Bad Request", "Missing device number");
		return true;
	}
	*command++ = 0;
	char *buffer = NULL;
	indigo_alpaca_device *alpaca_device = alpaca_devices;
	int number = atoi(device_number);
	while (alpaca_device) {
		if (alpaca_device->device_number == number) {
			if (alpaca_device->device_type && !strcasecmp(alpaca_device->device_type, device_type)) {
				break;
			}
			send_text_response(handle, path, 400, "Bad Request", "Device type doesn't match");
			return true;
		}
		alpaca_device = alpaca_device->next;
	}
	if (alpaca_device == NULL) {
		send_text_response(handle, path, 400, "Bad Request", "No such device");
		return true;
	}
	if (!strncmp(method, "GET", 3)) {
		parse_url_params(params, &client_id, &client_transaction_id, &id);
		if (!strncmp(command, "imagearray", 10)) {
			indigo_alpaca_ccd_get_imagearray(alpaca_device, 1, handle, client_transaction_id, server_transaction_id++, !strcmp(method, "GET/GZIP"), !strcmp(method, "GET/IMAGEBYTES"));
			return false;
		} else {
			buffer = indigo_alloc_large_buffer();
			long index = snprintf(buffer, INDIGO_BUFFER_SIZE, "{ ");
			long length = indigo_alpaca_get_command(alpaca_device, 1, command, id, buffer + index, INDIGO_BUFFER_SIZE - index);
			if (length > 0) {
				index += length;
				snprintf(buffer + index, INDIGO_BUFFER_SIZE - index, ", \"ClientTransactionID\": %u, \"ServerTransactionID\": %u }", client_transaction_id, server_transaction_id++);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "> %s", buffer);
				send_json_response(handle, path, 200, "OK", buffer);
			} else {
				send_text_response(handle, path, 400, "Bad Request", "Unrecognised command");
			}
		}
	} else if (!strcmp(method, "PUT")) {
		int content_length = 0;
		buffer = indigo_alloc_large_buffer();
		while (indigo_uni_read_line(handle, buffer, INDIGO_BUFFER_SIZE) > 0) {
			if (!strncasecmp(buffer, "Content-Length:", 15)) {
				content_length = atoi(buffer + 15);
			}
		}
		indigo_uni_read_line(handle, buffer, content_length);
		buffer[content_length] = 0;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "< %s", buffer);
		char *params = buffer;
		char args[5][128] = { 0 };
		int count = 0;
		while (true) {
			char *token = strtok_r(params, "&", &params);
			if (token == NULL) {
				break;
			}
			if (!strncmp(token, "ClientID", 8)) {
				if ((token = strchr(token, '='))) {
					client_id = atoi(token + 1);
				}
			} else if (!strncmp(token, "ClientTransactionID", 19)) {
				if ((token = strchr(token, '='))) {
					client_transaction_id = atoi(token + 1);
				}
			} else if (count < 5) {
				strncpy(args[count++], token, 128);
			}
		}
		if (count > 1) {
			qsort(args, count, 128, string_cmp);
		}
		long index = snprintf(buffer, INDIGO_BUFFER_SIZE, "{ ");
		long length = indigo_alpaca_set_command(alpaca_device, 1, command, buffer + index, INDIGO_BUFFER_SIZE - index, args[0], args[1]);
		if (length > 0) {
			index += length;
			snprintf(buffer + index, INDIGO_BUFFER_SIZE - index, ", \"ClientTransactionID\": %u, \"ServerTransactionID\": %u }", client_transaction_id, server_transaction_id++);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "> %s", buffer);
			send_json_response(handle, path, 200, "OK", buffer);
		} else {
			send_text_response(handle, path, 400, "Bad Request", "Unrecognised command");
		}
		
	} else {
		send_text_response(handle, path, 400, "Bad Request", "Invalid method");
	}
	if (buffer) {
		indigo_free_large_buffer(buffer);
	}
	return true;
}

// -------------------------------------------------------------------------------- INDIGO agent device implementation

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result agent_device_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_device_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AGENT) == INDIGO_OK) {
		// --------------------------------------------------------------------------------
		AGENT_DISCOVERY_PROPERTY = indigo_init_number_property(NULL, device->name, "AGENT_ALPACA_DISCOVERY", MAIN_GROUP, "Discovery Configuration", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (AGENT_DISCOVERY_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AGENT_DISCOVERY_PORT_ITEM, "PORT", "Discovery port", 0, 0xFFFF, 0, 32227);
		AGENT_DEVICES_PROPERTY = indigo_init_text_property(NULL, device->name, "AGENT_ALPACA_DEVICES", MAIN_GROUP, "Device mapping", INDIGO_OK_STATE, INDIGO_RW_PERM, ALPACA_MAX_ITEMS);
		if (AGENT_DISCOVERY_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		for (int i = 0; i < ALPACA_MAX_ITEMS; i++) {
			sprintf(AGENT_DEVICES_PROPERTY->items[i].name, "%d", i);
			sprintf(AGENT_DEVICES_PROPERTY->items[i].label, "Device #%d", i);
		}
		AGENT_DEVICES_PROPERTY->count = 0;

		AGENT_CAMERA_BAYERPAT_PROPERTY = indigo_init_text_property(NULL, device->name, "AGENT_ALPACA_CAMERA_BAYERPAT", MAIN_GROUP, "Camera Bayer pattern", INDIGO_OK_STATE, INDIGO_RW_PERM, ALPACA_MAX_ITEMS);
		if (AGENT_CAMERA_BAYERPAT_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		for (int i = 0; i < ALPACA_MAX_ITEMS; i++) {
			AGENT_CAMERA_BAYERPAT_PROPERTY->items[i].name[0] = '\0';
			AGENT_CAMERA_BAYERPAT_PROPERTY->items[i].label[0] = '\0';
			AGENT_CAMERA_BAYERPAT_PROPERTY->items[i].text.value[0] = '\0';
		}
		AGENT_CAMERA_BAYERPAT_PROPERTY->count = 0;
		// --------------------------------------------------------------------------------
		srand((unsigned)time(0));
		indigo_set_timer(device, 0, start_discovery_server, &private_data->discovery_server_timer);
		indigo_server_add_handler("/setup", &alpaca_setup_handler);
		indigo_server_add_handler("/management/apiversions", &alpaca_apiversions_handler);
		indigo_server_add_handler("/management/v1/description", &alpaca_v1_description_handler);
		indigo_server_add_handler("/management/v1/configureddevices", &alpaca_v1_configureddevices_handler);
		indigo_server_add_handler("/api/v1", &alpaca_v1_api_handler);
		CONNECTION_PROPERTY->hidden = true;
		CONFIG_PROPERTY->hidden = true;
		PROFILE_PROPERTY->hidden = true;
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return agent_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (client == indigo_agent_alpaca_client) {
		return INDIGO_OK;
	}
	INDIGO_DEFINE_MATCHING_PROPERTY(AGENT_DISCOVERY_PROPERTY);
	INDIGO_DEFINE_MATCHING_PROPERTY(AGENT_DEVICES_PROPERTY);
	INDIGO_DEFINE_MATCHING_PROPERTY(AGENT_CAMERA_BAYERPAT_PROPERTY);
	return indigo_device_enumerate_properties(device, client, property);
}

static indigo_result agent_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (client == indigo_agent_alpaca_client) {
		return INDIGO_OK;
	}
	if (indigo_property_match(AGENT_DISCOVERY_PROPERTY, property)) {
		indigo_property_copy_values(AGENT_DISCOVERY_PROPERTY, property, false);
		shutdown_discovery_server();
		indigo_set_timer(device, 0, start_discovery_server, &private_data->discovery_server_timer);
		AGENT_DISCOVERY_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_DISCOVERY_PROPERTY, NULL);
	} else if (indigo_property_match(AGENT_DEVICES_PROPERTY, property)) {
		int count = AGENT_DEVICES_PROPERTY->count;
		AGENT_DEVICES_PROPERTY->count = ALPACA_MAX_ITEMS;
		indigo_property_copy_values(AGENT_DEVICES_PROPERTY, property, false);
		for (int i = ALPACA_MAX_ITEMS; i; i--) {
			indigo_item *item = AGENT_DEVICES_PROPERTY->items + i - 1;
			if (*item->text.value) {
				AGENT_DEVICES_PROPERTY->count = i;
				break;
			}
		}
		AGENT_DEVICES_PROPERTY->state = INDIGO_OK_STATE;
		if (count == AGENT_DEVICES_PROPERTY->count) {
			indigo_update_property(device, AGENT_DEVICES_PROPERTY, NULL);
		} else {
			indigo_delete_property(device, AGENT_DEVICES_PROPERTY, NULL);
			indigo_define_property(device, AGENT_DEVICES_PROPERTY, NULL);
		}
		save_config(device);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_CAMERA_BAYERPAT_PROPERTY, property)) {
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = property->items + i;
			if (!get_bayer_RGGB_offsets(item->text.value, NULL, NULL) && item->text.value[0] != '\0') {
				AGENT_CAMERA_BAYERPAT_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, AGENT_CAMERA_BAYERPAT_PROPERTY, "Bayer pattern '%s' is not supported", item->text.value);
				return INDIGO_OK;
			}
		}
		indigo_property_copy_values(AGENT_CAMERA_BAYERPAT_PROPERTY, property, false);
		AGENT_CAMERA_BAYERPAT_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_CAMERA_BAYERPAT_PROPERTY, NULL);
		save_config(device);
		return INDIGO_OK;
	}
	return indigo_device_change_property(device, client, property);
}

static indigo_result agent_device_detach(indigo_device *device) {
	assert(device != NULL);
	shutdown_discovery_server();
	indigo_server_remove_resource("/setup");
	indigo_server_remove_resource("/management/apiversions");
	indigo_server_remove_resource("/management/v1/description");
	indigo_server_remove_resource("/management/v1/configureddevices");
	indigo_server_remove_resource("/api/v1");
	indigo_cancel_timer_sync(device, &private_data->discovery_server_timer);
	indigo_release_property(AGENT_DISCOVERY_PROPERTY);
	indigo_release_property(AGENT_DEVICES_PROPERTY);
	indigo_release_property(AGENT_CAMERA_BAYERPAT_PROPERTY);
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	return indigo_device_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO agent client implementation

static indigo_result agent_define_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (device == indigo_agent_alpaca_device) {
		return INDIGO_OK;
	}
	indigo_alpaca_device *alpaca_device = alpaca_devices;
	while (alpaca_device) {
		if (!strcmp(property->device, alpaca_device->indigo_device))
			break;
		alpaca_device = alpaca_device->next;
	}
	if (alpaca_device == NULL) {
		unsigned char digest[15] = { 0 };
		for (int i = 0, j = 0; property->device[i]; i++, j = (j + 1) % 15) {
			digest[j] = digest[j] ^ property->device[i];
		}
		alpaca_device = indigo_safe_malloc(sizeof(indigo_alpaca_device));
		strcpy(alpaca_device->indigo_device, property->device);
		alpaca_device->device_number = -1;
		strcpy(alpaca_device->device_uid, "xxxxxxxx-xxxx-4xxx-8xxx-xxxxxxxxxxxx");
		static char *hex = "0123456789ABCDEF";
		int i = 0;
		for (char *c = alpaca_device->device_uid; *c; c++) {
			if (*c == 'x') {
				int r = i % 2 == 0 ? digest[i / 2] % 15 : digest[i / 2] / 15;
				*c = hex[r];
				i++;
			}
		}
		pthread_mutex_init(&alpaca_device->mutex, NULL);
		alpaca_device->next = alpaca_devices;
		alpaca_devices = alpaca_device;
	}
	if (!strcmp(property->name, INFO_PROPERTY_NAME)) {
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = property->items + i;
			if (!strcmp(item->name, INFO_DEVICE_INTERFACE_ITEM_NAME)) {
				alpaca_device->indigo_interface = (indigo_device_interface)atol(item->text.value);
				if (IS_DEVICE_TYPE(alpaca_device, INDIGO_INTERFACE_AGENT)) {
					alpaca_device->device_type = NULL;
				} else if (IS_DEVICE_TYPE(alpaca_device, INDIGO_INTERFACE_CCD)) {
					alpaca_device->ccd.ccdtemperature = NAN;
					alpaca_device->device_type = "Camera";
				} else if (IS_DEVICE_TYPE(alpaca_device, INDIGO_INTERFACE_DOME)) {
					alpaca_device->device_type = "Dome";
				} else if (IS_DEVICE_TYPE(alpaca_device, INDIGO_INTERFACE_WHEEL)) {
					alpaca_device->device_type = "FilterWheel";
				} else if (IS_DEVICE_TYPE(alpaca_device, INDIGO_INTERFACE_FOCUSER)) {
					alpaca_device->device_type = "Focuser";
				} else if (IS_DEVICE_TYPE(alpaca_device, INDIGO_INTERFACE_ROTATOR)) {
					alpaca_device->device_type = "Rotator";
				} else if (IS_DEVICE_TYPE(alpaca_device, INDIGO_INTERFACE_AUX_POWERBOX) || IS_DEVICE_TYPE(alpaca_device, INDIGO_INTERFACE_AUX_GPIO)) {
					alpaca_device->device_type = "Switch";
				} else if (IS_DEVICE_TYPE(alpaca_device, INDIGO_INTERFACE_AO) || IS_DEVICE_TYPE(alpaca_device, INDIGO_INTERFACE_MOUNT) || IS_DEVICE_TYPE(alpaca_device, INDIGO_INTERFACE_GUIDER)) {
					alpaca_device->device_type = "Telescope";
				} else if (IS_DEVICE_TYPE(alpaca_device, INDIGO_INTERFACE_AUX_LIGHTBOX)) {
					alpaca_device->device_type = "CoverCalibrator";
				} else {
					alpaca_device->device_type = NULL;
				}
				if (alpaca_device->device_type) {
					int device_number;
					for (device_number = 0; device_number < AGENT_DEVICES_PROPERTY->count; device_number++) {
						indigo_item *item = AGENT_DEVICES_PROPERTY->items + device_number;
						if (!strcmp(property->device, item->text.value)) {
							alpaca_device->device_number = device_number;
							break;
						}
					}
					if (alpaca_device->device_number < 0) {
						for (device_number = 0; device_number < AGENT_DEVICES_PROPERTY->count; device_number++) {
							item = AGENT_DEVICES_PROPERTY->items + device_number;
							if (*AGENT_DEVICES_PROPERTY->items[device_number].text.value == 0) {
								break;
							}
						}
						if (device_number < ALPACA_MAX_ITEMS) {
							indigo_item *item = AGENT_DEVICES_PROPERTY->items + device_number;
							strcpy(item->text.value, property->device);
							alpaca_device->device_number = device_number;
							indigo_debug("Device %s mapped to #%d", property->device, device_number);
							indigo_delete_property(indigo_agent_alpaca_device, AGENT_DEVICES_PROPERTY, NULL);
							if (device_number == AGENT_DEVICES_PROPERTY->count) {
								AGENT_DEVICES_PROPERTY->count++;
							}
							indigo_define_property(indigo_agent_alpaca_device, AGENT_DEVICES_PROPERTY, NULL);
							save_config(indigo_agent_alpaca_device);
						} else {
							indigo_send_message(indigo_agent_alpaca_device, "Too many Alpaca devices configured");
						}
					}
					if (IS_DEVICE_TYPE(alpaca_device, INDIGO_INTERFACE_CCD)) {
						int cam_number;
						for (cam_number = 0; cam_number < AGENT_CAMERA_BAYERPAT_PROPERTY->count; cam_number++) {
							indigo_item *item = AGENT_CAMERA_BAYERPAT_PROPERTY->items + cam_number;
							if (!strcmp(property->device, item->label)) {
								indigo_debug("=== Camera %s already mapped to #%d", property->device, cam_number);
								break;
							}
						}
						if (cam_number == AGENT_CAMERA_BAYERPAT_PROPERTY->count) {
							indigo_debug("+++ Mapping camera %s to #%d", property->device, cam_number);
							indigo_item *item = AGENT_CAMERA_BAYERPAT_PROPERTY->items + cam_number;
							strcpy(item->label, property->device);
							sprintf(item->name, "%d", alpaca_device->device_number);
							alpaca_device->ccd.bayer_matrix = item;
							indigo_delete_property(indigo_agent_alpaca_device, AGENT_CAMERA_BAYERPAT_PROPERTY, NULL);
							AGENT_CAMERA_BAYERPAT_PROPERTY->count ++;
							indigo_define_property(indigo_agent_alpaca_device, AGENT_CAMERA_BAYERPAT_PROPERTY, NULL);
							indigo_load_properties(indigo_agent_alpaca_device, false);
						}
					}
				}
			} else if (!strcmp(item->name, INFO_DEVICE_NAME_ITEM_NAME)) {
				pthread_mutex_lock(&alpaca_device->mutex);
				strcpy(alpaca_device->device_name, item->text.value);
				pthread_mutex_unlock(&alpaca_device->mutex);
			} else if (!strcmp(item->name, INFO_DEVICE_DRIVER_ITEM_NAME)) {
				pthread_mutex_lock(&alpaca_device->mutex);
				strcpy(alpaca_device->driver_info, item->text.value);
				pthread_mutex_unlock(&alpaca_device->mutex);
			} else if (!strcmp(item->name, INFO_DEVICE_VERSION_ITEM_NAME)) {
				pthread_mutex_lock(&alpaca_device->mutex);
				strcpy(alpaca_device->driver_version, item->text.value);
				pthread_mutex_unlock(&alpaca_device->mutex);
			}
		}
	} else {
		indigo_alpaca_update_property(alpaca_device, property);
	}
	return INDIGO_OK;
}

static indigo_result agent_update_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	indigo_alpaca_device *alpaca_device = alpaca_devices;
	while (alpaca_device) {
		if (!strcmp(property->device, alpaca_device->indigo_device)) {
			indigo_alpaca_update_property(alpaca_device, property);
			break;
		}
		alpaca_device = alpaca_device->next;
	}
	return INDIGO_OK;
}

static indigo_result agent_delete_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	indigo_alpaca_device *alpaca_device = alpaca_devices, *previous = NULL;
	while (alpaca_device) {
		if (!strcmp(property->device, alpaca_device->indigo_device)) {
			if (*property->name == 0 || !strcmp(property->name, CONNECTION_PROPERTY_NAME)) {
				if (previous == NULL) {
					alpaca_devices = alpaca_device->next;
				} else {
					previous->next = alpaca_device->next;
				}
				indigo_safe_free(alpaca_device);
			}
			break;
		}
		previous = alpaca_device;
		alpaca_device = alpaca_device->next;
	}
	return INDIGO_OK;
}

static indigo_result agent_attach(indigo_client *client) {
	indigo_enumerate_properties(client, &INDIGO_ALL_PROPERTIES);
	return INDIGO_OK;
}

// -------------------------------------------------------------------------------- Initialization

indigo_result indigo_agent_alpaca(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device agent_device_template = INDIGO_DEVICE_INITIALIZER(
		ALPACA_AGENT_NAME,
		agent_device_attach,
		agent_enumerate_properties,
		agent_change_property,
		NULL,
		agent_device_detach
	);

	static indigo_client agent_client_template = {
		ALPACA_AGENT_NAME, false, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT, NULL,
		agent_attach,
		agent_define_property,
		agent_update_property,
		agent_delete_property,
		NULL,
		NULL
	};

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "ASCOM Alpaca bridge agent", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch(action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(alpaca_agent_private_data));
			indigo_agent_alpaca_device = indigo_safe_malloc_copy(sizeof(indigo_device), &agent_device_template);
			indigo_agent_alpaca_device->private_data = private_data;
			indigo_agent_alpaca_client = indigo_safe_malloc_copy(sizeof(indigo_client), &agent_client_template);
			indigo_agent_alpaca_client->client_context = indigo_agent_alpaca_device->device_context;
			indigo_attach_device(indigo_agent_alpaca_device);
			indigo_attach_client(indigo_agent_alpaca_client);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			last_action = action;
			if (indigo_agent_alpaca_client != NULL) {
				indigo_detach_client(indigo_agent_alpaca_client);
				free(indigo_agent_alpaca_client);
				indigo_agent_alpaca_client = NULL;
			}
			if (indigo_agent_alpaca_device != NULL) {
				indigo_detach_device(indigo_agent_alpaca_device);
				free(indigo_agent_alpaca_device);
				indigo_agent_alpaca_device = NULL;
			}
			if (private_data != NULL) {
				free(private_data);
				private_data = NULL;
			}
			indigo_alpaca_device *alpaca_device = alpaca_devices;
			while (alpaca_device) {
				indigo_alpaca_device *tmp = alpaca_device;
				alpaca_device = alpaca_device->next;
				indigo_safe_free(tmp);
			}
			alpaca_devices = NULL;
			break;

		case INDIGO_DRIVER_INFO:
			break;
	}
	return INDIGO_OK;
}
