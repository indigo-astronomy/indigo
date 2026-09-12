// Copyright (C) 2026 Rumen G. Bogdanovski
// All rights reserved.

// You may use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

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

// This file generated from indigo_focuser_askar.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

//+ include

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/time.h>
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <ifaddrs.h>
#elif defined(INDIGO_WINDOWS)
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_focuser_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_focuser_askar.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000004
#define DRIVER_NAME          "indigo_focuser_askar"
#define DRIVER_LABEL         "Askar-WAF Focuser"
#define FOCUSER_DEVICE_NAME  "Askar-WAF"
#define PRIVATE_DATA         ((askar_private_data *)device->private_data)

//+ define

#define SERIAL_BAUDRATE      "115200"
#define RESPONSE_TIMEOUT     0.5
#define ASKAR_CMD_LEN        64
#define ASKAR_MAX_TRAVEL_MIN 100
#define ASKAR_MAX_TRAVEL_MAX 1000000
#define ASKAR_BACKLASH_MAX   10000
#define ASKAR_DISCOVERY_PORT 7676
#define ASKAR_DISCOVERY_REQUEST "WAF:discover"
#define ASKAR_DISCOVERY_TIMEOUT 1
#define ASKAR_DISCOVERY_RETRIES 3
#define ASKAR_DISCOVERY_MAX_IFACES 32
#define ASKAR_DISCOVERY_MAX_DEVICES 20

//- define

#pragma mark - Property definitions

#define X_FOCUSER_MOTOR_MODE_PROPERTY                   (PRIVATE_DATA->x_focuser_motor_mode_property)
#define X_FOCUSER_MOTOR_MODE_HIGH_PERFORMANCE_ITEM      (X_FOCUSER_MOTOR_MODE_PROPERTY->items + 0)
#define X_FOCUSER_MOTOR_MODE_BALANCED_ITEM              (X_FOCUSER_MOTOR_MODE_PROPERTY->items + 1)

#define X_FOCUSER_MOTOR_MODE_PROPERTY_NAME              "X_FOCUSER_MOTOR_MODE"
#define X_FOCUSER_MOTOR_MODE_HIGH_PERFORMANCE_ITEM_NAME "HIGH_PERFORMANCE"
#define X_FOCUSER_MOTOR_MODE_BALANCED_ITEM_NAME         "BALANCED"

#pragma mark - Private data definition

typedef struct {
	indigo_uni_handle *handle;
	indigo_property *x_focuser_motor_mode_property;
	//+ data
	char response[ASKAR_CMD_LEN];
	int current_position, target_position, max_position, last_position, stalled;
	bool moving, uncertain;
	//- data
} askar_private_data;

#pragma mark - Low level code

//+ code

static void focuser_position_handler(indigo_device *device);
static void focuser_steps_handler(indigo_device *device);
static void focuser_connection_handler(indigo_device *device);

typedef struct {
	char url[64];
	char label[64];
} askar_wifi_device;

static double askar_now(void) {
	struct timeval time;
	gettimeofday(&time, NULL);
	return (double)time.tv_sec + time.tv_usec / 1000000.0;
}

static bool askar_command(indigo_device *device, const char *command, const char *expected, ...) {
	bool tcp = PRIVATE_DATA->handle && PRIVATE_DATA->handle->type == INDIGO_TCP_HANDLE;
	if (!PRIVATE_DATA->handle) {
		return false;
	}
	if (indigo_uni_discard(PRIVATE_DATA->handle) < 0) {
		return false;
	}
	va_list args;
	va_start(args, expected);
	long written = indigo_uni_vprintf(PRIVATE_DATA->handle, command, args);
	va_end(args);
	if (written <= 0) {
		return false;
	}
	if (!expected) {
		return true;
	}
	double deadline = askar_now() + RESPONSE_TIMEOUT;
	for (int frames = 0; frames < 4; frames++) {
		double remaining = deadline - askar_now();
		if (remaining <= 0) {
			return false;
		}
		long count = indigo_uni_read_section2(PRIVATE_DATA->handle, PRIVATE_DATA->response, sizeof(PRIVATE_DATA->response) - 1, "#", "\r\n", INDIGO_DELAY(remaining), INDIGO_DELAY(0.1));
		if (count <= 0 || PRIVATE_DATA->response[count - 1] != '#' || (long)strlen(PRIVATE_DATA->response) != count) {
			return false;
		}
		if (!strncmp(PRIVATE_DATA->response, expected, strlen(expected))) {
			return strncmp(PRIVATE_DATA->response, "FE", 2) != 0;
		}
	}
	if (tcp && PRIVATE_DATA->handle) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		indigo_execute_handler(device, focuser_connection_handler);
		CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_send_message(device, ALERT_PROPERTY, "Device disconnected unexpectedly", device->name);
	}
	return false;
}

