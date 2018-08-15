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
#include <syslog.h>
#include <assert.h>
#include <signal.h>
#include <dns_sd.h>
#include <libgen.h>
#include <pthread.h>

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#ifdef INDIGO_LINUX
#include <sys/prctl.h>
#endif
#ifdef INDIGO_MACOS
#include <CoreFoundation/CoreFoundation.h>
#endif

#include "indigo_bus.h"
#include "indigo_server_tcp.h"
#include "indigo_driver.h"
#include "indigo_client.h"
#include "indigo_xml.h"

#include "ccd_simulator/indigo_ccd_simulator.h"
#include "mount_simulator/indigo_mount_simulator.h"
#include "gps_simulator/indigo_gps_simulator.h"
#include "dome_simulator/indigo_dome_simulator.h"
#ifdef STATIC_DRIVERS
#include "ccd_sx/indigo_ccd_sx.h"
#include "wheel_sx/indigo_wheel_sx.h"
#include "ccd_ssag/indigo_ccd_ssag.h"
#include "ccd_asi/indigo_ccd_asi.h"
#include "guider_asi/indigo_guider_asi.h"
#include "wheel_asi/indigo_wheel_asi.h"
#include "ccd_atik/indigo_ccd_atik.h"
#include "wheel_atik/indigo_wheel_atik.h"
#include "ccd_qhy/indigo_ccd_qhy.h"
#include "focuser_fcusb/indigo_focuser_fcusb.h"
#include "ccd_iidc/indigo_ccd_iidc.h"
#include "mount_lx200/indigo_mount_lx200.h"
#include "mount_nexstar/indigo_mount_nexstar.h"
#include "mount_temma/indigo_mount_temma.h"
#include "ccd_fli/indigo_ccd_fli.h"
#include "wheel_fli/indigo_wheel_fli.h"
#include "focuser_fli/indigo_focuser_fli.h"
#include "ccd_apogee/indigo_ccd_apogee.h"
#include "focuser_usbv3/indigo_focuser_usbv3.h"
#include "focuser_wemacro/indigo_focuser_wemacro.h"
#include "ccd_mi/indigo_ccd_mi.h"
#include "aux_joystick/indigo_aux_joystick.h"
#include "mount_synscan/indigo_mount_synscan.h"
#ifndef __aarch64__
#include "ccd_sbig/indigo_ccd_sbig.h"
#endif
#include "ccd_dsi/indigo_ccd_dsi.h"
#include "ccd_qsi/indigo_ccd_qsi.h"
#include "gps_nmea/indigo_gps_nmea.h"
#ifdef INDIGO_MACOS
#include "ccd_ica/indigo_ccd_ica.h"
#include "guider_eqmac/indigo_guider_eqmac.h"
#include "focuser_wemacro_bt/indigo_focuser_wemacro_bt.h"
#endif
#ifdef INDIGO_LINUX
#include "ccd_gphoto2/indigo_ccd_gphoto2.h"
#endif
#include "agent_snoop/indigo_agent_snoop.h"
#include "agent_lx200_server/indigo_agent_lx200_server.h"
#endif

#define MDNS_INDIGO_TYPE    "_indigo._tcp"
#define MDNS_HTTP_TYPE      "_http._tcp"
#define SERVER_NAME         "INDIGO Server"

driver_entry_point static_drivers[] = {
	indigo_ccd_simulator,
	indigo_mount_simulator,
	indigo_gps_simulator,
	indigo_dome_simulator,
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
	indigo_guider_asi,
	indigo_ccd_iidc,
	indigo_mount_lx200,
	indigo_mount_nexstar,
	indigo_mount_temma,
	indigo_ccd_fli,
	indigo_wheel_fli,
	indigo_focuser_fli,
	indigo_ccd_apogee,
	indigo_focuser_usbv3,
	indigo_focuser_wemacro,
	indigo_ccd_mi,
	indigo_aux_joystick,
	indigo_mount_synscan,
#ifndef __aarch64__
	indigo_ccd_sbig,
#endif
	indigo_ccd_dsi,
/* Removed temporary as it is not stable and hangs servers on shutdown */
/*	indigo_ccd_qsi,  */
	indigo_gps_nmea,
#ifdef INDIGO_MACOS
	indigo_ccd_ica,
	indigo_guider_eqmac,
	indigo_focuser_wemacro_bt,
#endif
#ifdef INDIGO_LINUX
	indigo_ccd_gphoto2,
#endif
	indigo_agent_snoop,
	indigo_agent_lx200_server,
#endif
	NULL
};

