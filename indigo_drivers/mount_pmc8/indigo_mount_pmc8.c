// Copyright (c) 2020-2026 CloudMakers, s. r. o.
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

// This file generated from indigo_mount_pmc8.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

//+ include

#include <stdarg.h>

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_mount_driver.h>
#include <indigo/indigo_align.h>
#include <indigo/indigo_guider_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_mount_pmc8.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000009
#define DRIVER_NAME          "indigo_mount_pmc8"
#define DRIVER_LABEL         "PMC Eight Mount"
#define MOUNT_DEVICE_NAME    "Mount PMC Eight"
#define GUIDER_DEVICE_NAME   "Mount PMC Eight (guider)"
#define PRIVATE_DATA         ((pmc8_private_data *)device->private_data)

//+ define

#define PMC8_RESPONSE        (PRIVATE_DATA->response)

// -------------------------------------------------------------------------------- MODEL DATA

typedef struct {
	char *name;
	uint32_t count[2];
} mount_type_data;

typedef enum {
	PMC8_G11 = 0,
	PMC8_TITAN,
	PMC8_EXOS2,
	PMC8_IEXOS100
} model_type;

static mount_type_data MODELS[] = {
	{ "Losmandy G-11", { 4608000, 4608000 } },
	{ "Losmandy Titan", { 3456000, 3456000 } },
	{ "Explore Scientific EXOS II", { 4147200, 4147200 } },
	{ "Explore Scientific iEXOS-100", { 4147200, 4147200 } },
};

//- define

#pragma mark - Property definitions

#define CONNECTION_MODE_PROPERTY        (PRIVATE_DATA->connection_mode_property)
#define CONNECTION_UDP_ITEM             (CONNECTION_MODE_PROPERTY->items + 0)
#define CONNECTION_TCP_ITEM             (CONNECTION_MODE_PROPERTY->items + 1)
#define CONNECTION_SERIAL_ITEM          (CONNECTION_MODE_PROPERTY->items + 2)
#define CONNECTION_SERIAL_DTR_ITEM      (CONNECTION_MODE_PROPERTY->items + 3)

#define CONNECTION_MODE_PROPERTY_NAME   "CONNECTION_MODE"
#define CONNECTION_UDP_ITEM_NAME        "UDP"
#define CONNECTION_TCP_ITEM_NAME        "TCP"
#define CONNECTION_SERIAL_ITEM_NAME     "SERIAL"
#define CONNECTION_SERIAL_DTR_ITEM_NAME "SERIAL_DTR"

#define MOUNT_TYPE_PROPERTY            (PRIVATE_DATA->mount_type_property)
#define MOUNT_TYPE_AUTO                (MOUNT_TYPE_PROPERTY->items + 0)
#define MOUNT_TYPE_G11                 (MOUNT_TYPE_PROPERTY->items + 1)
#define MOUNT_TYPE_TITAN               (MOUNT_TYPE_PROPERTY->items + 2)
#define MOUNT_TYPE_EXOS2               (MOUNT_TYPE_PROPERTY->items + 3)
#define MOUNT_TYPE_IEXOS100            (MOUNT_TYPE_PROPERTY->items + 4)

#define MOUNT_TYPE_PROPERTY_NAME       "MOUNT_TYPE"
#define MOUNT_TYPE_AUTO_ITEM_NAME      "AUTO"
#define MOUNT_TYPE_G11_ITEM_NAME       "G11"
#define MOUNT_TYPE_TITAN_ITEM_NAME     "TITAN"
#define MOUNT_TYPE_EXOS2_ITEM_NAME     "EXOS-2"
#define MOUNT_TYPE_IEXOS100_ITEM_NAME  "iEXOS-100"

#pragma mark - Private data definition

typedef struct {
	int count;
	indigo_uni_handle *handle;
	indigo_property *connection_mode_property;
	indigo_property *mount_type_property;
	//+ data
	model_type type;
	int rate[3];
	pthread_mutex_t port_mutex;
	indigo_uni_handle_type proto;
	bool park;
	bool is_udp, is_tcp, is_serial;
	bool configured;
	int version;
	char response[128];
	//- data
} pmc8_private_data;

#pragma mark - Low level code

//+ code

static bool pmc8_validate_handle(indigo_device *device) {
	return PRIVATE_DATA->handle != NULL;
}

static void pmc8_update_connection_port(indigo_device *device) {
	if (CONNECTION_UDP_ITEM->sw.value) {
		strcpy(DEVICE_PORT_ITEM->text.value, "udp://192.168.47.1");
	} else if (CONNECTION_TCP_ITEM->sw.value) {
		strcpy(DEVICE_PORT_ITEM->text.value, "tcp://192.168.47.1");
	} else {
		if (DEVICE_PORTS_PROPERTY->count > 1) {
			strcpy(DEVICE_PORT_ITEM->text.value, DEVICE_PORTS_PROPERTY->items[1].name);
		} else {
			strcpy(DEVICE_PORT_ITEM->text.value, "");
		}
	}
	DEVICE_PORT_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, DEVICE_PORT_PROPERTY, NULL);
}

