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
#include <dirent.h>
#include <errno.h>

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

#include "indigo_cat_data.h"

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
#include "mount_synscan/indigo_mount_synscan.h"
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
#include "mount_ioptron/indigo_mount_ioptron.h"
#include "guider_cgusbst4/indigo_guider_cgusbst4.h"
#include "wheel_quantum/indigo_wheel_quantum.h"
#include "wheel_trutek/indigo_wheel_trutek.h"
#include "wheel_xagyl/indigo_wheel_xagyl.h"
#include "wheel_optec/indigo_wheel_optec.h"
#include "focuser_dmfc/indigo_focuser_dmfc.h"
#include "focuser_nstep/indigo_focuser_nstep.h"
#include "focuser_nfocus/indigo_focuser_nfocus.h"
#include "focuser_moonlite/indigo_focuser_moonlite.h"
#include "focuser_mjkzz/indigo_focuser_mjkzz.h"
#include "ccd_touptek/indigo_ccd_touptek.h"
#include "ccd_altair/indigo_ccd_altair.h"
#include "focuser_optec/indigo_focuser_optec.h"
#include "aux_upb/indigo_aux_upb.h"
#include "aux_rts/indigo_aux_rts.h"
#include "aux_dsusb/indigo_aux_dsusb.h"
#include "guider_gpusb/indigo_guider_gpusb.h"
#include "focuser_lakeside/indigo_focuser_lakeside.h"
#include "agent_imager/indigo_agent_imager.h"
#include "focuser_asi/indigo_focuser_asi.h"
#include "agent_alignment/indigo_agent_alignment.h"
#include "agent_mount/indigo_agent_mount.h"
#include "ao_sx/indigo_ao_sx.h"
#include "ccd_uvc/indigo_ccd_uvc.h"
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
#include "focuser_mjkzz_bt/indigo_focuser_mjkzz_bt.h"
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
#ifdef STATIC_DRIVERS
	indigo_agent_alignment,
	indigo_agent_imager,
	indigo_agent_lx200_server,
	indigo_agent_mount,
	indigo_agent_snoop,
	indigo_ao_sx,
	indigo_aux_dsusb,
	indigo_aux_joystick,
	indigo_aux_rts,
	indigo_aux_upb,
	indigo_ccd_altair,
	indigo_ccd_apogee,
	indigo_ccd_asi,
	indigo_ccd_atik,
	indigo_ccd_dsi,
	indigo_ccd_fli,
#ifdef INDIGO_LINUX
	indigo_ccd_gphoto2,
#endif
#ifdef INDIGO_MACOS
	indigo_ccd_ica,
#endif
	indigo_ccd_iidc,
	indigo_ccd_mi,
	indigo_ccd_qhy,
	indigo_ccd_qsi,
#ifndef __aarch64__
	indigo_ccd_sbig,
#endif
	indigo_ccd_simulator,
	indigo_ccd_ssag,
	indigo_ccd_sx,
	indigo_ccd_touptek,
	indigo_ccd_uvc,
	indigo_dome_simulator,
	indigo_focuser_asi,
	indigo_focuser_dmfc,
	indigo_focuser_fcusb,
	indigo_focuser_fli,
	indigo_focuser_lakeside,
	indigo_focuser_mjkzz,
#ifdef INDIGO_MACOS
	indigo_focuser_mjkzz_bt,
#endif
	indigo_focuser_moonlite,
	indigo_focuser_nfocus,
	indigo_focuser_nstep,
	indigo_focuser_optec,
	indigo_focuser_usbv3,
	indigo_focuser_wemacro,
#ifdef INDIGO_MACOS
	indigo_focuser_wemacro_bt,
#endif
	indigo_gps_simulator,
	indigo_guider_asi,
	indigo_guider_cgusbst4,
#ifdef INDIGO_MACOS
	indigo_guider_eqmac,
#endif
	indigo_guider_gpusb,
	indigo_mount_ioptron,
	indigo_mount_lx200,
	indigo_mount_nexstar,
	indigo_mount_simulator,
	indigo_mount_synscan,
	indigo_mount_synscan,
	indigo_mount_temma,
	indigo_wheel_asi,
	indigo_wheel_atik,
	indigo_wheel_fli,
	indigo_wheel_optec,
	indigo_wheel_quantum,
	indigo_wheel_sx,
	indigo_wheel_trutek,
	indigo_wheel_xagyl,
#endif
	NULL
};

static struct {
	char *name;
	char *description;
} dynamic_drivers[INDIGO_MAX_DRIVERS] = {
	NULL
};