static int first_driver = 4; /* This should be equial to number of simulator drivers */
static indigo_property *drivers_property;
static indigo_property *servers_property;
static indigo_property *load_property;
static indigo_property *unload_property;
static indigo_property *restart_property;
static indigo_property *log_level_property;
static DNSServiceRef sd_http;
static DNSServiceRef sd_indigo;

#ifdef INDIGO_MACOS
static bool runLoop = true;
#endif

#define LOG_LEVEL_ERROR_ITEM        (log_level_property->items + 0)
#define LOG_LEVEL_INFO_ITEM         (log_level_property->items + 1)
#define LOG_LEVEL_DEBUG_ITEM        (log_level_property->items + 2)
#define LOG_LEVEL_TRACE_ITEM        (log_level_property->items + 3)

static pid_t server_pid = 0;
static bool keep_server_running = true;
static bool use_sigkill = false;
static bool server_startup = true;
static bool use_bonjour = true;
static bool use_control_panel = true;

static char const *server_argv[128];
static int server_argc = 1;

static indigo_result attach(indigo_device *device);
static indigo_result enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);
static indigo_result change_property(indigo_device *device, indigo_client *client, indigo_property *property);
static indigo_result detach(indigo_device *device);

static indigo_device server_device = INDIGO_DEVICE_INITIALIZER(
	"Server",
	attach,
	enumerate_properties,
	change_property,
	NULL,
	detach
);

static unsigned char ctrl[] = {
#include "ctrl.data"
};

static unsigned char angular_js[] = {
#include "resource/angular.min.js.data"
};

static unsigned char bootstrap_js[] = {
#include"resource/bootstrap.min.js.data"
};

static unsigned char bootstrap_css[] = {
#include "resource/bootstrap.css.data"
};

static unsigned char jquery_js[] = {
#include "resource/jquery.min.js.data"
};

static unsigned char font_ttf[] = {
#include "resource/glyphicons-halflings-regular.ttf.data"
};

static unsigned char logo_png[] = {
#include "resource/logo.png.data"
};

static void server_callback(int count) {
	if (server_startup) {
		if (use_bonjour) {
			/* UGLY but the only way to suppress compat mode warning messages on Linux */
			setenv("AVAHI_COMPAT_NOWARN", "1", 1);
			if (*indigo_local_service_name == 0) {
				char hostname[INDIGO_NAME_SIZE];
				gethostname(hostname, sizeof(hostname));
				indigo_service_name(hostname, indigo_server_tcp_port, indigo_local_service_name);
			}
			DNSServiceRegister(&sd_http, 0, 0, indigo_local_service_name, MDNS_HTTP_TYPE, NULL, NULL, htons(indigo_server_tcp_port), 0, NULL, NULL, NULL);
			DNSServiceRegister(&sd_indigo, 0, 0, indigo_local_service_name, MDNS_INDIGO_TYPE, NULL, NULL, htons(indigo_server_tcp_port), 0, NULL, NULL, NULL);
		}
		server_startup = false;
	} else {
		INDIGO_LOG(indigo_log("%d clients", count));
	}
}