static void pmc8_update_mount_type_perm(indigo_device *device, indigo_property_perm perm) {
	indigo_delete_property(device, MOUNT_TYPE_PROPERTY, NULL);
	MOUNT_TYPE_PROPERTY->perm = perm;
	indigo_define_property(device, MOUNT_TYPE_PROPERTY, NULL);
}

static bool pmc8_open(indigo_device *device) {
	char *name = DEVICE_PORT_ITEM->text.value;
	if (!indigo_uni_is_url(name, "pmc8")) {
		PRIVATE_DATA->proto = INDIGO_COM_HANDLE;
		PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(name, 115200, INDIGO_LOG_DEBUG);
		if (CONNECTION_SERIAL_DTR_ITEM->sw.value) {
			indigo_uni_set_dtr(PRIVATE_DATA->handle, false);
		}
	} else {
		PRIVATE_DATA->proto = INDIGO_UDP_HANDLE;
		PRIVATE_DATA->handle = indigo_uni_open_url(name, 54372, PRIVATE_DATA->proto, INDIGO_LOG_DEBUG);
		if (PRIVATE_DATA->handle != NULL) {
			PRIVATE_DATA->proto = PRIVATE_DATA->handle->type;
			indigo_uni_set_socket_read_timeout(PRIVATE_DATA->handle, INDIGO_DELAY(0.5));
			indigo_uni_set_socket_write_timeout(PRIVATE_DATA->handle, INDIGO_DELAY(0.5));
		}
	}
	return PRIVATE_DATA->handle != NULL;
}

static bool pmc8_command(indigo_device *device, char *command, ...) {
	pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
	bool result = false;
	if (!pmc8_validate_handle(device)) {
		goto cleanup;
	}
	for (int repeat = 10; repeat >= 0; repeat--) {
		long bytes_read = 0;
		long io_result = indigo_uni_discard(PRIVATE_DATA->handle);
		if (io_result >= 0) {
			va_list args;
			va_start(args, command);
			io_result = indigo_uni_vprintf(PRIVATE_DATA->handle, command, args);
			va_end(args);
		}
		if (io_result >= 0) {
			if (PRIVATE_DATA->handle->type == INDIGO_UDP_HANDLE) {
				bytes_read = indigo_uni_read(PRIVATE_DATA->handle, PMC8_RESPONSE, sizeof(PRIVATE_DATA->response) - 1);
			} else {
				bytes_read = indigo_uni_read_section2(PRIVATE_DATA->handle, PMC8_RESPONSE, sizeof(PRIVATE_DATA->response) - 1, "!%#", "", INDIGO_DELAY(0.5), INDIGO_DELAY(0.1));
			}
		}
		if (bytes_read > 0) {
			PMC8_RESPONSE[bytes_read] = 0;
			char terminator = PMC8_RESPONSE[bytes_read - 1];
			result = terminator == '!' || terminator == '%' || terminator == '#';
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Command %s -> %s", command, PMC8_RESPONSE);
			for (char *tmp = PMC8_RESPONSE; *tmp; tmp++) {
				if (*tmp == '!') {
					*tmp = 0;
					break;
				}
			}
		}
		if (result) {
			break;
		}
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Command %s failed", command);
	}
cleanup:
	pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
	return result;
}

static void pmc8_set_mount_model(indigo_device *device, model_type type) {
	PRIVATE_DATA->type = type;
	strcpy(MOUNT_INFO_MODEL_ITEM->text.value, MODELS[type].name);
	double sec_per_count = 1296000.0/MODELS[type].count[0];
	PRIVATE_DATA->rate[0] = round(15.0 / sec_per_count * 25);
	PRIVATE_DATA->rate[1] = round(14.685 / sec_per_count * 25);
	PRIVATE_DATA->rate[2] = round(15.041 / sec_per_count * 25);
}

static void pmc8_update_mount_model(indigo_device *device) {
	if (MOUNT_TYPE_G11->sw.value) {
		pmc8_set_mount_model(device, PMC8_G11);
	} else if (MOUNT_TYPE_TITAN->sw.value) {
		pmc8_set_mount_model(device, PMC8_TITAN);
	} else if (MOUNT_TYPE_EXOS2->sw.value) {
		pmc8_set_mount_model(device, PMC8_EXOS2);
	} else if (MOUNT_TYPE_IEXOS100->sw.value) {
		pmc8_set_mount_model(device, PMC8_IEXOS100);
	}
}

static bool pmc8_read_mount_firmware(indigo_device *device) {
	if (!pmc8_command(device, "ESGv!") || strncmp(PMC8_RESPONSE, "ESGv", 4)) {
		return false;
	}
	strcpy(MOUNT_INFO_FIRMWARE_ITEM->text.value, PMC8_RESPONSE + 6);
	PRIVATE_DATA->version = atoi(PMC8_RESPONSE + 6);
	return true;
}

