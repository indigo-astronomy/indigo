// Copyright (c) 2020 CloudMakers, s. r. o. & Rumen G.Bogdanovski
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
//      & Rumen G.Bogdanovski <rumenastro@gmail.com>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
#include <unistd.h>
#endif
#if defined(INDIGO_WINDOWS)
#include <windows.h>
#pragma warning(disable:4996)
#endif
#include <indigo/indigo_bus.h>
#include <indigo/indigo_client.h>

#define SERVICE	"localhost"
#define CCD_SIMULATOR "CCD Imager Simulator @ " SERVICE

static bool connected = false;
static int count = 1;

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

int main(int argc, const char * argv[]) {
	indigo_main_argc = argc;
	indigo_main_argv = argv;
//#if defined(INDIGO_WINDOWS)
//	freopen("indigo.log", "w", stderr);
//#endif

	indigo_set_log_level(INDIGO_LOG_TRACE);
	indigo_start();

	indigo_server_entry *server;
	indigo_attach_client(&client);
	indigo_connect_server(SERVICE, SERVICE ".local", 7624, &server); // Check correct host name in 2nd arg!!!
	while (count > 0) {
		  indigo_sleep(1);
	}
	indigo_disconnect_server(server);
	indigo_detach_client(&client);
	indigo_stop();
	return 0;
}