static indigo_result attach(indigo_device *device) {
	assert(device != NULL);
	drivers_property = indigo_init_switch_property(NULL, server_device.name, "DRIVERS", "Main", "Active drivers", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, INDIGO_MAX_DRIVERS);
	drivers_property->count = 0;
	for (int i = 0; i < INDIGO_MAX_DRIVERS; i++)
		if (indigo_available_drivers[i].driver != NULL)
			indigo_init_switch_item(&drivers_property->items[drivers_property->count++], indigo_available_drivers[i].description, indigo_available_drivers[i].description, true);
	servers_property = indigo_init_light_property(NULL, server_device.name, "SERVERS", "Main", "Active servers", INDIGO_IDLE_STATE, 2 * INDIGO_MAX_SERVERS);
	servers_property->count = 0;
	for (int i = 0; i < INDIGO_MAX_SERVERS; i++) {
		indigo_server_entry *entry = indigo_available_servers + i;
		if (*entry->host) {
			char buf[INDIGO_NAME_SIZE];
			if (entry->port == 7624)
				strncpy(buf, entry->host, sizeof(buf));
			else
				snprintf(buf, sizeof(buf), "%s:%d", entry->host, entry->port);
			indigo_init_light_item(&servers_property->items[servers_property->count++], buf, buf, INDIGO_IDLE_STATE);
		}
	}
	for (int i = 0; i < INDIGO_MAX_SERVERS; i++) {
		indigo_subprocess_entry *entry = indigo_available_subprocesses + i;
		if (*entry->executable) {
			indigo_init_light_item(&servers_property->items[servers_property->count++], entry->executable, entry->executable, INDIGO_IDLE_STATE);
		}
	}
	load_property = indigo_init_text_property(NULL, server_device.name, "LOAD", "Main", "Load driver", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 1);
	indigo_init_text_item(&load_property->items[0], "DRIVER", "Load driver", "");
	unload_property = indigo_init_text_property(NULL, server_device.name, "UNLOAD", "Main", "Unload driver", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 1);
	indigo_init_text_item(&unload_property->items[0], "DRIVER", "Unload driver", "");
	restart_property = indigo_init_switch_property(NULL, server_device.name, "RESTART", "Main", "Restart", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
	indigo_init_switch_item(restart_property->items, "RESTART", "Restart server", false);
	log_level_property = indigo_init_switch_property(NULL, device->name, "LOG_LEVEL", MAIN_GROUP, "Log level", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 4);
	indigo_init_switch_item(&log_level_property->items[0], "ERROR", "Error", false);
	indigo_init_switch_item(&log_level_property->items[1], "INFO", "Info", false);
	indigo_init_switch_item(&log_level_property->items[2], "DEBUG", "Debug", false);
	indigo_init_switch_item(&log_level_property->items[3], "TRACE", "Trace", false);

	indigo_log_levels log_level = indigo_get_log_level();
	switch (log_level) {
		case INDIGO_LOG_ERROR:
			LOG_LEVEL_ERROR_ITEM->sw.value = true;
			break;
		case INDIGO_LOG_INFO:
			LOG_LEVEL_INFO_ITEM->sw.value = true;
			break;
		case INDIGO_LOG_DEBUG:
			LOG_LEVEL_DEBUG_ITEM->sw.value = true;
			break;
		case INDIGO_LOG_TRACE:
			LOG_LEVEL_TRACE_ITEM->sw.value = true;
			break;
	}
	if (indigo_load_properties(device, false) == INDIGO_FAILED)
		change_property(device, NULL, drivers_property);
	INDIGO_LOG(indigo_log("%s attached", device->name));
	return INDIGO_OK;
}

static indigo_result enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	indigo_define_property(device, drivers_property, NULL);
	if (servers_property->count > 0)
		indigo_define_property(device, servers_property, NULL);
	indigo_define_property(device, load_property, NULL);
	indigo_define_property(device, unload_property, NULL);
	indigo_define_property(device, restart_property, NULL);
	indigo_define_property(device, log_level_property, NULL);
	return INDIGO_OK;
}