static bool pmc8_detect_mount_type(indigo_device *device) {
	if (!pmc8_read_mount_firmware(device)) {
		return false;
	}
	if (PRIVATE_DATA->version < 20) {
		if (strstr(PMC8_RESPONSE, "G11")) {
			pmc8_set_mount_model(device, PMC8_G11);
		} else if (strstr(PMC8_RESPONSE, "TITAN")) {
			pmc8_set_mount_model(device, PMC8_TITAN);
		} else if (strstr(PMC8_RESPONSE, "EXOS2")) {
			pmc8_set_mount_model(device, PMC8_EXOS2);
		} else if (strstr(PMC8_RESPONSE, "ES1A")) {
			pmc8_set_mount_model(device, PMC8_IEXOS100);
		}
	} else {
		if (pmc8_command(device, "ESGi!") && !strncmp(PMC8_RESPONSE, "ESGi", 4) && strlen(PMC8_RESPONSE) >= 22) {
			int type = 10 * (PMC8_RESPONSE[20] - '0') + PMC8_RESPONSE[21] - '0';
			if (type >= 4 && type <= 7) {
				pmc8_set_mount_model(device, PMC8_G11);
			} else if (type >= 8 && type <= 11) {
				pmc8_set_mount_model(device, PMC8_EXOS2);
			} else {
				// TBD there is iExos200/300 mentioned with no clear data
				pmc8_set_mount_model(device, PMC8_IEXOS100);
			}
		}
	}
	return true;
}

static bool pmc8_configure_mount(indigo_device *device) {
	for (int retry = 0; retry < 3; retry++) {
		if (MOUNT_TYPE_AUTO->sw.value ? pmc8_detect_mount_type(device) : pmc8_read_mount_firmware(device)) {
			if (!MOUNT_TYPE_AUTO->sw.value) {
				pmc8_update_mount_model(device);
			}
			return true;
		}
		indigo_send_message(device, BUSY_PROPERTY, "Retrying connection in 10 seconds...");
		indigo_sleep(10);
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to initialize to %s", DEVICE_PORT_ITEM->text.value);
	return false;
}

static void pmc8_close(indigo_device *device) {
	if (device->master_device != NULL) {
		device = device->master_device;
	}
	if (PRIVATE_DATA->handle != NULL) {
		indigo_uni_close(&PRIVATE_DATA->handle);
		PRIVATE_DATA->configured = false;
	}
}

static bool pmc8_get_tracking_rate(indigo_device *device) {
	if (pmc8_command(device, "ESGx!")) {
		int rate = (int)strtol(PMC8_RESPONSE + 4, NULL, 16);
		if (rate == 0) {
			indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
		} else {
			indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
			if (rate == PRIVATE_DATA->rate[0]) {
				indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SIDEREAL_ITEM, true);
			} else if (rate == PRIVATE_DATA->rate[1]) {
				indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_LUNAR_ITEM, true);
			} else if (rate == PRIVATE_DATA->rate[2]) {
				indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SOLAR_ITEM, true);
			}
		}
		return true;
	}
	return false;
}

static bool pmc8_set_tracking_rate(indigo_device *device, int offset) {
	int rate = 0;
	if (MOUNT_TRACKING_ON_ITEM->sw.value) {
		if (MOUNT_TRACK_RATE_SIDEREAL_ITEM->sw.value) {
			rate = PRIVATE_DATA->rate[0];
		} else if (MOUNT_TRACK_RATE_LUNAR_ITEM->sw.value) {
			rate = PRIVATE_DATA->rate[1];
		} else if (MOUNT_TRACK_RATE_SOLAR_ITEM->sw.value) {
			rate = PRIVATE_DATA->rate[2];
		}
	}
	if (MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value >= 0) {
		if (!pmc8_command(device, "ESSd01!")) {
			return false;
		}
	} else {
		if (!pmc8_command(device, "ESSd00!")) {
			return false;
		}
	}
	if (pmc8_command(device, "ESTr%04X!", rate + offset)) {
		return true;
	}
	return false;
}

static bool pmc8_stop_tracking(indigo_device *device) {
	if (pmc8_command(device, "ESTr0000!")) {
		return true;
	}
	return false;
}

static bool pmc8_point(indigo_device *device, bool sync, int32_t ha, int32_t dec) {
	if (!pmc8_command(device, sync ? "ESSp0%06X!" : "ESPt0%06X!", ha & 0xFFFFFF)) {
		return false;
	}
	if (!pmc8_command(device, sync ? "ESSp1%06X!" : "ESPt1%06X!", dec & 0xFFFFFF)) {
		return false;
	}
	return true;
}

static bool pmc8_get_position(indigo_device *device, int32_t *ha, int32_t *dec) {
	int32_t raw_ha = 0, raw_dec = 0;
	if (pmc8_command(device, "ESGp0!")) {
		raw_ha = (int)strtol(PMC8_RESPONSE + 5, NULL, 16);
		if (raw_ha & 0x800000) {
			raw_ha |= 0xFF000000;
		}
		if (pmc8_command(device, "ESGp1!")) {
			raw_dec = (int)strtol(PMC8_RESPONSE + 5, NULL, 16);
			if (raw_dec & 0x800000) {
				raw_dec |= 0xFF000000;
			}
			*ha = raw_ha;
			*dec = raw_dec;
			return true;
		}
	}
	return false;
}

static bool pmc8_get_rate(indigo_device *device, int32_t *ra, int32_t *dec) {
	if (pmc8_command(device, "ESGr0!")) {
		*ra = (int)strtol(PMC8_RESPONSE + 5, NULL, 16);
		if (pmc8_command(device, "ESGr1!")) {
			*dec = (int)strtol(PMC8_RESPONSE + 5, NULL, 16);
			return true;
		}
	}
	return false;
}

