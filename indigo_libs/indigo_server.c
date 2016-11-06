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
// 2.0 Build 0 - PoC by Peter Polakovic <peter.polakovic@cloudmakers.eu>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <assert.h>
#include <libgen.h>
#include <signal.h>
#include <dlfcn.h>

#include "indigo_bus.h"
#include "indigo_server_xml.h"
#include "indigo_driver.h"

//#define STATIC_DRIVERS

#ifdef STATIC_DRIVERS
#include "ccd_simulator/indigo_ccd_simulator.h"
#include "mount_simulator/indigo_mount_simulator.h"
#include "ccd_sx/indigo_ccd_sx.h"
#include "wheel_sx/indigo_wheel_sx.h"
#include "ccd_ssag/indigo_ccd_ssag.h"
#include "ccd_asi/indigo_ccd_asi.h"
#include "wheel_asi/indigo_wheel_asi.h"
#include "ccd_atik/indigo_ccd_atik.h"
#include "ccd_qhy/indigo_ccd_qhy.h"
#include "focuser_fcusb/indigo_focuser_fcusb.h"
#endif

#define MAX_DRIVERS	100
#define SERVER_NAME	"INDIGO Server"

#ifdef STATIC_DRIVERS
static struct {
	const char *name;
	const char *drv_name;
	indigo_result (*driver)(bool state);
} static_drivers[] = {
	{ "CCD Simulator", "indigo_ccd_simulator", indigo_ccd_simulator },
	{ "Mount Simulator", "indigo_mount_simulator", indigo_mount_simulator },
	{ "SX CCD", "indigo_ccd_sx", indigo_ccd_sx },
	{ "SX Filter Wheel","indigo_wheel_sx", indigo_wheel_sx },
	{ "Atik CCD", "indigo_ccd_atik", indigo_ccd_atik },
	{ "QHY CCD", "indigo_ccd_qhy", indigo_ccd_qhy },
	{ "SSAG/QHY5 CCD", "indigo_ccd_ssag", indigo_ccd_ssag },
	{ "Shoestring FCUSB Focuser", "indigo_focuser_fcusb", indigo_focuser_fcusb },
	{ "ASI Filter wheel", "indigo_wheel_asi", indigo_wheel_asi },
	{ NULL, NULL, NULL }
};
#else

typedef struct {
	char name[INDIGO_NAME_SIZE];
	char drv_name[INDIGO_NAME_SIZE];
	indigo_result (*driver)(bool state);
	void *dl_handle;
} drivers;

static drivers dynamic_drivers[MAX_DRIVERS];
static int used_slots=0;

#endif

static int first_driver = 2;
static indigo_property *driver_property;


static void server_callback(int count) {
	INDIGO_LOG(indigo_log("%d clients", count));
}


static indigo_result add_driver(const char *name) {
#ifdef STATIC_DRIVERS
	INDIGO_LOG(indigo_log("Can not load '%s'. Drivers are statcally linked!", name));
	return INDIGO_OK;
#else
	char driver_name[INDIGO_NAME_SIZE];
	char *entry_point_name, *cp;
	void *dl_handle;
	int empty_slot;
	indigo_result (*driver)(bool);

	strncpy(driver_name, name, sizeof(driver_name));
	entry_point_name = basename(driver_name);
	cp = strchr(entry_point_name, '.');
	if (cp) *cp = '\0';

	empty_slot = used_slots; /* the first slot after the last used is a good candidate */
	int dc = used_slots-1;
	while (dc >= 0) {
		if (!strncmp(dynamic_drivers[dc].drv_name, entry_point_name, INDIGO_NAME_SIZE)) {
			INDIGO_LOG(indigo_log("Driver '%s' already loaded.", entry_point_name));
			return INDIGO_FAILED;
		} else if (dynamic_drivers[dc].driver == NULL) {
			empty_slot = dc; /* if there is a gap - fill it */
		}
		dc--;
	}

	if (empty_slot > MAX_DRIVERS) return INDIGO_TOO_MANY_ELEMENTS; /* no emty slot found, list is full */

	dl_handle = dlopen(name, RTLD_LAZY);
	if (!dl_handle) {
		INDIGO_LOG(indigo_log("Driver %s can not be loaded.", entry_point_name));
		return INDIGO_FAILED;
	}

	driver = dlsym(dl_handle, entry_point_name);
	const char* dlsym_error = dlerror();
	if (dlsym_error) {
		INDIGO_LOG(indigo_log("Cannot load %s(): %s", entry_point_name, dlsym_error));
		dlclose(dl_handle);
		return INDIGO_NOT_FOUND;
	}

	strncpy(dynamic_drivers[empty_slot].name, entry_point_name, INDIGO_NAME_SIZE); //TO BE CHANGED - DRIVER SHOULD REPORT NAME!!!
	strncpy(dynamic_drivers[empty_slot].drv_name, entry_point_name, INDIGO_NAME_SIZE);
	dynamic_drivers[empty_slot].driver = driver;
	dynamic_drivers[empty_slot].dl_handle = dl_handle;
	//INDIGO_LOG(indigo_log("load entry point %d %p", empty_slot, dynamic_drivers[empty_slot].driver));
	//INDIGO_LOG(indigo_log("dlopen %d %p", empty_slot, dynamic_drivers[empty_slot].dl_handle));

	if (empty_slot == used_slots) used_slots++; /* if we are not filling a gap - increase used_slots */

	INDIGO_LOG(indigo_log("Driver %s loaded.", entry_point_name));
	return INDIGO_OK;
#endif
}