static indigo_result change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(property != NULL);
	if (indigo_property_match(drivers_property, property)) {
	// -------------------------------------------------------------------------------- DRIVERS
		indigo_property_copy_values(drivers_property, property, false);
		for (int i = 0; i < drivers_property->count; i++)
			if (drivers_property->items[i].sw.value) {
				indigo_available_drivers[i].initialized = indigo_available_drivers[i].driver(INDIGO_DRIVER_INIT, NULL) == INDIGO_OK;
				if (!indigo_available_drivers[i].initialized)
					drivers_property->items[i].sw.value = false;
			} else if (indigo_available_drivers[i].initialized) {
				indigo_available_drivers[i].driver(INDIGO_DRIVER_SHUTDOWN, NULL);
				indigo_available_drivers[i].initialized = false;
			}
		drivers_property->state = INDIGO_OK_STATE;
		indigo_update_property(device, drivers_property, NULL);
		int handle = 0;
		indigo_save_property(device, &handle, drivers_property);
		close(handle);
	} else if (indigo_property_match(load_property, property)) {
		// -------------------------------------------------------------------------------- LOAD
		indigo_property_copy_values(load_property, property, false);
		if (*load_property->items[0].text.value) {
			if (indigo_load_driver(load_property->items[0].text.value, true, NULL) == INDIGO_OK) {
				indigo_delete_property(device, drivers_property, NULL);
				drivers_property->count = 0;
				for (int i = 0; i < INDIGO_MAX_DRIVERS; i++)
					if (indigo_available_drivers[i].driver != NULL)
						indigo_init_switch_item(&drivers_property->items[drivers_property->count++], indigo_available_drivers[i].description, indigo_available_drivers[i].description, indigo_available_drivers[i].initialized);
				indigo_define_property(device, drivers_property, NULL);
				load_property->state = INDIGO_OK_STATE;
				char *name = basename(load_property->items[0].text.value);
				for (int i = 0; i < INDIGO_MAX_DRIVERS; i++)
					if (indigo_available_drivers[i].driver != NULL && !strcmp(name, indigo_available_drivers[i].name)) {
						indigo_update_property(device, load_property, "Driver %s (%s) loaded", name, indigo_available_drivers[i].description);
					}
			} else {
				load_property->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, load_property, indigo_last_message);
			}
		}
	} else if (indigo_property_match(unload_property, property)) {
		// -------------------------------------------------------------------------------- UNLOAD
		indigo_property_copy_values(unload_property, property, false);
		if (*unload_property->items[0].text.value) {
			indigo_driver_entry *driver = NULL;
			for (int i = 0; i < INDIGO_MAX_DRIVERS; i++) {
				if (!strcmp(indigo_available_drivers[i].name, unload_property->items[0].text.value)) {
					driver = &indigo_available_drivers[i];
					break;
				}
			}
			if (driver != NULL && indigo_remove_driver(driver) == INDIGO_OK) {
				indigo_delete_property(device, drivers_property, NULL);
				drivers_property->count = 0;
				for (int i = 0; i < INDIGO_MAX_DRIVERS; i++)
					if (indigo_available_drivers[i].driver != NULL)
						indigo_init_switch_item(&drivers_property->items[drivers_property->count++], indigo_available_drivers[i].description, indigo_available_drivers[i].description, indigo_available_drivers[i].initialized);
				indigo_define_property(device, drivers_property, NULL);
				load_property->state = INDIGO_OK_STATE;
				indigo_update_property(device, unload_property, "Driver %s unloaded", unload_property->items[0].text.value);
			} else {
				load_property->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, unload_property, indigo_last_message);
			}
		}
	} else if (indigo_property_match(restart_property, property)) {
	// -------------------------------------------------------------------------------- RESTART
		indigo_property_copy_values(restart_property, property, false);
		if (restart_property->items[0].sw.value) {
			INDIGO_LOG(indigo_log("Restarting..."));
			indigo_server_shutdown();
		}
	} else if (indigo_property_match(log_level_property, property)) {
		// -------------------------------------------------------------------------------- LOG_LEVEL
		indigo_property_copy_values(log_level_property, property, false);
		if (log_level_property->items[0].sw.value) {
			indigo_set_log_level(INDIGO_LOG_ERROR);
		} else if (log_level_property->items[1].sw.value) {
			indigo_set_log_level(INDIGO_LOG_INFO);
		} else if (log_level_property->items[2].sw.value) {
			indigo_set_log_level(INDIGO_LOG_DEBUG);
		} else if (log_level_property->items[3].sw.value) {
			indigo_set_log_level(INDIGO_LOG_TRACE);
		}
		log_level_property->state = INDIGO_OK_STATE;
		indigo_update_property(device, log_level_property, NULL);
	// --------------------------------------------------------------------------------
	}
	return INDIGO_OK;
}

static indigo_result detach(indigo_device *device) {
	assert(device != NULL);
	indigo_delete_property(device, drivers_property, NULL);
	if (servers_property->count > 0)
		indigo_delete_property(device, servers_property, NULL);
	indigo_delete_property(device, load_property, NULL);
	indigo_delete_property(device, unload_property, NULL);
	indigo_delete_property(device, log_level_property, NULL);
	INDIGO_LOG(indigo_log("%s detached", device->name));
	return INDIGO_OK;
}

