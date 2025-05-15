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
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <indigo/indigo_bus.h>
#include <indigo/indigo_client.h>
#include <indigo/indigo_client_xml.h>

#define CCD_SIMULATOR "CCD Imager Simulator @ indigo_ccd_simulator"

static int device_pid;
static bool connected = false;
static int count = 5;

static indigo_result client_attach(indigo_client *client) {
	indigo_log("attached to INDIGO bus...");
	indigo_enumerate_properties(client, &INDIGO_ALL_PROPERTIES);
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
				indigo_stop();
				connected = false;
			}
		}
		return INDIGO_OK;
	}
	if (!strcmp(property->name, CCD_IMAGE_PROPERTY_NAME) && property->state == INDIGO_OK_STATE) {
		/* URL blob transfer is available only in client - server setup.
		   This will never be called in case of a client loading a driver. */
		if (*property->items[0].blob.url && indigo_populate_http_blob_item(&property->items[0]))
			indigo_log("image URL received (%s, %d bytes)...", property->items[0].blob.url, property->items[0].blob.size);

		if (property->items[0].blob.value) {
			char name[32];
			sprintf(name, "img_%02d.fits", count);
			FILE *f = fopen(name, "wb");
			fwrite(property->items[0].blob.value, property->items[0].blob.size, 1, f);
			fclose(f);
			indigo_log("image saved to %s...", name);
			/* In case we have URL BLOB transfer we need to release the blob ourselves */
			if (*property->items[0].blob.url) {
				free(property->items[0].blob.value);
				property->items[0].blob.value = NULL;
			}
		}
	}
	if (!strcmp(property->name, CCD_EXPOSURE_PROPERTY_NAME)) {
		if (property->state == INDIGO_BUSY_STATE) {
			indigo_log("exposure %gs...", property->items[0].number.value);
		} else if (property->state == INDIGO_OK_STATE) {
			indigo_log("exposure done...");
			if (--count > 0) {
				indigo_change_number_property(client, CCD_SIMULATOR, CCD_EXPOSURE_PROPERTY_NAME, 1, items, values);
			} else {
				indigo_device_disconnect(client, CCD_SIMULATOR);
			}
		}
		return INDIGO_OK;
	}
	return INDIGO_OK;
}

static indigo_result client_detach(indigo_client *client) {
	indigo_log("detached from INDIGO bus...");
	kill(device_pid, SIGKILL);
	exit(0);
	return INDIGO_OK;
}

static indigo_client client = {
	"Executable driver client", false, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT, NULL,
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
	int input[2], output[2];
	if (pipe(input) < 0 || pipe(output) < 0) {
		indigo_log("Can't create local pipe for device (%s)", strerror(errno));
		return 0;
	}
	device_pid = fork();
	if (device_pid == 0) {
		close(0);
		dup2(output[0], 0);
		close(1);
		dup2(input[1], 1);
		execl("../build/drivers/indigo_ccd_simulator", "indigo_ccd_simulator", NULL);
	} else {
		close(input[1]);
		close(output[0]);
		indigo_set_log_level(INDIGO_LOG_INFO);
		indigo_start();
		indigo_device *protocol_adapter = indigo_xml_client_adapter("indigo_ccd_simulator", "", input[0], output[1]);
		indigo_attach_device(protocol_adapter);
		indigo_attach_client(&client);
		indigo_xml_parse(protocol_adapter, &client);
		indigo_stop();
	}
	return 0;
}
