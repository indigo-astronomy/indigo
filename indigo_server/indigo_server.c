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

#include <indigo/indigo_bus.h>
#include <indigo/indigo_server_tcp.h>
#include <indigo/indigo_driver.h>
#include <indigo/indigo_client.h>
#include <indigo/indigo_xml.h>
#include <indigo/indigo_token.h>

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
#include "ccd_qhy2/indigo_ccd_qhy.h"
#include "focuser_fcusb/indigo_focuser_fcusb.h"
#include "ccd_iidc/indigo_ccd_iidc.h"
#include "mount_lx200/indigo_mount_lx200.h"
#include "mount_synscan/indigo_mount_synscan.h"
#include "mount_nexstar/indigo_mount_nexstar.h"
#include "mount_nexstaraux/indigo_mount_nexstaraux.h"
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
#include "aux_ppb/indigo_aux_ppb.h"
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
#include "agent_guider/indigo_agent_guider.h"
#include "system_ascol/indigo_system_ascol.h"
#include "focuser_steeldrive2/indigo_focuser_steeldrive2.h"
#include "agent_auxiliary/indigo_agent_auxiliary.h"
#include "aux_sqm/indigo_aux_sqm.h"
#include "focuser_dsd/indigo_focuser_dsd.h"
#include "ccd_ptp/indigo_ccd_ptp.h"
#include "focuser_efa/indigo_focuser_efa.h"
#include "aux_arteskyflat/indigo_aux_arteskyflat.h"
#include "aux_flipflat/indigo_aux_flipflat.h"
#include "dome_nexdome/indigo_dome_nexdome.h"
#include "dome_nexdome3/indigo_dome_nexdome3.h"
#include "aux_usbdp/indigo_aux_usbdp.h"
#include "aux_flatmaster/indigo_aux_flatmaster.h"
#include "focuser_focusdreampro/indigo_focuser_focusdreampro.h"
#include "aux_fbc/indigo_aux_fbc.h"
#include "mount_rainbow/indigo_mount_rainbow.h"
#include "gps_gpsd/indigo_gps_gpsd.h"
#include "focuser_lunatico/indigo_focuser_lunatico.h"
#include "rotator_simulator/indigo_rotator_simulator.h"
#include "rotator_lunatico/indigo_rotator_lunatico.h"
#include "aux_dragonfly/indigo_aux_dragonfly.h"
#include "dome_dragonfly/indigo_dome_dragonfly.h"
#include "aux_cloudwatcher/indigo_aux_cloudwatcher.h"
#include "wheel_manual/indigo_wheel_manual.h"
#include "aux_mgbox/indigo_aux_mgbox.h"
#include "dome_baader/indigo_dome_baader.h"
#include "mount_pmc8/indigo_mount_pmc8.h"
#include "focuser_robofocus/indigo_focuser_robofocus.h"
#include "wheel_qhy/indigo_wheel_qhy.h"
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
	indigo_agent_auxiliary,
	indigo_agent_guider,
	indigo_agent_imager,
	indigo_agent_lx200_server,
	indigo_agent_mount,
	indigo_agent_snoop,
	indigo_ao_sx,
	indigo_aux_arteskyflat,
	indigo_aux_cloudwatcher,
	indigo_aux_dragonfly,
	indigo_aux_dsusb,
	indigo_aux_fbc,
	indigo_aux_flatmaster,
	indigo_aux_flipflat,
	indigo_aux_joystick,
	indigo_aux_mgbox,
	indigo_aux_ppb,
	indigo_aux_rts,
	indigo_aux_sqm,
	indigo_aux_upb,
	indigo_aux_usbdp,
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
	//indigo_ccd_mi,
	indigo_ccd_ptp,
	indigo_ccd_qhy2,
	indigo_ccd_qsi,
#ifndef __aarch64__
	indigo_ccd_sbig,
#endif
	indigo_ccd_simulator,
	indigo_ccd_ssag,
	indigo_ccd_sx,
	indigo_ccd_touptek,
	indigo_ccd_uvc,
	indigo_dome_baader,
	indigo_dome_dragonfly,
	indigo_dome_nexdome,
	indigo_dome_nexdome3,
	indigo_dome_simulator,
	indigo_focuser_asi,
	indigo_focuser_efa,
	indigo_focuser_dmfc,
	indigo_focuser_dsd,
	indigo_focuser_fcusb,
	indigo_focuser_focusdreampro,
	indigo_focuser_fli,
	indigo_focuser_lakeside,
	indigo_focuser_lunatico,
	indigo_focuser_mjkzz,
#ifdef INDIGO_MACOS
	indigo_focuser_mjkzz_bt,
#endif
	indigo_focuser_moonlite,
	indigo_focuser_nfocus,
	indigo_focuser_nstep,
	indigo_focuser_optec,
	indigo_focuser_robofocus,
	indigo_focuser_usbv3,
	indigo_focuser_steeldrive2,
	indigo_focuser_wemacro,
#ifdef INDIGO_MACOS
	indigo_focuser_wemacro_bt,
#endif
	indigo_gps_gpsd,
	indigo_gps_nmea,
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
	indigo_mount_nexstaraux,
	indigo_mount_pmc8,
	indigo_mount_rainbow,
	indigo_mount_simulator,
	indigo_mount_synscan,
	indigo_mount_synscan,
	indigo_mount_temma,
	indigo_rotator_lunatico,
	indigo_rotator_simulator,
	indigo_wheel_asi,
	indigo_wheel_atik,
	indigo_wheel_fli,
	indigo_wheel_manual,
	indigo_wheel_optec,
	indigo_wheel_qhy,
	indigo_wheel_quantum,
	indigo_wheel_sx,
	indigo_wheel_trutek,
	indigo_wheel_xagyl,
	indigo_system_ascol,
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