static void server_main() {
	indigo_log("INDIGO server %d.%d-%d built on %s", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF, INDIGO_BUILD, __TIMESTAMP__);

	indigo_start_usb_event_handler();

	indigo_start();

	for (int i = 1; i < server_argc; i++) {
		if ((!strcmp(server_argv[i], "-p") || !strcmp(server_argv[i], "--port")) && i < server_argc - 1) {
			indigo_server_tcp_port = atoi(server_argv[i + 1]);
			i++;
		} else if (!strcmp(server_argv[i], "-s") || !strcmp(server_argv[i], "--enable-simulators")) {
			first_driver = 0;
		} else if ((!strcmp(server_argv[i], "-r") || !strcmp(server_argv[i], "--remote-server")) && i < server_argc - 1) {
			char host[INDIGO_NAME_SIZE];
			strncpy(host, server_argv[i + 1], INDIGO_NAME_SIZE);
			char *colon = strchr(host, ':');
			int port = 7624;
			if (colon != NULL) {
				*colon++ = 0;
				port = atoi(colon);
			}
			indigo_reshare_remote_devices = true;
			indigo_connect_server(NULL, host, port, NULL);
			i++;
		} else if ((!strcmp(server_argv[i], "-i") || !strcmp(server_argv[i], "--indi-driver")) && i < server_argc - 1) {
			char executable[INDIGO_NAME_SIZE];
			strncpy(executable, server_argv[i + 1], INDIGO_NAME_SIZE);
			indigo_reshare_remote_devices = true;
			indigo_start_subprocess(executable, NULL);
			i++;
		} else if (!strcmp(server_argv[i], "-b-") || !strcmp(server_argv[i], "--disable-bonjour")) {
			use_bonjour = false;
		} else if (!strcmp(server_argv[i], "-b") || !strcmp(server_argv[i], "--bonjour")) {
			strncpy(indigo_local_service_name, server_argv[i + 1], INDIGO_NAME_SIZE);
			i++;
		} else if (!strcmp(server_argv[i], "-c-") || !strcmp(server_argv[i], "--disable-control-panel")) {
			use_control_panel = false;
		} else if (!strcmp(server_argv[i], "-u-") || !strcmp(server_argv[i], "--disable-blob-urls")) {
			indigo_use_blob_urls = false;
		} else if(server_argv[i][0] != '-') {
			indigo_load_driver(server_argv[i], false, NULL);
		}
	}

	if (use_control_panel) {
		indigo_server_add_resource("/ctrl", ctrl, sizeof(ctrl), "text/html");
		indigo_server_add_resource("/resource/angular.min.js", angular_js, sizeof(angular_js), "text/javascript");
		indigo_server_add_resource("/resource/bootstrap.min.js", bootstrap_js, sizeof(bootstrap_js), "text/javascript");
		indigo_server_add_resource("/resource/bootstrap.css", bootstrap_css, sizeof(bootstrap_css), "text/css");
		indigo_server_add_resource("/resource/jquery.min.js", jquery_js, sizeof(jquery_js), "text/javascript");
		indigo_server_add_resource("/fonts/glyphicons-halflings-regular.ttf", font_ttf, sizeof(font_ttf), "application/x-font-ttf");
		indigo_server_add_resource("/resource/logo.png", logo_png, sizeof(logo_png), "image/png");
	}

	for (int i = first_driver; static_drivers[i]; i++) {
		indigo_add_driver(static_drivers[i], false, NULL);
	}

	indigo_attach_device(&server_device);
	
#ifdef INDIGO_LINUX
	indigo_server_start(server_callback);
#endif
#ifdef INDIGO_MACOS
	pthread_t server_thread;
	if (pthread_create(&server_thread, NULL, (void * (*)(void *))indigo_server_start, server_callback)) {
		INDIGO_ERROR(indigo_error("Error creating thread for server"));
	}
	runLoop = true;
	while (runLoop) {
		CFRunLoopRunResult result = CFRunLoopRunInMode(kCFRunLoopDefaultMode, 1, true);
	}
#endif

#ifdef INDIGO_MACOS
	DNSServiceRefDeallocate(sd_indigo);
	DNSServiceRefDeallocate(sd_http);
#endif

	indigo_detach_device(&server_device);
	indigo_stop();
	for (int i = 0; i < INDIGO_MAX_DRIVERS; i++) {
		if (indigo_available_drivers[i].driver) {
			indigo_remove_driver(&indigo_available_drivers[i]);
		}
	}
	for (int i = 0; i < INDIGO_MAX_SERVERS; i++) {
		if (indigo_available_servers[i].thread_started)
			indigo_disconnect_server(&indigo_available_servers[i]);
	}
	for (int i = 0; i < INDIGO_MAX_SERVERS; i++) {
		if (indigo_available_subprocesses[i].thread_started)
			indigo_kill_subprocess(&indigo_available_subprocesses[i]);
	}
	exit(EXIT_SUCCESS);
}