static int static_drivers_count = 0;
static int dynamic_drivers_count = 0;
static bool command_line_drivers = false;

static indigo_property *drivers_property;
static indigo_property *servers_property;
static indigo_property *load_property;
static indigo_property *unload_property;
static indigo_property *restart_property;
static indigo_property *log_level_property;
static indigo_property *server_features_property;

#ifdef RPI_MANAGEMENT
static indigo_property *wifi_ap_property;
static indigo_property *wifi_infrastructure_property;
static indigo_property *host_time_property;
static indigo_property *shutdown_property;
static indigo_property *reboot_property;
static indigo_property *install_property;
#endif

static DNSServiceRef sd_http;
static DNSServiceRef sd_indigo;

static void *star_data = NULL;
static void *dso_data = NULL;
static void *constellation_data = NULL;

#ifdef INDIGO_MACOS
static bool runLoop = true;
#endif

#define RESTART_ITEM                (restart_property->items + 0)

#define LOAD_ITEM                   (load_property->items + 0)
#define UNLOAD_ITEM                 (unload_property->items + 0)

#define LOG_LEVEL_ERROR_ITEM        (log_level_property->items + 0)
#define LOG_LEVEL_INFO_ITEM         (log_level_property->items + 1)
#define LOG_LEVEL_DEBUG_ITEM        (log_level_property->items + 2)
#define LOG_LEVEL_TRACE_ITEM        (log_level_property->items + 3)

#define BONJOUR_ITEM                (server_features_property->items + 0)
#define CTRL_PANEL_ITEM             (server_features_property->items + 1)
#define WEB_APPS_ITEM               (server_features_property->items + 2)

static pid_t server_pid = 0;
static bool keep_server_running = true;
static bool use_sigkill = false;
static bool server_startup = true;
static bool use_bonjour = true;
static bool use_ctrl_panel = true;
static bool use_web_apps = true;

#ifdef RPI_MANAGEMENT
static bool use_rpi_management = false;
#endif /* RPI_MANAGEMENT */

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

#ifdef RPI_MANAGEMENT
static indigo_result execute_command(indigo_device *device, indigo_property *property, char *command, ...) {
	char buffer[1024];
	va_list args;
	va_start(args, command);
	vsnprintf(buffer, sizeof(buffer), command, args);
	va_end(args);
	property->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, property, NULL);
	FILE *output = popen(buffer, "r");
	if (output) {
		char *line = NULL;
		size_t size = 0;
		if (getline(&line, &size, output) >= 0) {
			char *nl = strchr(line, '\n');
			if (nl)
				*nl = 0;
			if (!strncmp(line, "ALERT", 5)) {
				property->state = INDIGO_ALERT_STATE;
			} else {
				property->state = INDIGO_OK_STATE;
			}
			indigo_update_property(device, property, line);
		} else {
			property->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, property, "No reply from %s", buffer);
		}
		if (line)
			free(line);
		pclose(output);

		return INDIGO_OK;
	}
	property->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, property, strerror(errno));
	return INDIGO_OK;
}

static char *execute_query(char *command, ...) {
	char buffer[1024];
	va_list args;
	va_start(args, command);
	vsnprintf(buffer, sizeof(buffer), command, args);
	va_end(args);
	FILE *output = popen(buffer, "r");
	if (output) {
		char *line = NULL;
		size_t size = 0;
		if (getline(&line, &size, output) >= 0) {
			char *nl = strchr(line, '\n');
			if (nl)
				*nl = 0;
			pclose(output);
			return line;
		}
		pclose(output);
	}
	return NULL;
}

#endif