static indigo_property *info_property;
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
static indigo_property *internet_sharing_property;
static indigo_property *host_time_property;
static indigo_property *shutdown_property;
static indigo_property *reboot_property;
static indigo_property *install_property;
static pthread_mutex_t install_property_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

static DNSServiceRef sd_http;
static DNSServiceRef sd_indigo;

static void *star_data = NULL;
static void *dso_data = NULL;
static void *constellation_data = NULL;

#ifdef INDIGO_MACOS
static bool runLoop = true;
#endif

#define SERVER_INFO_PROPERTY											info_property
#define SERVER_INFO_VERSION_ITEM									(SERVER_INFO_PROPERTY->items + 0)
#define SERVER_INFO_SERVICE_ITEM									(SERVER_INFO_PROPERTY->items + 1)

#define SERVER_DRIVERS_PROPERTY										drivers_property

#define SERVER_SERVERS_PROPERTY										servers_property

#define SERVER_LOAD_PROPERTY											load_property
#define SERVER_LOAD_ITEM													(SERVER_LOAD_PROPERTY->items + 0)

#define SERVER_UNLOAD_PROPERTY										unload_property
#define SERVER_UNLOAD_ITEM												(SERVER_UNLOAD_PROPERTY->items + 0)

#define SERVER_RESTART_PROPERTY										restart_property
#define SERVER_RESTART_ITEM												(SERVER_RESTART_PROPERTY->items + 0)

#define SERVER_LOG_LEVEL_PROPERTY									log_level_property
#define SERVER_LOG_LEVEL_ERROR_ITEM								(SERVER_LOG_LEVEL_PROPERTY->items + 0)
#define SERVER_LOG_LEVEL_INFO_ITEM								(SERVER_LOG_LEVEL_PROPERTY->items + 1)
#define SERVER_LOG_LEVEL_DEBUG_ITEM								(SERVER_LOG_LEVEL_PROPERTY->items + 2)
#define SERVER_LOG_LEVEL_TRACE_ITEM								(SERVER_LOG_LEVEL_PROPERTY->items + 3)

#define SERVER_FEATURES_PROPERTY									server_features_property
#define SERVER_BONJOUR_ITEM												(SERVER_FEATURES_PROPERTY->items + 0)
#define SERVER_CTRL_PANEL_ITEM										(SERVER_FEATURES_PROPERTY->items + 1)
#define SERVER_WEB_APPS_ITEM											(SERVER_FEATURES_PROPERTY->items + 2)

#define SERVER_WIFI_AP_PROPERTY										wifi_ap_property
#define SERVER_WIFI_AP_SSID_ITEM									(SERVER_WIFI_AP_PROPERTY->items + 0)
#define SERVER_WIFI_AP_PASSWORD_ITEM							(SERVER_WIFI_AP_PROPERTY->items + 1)

#define SERVER_WIFI_INFRASTRUCTURE_PROPERTY				wifi_infrastructure_property
#define SERVER_WIFI_INFRASTRUCTURE_SSID_ITEM			(SERVER_WIFI_INFRASTRUCTURE_PROPERTY->items + 0)
#define SERVER_WIFI_INFRASTRUCTURE_PASSWORD_ITEM	(SERVER_WIFI_INFRASTRUCTURE_PROPERTY->items + 1)

#define SERVER_INTERNET_SHARING_PROPERTY					internet_sharing_property
#define SERVER_INTERNET_SHARING_DISABLED_ITEM			(SERVER_INTERNET_SHARING_PROPERTY->items + 0)
#define SERVER_INTERNET_SHARING_ENABLED_ITEM			(SERVER_INTERNET_SHARING_PROPERTY->items + 1)

#define SERVER_HOST_TIME_PROPERTY									host_time_property
#define SERVER_HOST_TIME_ITEM											(SERVER_HOST_TIME_PROPERTY->items + 0)

#define SERVER_SHUTDOWN_PROPERTY									shutdown_property
#define SERVER_SHUTDOWN_ITEM											(SERVER_SHUTDOWN_PROPERTY->items + 0)

#define SERVER_REBOOT_PROPERTY										reboot_property
#define SERVER_REBOOT_ITEM												(SERVER_REBOOT_PROPERTY->items + 0)

#define SERVER_INSTALL_PROPERTY										install_property


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
		char hostname[INDIGO_NAME_SIZE];
		gethostname(hostname, sizeof(hostname));
		if (use_bonjour) {
			/* UGLY but the only way to suppress compat mode warning messages on Linux */
			setenv("AVAHI_COMPAT_NOWARN", "1", 1);
			if (*indigo_local_service_name == 0) {
				indigo_service_name(hostname, indigo_server_tcp_port, indigo_local_service_name);
			}
			DNSServiceRegister(&sd_http, 0, 0, indigo_local_service_name, MDNS_HTTP_TYPE, NULL, NULL, htons(indigo_server_tcp_port), 0, NULL, NULL, NULL);
			DNSServiceRegister(&sd_indigo, 0, 0, indigo_local_service_name, MDNS_INDIGO_TYPE, NULL, NULL, htons(indigo_server_tcp_port), 0, NULL, NULL, NULL);
		}
		strcpy(SERVER_INFO_SERVICE_ITEM->text.value, hostname);
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
			indigo_update_property(device, property, NULL);
		} else {
			property->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, property, "No reply from rpi_ctrl.sh");
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

