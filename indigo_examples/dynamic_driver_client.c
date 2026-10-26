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
#include <stdatomic.h>
#include "example_image.h"

#include <indigo/indigo_bus.h>
#include <indigo/indigo_names.h>
#include <indigo/indigo_client.h>

#define CCD_SIMULATOR "CCD Imager Simulator"

static atomic_bool connected = false;
static atomic_bool config_ready = false;
static atomic_bool capture_started = false;
static bool exposure_done = false;
static bool image_received = false;
static atomic_bool finished = false;
static atomic_bool failed = false;
static int count = 5;


/* If we link the client to the driver we can not call indigo_change_XXX_property() from property handles.
   We need to call it asynchronously. This is achieved with indigo_handle_property_async().
*/

static void start_exposure(indigo_device *device, indigo_client *client, indigo_property *property) {
	static const char * items[] = { CCD_EXPOSURE_ITEM_NAME };
	static const double values[] = { 3.0 };
	indigo_change_number_property(client, CCD_SIMULATOR, CCD_EXPOSURE_PROPERTY_NAME, 1, items, values);
}

static void prepare_capture(indigo_device *device, indigo_client *client, indigo_property *property) {
	static const char *upload_items[] = { CCD_UPLOAD_MODE_CLIENT_ITEM_NAME };
	static const char *format_items[] = { CCD_IMAGE_FORMAT_FITS_ITEM_NAME };
	static const bool values[] = { true };
	indigo_change_switch_property(client, CCD_SIMULATOR, CCD_UPLOAD_MODE_PROPERTY_NAME, 1, upload_items, values);
	indigo_change_switch_property(client, CCD_SIMULATOR, CCD_IMAGE_FORMAT_PROPERTY_NAME, 1, format_items, values);
	start_exposure(device, client, property);
}

static void start_capture_if_ready(indigo_device *device, indigo_client *client) {
	if (connected && config_ready && !atomic_exchange(&capture_started, true)) {
		indigo_handle_property_async(prepare_capture, device, client, NULL);
	}
}

static void next_exposure(indigo_device *device, indigo_client *client) {
	if (exposure_done && image_received) {
		exposure_done = false;
		image_received = false;
		if (--count > 0) {
			indigo_handle_property_async(start_exposure, device, client, NULL);
		} else {
			indigo_device_disconnect(client, CCD_SIMULATOR);
		}
	}
}

static indigo_result client_attach(indigo_client *client) {
	indigo_log("attached to INDIGO bus...");
	indigo_enumerate_properties(client, &INDIGO_ALL_PROPERTIES);
	return INDIGO_OK;
}

static indigo_result client_define_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (strcmp(property->device, CCD_SIMULATOR))
		return INDIGO_OK;
	/* SAVE-only CONFIG means there is no stored configuration to restore. */
	if (!strcmp(property->name, CONFIG_PROPERTY_NAME) && indigo_get_item(property, CONFIG_LOAD_ITEM_NAME) == NULL) {
		config_ready = true;
		start_capture_if_ready(device, client);
	}
	if (!strcmp(property->name, CONNECTION_PROPERTY_NAME)) {
		if (indigo_get_switch(property, CONNECTION_CONNECTED_ITEM_NAME)) {
			connected = true;
			indigo_log("already connected...");
			start_capture_if_ready(device, client);
		} else {
			indigo_device_connect(client, CCD_SIMULATOR);
			return INDIGO_OK;
		}
	}
	if (!strcmp(property->name, CCD_IMAGE_PROPERTY_NAME)) {
		indigo_enable_blob(client, property, INDIGO_ENABLE_BLOB_ALSO);
	}
	return INDIGO_OK;
}

static indigo_result client_update_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (strcmp(property->device, CCD_SIMULATOR))
		return INDIGO_OK;

	/* Automatic configuration restore can override upload mode after connection. */
	if (!strcmp(property->name, CONFIG_PROPERTY_NAME)) {
		config_ready = property->state == INDIGO_OK_STATE || property->state == INDIGO_ALERT_STATE;
		start_capture_if_ready(device, client);
	}
	if (!strcmp(property->name, CONNECTION_PROPERTY_NAME) && property->state == INDIGO_OK_STATE) {
		if (indigo_get_switch(property, CONNECTION_CONNECTED_ITEM_NAME)) {
			if (!connected) {
				connected = true;
				indigo_log("connected...");
				start_capture_if_ready(device, client);
			}
		} else {
			if (connected) {
				indigo_log("disconnected...");
				connected = false;
				if (count > 0) {
					failed = true;
				}
				finished = true;
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
	"Dynamic driver client", false, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT, NULL,
	client_attach,
	client_define_property,
	client_update_property,
	NULL,
	NULL,
	client_detach
};

int main(int argc, const char * argv[]) {
	indigo_main_argc = argc;
	indigo_main_argv = argv;

	indigo_set_log_level(INDIGO_LOG_INFO);
	indigo_start();

	indigo_driver_entry *driver = NULL;
	int result = EXIT_FAILURE;
	indigo_attach_client(&client);
	if (indigo_load_driver("indigo_ccd_simulator", true, &driver) == INDIGO_OK) {
		for (int remaining = 120; !finished && remaining > 0; remaining--) {
			indigo_sleep(1);
		}
		if (!finished) {
			indigo_error("Timed out waiting for simulator capture");
		} else if (!failed) {
			result = EXIT_SUCCESS;
		}
		if (connected) {
			indigo_device_disconnect(&client, CCD_SIMULATOR);
			for (int remaining = 50; connected && remaining > 0; remaining--) {
				indigo_usleep(100000);
			}
		}
		if (indigo_remove_driver(driver) != INDIGO_OK) {
			indigo_error("Cannot unload simulator driver");
			result = EXIT_FAILURE;
		}
	} else {
		indigo_error("Cannot load indigo_ccd_simulator");
	}
	indigo_detach_client(&client);
	indigo_stop();
	return result;
}