static bool askar_get_string(indigo_device *device, const char *command, const char *prefix, char *value, int size) {
	if (!askar_command(device, command, prefix)) {
		return false;
	}
	char *src = PRIVATE_DATA->response + strlen(prefix);
	char *hash = strchr(src, '#');
	if (hash) {
		*hash = 0;
	}
	snprintf(value, (size_t)size, "%s", src);
	return true;
}

static bool askar_get_int(indigo_device *device, const char *command, const char *prefix, int *value) {
	char tail = 0;
	if (!askar_command(device, command, prefix)) {
		return false;
	}
	return sscanf(PRIVATE_DATA->response + strlen(prefix), "%d#%c", value, &tail) == 1;
}

static bool askar_bool(indigo_device *device, const char *command, const char *prefix, bool *value) {
	int state = 0;
	if (!askar_get_int(device, command, prefix, &state) || (state != 0 && state != 1)) {
		return false;
	}
	*value = state != 0;
	return true;
}

static bool askar_get_position(indigo_device *device, int *position) {
	return askar_get_int(device, "Fp#", "Fp", position) && *position >= 0 && *position <= ASKAR_MAX_TRAVEL_MAX;
}

static bool askar_is_moving(indigo_device *device, bool *moving) {
	return askar_bool(device, "FQ#", "FQ", moving);
}

static bool askar_get_max_position(indigo_device *device, int *max_position) {
	return askar_get_int(device, "Fm#", "Fm", max_position) && *max_position >= ASKAR_MAX_TRAVEL_MIN && *max_position <= ASKAR_MAX_TRAVEL_MAX;
}

static bool askar_set_max_position(indigo_device *device, int max_position) {
	return max_position >= ASKAR_MAX_TRAVEL_MIN && max_position <= ASKAR_MAX_TRAVEL_MAX && askar_command(device, "FM%d#", "FM", max_position);
}

static bool askar_goto_position(indigo_device *device, int position) {
	return askar_command(device, "FP%d#", "FP", position);
}

static bool askar_sync_position(indigo_device *device, int position) {
	return askar_command(device, "FY%d#", "FY", position);
}

static bool askar_stop(indigo_device *device) {
	return askar_command(device, "FS#", "FS");
}

static bool askar_set_backlash(indigo_device *device, int backlash) {
	backlash = backlash < 0 ? 0 : backlash > ASKAR_BACKLASH_MAX ? ASKAR_BACKLASH_MAX : backlash;
	return askar_command(device, "FB%d#", "FB", backlash);
}

static bool askar_set_reverse(indigo_device *device, bool reversed) {
	return askar_command(device, "FR%d#", "FR", reversed ? 1 : 0);
}

static bool askar_set_motor_mode(indigo_device *device, bool balanced) {
	return askar_command(device, "FO%d#", "FO", balanced ? 1 : 0);
}

static void askar_motion_state(indigo_device *device, indigo_property_state state) {
	FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = state;
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
}

static void motion_finalizer(indigo_device *device) {
	bool moving = false;
	int position = 0;
	if (!IS_CONNECTED || !PRIVATE_DATA->moving) {
		return;
	}
	if (!askar_is_moving(device, &moving) || !askar_get_position(device, &position)) {
		PRIVATE_DATA->moving = false;
		PRIVATE_DATA->uncertain = !askar_stop(device);
		askar_motion_state(device, INDIGO_ALERT_STATE);
		return;
	}
	PRIVATE_DATA->current_position = position;
	FOCUSER_POSITION_ITEM->number.value = position;
	if (!moving || position == PRIVATE_DATA->target_position) {
		bool reached = position == PRIVATE_DATA->target_position;
		PRIVATE_DATA->moving = false;
		FOCUSER_POSITION_ITEM->number.target = position;
		askar_motion_state(device, reached ? INDIGO_OK_STATE : INDIGO_ALERT_STATE);
		return;
	}
	if (position != PRIVATE_DATA->last_position) {
		PRIVATE_DATA->last_position = position;
		PRIVATE_DATA->stalled = 0;
	} else if (++PRIVATE_DATA->stalled >= 100) {
		PRIVATE_DATA->moving = false;
		PRIVATE_DATA->uncertain = !askar_stop(device);
		askar_motion_state(device, INDIGO_ALERT_STATE);
		return;
	}
	askar_motion_state(device, INDIGO_BUSY_STATE);
	indigo_execute_handler_in(device, 0.1, motion_finalizer);
}