static void check_versions(indigo_device *device) {
	while (true) {
		bool redefine = false;
		char *line = execute_query("s_rpi_ctrl.sh --list-available-versions");
		if (line && strlen(line) > 0) {
			if (SERVER_INSTALL_PROPERTY) {
				indigo_delete_property(device, SERVER_INSTALL_PROPERTY, NULL);
				redefine = true;
			}
			pthread_mutex_lock(&install_property_mutex);
			if (redefine)
				indigo_release_property(SERVER_INSTALL_PROPERTY);
			SERVER_INSTALL_PROPERTY = indigo_init_switch_property(NULL, server_device.name, SERVER_INSTALL_PROPERTY_NAME, MAIN_GROUP, "Available versions", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 10);
			SERVER_INSTALL_PROPERTY->count = 0;
			char *pnt, *versions[10] = { strtok_r(line, " ", &pnt) };
			int count = 1;
			while ((versions[count] = strtok_r(NULL, " ", &pnt))) {
				if (count == 9)
					count = 1;
				else
					count++;
			}
			for (int i = 0; i < count; i++) {
				int smallest = 100000;
				int ii = 0;
				for (int j = 0; j < count; j++) {
					if (versions[j] == NULL)
						continue;
					char *build = strchr(versions[j], '-');
					if (build == NULL)
						continue;
					int build_number = atoi(build + 1);
					if (build_number < smallest) {
						smallest = build_number;
						ii = j;
					}
				}
				indigo_init_switch_item(SERVER_INSTALL_PROPERTY->items + i, versions[ii], versions[ii], versions[ii] == line);
				SERVER_INSTALL_PROPERTY->count++;
				versions[ii] = NULL;
			}
			pthread_mutex_unlock(&install_property_mutex);
			free(line);
			if (redefine)
				indigo_define_property(device, SERVER_INSTALL_PROPERTY, NULL);
		}
		indigo_usleep(10 * 60 * ONE_SECOND_DELAY);
	}
}

#endif

