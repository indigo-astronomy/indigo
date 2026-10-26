// Copyright (c) 2020-2025 CloudMakers, s. r. o. & Rumen G. Bogdanovski
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
//      & Rumen G. Bogdanovski <rumenastro@gmail.com>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "example_image.h"

#include <indigo/indigo_bus.h>
#include <indigo/indigo_names.h>
#include <indigo/indigo_client.h>

#define CCD_SIMULATOR remote_device

static char remote_device[INDIGO_NAME_SIZE];
#include "example_remote.h"

static bool connected = false;
static bool exposure_done = false;
static bool image_received = false;
static bool failed = false;
static int count = 1;

static void next_exposure(indigo_device *device, indigo_client *client) {
	if (exposure_done && image_received) {
		exposure_done = false;
		image_received = false;
		if (--count > 0) {
			static const char *items[] = { CCD_EXPOSURE_ITEM_NAME };
			static const double values[] = { 3.0 };
			indigo_change_number_property(client, CCD_SIMULATOR, CCD_EXPOSURE_PROPERTY_NAME, 1, items, values);
		} else {
			indigo_device_disconnect(client, CCD_SIMULATOR);
		}
	}
}

static indigo_result client_attach(indigo_client *client) {
	indigo_log("attached to INDIGO bus...");
	return INDIGO_OK;
}

static indigo_result client_define_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (strcmp(property->device, CCD_SIMULATOR))
		return INDIGO_OK;
	if (!strcmp(property->name, CONNECTION_PROPERTY_NAME)) {
		if (indigo_get_switch(property, CONNECTION_CONNECTED_ITEM_NAME)) {
			connected = true;
			indigo_log("already connected...");
			static const char * items[] = { CCD_EXPOSURE_ITEM_NAME };
			static double values[] = { 3.0 };
			indigo_change_number_property(client, CCD_SIMULATOR, CCD_EXPOSURE_PROPERTY_NAME, 1, items, values);
		} else {
			indigo_device_connect(client, CCD_SIMULATOR);
			return INDIGO_OK;
		}
	}
	if (!strcmp(property->name, "FILE_NAME")) {
		char value[1024] = { 0 };
		static const char * items[] = { "PATH" };
		static const char *values[1];
		values[0] = value;
		for (int i = 0 ; i < 1023; i++)
			value[i] = '0' + i % 10;
		indigo_change_text_property(client, CCD_SIMULATOR, "FILE_NAME", 1, items, values);
	}
	if (!strcmp(property->name, CCD_IMAGE_PROPERTY_NAME)) {
		if (device->version >= INDIGO_VERSION_2_0)
			indigo_enable_blob(client, property, INDIGO_ENABLE_BLOB_URL);
		else
			indigo_enable_blob(client, property, INDIGO_ENABLE_BLOB_ALSO);
	}
	if (!strcmp(property->name, CCD_IMAGE_FORMAT_PROPERTY_NAME)) {
		static const char * items[] = { CCD_IMAGE_FORMAT_FITS_ITEM_NAME };
		static bool values[] = { true };
		indigo_change_switch_property(client, CCD_SIMULATOR, CCD_IMAGE_FORMAT_PROPERTY_NAME, 1, items, values);
	}
	return INDIGO_OK;
}

static indigo_result client_update_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (strcmp(property->device, CCD_SIMULATOR))
		return INDIGO_OK;
	static const char * items[] = { CCD_EXPOSURE_ITEM_NAME };
	static double values[] = { 3.0 };
	if (!strcmp(property->name, CONNECTION_PROPERTY_NAME) && property->state == INDIGO_OK_STATE) {
		if (indigo_get_switch(property, CONNECTION_CONNECTED_ITEM_NAME)) {
			if (!connected) {
				connected = true;
				indigo_log("connected...");
				indigo_change_number_property(client, CCD_SIMULATOR, CCD_EXPOSURE_PROPERTY_NAME, 1, items, values);
			}
		} else {
			if (connected) {
				indigo_log("disconnected...");
				connected = false;
				if (count > 0) {
					failed = true;
				}
				example_finish();
			}
		}
		return INDIGO_OK;
	}
	if (!strcmp(property->name, CCD_IMAGE_PROPERTY_NAME) && property->state == INDIGO_OK_STATE && count > 0) {
		indigo_item *image = &property->items[0];
		bool populated = !*image->blob.url || indigo_populate_http_blob_item(image);
		char name[32];
		snprintf(name, sizeof(name), "img_%02d.fits", count);
		bool saved = populated && image->blob.value && image->blob.size > 0 && example_save_image(name, image->blob.value, image->blob.size);
		if (*image->blob.url) {
			free(image->blob.value);
			image->blob.value = NULL;
		}
		if (!saved) {
			indigo_error("Image capture could not be saved");
			failed = true;
			indigo_device_disconnect(client, CCD_SIMULATOR);
		} else {
			image_received = true;
			next_exposure(device, client);
		}
	}
	if (!strcmp(property->name, CCD_EXPOSURE_PROPERTY_NAME)) {
		if (property->state == INDIGO_BUSY_STATE) {
			indigo_log("exposure %gs...", property->items[0].number.value);
		} else if (property->state == INDIGO_OK_STATE) {
			indigo_log("exposure done...");
			exposure_done = true;
			next_exposure(device, client);
		}
		return INDIGO_OK;
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
	if (!example_remote_options(argc, argv, &host, &service, remote_device, "CCD Imager Simulator", &timeout, &port)) {
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