static void askar_ranges(indigo_device *device, int maximum) {
	PRIVATE_DATA->max_position = maximum;
	FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.target = 0;
	FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target = maximum;
	FOCUSER_POSITION_ITEM->number.max = maximum;
	FOCUSER_STEPS_ITEM->number.max = maximum;
}

static void askar_start_motion(indigo_device *device, int target) {
	if (!IS_CONNECTED || PRIVATE_DATA->uncertain) {
		askar_motion_state(device, INDIGO_ALERT_STATE);
		return;
	}
	target = target < 0 ? 0 : target > PRIVATE_DATA->max_position ? PRIVATE_DATA->max_position : target;
	PRIVATE_DATA->target_position = target;
	FOCUSER_POSITION_ITEM->number.target = target;
	if (target == PRIVATE_DATA->current_position) {
		askar_motion_state(device, INDIGO_OK_STATE);
		return;
	}
	if (!askar_goto_position(device, target)) {
		PRIVATE_DATA->uncertain = true;
		askar_motion_state(device, INDIGO_ALERT_STATE);
		return;
	}
	PRIVATE_DATA->moving = true;
	PRIVATE_DATA->stalled = 0;
	PRIVATE_DATA->last_position = PRIVATE_DATA->current_position;
	askar_motion_state(device, INDIGO_BUSY_STATE);
}

static bool askar_open(indigo_device *device) {
	char *name = DEVICE_PORT_ITEM->text.value;
	if (!indigo_uni_is_url(name, "askar")) {
		PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(name, atoi(DEVICE_BAUDRATE_ITEM->text.value), INDIGO_LOG_DEBUG);
	} else {
		char *host = strstr(name, "://");
		host = host ? host + 3 : name;
		bool resolved = *host != 0;
		if (!resolved) {
			for (int i = 0; i < DEVICE_PORTS_PROPERTY->count; i++) {
				if (indigo_uni_is_url(DEVICE_PORTS_PROPERTY->items[i].name, "askar")) {
					INDIGO_COPY_VALUE(DEVICE_PORT_ITEM->text.value, DEVICE_PORTS_PROPERTY->items[i].name);
					name = DEVICE_PORT_ITEM->text.value;
					indigo_update_property(device, DEVICE_PORT_PROPERTY, "Askar-WAF selected %s", name);
					resolved = true;
					break;
				}
			}
			if (!resolved) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "No Wi-Fi focuser discovered (try Refresh)");
			}
		}
		PRIVATE_DATA->handle = resolved ? indigo_uni_open_url(name, 8080, INDIGO_TCP_HANDLE, INDIGO_LOG_DEBUG) : NULL;
	}
	if (!PRIVATE_DATA->handle) {
		return false;
	}
	if (!askar_get_position(device, &PRIVATE_DATA->current_position)) {
		indigo_uni_close(&PRIVATE_DATA->handle);
		return false;
	}
	return true;
}

static void askar_close(indigo_device *device) {
	indigo_uni_close(&PRIVATE_DATA->handle);
}

static bool askar_parse_reply(const char *reply, askar_wifi_device *dev) {
	char ip[64];
	int port;
	if (sscanf(reply, "WAF:%63[^:]:%d", ip, &port) == 2) {
		snprintf(dev->url, sizeof(dev->url), "askar://%s:%d", ip, port);
		snprintf(dev->label, sizeof(dev->label), "%s:%d", ip, port);
		return true;
	}
	return false;
}

static bool askar_wifi_list_contains(const askar_wifi_device *list, int count, const char *url) {
	for (int i = 0; i < count; i++) {
		if (!strncmp(list[i].url, url, sizeof(list[i].url))) {
			return true;
		}
	}
	return false;
}