static void signal_handler(int signo) {
	if (server_pid == 0) {
		/* SIGINT is delivered twise with CTRL-C
		   this leads to freeze during shutdown so
		   we ignore the second SIGINT */
		if (signo == SIGINT) {
			signal(SIGINT, SIG_IGN);
		}
		INDIGO_LOG(indigo_log("Shutdown initiated (signal %d)...", signo));
		indigo_server_shutdown();
#ifdef INDIGO_MACOS
		runLoop = false;
#endif
	} else {
		INDIGO_LOG(indigo_log("Signal %d received...", signo));
		keep_server_running = (signo == SIGHUP);
		if (use_sigkill)
			kill(server_pid, SIGKILL);
		else
			kill(server_pid, SIGINT);

		use_sigkill = true;
	}
}

int main(int argc, const char * argv[]) {
	bool do_fork = true;
	server_argv[0] = argv[0];
	indigo_main_argc = argc;
	indigo_main_argv = argv;
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--") || !strcmp(argv[i], "--do-not-fork")) {
			do_fork = false;
		} else if (!strcmp(argv[i], "-l") || !strcmp(argv[i], "--use-syslog")) {
			indigo_use_syslog = true;
		} else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
			printf("%s [-h|--help]\n", argv[0]);
			printf("%s [--|--do-not-fork] [-l|--use-syslog] [-s|--enable-simulators] [-p|--port port] [-u-|--disable-blob-urls] [-b|--bonjour name] [-b-|--disable-bonjour] [-c-|--disable-control-panel] [-v|--enable-info] [-vv|--enable-debug] [-vvv|--enable-trace] [-r|--remote-server host:port] [-i|--indi-driver driver_executable] indigo_driver_name indigo_driver_name ...\n", argv[0]);
			return 0;
		} else {
			server_argv[server_argc++] = argv[i];
		}
	}
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGHUP, signal_handler);
	if (do_fork) {
		while(keep_server_running) {
			server_pid = fork();
			if (server_pid == -1) {
				INDIGO_ERROR(indigo_error("Server start failed!"));
				return EXIT_FAILURE;
			} else if (server_pid == 0) {
#ifdef INDIGO_LINUX
				/* Preserve process name for logging */
				char *name = strrchr(server_argv[0], '/');
				if (name != NULL) {
					name++;
				} else {
					name = (char *)server_argv[0];
				}
				strncpy(indigo_log_name, name, 255);

				/* Change process name for user convinience */
				static char process_name[]="indigo_worker";
				int len = strlen(server_argv[0]);
				strncpy((char*)server_argv[0], process_name, len);
				prctl(PR_SET_PDEATHSIG, SIGINT, 0, 0, 0);
				/* Linux requires additional step to change process name */
				prctl(PR_SET_NAME, process_name, 0, 0, 0);
#endif
				server_main();
				return EXIT_SUCCESS;
			} else {
				if (waitpid(server_pid, NULL, 0) == -1 ) {
					INDIGO_ERROR(indigo_error("waitpid() failed."));
					return EXIT_FAILURE;
				}
				use_sigkill = false;
				if (keep_server_running) {
					INDIGO_LOG(indigo_log("Shutdown complete! Starting up..."));
					sleep(2);
				}
			}
		}
		INDIGO_LOG(indigo_log("Shutdown complete! See you!"));
	} else {
		server_main();
	}
}
