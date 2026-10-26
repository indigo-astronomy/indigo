// Copyright (c) 2024 Rumen G.Bogdanovski
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
// 2.0 by Rumen G.Bogdanovski <rumenastro@gmail.com>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <indigo/indigo_bus.h>
#include <indigo/indigo_names.h>
#include <indigo/indigo_client.h>

#define MOUNT_SIMULATOR remote_device

static char remote_device[INDIGO_NAME_SIZE];
#include "example_remote.h"

#define RA   3.1416 /* in decimal hours */
#define DEC  2.7183 /* in degrees */

static bool connected = false;
static bool slew_completed = false;
static bool failed = false;

static void slew_mount(indigo_client *client, double ra, double dec) {
	static const char * items[] = {
		MOUNT_EQUATORIAL_COORDINATES_RA_ITEM_NAME,
		MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM_NAME
	};
	static double values[2];
	values[0] = ra;
	values[1] = dec;

	indigo_change_number_property(client, MOUNT_SIMULATOR, MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, 2, items, values);
}

static bool get_mount_coordinates(indigo_property *property, double *ra, double *dec) {
	if (strcmp(property->name, MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME))
		return false;

	indigo_item *item = indigo_get_item(property, MOUNT_EQUATORIAL_COORDINATES_RA_ITEM_NAME);
	if (item) {
		*ra = item->number.value;
	} else {
		return false;
	}

	item = indigo_get_item(property, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM_NAME);
	if (item) {
		*dec = item->number.value;
	} else {
		return false;
	}

	return true;
}

static void unpark_mount(indigo_client *client) {
	static const char * items[] = { MOUNT_PARK_UNPARKED_ITEM_NAME };
	static bool values[] = { true };
	indigo_change_switch_property(client, MOUNT_SIMULATOR, MOUNT_PARK_PROPERTY_NAME, 1, items, values);
}

static indigo_result client_attach(indigo_client *client) {
	indigo_log("attached to INDIGO bus...");
	indigo_enumerate_properties(client, &INDIGO_ALL_PROPERTIES);
	return INDIGO_OK;
}

static indigo_result client_define_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (strcmp(property->device, MOUNT_SIMULATOR))
		return INDIGO_OK;
	if (!strcmp(property->name, CONNECTION_PROPERTY_NAME)) {
		if (indigo_get_switch(property, CONNECTION_CONNECTED_ITEM_NAME)) {
			connected = true;
			indigo_log("already connected...");
			unpark_mount(client);
		} else {
			indigo_device_connect(client, MOUNT_SIMULATOR);
			return INDIGO_OK;
		}
	} else if (!strcmp(property->name, MOUNT_PARK_PROPERTY_NAME) && property->state == INDIGO_OK_STATE) {
		if (indigo_get_switch(property, MOUNT_PARK_UNPARKED_ITEM_NAME)) {
			indigo_log("already unparked...");
			slew_mount(client, RA, DEC);
			indigo_log("target set: RA = %f, Dec = %f", RA, DEC);
		}
	}
	return INDIGO_OK;
}

static indigo_result client_update_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (strcmp(property->device, MOUNT_SIMULATOR))
		return INDIGO_OK;

	if (!strcmp(property->name, CONNECTION_PROPERTY_NAME) && property->state == INDIGO_OK_STATE) {
		if (indigo_get_switch(property, CONNECTION_CONNECTED_ITEM_NAME)) {
			if (!connected) {
				connected = true;
				indigo_log("connected...");
				unpark_mount(client);
			}
		} else {
			if (connected) {
				indigo_log("disconnected...");
				connected = false;
				if (!slew_completed) {
					failed = true;
				}
				example_finish();
			}
		}
	} else if (!strcmp(property->name, MOUNT_PARK_PROPERTY_NAME) && property->state == INDIGO_OK_STATE) {
		if (indigo_get_switch(property, MOUNT_PARK_UNPARKED_ITEM_NAME)) {
			indigo_log("unparked...");
			slew_mount(client, RA, DEC);
			indigo_log("target set: RA = %f, Dec = %f", RA, DEC);
		}
	} else if (!strcmp(property->name, MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME)) {
		if (property->state == INDIGO_BUSY_STATE) {
			double ra = 0, dec = 0;
			if (get_mount_coordinates(property, &ra, &dec)) {
				indigo_log("slewing to target... (current RA = %f, Dec = %f)", ra, dec);
			} else {
				indigo_error("slewing to target...");
			}
		} else if (property->state == INDIGO_ALERT_STATE) {
			indigo_error("slew failed");
			failed = true;
			indigo_device_disconnect(client, MOUNT_SIMULATOR);
		} else {
			double ra = 0, dec = 0;
			if (get_mount_coordinates(property, &ra, &dec)) {
				indigo_log("slew finished: RA = %f, Dec = %f", ra, dec);
				slew_completed = true;
			} else {
				indigo_error("slew finished, but coordinates can not be read!");
				failed = true;
			}
			indigo_device_disconnect(client, MOUNT_SIMULATOR);
		}
	}
	return INDIGO_OK;
}

static indigo_result client_detach(indigo_client *client) {
	indigo_log("detached from INDIGO bus...");
	return INDIGO_OK;
}

static indigo_client client = {
	"Remote server client", false, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT, NULL,
	client_attach,
	client_define_property,
	client_update_property,
	NULL,
	NULL,
	client_detach
};

int main(int argc, const char *argv[]) {
	const char *host, *service;
	int timeout, port;
	if (!example_remote_options(argc, argv, &host, &service, remote_device, "Mount Simulator", &timeout, &port)) {
		fprintf(stderr, "Usage: %s [host [service [device [timeout_seconds [port]]]]]\n", argv[0]);
		return EXIT_FAILURE;
	}
	indigo_main_argc = argc;
	indigo_main_argv = argv;
	indigo_set_log_level(INDIGO_LOG_INFO);
	indigo_start();
	indigo_server_entry *server = NULL;
	int result = EXIT_FAILURE;
	indigo_attach_client(&client);
	if (indigo_connect_server(service, host, port, &server, NULL) == INDIGO_OK) {
		for (int remaining = timeout; !example_is_finished() && remaining > 0; remaining--) {
			indigo_sleep(1);
		}
		if (!example_is_finished()) {
			indigo_error("Timed out waiting for %s on %s:%d", remote_device, host, port);
		} else if (!failed) {
			result = EXIT_SUCCESS;
		}
		indigo_disconnect_server(server);
		/* The server API owns a detached parser thread; let it release its adapter before stopping the bus. */
		for (int remaining = 100; server->thread_started && remaining > 0; remaining--) {
			indigo_usleep(100000);
		}
		if (server->thread_started) {
			indigo_error("Server shutdown timed out");
			return EXIT_FAILURE;
		}
	} else {
		indigo_error("Cannot start server connection to %s:%d", host, port);
	}
	indigo_detach_client(&client);
	indigo_stop();
	return result;
}
