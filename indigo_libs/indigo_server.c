//  Copyright (c) 2016 CloudMakers, s. r. o.
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions
//  are met:
//
//  1. Redistributions of source code must retain the above copyright
//  notice, this list of conditions and the following disclaimer.
//
//  2. Redistributions in binary form must reproduce the above
//  copyright notice, this list of conditions and the following
//  disclaimer in the documentation and/or other materials provided
//  with the distribution.
//
//  3. The name of the author may not be used to endorse or promote
//  products derived from this software without specific prior
//  written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE AUTHOR 'AS IS' AND ANY EXPRESS
//  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
//  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
//  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
//  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
//  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//  version history
//  2.0 Build 0 - PoC by Peter Polakovic <peter.polakovic@cloudmakers.eu>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <assert.h>

#include "indigo_bus.h"
#include "indigo_server_xml.h"

#include "ccd_simulator/indigo_ccd_simulator.h"
#include "ccd_sx/indigo_ccd_sx.h"
#include "ccd_ssag/indigo_ccd_ssag.h"
#include "ccd_asi/indigo_ccd_asi.h"
#include "ccd_atik/indigo_ccd_atik.h"
#include "wheel_sx/indigo_wheel_sx.h"
#include "focuser_fcusb/indigo_focuser_fcusb.h"

#define MAX_DRIVERS	100
#define SERVER_NAME	"INDIGO Server"

static struct {
	const char *name;
	indigo_result (*driver)(bool state);
} drivers[] = {
	{ "Simulator", indigo_ccd_simulator },
	{ "SX CCD", indigo_ccd_sx },
	{ "SX Filter Wheel", indigo_wheel_sx },
	{ "Atik CCD", indigo_ccd_atik },
	{ "SSAG/QHY5 CCD", indigo_ccd_ssag },
	{ "Shoestring FCUSB Focuser", indigo_focuser_fcusb },
	{ NULL, NULL }
};

static int first_driver = 1;
static indigo_property *driver_property;

static void server_callback(int count) {
	INDIGO_LOG(indigo_log("%d clients", count));
}

static indigo_result change_property(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result attach(indigo_device *device) {
	assert(device != NULL);
	driver_property = indigo_init_switch_property(NULL, "INDIGO Server", "DRIVERS", "Main", "Active drivers", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, MAX_DRIVERS);
	driver_property->count = 0;
	for (int i = first_driver; i < MAX_DRIVERS && drivers[i].name; i++) {
		indigo_init_switch_item(&driver_property->items[driver_property->count++], drivers[i].name, drivers[i].name, true);
	}
	if (indigo_load_properties(device, false) == INDIGO_FAILED)
		change_property(device, NULL, driver_property);
	INDIGO_LOG(indigo_log("%s attached", device->name));
	return INDIGO_OK;
}

static indigo_result enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	indigo_define_property(device, driver_property, NULL);
	return INDIGO_OK;
}

static indigo_result change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(property != NULL);
	if (indigo_property_match(driver_property, property)) {
	// -------------------------------------------------------------------------------- DRIVERS
		indigo_property_copy_values(driver_property, property, false);
		for (int i = 0; i < driver_property->count; i++)
			drivers[i + first_driver].driver(driver_property->items[i].sw.value);
		driver_property->state = INDIGO_OK_STATE;
		indigo_update_property(device, driver_property, NULL);
		int handle = 0;
		indigo_save_property(device, &handle, driver_property);
		close(handle);
	// --------------------------------------------------------------------------------
	}
	return INDIGO_OK;
}

static indigo_result detach(indigo_device *device) {
	assert(device != NULL);
	INDIGO_LOG(indigo_log("%s detached", device->name));
	return INDIGO_OK;
}

int main(int argc, const char * argv[]) {
	indigo_main_argc = argc;
	indigo_main_argv = argv;
	
	indigo_log("INDIGO server %d.%d-%d built on %s", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF, INDIGO_BUILD, __TIMESTAMP__);
	
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-s") || !strcmp(argv[i], "--enable-simulators"))
			first_driver = 0;
		else if ((!strcmp(argv[i], "-p") || !strcmp(argv[i], "--port")) && i < argc - 1)
			indigo_server_xml_port = atoi(argv[i+1]);
		else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
			printf("\n%s [-s|--enable-simulators] [-p|--port port] [-h|--help]\n\n", argv[0]);
			exit(0);
		}
	}
	
	static indigo_device device = {
		SERVER_NAME, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
		attach,
		enumerate_properties,
		change_property,
		detach
	};

	if (strstr(argv[0], "macOS"))
		indigo_use_syslog = true; // embeded into INDIGO Server for macOS
	
	indigo_start();
	indigo_attach_device(&device);

	indigo_server_xml(server_callback);
	return 0;
}