static int askar_discover_all(askar_wifi_device *list, int max) {
	int found = 0;
	struct in_addr targets[ASKAR_DISCOVERY_MAX_IFACES];
	int target_count = 0;
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "WAF discovery: socket() failed (%s)", strerror(errno));
		return 0;
	}
	int broadcast = 1;
	struct timeval tv;
	tv.tv_sec = ASKAR_DISCOVERY_TIMEOUT;
	tv.tv_usec = 0;
	setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	struct ifaddrs *ifaddr = NULL;
	if (getifaddrs(&ifaddr) == 0) {
		for (struct ifaddrs *ifa = ifaddr; ifa != NULL && target_count < ASKAR_DISCOVERY_MAX_IFACES; ifa = ifa->ifa_next) {
			if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_INET) {
				continue;
			}
			if (!(ifa->ifa_flags & IFF_UP) || !(ifa->ifa_flags & IFF_BROADCAST) || (ifa->ifa_flags & IFF_LOOPBACK)) {
				continue;
			}
			if (ifa->ifa_broadaddr == NULL) {
				continue;
			}
			targets[target_count++] = ((struct sockaddr_in *)ifa->ifa_broadaddr)->sin_addr;
		}
		freeifaddrs(ifaddr);
	}
	if (target_count == 0) {
		targets[target_count++].s_addr = htonl(INADDR_BROADCAST);
	}
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(ASKAR_DISCOVERY_PORT);
	for (int i = 0; i < ASKAR_DISCOVERY_RETRIES && found < max; i++) {
		for (int t = 0; t < target_count; t++) {
			addr.sin_addr = targets[t];
			if (sendto(sock, ASKAR_DISCOVERY_REQUEST, strlen(ASKAR_DISCOVERY_REQUEST), 0, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "WAF discovery: sendto(%s) failed (%s)", inet_ntoa(targets[t]), strerror(errno));
			}
		}
		while (found < max) {
			struct sockaddr_in from;
			socklen_t from_len = sizeof(from);
			char reply[64] = { 0 };
			long n = recvfrom(sock, reply, sizeof(reply) - 1, 0, (struct sockaddr *)&from, &from_len);
			if (n <= 0) {
				break;
			}
			reply[n] = 0;
			askar_wifi_device dev;
			if (askar_parse_reply(reply, &dev) && !askar_wifi_list_contains(list, found, dev.url)) {
				list[found++] = dev;
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "WAF discovery: focuser detected at %s", dev.label);
			}
		}
	}
	close(sock);
#elif defined(INDIGO_WINDOWS)
	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "WAF discovery: socket() failed (%d)", WSAGetLastError());
		return 0;
	}
	BOOL broadcast = TRUE;
	DWORD timeout_ms = ASKAR_DISCOVERY_TIMEOUT * 1000;
	setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (const char *)&broadcast, sizeof(broadcast));
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout_ms, sizeof(timeout_ms));
	INTERFACE_INFO if_list[ASKAR_DISCOVERY_MAX_IFACES];
	DWORD bytes_returned = 0;
	if (WSAIoctl(sock, SIO_GET_INTERFACE_LIST, NULL, 0, if_list, sizeof(if_list), &bytes_returned, NULL, NULL) == 0) {
		int count = (int)(bytes_returned / sizeof(INTERFACE_INFO));
		for (int i = 0; i < count && target_count < ASKAR_DISCOVERY_MAX_IFACES; i++) {
			u_long flags = if_list[i].iiFlags;
			if (!(flags & IFF_UP) || !(flags & IFF_BROADCAST) || (flags & IFF_LOOPBACK)) {
				continue;
			}
			u_long ip = if_list[i].iiAddress.AddressIn.sin_addr.s_addr;
			u_long mask = if_list[i].iiNetmask.AddressIn.sin_addr.s_addr;
			targets[target_count++].s_addr = ip | ~mask;
		}
	}
	if (target_count == 0) {
		targets[target_count++].s_addr = htonl(INADDR_BROADCAST);
	}
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(ASKAR_DISCOVERY_PORT);
	for (int i = 0; i < ASKAR_DISCOVERY_RETRIES && found < max; i++) {
		for (int t = 0; t < target_count; t++) {
			addr.sin_addr = targets[t];
			if (sendto(sock, ASKAR_DISCOVERY_REQUEST, (int)strlen(ASKAR_DISCOVERY_REQUEST), 0, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR) {
				char ip_str[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, &targets[t], ip_str, sizeof(ip_str));
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "WAF discovery: sendto(%s) failed (%d)", ip_str, WSAGetLastError());
			}
		}
		while (found < max) {
			struct sockaddr_in from;
			int from_len = sizeof(from);
			char reply[64] = { 0 };
			int n = recvfrom(sock, reply, sizeof(reply) - 1, 0, (struct sockaddr *)&from, &from_len);
			if (n <= 0) {
				break;
			}
			reply[n] = 0;
			askar_wifi_device dev;
			if (askar_parse_reply(reply, &dev) && !askar_wifi_list_contains(list, found, dev.url)) {
				list[found++] = dev;
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "WAF discovery: focuser detected at %s", dev.label);
			}
		}
	}
	closesocket(sock);