static indigo_result remove_driver(const char *entry_point_name) {
#ifdef STATIC_DRIVERS
	INDIGO_LOG(indigo_log("Can no remove '%s'. Drivers are statcally linked!", entry_point_name));
	return INDIGO_OK;
#else
	if (entry_point_name[0] == '\0') return INDIGO_OK;

	int dc = used_slots-1;
	while (dc >= 0) {
		if (!strncmp(dynamic_drivers[dc].drv_name, entry_point_name, INDIGO_NAME_SIZE)) {

			if (dynamic_drivers[dc].driver) {
				//INDIGO_LOG(indigo_log("unload entry point %d %p", dc, dynamic_drivers[dc].driver));
				dynamic_drivers[dc].driver(false); /* deregister */
			}
			if (dynamic_drivers[dc].dl_handle) {
				//INDIGO_LOG(indigo_log("dlclose %d %p,", dc, dynamic_drivers[dc].dl_handle));
				dlclose(dynamic_drivers[dc].dl_handle);
			}
			INDIGO_LOG(indigo_log("Driver %s removed.", entry_point_name));
			dynamic_drivers[dc].name[0] = '\0';
			dynamic_drivers[dc].drv_name[0] = '\0';
			dynamic_drivers[dc].driver = NULL;
			dynamic_drivers[dc].dl_handle = NULL;
			return INDIGO_OK;
		}
		dc--;
	}
	INDIGO_LOG(indigo_log("Driver %s not found. Is it loaded?", entry_point_name));
	return INDIGO_NOT_FOUND;
#endif
}


static indigo_result change_property(indigo_device *device, indigo_client *client, indigo_property *property);


static indigo_result attach(indigo_device *device) {
	assert(device != NULL);
	driver_property = indigo_init_switch_property(NULL, "INDIGO Server", "DRIVERS", "Main", "Active drivers", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, MAX_DRIVERS);
	driver_property->count = 0;

#ifdef STATIC_DRIVERS
	for (int i = first_driver; i < MAX_DRIVERS && static_drivers[i].name; i++) {
		indigo_init_switch_item(&driver_property->items[driver_property->count++], static_drivers[i].name, static_drivers[i].name, true);
	}
#else
	int i = 0;
	while (i < used_slots) {
		indigo_init_switch_item(&driver_property->items[driver_property->count++], dynamic_drivers[i].name, dynamic_drivers[i].name, true);
		i++;
	}
#endif
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
	#ifdef STATIC_DRIVERS
		for (int i = 0; i < driver_property->count; i++)
			static_drivers[i + first_driver].driver(driver_property->items[i].sw.value);
	#else
		int i = 0;
		while (i < used_slots) {
			dynamic_drivers[i].driver(driver_property->items[i].sw.value);
			i++;
		}
	#endif
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


void signal_handler(int signo) {
	INDIGO_LOG(indigo_log("Signal %d received. Shutting down!", signo));
#ifndef STATIC_DRIVERS
	int dc = used_slots-1;
	while (dc >= 0) {
		remove_driver(dynamic_drivers[dc].drv_name);
		dc--;
	}
#endif
	INDIGO_LOG(indigo_log("Shutdown complete! See you!"));
	exit(0);
}


int main(int argc, const char * argv[]) {
	indigo_main_argc = argc;
	indigo_main_argv = argv;

	indigo_log("INDIGO server %d.%d-%d built on %s", (INDIGO_VERSION >> 8) & 0xFF, INDIGO_VERSION & 0xFF, INDIGO_BUILD, __TIMESTAMP__);
	
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-s") || !strcmp(argv[i], "--enable-simulators"))
			first_driver = 0;
		else if ((!strcmp(argv[i], "-p") || !strcmp(argv[i], "--port")) && i < argc - 1)
			indigo_server_xml_port = atoi(argv[i+1]);
		else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
			printf("\n%s [-s|--enable-simulators] [-p|--port port] [-h|--help]\n\n", argv[0]);
			exit(0);
		}
		else if(argv[i][0] != '-') {
			add_driver(argv[i]);
		}
	}

	indigo_start_usb_event_handler();
	
	signal(SIGINT, signal_handler);

	static indigo_device device = {
		SERVER_NAME, NULL, INDIGO_OK, INDIGO_VERSION,
		attach,
		enumerate_properties,
		change_property,
		detach
	};

	if (strstr(argv[0], "MacOS"))
		indigo_use_syslog = true; // embeded into INDIGO Server for macOS
	
	indigo_start();
	indigo_attach_device(&device);

	indigo_server_xml(server_callback);
	return 0;
}