static indigo_result attach(indigo_device *device) {
	assert(device != NULL);
	drivers_property = indigo_init_switch_property(NULL, server_device.name, "DRIVERS", MAIN_GROUP, "Available drivers", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, INDIGO_MAX_DRIVERS);
	drivers_property->count = 0;
	for (int i = 0; i < INDIGO_MAX_DRIVERS; i++)
		if (indigo_available_drivers[i].driver != NULL)
			indigo_init_switch_item(&drivers_property->items[drivers_property->count++], indigo_available_drivers[i].name, indigo_available_drivers[i].description, indigo_available_drivers[i].initialized);
	for (int i = 0; i < dynamic_drivers_count && drivers_property->count < INDIGO_MAX_DRIVERS; i++)
		indigo_init_switch_item(&drivers_property->items[drivers_property->count++], dynamic_drivers[i].name, dynamic_drivers[i].description, false);
	servers_property = indigo_init_light_property(NULL, server_device.name, "SERVERS", MAIN_GROUP, "Configured servers", INDIGO_OK_STATE, 2 * INDIGO_MAX_SERVERS);
	servers_property->count = 0;
	for (int i = 0; i < INDIGO_MAX_SERVERS; i++) {
		indigo_server_entry *entry = indigo_available_servers + i;
		if (*entry->host) {
			char buf[INDIGO_NAME_SIZE];
			if (entry->port == 7624)
				strncpy(buf, entry->host, sizeof(buf));
			else
				snprintf(buf, sizeof(buf), "%s:%d", entry->host, entry->port);
			indigo_init_light_item(&servers_property->items[servers_property->count++], buf, buf, INDIGO_OK_STATE);
		}
	}
	for (int i = 0; i < INDIGO_MAX_SERVERS; i++) {
		indigo_subprocess_entry *entry = indigo_available_subprocesses + i;
		if (*entry->executable) {
			indigo_init_light_item(&servers_property->items[servers_property->count++], entry->executable, entry->executable, INDIGO_OK_STATE);
		}
	}
	load_property = indigo_init_text_property(NULL, server_device.name, "LOAD", MAIN_GROUP, "Load driver", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
	indigo_init_text_item(LOAD_ITEM, "DRIVER", "Load driver", "");
	unload_property = indigo_init_text_property(NULL, server_device.name, "UNLOAD", MAIN_GROUP, "Unload driver", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
	indigo_init_text_item(UNLOAD_ITEM, "DRIVER", "Unload driver", "");
	restart_property = indigo_init_switch_property(NULL, server_device.name, "RESTART", MAIN_GROUP, "Restart", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
	indigo_init_switch_item(RESTART_ITEM, "RESTART", "Restart server", false);
	log_level_property = indigo_init_switch_property(NULL, device->name, "LOG_LEVEL", MAIN_GROUP, "Log level", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 4);
	indigo_init_switch_item(LOG_LEVEL_ERROR_ITEM, "ERROR", "Error", false);
	indigo_init_switch_item(LOG_LEVEL_INFO_ITEM, "INFO", "Info", false);
	indigo_init_switch_item(LOG_LEVEL_DEBUG_ITEM, "DEBUG", "Debug", false);
	indigo_init_switch_item(LOG_LEVEL_TRACE_ITEM, "TRACE", "Trace", false);
	server_features_property = indigo_init_switch_property(NULL, device->name, "FEATURES", MAIN_GROUP, "Features", INDIGO_OK_STATE, INDIGO_RO_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
	indigo_init_switch_item(BONJOUR_ITEM, "BONJOUR", "Bonjour", use_bonjour);
	indigo_init_switch_item(CTRL_PANEL_ITEM, "CTRL_PANEL", "Control panel / Server manager", use_ctrl_panel);
	indigo_init_switch_item(WEB_APPS_ITEM, "WEB_APPS", "Web applications", use_web_apps);
#ifdef RPI_MANAGEMENT
	if (use_rpi_management) {
		char *line;
		wifi_ap_property = indigo_init_text_property(NULL, server_device.name, "WIFI_AP", MAIN_GROUP, "Configure access point WiFi mode", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		indigo_init_text_item(wifi_ap_property->items + 0, "SSID", "SSID", "");
		indigo_init_text_item(wifi_ap_property->items + 1, "PASSWORD", "Password", "");
		line = execute_query("s_rpi_ctrl.sh --get-wifi-server");
		if (line) {
			char *token = strtok(line, " ");
			if (token)
				strncpy(wifi_ap_property->items[0].text.value, token, INDIGO_VALUE_SIZE);
			token = strtok(NULL, " ");
			if (token)
				strncpy(wifi_ap_property->items[1].text.value, token, INDIGO_VALUE_SIZE);
		}
		wifi_infrastructure_property = indigo_init_text_property(NULL, server_device.name, "WIFI_INFRASTRUCTURE", MAIN_GROUP, "Configure infrastructure WiFi mode", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		indigo_init_text_item(wifi_infrastructure_property->items + 0, "SSID", "SSID", "");
		indigo_init_text_item(wifi_infrastructure_property->items + 1, "PASSWORD", "Password", "");
		line = execute_query("s_rpi_ctrl.sh --get-wifi-client");
		if (line) {
			char *token = strtok(line, " ");
			if (token)
				strncpy(wifi_infrastructure_property->items[0].text.value, token, INDIGO_VALUE_SIZE);
		}
		host_time_property = indigo_init_text_property(NULL, server_device.name, "HOST_TIME", MAIN_GROUP, "Set host time", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		indigo_init_text_item(host_time_property->items + 0, "TIME", "Host time", "");
		shutdown_property = indigo_init_switch_property(NULL, server_device.name, "SHUTDOWN", MAIN_GROUP, "Shutdown host computer", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
		indigo_init_switch_item(shutdown_property->items + 0, "SHUTDOWN", "Shutdown", false);
		reboot_property = indigo_init_switch_property(NULL, server_device.name, "REBOOT", MAIN_GROUP, "Reboot host computer", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
		indigo_init_switch_item(reboot_property->items + 0, "REBOOT", "Reboot", false);
		install_property = indigo_init_switch_property(NULL, server_device.name, "INSTALL", MAIN_GROUP, "Available versions", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 10);
		install_property->count = 0;
		line = execute_query("s_rpi_ctrl.sh --list-available-versions");
		if (line) {
			char *versions[10] = { strtok(line, " ") };
			int count = 1;
			while ((versions[count] = strtok(NULL, " "))) {
				if (count == 9)
					count = 1;
				else
					count++;
			}
			for (int i = 0; i < count; i++) {
				char *smallest = "XXXXX";
				int ii = 0;
				for (int j = 0; j < count; j++) {
					if (versions[j] && strcmp(versions[j], smallest) < 0) {
						smallest = versions[j];
						ii = j;
					}
				}
				versions[ii] = NULL;
				indigo_init_switch_item(install_property->items + i, smallest, smallest, smallest == line);
				install_property->count++;
			}
			free(line);
		}
	}
#endif /* RPI_MANAGEMENT */
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
	if (!command_line_drivers)
		indigo_load_properties(device, false);
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
	indigo_define_property(device, server_features_property, NULL);
#ifdef RPI_MANAGEMENT
	if (use_rpi_management) {
		indigo_define_property(device, wifi_ap_property, NULL);
		indigo_define_property(device, wifi_infrastructure_property, NULL);
		indigo_define_property(device, host_time_property, NULL);
		indigo_define_property(device, shutdown_property, NULL);
		indigo_define_property(device, reboot_property, NULL);
		indigo_define_property(device, install_property, NULL);
	}
#endif /* RPI_MANAGEMENT */
	return INDIGO_OK;
}

static indigo_result change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(property != NULL);
	if (indigo_property_match(drivers_property, property)) {
	// -------------------------------------------------------------------------------- DRIVERS
		if (command_line_drivers && !strcmp(client->name, CONFIG_READER))
			return INDIGO_OK;
		indigo_property_copy_values(drivers_property, property, false);
		for (int i = 0; i < drivers_property->count; i++) {
			char *name = drivers_property->items[i].name;
			indigo_driver_entry *driver = NULL;
			for (int j = 0; j < INDIGO_MAX_DRIVERS; j++) {
				if (!strcmp(indigo_available_drivers[j].name, name)) {
					driver = &indigo_available_drivers[j];
					break;
				}
			}
			if (drivers_property->items[i].sw.value) {
				if (driver) {
					if (driver->dl_handle == NULL && !driver->initialized)
						drivers_property->items[i].sw.value = indigo_available_drivers[i].initialized = indigo_available_drivers[i].driver(INDIGO_DRIVER_INIT, NULL) == INDIGO_OK;
				} else {
					drivers_property->items[i].sw.value = indigo_available_drivers[i].initialized = indigo_load_driver(name, true, NULL) == INDIGO_OK;
				}
			} else if (driver) {
				if (driver->dl_handle) {
					indigo_remove_driver(driver);
				} else if (driver->initialized) {
					indigo_available_drivers[i].driver(INDIGO_DRIVER_SHUTDOWN, NULL);
					indigo_available_drivers[i].initialized = false;
				}
			}
		}
		drivers_property->state = INDIGO_OK_STATE;
		indigo_update_property(device, drivers_property, NULL);
		int handle = 0;
		if (!command_line_drivers)
			indigo_save_property(device, &handle, drivers_property);
		close(handle);
		return INDIGO_OK;
	} else if (indigo_property_match(load_property, property)) {
		// -------------------------------------------------------------------------------- LOAD
		indigo_property_copy_values(load_property, property, false);
		if (*LOAD_ITEM->text.value) {
			char *name = basename(LOAD_ITEM->text.value);
			for (int i = 0; i < INDIGO_MAX_DRIVERS; i++)
				if (!strcmp(name, indigo_available_drivers[i].name)) {
					load_property->state = INDIGO_ALERT_STATE;
					indigo_update_property(device, load_property, "Driver %s (%s) is already loaded", name, indigo_available_drivers[i].description);
					return INDIGO_OK;
				}
			indigo_driver_entry *driver;
			if (indigo_load_driver(LOAD_ITEM->text.value, true, &driver) == INDIGO_OK) {
				bool found = false;
				for (int i = 0; i < drivers_property->count; i++) {
					if (!strcmp(drivers_property->items[i].name, name)) {
						drivers_property->items[i].sw.value = true;
						drivers_property->state = INDIGO_OK_STATE;
						indigo_update_property(device, drivers_property, NULL);
						found = true;
						break;
					}
				}
				if (!found) {
					indigo_delete_property(device, drivers_property, NULL);
					indigo_init_switch_item(&drivers_property->items[drivers_property->count++], driver->name, driver->description, driver->initialized);
					drivers_property->state = INDIGO_OK_STATE;
					indigo_define_property(device, drivers_property, NULL);
				}
				load_property->state = INDIGO_OK_STATE;
				indigo_update_property(device, load_property, "Driver %s (%s) loaded", name, driver->description);
			} else {
				load_property->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, load_property, indigo_last_message);
			}
		}
		return INDIGO_OK;
	} else if (indigo_property_match(unload_property, property)) {
		// -------------------------------------------------------------------------------- UNLOAD
		indigo_property_copy_values(unload_property, property, false);
		if (*UNLOAD_ITEM->text.value) {
			char *name = basename(UNLOAD_ITEM->text.value);
			for (int i = 0; i < INDIGO_MAX_DRIVERS; i++)
				if (!strcmp(name, indigo_available_drivers[i].name)) {
					if (indigo_available_drivers[i].dl_handle) {
						indigo_remove_driver(&indigo_available_drivers[i]);
					} else {
						indigo_available_drivers[i].driver(INDIGO_DRIVER_SHUTDOWN, NULL);
					}
					for (int j = 0; j < drivers_property->count; j++) {
						if (!strcmp(drivers_property->items[j].name, name)) {
							drivers_property->items[j].sw.value = false;
							drivers_property->state = INDIGO_OK_STATE;
							indigo_update_property(device, drivers_property, NULL);
							break;
						}
					}
					unload_property->state = INDIGO_OK_STATE;
					indigo_update_property(device, unload_property, NULL);
					return INDIGO_OK;
				}
			unload_property->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, unload_property, "Driver %s is not loaded", name);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(restart_property, property)) {
	// -------------------------------------------------------------------------------- RESTART
		indigo_property_copy_values(restart_property, property, false);
		if (RESTART_ITEM->sw.value) {
			INDIGO_LOG(indigo_log("Restarting..."));
			indigo_server_shutdown();
			exit(0);
		}
		return INDIGO_OK;
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
		return INDIGO_OK;
#ifdef RPI_MANAGEMENT
	} else if (indigo_property_match(wifi_ap_property, property)) {
		// -------------------------------------------------------------------------------- WIFI_AP
		indigo_property_copy_values(wifi_ap_property, property, false);
		return execute_command(device, wifi_ap_property, "s_rpi_ctrl.sh --set-wifi-server \"%s\" \"%s\"", wifi_ap_property->items[0].text.value, wifi_ap_property->items[1].text.value);
	} else if (indigo_property_match(wifi_infrastructure_property, property)) {
		// -------------------------------------------------------------------------------- WIFI_INFRASTRUCTURE
		indigo_property_copy_values(wifi_infrastructure_property, property, false);
		return execute_command(device, wifi_infrastructure_property, "s_rpi_ctrl.sh --set-wifi-client \"%s\" \"%s\"", wifi_infrastructure_property->items[0].text.value, wifi_infrastructure_property->items[1].text.value);
	} else if (indigo_property_match(host_time_property, property)) {
		// -------------------------------------------------------------------------------- HOST_TIME
		indigo_property_copy_values(host_time_property, property, false);
		execute_command(device, host_time_property, "s_rpi_ctrl.sh --set-date \"%s\"", host_time_property->items[0].text.value);
		if (host_time_property->state == INDIGO_OK_STATE) {
			indigo_delete_property(device, host_time_property, NULL);
			host_time_property->hidden = true;
		}
		return INDIGO_OK;
	} else if (indigo_property_match(shutdown_property, property)) {
		// -------------------------------------------------------------------------------- SHUTDOWN
		indigo_property_copy_values(shutdown_property, property, false);
		return execute_command(device, shutdown_property, "s_rpi_ctrl.sh --poweroff");
	} else if (indigo_property_match(reboot_property, property)) {
		// -------------------------------------------------------------------------------- REBOOT
		indigo_property_copy_values(reboot_property, property, false);
		return execute_command(device, reboot_property, "s_rpi_ctrl.sh --reboot");
	} else if (indigo_property_match(install_property, property)) {
		// -------------------------------------------------------------------------------- INSTALL
		indigo_property_copy_values(install_property, property, false);
		for (int i = 0; i < property->count; i++) {
			if (property->items[i].sw.value)
				return execute_command(device, install_property, "s_rpi_ctrl.sh --install-version %s", property->items[i].name);
		}
#endif /* RPI_MANAGEMENT */
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
	indigo_delete_property(device, server_features_property, NULL);
#ifdef RPI_MANAGEMENT
	if (use_rpi_management) {
		indigo_delete_property(device, wifi_ap_property, NULL);
		indigo_delete_property(device, wifi_infrastructure_property, NULL);
		indigo_delete_property(device, host_time_property, NULL);
		indigo_delete_property(device, shutdown_property, NULL);
		indigo_delete_property(device, reboot_property, NULL);
		indigo_delete_property(device, install_property, NULL);
	}
#endif /* RPI_MANAGEMENT */
	INDIGO_LOG(indigo_log("%s detached", device->name));
	return INDIGO_OK;
}

static void add_drivers(const char *folder) {
	char folder_path[PATH_MAX];
	if(NULL == realpath(folder, folder_path)) {
		INDIGO_DEBUG(indigo_debug("realpath(%s, folder_path): failed", folder));
		return;
	}
	DIR *dir = opendir(folder_path);
	if (dir) {
		struct dirent *ent;
		char *line = NULL;
		size_t len = 0;
		while ((ent = readdir (dir)) != NULL) {
			if (!strncmp(ent->d_name, "indigo_", 7)) {
				char path[PATH_MAX];
				sprintf(path, "%s/%s", folder_path, ent->d_name);
				indigo_log("Loading driver list from %s", path);
				FILE *list = fopen(path, "r");
				if (list) {
					while (getline(&line, &len, list) > 0 && dynamic_drivers_count < INDIGO_MAX_DRIVERS) {
						char *token = strtok(line, ",");
						if (token && (token = strchr(token, '"'))) {
							char *end = strchr(++token, '"');
							if (end) {
								*end = 0;
								for (int i = 0; i < INDIGO_MAX_DRIVERS; i++) {
									if (!strcmp(indigo_available_drivers[i].name, token)) {
										token = NULL;
										break;
									}
								}
								if (token) {
									for (int i = 0; i < dynamic_drivers_count; i++) {
										if (!strcmp(dynamic_drivers[i].name, token)) {
											token = NULL;
											break;
										}
									}
								}
								if (token) {
									dynamic_drivers[dynamic_drivers_count].name = strdup(token);
								} else {
									continue;
								}
							}
						}
						token = strtok(NULL, ",");
						if (token && (token = strchr(token, '"'))) {
							char *end = strchr(token + 1, '"');
							if (end) {
								*end = 0;
								dynamic_drivers[dynamic_drivers_count].description = strdup(token + 1);
							}
						}
						dynamic_drivers_count++;
					}
					fclose(list);
				}
			}
		}
		closedir(dir);
		if (line)
			free(line);
	}
}

static void server_main() {
	indigo_start_usb_event_handler();
	indigo_start();
	indigo_log("INDIGO server %d.%d-%d built on %s %s", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF, INDIGO_BUILD, __DATE__, __TIME__);

	for (int i = 1; i < server_argc; i++) {
		if ((!strcmp(server_argv[i], "-p") || !strcmp(server_argv[i], "--port")) && i < server_argc - 1) {
			indigo_server_tcp_port = atoi(server_argv[i + 1]);
			i++;
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
			use_ctrl_panel = false;
		} else if (!strcmp(server_argv[i], "-w-") || !strcmp(server_argv[i], "--disable-web-apps")) {
			use_web_apps = false;
		} else if (!strcmp(server_argv[i], "-u-") || !strcmp(server_argv[i], "--disable-blob-urls")) {
			indigo_use_blob_urls = false;
#ifdef RPI_MANAGEMENT
		} else if (!strcmp(server_argv[i], "-f") || !strcmp(server_argv[i], "--enable-rpi-management")) {
			FILE *output = popen("which s_rpi_ctrl.sh", "r");
			if (output) {
				char *line = NULL;
				size_t size = 0;
				if (getline(&line, &size, output) >= 0) {
					use_rpi_management = true;
				}
				pclose(output);
			}
			if (!use_rpi_management) {
				indigo_log("No s_rpi_ctrl.sh found");
			}
#endif /* RPI_MANAGEMENT */
		} else if(server_argv[i][0] != '-') {
			indigo_load_driver(server_argv[i], true, NULL);
			command_line_drivers = true;
		}
	}

	use_ctrl_panel |= use_web_apps;

	if (use_ctrl_panel) {
		// INDIGO Server Manager
		static unsigned char mng_html[] = {
			#include "resource/mng.html.data"
		};
		indigo_server_add_resource("/mng.html", mng_html, sizeof(mng_html), "text/html");
		static unsigned char mng_png[] = {
			#include "resource/mng.png.data"
		};
		indigo_server_add_resource("/mng.png", mng_png, sizeof(mng_png), "image/png");
		// INDIGO Control Panel
		static unsigned char ctrl_html[] = {
			#include "resource/ctrl.html.data"
		};
		indigo_server_add_resource("/ctrl.html", ctrl_html, sizeof(ctrl_html), "text/html");
		static unsigned char ctrl_png[] = {
			#include "resource/ctrl.png.data"
		};
		indigo_server_add_resource("/ctrl.png", ctrl_png, sizeof(ctrl_png), "image/png");
		// INDIGO
		static unsigned char indigo_js[] = {
			#include "resource/indigo.js.data"
		};
		indigo_server_add_resource("/indigo.js", indigo_js, sizeof(indigo_js), "text/javascript");
		static unsigned char components_js[] = {
			#include "resource/components.js.data"
		};
		indigo_server_add_resource("/components.js", components_js, sizeof(components_js), "text/javascript");
		static unsigned char indigo_css[] = {
			#include "resource/indigo.css.data"
		};
		indigo_server_add_resource("/indigo.css", indigo_css, sizeof(indigo_css), "text/css");
		// Bootstrap
		static unsigned char bootstrap_css[] = {
			#include "resource/bootstrap.min.css.data"
		};
		indigo_server_add_resource("/bootstrap.min.css", bootstrap_css, sizeof(bootstrap_css), "text/css");
		static unsigned char bootstrap_js[] = {
			#include "resource/bootstrap.min.js.data"
		};
		indigo_server_add_resource("/bootstrap.min.js", bootstrap_js, sizeof(bootstrap_js), "text/javascript");
		static unsigned char popper_js[] = {
			#include "resource/popper.min.js.data"
		};
		indigo_server_add_resource("/popper.min.js", popper_js, sizeof(popper_js), "text/javascript");
		static unsigned char glyphicons_css[] = {
			#include "resource/glyphicons.css.data"
		};
		indigo_server_add_resource("/glyphicons.css", glyphicons_css, sizeof(glyphicons_css), "text/css");
		static unsigned char glyphicons_ttf[] = {
			#include "resource/glyphicons-regular.ttf.data"
		};
		indigo_server_add_resource("/glyphicons-regular.ttf", glyphicons_ttf, sizeof(glyphicons_ttf), "font/ttf");
		// JQuery
		static unsigned char jquery_js[] = {
			#include "resource/jquery.min.js.data"
		};
		indigo_server_add_resource("/jquery.min.js", jquery_js, sizeof(jquery_js), "text/javascript");
		// VueJS
		static unsigned char vue_js[] = {
			#include "resource/vue.min.js.data"
		};
		indigo_server_add_resource("/vue.min.js", vue_js, sizeof(vue_js), "text/javascript");
	}
	if (use_web_apps) {
		// INDIGO Imager
		static unsigned char imager_html[] = {
			#include "resource/imager.html.data"
		};
		indigo_server_add_resource("/imager.html", imager_html, sizeof(imager_html), "text/html");
		static unsigned char imager_png[] = {
			#include "resource/imager.png.data"
		};
		indigo_server_add_resource("/imager.png", imager_png, sizeof(imager_png), "image/png");
		// INDIGO Mount
		static unsigned char mount_html[] = {
			#include "resource/mount.html.data"
		};		indigo_server_add_resource("/mount.html", mount_html, sizeof(mount_html), "text/html");
		static unsigned char mount_png[] = {
			#include "resource/mount.png.data"
		};
		indigo_server_add_resource("/mount.png", mount_png, sizeof(mount_png), "image/png");
		static unsigned char celestial_js[] = {
			#include "resource/celestial.min.js.data"
		};
		indigo_server_add_resource("/celestial.min.js", celestial_js, sizeof(celestial_js), "text/javascript");
		static unsigned char d3_js[] = {
			#include "resource/d3.min.js.data"
		};
		indigo_server_add_resource("/d3.min.js", d3_js, sizeof(d3_js), "text/javascript");
		static unsigned char celestial_css[] = {
			#include "resource/celestial.css.data"
		};
		indigo_server_add_resource("/celestial.css", celestial_css, sizeof(celestial_css), "text/css");
		static unsigned char constellations_json[] = {
			#include "resource/data/constellations.json.data"
		};
		indigo_server_add_resource("/data/constellations.json", constellations_json, sizeof(constellations_json), "application/json; charset=utf-8");
		static unsigned char constellations_bounds_json[] = {
			#include "resource/data/constellations.bounds.json.data"
		};
		indigo_server_add_resource("/data/constellations.bounds.json", constellations_bounds_json, sizeof(constellations_bounds_json), "application/json; charset=utf-8");
		static unsigned char mv_json[] = {
			#include "resource/data/mw.json.data"
		};
		indigo_server_add_resource("/data/mw.json", mv_json, sizeof(mv_json), "application/json; charset=utf-8");
		static unsigned char planets_json[] = {
			#include "resource/data/planets.json.data"
		};
		indigo_server_add_resource("/data/planets.json", planets_json, sizeof(planets_json), "application/json; charset=utf-8");
		star_data = indigo_add_star_json_resource(6);
		dso_data = indigo_add_dso_json_resource(10);
		constellation_data = indigo_add_constellations_lines_json_resource();
		// INDIGO Guider
		static unsigned char guider_png[] = {
			#include "resource/guider.png.data"
		};
		indigo_server_add_resource("/guider.png", guider_png, sizeof(guider_png), "image/png");
	}

	if (!command_line_drivers) {
		for (static_drivers_count = 0; static_drivers[static_drivers_count]; static_drivers_count++) {
			indigo_add_driver(static_drivers[static_drivers_count], false, NULL);
		}
		char *last = strrchr(server_argv[0], '/');
		if (last) {
			char path[PATH_MAX];
			long len = last - server_argv[0];
			strncpy(path, server_argv[0], len);
			path[len] = 0;
			strcat(path, "/../share/indigo");
			add_drivers(path);
		}
		add_drivers("/usr/share/indigo");
		add_drivers("/usr/local/share/indigo");
	}
	indigo_attach_device(&server_device);

#ifdef INDIGO_LINUX
	indigo_server_start(server_callback);
#endif
#ifdef INDIGO_MACOS
	if (!indigo_async((void * (*)(void *))indigo_server_start, server_callback)) {
		INDIGO_ERROR(indigo_error("Error creating thread for server"));
	}
	runLoop = true;
	while (runLoop) {
		CFRunLoopRunInMode(kCFRunLoopDefaultMode, 1, true);
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
	if (star_data)
		free(star_data);
	if (dso_data)
		free(dso_data);
	if (constellation_data)
		free(constellation_data);
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
			printf("usage: %s [-h | --help]\n", argv[0]);
			printf("       %s [options] indigo_driver_name indigo_driver_name ...\n", argv[0]);
			printf("options:\n"
			       "       --  | --do-not-fork\n"
			       "       -l  | --use-syslog\n"
			       "       -p  | --port port                     (default: 7624)\n"
			       "       -b  | --bonjour name                  (default: hostname)\n"
			       "       -b- | --disable-bonjour\n"
			       "       -u- | --disable-blob-urls\n"
			       "       -w- | --disable-web-apps\n"
			       "       -c- | --disable-control-panel\n"
#ifdef RPI_MANAGEMENT
			       "       -f  | --enable-rpi-management\n"
#endif /* RPI_MANAGEMENT */
			       "       -v  | --enable-info\n"
			       "       -vv | --enable-debug\n"
			       "       -vvv| --enable-trace\n"
			       "       -r  | --remote-server host[:port]     (default port: 7624)\n"
			       "       -i  | --indi-driver driver_executable\n"
			);
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
				char *server_string = strstr(server_argv[0], "indigo_server");
				if (server_string) {
					static char process_name[] = "indigo_worker";
					int len = strlen(server_argv[0]);
					strcpy(server_string, process_name);
					prctl(PR_SET_PDEATHSIG, SIGINT, 0, 0, 0);
					/* Linux requires additional step to change process name */
					prctl(PR_SET_NAME, process_name, 0, 0, 0);
				}
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