#endif
	return found;
}

static void askar_scan_ports(indigo_device *device) {
	askar_wifi_device found[ASKAR_DISCOVERY_MAX_DEVICES];
	int count = askar_discover_all(found, ASKAR_DISCOVERY_MAX_DEVICES);
	int base = DEVICE_PORTS_PROPERTY->count;
	if (count == 0) {
		return;
	}
	DEVICE_PORTS_PROPERTY = indigo_resize_property(DEVICE_PORTS_PROPERTY, base + count);
	for (int i = 0; i < count; i++) {
		char label[INDIGO_NAME_SIZE];
		snprintf(label, sizeof(label), "Askar-WAF (%s)", found[i].label);
		indigo_init_switch_item(DEVICE_PORTS_PROPERTY->items + base + i, found[i].url, label, false);
	}
}

static void scan_ports_finalizer(indigo_device *device) {
	indigo_delete_property(device, DEVICE_PORTS_PROPERTY, NULL);
	indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
	askar_scan_ports(device);
	DEVICE_PORTS_PROPERTY->items[0].sw.value = false;
	DEVICE_PORTS_PROPERTY->state = INDIGO_OK_STATE;
	indigo_define_property(device, DEVICE_PORTS_PROPERTY, NULL);
}

//- code

#pragma mark - High level code (focuser)

static void focuser_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ focuser.on_timer
	if (!PRIVATE_DATA->moving && FOCUSER_POSITION_PROPERTY->state != INDIGO_BUSY_STATE && FOCUSER_STEPS_PROPERTY->state != INDIGO_BUSY_STATE) {
		int position = 0;
		if (askar_get_position(device, &position)) {
			PRIVATE_DATA->current_position = position;
			PRIVATE_DATA->target_position = position;
			FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = position;
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		} else {
			askar_motion_state(device, INDIGO_ALERT_STATE);
		}
	}
	indigo_execute_handler_in(device, 1, focuser_timer_callback);
	//- focuser.on_timer
}

static void focuser_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = askar_open(device);
		if (connection_result) {
			//+ focuser.on_connect
			char model[ASKAR_CMD_LEN] = "Askar-WAF";
			char firmware[ASKAR_CMD_LEN] = "N/A";
			int position = 0, maximum = 0, backlash = 0;
			bool reversed = false, balanced = false;
			connection_result = askar_get_position(device, &position) && askar_get_max_position(device, &maximum);
			if (connection_result) {
				askar_get_string(device, "FI#", "FI", model, sizeof(model));
				askar_get_string(device, "FV#", "FV", firmware, sizeof(firmware));
				INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, model);
				INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, firmware);
				indigo_update_property(device, INFO_PROPERTY, NULL);
				PRIVATE_DATA->current_position = PRIVATE_DATA->target_position = position;
				FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = position;
				askar_ranges(device, maximum);
				if (askar_get_int(device, "Fb#", "Fb", &backlash)) {
					FOCUSER_BACKLASH_ITEM->number.value = FOCUSER_BACKLASH_ITEM->number.target = backlash;
				}
				if (askar_bool(device, "Fr#", "Fr", &reversed)) {
					indigo_set_switch(FOCUSER_REVERSE_MOTION_PROPERTY, reversed ? FOCUSER_REVERSE_MOTION_ENABLED_ITEM : FOCUSER_REVERSE_MOTION_DISABLED_ITEM, true);
				}
				if (askar_bool(device, "Fo#", "Fo", &balanced)) {
					indigo_set_switch(X_FOCUSER_MOTOR_MODE_PROPERTY, balanced ? X_FOCUSER_MOTOR_MODE_BALANCED_ITEM : X_FOCUSER_MOTOR_MODE_HIGH_PERFORMANCE_ITEM, true);
				}
				X_FOCUSER_MOTOR_MODE_PROPERTY->state = INDIGO_OK_STATE;
				PRIVATE_DATA->moving = PRIVATE_DATA->uncertain = false;
				askar_motion_state(device, INDIGO_OK_STATE);
			} else {
				askar_close(device);
			}
			//- focuser.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, X_FOCUSER_MOTOR_MODE_PROPERTY, NULL);
			indigo_execute_handler(device, focuser_timer_callback);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", FOCUSER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", FOCUSER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ focuser.on_disconnect
		if (PRIVATE_DATA->moving || PRIVATE_DATA->uncertain) {
			askar_stop(device);
		}
		PRIVATE_DATA->moving = PRIVATE_DATA->uncertain = false;
		FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
		//- focuser.on_disconnect
		indigo_delete_property(device, X_FOCUSER_MOTOR_MODE_PROPERTY, NULL);
		askar_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void focuser_limits_handler(indigo_device *device) {
	FOCUSER_LIMITS_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_LIMITS.on_change
	int requested = (int)FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target;
	if (!IS_CONNECTED || PRIVATE_DATA->moving || requested < PRIVATE_DATA->current_position || !askar_set_max_position(device, requested)) {
		FOCUSER_LIMITS_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		int maximum = 0, position = 0;
		if (askar_get_max_position(device, &maximum)) {
			askar_ranges(device, maximum);
			indigo_delete_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		}
		if (askar_get_position(device, &position)) {
			PRIVATE_DATA->current_position = PRIVATE_DATA->target_position = position;
			FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = position;
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		}
	}
	//- focuser.FOCUSER_LIMITS.on_change
	indigo_update_property(device, FOCUSER_LIMITS_PROPERTY, NULL);
}

