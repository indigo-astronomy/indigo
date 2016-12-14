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
#include <signal.h>
#include <dns_sd.h>
#include <arpa/inet.h>

#include "indigo_bus.h"
#include "indigo_server_tcp.h"
#include "indigo_driver.h"
#include "indigo_client.h"

#include "ccd_simulator/indigo_ccd_simulator.h"
#include "mount_simulator/indigo_mount_simulator.h"
#ifdef STATIC_DRIVERS
#include "ccd_sx/indigo_ccd_sx.h"
#include "wheel_sx/indigo_wheel_sx.h"
#include "ccd_ssag/indigo_ccd_ssag.h"
#include "ccd_asi/indigo_ccd_asi.h"
#include "wheel_asi/indigo_wheel_asi.h"
#include "ccd_atik/indigo_ccd_atik.h"
#include "wheel_atik/indigo_wheel_atik.h"
#include "ccd_qhy/indigo_ccd_qhy.h"
#include "focuser_fcusb/indigo_focuser_fcusb.h"
#include "ccd_iidc/indigo_ccd_iidc.h"
#include "mount_lx200/indigo_mount_lx200.h"
#include "mount_nexstar/indigo_mount_nexstar.h"
#endif

#define MDNS_INDIGO_TYPE    "_indigo._tcp"
#define MDNS_HTTP_TYPE      "_http._tcp"
#define SERVER_NAME         "INDIGO Server"

driver_entry_point static_drivers[] = {
	indigo_ccd_simulator,
	indigo_mount_simulator,
#ifdef STATIC_DRIVERS
	indigo_ccd_sx,
	indigo_wheel_sx,
	indigo_ccd_atik,
	indigo_wheel_atik,
	indigo_ccd_qhy,
	indigo_ccd_ssag,
	indigo_focuser_fcusb,
	indigo_ccd_asi,
	indigo_wheel_asi,
	indigo_ccd_iidc,
	indigo_mount_lx200,
	indigo_mount_nexstar,
#endif
	NULL
};

static int first_driver = 2;
static indigo_property *driver_property;
static DNSServiceRef sd_http;
static DNSServiceRef sd_indigo;

static unsigned char ctrl[] = {
#include "ctrl.data"
};

static void server_callback(int count) {
	INDIGO_LOG(indigo_log("%d clients", count));
}

static indigo_result change_property(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result attach(indigo_device *device) {
	assert(device != NULL);
	driver_property = indigo_init_switch_property(NULL, "INDIGO Server", "DRIVERS", "Main", "Active drivers", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, INDIGO_MAX_DRIVERS);
	driver_property->count = 0;
	for (int i = 0; i < INDIGO_MAX_DRIVERS; i++)
		if (indigo_available_drivers[i].driver != NULL)
			indigo_init_switch_item(&driver_property->items[driver_property->count++], indigo_available_drivers[i].description, indigo_available_drivers[i].description, true);
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
			if (driver_property->items[i].sw.value)
				indigo_available_drivers[i].driver(INDIGO_DRIVER_INIT, NULL);
			else
				indigo_available_drivers[i].driver(INDIGO_DRIVER_SHUTDOWN, NULL);
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
	indigo_delete_property(device, driver_property, NULL);
	INDIGO_LOG(indigo_log("%s detached", device->name));
	return INDIGO_OK;
}

static indigo_device server_device = {
	SERVER_NAME, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
	attach,
	enumerate_properties,
	change_property,
	detach
};

void signal_handler(int signo) {
	INDIGO_LOG(indigo_log("Signal %d received. Shutting down!", signo));

	DNSServiceRefDeallocate(sd_indigo);
	DNSServiceRefDeallocate(sd_http);

	for (int i = 0; i < INDIGO_MAX_DRIVERS; i++) {
		if (indigo_available_drivers[i].driver) {
			if (indigo_available_drivers[i].dl_handle != NULL)
				indigo_unload_driver(indigo_available_drivers[i].name);
			else
				indigo_remove_driver(indigo_available_drivers[i].driver);
		}
	}
	for (int i = 0; i < INDIGO_MAX_SERVERS; i++) {
		if (indigo_available_servers[i].thread_started)
			indigo_disconnect_server(indigo_available_servers[i].host, indigo_available_servers[i].port);
	}
	indigo_detach_device(&server_device);
	INDIGO_LOG(indigo_log("Shutdown complete! See you!"));
	exit(0);
}

int main(int argc, const char * argv[]) {
	indigo_main_argc = argc;
	indigo_main_argv = argv;

	indigo_log("INDIGO server %d.%d-%d built on %s", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF, INDIGO_BUILD, __TIMESTAMP__);

	for (int i = 1; i < argc; i++) {
		if ((!strcmp(argv[i], "-p") || !strcmp(argv[i], "--port")) && i < argc - 1) {
			indigo_server_tcp_port = atoi(argv[i + 1]);
			i++;
		} else if (!strcmp(argv[i], "-s") || !strcmp(argv[i], "--enable-simulators")) {
			first_driver = 0;
		} else if ((!strcmp(argv[i], "-r") || !strcmp(argv[i], "--remote-server")) && i < argc - 1) {
			char host[INDIGO_NAME_SIZE];
			strncpy(host, argv[i + 1], INDIGO_NAME_SIZE);
			char *colon = strchr(host, ':');
			int port = 7624;
			if (colon != NULL) {
				*colon++ = 0;
				port = atoi(colon);
			}
			indigo_connect_server(host, port);
			i++;
		} else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
			printf("\n%s [-s|--enable-simulators] [-p|--port port] [-h|--help] driver_name driver_name ...\n\n", argv[0]);
			exit(0);
		} else if(argv[i][0] != '-') {
			indigo_load_driver(argv[i], false);
		}
	}
	
	indigo_server_add_resource("/ctrl", ctrl, sizeof(ctrl), "text/html");

	/* UGLY but the only way to suppress compat mode warning messages on Linux */
	setenv("AVAHI_COMPAT_NOWARN", "1", 1);
	DNSServiceRegister(&sd_http, 0, 0, NULL, MDNS_HTTP_TYPE, NULL, NULL, htons(indigo_server_tcp_port), 0, NULL, NULL, NULL);
	DNSServiceRegister(&sd_indigo, 0, 0, NULL, MDNS_INDIGO_TYPE, NULL, NULL, htons(indigo_server_tcp_port), 0, NULL, NULL, NULL);

	for (int i = first_driver; static_drivers[i]; i++) {
		indigo_add_driver(static_drivers[i], false);
	}

	indigo_start_usb_event_handler();

	signal(SIGINT, signal_handler);

	if (strstr(argv[0], "MacOS"))
		indigo_use_syslog = true; // embeded into INDIGO Server for macOS

	indigo_start();
	indigo_attach_device(&server_device);

	indigo_server_tcp(server_callback);
	return 0;
}