static bool pmc8_move(indigo_device *device, int axis, int direction, int rate) {
	if (rate == 0) {
		if (axis == 0) {
			return pmc8_set_tracking_rate(device, 0);
		} else {
			return pmc8_command(device, "ESSr10000!");
		}
	} else {
		if (!pmc8_command(device, "ESSd%d%d!", axis, direction)) {
			return false;
		}
		if (!pmc8_command(device, "ESSr%d%04X!", axis, abs(rate))) {
			return false;
		}
	}
	return true;
}

//- code

//+ mount.code

static void mount_connection_handler(indigo_device *device);

static void mount_switch_connection_handler(indigo_device *device) {
	CONNECTION_MODE_PROPERTY->state = INDIGO_OK_STATE;
    if (PRIVATE_DATA->is_udp && CONNECTION_TCP_ITEM->sw.value) {
			if (!pmc8_command(device, "ESY!") || strcmp(PMC8_RESPONSE, "ESY0")) {
				CONNECTION_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		mount_connection_handler(device);
    } else if (PRIVATE_DATA->is_udp && (CONNECTION_SERIAL_ITEM->sw.value || CONNECTION_SERIAL_DTR_ITEM->sw.value)) {
			indigo_send_message(device, ALERT_PROPERTY, "Can't switch from UDP to SERIAL directly, switch to TCP first!");
			indigo_set_switch(CONNECTION_MODE_PROPERTY, CONNECTION_UDP_ITEM, true);
			CONNECTION_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
    } else if (PRIVATE_DATA->is_tcp && CONNECTION_UDP_ITEM->sw.value) {
			if (!pmc8_command(device, "ESY!") || strcmp(PMC8_RESPONSE, "ESY1")) {
				CONNECTION_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		mount_connection_handler(device);
    } else if (PRIVATE_DATA->is_tcp && (CONNECTION_SERIAL_ITEM->sw.value || CONNECTION_SERIAL_DTR_ITEM->sw.value)) {
			if (!pmc8_command(device, "ESX!") || strcmp(PMC8_RESPONSE, "ESX0")) {
				CONNECTION_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		mount_connection_handler(device);
    } else if (PRIVATE_DATA->is_serial && CONNECTION_UDP_ITEM->sw.value) {
			indigo_send_message(device, ALERT_PROPERTY, "Can't switch from SERIAL to UDP directly, switch to TCP first!");
			indigo_set_switch(CONNECTION_MODE_PROPERTY, CONNECTION_SERIAL_ITEM, true);
			CONNECTION_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
    } else if (PRIVATE_DATA->is_serial && CONNECTION_TCP_ITEM->sw.value) {
			if (!pmc8_command(device, "ESX!") || strcmp(PMC8_RESPONSE, "ESX1")) {
				CONNECTION_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		mount_connection_handler(device);
    }
	if (CONNECTION_MODE_PROPERTY->state == INDIGO_OK_STATE) {
		pmc8_update_connection_port(device);
	}
	indigo_update_property(device, CONNECTION_MODE_PROPERTY, NULL);
}

//- mount.code

//+ guider.code

static void guider_guide_ra_finalizer(indigo_device *device) {
	if (!CONNECTION_CONNECTED_ITEM->sw.value) {
		return;
	}
	pmc8_set_tracking_rate(device->master_device, 0);
	GUIDER_GUIDE_EAST_ITEM->number.value = 0;
	GUIDER_GUIDE_WEST_ITEM->number.value = 0;
	GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
}

static void guider_guide_dec_finalizer(indigo_device *device) {
	if (!CONNECTION_CONNECTED_ITEM->sw.value) {
		return;
	}
	pmc8_move(device, 1, 0, 0);
	GUIDER_GUIDE_NORTH_ITEM->number.value = 0;
	GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
	GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
}

//- guider.code

#pragma mark - High level code (mount)

static void mount_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ mount.on_timer
	if (PRIVATE_DATA->handle != NULL) {
		int32_t raw_ha = 0, raw_dec = 0;
		if (pmc8_get_position(device, &raw_ha, &raw_dec)) {
			if (abs(raw_ha) < 0xFFF && abs(raw_dec) < 0xFFF && MOUNT_TRACKING_OFF_ITEM->sw.value && PRIVATE_DATA->park) {
				PRIVATE_DATA->park = false;
				MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
			}
			indigo_item *side_of_pier;
			uint32_t ra_count = MODELS[PRIVATE_DATA->type].count[0];
			uint32_t dec_count = MODELS[PRIVATE_DATA->type].count[1];
			double ha_angle = ((double)raw_ha / ra_count) * 24;
			double dec_angle = ((double)raw_dec / dec_count) * 360;
			double lst = indigo_lst(NULL, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value);
			double ha, ra, dec;
			if (MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value >= 0) {
				if (raw_dec >= -1) {
					dec = 90 - dec_angle;
					ha = ha_angle - 6;
					side_of_pier = MOUNT_SIDE_OF_PIER_WEST_ITEM;
				} else {
					dec = 90 + dec_angle;
					ha = ha_angle + 6;
					side_of_pier = MOUNT_SIDE_OF_PIER_EAST_ITEM;
				}
			} else {
				if (raw_dec >= -1) {
					dec = -90 + dec_angle;
					ha = -(ha_angle - 6);
					side_of_pier = MOUNT_SIDE_OF_PIER_EAST_ITEM;
				} else {
					dec = -90 - dec_angle;
					ha = -(ha_angle + 6);
					side_of_pier = MOUNT_SIDE_OF_PIER_WEST_ITEM;
				}
			}
			ra = lst - ha;
			if (ra < 0) {
				ra += 24;
			} else if (ra > 24)
				ra -= 24;
			indigo_eq_to_j2k(MOUNT_EPOCH_ITEM->number.value, &ra, &dec);
			MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = ra;
			MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = dec;
			if (!side_of_pier->sw.value) {
				indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, side_of_pier, true);
				indigo_update_property(device, MOUNT_SIDE_OF_PIER_PROPERTY, NULL);
			}
			indigo_update_coordinates(device, NULL);
			indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
		}
		indigo_execute_handler_in(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE ? 0.5 : 1, mount_timer_callback);
	}
	//- mount.on_timer
}

static void mount_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = pmc8_open(device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ mount.on_connect
			if (!PRIVATE_DATA->configured) {
				connection_result = pmc8_configure_mount(device);
			}
			if (connection_result) {
				PRIVATE_DATA->configured = true;
				indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
				if (pmc8_get_tracking_rate(device)) {
					MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
					MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_OK_STATE;
				} else {
					MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
					MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
				}
				pmc8_update_mount_type_perm(device, INDIGO_RO_PERM);
			}
			//- mount.on_connect
		}
		if (connection_result) {
			indigo_execute_handler(device, mount_timer_callback);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", MOUNT_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", MOUNT_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				pmc8_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ mount.on_disconnect
		pmc8_update_mount_type_perm(device, INDIGO_RW_PERM);
		//- mount.on_disconnect
		if (--PRIVATE_DATA->count == 0) {
			pmc8_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_mount_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void mount_connection_mode_handler(indigo_device *device) {
	CONNECTION_MODE_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.CONNECTION_MODE.on_change
	PRIVATE_DATA->is_udp = CONNECTION_UDP_ITEM->sw.value;
	PRIVATE_DATA->is_tcp = CONNECTION_TCP_ITEM->sw.value;
	PRIVATE_DATA->is_serial = (CONNECTION_SERIAL_ITEM->sw.value || CONNECTION_SERIAL_DTR_ITEM->sw.value);
	if (IS_CONNECTED) {
		CONNECTION_MODE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_MODE_PROPERTY, NULL);
		indigo_execute_handler(device, mount_switch_connection_handler);
	} else {
		pmc8_update_connection_port(device);
		CONNECTION_MODE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, CONNECTION_MODE_PROPERTY, NULL);
	}
	//- mount.CONNECTION_MODE.on_change
	indigo_update_property(device, CONNECTION_MODE_PROPERTY, NULL);
}

static void mount_type_handler(indigo_device *device) {
	MOUNT_TYPE_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_TYPE.on_change
	pmc8_update_mount_model(device);
	MOUNT_TYPE_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_TYPE_PROPERTY, NULL);
	//- mount.MOUNT_TYPE.on_change
	indigo_update_property(device, MOUNT_TYPE_PROPERTY, NULL);
}

static void mount_equatorial_coordinates_handler(indigo_device *device) {
	if (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {
		indigo_send_message(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, "Mount is parked!");
		INDIGO_UPDATE_PROPERTY_STATE(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, INDIGO_ALERT_STATE, NULL);
		return;
	}
	MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_EQUATORIAL_COORDINATES.on_change
	pmc8_stop_tracking(device);
	indigo_usleep(200000);
	for (int i = 0; i < 3 && MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE; i++) {
		double ra_angle = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target;
		double dec_angle = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target;
		indigo_j2k_to_eq(MOUNT_EPOCH_ITEM->number.value, &ra_angle, &dec_angle);
		double lst = indigo_lst(NULL, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value);
		double ha_angle = lst - ra_angle;
		if (ha_angle < -12) {
			ha_angle += 24;
		} else if (ha_angle >= 12) {
			ha_angle -= 24;
		}
		if (MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value >= 0) {
			if (ha_angle < 0) {
				ha_angle = ha_angle + 6;
				dec_angle = -(dec_angle - 90);
			} else {
				ha_angle = ha_angle - 6;
				dec_angle = dec_angle - 90;
			}
		} else {
			if (ha_angle < 0) {
				ha_angle = -(ha_angle + 6);
				dec_angle = -(dec_angle + 90);
			} else {
				ha_angle = -(ha_angle - 6);
				dec_angle = dec_angle + 90;
			}
		}
		uint32_t ra_count = MODELS[PRIVATE_DATA->type].count[0];
		uint32_t dec_count = MODELS[PRIVATE_DATA->type].count[1];
		int32_t raw_dec = (dec_angle / 360.0) * dec_count;
		int32_t raw_ha = (ha_angle / 24.0) * ra_count;
		if (!pmc8_point(device, MOUNT_ON_COORDINATES_SET_SYNC_ITEM->sw.value, raw_ha, raw_dec)) {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		if (MOUNT_ON_COORDINATES_SET_SYNC_ITEM->sw.value) {
			break;
		}		
		indigo_usleep(1000000);
		while (MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE) {
			int32_t ra_rate, dec_rate;
			if (pmc8_get_rate(device, &ra_rate, &dec_rate)) {
				if (ra_rate <= PRIVATE_DATA->rate[2] && dec_rate == 0) {
					break;
				}
			} else {
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			indigo_usleep(200000);
		}
		indigo_usleep(500000);
	}
	indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
	if (pmc8_set_tracking_rate(device, 0)) {
		MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
	if (MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE) {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, NULL);
	//- mount.MOUNT_EQUATORIAL_COORDINATES.on_change
	indigo_update_coordinates(device, NULL);
}

static void mount_tracking_handler(indigo_device *device) {
	if (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {
		indigo_send_message(device, MOUNT_TRACKING_PROPERTY, "Mount is parked!");
		INDIGO_UPDATE_PROPERTY_STATE(MOUNT_TRACKING_PROPERTY, INDIGO_ALERT_STATE, NULL);
		return;
	}
	MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_TRACKING.on_change
	if (pmc8_set_tracking_rate(device, 0)) {
		MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
	//- mount.MOUNT_TRACKING.on_change
	indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
}

static void mount_track_rate_handler(indigo_device *device) {
	MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_TRACK_RATE.on_change
	if (pmc8_set_tracking_rate(device, 0)) {
		MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
	//- mount.MOUNT_TRACK_RATE.on_change
	indigo_update_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
}

static void mount_park_handler(indigo_device *device) {
	MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_PARK.on_change
	MOUNT_TRACKING_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
	mount_tracking_handler(device);
	PRIVATE_DATA->park = true;
	if (!pmc8_point(device, false, 0, 0)) {
		MOUNT_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
	}
	//- mount.MOUNT_PARK.on_change
	indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
}

static void mount_abort_motion_handler(indigo_device *device) {
	MOUNT_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_ABORT_MOTION.on_change
	if (pmc8_move(device, 0, 0, 0) && pmc8_move(device, 1, 0, 0)) {
		MOUNT_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		MOUNT_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	if (MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE) {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, NULL);
	}
	MOUNT_MOTION_WEST_ITEM->sw.value = MOUNT_MOTION_EAST_ITEM->sw.value = false;
	indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
	MOUNT_MOTION_NORTH_ITEM->sw.value = MOUNT_MOTION_SOUTH_ITEM->sw.value = false;
	indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
	indigo_update_property(device, MOUNT_ABORT_MOTION_PROPERTY, NULL);
	//- mount.MOUNT_ABORT_MOTION.on_change
	indigo_update_property(device, MOUNT_ABORT_MOTION_PROPERTY, NULL);
}

static void mount_motion_dec_handler(indigo_device *device) {
	if (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {
		indigo_send_message(device, MOUNT_MOTION_DEC_PROPERTY, "Mount is parked!");
		INDIGO_UPDATE_PROPERTY_STATE(MOUNT_MOTION_DEC_PROPERTY, INDIGO_ALERT_STATE, NULL);
		return;
	}
	MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_MOTION_DEC.on_change
	int rate = 0;
	if (MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value) {
		rate = PRIVATE_DATA->rate[0];
	} else if (MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value) {
		rate = 0x1000;
	} else if (MOUNT_SLEW_RATE_FIND_ITEM->sw.value) {
		rate = 0x3000;
	} else if (MOUNT_SLEW_RATE_MAX_ITEM->sw.value) {
		rate = 0xFFFF;
	}
	int direction = 0, dec_rate = rate;
	if (MOUNT_MOTION_NORTH_ITEM->sw.value) {
		direction = 0;
	} else if (MOUNT_MOTION_SOUTH_ITEM->sw.value) {
		direction = 1;
	} else {
		dec_rate = 0;
	}
	if (pmc8_move(device, 1, direction, dec_rate)) {
		MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
	//- mount.MOUNT_MOTION_DEC.on_change
	indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
}

static void mount_motion_ra_handler(indigo_device *device) {
	if (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {
		indigo_send_message(device, MOUNT_MOTION_RA_PROPERTY, "Mount is parked!");
		INDIGO_UPDATE_PROPERTY_STATE(MOUNT_MOTION_RA_PROPERTY, INDIGO_ALERT_STATE, NULL);
		return;
	}
	MOUNT_MOTION_RA_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_MOTION_RA.on_change
	int rate = 0;
	if (MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value) {
		rate = PRIVATE_DATA->rate[0];
	} else if (MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value) {
		rate = 0x1000;
	} else if (MOUNT_SLEW_RATE_FIND_ITEM->sw.value) {
		rate = 0x3000;
	} else if (MOUNT_SLEW_RATE_MAX_ITEM->sw.value) {
		rate = 0xFFFF;
	}
	int direction = 0, ra_rate = rate;
	if (MOUNT_MOTION_WEST_ITEM->sw.value) {
		direction = 0;
	} else if (MOUNT_MOTION_EAST_ITEM->sw.value) {
		direction = 1;
	} else {
		ra_rate = 0;
	}
	if (pmc8_move(device, 0, direction, ra_rate)) {
		MOUNT_MOTION_RA_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		MOUNT_MOTION_RA_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
	//- mount.MOUNT_MOTION_RA.on_change
	indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
}

#pragma mark - Device API (mount)

static indigo_result mount_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result mount_attach(indigo_device *device) {
	if (indigo_mount_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		//+ mount.on_attach
		// -------------------------------------------------------------------------------- DEVICE_PORT
		strcpy(DEVICE_PORT_ITEM->text.value, "udp://192.168.47.1");
		DEVICE_PORT_PROPERTY->state = INDIGO_OK_STATE;
		// -------------------------------------------------------------------------------- MOUNT_INFO
		strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "Explore Scientific");
		// -------------------------------------------------------------------------------- MOUNT_ON_COORDINATES_SET
		MOUNT_ON_COORDINATES_SET_PROPERTY->count = 2;
		// -------------------------------------------------------------------------------- MOUNT_GUIDE_RATE
		MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- MOUNT_SIDE_OF_PIER
		MOUNT_SIDE_OF_PIER_PROPERTY->hidden = false;
		ADDITIONAL_INSTANCES_PROPERTY->hidden = device->base_device != NULL;
		pthread_mutex_init(&PRIVATE_DATA->port_mutex, NULL);
		//- mount.on_attach
		CONNECTION_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, CONNECTION_MODE_PROPERTY_NAME, MAIN_GROUP, "Connection mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 4);
		if (CONNECTION_MODE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(CONNECTION_UDP_ITEM, CONNECTION_UDP_ITEM_NAME, "UDP", true);
		indigo_init_switch_item(CONNECTION_TCP_ITEM, CONNECTION_TCP_ITEM_NAME, "TCP", false);
		indigo_init_switch_item(CONNECTION_SERIAL_ITEM, CONNECTION_SERIAL_ITEM_NAME, "Serial", false);
		indigo_init_switch_item(CONNECTION_SERIAL_DTR_ITEM, CONNECTION_SERIAL_DTR_ITEM_NAME, "Serial (clear DTR)", false);
		MOUNT_TYPE_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_TYPE_PROPERTY_NAME, MAIN_GROUP, "Mount type", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 5);
		if (MOUNT_TYPE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(MOUNT_TYPE_AUTO, MOUNT_TYPE_AUTO_ITEM_NAME, "Auto", true);
		indigo_init_switch_item(MOUNT_TYPE_G11, MOUNT_TYPE_G11_ITEM_NAME, MODELS[0].name, false);
		indigo_init_switch_item(MOUNT_TYPE_TITAN, MOUNT_TYPE_TITAN_ITEM_NAME, MODELS[1].name, false);
		indigo_init_switch_item(MOUNT_TYPE_EXOS2, MOUNT_TYPE_EXOS2_ITEM_NAME, MODELS[2].name, false);
		indigo_init_switch_item(MOUNT_TYPE_IEXOS100, MOUNT_TYPE_IEXOS100_ITEM_NAME, MODELS[3].name, false);
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->hidden = false;
		MOUNT_TRACKING_PROPERTY->hidden = false;
		MOUNT_TRACK_RATE_PROPERTY->hidden = false;
		MOUNT_PARK_PROPERTY->hidden = false;
		MOUNT_ABORT_MOTION_PROPERTY->hidden = false;
		MOUNT_MOTION_DEC_PROPERTY->hidden = false;
		MOUNT_MOTION_RA_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return mount_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result mount_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	INDIGO_DEFINE_MATCHING_PROPERTY(CONNECTION_MODE_PROPERTY);
	INDIGO_DEFINE_MATCHING_PROPERTY(MOUNT_TYPE_PROPERTY);
	return indigo_mount_enumerate_properties(device, client, property);
}

static indigo_result mount_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			indigo_execute_handler(device, mount_connection_handler);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CONNECTION_MODE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CONNECTION_MODE_PROPERTY, mount_connection_mode_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_TYPE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_TYPE_PROPERTY, mount_type_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, mount_equatorial_coordinates_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_TRACKING_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_TRACKING_PROPERTY, mount_tracking_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_TRACK_RATE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_TRACK_RATE_PROPERTY, mount_track_rate_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_PARK_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_PARK_PROPERTY, mount_park_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_ABORT_MOTION_PROPERTY, mount_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_MOTION_DEC_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE_ANYTIME(MOUNT_MOTION_DEC_PROPERTY, mount_motion_dec_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_MOTION_RA_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE_ANYTIME(MOUNT_MOTION_RA_PROPERTY, mount_motion_ra_handler);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, CONNECTION_MODE_PROPERTY);
			indigo_save_property(device, NULL, MOUNT_TYPE_PROPERTY);
		}
	}
	return indigo_mount_change_property(device, client, property);
}

static indigo_result mount_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		mount_connection_handler(device);
	}
	indigo_release_property(CONNECTION_MODE_PROPERTY);
	indigo_release_property(MOUNT_TYPE_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_mount_detach(device);
}

#pragma mark - High level code (guider)

static void guider_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = pmc8_open(device->master_device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ guider.on_connect
			if (!PRIVATE_DATA->configured) {
				connection_result = pmc8_configure_mount(device->master_device);
			}
			if (connection_result) {
				PRIVATE_DATA->configured = true;
			}
			//- guider.on_connect
		}
		if (connection_result) {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", GUIDER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", GUIDER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				pmc8_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ guider.on_disconnect
		indigo_cancel_pending_handlers(device);
		indigo_cancel_pending_handler(device, guider_guide_dec_finalizer);
		indigo_cancel_pending_handler(device, guider_guide_ra_finalizer);
		//- guider.on_disconnect
		if (--PRIVATE_DATA->count == 0) {
			pmc8_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_guider_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void guider_guide_ra_handler(indigo_device *device) {
	//+ guider.GUIDER_GUIDE_RA.on_change
	indigo_cancel_pending_handler(device, guider_guide_ra_finalizer);
	int rate = PRIVATE_DATA->rate[0] * (GUIDER_RATE_ITEM->number.value / 100.0);
	double duration = 0;
	if (GUIDER_GUIDE_EAST_ITEM->number.value > 0) {
		pmc8_set_tracking_rate(device->master_device, -rate);
		duration = GUIDER_GUIDE_EAST_ITEM->number.value / 1000.0;
	} else if (GUIDER_GUIDE_WEST_ITEM->number.value > 0) {
		pmc8_set_tracking_rate(device->master_device, rate);
		duration = GUIDER_GUIDE_WEST_ITEM->number.value / 1000.0;
	}
	if (duration > 0) {
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, duration, guider_guide_ra_finalizer);
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
	} else {
		guider_guide_ra_finalizer(device);
	}
	//- guider.GUIDER_GUIDE_RA.on_change
}

static void guider_guide_dec_handler(indigo_device *device) {
	//+ guider.GUIDER_GUIDE_DEC.on_change
	indigo_cancel_pending_handler(device, guider_guide_dec_finalizer);
	int rate = PRIVATE_DATA->rate[0] * (GUIDER_RATE_ITEM->number.value / 2500.0);
	double duration = 0;
	if (GUIDER_GUIDE_NORTH_ITEM->number.value > 0) {
		pmc8_move(device, 1, 1, rate);
		duration = GUIDER_GUIDE_NORTH_ITEM->number.value / 1000.0;
	} else if (GUIDER_GUIDE_SOUTH_ITEM->number.value > 0) {
		pmc8_move(device, 1, 0, rate);
		duration = GUIDER_GUIDE_SOUTH_ITEM->number.value / 1000.0;
	}
	if (duration > 0) {
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, duration, guider_guide_dec_finalizer);
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
	} else {
		guider_guide_dec_finalizer(device);
	}
	//- guider.GUIDER_GUIDE_DEC.on_change
}

#pragma mark - Device API (guider)

static indigo_result guider_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result guider_attach(indigo_device *device) {
	if (indigo_guider_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		//+ guider.on_attach
		// -------------------------------------------------------------------------------- GUIDER_RATE
		GUIDER_RATE_PROPERTY->hidden = false;
		// --------------------------------------------------------------------------------
		//- guider.on_attach
		GUIDER_GUIDE_RA_PROPERTY->hidden = false;
		GUIDER_GUIDE_DEC_PROPERTY->hidden = false;
		GUIDER_RATE_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result guider_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	return indigo_guider_enumerate_properties(device, client, property);
}

static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			indigo_execute_handler(device, guider_connection_handler);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_RA_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_PRIORITY_CHANGE(GUIDER_GUIDE_RA_PROPERTY, guider_guide_ra_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_PRIORITY_CHANGE(GUIDER_GUIDE_DEC_PROPERTY, guider_guide_dec_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_RATE_PROPERTY, property)) {
		indigo_property_copy_values(GUIDER_RATE_PROPERTY, property, false);
		GUIDER_RATE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_RATE_PROPERTY, NULL);
		return INDIGO_OK;
	}
	return indigo_guider_change_property(device, client, property);
}

static indigo_result guider_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		guider_connection_handler(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_guider_detach(device);
}

#pragma mark - Device templates

static indigo_device mount_template = INDIGO_DEVICE_INITIALIZER(MOUNT_DEVICE_NAME, mount_attach, mount_enumerate_properties, mount_change_property, NULL, mount_detach);

static indigo_device guider_template = INDIGO_DEVICE_INITIALIZER(GUIDER_DEVICE_NAME, guider_attach, guider_enumerate_properties, guider_change_property, NULL, guider_detach);

#pragma mark - Main code

indigo_result indigo_mount_pmc8(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static pmc8_private_data *private_data = NULL;
	static indigo_device *mount = NULL;
	static indigo_device *guider = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(pmc8_private_data));
			mount = indigo_safe_malloc_copy(sizeof(indigo_device), &mount_template);
			mount->private_data = private_data;
			indigo_attach_device(mount);
			guider = indigo_safe_malloc_copy(sizeof(indigo_device), &guider_template);
			guider->private_data = private_data;
			guider->master_device = mount;
			indigo_attach_device(guider);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			VERIFY_NOT_CONNECTED(mount);
			VERIFY_NOT_CONNECTED(guider);
			last_action = action;
			if (mount != NULL) {
				indigo_detach_device(mount);
				free(mount);
				mount = NULL;
			}
			if (guider != NULL) {
				indigo_detach_device(guider);
				free(guider);
				guider = NULL;
			}
			if (private_data != NULL) {
				free(private_data);
				private_data = NULL;
			}
			break;

		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