static void focuser_position_handler(indigo_device *device) {
	//+ focuser.FOCUSER_POSITION.on_change
	int target = (int)FOCUSER_POSITION_ITEM->number.target;
	if (FOCUSER_ON_POSITION_SET_GOTO_ITEM->sw.value) {
		askar_start_motion(device, target);
		if (PRIVATE_DATA->moving) {
			indigo_execute_handler_in(device, 0.1, motion_finalizer);
		}
	} else if (IS_CONNECTED && !PRIVATE_DATA->moving && !PRIVATE_DATA->uncertain && askar_sync_position(device, target) && askar_get_position(device, &PRIVATE_DATA->current_position)) {
		PRIVATE_DATA->target_position = PRIVATE_DATA->current_position;
		FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = PRIVATE_DATA->current_position;
		askar_motion_state(device, INDIGO_OK_STATE);
	} else {
		PRIVATE_DATA->uncertain = true;
		askar_motion_state(device, INDIGO_ALERT_STATE);
	}
	//- focuser.FOCUSER_POSITION.on_change
}

static void focuser_steps_handler(indigo_device *device) {
	//+ focuser.FOCUSER_STEPS.on_change
	int steps = (int)FOCUSER_STEPS_ITEM->number.value;
	int target = PRIVATE_DATA->current_position + (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value ? -steps : steps);
	askar_start_motion(device, target);
	if (PRIVATE_DATA->moving) {
		indigo_execute_handler_in(device, 0.1, motion_finalizer);
	}
	//- focuser.FOCUSER_STEPS.on_change
}

