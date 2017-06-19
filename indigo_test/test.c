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
// 2.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "indigo_bus.h"
#include "indigo_client.h"
#include "ccd_simulator/indigo_ccd_simulator.h"

#include "indigo_json.h"
#include "indigo_driver_json.h"

#define CCD_SIMULATOR "CCD Imager Simulator"

static bool connected = false;

static indigo_result test_attach(indigo_client *client) {
	indigo_log("attached to INDI bus...");
	indigo_enumerate_properties(client, &INDIGO_ALL_PROPERTIES);
	return INDIGO_OK;
}

static indigo_result test_define_property(struct indigo_client *client, struct indigo_device *device, indigo_property *property, const char *message) {
	if (strcmp(property->device, CCD_SIMULATOR_IMAGER_CAMERA_NAME))
		return INDIGO_OK;
	if (!strcmp(property->name, CONNECTION_PROPERTY_NAME)) {
		indigo_device_connect(client, CCD_SIMULATOR);
		return INDIGO_OK;
	}
	return INDIGO_OK;
}

static indigo_result test_update_property(struct indigo_client *client, struct indigo_device *device, indigo_property *property, const char *message) {
	if (strcmp(property->device, CCD_SIMULATOR_IMAGER_CAMERA_NAME))
		return INDIGO_OK;
	if (!strcmp(property->name, CONNECTION_PROPERTY_NAME) && property->state == INDIGO_OK_STATE) {
		if (indigo_get_switch(property, CONNECTION_CONNECTED_ITEM_NAME)) {
			if (!connected) {
				connected = true;
				indigo_log("connected...");
				static const char * items[] = { CCD_EXPOSURE_ITEM_NAME };
				static double values[] = { 3.0 };
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
	if (!strcmp(property->name, CCD_EXPOSURE_PROPERTY_NAME)) {
		if (property->state == INDIGO_BUSY_STATE) {
			indigo_log("exposure %gs...", property->items[0].number.value);
		} else if (property->state == INDIGO_OK_STATE) {
			indigo_log("exposure done...");
		}
		return INDIGO_OK;
	}
	if (!strcmp(property->name, CCD_IMAGE_PROPERTY_NAME) && property->state == INDIGO_OK_STATE) {
		indigo_log("image received (%d bytes)...", property->items[0].blob.size);
		indigo_device_disconnect(client, CCD_SIMULATOR);
	}
	return INDIGO_OK;
}

static indigo_result test_detach(indigo_client *client) {
	indigo_log("detached from INDI bus...");
	exit(0);
	return INDIGO_OK;
}

static indigo_client test = {
	"Test", NULL, INDIGO_OK, INDIGO_VERSION_CURRENT, INDIGO_ENABLE_BLOB_ALSO,
	test_attach,
	test_define_property,
	test_update_property,
	NULL,
	NULL,
	test_detach
};


int main(int argc, const char * argv[]) {
	indigo_main_argc = argc;
	indigo_main_argv = argv;
	indigo_start();

	indigo_client *protocol_adapter = indigo_json_device_adapter(0, 1, 0);
	indigo_attach_client(protocol_adapter);

	indigo_driver_entry *driver;
	indigo_add_driver(&indigo_ccd_simulator, true, &driver);
	indigo_attach_client(&test);
	sleep(1000);
	indigo_remove_driver(driver);
	indigo_stop();
	return 0;
}