static indigo_result attach(indigo_device *device) {
	assert(device != NULL);
	SERVER_INFO_PROPERTY = indigo_init_text_property(NULL, server_device.name, SERVER_INFO_PROPERTY_NAME, MAIN_GROUP, "Server info", INDIGO_OK_STATE, INDIGO_RO_PERM, 2);
	indigo_init_text_item(SERVER_INFO_VERSION_ITEM, SERVER_INFO_VERSION_ITEM_NAME, "INDIGO version", "%d.%d-%s", INDIGO_VERSION_MAJOR(INDIGO_VERSION_CURRENT), INDIGO_VERSION_MINOR(INDIGO_VERSION_CURRENT), INDIGO_BUILD);
	indigo_init_text_item(SERVER_INFO_SERVICE_ITEM, SERVER_INFO_SERVICE_ITEM_NAME, "INDIGO service", "");
	SERVER_DRIVERS_PROPERTY = indigo_init_switch_property(NULL, server_device.name, SERVER_DRIVERS_PROPERTY_NAME, MAIN_GROUP, "Available drivers", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, INDIGO_MAX_DRIVERS);
	SERVER_DRIVERS_PROPERTY->count = 0;
	for (int i = 0; i < INDIGO_MAX_DRIVERS; i++)
		if (indigo_available_drivers[i].driver != NULL)
			indigo_init_switch_item(&SERVER_DRIVERS_PROPERTY->items[SERVER_DRIVERS_PROPERTY->count++], indigo_available_drivers[i].name, indigo_available_drivers[i].description, indigo_available_drivers[i].initialized);
	for (int i = 0; i < dynamic_drivers_count && SERVER_DRIVERS_PROPERTY->count < INDIGO_MAX_DRIVERS; i++)
		indigo_init_switch_item(&SERVER_DRIVERS_PROPERTY->items[SERVER_DRIVERS_PROPERTY->count++], dynamic_drivers[i].name, dynamic_drivers[i].description, false);
	indigo_property_sort_items(SERVER_DRIVERS_PROPERTY);
	SERVER_SERVERS_PROPERTY = indigo_init_light_property(NULL, server_device.name, SERVER_SERVERS_PROPERTY_NAME, MAIN_GROUP, "Configured servers", INDIGO_OK_STATE, 2 * INDIGO_MAX_SERVERS);
	SERVER_SERVERS_PROPERTY->count = 0;
	for (int i = 0; i < INDIGO_MAX_SERVERS; i++) {
		indigo_server_entry *entry = indigo_available_servers + i;
		if (*entry->host) {
			char buf[INDIGO_NAME_SIZE];
			if (entry->port == 7624)
				strncpy(buf, entry->host, sizeof(buf));
			else
				snprintf(buf, sizeof(buf), "%s:%d", entry->host, entry->port);
			indigo_init_light_item(&SERVER_SERVERS_PROPERTY->items[SERVER_SERVERS_PROPERTY->count++], buf, buf, INDIGO_OK_STATE);
		}
	}
	for (int i = 0; i < INDIGO_MAX_SERVERS; i++) {
		indigo_subprocess_entry *entry = indigo_available_subprocesses + i;
		if (*entry->executable) {
			indigo_init_light_item(&SERVER_SERVERS_PROPERTY->items[SERVER_SERVERS_PROPERTY->count++], entry->executable, entry->executable, INDIGO_OK_STATE);
		}
	}
	indigo_property_sort_items(SERVER_SERVERS_PROPERTY);
	SERVER_LOAD_PROPERTY = indigo_init_text_property(NULL, server_device.name, SERVER_LOAD_PROPERTY_NAME, MAIN_GROUP, "Load driver", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
	indigo_init_text_item(SERVER_LOAD_ITEM, SERVER_LOAD_ITEM_NAME, "Load driver", "");
	SERVER_UNLOAD_PROPERTY = indigo_init_text_property(NULL, server_device.name, SERVER_UNLOAD_PROPERTY_NAME, MAIN_GROUP, "Unload driver", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
	indigo_init_text_item(SERVER_UNLOAD_ITEM,SERVER_UNLOAD_ITEM_NAME, "Unload driver", "");
	SERVER_RESTART_PROPERTY = indigo_init_switch_property(NULL, server_device.name, SERVER_RESTART_PROPERTY_NAME, MAIN_GROUP, "Restart", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
	indigo_init_switch_item(SERVER_RESTART_ITEM, SERVER_RESTART_ITEM_NAME, "Restart server", false);
	SERVER_LOG_LEVEL_PROPERTY = indigo_init_switch_property(NULL, device->name, SERVER_LOG_LEVEL_PROPERTY_NAME, MAIN_GROUP, "Log level", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 4);
	indigo_init_switch_item(SERVER_LOG_LEVEL_ERROR_ITEM, SERVER_LOG_LEVEL_ERROR_ITEM_NAME, "Error", false);
	indigo_init_switch_item(SERVER_LOG_LEVEL_INFO_ITEM, SERVER_LOG_LEVEL_INFO_ITEM_NAME, "Info", false);
	indigo_init_switch_item(SERVER_LOG_LEVEL_DEBUG_ITEM, SERVER_LOG_LEVEL_DEBUG_ITEM_NAME, "Debug", false);
	indigo_init_switch_item(SERVER_LOG_LEVEL_TRACE_ITEM, SERVER_LOG_LEVEL_TRACE_ITEM_NAME, "Trace", false);
	SERVER_FEATURES_PROPERTY = indigo_init_switch_property(NULL, device->name, SERVER_FEATURES_PROPERTY_NAME, MAIN_GROUP, "Features", INDIGO_OK_STATE, INDIGO_RO_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
	indigo_init_switch_item(SERVER_BONJOUR_ITEM, SERVER_BONJOUR_ITEM_NAME, "Bonjour", use_bonjour);
	indigo_init_switch_item(SERVER_CTRL_PANEL_ITEM, SERVER_CTRL_PANEL_ITEM_NAME, "Control panel / Server manager", use_ctrl_panel);
	indigo_init_switch_item(SERVER_WEB_APPS_ITEM, SERVER_WEB_APPS_ITEM_NAME, "Web applications", use_web_apps);
#ifdef RPI_MANAGEMENT
	if (use_rpi_management) {
		char *line;
		SERVER_WIFI_AP_PROPERTY = indigo_init_text_property(NULL, server_device.name, SERVER_WIFI_AP_PROPERTY_NAME, MAIN_GROUP, "Configure access point WiFi mode", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		indigo_init_text_item(SERVER_WIFI_AP_SSID_ITEM, SERVER_WIFI_AP_SSID_ITEM_NAME, "Network name", "");
		indigo_init_text_item(SERVER_WIFI_AP_PASSWORD_ITEM, SERVER_WIFI_AP_PASSWORD_ITEM_NAME, "Password", "");
		line = execute_query("s_rpi_ctrl.sh --get-wifi-server");
		if (line) {
			char *pnt, *token = strtok_r(line, "\t", &pnt);
			if (token)
				strncpy(SERVER_WIFI_AP_SSID_ITEM->text.value, token, INDIGO_VALUE_SIZE);
			token = strtok_r(NULL, "\t", &pnt);
			if (token)
				strncpy(SERVER_WIFI_AP_PASSWORD_ITEM->text.value, token, INDIGO_VALUE_SIZE);
			free(line);
		}
		SERVER_WIFI_INFRASTRUCTURE_PROPERTY = indigo_init_text_property(NULL, server_device.name, SERVER_WIFI_INFRASTRUCTURE_PROPERTY_NAME, MAIN_GROUP, "Configure infrastructure WiFi mode", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		indigo_init_text_item(SERVER_WIFI_INFRASTRUCTURE_SSID_ITEM, SERVER_WIFI_INFRASTRUCTURE_SSID_ITEM_NAME, "SSID", "");
		indigo_init_text_item(SERVER_WIFI_INFRASTRUCTURE_PASSWORD_ITEM, SERVER_WIFI_INFRASTRUCTURE_PASSWORD_ITEM_NAME, "Password", "");
		line = execute_query("s_rpi_ctrl.sh --get-wifi-client");
		if (line) {
			char *pnt, *token = strtok_r(line, "\t", &pnt);
			if (token)
				strncpy(SERVER_WIFI_INFRASTRUCTURE_SSID_ITEM->text.value, token, INDIGO_VALUE_SIZE);
			free(line);
		}
		SERVER_INTERNET_SHARING_PROPERTY = indigo_init_switch_property(NULL, server_device.name, SERVER_INTERNET_SHARING_PROPERTY_NAME, MAIN_GROUP, "Internet sharing", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		indigo_init_switch_item(SERVER_INTERNET_SHARING_DISABLED_ITEM, SERVER_INTERNET_SHARING_DISABLED_ITEM_NAME, "Disabled", true);
		indigo_init_switch_item(SERVER_INTERNET_SHARING_ENABLED_ITEM, SERVER_INTERNET_SHARING_ENABLED_ITEM_NAME, "Enabled", false);
		line = execute_query("s_rpi_ctrl.sh --get-forwarding");
		if (line) {
			if (!strncmp(line, "1", 1)) {
				indigo_set_switch(SERVER_INTERNET_SHARING_PROPERTY, SERVER_INTERNET_SHARING_ENABLED_ITEM, true);
			}
			free(line);
		}
		SERVER_HOST_TIME_PROPERTY = indigo_init_text_property(NULL, server_device.name, SERVER_HOST_TIME_PROPERTY_NAME, MAIN_GROUP, "Set host time", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		indigo_init_text_item(SERVER_HOST_TIME_ITEM, SERVER_HOST_TIME_ITEM_NAME, "Host time", "");
		SERVER_SHUTDOWN_PROPERTY = indigo_init_switch_property(NULL, server_device.name, SERVER_SHUTDOWN_PROPERTY_NAME, MAIN_GROUP, "Shutdown host computer", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
		indigo_init_switch_item(SERVER_SHUTDOWN_ITEM, SERVER_SHUTDOWN_ITEM_NAME, "Shutdown", false);
		SERVER_REBOOT_PROPERTY = indigo_init_switch_property(NULL, server_device.name, SERVER_REBOOT_PROPERTY_NAME, MAIN_GROUP, "Reboot host computer", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
		indigo_init_switch_item(SERVER_REBOOT_ITEM, SERVER_REBOOT_ITEM_NAME, "Reboot", false);
		indigo_async((void *(*)(void *))check_versions, device);
	}
#endif /* RPI_MANAGEMENT */
	indigo_log_levels log_level = indigo_get_log_level();
	switch (log_level) {
		case INDIGO_LOG_ERROR:
			SERVER_LOG_LEVEL_ERROR_ITEM->sw.value = true;
			break;
		case INDIGO_LOG_INFO:
			SERVER_LOG_LEVEL_INFO_ITEM->sw.value = true;
			break;
		case INDIGO_LOG_DEBUG:
			SERVER_LOG_LEVEL_DEBUG_ITEM->sw.value = true;
			break;
		case INDIGO_LOG_TRACE:
			SERVER_LOG_LEVEL_TRACE_ITEM->sw.value = true;
			break;
	}
	if (!command_line_drivers)
		indigo_load_properties(device, false);
	INDIGO_LOG(indigo_log("%s attached", device->name));
	return INDIGO_OK;
}

static indigo_result enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	indigo_define_property(device, SERVER_INFO_PROPERTY, NULL);
	indigo_define_property(device, SERVER_DRIVERS_PROPERTY, NULL);
	if (SERVER_SERVERS_PROPERTY->count > 0)
		indigo_define_property(device, SERVER_SERVERS_PROPERTY, NULL);
	indigo_define_property(device, SERVER_LOAD_PROPERTY, NULL);
	indigo_define_property(device, SERVER_UNLOAD_PROPERTY, NULL);
	indigo_define_property(device, SERVER_RESTART_PROPERTY, NULL);
	indigo_define_property(device, SERVER_LOG_LEVEL_PROPERTY, NULL);
	indigo_define_property(device, SERVER_FEATURES_PROPERTY, NULL);
#ifdef RPI_MANAGEMENT
	if (use_rpi_management) {
		indigo_define_property(device, SERVER_WIFI_AP_PROPERTY, NULL);
		indigo_define_property(device, SERVER_WIFI_INFRASTRUCTURE_PROPERTY, NULL);
		indigo_define_property(device, SERVER_INTERNET_SHARING_PROPERTY, NULL);
		indigo_define_property(device, SERVER_HOST_TIME_PROPERTY, NULL);
		indigo_define_property(device, SERVER_SHUTDOWN_PROPERTY, NULL);
		indigo_define_property(device, SERVER_REBOOT_PROPERTY, NULL);
		indigo_define_property(device, SERVER_INSTALL_PROPERTY, NULL);
	}
#endif /* RPI_MANAGEMENT */
	return INDIGO_OK;
}

static indigo_result change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(property != NULL);
#ifdef RPI_MANAGEMENT
	pthread_mutex_lock(&install_property_mutex);
	if (indigo_property_match(SERVER_INSTALL_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- INSTALL
		indigo_property_copy_values(SERVER_INSTALL_PROPERTY, property, false);
		for (int i = 0; i < property->count; i++) {
			if (property->items[i].sw.value) {
				indigo_result result = execute_command(device, SERVER_INSTALL_PROPERTY, "s_rpi_ctrl.sh --install-version %s", property->items[i].name);
				pthread_mutex_unlock(&install_property_mutex);
				return result;
			}
		}
	}
	pthread_mutex_unlock(&install_property_mutex);
#endif /* RPI_MANAGEMENT */
	if (indigo_property_match(SERVER_DRIVERS_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- DRIVERS
		if (command_line_drivers && !strcmp(client->name, CONFIG_READER))
			return INDIGO_OK;
		indigo_property_copy_values(SERVER_DRIVERS_PROPERTY, property, false);
		for (int i = 0; i < SERVER_DRIVERS_PROPERTY->count; i++) {
			char *name = SERVER_DRIVERS_PROPERTY->items[i].name;
			indigo_driver_entry *driver = NULL;
			for (int j = 0; j < INDIGO_MAX_DRIVERS; j++) {
				if (!strcmp(indigo_available_drivers[j].name, name)) {
					driver = &indigo_available_drivers[j];
					break;
				}
			}
			if (SERVER_DRIVERS_PROPERTY->items[i].sw.value) {
				if (driver) {
					if (driver->dl_handle == NULL && !driver->initialized) {
						SERVER_DRIVERS_PROPERTY->items[i].sw.value = driver->initialized = driver->driver(INDIGO_DRIVER_INIT, NULL) == INDIGO_OK;
					} else if (driver->dl_handle != NULL && !driver->initialized) {
						SERVER_DRIVERS_PROPERTY->items[i].sw.value = driver->initialized = driver->driver(INDIGO_DRIVER_INIT, NULL) == INDIGO_OK;
						if (driver && !driver->initialized) indigo_remove_driver(driver);
					}
				} else {
					SERVER_DRIVERS_PROPERTY->items[i].sw.value = indigo_load_driver(name, true, &driver) == INDIGO_OK;
					if (driver && !driver->initialized) {
						indigo_send_message(device, "Driver %s failed to load", name);
						indigo_remove_driver(driver);
					}
				}
			} else if (driver) {
				indigo_result result = INDIGO_OK;
				if (driver->dl_handle) {
					result = indigo_remove_driver(driver);
					if (result != INDIGO_OK) {
						SERVER_DRIVERS_PROPERTY->items[i].sw.value = true;
					}
				} else if (driver->initialized) {
					result = driver->driver(INDIGO_DRIVER_SHUTDOWN, NULL);
					if (result != INDIGO_OK) {
						SERVER_DRIVERS_PROPERTY->items[i].sw.value = true;
					} else {
						driver->initialized = false;
					}
				}
				if (result != INDIGO_OK) {
					if (result == INDIGO_BUSY) {
						indigo_send_message(device, "Driver %s is in use, can't be unloaded", name);
					} else {
						indigo_send_message(device, "Driver %s failed to unload", name);
					}
				}
			}
		}
		SERVER_DRIVERS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, SERVER_DRIVERS_PROPERTY, NULL);
		int handle = 0;
		if (!command_line_drivers)
			indigo_save_property(device, &handle, SERVER_DRIVERS_PROPERTY);
		close(handle);
		return INDIGO_OK;
	} else if (indigo_property_match(SERVER_LOAD_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- LOAD
		indigo_property_copy_values(SERVER_LOAD_PROPERTY, property, false);
		if (*SERVER_LOAD_ITEM->text.value) {
			char *name = basename(SERVER_LOAD_ITEM->text.value);
			for (int i = 0; i < INDIGO_MAX_DRIVERS; i++)
				if (!strcmp(name, indigo_available_drivers[i].name)) {
					SERVER_LOAD_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_update_property(device, SERVER_LOAD_PROPERTY, "Driver %s (%s) is already loaded", name, indigo_available_drivers[i].description);
					return INDIGO_OK;
				}
			indigo_driver_entry *driver;
			if (indigo_load_driver(SERVER_LOAD_ITEM->text.value, true, &driver) == INDIGO_OK) {
				bool found = false;
				for (int i = 0; i < SERVER_DRIVERS_PROPERTY->count; i++) {
					if (!strcmp(SERVER_DRIVERS_PROPERTY->items[i].name, name)) {
						SERVER_DRIVERS_PROPERTY->items[i].sw.value = true;
						SERVER_DRIVERS_PROPERTY->state = INDIGO_OK_STATE;
						indigo_update_property(device, SERVER_DRIVERS_PROPERTY, NULL);
						found = true;
						break;
					}
				}
				if (!found) {
					indigo_delete_property(device, SERVER_DRIVERS_PROPERTY, NULL);
					indigo_init_switch_item(&SERVER_DRIVERS_PROPERTY->items[SERVER_DRIVERS_PROPERTY->count++], driver->name, driver->description, driver->initialized);
					SERVER_DRIVERS_PROPERTY->state = INDIGO_OK_STATE;
					indigo_define_property(device, SERVER_DRIVERS_PROPERTY, NULL);
				}
				SERVER_LOAD_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, SERVER_LOAD_PROPERTY, "Driver %s (%s) loaded", name, driver->description);
			} else {
				SERVER_LOAD_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, SERVER_LOAD_PROPERTY, indigo_last_message);
			}
		}
		return INDIGO_OK;
	} else if (indigo_property_match(SERVER_UNLOAD_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- UNLOAD
		indigo_property_copy_values(SERVER_UNLOAD_PROPERTY, property, false);
		if (*SERVER_UNLOAD_ITEM->text.value) {
			char *name = basename(SERVER_UNLOAD_ITEM->text.value);
			for (int i = 0; i < INDIGO_MAX_DRIVERS; i++)
				if (!strcmp(name, indigo_available_drivers[i].name)) {
					indigo_result result;
					if (indigo_available_drivers[i].dl_handle) {
						result = indigo_remove_driver(&indigo_available_drivers[i]);
					} else {
						result = indigo_available_drivers[i].driver(INDIGO_DRIVER_SHUTDOWN, NULL);
					}
					if (result == INDIGO_OK) {
						for (int j = 0; j < SERVER_DRIVERS_PROPERTY->count; j++) {
							if (!strcmp(SERVER_DRIVERS_PROPERTY->items[j].name, name)) {
								SERVER_DRIVERS_PROPERTY->items[j].sw.value = false;
								SERVER_DRIVERS_PROPERTY->state = INDIGO_OK_STATE;
								indigo_update_property(device, SERVER_DRIVERS_PROPERTY, NULL);
								break;
							}
						}
						SERVER_UNLOAD_PROPERTY->state = INDIGO_OK_STATE;
						indigo_update_property(device, SERVER_UNLOAD_PROPERTY, "Driver %s unloaded", name);
					} else if (result == INDIGO_BUSY) {
						SERVER_UNLOAD_PROPERTY->state = INDIGO_ALERT_STATE;
						indigo_update_property(device, SERVER_UNLOAD_PROPERTY, "Driver %s is in use, can't be unloaded", name);
					} else {
						SERVER_UNLOAD_PROPERTY->state = INDIGO_ALERT_STATE;
						indigo_update_property(device, SERVER_UNLOAD_PROPERTY, "Driver %s failed to unload", name);
					}
					return INDIGO_OK;
				}
			SERVER_UNLOAD_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, SERVER_UNLOAD_PROPERTY, "Driver %s is not loaded", name);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(SERVER_RESTART_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- RESTART
		indigo_property_copy_values(SERVER_RESTART_PROPERTY, property, false);
		if (SERVER_RESTART_ITEM->sw.value) {
			INDIGO_LOG(indigo_log("Restarting..."));
			indigo_server_shutdown();
			exit(0);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(SERVER_LOG_LEVEL_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- LOG_LEVEL
		indigo_property_copy_values(SERVER_LOG_LEVEL_PROPERTY, property, false);
		if (SERVER_LOG_LEVEL_ERROR_ITEM->sw.value) {
			indigo_set_log_level(INDIGO_LOG_ERROR);
		} else if (SERVER_LOG_LEVEL_INFO_ITEM->sw.value) {
			indigo_set_log_level(INDIGO_LOG_INFO);
		} else if (SERVER_LOG_LEVEL_ERROR_ITEM->sw.value) {
			indigo_set_log_level(INDIGO_LOG_DEBUG);
		} else if (SERVER_LOG_LEVEL_TRACE_ITEM->sw.value) {
			indigo_set_log_level(INDIGO_LOG_TRACE);
		}
		SERVER_LOG_LEVEL_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, SERVER_LOG_LEVEL_PROPERTY, NULL);
		return INDIGO_OK;
#ifdef RPI_MANAGEMENT
	} else if (indigo_property_match(SERVER_WIFI_AP_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- WIFI_AP
		indigo_property_copy_values(SERVER_WIFI_AP_PROPERTY, property, false);
		return execute_command(device, SERVER_WIFI_AP_PROPERTY, "s_rpi_ctrl.sh --set-wifi-server \"%s\" \"%s\"", SERVER_WIFI_AP_SSID_ITEM->text.value, SERVER_WIFI_AP_PASSWORD_ITEM->text.value);
	} else if (indigo_property_match(SERVER_WIFI_INFRASTRUCTURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- WIFI_INFRASTRUCTURE
		indigo_property_copy_values(SERVER_WIFI_INFRASTRUCTURE_PROPERTY, property, false);
		return execute_command(device, SERVER_WIFI_INFRASTRUCTURE_PROPERTY, "s_rpi_ctrl.sh --set-wifi-client \"%s\" \"%s\"", SERVER_WIFI_INFRASTRUCTURE_SSID_ITEM->text.value, SERVER_WIFI_INFRASTRUCTURE_PASSWORD_ITEM->text.value);
	} else if (indigo_property_match(SERVER_INTERNET_SHARING_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- INTERNET_SHARING
		indigo_property_copy_values(SERVER_INTERNET_SHARING_PROPERTY, property, false);
		if (SERVER_INTERNET_SHARING_ENABLED_ITEM->sw.value) { /* item[1] is enable forwarding, aka network sharing */
			indigo_send_message(device, "Internet sharing is potentially dangerous, everyone connected to your INDIGO Sky can access your network!");
			return execute_command(device, SERVER_INTERNET_SHARING_PROPERTY, "s_rpi_ctrl.sh --enable-forwarding");
		} else {
			return execute_command(device, SERVER_INTERNET_SHARING_PROPERTY, "s_rpi_ctrl.sh --disable-forwarding");
		}
	} else if (indigo_property_match(SERVER_HOST_TIME_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- HOST_TIME
		indigo_property_copy_values(SERVER_HOST_TIME_PROPERTY, property, false);
		execute_command(device, SERVER_HOST_TIME_PROPERTY, "s_rpi_ctrl.sh --set-date \"%s\"", SERVER_HOST_TIME_ITEM->text.value);
		if (SERVER_HOST_TIME_PROPERTY->state == INDIGO_OK_STATE) {
			indigo_delete_property(device, SERVER_HOST_TIME_PROPERTY, NULL);
			SERVER_HOST_TIME_PROPERTY->hidden = true;
		}
		return INDIGO_OK;
	} else if (indigo_property_match(SERVER_SHUTDOWN_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- SHUTDOWN
		indigo_property_copy_values(SERVER_SHUTDOWN_PROPERTY, property, false);
		return execute_command(device, SERVER_SHUTDOWN_PROPERTY, "s_rpi_ctrl.sh --poweroff");
	} else if (indigo_property_match(SERVER_REBOOT_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- REBOOT
		indigo_property_copy_values(SERVER_REBOOT_PROPERTY, property, false);
		return execute_command(device, SERVER_REBOOT_PROPERTY, "s_rpi_ctrl.sh --reboot");
#endif /* RPI_MANAGEMENT */
	// --------------------------------------------------------------------------------
	}
	return INDIGO_OK;
}


static indigo_result detach(indigo_device *device) {
	assert(device != NULL);
	indigo_delete_property(device, SERVER_INFO_PROPERTY, NULL);
	indigo_delete_property(device, SERVER_DRIVERS_PROPERTY, NULL);
	if (SERVER_SERVERS_PROPERTY->count > 0)
		indigo_delete_property(device, SERVER_SERVERS_PROPERTY, NULL);
	indigo_delete_property(device, SERVER_LOAD_PROPERTY, NULL);
	indigo_delete_property(device, SERVER_UNLOAD_PROPERTY, NULL);
	indigo_delete_property(device, SERVER_RESTART_PROPERTY, NULL);
	indigo_delete_property(device, SERVER_LOG_LEVEL_PROPERTY, NULL);
	indigo_delete_property(device, SERVER_FEATURES_PROPERTY, NULL);
#ifdef RPI_MANAGEMENT
	if (use_rpi_management) {
		indigo_delete_property(device, SERVER_WIFI_AP_PROPERTY, NULL);
		indigo_delete_property(device, SERVER_WIFI_INFRASTRUCTURE_PROPERTY, NULL);
		indigo_delete_property(device, SERVER_INTERNET_SHARING_PROPERTY, NULL);
		indigo_delete_property(device, SERVER_HOST_TIME_PROPERTY, NULL);
		indigo_delete_property(device, SERVER_SHUTDOWN_PROPERTY, NULL);
		indigo_delete_property(device, SERVER_REBOOT_PROPERTY, NULL);
		indigo_delete_property(device, SERVER_INSTALL_PROPERTY, NULL);
	}
#endif /* RPI_MANAGEMENT */
	indigo_release_property(SERVER_INFO_PROPERTY);
	indigo_release_property(SERVER_DRIVERS_PROPERTY);
	indigo_release_property(SERVER_SERVERS_PROPERTY);
	indigo_release_property(SERVER_LOAD_PROPERTY);
	indigo_release_property(SERVER_UNLOAD_PROPERTY);
	indigo_release_property(SERVER_RESTART_PROPERTY);
	indigo_release_property(SERVER_LOG_LEVEL_PROPERTY);
	indigo_release_property(SERVER_FEATURES_PROPERTY);
#ifdef RPI_MANAGEMENT
	indigo_release_property(SERVER_WIFI_AP_PROPERTY);
	indigo_release_property(SERVER_WIFI_INFRASTRUCTURE_PROPERTY);
	indigo_release_property(SERVER_INTERNET_SHARING_PROPERTY);
	indigo_release_property(SERVER_HOST_TIME_PROPERTY);
	indigo_release_property(SERVER_SHUTDOWN_PROPERTY);
	indigo_release_property(SERVER_REBOOT_PROPERTY);
	indigo_release_property(SERVER_INSTALL_PROPERTY);
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
						char *pnt, *token = strtok_r(line, ",", &pnt);
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
						token = strtok_r(NULL, ",", &pnt);
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
	indigo_log("INDIGO server %d.%d-%s built on %s %s", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF, INDIGO_BUILD, __DATE__, __TIME__);

	/* Make sure master token and ACL are loaded before drivers */
	for (int i = 1; i < server_argc; i++) {
		if ((!strcmp(server_argv[i], "-T") || !strcmp(server_argv[i], "--master-token")) && i < server_argc - 1) {
			indigo_set_master_token(indigo_string_to_token(server_argv[i + 1]));
			i++;
		} else if ((!strcmp(server_argv[i], "-a") || !strcmp(server_argv[i], "--acl-file")) && i < server_argc - 1) {
			indigo_load_device_tokens_from_file(server_argv[i + 1]);
			i++;
		}
	}
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
		} else if ((!strcmp(server_argv[i], "-T") || !strcmp(server_argv[i], "--master-token")) && i < server_argc - 1) {
			/* just skip it - handled above */
			i++;
		} else if ((!strcmp(server_argv[i], "-a") || !strcmp(server_argv[i], "--acl-file")) && i < server_argc - 1) {
			/* just skip it - handled above */
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
		static unsigned char guider_html[] = {
			#include "resource/guider.html.data"
		};
		indigo_server_add_resource("/guider.html", guider_html, sizeof(guider_html), "text/html");
		static unsigned char guider_png[] = {
			#include "resource/guider.png.data"
		};
		indigo_server_add_resource("/guider.png", guider_png, sizeof(guider_png), "image/png");
	}

	indigo_server_add_file_resource("/log", "indigo.log", "text/plain; charset=UTF-8");

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
	indigo_server_remove_resources();
	if (star_data)
		free(star_data);
	if (dso_data)
		free(dso_data);
	if (constellation_data)
		free(constellation_data);
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
	if (signo == SIGCHLD) {
		int status;
		while ((waitpid(-1, &status, WNOHANG)) > 0);
		return;
	}
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
			printf("INDIGO server v.%d.%d-%s built on %s %s.\n", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF, INDIGO_BUILD, __DATE__, __TIME__);
			printf("usage: %s [-h | --help]\n", argv[0]);
			printf("       %s [options] indigo_driver_name indigo_driver_name ...\n", argv[0]);
			printf("options:\n"
			       "       --  | --do-not-fork\n"
			       "       -l  | --use-syslog\n"
			       "       -p  | --port port                     (default: 7624)\n"
			       "       -b  | --bonjour name                  (default: hostname)\n"
			       "       -T  | --master-token token            (master token for devce access default: 0 = none)\n"
			       "       -a  | --acl-file file\n"
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
	signal(SIGCHLD, signal_handler);
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
					  indigo_usleep(2 * ONE_SECOND_DELAY);
				}
			}
		}
		INDIGO_LOG(indigo_log("Shutdown complete! See you!"));
	} else {
		server_main();
	}
}