static void focuser_abort_motion_handler(indigo_device *device) {
	//+ focuser.FOCUSER_ABORT_MOTION.on_change
	if (FOCUSER_ABORT_MOTION_ITEM->sw.value && IS_CONNECTED) {
		indigo_cancel_pending_handler(device, focuser_position_handler);
		indigo_cancel_pending_handler(device, focuser_steps_handler);
		if (!PRIVATE_DATA->moving && (FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE)) {
			askar_motion_state(device, INDIGO_ALERT_STATE);
		}
		if (askar_stop(device) && askar_get_position(device, &PRIVATE_DATA->current_position)) {
			indigo_cancel_pending_handler(device, motion_finalizer);
			PRIVATE_DATA->moving = PRIVATE_DATA->uncertain = false;
			PRIVATE_DATA->target_position = PRIVATE_DATA->current_position;
			FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = PRIVATE_DATA->current_position;
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
			askar_motion_state(device, INDIGO_OK_STATE);
		} else {
			PRIVATE_DATA->uncertain = true;
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	} else {
		FOCUSER_ABORT_MOTION_PROPERTY->state = IS_CONNECTED ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	}
	FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
	indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
	//- focuser.FOCUSER_ABORT_MOTION.on_change
}

static void focuser_backlash_handler(indigo_device *device) {
	FOCUSER_BACKLASH_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_BACKLASH.on_change
	int requested = (int)FOCUSER_BACKLASH_ITEM->number.target;
	if (IS_CONNECTED && !PRIVATE_DATA->moving && askar_set_backlash(device, requested) && askar_get_int(device, "Fb#", "Fb", &requested)) {
		FOCUSER_BACKLASH_ITEM->number.value = FOCUSER_BACKLASH_ITEM->number.target = requested;
	} else {
		FOCUSER_BACKLASH_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.FOCUSER_BACKLASH.on_change
	indigo_update_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
}

static void focuser_reverse_motion_handler(indigo_device *device) {
	FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_REVERSE_MOTION.on_change
	bool reversed = FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value;
	if (IS_CONNECTED && !PRIVATE_DATA->moving && askar_set_reverse(device, reversed) && askar_bool(device, "Fr#", "Fr", &reversed)) {
		indigo_set_switch(FOCUSER_REVERSE_MOTION_PROPERTY, reversed ? FOCUSER_REVERSE_MOTION_ENABLED_ITEM : FOCUSER_REVERSE_MOTION_DISABLED_ITEM, true);
		int position = 0;
		if (askar_get_position(device, &position)) {
			PRIVATE_DATA->current_position = PRIVATE_DATA->target_position = position;
			FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = position;
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		}
	} else {
		FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.FOCUSER_REVERSE_MOTION.on_change
	indigo_update_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
}

static void focuser_device_ports_handler(indigo_device *device) {
	//+ focuser.DEVICE_PORTS.on_change
	if (DEVICE_PORTS_PROPERTY->items[0].sw.value) {
		DEVICE_PORTS_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, DEVICE_PORTS_PROPERTY, NULL);
		indigo_execute_handler(device, scan_ports_finalizer);
	}
	//- focuser.DEVICE_PORTS.on_change
}

static void focuser_x_focuser_motor_mode_handler(indigo_device *device) {
	X_FOCUSER_MOTOR_MODE_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_FOCUSER_MOTOR_MODE.on_change
	bool balanced = X_FOCUSER_MOTOR_MODE_BALANCED_ITEM->sw.value;
	if (IS_CONNECTED && !PRIVATE_DATA->moving && askar_set_motor_mode(device, balanced) && askar_bool(device, "Fo#", "Fo", &balanced)) {
		indigo_set_switch(X_FOCUSER_MOTOR_MODE_PROPERTY, balanced ? X_FOCUSER_MOTOR_MODE_BALANCED_ITEM : X_FOCUSER_MOTOR_MODE_HIGH_PERFORMANCE_ITEM, true);
		int position = 0;
		if (askar_get_position(device, &position)) {
			PRIVATE_DATA->current_position = PRIVATE_DATA->target_position = position;
			FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = position;
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		}
	} else {
		X_FOCUSER_MOTOR_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.X_FOCUSER_MOTOR_MODE.on_change
	indigo_update_property(device, X_FOCUSER_MOTOR_MODE_PROPERTY, NULL);
}

#pragma mark - Device API (focuser)

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result focuser_attach(indigo_device *device) {
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		ADDITIONAL_INSTANCES_PROPERTY->hidden = device->base_device != NULL;
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		DEVICE_BAUDRATE_PROPERTY->hidden = false;
		//+ focuser.on_attach
		INFO_PROPERTY->count = 6;
		DEVICE_BAUDRATE_PROPERTY->hidden = false;
		INDIGO_COPY_VALUE(DEVICE_BAUDRATE_ITEM->text.value, SERIAL_BAUDRATE);
		indigo_execute_handler(device, scan_ports_finalizer);
		//- focuser.on_attach
		FOCUSER_SPEED_PROPERTY->hidden = true;
		FOCUSER_TEMPERATURE_PROPERTY->hidden = true;
		FOCUSER_COMPENSATION_PROPERTY->hidden = true;
		FOCUSER_MODE_PROPERTY->hidden = true;
		FOCUSER_ON_POSITION_SET_PROPERTY->hidden = false;
		FOCUSER_LIMITS_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_LIMITS.on_attach
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.min = 0;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.max = 0;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.target = 0;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.min = ASKAR_MAX_TRAVEL_MIN;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.max = ASKAR_MAX_TRAVEL_MAX;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target = 100000;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.step = 100;
		//- focuser.FOCUSER_LIMITS.on_attach
		FOCUSER_POSITION_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_POSITION.on_attach
		FOCUSER_POSITION_ITEM->number.min = 0;
		FOCUSER_POSITION_ITEM->number.max = ASKAR_MAX_TRAVEL_MAX;
		FOCUSER_POSITION_ITEM->number.step = 100;
		//- focuser.FOCUSER_POSITION.on_attach
		FOCUSER_STEPS_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_STEPS.on_attach
		FOCUSER_STEPS_ITEM->number.min = 0;
		FOCUSER_STEPS_ITEM->number.max = ASKAR_MAX_TRAVEL_MAX;
		FOCUSER_STEPS_ITEM->number.step = 1;
		//- focuser.FOCUSER_STEPS.on_attach
		FOCUSER_ABORT_MOTION_PROPERTY->hidden = false;
		FOCUSER_BACKLASH_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_BACKLASH.on_attach
		FOCUSER_BACKLASH_ITEM->number.min = 0;
		FOCUSER_BACKLASH_ITEM->number.max = ASKAR_BACKLASH_MAX;
		FOCUSER_BACKLASH_ITEM->number.step = 1;
		//- focuser.FOCUSER_BACKLASH.on_attach
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		X_FOCUSER_MOTOR_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, X_FOCUSER_MOTOR_MODE_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "Motor mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_FOCUSER_MOTOR_MODE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_FOCUSER_MOTOR_MODE_HIGH_PERFORMANCE_ITEM, X_FOCUSER_MOTOR_MODE_HIGH_PERFORMANCE_ITEM_NAME, "High performance", true);
		indigo_init_switch_item(X_FOCUSER_MOTOR_MODE_BALANCED_ITEM, X_FOCUSER_MOTOR_MODE_BALANCED_ITEM_NAME, "Balanced", false);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_FOCUSER_MOTOR_MODE_PROPERTY);
	}
	return indigo_focuser_enumerate_properties(device, client, property);
}

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			indigo_execute_handler(device, focuser_connection_handler);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_LIMITS_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(FOCUSER_LIMITS_PROPERTY, focuser_limits_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_POSITION_PROPERTY, property)) {
		//+ focuser.FOCUSER_POSITION.on_change_request
		if (FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_ABORT_MOTION_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, "Another motion operation is pending");
			return INDIGO_OK;
		}
		//- focuser.FOCUSER_POSITION.on_change_request
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(FOCUSER_POSITION_PROPERTY, focuser_position_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		//+ focuser.FOCUSER_STEPS.on_change_request
		if (FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_ABORT_MOTION_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, "Another motion operation is pending");
			return INDIGO_OK;
		}
		//- focuser.FOCUSER_STEPS.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_STEPS_PROPERTY, focuser_steps_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(FOCUSER_ABORT_MOTION_PROPERTY, focuser_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_BACKLASH_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(FOCUSER_BACKLASH_PROPERTY, focuser_backlash_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_REVERSE_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_REVERSE_MOTION_PROPERTY, focuser_reverse_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(DEVICE_PORTS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(DEVICE_PORTS_PROPERTY, focuser_device_ports_handler);
	} else if (indigo_property_match_changeable(X_FOCUSER_MOTOR_MODE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_FOCUSER_MOTOR_MODE_PROPERTY, focuser_x_focuser_motor_mode_handler);
		return INDIGO_OK;
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_connection_handler(device);
	}
	indigo_release_property(X_FOCUSER_MOTOR_MODE_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

#pragma mark - Device templates

static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(FOCUSER_DEVICE_NAME, focuser_attach, focuser_enumerate_properties, focuser_change_property, NULL, focuser_detach);

#pragma mark - Main code

indigo_result indigo_focuser_askar(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static askar_private_data *private_data = NULL;
	static indigo_device *focuser = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			private_data = (askar_private_data *)indigo_safe_malloc(sizeof(askar_private_data));
			focuser = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &focuser_template);
			focuser->private_data = private_data;
			indigo_attach_device(focuser);
			break;

		}
		case INDIGO_DRIVER_SHUTDOWN: {
			VERIFY_NOT_CONNECTED(focuser);
			last_action = action;
			if (focuser != NULL) {
				indigo_detach_device(focuser);
				indigo_safe_free(focuser);
				focuser = NULL;
			}
			if (private_data != NULL) {
				indigo_safe_free(private_data);
				private_data = NULL;
			}
			break;

		}
		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
